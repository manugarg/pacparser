#!/bin/bash
# Utilities to test the 'pactester' utility (and with it, indirectly,
# the 'pacparser' library).

set -e -u

die() {
  printf '%s: %s\n' "${0##*/}" "$*" >&2
  exit 2
}

export DYLD_LIBRARY_PATH=$(pwd):${DYLD_LIBRARY_PATH-}
export LD_LIBRARY_PATH=$(pwd):${LD_LIBRARY_PATH-}

#=== Helper variables and constants ===#

readonly PACTESTER=${PACTESTER-$(pwd)/pactester}
readonly PAC=pac.js.tmp
readonly OUT=stdout.tmp
readonly ERR=stderr.tmp

# Regular expressions to roughly match IPv4 and IPv6 addresses.
# Not very strict, but good enough.
readonly ip4_rx='([0-9]{1,3}\.){3}[0-9]{1,3}'
readonly ip6_rx='[a-f0-9:]+:[a-f0-9:]*'

# These IP-matching regexes are based on tests about the hostnames
# 'uberproxy4.l.google.com' and 'uberproxy6.l.google.com'; don't try
# to be more precise than this, all the tests will become overly
# brittle (been there, done that).
readonly up_ip4_rx='([0-9]{1,3}\.){3}129'
readonly up_ip6_rx='[a-f0-9:]+:81'

declare -i test_count=0

# This should be set to "FAIL" to declare a test failure.
global_result=PASS

# The client tests can append to this.
PACPARSER_COMMON_ARGS=(-u http://invalid)

#=== Early sanity check ===#

(set +e; ${PACTESTER} --help; test $? -eq 1) \
  || die "couldn't run 'pactester --help' as expected"

#=== Helper functions ===#

register_failure() {
  global_result=FAIL
  echo '!!! FAIL !!!'
  (set -x && cat ${PAC} && cat ${OUT} && cat ${ERR}) || die "unexpected error"
}

do_test_status() {
  (($# > 0)) || die "do_test_status(): missing expected exit_status argument"
  local expected_exit_status=${1}; shift

  let ++test_count
  echo === TEST ${test_count} ===
  cat >"${PAC}"

  local exit_status=0 test_ok=true
  declare -a args=("${PACPARSER_COMMON_ARGS[@]}" "$@" -p "${PAC}")
  (set -x && ${PACTESTER} "${args[@]}" >${OUT} 2>${ERR}) \
    || exit_status=$?
  [[ ${exit_status} -eq ${expected_exit_status} ]] || test_ok=false
  if [[ ${expected_exit_status} -eq 0 ]]; then
    [[ "$(<${OUT})" == OK ]] || test_ok=false
  fi
  ${test_ok} || register_failure
}

do_test_status_from_body() {
  local body=$(</dev/stdin)
  do_test_status "$@" <<<"$(
    printf '%s\n' 'function FindProxyForURL(host, url) {'
    printf '%s\n' "  ${body}"
    printf '%s\n' '  return "KO"'
    printf '%s\n' '}'
  )"
}

ok() {
  do_test_status_from_body 0 "$@";
}

ko() {
  local rx=${1}; shift
  do_test_status_from_body 1 "$@"
  (set -x && grep -E -e "${rx}" ${ERR}) || register_failure
}

do_test_truth() {
  (($# > 0)) || die "do_test_truth(): missing expected truth argument"
  case ${1} in
     true) local flip='';;
    false) local flip='!';;
        *) die "do_test_truth(): invalid truth argument '${1}'"
  esac
  shift
  local body=$(</dev/stdin)
  do_test_status_from_body 0 "$@" <<<"return ${flip}(${body}) ? 'OK' : 'KO'"
}

js_true() { do_test_truth true "$@"; }
js_false () { do_test_truth false "$@"; }

declare_test_results_and_exit() {
  echo "${global_result}"
  if [[ $global_result == PASS ]]; then
    exit 0
  else
    exit 1
  fi
}
