Telive from it's begining used gnuradio 3.6, and later 3.7. These versions use
python 2.x. 

Gnuradio 3.8 and later versions started using python 3, and had other changes
(such as dropping the WX gui). This caused all gnuradio flowgraphs and python 
code to be incompatible with the newer versions. 

Newer linux distributions started including the new python 3 based gnuradio.
For example this happened in Debian starting with version 11.

While it is possible to run old gnuradio versions on a modern system, it is
getting increasingly difficult. So the tools were put into two main
directories: python2_based_gnuradio and python3_based_gnuradio.


#### Python 2.x based gnuradio

The python2_based_gnuradio directory contains the original telive tools. For
now (July 2023) most of the references on the internet will use tools in this
directory.

They are split into these directories:

receiver_pipe - grc flowgraphs with unix pipe tansport

receiver_udp - grc flowgraphs with udp  transport

receiver_xmlrpc - grc flowgraphs with udp  transport and xmlrpc interface for
receiver control

sdrplay - grc flowgraphs for the SDRplay receivers (currently i RSP-1A only,
because i only have this RX), uses gr_sdrplay


#### Python 3.x based gnuradio

The python3_based_gnuradio directory contains flowgraphs which were developed
on gnuradio 3.10 in debian 12 (earlier gnuradio might also work, but it wasn't
tested and will not be supported). 

Flowgraphs which use unix pipe transport, or which don't have the xmlrpc
server enabled have been dropped. This reduces the number of versions
considerably, so there is no need to make further subdirectories.



