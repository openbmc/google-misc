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

UpdateRAGW() {
  local netdev="$1"

  while true; do
    local disc
    if ! disc="$(RunInterruptible DiscoverRouter6 "$1" -1 360000)"; then
      echo "Failed to discover router" >&2
      continue
    fi

    local vars
    vars="$(echo "$disc" | JSONToVars)" || return
    eval "$vars" || return
    [ -n "$stateful_address" ] || continue
    echo "GW($netdev) $router_ip MAC: $router_mac" >&2

    SuppressTerm
    local service='xyz.openbmc_project.Network'
    local rc=0
    UpdateGateway "$service" "$router_ip" && \
      UpdateNeighbor "$service" "$netdev" "$router_ip" "$router_mac" || rc=$?
    UnsuppressTerm
  done
}

Main() {
  set -o nounset
  set -o errexit
  set -o pipefail

  InitTerm
  UpdateRAGW "$@"
}

return 0 2>/dev/null
Main "$@"
