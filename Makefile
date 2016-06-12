CFLAGS=-O0 -g  #please don't remove the -g, it really helps debugging
CC=gcc

default: telive

telive_util.o: telive_util.c telive_util.h
	$(CC) $(CFLAGS) -c $^

telive_receiver.o: telive_receiver.c telive_receiver.h
	$(CC) $(CFLAGS) -c $^ `xml2-config --cflags`

telive: telive.c telive.h telive_receiver.o telive_util.o
	$(CC) $(CFLAGS) telive.c telive_receiver.o telive_util.o -o telive -lncurses `xml2-config --cflags --libs` 

clean:
	rm telive telive_receiver.o telive_util.o

