Telive receivers with UDP transport and XMLRPC control, and helper files:

kill_wrapper - a script to run an application and kill it when  ^C is
pressed. takes the command to be ran as argument, 
for example ./kill_wrapper ./telive_6ch_gr37_udp_xmlrpc_headless.py

telive_1ch_gr37_udp_xmlrpc_headless.grc - headless 1 channel receiver grc
script

telive_1ch_gr37_udp_xmlrpc_headless.py -
telive_1ch_gr37_udp_xmlrpc_headless.grc compiled to a python script via
gnuradio companion

telive_1ch_gr37_udp_xmlrpc_headless_slow.grc - headless 1 channel receiver grc
script with 256ks/s sammple rate for low cpu machines

telive_1ch_gr37_udp_xmlrpc_headless_slow.py -
telive_1ch_gr37_udp_xmlrpc_headless_slow.grc  compiled to a python script via
gnuradio companion

telive_1ch_simple_gr37_slow_udp_xmlrpc.grc - 1 channel receiver grc with WX
gui and 256ks/s sammple rate for low cpu machines

telive_1ch_simple_gr37_udp_xmlrpc.grc - 1 channel receiver grc with WX
gui

telive_6ch_gr37_udp_xmlrpc.grc - 6 channel receiver grc with WX
gui

telive_6ch_gr37_udp_xmlrpc_headless.grc - headless 6 channel receiver grc
script 

telive_6ch_gr37_udp_xmlrpc_headless.py -
telive_6ch_gr37_udp_xmlrpc_headless.grc compiled to a python script via
gnuradio companion

