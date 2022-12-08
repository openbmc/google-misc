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

function UpdateNeighbor() {
    local netdev="$1"
    local service="$2"
    local object="$3"

    eval "$(GetNeighbor "$service" "$object" | JSONToVars)" || return $?
    local new_mac
    if ! new_mac="$(DetermineNeighbor "$netdev" "$IPAddress")"; then
        echo "Faild to find $IPAddress" >&2
        return 10
    fi
    new_mac=$(normalize_mac "$new_mac") || return
    if [ "$new_mac" != "$(normalize_mac "$MACAddress")" ]; then
        echo "Updating $IPAddress: $MACAddress -> $new_mac" >&2
        SuppressTerm
        local rc=0
        DeleteObject "$service" "$object" && \
            AddNeighbor "$service" "$netdev" "$IPAddress" "$new_mac" || \
            rc=$?
        UnsuppressTerm
        return $rc
    fi
}

function UpdateNeighbors() {
    local netdev="$1"

    local entry
    while read entry; do
        eval "$(echo "$entry" | JSONToVars)"
        RunInterruptibleBg UpdateNeighbor "$netdev" "$service" "$object"
    done < <(GetNeighborObjects "$netdev" 2>/dev/null)
    WaitInterruptibleBg
}

function Main() {
    set -o nounset
    set -o errexit
    set -o pipefail

    InitTerm
    UpdateNeighbors "$@"
}

return 0 2>/dev/null
Main "$@"
