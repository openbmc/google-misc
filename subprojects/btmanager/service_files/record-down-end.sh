#!/bin/sh

DIR="/usr/share/btmanager/"
FILE=${DIR}"resume.json"
TEMP_FILE=${DIR}"temp.json"

if [ ! -f "${FILE}" ]; then
    echo "${FILE} doesn't exist, create it."
    echo "{}" > ${FILE}
fi

# /proc/uptime store uptime in second, change to millisecond
UPTIME=$(< "/proc/uptime" cut -d' ' -f1 | sed 's/\.//g')
UPTIME="${UPTIME}0"
# Definition here follows BTDefinition.hpp
OS_KERNEl_DOWN_END_TIME_POINT=5
BMC_DOWN_END_TIME_POINT=6
# Perform the process only if CurrentTimePoint == kOSKernelDownEnd
current=$(jq '.Runtime.CurrentTimePoint' ${FILE})
if [ "${current}" -ne "${OS_KERNEl_DOWN_END_TIME_POINT}" ]; then
    echo 'Time point is not kOSKernelDownEnd. Do nothing.'
    exit 1
fi
# Add time point
jq --arg key ${BMC_DOWN_END_TIME_POINT} --arg value ${UPTIME} '.TimePoint += {"6": $value|tonumber}' ${FILE} > ${TEMP_FILE}
# Change CurrentTimePoint
jq --arg value ${BMC_DOWN_END_TIME_POINT} '.Runtime += {"CurrentTimePoint": $value|tonumber}' ${TEMP_FILE} > ${FILE}
