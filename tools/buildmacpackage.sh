#!/bin/sh
#
# This script creates a Mac package, that can be directly installed by
# Installer on Mac.

ver=$1
[ -z $ver ] && echo "Please specify package version." && exit
major_ver=${ver/.*.*/}

# $stage_dir is where we'll install our package files.
stage_dir=/tmp/pacparser_$RANDOM
sudo rm -rf /tmp/pacparser*
mkdir -p $stage_dir

cd $(dirname $0); tools_dir=$PWD; cd -
cd $tools_dir/..

# Build pactester and pacparser library and install in $stage_dir
make -C src
DESTDIR=$stage_dir make -C src install
# Build python module and install it in $stage_dir
make -C src pymod
DESTDIR=$stage_dir make -C src install-pymod

sudo chown -R root:wheel ${stage_dir}
/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker \
 -v -i com.manugarg -r ${stage_dir} -n ${ver} -t pacparser -m -o pacparser.pkg

cd -
