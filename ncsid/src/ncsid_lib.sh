# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Internal handler used for signalling child processes that they should
# terminate.
HandleTerm() {
  GOT_TERM=1
  if ShouldTerm && (( ${#CHILD_PIDS[@]} > 0 )); then
    kill "${!CHILD_PIDS[@]}"
  fi
}

# Sets up the signal handler and global variables needed to run interruptible
# services that can be killed gracefully.
InitTerm() {
  declare -g -A CHILD_PIDS=()
  declare -g GOT_TERM=0
  declare -g SUPPRESS_TERM=0
  trap HandleTerm TERM
}

# Used to suppress the handling of SIGTERM for critical components that should
# not respect SIGTERM. To finish suppressing, use UnsuppressTerm()
SuppressTerm() {
  SUPPRESS_TERM=$((SUPPRESS_TERM + 1))
}

# Stops suppressing SIGTERM for a single invocation of SuppresssTerm()
UnsuppressTerm() {
  SUPPRESS_TERM=$((SUPPRESS_TERM - 1))
}

# Determines if we got a SIGTERM and should respect it
ShouldTerm() {
  (( GOT_TERM == 1 && SUPPRESS_TERM == 0 ))
}

# Internal, ensures that functions called in a subprocess properly initialize
# their SIGTERM handling logic
RunInterruptibleFunction() {
  CHILD_PIDS=()
  trap HandleTerm TERM
  "$@"
}

# Runs the provided commandline in the background, and passes any received
# SIGTERMS to the child. Can be waited on using WaitInterruptibleBg
RunInterruptibleBg() {
  if ShouldTerm; then
    return 143
  fi
  if [ "$(type -t "$1")" = "function" ]; then
    RunInterruptibleFunction "$@" &
  else
    "$@" &
  fi
  CHILD_PIDS["$!"]=1
}

# Runs the provided commandline to completion, and passes any received
# SIGTERMS to the child.
RunInterruptible() {
  RunInterruptibleBg "$@" || return
  local child_pid="$!"
  wait "$child_pid" || true
  unset CHILD_PIDS["$child_pid"]
  wait "$child_pid"
}

# Waits until all of the RunInterruptibleBg() jobs have terminated
WaitInterruptibleBg() {
  local wait_on=("${!CHILD_PIDS[@]}")
  if (( ${#wait_on[@]} > 0 )); then
    wait "${wait_on[@]}" || true
    CHILD_PIDS=()
    local rc=0
    local id
    for id in "${wait_on[@]}"; do
      wait "$id" || rc=$?
    done
    return $rc
  fi
}

# Determines if an address could be a valid IPv4 address
# NOTE: this doesn't sanitize invalid IPv4 addresses
IsIPv4() {
  local ip="$1"

  [[ "$ip" =~ ^([0-9]{1,3}\.){3}[0-9]{1,3}$ ]]
}

# Takes lines of text from an application on stdin and parses out a single
# MAC address per line of input.
ParseMACFromLine() {
  sed -n 's,.*\(\([0-9a-fA-F]\{2\}:\)\{5\}[0-9a-fA-F]\{2\}\).*,\1,p'
}

# Looks up the MAC address of the IPv4 neighbor using ARP
DetermineNeighbor4() {
  local netdev="$1"
  local ip="$2"

  # Grep intentionally prevented from returning an error to preserve the error
  # value of arping
  RunInterruptible arping -f -c 5 -w 5 -I "$netdev" "$ip" | \
    { grep 'reply from' || true; } | ParseMACFromLine
}

# Looks up the MAC address of the IPv6 neighbor using ICMPv6 ND
DetermineNeighbor6() {
  local netdev="$1"
  local ip="$2"

  RunInterruptible ndisc6 -1 -r 5 -w 1000 -q "$ip" "$netdev"
}

# Looks up the MAC address of the neighbor regardless of type
DetermineNeighbor() {
  local netdev="$1"
  local ip="$2"

  if IsIPv4 "$ip"; then
    DetermineNeighbor4 "$netdev" "$ip"
  else
    DetermineNeighbor6 "$netdev" "$ip"
  fi
}

# Performs a mapper call to get the subroot for the object root
# with a maxdepth and list of required interfaces. Returns a streamed list
# of JSON objects that contain an { object, service }.
GetSubTree() {
  local root="$1"
  shift
  local max_depth="$1"
  shift

  busctl --json=short call \
      'xyz.openbmc_project.ObjectMapper' \
      '/xyz/openbmc_project/object_mapper' \
      'xyz.openbmc_project.ObjectMapper' \
      'GetSubTree' sias "$root" "$max_depth" "$#" "$@" | \
    jq -c '.data[0] | to_entries[] | { object: .key, service: (.value | keys[0]) }'
}

# Returns all of the properties for a DBus interface on an object as a JSON
# object where the keys are the property names
GetProperties() {
  local service="$1"
  local object="$2"
  local interface="$3"

  busctl --json=short call \
      "$service" \
      "$object" \
      'org.freedesktop.DBus.Properties' \
      'GetAll' s "$interface" | \
    jq -c '.data[0] | with_entries({ key, value: .value.data })'
}

# Returns the property for a DBus interface on an object
GetProperty() {
  local service="$1"
  local object="$2"
  local interface="$3"
  local property="$4"

  busctl --json=short call \
      "$service" \
      "$object" \
      'org.freedesktop.DBus.Properties' \
      'Get' ss "$interface" "$property" | \
    jq -r '.data[0].data'
}

# Deletes any OpenBMC DBus object from a service
DeleteObject() {
  local service="$1"
  local object="$2"

  busctl call \
    "$service" \
    "$object" \
    'xyz.openbmc_project.Object.Delete' \
    'Delete'
}

# Transforms the given JSON dictionary into bash local variable
# statements that can be directly evaluated by the interpreter
JSONToVars() {
  jq -r 'to_entries[] | @sh "local \(.key)=\(.value)"'
}

# Returns the DBus object root for the ethernet interface
EthObjRoot() {
  local netdev="$1"

  echo "/xyz/openbmc_project/network/$netdev"
}

# Returns the DBus object root for the static neighbors of an intrerface
StaticNeighborObjRoot() {
  local netdev="$1"

  echo "$(EthObjRoot "$netdev")/static_neighbor"
}

# Returns all of the neighbor { service, object } data for an interface as if
# a call to GetSubTree() was made
GetNeighborObjects() {
  local netdev="$1"

  GetSubTree "$(StaticNeighborObjRoot "$netdev")" 0 \
    'xyz.openbmc_project.Network.Neighbor'
}

# Returns the neighbor properties as a JSON object
GetNeighbor() {
  local service="$1"
  local object="$2"

  GetProperties "$service" "$object" 'xyz.openbmc_project.Network.Neighbor'
}

# Adds a static neighbor to the system network daemon
AddNeighbor() {
  local service="$1"
  local netdev="$2"
  local ip="$3"
  local mac="$4"

  busctl call \
    "$service" \
    "$(EthObjRoot "$netdev")" \
    'xyz.openbmc_project.Network.Neighbor.CreateStatic' \
    'Neighbor' ss "$ip" "$mac" >/dev/null
}

# Returns all of the IP { service, object } data for an interface as if
# a call to GetSubTree() was made
GetIPObjects() {
  local netdev="$1"

  GetSubTree "$(EthObjRoot "$netdev")" 0 \
    'xyz.openbmc_project.Network.IP'
}

# Returns the IP properties as a JSON object
GetIP() {
  local service="$1"
  local object="$2"

  GetProperties "$service" "$object" 'xyz.openbmc_project.Network.IP'
}

# Adds a static IP to the system network daemon
AddIP() {
  local service="$1"
  local netdev="$2"
  local ip="$3"
  local prefix="$4"

  local protocol='xyz.openbmc_project.Network.IP.Protocol.IPv4'
  if ! IsIPv4 "$ip"; then
    protocol='xyz.openbmc_project.Network.IP.Protocol.IPv6'
  fi

  busctl call \
    "$service" \
    "$(EthObjRoot "$netdev")" \
    'xyz.openbmc_project.Network.IP.Create' \
    'IP' ssys "$protocol" "$ip" "$prefix" '' >/dev/null
}

# Determines if two IP addresses have the same address family
# IE: Both are IPv4 or both are IPv6
MatchingAF() {
  local rc1=0 rc2=0
  IsIPv4 "$1" || rc1=$?
  IsIPv4 "$2" || rc2=$?
  (( rc1 == rc2 ))
}

# Checks to see if the machine has the provided IP address information
# already configured. If not, it deletes all of the information for that
# address family and adds the provided IP address.
UpdateIP() {
  local service="$1"
  local netdev="$2"
  local ip="$3"
  local prefix="$4"

  local should_add=1
  local delete_services=()
  local delete_objects=()
  local entry
  while read entry; do
    eval "$(echo "$entry" | JSONToVars)" || return $?
    eval "$(GetIP "$service" "$object" | JSONToVars)" || return $?
    if [ "$(normalize_ip "$Address")" = "$(normalize_ip "$ip")" ] && \
        [ "$PrefixLength" = "$prefix" ]; then
      should_add=0
    elif MatchingAF "$ip" "$Address"; then
      echo "Deleting spurious IP: $Address/$PrefixLength" >&2
      delete_services+=("$service")
      delete_objects+=("$object")
    fi
  done < <(GetIPObjects "$netdev")

  local i
  for (( i=0; i<${#delete_objects[@]}; ++i )); do
    DeleteObject "${delete_services[$i]}" "${delete_objects[$i]}" || return $?
  done

  if (( should_add == 0 )); then
    echo "Not adding IP: $ip/$prefix" >&2
  else
    echo "Adding IP: $ip/$prefix" >&2
    AddIP "$service" "$netdev" "$ip" "$prefix" || return $?
  fi
}

# Sets the system gateway property to the provided IP address if not already
# set to the current value.
UpdateGateway() {
  local service="$1"
  local ip="$2"

  local object='/xyz/openbmc_project/network/config'
  local interface='xyz.openbmc_project.Network.SystemConfiguration'
  local property='DefaultGateway'
  if ! IsIPv4 "$ip"; then
    property='DefaultGateway6'
  fi

  local current_ip
  current_ip="$(GetProperty "$service" "$object" "$interface" "$property")" || \
    return $?
  if [ -n "$current_ip" ] && \
      [ "$(normalize_ip "$ip")" = "$(normalize_ip "$current_ip")" ]; then
    echo "Not reconfiguring gateway: $ip" >&2
    return 0
  fi

  echo "Setting gateway: $ip" >&2
  busctl set-property "$service" "$object" "$interface" "$property" s "$ip"
}

# Checks to see if the machine has the provided neighbor information
# already configured. If not, it deletes all of the information for that
# address family and adds the provided neighbor entry.
UpdateNeighbor() {
  local service="$1"
  local netdev="$2"
  local ip="$3"
  local mac="$4"

  local should_add=1
  local delete_services=()
  local delete_objects=()
  local entry
  while read entry; do
    eval "$(echo "$entry" | JSONToVars)" || return $?
    eval "$(GetNeighbor "$service" "$object" | JSONToVars)" || return $?
    if [ "$(normalize_ip "$IPAddress")" = "$(normalize_ip "$ip")" ] && \
        [ "$(normalize_mac "$MACAddress")" = "$(normalize_mac "$mac")" ]; then
      should_add=0
    elif MatchingAF "$ip" "$IPAddress"; then
      echo "Deleting spurious neighbor: $IPAddress $MACAddress" >&2
      delete_services+=("$service")
      delete_objects+=("$object")
    fi
  done < <(GetNeighborObjects "$netdev" 2>/dev/null)

  local i
  for (( i=0; i<${#delete_objects[@]}; ++i )); do
    DeleteObject "${delete_services[$i]}" "${delete_objects[$i]}" || return $?
  done

  if (( should_add == 0 )); then
    echo "Not adding neighbor: $ip $mac" >&2
  else
    echo "Adding neighbor: $ip $mac" >&2
    AddNeighbor "$service" "$netdev" "$ip" "$mac" || return $?
  fi
}
