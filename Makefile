default: telive

telive: telive.c telive.h
	gcc telive.c -o telive -lncurses -g

