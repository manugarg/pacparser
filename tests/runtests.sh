#!/bin/bash

pushd $(dirname $0) > /dev/null; script_dir=$PWD; popd > /dev/null

PACTESTER=$script_dir/../src/pactester
PACFILE=$script_dir/proxy.pac
TESTDATA=$script_dir/testdata
LIBRARY_PATH=$script_dir/../src
export DYLD_LIBRARY_PATH=$LIBRARY_PATH
export LD_LIBRARY_PATH=$LIBRARY_PATH

OS_ARCH=$(uname -s | sed /\ /s//_/)
if [ "$OS_ARCH" = "Linux" ]; then
  LIB=$LIBRARY_PATH/libpacparser.so.1
elif [ "$OS_ARCH" = "Darwin" ]; then
  LIB=$LIBRARY_PATH/libpacparser.1.dylib
fi

if test ! -f "$LIB"; then
  echo "Test failed. pacparser library not found."
  exit 1
fi

while read line
  do
    echo "$line" | grep -q "^#" && continue
    PARAMS=$(echo "$line"|cut -d"|" -f1)
    EXPECTED_RESULT=$(echo $line|cut -d"|" -f2)
    RESULT=$($PACTESTER -p $PACFILE $PARAMS)
    [ $DEBUG ] && echo "Test line: $line"
    [ $DEBUG ] && echo "Params: $PARAMS"
    if [ "$RESULT" != "$EXPECTED_RESULT" ]; then
      echo "Test failed: got \"$RESULT\", expected \"$EXPECTED_RESULT\""
      echo "Params were: $PARAMS"
      exit 1;
    fi
  done < $TESTDATA

echo "All tests were successful."
