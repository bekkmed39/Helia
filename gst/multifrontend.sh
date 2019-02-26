#!/bin/sh

### sh multifrontend.sh


PACKAGE="gst-plugins-bad"
VERSION="1.14.1"

DIR=dvb

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

	cp -r gst-plugins-bad-$VERSION/sys/$DIR .

	rm -f $DIR/Makefile.am $DIR/Makefile.in $DIR/meson.build

	patch -p 1 < multifrontend.diff

	cp src/Makefile $DIR/

	cd $DIR

	sed 's|include <gst/gst-i18n-plugin.h>|include <gst-i18n-plugin.h>|' -i gstdvb.c
	sed 's|GST_LICENSE|"LGPL"|;s|GST_PACKAGE_NAME|"GStreamer Bad Plugins"|' -i gstdvb.c
	sed 's|GST_PACKAGE_ORIGIN|"https://gstreamer.freedesktop.org/src/gst-plugins-bad"|' -i gstdvb.c
	sed 's|include <gst/gst-i18n-plugin.h>|include <gst-i18n-plugin.h>|' -i gstdvbsrc.c
	sed 's|include <gst/glib-compat-private.h>|include <glib-compat-private.h>|' -i gstdvbsrc.c
	sed 's|include <gst/gst-i18n-plugin.h>|include <gst-i18n-plugin.h>|' -i parsechannels.c
	sed 's|GST_API_VERSION|"1.0"|' -i parsechannels.c
fi

echo ""

make library=libgstdvb

echo ""


### Inspect

make library=libgstdvb inspect

