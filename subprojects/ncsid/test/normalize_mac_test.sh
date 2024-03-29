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

TEST_DIR="$(dirname "${BASH_SOURCE[0]}")"
source "$TEST_DIR"/test_lib.sh

function TestNormalizeMACInvalidArgs() {
    ! "$NORMALIZE_MAC"
    ! "$NORMALIZE_MAC" '0:0:0:0:0:0' 'extra'
}

function TestNormalizeMACBadMAC() {
    ! "$NORMALIZE_MAC" '0:0'
    ! "$NORMALIZE_MAC" '0:0:0:0:0:0:0'
    ! "$NORMALIZE_MAC" '1ff:0:0:0:0'
}

function TestNormalizeMACSuccess() {
    StrEq "$("$NORMALIZE_MAC" '0:0:0:0:0:0')" '00:00:00:00:00:00'
    StrEq "$("$NORMALIZE_MAC" 'ff:0f:0:0:11:1')" 'ff:0f:00:00:11:01'
    StrEq "$("$NORMALIZE_MAC" '0:0:0:0:0:ff')" "$("$NORMALIZE_MAC" '0:0:0:0:0:FF')"
}

TESTS+=(
    TestNormalizeMACInvalidArgs
    TestNormalizeMACBadMAC
    TestNormalizeMACSuccess
)

return 0 2>/dev/null
TestAnythingMain
