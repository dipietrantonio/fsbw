
all : dirs bin/fsbw

LIBS=-lm

dirs :
	[ -d bin ] || mkdir bin
	[ -d obj ] || mkdir obj

obj/experiments.o : src/experiments.c
	gcc -c -o $@ $^

obj/program_options.o : src/program_options.c
	gcc -c -o $@ $^

obj/stats.o : src/stats.c 
	gcc -c -o $@ $^

obj/main.o : src/main.c
	gcc -c -o $@ $^

bin/fsbw : obj/main.o obj/stats.o obj/program_options.o obj/experiments.o
	gcc -o $@ $^ $(LIBS)


clean :
	rm obj/*
	rm bin/*