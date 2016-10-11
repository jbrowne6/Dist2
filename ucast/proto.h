#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include "sendto_dbg.h"
#include <netdb.h>
#include <errno.h>

#define HEADER_SIZE			4		//in bytes
#define PORT                10200   //or 10210
#define MAX_PACKET_SIZE     1400    //the total allowed size of our packet in byte
#define BUFF_SIZE       	1400    //size of packets in bytes
#define MAX_DATA_SIZE		1396	//max amount of data in bytes per packet
#define MAX_WINDOW_SIZE     10   //this should be 2^16
#define NAME_LENGTH         80		//the max length of the file??
#define WINDOW_SIZE         1000    //intial window size, this is fixed for now
#define TIMEOUT_SEC         1       //timeout for the FD select in seconds
#define TIMEOUT_USEC        0       //timeout for the FD select in useconds

//packet header macros
#define PACKET_TYPE    		0xF0000000
#define PACKET_SIZE    		0x0FFF0000
#define PACKET_ID      		0x0000FFFF
#define PACKET_TYPE_HELLO   0x1
#define PACKET_TYPE_BYE     0x2
#define PACKET_TYPE_BUSY    0x3
#define PACKET_TYPE_ACK     0x4
#define PACKET_TYPE_NACK    0x5
#define PACKET_TYPE_DATA    0x6
#define PACKET_TYPE_EOF     0x7
#define PACKET_TYPE_EMPTY   0x8
#define PACKET_TYPE_BIT     28
#define PACKET_SIZE_BIT     16
#define PACKET_ID_BIT       0

typedef struct __attribute__((packed)) {
    int size;
    int type;
    int id;
} header_t;

typedef struct __attribute__((packed)) {
	header_t header;
    char p_data[MAX_DATA_SIZE]; //holds the data
} desc_t;

void tx_desc(desc_t* desc, int sr, int addr);
int rx_desc_probe(desc_t* desc, int sr);
