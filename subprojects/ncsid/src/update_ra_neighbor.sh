#!/bin/bash
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

source "$(dirname "${BASH_SOURCE[0]}")"/ncsid_lib.sh

UpdateRA() {
  local netdev="$1"

  local service='xyz.openbmc_project.Network'
  local gateways
  if ! gateways="$(GetGateways "$service" "$netdev")"; then
    echo "Failed to look up gateways" >&2
    return 1
  fi
  local vars
  vars="$(echo "$gateways" | JSONToVars)" || return
  eval "$vars" || return

  echo "GW($netdev): ${DefaultGateway6:-(none)}" >&2
  if [ -z "$DefaultGateway6" ]; then
    return 0
  fi

  local disc
  if ! disc="$(DiscoverRouter6 "$netdev" -1 360000 "$DefaultGateway6")"; then
    echo "Failed to discover router" >&2
    return 1
  fi
  local vars
  vars="$(echo "$disc" | JSONToVars)" || return
  eval "$vars" || return
  echo "GW($netdev) MAC: $router_mac" >&2

  SuppressTerm
  local rc=0
  UpdateNeighbor "$service" "$netdev" "$router_ip" "$router_mac" || rc=$?
  UnsuppressTerm
  return $rc
}

Main() {
  set -o nounset
  set -o errexit
  set -o pipefail

  InitTerm
  UpdateRA "$@"
}

return 0 2>/dev/null
Main "$@"
