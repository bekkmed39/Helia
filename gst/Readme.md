## GstTSDemux - scrambling
#### Ver. 1.14.1 ( gst-plugins-bad )

* ... install libdvbcsa libdvbcsa-dev
* sh scrambling.sh
* path=`gst-inspect-1.0 tsdemux | grep Filename | sed 's|^.*/usr|/usr|;s|libgstmpegtsdemux.so||'`
* sudo cp mpegtsdemux/build/libgstmpegtsdemux.so $path
* cp SoftCam.Key $HOME/.config/helia/cam.key


## GstDvbSrc - multifrontend
#### Ver. 1.14.1 ( gst-plugins-bad )

* sh multifrontend.sh
* path=`gst-inspect-1.0 dvbsrc | grep Filename | sed 's|^.*/usr|/usr|;s|libgstdvb.so||'`
* sudo cp dvb/build/libgstdvb.so $path
