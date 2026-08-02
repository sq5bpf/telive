CFLAGS=-O0 -g  #please don't remove the -g, it really helps debugging
CC=gcc

# Match Apple Clang to the architecture reported by the current shell. This is
# especially important under Rosetta, where the compiler may otherwise select
# arm64 while pkg-config resolves x86_64 libraries.
ifeq ($(shell uname -s),Darwin)
CC=gcc -arch $(shell uname -m)
endif

default: telive

telive_util.o: telive_util.c telive_util.h
	$(CC) $(CFLAGS) -c $^

telive_receiver.o: telive_receiver.c telive_receiver.h
	$(CC) $(CFLAGS) -c $^ `xml2-config --cflags`

telive: telive.c telive.h telive_receiver.o telive_util.o
	$(CC) $(CFLAGS) telive.c telive_receiver.o telive_util.o -o telive `pkg-config ncurses --cflags --libs` `xml2-config --cflags --libs` 

clean:
	rm telive telive_receiver.o telive_util.o
