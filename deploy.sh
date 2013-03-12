#!/bin/bash

if [[ ! $1 ]] ; then
	echo "Must provide package name"
	exit 1
else
	package=$1
fi

if [[ ! $2 ]] ; then
	echo "Must provide hostname"
	exit 2
else
	host=$2
fi

root_pkg=$(tar -tzf $package | head -n 1)

cat $package | ssh -v -t $host "tar -xzv ; cd $root_pkg ; chmod -v +x ./install.sh ; ./install.sh "
