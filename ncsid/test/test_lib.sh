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
