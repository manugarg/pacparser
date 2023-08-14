#!/usr/bin/env bash

pushd $(dirname $0) > /dev/null; script_dir=$PWD; popd > /dev/null

pactester=$script_dir/../src/pactester
pacfile=$script_dir/proxy.pac
testdata=$script_dir/testdata
library_path=$script_dir/../src
export DYLD_LIBRARY_PATH=$library_path:$DYLD_LIBRARY_PATH
export LD_LIBRARY_PATH=$library_path:$LD_LIBRARY_PATH

lib=$library_path/libpacparser.so.1

os_arch=$(uname -s | sed /\ /s//_/)
if [ "$os_arch" = "Darwin" ]; then
  lib=$library_path/libpacparser.1.dylib
fi

if test ! -f "$lib"; then
  echo "Test failed. pacparser library not found."
  exit 1
fi

while read line
  do
    comment="${line#*#}"
    line="${line%%#*}"
    line=${line%"${line##*[^[:space:]]}"}
    test -z "$line" && continue
    # If machine is not connected to the internet and a test requires internet
    # just skip that test.
    test ! -z $NO_INTERNET && \
      test "${comment/INTERNET_REQUIRED/}" != "${comment}" && \
      continue
    params=${line%%|*}
    expected_result=${line##*|}
    result=$($pactester -p $pacfile $params)
    if [ $? != 0 ]; then
      echo "pactester execution failed."
      echo "Command tried: $pactester -p $pacfile $params"
      echo "Running with debug mode on..."
      echo "DEBUG=1 $pactester -p $pacfile $params"
      DEBUG=1 $pactester -p $pacfile $params
      exit 1
    fi
    [ $DEBUG ] && echo "Test line: $line"
    [ $DEBUG ] && echo "Params: $params"
    if [ "$result" != "$expected_result" ]; then
      echo "Test failed: got \"$result\", expected \"$expected_result\""
      echo "Command tried: $pactester -p $pacfile $params"
      echo "Running with debug mode on..."
      echo "DEBUG=1 $pactester -p $pacfile $params"
      DEBUG=1 $pactester -p $pacfile $params
      exit 1;
    fi
  done < $testdata

echo "All tests were successful."
