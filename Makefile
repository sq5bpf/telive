default: telive

telive: telive.c telive.h
	gcc `xml2-config --cflags --libs` telive.c -o telive -lncurses -g

