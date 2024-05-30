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
    [ -n "$rtr" -a -n "$lifetime" ] || return

    # Reconfigure gateway in case of anything goes wrong
    if ! ip -6 route show | grep -q '^default'; then
        echo 'default route missing, reconfiguring...' >&2
        old_rtr=
        old_mac=
    fi

    [ "$rtr" != "$old_rtr" -a "$mac" != "$old_mac" ] || return
    # Only valid default routers can be considered, 0 lifetime implies
    # a non-default router
    (( lifetime > 0 )) || return

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
    old_mac="$mac"
    old_rtr="$rtr"
}

retries=1
w=60
while true; do
    start=$SECONDS
    args=(-m "$NCSI_IF" -w $(( w * 1000 )))
    if (( retries > 0 )); then
        args+=(-r "$retries")
    else
        args+=(-d)
    fi
    while read line; do
        # `script` terminates all lines with a CRLF, remove it
        line="${line:0:-1}"
        if [ -z "$line" ]; then
            lifetime=
            mac=
        elif [[ "$line" =~ ^Router' 'lifetime' '*:' '*([0-9]*) ]]; then
            lifetime="${BASH_REMATCH[1]}"
        elif [[ "$line" =~ ^Source' 'link-layer' 'address' '*:' '*([a-fA-F0-9:]*)$ ]]; then
            mac="${BASH_REMATCH[1]}"
        elif [[ "$line" =~ ^from' '(.*)$ ]]; then
            rtr="${BASH_REMATCH[1]}"
            set_rtr || true
            lifetime=
            mac=
            rtr=
        fi
    done < <(exec script -q -c "rdisc6 ${args[*]}" /dev/null 2>/dev/null)
    # If rdisc6 exits early we still want to wait the full `w` time before
    # starting again.
    (( timeout = start + w - SECONDS ))
    sleep $(( timeout < 0 ? 0 : timeout ))
done
