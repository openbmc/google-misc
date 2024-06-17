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

NCSI_IF="$1"

old_rtr=
old_mac=

function set_rtr() {
    if ! ip -6 route show | grep -q '^default'; then
        echo 'default route missing, reconfiguring...' >&2
        old_rtr=
        old_mac=
    fi
    [ "$rtr" != "$old_rtr" -a "$mac" != "$old_mac" ] || return

    echo "Setting default router: $rtr at $mac" >&2

    # Delete and static gateways and neighbors
    while read entry; do
        eval "$(echo "$entry" | JSONToVars)" || return
        echo "Deleting neighbor $object"
        DeleteObject "$service" "$object" || true
    done < <(GetNeighborObjects "$netdev" 2>/dev/null)

    busctl set-property xyz.openbmc_project.Network "$(EthObjRoot "$NCSI_IF")" \
        xyz.openbmc_project.Network.EthernetInterface DefaultGateway6 s "" || true

    # In case we don't have a base network file, make one
    net_file=/run/systemd/network/00-bmc-$NCSI_IF.network
    printf '[Match]\nName=%s\n[Network]\nDHCP=false\nIPv6AcceptRA=false\nLinkLocalAddressing=yes' \
        "$NCSI_IF" >$net_file

    # Override any existing gateway info
    mkdir -p $net_file.d
    printf '[Network]\nGateway=%s\n[Neighbor]\nMACAddress=%s\nAddress=%s' \
        "$rtr" "$mac" "$rtr" >$net_file.d/10-gateway.conf

    networkctl reload && networkctl reconfigure "$NCSI_IF" || true

    retries=-1
    deadline=$dl
    old_mac="$mac"
    old_rtr="$rtr"
}

retries=1
min_w=10
deadline=$min_w
while true; do
    curr_dl=$deadline
    args=(-m "$NCSI_IF" -w $(( (curr_dl - SECONDS) * 1000 )))
    if (( retries > 0 )); then
        args+=(-r "$retries")
    else
        args+=(-d)
    fi
    declare -A rtrs
    rtrs=()
    while read line; do
        # `script` terminates all lines with a CRLF, remove it
        line="${line:0:-1}"
        if [ -z "$line" ]; then
            lifetime=-1
            mac=
        elif [[ "$line" =~ ^Router' 'lifetime' '*:' '*([0-9]*) ]]; then
            lifetime="${BASH_REMATCH[1]}"
        elif [[ "$line" =~ ^Source' 'link-layer' 'address' '*:' '*([a-fA-F0-9:]*)$ ]]; then
            mac="${BASH_REMATCH[1]}"
        elif [[ "$line" =~ ^from' '(.*)$ ]]; then
            rtr="${BASH_REMATCH[1]}"
            # Only valid default routers can be considered, 0 lifetime implies
            # a non-default router
            if (( lifetime > 0 )); then
                dl=$((lifetime + SECONDS))
                rtrs["$rtr"]="$mac $dl"
                # We have some notoriously noisy lab environments with many routers being broadcast
                # We always prefer "fe80::1" in prod and labs for routing, so prefer that gateway.
                # We also want to take the first router we find to speed up acquisition on boot.
                if [ "$rtr" = "fe80::1" -o -z "$old_rtr" ]; then
                    set_rtr || true
                fi
            fi
            lifetime=-1
            mac=
        fi
    done < <(exec script -q -c "rdisc6 ${args[*]}" /dev/null 2>/dev/null)
    # Purge any expired routers
    for rtr in "${!rtrs[@]}"; do
        data=(${rtrs["$rtr"]})
        dl=${data[1]}
	if (( dl <= SECONDS )); then
            unset rtrs["$rtr"]
        fi
    done
    # Consider changing the gateway if the old one doesn't send RAs for the entire period
    # This ensures we don't flip flop between multiple defaults if they exist.
    if [ -z "${rtrs["$old_rtr"]-}" ]; then
        echo "Old router $old_rtr disappeared" >&2
        for rtr in "${!rtrs[@]}"; do
            data=(${rtrs["$rtr"]})
            mac=${data[0]}
            dl=${data[1]}
            set_rtr && break
        done
    fi

    # If rdisc6 exits early we still want to wait for the deadline before retrying
    (( timeout = curr_dl - SECONDS ))
    sleep $(( timeout < 0 ? 0 : timeout ))
done
