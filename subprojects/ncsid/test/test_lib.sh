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

# Compares two strings and prints out an error message if they are not equal
StrEq() {
  if [ "$1" != "$2" ]; then
    echo "${BASH_SOURCE[1]}:${BASH_LINENO[0]} Mismatched strings" >&2
    echo "  Expected: $2" >&2
    echo "  Got:      $1" >&2
    exit 1
  fi
}

TESTS=()

# Runs tests and emits output specified by the Test Anything Protocol
# https://testanything.org/
TestAnythingMain() {
  set -o nounset
  set -o errexit
  set -o pipefail

  echo "TAP version 13"
  echo "1..${#TESTS[@]}"

  local i
  for ((i=0; i <${#TESTS[@]}; ++i)); do
    local t="${TESTS[i]}"
    local tap_i=$((i + 1))
    if ! "$t"; then
      printf "not "
    fi
    printf "ok %d - %s\n" "$tap_i" "$t"
  done
}
