library		= libgstmpegtsdemux

CC			= gcc
COMMON		= -Wall

INCLUDES	= -I../include
OPTIMZ		= -O2
# DEBUG		= -g -ggdb

DEFS		= -DPACKAGE=\"gst-plugins-bad\" -DVERSION=\"1.14.1\" -DGETTEXT_PACKAGE=\"gst-plugins-bad-1.0\" -DGST_USE_UNSTABLE_API

LIBS_PKG	= gstreamer-1.0

CFLADD		= $(COMMON) $(INCLUDES) $(OPTIMZ) $(DEBUG)
CFLAGS		= $(CFLADD) $(shell pkg-config --cflags $(LIBS_PKG))

LDADD		= -lgstreamer-1.0 -lgstbase-1.0 -lgstmpegts-1.0 -lgstcodecparsers-1.0 -lgsttag-1.0 -lgstpbutils-1.0 -lgstaudio-1.0 -lm -ldvbcsa
LDFLAGS		= $(shell pkg-config --libs $(LIBS_PKG)) $(LDADD)

sourcedir	= src
builddir	= build
buildsrcdir	= $(builddir)/src

sources		= $(wildcard *.c)
objects		= $(patsubst %.c,$(buildsrcdir)/%.o,$(sources))

libshared	= $(builddir)/$(library).so


all: dirs $(libshared)

dirs:
	@mkdir -p $(builddir) $(buildsrcdir)

$(libshared): $(objects) 
	@echo
	@echo '  CCLD  shared  ' $@
	@$(CC) -shared $^ -o $@ $(LDFLAGS)

$(objects): $(buildsrcdir)/%.o : %.c
	@echo '  CC	 ' $<
	@$(CC) -fPIC $(DEFS) $(CFLAGS) -c $< -o $@ 

clean:
	rm -fr $(builddir)

inspect:
	gst-inspect-1.0 $(libshared)

