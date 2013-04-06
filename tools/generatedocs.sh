#!/bin/bash

tools_dir=$(dirname $0)
if [ "${tools_dir:0:1}" != "/" ]; then
  tools_dir=$PWD/$tools_dir
fi

docs_dir=$tools_dir/../docs
src_dir=$tools_dir/../src

tmpdir=$TMPDIR/pacparser_doxygen_temp_$$
mkdir -p $tmpdir

cd $tmpdir

cp $src_dir/pacparser.h .
doxygen $docs_dir/doxygen.config
if [ $? != 0 ]; then
  echo "Doxygen returned error. Not continuing."
  exit
fi

mkdir -p $docs_dir/html

# Fix HTMLs.
mv html/group__pacparser.html $docs_dir/html/pacparser.html
mv html/doxygen.css $docs_dir/html/
sed -i '' -e 's/group__pacparser.html//g' $docs_dir/html/pacparser.html
# Remove Doxygen logo.
sed -i '' -e '/doxygen\.png/s/^.*$/Doxygen/g' $docs_dir/html/pacparser.html

mkdir -p $docs_dir/man/man3
mv man/man3/* $docs_dir/man/man3/
# Fix man page.
sed -i '' -e 's/pacparser \\\-/pacparser/g' $docs_dir/man/man3/*.3
cd -

echo $tmpdir
# Cleanup temp dir
rm -rf $tmpdir
