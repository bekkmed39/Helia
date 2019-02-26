#!/bin/sh

### sh scrambling.sh


PACKAGE="gst-plugins-bad"
VERSION="1.14.1"

DIR=mpegtsdemux

### Download & Configure

if [ -d $DIR ]; then

	cd $DIR
	echo "  make"

else

	if [ ! -d gst-plugins-bad-$VERSION ]; then

		if [ -f gst-plugins-bad-$VERSION.tar.xz ]; then
			tar -xvf gst-plugins-bad-$VERSION.tar.xz
		else
			echo "Download: gst-plugins-bad-$VERSION.tar.xz"
	
			wget https://gstreamer.freedesktop.org/src/gst-plugins-bad/gst-plugins-bad-$VERSION.tar.xz
			tar -xvf gst-plugins-bad-$VERSION.tar.xz
		fi

	fi

	cp -r gst-plugins-bad-$VERSION/gst/$DIR .

	rm -f $DIR/Makefile.am $DIR/Makefile.in $DIR/meson.build

	patch -p 1 < scrambling-$VERSION.diff

	cp src/Makefile src/systems.c src/systems.h $DIR/

	cd $DIR

fi

echo ""

make library=libgstmpegtsdemux

echo ""


### Inspect

make library=libgstmpegtsdemux inspect

