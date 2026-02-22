#!/usr/bin/env bash

gnuradio-companion ~/tetra/telive/gnuradio-companion/receiver_udp/telive_1ch_simple_gr37_udp.grc &
xterm -e "cd ~/tetra/osmo-tetra-sq5bpf/src/ && ./receiver1udp 1" &
xterm -geom 203x60 -e "cd ~/tetra/telive/ && ./rxx" &
/bin/bash