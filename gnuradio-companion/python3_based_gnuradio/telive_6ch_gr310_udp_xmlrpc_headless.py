#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: SQ5BPF Tetra live receiver 6ch UDP HEADLESS for
# Author: Jacek Lipkowski SQ5BPF
# Description: 6 channel receiver for Telive, with XMLRPC server and UDP output
# GNU Radio version: 3.10.5.1

from gnuradio import analog
from gnuradio import filter
from gnuradio.filter import firdes
from gnuradio import gr
from gnuradio.fft import window
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import network
from xmlrpc.server import SimpleXMLRPCServer
import threading
import osmosdr
import time




class telive_6ch_gr310_udp_xmlrpc_headless(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "SQ5BPF Tetra live receiver 6ch UDP HEADLESS for", catch_exceptions=True)

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 2400000
        self.first_decim = first_decim = 32
        self.xlate_offset_fine6 = xlate_offset_fine6 = 0
        self.xlate_offset_fine5 = xlate_offset_fine5 = 0
        self.xlate_offset_fine4 = xlate_offset_fine4 = 0
        self.xlate_offset_fine3 = xlate_offset_fine3 = 0
        self.xlate_offset_fine2 = xlate_offset_fine2 = 0
        self.xlate_offset_fine1 = xlate_offset_fine1 = 0
        self.xlate_offset6 = xlate_offset6 = 500e3
        self.xlate_offset5 = xlate_offset5 = 500e3
        self.xlate_offset4 = xlate_offset4 = 500e3
        self.xlate_offset3 = xlate_offset3 = 500e3
        self.xlate_offset2 = xlate_offset2 = 500e3
        self.xlate_offset1 = xlate_offset1 = 500e3
        self.udp_packet_size = udp_packet_size = 1472
        self.udp_dest_addr = udp_dest_addr = "127.0.0.1"
        self.telive_receiver_name = telive_receiver_name = 'SQ5BPF 6-channel headless rx for telive'
        self.telive_receiver_channels = telive_receiver_channels = 6
        self.sdr_ifgain = sdr_ifgain = 20
        self.sdr_gain = sdr_gain = 38
        self.ppm_corr = ppm_corr = 0
        self.out_sample_rate = out_sample_rate = 36000
        self.options_low_pass = options_low_pass = 12500
        self.if_samp_rate = if_samp_rate = samp_rate/first_decim
        self.freq = freq = 435e6
        self.first_port = first_port = 42000

        ##################################################
        # Blocks
        ##################################################

        self.xmlrpc_server_0 = SimpleXMLRPCServer(('0.0.0.0', first_port), allow_none=True)
        self.xmlrpc_server_0.register_instance(self)
        self.xmlrpc_server_0_thread = threading.Thread(target=self.xmlrpc_server_0.serve_forever)
        self.xmlrpc_server_0_thread.daemon = True
        self.xmlrpc_server_0_thread.start()
        self.osmosdr_source_0 = osmosdr.source(
            args="numchan=" + str(1) + " " + ''
        )
        self.osmosdr_source_0.set_time_unknown_pps(osmosdr.time_spec_t())
        self.osmosdr_source_0.set_sample_rate(samp_rate)
        self.osmosdr_source_0.set_center_freq(freq, 0)
        self.osmosdr_source_0.set_freq_corr(ppm_corr, 0)
        self.osmosdr_source_0.set_dc_offset_mode(0, 0)
        self.osmosdr_source_0.set_iq_balance_mode(0, 0)
        self.osmosdr_source_0.set_gain_mode(False, 0)
        self.osmosdr_source_0.set_gain(sdr_gain, 0)
        self.osmosdr_source_0.set_if_gain(sdr_ifgain, 0)
        self.osmosdr_source_0.set_bb_gain(20, 0)
        self.osmosdr_source_0.set_antenna('', 0)
        self.osmosdr_source_0.set_bandwidth(0, 0)
        self.network_udp_sink_0_4 = network.udp_sink(gr.sizeof_gr_complex, 1, udp_dest_addr, (first_port+6), 0, udp_packet_size, False)
        self.network_udp_sink_0_3 = network.udp_sink(gr.sizeof_gr_complex, 1, udp_dest_addr, (first_port+5), 0, udp_packet_size, False)
        self.network_udp_sink_0_2 = network.udp_sink(gr.sizeof_gr_complex, 1, udp_dest_addr, (first_port+4), 0, udp_packet_size, False)
        self.network_udp_sink_0_1 = network.udp_sink(gr.sizeof_gr_complex, 1, udp_dest_addr, (first_port+3), 0, udp_packet_size, False)
        self.network_udp_sink_0_0 = network.udp_sink(gr.sizeof_gr_complex, 1, udp_dest_addr, (first_port+2), 0, udp_packet_size, False)
        self.network_udp_sink_0 = network.udp_sink(gr.sizeof_gr_complex, 1, udp_dest_addr, (first_port+1), 0, udp_packet_size, False)
        self.mmse_resampler_xx_0_4 = filter.mmse_resampler_cc(0, (float(float(if_samp_rate)/float(out_sample_rate))))
        self.mmse_resampler_xx_0_3 = filter.mmse_resampler_cc(0, (float(float(if_samp_rate)/float(out_sample_rate))))
        self.mmse_resampler_xx_0_2 = filter.mmse_resampler_cc(0, (float(float(if_samp_rate)/float(out_sample_rate))))
        self.mmse_resampler_xx_0_1 = filter.mmse_resampler_cc(0, (float(float(if_samp_rate)/float(out_sample_rate))))
        self.mmse_resampler_xx_0_0 = filter.mmse_resampler_cc(0, (float(float(if_samp_rate)/float(out_sample_rate))))
        self.mmse_resampler_xx_0 = filter.mmse_resampler_cc(0, (float(float(if_samp_rate)/float(out_sample_rate))))
        self.freq_xlating_fir_filter_xxx_0_0_0_0_0_0 = filter.freq_xlating_fir_filter_ccc(first_decim, firdes.low_pass(1, samp_rate, options_low_pass, options_low_pass*0.2), (xlate_offset6+xlate_offset_fine6), samp_rate)
        self.freq_xlating_fir_filter_xxx_0_0_0_0_0 = filter.freq_xlating_fir_filter_ccc(first_decim, firdes.low_pass(1, samp_rate, options_low_pass, options_low_pass*0.2), (xlate_offset5+xlate_offset_fine5), samp_rate)
        self.freq_xlating_fir_filter_xxx_0_0_0_0 = filter.freq_xlating_fir_filter_ccc(first_decim, firdes.low_pass(1, samp_rate, options_low_pass, options_low_pass*0.2), (xlate_offset4+xlate_offset_fine4), samp_rate)
        self.freq_xlating_fir_filter_xxx_0_0_0 = filter.freq_xlating_fir_filter_ccc(first_decim, firdes.low_pass(1, samp_rate, options_low_pass, options_low_pass*0.2), (xlate_offset3+xlate_offset_fine3), samp_rate)
        self.freq_xlating_fir_filter_xxx_0_0 = filter.freq_xlating_fir_filter_ccc(first_decim, firdes.low_pass(1, samp_rate, options_low_pass, options_low_pass*0.2), (xlate_offset2+xlate_offset_fine2), samp_rate)
        self.freq_xlating_fir_filter_xxx_0 = filter.freq_xlating_fir_filter_ccc(first_decim, firdes.low_pass(1, samp_rate, options_low_pass, options_low_pass*0.2), (xlate_offset1+xlate_offset_fine1), samp_rate)
        self.analog_agc3_xx_0_0_0_0_0_0 = analog.agc3_cc((1e-3), (1e-4), 1.0, 1.0, 1)
        self.analog_agc3_xx_0_0_0_0_0_0.set_max_gain(65536)
        self.analog_agc3_xx_0_0_0_0_0 = analog.agc3_cc((1e-3), (1e-4), 1.0, 1.0, 1)
        self.analog_agc3_xx_0_0_0_0_0.set_max_gain(65536)
        self.analog_agc3_xx_0_0_0_0 = analog.agc3_cc((1e-3), (1e-4), 1.0, 1.0, 1)
        self.analog_agc3_xx_0_0_0_0.set_max_gain(65536)
        self.analog_agc3_xx_0_0_0 = analog.agc3_cc((1e-3), (1e-4), 1.0, 1.0, 1)
        self.analog_agc3_xx_0_0_0.set_max_gain(65536)
        self.analog_agc3_xx_0_0 = analog.agc3_cc((1e-3), (1e-4), 1.0, 1.0, 1)
        self.analog_agc3_xx_0_0.set_max_gain(65536)
        self.analog_agc3_xx_0 = analog.agc3_cc((1e-3), (1e-4), 1.0, 1.0, 1)
        self.analog_agc3_xx_0.set_max_gain(65536)


        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_agc3_xx_0, 0), (self.mmse_resampler_xx_0, 0))
        self.connect((self.analog_agc3_xx_0_0, 0), (self.mmse_resampler_xx_0_0, 0))
        self.connect((self.analog_agc3_xx_0_0_0, 0), (self.mmse_resampler_xx_0_1, 0))
        self.connect((self.analog_agc3_xx_0_0_0_0, 0), (self.mmse_resampler_xx_0_2, 0))
        self.connect((self.analog_agc3_xx_0_0_0_0_0, 0), (self.mmse_resampler_xx_0_3, 0))
        self.connect((self.analog_agc3_xx_0_0_0_0_0_0, 0), (self.mmse_resampler_xx_0_4, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0, 0), (self.analog_agc3_xx_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0_0, 0), (self.analog_agc3_xx_0_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0_0_0, 0), (self.analog_agc3_xx_0_0_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0_0_0_0, 0), (self.analog_agc3_xx_0_0_0_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0_0_0_0_0, 0), (self.analog_agc3_xx_0_0_0_0_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0_0_0_0_0_0, 0), (self.analog_agc3_xx_0_0_0_0_0_0, 0))
        self.connect((self.mmse_resampler_xx_0, 0), (self.network_udp_sink_0, 0))
        self.connect((self.mmse_resampler_xx_0_0, 0), (self.network_udp_sink_0_0, 0))
        self.connect((self.mmse_resampler_xx_0_1, 0), (self.network_udp_sink_0_1, 0))
        self.connect((self.mmse_resampler_xx_0_2, 0), (self.network_udp_sink_0_2, 0))
        self.connect((self.mmse_resampler_xx_0_3, 0), (self.network_udp_sink_0_3, 0))
        self.connect((self.mmse_resampler_xx_0_4, 0), (self.network_udp_sink_0_4, 0))
        self.connect((self.osmosdr_source_0, 0), (self.freq_xlating_fir_filter_xxx_0, 0))
        self.connect((self.osmosdr_source_0, 0), (self.freq_xlating_fir_filter_xxx_0_0, 0))
        self.connect((self.osmosdr_source_0, 0), (self.freq_xlating_fir_filter_xxx_0_0_0, 0))
        self.connect((self.osmosdr_source_0, 0), (self.freq_xlating_fir_filter_xxx_0_0_0_0, 0))
        self.connect((self.osmosdr_source_0, 0), (self.freq_xlating_fir_filter_xxx_0_0_0_0_0, 0))
        self.connect((self.osmosdr_source_0, 0), (self.freq_xlating_fir_filter_xxx_0_0_0_0_0_0, 0))


    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_if_samp_rate(self.samp_rate/self.first_decim)
        self.freq_xlating_fir_filter_xxx_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))
        self.freq_xlating_fir_filter_xxx_0_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))
        self.freq_xlating_fir_filter_xxx_0_0_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))
        self.freq_xlating_fir_filter_xxx_0_0_0_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))
        self.freq_xlating_fir_filter_xxx_0_0_0_0_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))
        self.freq_xlating_fir_filter_xxx_0_0_0_0_0_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))
        self.osmosdr_source_0.set_sample_rate(self.samp_rate)

    def get_first_decim(self):
        return self.first_decim

    def set_first_decim(self, first_decim):
        self.first_decim = first_decim
        self.set_if_samp_rate(self.samp_rate/self.first_decim)

    def get_xlate_offset_fine6(self):
        return self.xlate_offset_fine6

    def set_xlate_offset_fine6(self, xlate_offset_fine6):
        self.xlate_offset_fine6 = xlate_offset_fine6
        self.freq_xlating_fir_filter_xxx_0_0_0_0_0_0.set_center_freq((self.xlate_offset6+self.xlate_offset_fine6))

    def get_xlate_offset_fine5(self):
        return self.xlate_offset_fine5

    def set_xlate_offset_fine5(self, xlate_offset_fine5):
        self.xlate_offset_fine5 = xlate_offset_fine5
        self.freq_xlating_fir_filter_xxx_0_0_0_0_0.set_center_freq((self.xlate_offset5+self.xlate_offset_fine5))

    def get_xlate_offset_fine4(self):
        return self.xlate_offset_fine4

    def set_xlate_offset_fine4(self, xlate_offset_fine4):
        self.xlate_offset_fine4 = xlate_offset_fine4
        self.freq_xlating_fir_filter_xxx_0_0_0_0.set_center_freq((self.xlate_offset4+self.xlate_offset_fine4))

    def get_xlate_offset_fine3(self):
        return self.xlate_offset_fine3

    def set_xlate_offset_fine3(self, xlate_offset_fine3):
        self.xlate_offset_fine3 = xlate_offset_fine3
        self.freq_xlating_fir_filter_xxx_0_0_0.set_center_freq((self.xlate_offset3+self.xlate_offset_fine3))

    def get_xlate_offset_fine2(self):
        return self.xlate_offset_fine2

    def set_xlate_offset_fine2(self, xlate_offset_fine2):
        self.xlate_offset_fine2 = xlate_offset_fine2
        self.freq_xlating_fir_filter_xxx_0_0.set_center_freq((self.xlate_offset2+self.xlate_offset_fine2))

    def get_xlate_offset_fine1(self):
        return self.xlate_offset_fine1

    def set_xlate_offset_fine1(self, xlate_offset_fine1):
        self.xlate_offset_fine1 = xlate_offset_fine1
        self.freq_xlating_fir_filter_xxx_0.set_center_freq((self.xlate_offset1+self.xlate_offset_fine1))

    def get_xlate_offset6(self):
        return self.xlate_offset6

    def set_xlate_offset6(self, xlate_offset6):
        self.xlate_offset6 = xlate_offset6
        self.freq_xlating_fir_filter_xxx_0_0_0_0_0_0.set_center_freq((self.xlate_offset6+self.xlate_offset_fine6))

    def get_xlate_offset5(self):
        return self.xlate_offset5

    def set_xlate_offset5(self, xlate_offset5):
        self.xlate_offset5 = xlate_offset5
        self.freq_xlating_fir_filter_xxx_0_0_0_0_0.set_center_freq((self.xlate_offset5+self.xlate_offset_fine5))

    def get_xlate_offset4(self):
        return self.xlate_offset4

    def set_xlate_offset4(self, xlate_offset4):
        self.xlate_offset4 = xlate_offset4
        self.freq_xlating_fir_filter_xxx_0_0_0_0.set_center_freq((self.xlate_offset4+self.xlate_offset_fine4))

    def get_xlate_offset3(self):
        return self.xlate_offset3

    def set_xlate_offset3(self, xlate_offset3):
        self.xlate_offset3 = xlate_offset3
        self.freq_xlating_fir_filter_xxx_0_0_0.set_center_freq((self.xlate_offset3+self.xlate_offset_fine3))

    def get_xlate_offset2(self):
        return self.xlate_offset2

    def set_xlate_offset2(self, xlate_offset2):
        self.xlate_offset2 = xlate_offset2
        self.freq_xlating_fir_filter_xxx_0_0.set_center_freq((self.xlate_offset2+self.xlate_offset_fine2))

    def get_xlate_offset1(self):
        return self.xlate_offset1

    def set_xlate_offset1(self, xlate_offset1):
        self.xlate_offset1 = xlate_offset1
        self.freq_xlating_fir_filter_xxx_0.set_center_freq((self.xlate_offset1+self.xlate_offset_fine1))

    def get_udp_packet_size(self):
        return self.udp_packet_size

    def set_udp_packet_size(self, udp_packet_size):
        self.udp_packet_size = udp_packet_size

    def get_udp_dest_addr(self):
        return self.udp_dest_addr

    def set_udp_dest_addr(self, udp_dest_addr):
        self.udp_dest_addr = udp_dest_addr

    def get_telive_receiver_name(self):
        return self.telive_receiver_name

    def set_telive_receiver_name(self, telive_receiver_name):
        self.telive_receiver_name = telive_receiver_name

    def get_telive_receiver_channels(self):
        return self.telive_receiver_channels

    def set_telive_receiver_channels(self, telive_receiver_channels):
        self.telive_receiver_channels = telive_receiver_channels

    def get_sdr_ifgain(self):
        return self.sdr_ifgain

    def set_sdr_ifgain(self, sdr_ifgain):
        self.sdr_ifgain = sdr_ifgain
        self.osmosdr_source_0.set_if_gain(self.sdr_ifgain, 0)

    def get_sdr_gain(self):
        return self.sdr_gain

    def set_sdr_gain(self, sdr_gain):
        self.sdr_gain = sdr_gain
        self.osmosdr_source_0.set_gain(self.sdr_gain, 0)

    def get_ppm_corr(self):
        return self.ppm_corr

    def set_ppm_corr(self, ppm_corr):
        self.ppm_corr = ppm_corr
        self.osmosdr_source_0.set_freq_corr(self.ppm_corr, 0)

    def get_out_sample_rate(self):
        return self.out_sample_rate

    def set_out_sample_rate(self, out_sample_rate):
        self.out_sample_rate = out_sample_rate
        self.mmse_resampler_xx_0.set_resamp_ratio((float(float(self.if_samp_rate)/float(self.out_sample_rate))))
        self.mmse_resampler_xx_0_0.set_resamp_ratio((float(float(self.if_samp_rate)/float(self.out_sample_rate))))
        self.mmse_resampler_xx_0_1.set_resamp_ratio((float(float(self.if_samp_rate)/float(self.out_sample_rate))))
        self.mmse_resampler_xx_0_2.set_resamp_ratio((float(float(self.if_samp_rate)/float(self.out_sample_rate))))
        self.mmse_resampler_xx_0_3.set_resamp_ratio((float(float(self.if_samp_rate)/float(self.out_sample_rate))))
        self.mmse_resampler_xx_0_4.set_resamp_ratio((float(float(self.if_samp_rate)/float(self.out_sample_rate))))

    def get_options_low_pass(self):
        return self.options_low_pass

    def set_options_low_pass(self, options_low_pass):
        self.options_low_pass = options_low_pass
        self.freq_xlating_fir_filter_xxx_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))
        self.freq_xlating_fir_filter_xxx_0_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))
        self.freq_xlating_fir_filter_xxx_0_0_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))
        self.freq_xlating_fir_filter_xxx_0_0_0_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))
        self.freq_xlating_fir_filter_xxx_0_0_0_0_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))
        self.freq_xlating_fir_filter_xxx_0_0_0_0_0_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))

    def get_if_samp_rate(self):
        return self.if_samp_rate

    def set_if_samp_rate(self, if_samp_rate):
        self.if_samp_rate = if_samp_rate
        self.mmse_resampler_xx_0.set_resamp_ratio((float(float(self.if_samp_rate)/float(self.out_sample_rate))))
        self.mmse_resampler_xx_0_0.set_resamp_ratio((float(float(self.if_samp_rate)/float(self.out_sample_rate))))
        self.mmse_resampler_xx_0_1.set_resamp_ratio((float(float(self.if_samp_rate)/float(self.out_sample_rate))))
        self.mmse_resampler_xx_0_2.set_resamp_ratio((float(float(self.if_samp_rate)/float(self.out_sample_rate))))
        self.mmse_resampler_xx_0_3.set_resamp_ratio((float(float(self.if_samp_rate)/float(self.out_sample_rate))))
        self.mmse_resampler_xx_0_4.set_resamp_ratio((float(float(self.if_samp_rate)/float(self.out_sample_rate))))

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.osmosdr_source_0.set_center_freq(self.freq, 0)

    def get_first_port(self):
        return self.first_port

    def set_first_port(self, first_port):
        self.first_port = first_port




def main(top_block_cls=telive_6ch_gr310_udp_xmlrpc_headless, options=None):
    tb = top_block_cls()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        sys.exit(0)

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    tb.start()

    tb.wait()


if __name__ == '__main__':
    main()
