## [Helia](https://github.com/vl-nix/Helia)

* Digital TV
  * DVB-T2/S2/C, ATSC, DTMB, ISDB
  * [Multifrontend](gst#gstdvbsrc---multifrontend)
  * [Scrambling](gst#gsttsdemux---scrambling)
* Media Player
  * Drag and Drop
    * files, folders, playlists - [M3U, M3U8](https://en.wikipedia.org/wiki/M3U)
  * Record IPTV


#### Requirements

* Graphical user interface - [Gtk+3](https://developer.gnome.org/gtk3)
* Audio & Video & Digital TV - [Gstreamer 1.0](https://gstreamer.freedesktop.org)
* [GNU Lesser General Public License](http://www.gnu.org/licenses/lgpl.html)


#### Depends

* libgtk 3.0 ( & dev )
* gstreamer 1.0 ( & dev )
* all gst-plugins 1.0 ( & dev )
* gst-libav
* gstreamer-tools ( or gstreamer-utils )


#### Build ( two variants )

##### 1. Makefile
  
  * make help
  * make
  * make install ( or sudo make install )
  
##### 2. Autogen

  * sh autogen.sh
  * sh configure ( or sh configure --prefix=PREFIX  )
  * make
  * make install ( or sudo make install )


#### [Preview](https://www.opendesktop.org/p/1267820/)
