////////////////////////////////////////////////////////////////////////////////
//
//    James D. Browne
//    Eric Carlson
//    Distributed Assignment 2
//
//    mutils.c
//    Functions required by mcast.c
//    Date: 2016/10/10
//
////////////////////////////////////////////////////////////////////////////////

# include "mutils.h"

int main(int argc, char *argv[])
{
    struct sockaddr_in name;
    struct sockaddr_in send_addr;

    int                mcast_addr;

    struct ip_mreq     mreq;
    unsigned char      ttl_val;

    int                ss,sr;
    fd_set             mask;
    fd_set             dummy_mask,temp_mask;
    int                bytes;
    int                num;
    char               mess_buf[MAX_MESS_LEN];
    char               input_buf[80];

	long num_packets; //the number of packets to send.
	int machine_index;
	int num_machines;
	int loss_rate;
	int state = 0;
////////////////////////////////////////////////////////////////////////
//get input from user.
////////////////////////////////////////////////////////////////////////

	get_cli(&num_packets, &machine_index, &num_machines, &loss_rate, argv, argc);

///////////////////////////////////////////////////////////////////////
//setup networking
///////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////
	//Place everything in this large block into functions.
	//    int makeSock();
	//    void setSock(int socket, ip_mreq *mreq);
	//    void setSendAddress(sockaddr_in *send_addr, int *mcast_addr);
	//
	//    Make sure to get rid of all unused variables after transition.
	//

	//set multicast address
    mcast_addr = 225 << 24 | 0 << 16 | 1 << 8 | 1; /* (225.0.1.1) */

    sr = socket(AF_INET, SOCK_DGRAM, 0); /* socket for receiving */
    if (sr<0) {
        perror("Mcast: socket");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    if ( bind( sr, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("Mcast: bind");
        exit(1);
    }

    mreq.imr_multiaddr.s_addr = htonl( mcast_addr );

    /* the interface could be changed to a specific interface if needed */
    mreq.imr_interface.s_addr = htonl( INADDR_ANY );

    if (setsockopt(sr, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq,
        sizeof(mreq)) < 0)
    {
        perror("Mcast: problem in setsockopt to join multicast address" );
    }

    ss = socket(AF_INET, SOCK_DGRAM, 0); /* Socket for sending */
    if (ss<0) {
        perror("Mcast: socket");
        exit(1);
    }

    ttl_val = 1;
    if (setsockopt(ss, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl_val,
        sizeof(ttl_val)) < 0)
    {
        printf("Mcast: problem in setsockopt of multicast ttl %d - ignore in WinNT or Win95\n", ttl_val );
    }

    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = htonl(mcast_addr);  /* mcast address */
    send_addr.sin_port = htons(PORT);
//////////////////////////////////////////////////////////////////////////////////
//The block above should become three lines.
//////////////////////////////////////////////////////////////////////////////////

	for(;;){
/////////////////////////////////////////////////////////////////////////////////
//if not id #1 wait and listen this will be state 0;
//make sure not lossy listen
////////////////////////////////////////////////////////////////////////////////
if (state == 0){

//////////////////////////////////////////////////////////////////////////////////
//state 1 will be when everyone learns all the unicast IPs.
//machine id 1 will move to state 2 all others will move to state 4
/////////////////////////////////////////////////////////////////////////////////
}else if( state == 1){

/////////////////////////////////////////////////////////////////////////////////
//State 2 means you have the token.
//At end of state you should send token and move to state 3.
////////////////////////////////////////////////////////////////////////////////
}else if(state == 2){

//////////////////////////////////////////////////////////////////////////////
//State 3 means you sent the token but you want to make sure the token was
//recieved.  When confirmed, moved to state 4
//While searching for the token ensure you continue to process all packets.
//////////////////////////////////////////////////////////////////////////////
}else if( state == 3){

/////////////////////////////////////////////////////////////////////////////
//This is the recieving state.  All packets should be processed as received.
/////////////////////////////////////////////////////////////////////////////
}else if( state == 4){

/////////////////////////////////////////////////////////////////////////////
//when everyone is done move to state 5 and say goodbye.
////////////////////////////////////////////////////////////////////////////
}else if ( state == 5){

	}
///////////////////////////////////////////////////////////////////////////////
//the below is just useful code, but of course it doesn't belong here.
////////////////////////////////////////////////////////////////////////////////
    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );
    FD_SET( (long)0, &mask );    /* stdin */
    for(;;)
    {
        temp_mask = mask;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, NULL);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                bytes = recv( sr, mess_buf, sizeof(mess_buf), 0 );
                mess_buf[bytes] = 0;
                printf( "received : %s\n", mess_buf );
            }else if( FD_ISSET(0, &temp_mask) ) {
                bytes = read( 0, input_buf, sizeof(input_buf) );
                input_buf[bytes] = 0;
                printf( "there is an input: %s\n", input_buf );
                sendto( ss, input_buf, strlen(input_buf), 0,
                    (struct sockaddr *)&send_addr, sizeof(send_addr) );
            }
        }
    }
	}
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
//
//  Revsion History
//
//
////////////////////////////////////////////////////////////////////////////////
