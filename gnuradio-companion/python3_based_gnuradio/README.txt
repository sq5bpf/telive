These are the files that support gnuradio 3.10, and maybe other python 3 based
versions (but this is untested).

This is provided so that people can easily use distributions which ship with
newer gnuradio (like debian 12, which i'm using).
If you want stability, somehow install gnuradio 3.7 and use the flowgraphs in
the python2_based_gnuradio directory.


### 1 channel headless receiver using the rtl-sdr dongle:
telive_1ch_gr310_udp_xmlrpc_headless.grc
telive_1ch_gr310_udp_xmlrpc_headless.py

### 6 channel headless receiver using the rtl-sdr dongle:
telive_6ch_gr310_udp_xmlrpc_headless.grc
telive_6ch_gr310_udp_xmlrpc_headless.py


### 1 channel receiver with QT gui using the rtl-sdr dongle:
telive_1ch_simple_gr310_udp_xmlrpc.grc
telive_1ch_simple_gr310_udp_xmlrpc.py

Please note that the QT gui in gnuradio 3.10 has bugs which cause the receiver
to crash when it is operated via the xmlrpc interface (gnuradio bug #6766).
Also the QT gui is not as nice looking and functional as the WX gui interface
(which was unfortunately removed in gnuradio 3.8). 


