#!/bin/bash


if [ ! $1 ] ; then
	ARCH=$(uname -m)
	make
else
	ARCH=$1
	make $ARCH
fi

export DAEMON_NAME="boltsnapd"

DAEMON="${ARCH}/$DAEMON_NAME"
GENCGI="boltgen.cgi.py"
PUBLIC_HTML_DIR="public_html"

export GENCGI_RENAME="boltgen.cgi"
export INSTALL_SCRIPT="install.sh"

# first make the binaries
ec=$?
if [[ $ec -ne 0 ]] ; then
	echo "Make failed! Exit code $ec"
fi

package=${ARCH}_package

binaries=$(find . | grep "${ARCH}/bin")

cp -rv $PUBLIC_HTML_DIR $package/

mkdir -p $package/$PUBLIC_HTML_DIR/cgi-bin/bin
mkdir -p $package/boltsnap/bin

cp -v $binaries $package/boltsnap/bin
cp -v $DAEMON $package/
cp -v $GENCGI $package/$PUBLIC_HTML_DIR/cgi-bin/$GENCGI_RENAME
cp -v $INSTALL_SCRIPT $package/

tar -cv $package | gzip > $package.tgz
rm -rf $package
