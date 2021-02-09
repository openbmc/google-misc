#!/bin/bash
TEST_DIR="$(dirname "${BASH_SOURCE[0]}")"
source "$TEST_DIR"/test_lib.sh

TestNormalizeMACInvalidArgs() {
  ! "$NORMALIZE_MAC"
  ! "$NORMALIZE_MAC" '0:0:0:0:0:0' 'extra'
}

TestNormalizeMACBadMAC() {
  ! "$NORMALIZE_MAC" '0:0'
  ! "$NORMALIZE_MAC" '0:0:0:0:0:0:0'
  ! "$NORMALIZE_MAC" '1ff:0:0:0:0'
}

TestNormalizeMACSuccess() {
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
