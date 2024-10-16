
all : dirs bin/fsbw

LIBS=-lm

dirs :
	[ -d bin ] || mkdir bin
	[ -d obj ] || mkdir obj

obj/experiments.o : src/experiments.c src/common.h src/program_options.h src/stats.h
	gcc -c -o $@ $<

obj/program_options.o : src/program_options.c src/common.h
	gcc -c -o $@ $<

obj/stats.o : src/stats.c 
	gcc -c -o $@ $<

obj/main.o : src/main.c src/common.h src/experiments.h src/program_options.h
	gcc -c -o $@ $<

bin/fsbw : obj/main.o obj/stats.o obj/program_options.o obj/experiments.o
	gcc -o $@ $^ $(LIBS)

install : bin/fsbw 
	install -d $(PREFIX)/bin/
	install -m 755 bin/fsbw $(PREFIX)/bin/

clean :
	rm obj/*
	rm bin/*
