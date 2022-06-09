#!/bin/sh

FILE="/usr/share/btmanager/host-off-time-points.json"

if [[ -f "${FILE}" ]]; then
    t=$(cat /proc/uptime | cut -d' ' -f1)
    jq --arg endtime ${t} '."6"=$endtime' ${FILE}
else
    echo "${FILE} doesn't exist"
fi
