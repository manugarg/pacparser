#!/bin/bash -x

tools_dir=$(dirname $0)
if [ "$tools_dir" == "." ]; then
  tools_dir=$PWD
fi

cd ${tools_dir}/../docs
for file in *.{1,3}
do
  rman  -f  html  -r  '%s.%s.html'  $file > html/$file.html
done
cd -
