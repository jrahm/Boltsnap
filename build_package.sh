#!/bin/bash
set -e

export BUILD_DIR=$(realpath ./build)
export DAEMON_NAME="boltsnapd"
export SOURCE_DIR="src"

if [ ! $1 ] ; then
	ARCH=$(uname -m)
	make $@
else
	ARCH=$1
	make $@
fi

DAEMON="$BUILD_DIR/${ARCH}/$DAEMON_NAME"

GENCGI="$SOURCE_DIR/boltgen.cgi.py"
PUBLIC_HTML_DIR="$SOURCE_DIR/public_html"

export GENCGI_RENAME="boltgen.cgi"
export INSTALL_SCRIPT="install.sh"

# first make the binaries
ec=$?
if [[ $ec -ne 0 ]] ; then
	echo "Make failed! Exit code $ec"
fi

package=$BUILD_DIR/${ARCH}_package
export BUILD_BIN_DIR="$package/boltsnap/bin"
export BUILD_PUBLIC_HTML="$package/public_html"

binaries=$(find $BUILD_DIR/$ARCH | grep "bin")

mkdir -p $package
cp -rv $PUBLIC_HTML_DIR $BUILD_PUBLIC_HTML

mkdir -p $BUILD_PUBLIC_HTML/cgi-bin/bin
mkdir -p $BUILD_BIN_DIR

cp -v $binaries $BUILD_BIN_DIR || true
cp -v $DAEMON $package/
cp -v $GENCGI $BUILD_PUBLIC_HTML/cgi-bin/$GENCGI_RENAME
cp -v $INSTALL_SCRIPT $package/

tar -cv $package | gzip > $package.tgz
rm -rf $package
