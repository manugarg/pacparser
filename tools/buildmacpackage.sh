#!/bin/sh
#
# This script creates a Mac package, that can be directly installed by
# Installer on Mac.

ver=$1
[ -z $ver ] && echo "Please specify package version." && exit
major_ver=${ver/.*.*/}

cd $(dirname $0); tools_dir=$PWD; cd -

stage_dir=/tmp/pacparser_$RANDOM

sudo rm -rf /tmp/pacparser*
mkdir -p ${stage_dir}/usr/{bin,lib}

install -m 555 src/libpacparser.${major_ver}.dylib ${stage_dir}/usr/lib
ln -sf libpacparser.${major_ver}.dylib ${stage_dir}/usr/lib/libpacparser.dylib
install -m 555 src/pactester ${stage_dir}/usr/bin

sudo chown -R root:wheel ${stage_dir}

/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker \
 -v -i com.manugarg -r ${stage_dir} -n ${ver} -t pacparser -m -o pacparser.pkg

cd -
