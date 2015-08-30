#!/bin/sh -e
#
# This script creates a Mac package, that can be directly installed by
# Installer on Mac.

ver=$1
[ -z $ver ] && echo "Please specify package version." && exit

# $stage_dir is where we'll install our package files.
stage_dir=/tmp/pacparser_$RANDOM
sudo rm -rf /tmp/pacparser*
mkdir -p $stage_dir

if [ ! -e src/Makefile ]; then
  echo "Call this script from the root of the source directory"
  exit 1
fi

# Build pactester and pacparser library and install in $stage_dir
make -C src
DESTDIR=$stage_dir make -C src install
# Build python module and install it in $stage_dir
make -C src pymod
DESTDIR=$stage_dir make -C src install-pymod

sudo chown -R root:wheel ${stage_dir}

pkgbuild --root ${stage_dir} --identifier pacparser --version ${ver} pacparser.pkg

sudo rm -rf $stage_dir

# Build disk image
tmp_dir=/tmp/pacparser-$ver-$RANDOM
disk_image=pacparser-$ver.dmg
rm -rf $tmp_dir $disk_image
mkdir $tmp_dir
mv pacparser.pkg $tmp_dir
hdiutil create -srcfolder $tmp_dir pacparser-$ver.dmg
rm -rf $tmp_dir
