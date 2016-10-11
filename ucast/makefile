CC=gcc

CFLAGS = -g -c -Wall -pedantic

all: ncp rcv

ncp: sendto_dbg.o proto.o ncp.c
	$(CC) -o ncp ncp.c proto.o sendto_dbg.o
rcv: sendto_dbg.o proto.o rcv.c
	$(CC) -o rcv rcv.c proto.o sendto_dbg.o

sendto_dbg.o: sendto_dbg.c sendto_dbg.h
	$(CC) -c sendto_dbg.c
proto.o: proto.c proto.h
	$(CC) -c proto.c

clean:
	rm *.o
	rm rcv
	rm ncp

%.o:    %.c
	$(CC) $(CFLAGS) $*.c
