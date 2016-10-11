CC=gcc

CFLAGS = -g -Wall -pedantic

all: mcast

mcast: obj/sendto_dbg.o obj/mutils.o
	$(CC) -o $@ mcast.c obj/mutils.o obj/sendto_dbg.o $(CFLAGS)

./obj/sendto_dbg.o: ./lib/sendto_dbg.c ./lib/sendto_dbg.h
	$(CC) -c -o ./obj/sendto_dbg.o ./lib/sendto_dbg.c

./obj/mutils.o: mutils.c mutils.h
	$(CC) -c -o ./obj/mutils.o mutils.c

clean:
	rm obj/*.o
	rm mcast
