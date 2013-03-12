#!/bin/bash

if [[ ! $DAEMON_NAME ]] ; then
	DAEMON_NAME="boltsnapd"
fi

if [[ ! $GENCGI_RENAME ]] ; then
	GENCGI_RENAME="boltgen.cgi"
fi

sudo cp -rv boltsnap /usr/lib
cp -rvf public_html $HOME/

for i in $(ls /usr/lib/boltsnap/bin) ; do
	fullpath=/usr/lib/boltsnap/bin/$i
	echo "sudo ln -sf $fullpath /usr/bin/"

	sudo rm -fv /usr/bin/$i
	sudo ln -sfv $fullpath /usr/bin/$i

	rm -fv $HOME/public_html/cgi-bin/bin/$i
	ln -svf $fullpath $HOME/public_html/cgi-bin/bin/
done
