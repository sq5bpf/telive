default: telive

telive: telive.c telive.h
	gcc telive.c -o telive -lncurses `xml2-config --cflags --libs` -g 

clean:
	rm telive

