#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: SQ5BPF Tetra live receiver 1ch UDP HEADLESS for slow CPU
# Author: Jacek Lipkowski SQ5BPF
# Generated: Sun Jun 12 23:20:13 2016
##################################################

from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import SimpleXMLRPCServer
import osmosdr
import threading

class top_block(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "SQ5BPF Tetra live receiver 1ch UDP HEADLESS for slow CPU")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 256000
        self.first_decim = first_decim = 4
        self.xlate_offset_fine1 = xlate_offset_fine1 = 0
        self.xlate_offset1 = xlate_offset1 = 500e3
        self.udp_packet_size = udp_packet_size = 1472
        self.udp_dest_addr = udp_dest_addr = "127.0.0.1"
        self.telive_receiver_name = telive_receiver_name = 'SQ5BPF 1-channel headless rx for telive; 256ks/s version' 
        self.telive_receiver_channels = telive_receiver_channels = 1
        self.sdr_ifgain = sdr_ifgain = 20
        self.sdr_gain = sdr_gain = 38
        self.ppm_corr = ppm_corr = 56
        self.out_sample_rate = out_sample_rate = 36000
        self.options_low_pass = options_low_pass = 12500
        self.if_samp_rate = if_samp_rate = samp_rate/first_decim
        self.freq = freq = 435e6
        self.first_port = first_port = 42000

        ##################################################
        # Blocks
        ##################################################
        self.xmlrpc_server_0 = SimpleXMLRPCServer.SimpleXMLRPCServer(("0.0.0.0", first_port), allow_none=True)
        self.xmlrpc_server_0.register_instance(self)
        threading.Thread(target=self.xmlrpc_server_0.serve_forever).start()
        self.osmosdr_source_0 = osmosdr.source( args="numchan=" + str(1) + " " + "rtl=0,buflen=4096" )
        self.osmosdr_source_0.set_sample_rate(samp_rate)
        self.osmosdr_source_0.set_center_freq(freq, 0)
        self.osmosdr_source_0.set_freq_corr(ppm_corr, 0)
        self.osmosdr_source_0.set_dc_offset_mode(0, 0)
        self.osmosdr_source_0.set_iq_balance_mode(0, 0)
        self.osmosdr_source_0.set_gain_mode(False, 0)
        self.osmosdr_source_0.set_gain(sdr_gain, 0)
        self.osmosdr_source_0.set_if_gain(sdr_ifgain, 0)
        self.osmosdr_source_0.set_bb_gain(20, 0)
        self.osmosdr_source_0.set_antenna("", 0)
        self.osmosdr_source_0.set_bandwidth(0, 0)
          
        self.freq_xlating_fir_filter_xxx_0 = filter.freq_xlating_fir_filter_ccc(first_decim, (firdes.low_pass(1, samp_rate, options_low_pass, options_low_pass*0.2)), xlate_offset1+xlate_offset_fine1, samp_rate)
        self.fractional_resampler_xx_0 = filter.fractional_resampler_cc(0, float(float(if_samp_rate)/float(out_sample_rate)))
        self.blocks_udp_sink_0 = blocks.udp_sink(gr.sizeof_gr_complex*1, udp_dest_addr, first_port+1, udp_packet_size, False)
        self.analog_agc3_xx_0 = analog.agc3_cc(1e-3, 1e-4, 1.0, 1.0, 1)
        self.analog_agc3_xx_0.set_max_gain(65536)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.osmosdr_source_0, 0), (self.freq_xlating_fir_filter_xxx_0, 0))
        self.connect((self.fractional_resampler_xx_0, 0), (self.blocks_udp_sink_0, 0))
        self.connect((self.analog_agc3_xx_0, 0), (self.fractional_resampler_xx_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0, 0), (self.analog_agc3_xx_0, 0))



    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_if_samp_rate(self.samp_rate/self.first_decim)
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2)))
        self.osmosdr_source_0.set_sample_rate(self.samp_rate)

    def get_first_decim(self):
        return self.first_decim

    def set_first_decim(self, first_decim):
        self.first_decim = first_decim
        self.set_if_samp_rate(self.samp_rate/self.first_decim)

    def get_xlate_offset_fine1(self):
        return self.xlate_offset_fine1

    def set_xlate_offset_fine1(self, xlate_offset_fine1):
        self.xlate_offset_fine1 = xlate_offset_fine1
        self.freq_xlating_fir_filter_xxx_0.set_center_freq(self.xlate_offset1+self.xlate_offset_fine1)

    def get_xlate_offset1(self):
        return self.xlate_offset1

    def set_xlate_offset1(self, xlate_offset1):
        self.xlate_offset1 = xlate_offset1
        self.freq_xlating_fir_filter_xxx_0.set_center_freq(self.xlate_offset1+self.xlate_offset_fine1)

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
        self.fractional_resampler_xx_0.set_resamp_ratio(float(float(self.if_samp_rate)/float(self.out_sample_rate)))

    def get_options_low_pass(self):
        return self.options_low_pass

    def set_options_low_pass(self, options_low_pass):
        self.options_low_pass = options_low_pass
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2)))

    def get_if_samp_rate(self):
        return self.if_samp_rate

    def set_if_samp_rate(self, if_samp_rate):
        self.if_samp_rate = if_samp_rate
        self.fractional_resampler_xx_0.set_resamp_ratio(float(float(self.if_samp_rate)/float(self.out_sample_rate)))

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.osmosdr_source_0.set_center_freq(self.freq, 0)

    def get_first_port(self):
        return self.first_port

    def set_first_port(self, first_port):
        self.first_port = first_port

if __name__ == '__main__':
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    (options, args) = parser.parse_args()
    tb = top_block()
    tb.start()
    tb.wait()
