
all : dirs bin/af

LIBS=-lm

dirs :
	[ -d bin ] || mkdir bin

bin/af : src/af.c
	gcc -o bin/af src/af.c $(LIBS)
