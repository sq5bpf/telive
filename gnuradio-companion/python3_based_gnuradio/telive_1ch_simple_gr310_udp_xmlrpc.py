#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: SQ5BPF Tetra live receiver 1ch simple UDP demo with fixed offset (gnuradio 3.10 version) xmlrpc
# Author: Jacek Lipkowski SQ5BPF
# GNU Radio version: 3.10.5.1

from packaging.version import Version as StrictVersion

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print("Warning: failed to XInitThreads()")

from PyQt5 import Qt
from gnuradio import eng_notation
from gnuradio import qtgui
from gnuradio.filter import firdes
import sip
from gnuradio import analog
from gnuradio import filter
from gnuradio import gr
from gnuradio.fft import window
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import network
from gnuradio.qtgui import Range, RangeWidget
from PyQt5 import QtCore
from xmlrpc.server import SimpleXMLRPCServer
import threading
import osmosdr
import time



from gnuradio import qtgui

class telive_1ch_simple_gr310_udp_xmlrpc(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "SQ5BPF Tetra live receiver 1ch simple UDP demo with fixed offset (gnuradio 3.10 version) xmlrpc", catch_exceptions=True)
        Qt.QWidget.__init__(self)
        self.setWindowTitle("SQ5BPF Tetra live receiver 1ch simple UDP demo with fixed offset (gnuradio 3.10 version) xmlrpc")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except:
            pass
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("GNU Radio", "telive_1ch_simple_gr310_udp_xmlrpc")

        try:
            if StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
                self.restoreGeometry(self.settings.value("geometry").toByteArray())
            else:
                self.restoreGeometry(self.settings.value("geometry"))
        except:
            pass

        ##################################################
        # Variables
        ##################################################
        self.xlate_offset_fine1 = xlate_offset_fine1 = 0
        self.samp_rate = samp_rate = 2000000
        self.freq = freq = 438.0125e6
        self.first_decim = first_decim = 32
        self.xlate_offset1 = xlate_offset1 = 500000
        self.variable_qtgui_label_0_0 = variable_qtgui_label_0_0 = (freq+xlate_offset_fine1)
        self.udp_packet_size = udp_packet_size = 1472
        self.udp_dest_addr = udp_dest_addr = "127.0.0.1"
        self.telive_receiver_name = telive_receiver_name = 'SQ5BPF 1-channel rx for telive'
        self.telive_receiver_channels = telive_receiver_channels = 1
        self.sdr_ifgain = sdr_ifgain = 20
        self.sdr_gain = sdr_gain = 30
        self.ppm_corr = ppm_corr = 0
        self.out_sample_rate = out_sample_rate = 36000
        self.options_low_pass = options_low_pass = 12500
        self.if_samp_rate = if_samp_rate = samp_rate/first_decim
        self.first_port = first_port = 42000

        ##################################################
        # Blocks
        ##################################################

        self._xlate_offset_fine1_range = Range(-5e3, +5e3, 1, 0, 200)
        self._xlate_offset_fine1_win = RangeWidget(self._xlate_offset_fine1_range, self.set_xlate_offset_fine1, "Fine tune1", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_grid_layout.addWidget(self._xlate_offset_fine1_win, 0, 2, 1, 3)
        for r in range(0, 1):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(2, 5):
            self.top_grid_layout.setColumnStretch(c, 1)
        self._sdr_gain_range = Range(0, 50, 1, 30, 200)
        self._sdr_gain_win = RangeWidget(self._sdr_gain_range, self.set_sdr_gain, "gain", "counter_slider", int, QtCore.Qt.Horizontal)
        self.top_grid_layout.addWidget(self._sdr_gain_win, 0, 8, 1, 1)
        for r in range(0, 1):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(8, 9):
            self.top_grid_layout.setColumnStretch(c, 1)
        self._ppm_corr_range = Range(-100, 100, 0.5, 0, 200)
        self._ppm_corr_win = RangeWidget(self._ppm_corr_range, self.set_ppm_corr, "ppm", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_grid_layout.addWidget(self._ppm_corr_win, 0, 5, 1, 3)
        for r in range(0, 1):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(5, 8):
            self.top_grid_layout.setColumnStretch(c, 1)
        self._freq_tool_bar = Qt.QToolBar(self)
        self._freq_tool_bar.addWidget(Qt.QLabel("Frequency" + ": "))
        self._freq_line_edit = Qt.QLineEdit(str(self.freq))
        self._freq_tool_bar.addWidget(self._freq_line_edit)
        self._freq_line_edit.returnPressed.connect(
            lambda: self.set_freq(eng_notation.str_to_num(str(self._freq_line_edit.text()))))
        self.top_grid_layout.addWidget(self._freq_tool_bar, 0, 0, 1, 2)
        for r in range(0, 1):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(0, 2):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.xmlrpc_server_0 = SimpleXMLRPCServer(('0.0.0.0', first_port), allow_none=True)
        self.xmlrpc_server_0.register_instance(self)
        self.xmlrpc_server_0_thread = threading.Thread(target=self.xmlrpc_server_0.serve_forever)
        self.xmlrpc_server_0_thread.daemon = True
        self.xmlrpc_server_0_thread.start()
        self._variable_qtgui_label_0_0_tool_bar = Qt.QToolBar(self)

        if lambda x: f'{x/1000000:.4f} MHz':
            self._variable_qtgui_label_0_0_formatter = lambda x: f'{x/1000000:.4f} MHz'
        else:
            self._variable_qtgui_label_0_0_formatter = lambda x: eng_notation.num_to_str(x)

        self._variable_qtgui_label_0_0_tool_bar.addWidget(Qt.QLabel("Receive frequency: "))
        self._variable_qtgui_label_0_0_label = Qt.QLabel(str(self._variable_qtgui_label_0_0_formatter(self.variable_qtgui_label_0_0)))
        self._variable_qtgui_label_0_0_tool_bar.addWidget(self._variable_qtgui_label_0_0_label)
        self.top_layout.addWidget(self._variable_qtgui_label_0_0_tool_bar)
        self.qtgui_freq_sink_x_0_0 = qtgui.freq_sink_c(
            256, #size
            window.WIN_BLACKMAN_hARRIS, #wintype
            0, #fc
            if_samp_rate, #bw
            "IF", #name
            1,
            None # parent
        )
        self.qtgui_freq_sink_x_0_0.set_update_time(0.01)
        self.qtgui_freq_sink_x_0_0.set_y_axis((-180), (-30))
        self.qtgui_freq_sink_x_0_0.set_y_label('Relative Gain', 'dB')
        self.qtgui_freq_sink_x_0_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, 0.0, 0, "")
        self.qtgui_freq_sink_x_0_0.enable_autoscale(True)
        self.qtgui_freq_sink_x_0_0.enable_grid(True)
        self.qtgui_freq_sink_x_0_0.set_fft_average(1.0)
        self.qtgui_freq_sink_x_0_0.enable_axis_labels(True)
        self.qtgui_freq_sink_x_0_0.enable_control_panel(True)
        self.qtgui_freq_sink_x_0_0.set_fft_window_normalized(False)



        labels = ['', '', '', '', '',
            '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
            "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
            1.0, 1.0, 1.0, 1.0, 1.0]

        for i in range(1):
            if len(labels[i]) == 0:
                self.qtgui_freq_sink_x_0_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_freq_sink_x_0_0.set_line_label(i, labels[i])
            self.qtgui_freq_sink_x_0_0.set_line_width(i, widths[i])
            self.qtgui_freq_sink_x_0_0.set_line_color(i, colors[i])
            self.qtgui_freq_sink_x_0_0.set_line_alpha(i, alphas[i])

        self._qtgui_freq_sink_x_0_0_win = sip.wrapinstance(self.qtgui_freq_sink_x_0_0.qwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_freq_sink_x_0_0_win, 1, 6, 1, 6)
        for r in range(1, 2):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(6, 12):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.qtgui_freq_sink_x_0 = qtgui.freq_sink_c(
            1024, #size
            window.WIN_BLACKMAN_hARRIS, #wintype
            (freq-xlate_offset1), #fc
            samp_rate, #bw
            "", #name
            1,
            None # parent
        )
        self.qtgui_freq_sink_x_0.set_update_time(0.10)
        self.qtgui_freq_sink_x_0.set_y_axis((-140), 10)
        self.qtgui_freq_sink_x_0.set_y_label('Relative Gain', 'dB')
        self.qtgui_freq_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, 0.0, 0, "")
        self.qtgui_freq_sink_x_0.enable_autoscale(False)
        self.qtgui_freq_sink_x_0.enable_grid(False)
        self.qtgui_freq_sink_x_0.set_fft_average(0.2)
        self.qtgui_freq_sink_x_0.enable_axis_labels(True)
        self.qtgui_freq_sink_x_0.enable_control_panel(True)
        self.qtgui_freq_sink_x_0.set_fft_window_normalized(False)



        labels = ['', '', '', '', '',
            '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
            "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
            1.0, 1.0, 1.0, 1.0, 1.0]

        for i in range(1):
            if len(labels[i]) == 0:
                self.qtgui_freq_sink_x_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_freq_sink_x_0.set_line_label(i, labels[i])
            self.qtgui_freq_sink_x_0.set_line_width(i, widths[i])
            self.qtgui_freq_sink_x_0.set_line_color(i, colors[i])
            self.qtgui_freq_sink_x_0.set_line_alpha(i, alphas[i])

        self._qtgui_freq_sink_x_0_win = sip.wrapinstance(self.qtgui_freq_sink_x_0.qwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_freq_sink_x_0_win, 1, 0, 1, 6)
        for r in range(1, 2):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(0, 6):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.osmosdr_source_0 = osmosdr.source(
            args="numchan=" + str(1) + " " + ''
        )
        self.osmosdr_source_0.set_time_unknown_pps(osmosdr.time_spec_t())
        self.osmosdr_source_0.set_sample_rate(samp_rate)
        self.osmosdr_source_0.set_center_freq((freq-xlate_offset1), 0)
        self.osmosdr_source_0.set_freq_corr(ppm_corr, 0)
        self.osmosdr_source_0.set_dc_offset_mode(0, 0)
        self.osmosdr_source_0.set_iq_balance_mode(0, 0)
        self.osmosdr_source_0.set_gain_mode(False, 0)
        self.osmosdr_source_0.set_gain(sdr_gain, 0)
        self.osmosdr_source_0.set_if_gain(sdr_ifgain, 0)
        self.osmosdr_source_0.set_bb_gain(20, 0)
        self.osmosdr_source_0.set_antenna('', 0)
        self.osmosdr_source_0.set_bandwidth(0, 0)
        self.network_udp_sink_0 = network.udp_sink(gr.sizeof_gr_complex, 1, udp_dest_addr, (first_port+1), 0, udp_packet_size, False)
        self.mmse_resampler_xx_0 = filter.mmse_resampler_cc(0, (float(float(if_samp_rate)/float(out_sample_rate))))
        self.freq_xlating_fir_filter_xxx_0 = filter.freq_xlating_fir_filter_ccc(first_decim, firdes.low_pass(1, samp_rate, options_low_pass, options_low_pass*0.2), (xlate_offset1+xlate_offset_fine1), samp_rate)
        self.analog_agc3_xx_0 = analog.agc3_cc((1e-3), (1e-4), 1.0, 1.0, 1)
        self.analog_agc3_xx_0.set_max_gain(65536)


        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_agc3_xx_0, 0), (self.mmse_resampler_xx_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0, 0), (self.analog_agc3_xx_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0, 0), (self.qtgui_freq_sink_x_0_0, 0))
        self.connect((self.mmse_resampler_xx_0, 0), (self.network_udp_sink_0, 0))
        self.connect((self.osmosdr_source_0, 0), (self.freq_xlating_fir_filter_xxx_0, 0))
        self.connect((self.osmosdr_source_0, 0), (self.qtgui_freq_sink_x_0, 0))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "telive_1ch_simple_gr310_udp_xmlrpc")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_xlate_offset_fine1(self):
        return self.xlate_offset_fine1

    def set_xlate_offset_fine1(self, xlate_offset_fine1):
        self.xlate_offset_fine1 = xlate_offset_fine1
        self.set_variable_qtgui_label_0_0((self.freq+self.xlate_offset_fine1))
        self.freq_xlating_fir_filter_xxx_0.set_center_freq((self.xlate_offset1+self.xlate_offset_fine1))

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_if_samp_rate(self.samp_rate/self.first_decim)
        self.freq_xlating_fir_filter_xxx_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))
        self.osmosdr_source_0.set_sample_rate(self.samp_rate)
        self.qtgui_freq_sink_x_0.set_frequency_range((self.freq-self.xlate_offset1), self.samp_rate)

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        Qt.QMetaObject.invokeMethod(self._freq_line_edit, "setText", Qt.Q_ARG("QString", eng_notation.num_to_str(self.freq)))
        self.set_variable_qtgui_label_0_0((self.freq+self.xlate_offset_fine1))
        self.osmosdr_source_0.set_center_freq((self.freq-self.xlate_offset1), 0)
        self.qtgui_freq_sink_x_0.set_frequency_range((self.freq-self.xlate_offset1), self.samp_rate)

    def get_first_decim(self):
        return self.first_decim

    def set_first_decim(self, first_decim):
        self.first_decim = first_decim
        self.set_if_samp_rate(self.samp_rate/self.first_decim)

    def get_xlate_offset1(self):
        return self.xlate_offset1

    def set_xlate_offset1(self, xlate_offset1):
        self.xlate_offset1 = xlate_offset1
        self.freq_xlating_fir_filter_xxx_0.set_center_freq((self.xlate_offset1+self.xlate_offset_fine1))
        self.osmosdr_source_0.set_center_freq((self.freq-self.xlate_offset1), 0)
        self.qtgui_freq_sink_x_0.set_frequency_range((self.freq-self.xlate_offset1), self.samp_rate)

    def get_variable_qtgui_label_0_0(self):
        return self.variable_qtgui_label_0_0

    def set_variable_qtgui_label_0_0(self, variable_qtgui_label_0_0):
        self.variable_qtgui_label_0_0 = variable_qtgui_label_0_0
        Qt.QMetaObject.invokeMethod(self._variable_qtgui_label_0_0_label, "setText", Qt.Q_ARG("QString", str(self._variable_qtgui_label_0_0_formatter(self.variable_qtgui_label_0_0))))

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

    def get_options_low_pass(self):
        return self.options_low_pass

    def set_options_low_pass(self, options_low_pass):
        self.options_low_pass = options_low_pass
        self.freq_xlating_fir_filter_xxx_0.set_taps(firdes.low_pass(1, self.samp_rate, self.options_low_pass, self.options_low_pass*0.2))

    def get_if_samp_rate(self):
        return self.if_samp_rate

    def set_if_samp_rate(self, if_samp_rate):
        self.if_samp_rate = if_samp_rate
        self.mmse_resampler_xx_0.set_resamp_ratio((float(float(self.if_samp_rate)/float(self.out_sample_rate))))
        self.qtgui_freq_sink_x_0_0.set_frequency_range(0, self.if_samp_rate)

    def get_first_port(self):
        return self.first_port

    def set_first_port(self, first_port):
        self.first_port = first_port




def main(top_block_cls=telive_1ch_simple_gr310_udp_xmlrpc, options=None):

    if StrictVersion("4.5.0") <= StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()

    tb.start()

    tb.show()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        Qt.QApplication.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    qapp.exec_()

if __name__ == '__main__':
    main()
