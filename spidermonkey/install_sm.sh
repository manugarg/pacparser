#!/bin/bash -ex
#
# $Id$
# Copyright 2009 Google Inc. All Rights Reserved.
# Author: manugarg@google.com (Manu Garg)

function install_stuff
{
  if [ $(id -u) == 0 ]; then
    find . -name "libjs.so" -exec install -m 644 {} /usr/lib/ \;
    mkdir -p /usr/include/js
    install -m 644 js/src/*.h /usr/include/js/
    exit 0
  else
    echo "[Warning] Not continuing. You will need root credentials to install"\
         "SM library. Please run this script again with sudo."
    exit 1
  fi
}

cd $(dirname $0)

# If it's already built,
if find . -name "libjs.so" | grep libjs.so; then
  install_stuff
fi

if [ ! -e js*.tar.gz ];
then
  echo -e "[Warning] SpiderMonkey source code tarball not found. Trying to"\
          "download from mozilla website.\n"
  if ! wget http://ftp.mozilla.org/pub/mozilla.org/js/js-1.7.0.tar.gz;
  then
    echo -e "[FATAL] Could not download SM source code. Please download it"\
            "yourself, and place it in '$(dirname $0)/' directory.\n"
    cd -; exit
  fi
fi

if [ ! -d js/src ]; then
  tar xzf js*.tar.gz
fi

echo -e "[Note] Compiling SM...\n"
sleep 1
cd js/src; make -f Makefile.ref

if find . -name "libjs.so" | grep libjs.so; then
  echo -e "\nCompiled successfully.\n"
  # Install now.
  install_stuff
fi
