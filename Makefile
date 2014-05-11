CC=gcc

CFLAGS= -g -O3 -lpthread -D_REENTRANT -Werror -W -Wextra -Wcast-qual -Wcast-align -Wfloat-equal -Wshadow -Wpointer-arith -Wunreachable-code -Wchar-subscripts -Wcomment -Wformat -Werror-implicit-function-declaration -Wmain -Wmissing-braces -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wuninitialized -Wundef -Wshadow -Wwrite-strings -Wsign-compare -pedantic -Wconversion -Wmissing-noreturn -Wall -Wunused -Wsign-conversion -Wunused -Wstrict-aliasing -Wstrict-overflow -Wconversion -Wdisabled-optimization -Wlogical-op -Wunsafe-loop-optimizations

EXEC=setup tracker client

all: $(EXEC)

setup:
	mkdir -p obj

obj/tracker.o: src/tracker.c src/tracker.h
	$(CC) -c -o $@ $< $(CFLAGS)

obj/peerfunc.o: src/peerfunc.c src/peerfunc.h
	$(CC) -c -o $@ $< $(CFLAGS)
	
obj/client.o: src/client.c src/client.h
	$(CC) -c -o $@ $< $(CFLAGS)

tracker: obj/tracker.o
	gcc -o $@ $^ $(CFLAGS)
	
client: obj/client.o obj/peerfunc.o
	gcc -o $@ $^ $(CFLAGS)
	
clean:
	rm -rf obj $(EXEC) *~ */*~
