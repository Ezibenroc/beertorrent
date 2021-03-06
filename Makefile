CC=gcc #colorgcc

CFLAGS= -g -O3 -lpthread -D_REENTRANT -W -Wextra -Wcast-qual -Wcast-align -Wfloat-equal -Wshadow -Wpointer-arith -Wunreachable-code -Wchar-subscripts -Wcomment -Wformat -Werror-implicit-function-declaration -Wmain -Wmissing-braces -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wuninitialized -Wundef -Wshadow -Wwrite-strings -Wsign-compare -pedantic -Wconversion -Wall -Wunused -Wsign-conversion -Wunused -Wstrict-aliasing -Wstrict-overflow -Wconversion -Wdisabled-optimization -Wlogical-op -Wunsafe-loop-optimizations # -Werror -Wmissing-noreturn

EXEC=setup tracker client torrent_maker

all: $(EXEC)

setup:
	mkdir -p obj

obj/tracker.o: src/tracker.c src/tracker.h
	$(CC) -c -o $@ $< $(CFLAGS)

obj/peerfunc.o: src/peerfunc.c src/peerfunc.h
	$(CC) -c -o $@ $< $(CFLAGS)
	
obj/client.o: src/client.c src/common.c src/common.h src/peerfunc.c src/peerfunc.h
	$(CC) -c -o $@ $< $(CFLAGS)
	
obj/common.o: src/common.c src/common.h src/peerfunc.c src/peerfunc.h
	$(CC) -c -o $@ $< $(CFLAGS)
	
obj/rename.o: src/rename.c src/rename.h
	$(CC) -c -o $@ $< $(CFLAGS)

tracker: obj/tracker.o 
	gcc -o $@ $^ $(CFLAGS)
	
client: obj/client.o obj/peerfunc.o obj/common.o obj/rename.o
	gcc -o $@ $^ $(CFLAGS)
	
clean:
	rm -rf obj $(EXEC) *~ */*~
	
	
############### TORRENT MAKER
obj/torrent_maker.o: src/torrent_maker.c
	$(CC) -c -o $@ $< $(CFLAGS)

torrent_maker: obj/torrent_maker.o obj/common.o obj/peerfunc.o obj/rename.o
	gcc -o $@ $^ $(CFLAGS)
	
	
