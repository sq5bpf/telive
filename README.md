telive - Tetra Live Monitor
(c) 2014-2015 Jacek Lipkowski <sq5bpf@lipkowski.org>

telive is a program which can be used to display information like
signalling, calls etc from a Tetra network. It is also possible to
log the signalling information, listen to the audio in realtime and
to record the audio. Playing the audio and recompressing it into ogg
is done via external scripts.

Please read telive_doc.pdf, either in this directory or here:
https://github.com/sq5bpf/telive/raw/master/telive_doc.pdf

----------- Docker ----------

```
cd docker
./run.sh
```

If you get audio issues, check your pulse audio configuration

```echo "load-module module-esound-protocol-tcp auth-anonymous=1" >> /etc/pulse/system.pa```

----------- Disclaimer ----------

The program is licenced under GPL v3 (license text is also included in the 
file LICENSE). I may not be held responsible for anything associated with 
the use of this tool.


This was a quick hack written in my spare time, and for my own pleasure,
and because of this the code is really ugly. The code is also based on wrong 
assumptions - using the usage identifier as the key is not a good way of 
following calls (but it works most of the time). Maybe one day this will
be rewritten to look better (or not).

-----------------------------------

