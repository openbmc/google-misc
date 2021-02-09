#!/bin/bash
TEST_DIR="$(dirname "${BASH_SOURCE[0]}")"
source "$TEST_DIR"/test_lib.sh

TestNormalizeIPInvalidArgs() {
  ! "$NORMALIZE_IP"
  ! "$NORMALIZE_IP" '192.168.10.1' 'extra'
}

TestNormalizeIPBadIP() {
  ! "$NORMALIZE_IP" '0f0.100.595.444'
  ! "$NORMALIZE_IP" 'fx80::1'
}

TestNormalizeIPv4() {
  StrEq "$("$NORMALIZE_IP" '192.168.10.1')" '192.168.10.1'
  StrEq "$("$NORMALIZE_IP" '1.1.1.1')" '1.1.1.1'
}

TestNormalizeIPv6() {
  StrEq "$("$NORMALIZE_IP" 'fe80:00B1::0000:1')" 'fe80:b1::1'
}

TESTS+=(
  TestNormalizeIPInvalidArgs
  TestNormalizeIPBadIP
  TestNormalizeIPv4
  TestNormalizeIPv6
)

return 0 2>/dev/null
TestAnythingMain
