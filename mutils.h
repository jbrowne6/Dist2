////////////////////////////////////////////////////////////////////////////////
//
//    James D. Browne
//    Eric
//    Distributed Assignment 2
//
//    mutils.h
//    Header file for functions required by mcast.c
//    Date: 2016/10/10
//
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>

#include <stdlib.h>

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>

#include <errno.h>

#define PORT	     11011

#define MAX_MESS_LEN 1400

//header_t holds all header information based on design document
typedef struct __attribute__((packed)) {
    int size;
    int type;
    int id;
} header_t;

//desc_t is the combined header and header and data.  This is our packet
//structure.
typedef struct __attribute__((packed)) {
	header_t header;
    char p_data[MAX_DATA_SIZE]; //holds the data
} desc_t;

//send a unicast packet.  Takes a packet, a socket, and destination address.
void tx_desc(desc_t* desc, int sr, int addr);

// receive a packet.  returns the number of bytes in packet and sets desc to
// received packet.  Takes a pointer to a packet to modify and a socket.
int rx_desc_probe(desc_t* desc, int sr);

////////////////////////////////////////////////////////////////////////////////
//
//  Revsion History
//
//
////////////////////////////////////////////////////////////////////////////////
