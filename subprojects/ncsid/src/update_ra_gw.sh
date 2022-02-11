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

set_rtr() {
  [ -n "$rtr" -a -n "$lifetime" ] || return
  [ "$rtr" != "$old_rtr" -a "$mac" != "$old_mac" ] || return
  # Only valid default routers can be considered, 0 lifetime implies
  # a non-default router
  (( lifetime > 0 )) || return

  echo "Setting default router: $rtr at $mac" >&2
  UpdateGateway xyz.openbmc_project.Network "$NCSI_IF" "$rtr" || return
  UpdateNeighbor xyz.openbmc_project.Network "$NCSI_IF" "$rtr" "$mac" || return

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
  done < <(exec rdisc6 "${args[@]}" 2>/dev/null)
  # If rdisc6 exits early we still want to wait the full `w` time before
  # starting again.
  (( timeout = start + w - SECONDS ))
  sleep $(( timeout < 0 ? 0 : timeout ))
done
