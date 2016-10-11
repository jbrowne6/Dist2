#include "proto.h"
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define SUB_WIN_SIZE 5
#define BUSY_TIMEOUT 3		//seconds
#define BYTE_WRITE_LIM 1048576

//this is rcv
int main (int argc, char **argv)
{
    FILE *fw;			/* Pointer to source file, which we read */
    char buf[BUFF_SIZE];
    char ack_idx[WINDOW_SIZE] = { 0 };
    int state = 0;
    struct sockaddr_in name;
    struct sockaddr_in send_addr;
    struct sockaddr_in from_addr;
    socklen_t from_len;
    struct hostent h_ent;
    struct hostent *p_h_ent;
    char my_name[NAME_LENGTH] = { '\0' };
    int ss, sr, bytes, num, nread, from_ip, host_num;
    fd_set mask;
    fd_set dummy_mask, temp_mask;
    char msg_buff[BUFF_SIZE];	//use to read from file
    char input_buf[80];
    int window_pos = 0;
    int start_pos = 0;
    int target_ip;
    int packets_to_fill = WINDOW_SIZE;
    int number_of_bytes_written = 0;
    struct timeval timeout;
    desc_t default_desc;
    desc_t window[WINDOW_SIZE];

    if (argc < 2)
    {
        perror ("rcv <loss_rate_percent>\n");
        exit (1);
    }

    // grab CLI
    int loss_rate = (int) strtol (argv[1], NULL, 0);

    // create a socket to rcv and send data on
    sr = socket (AF_INET, SOCK_DGRAM, 0);	/* socket for receiving and sending(udp) */
    if (sr < 0)
    {
        perror ("Ucast: socket");
        exit (1);
    }

    sendto_dbg_init(loss_rate);

    // set essential socket properties
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons (PORT);

    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = 0;
    send_addr.sin_port = htons (PORT);

    // bind our receive socket to PORT
    if (bind (sr, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
        perror ("Ucast: bind");
        exit (1);
    }

    FD_ZERO (&mask);
    FD_ZERO (&dummy_mask);
    FD_SET (sr, &mask);

    default_desc.header.size = 0;
    default_desc.header.type = PACKET_TYPE_HELLO;
    default_desc.header.id = 0;
    // main app loop
    while (1)
    {

        /*STATE_0 the reciever waits for a hello packet.  When recieved move to state 2. */
        if (state == 0)		//current_state == STATE_0)      //needs strcmp instead.  int compare is better
        {
            while (state == 0)
            {
                target_ip = rx_desc_probe (&default_desc, sr);
                if (!target_ip) printf (".");
                else
                {
                    int header_type = default_desc.header.type;
                    if (header_type == PACKET_TYPE_HELLO)
                    {
                        printf ("\ngot a hello packet\n");
                        tx_desc (&default_desc, sr, target_ip);
                        default_desc.p_data[default_desc.header.size] = 0;
                        if ((fw = fopen (default_desc.p_data, "w+")) == NULL)
                        {
                            perror ("fopen");
                            exit (0);
                        }
                        state = 1;
                    }
                }
            }
        }
        else if (state == 1)
        {
            printf("Made communication with a client, receiving data...\n");
            while (state == 1)
            {
                int from_ip = rx_desc_probe (&default_desc, sr);
                int header_type = default_desc.header.type;

                if (header_type == PACKET_TYPE_HELLO)
                {
                    if (from_ip == target_ip)
                    {
                        default_desc.header.size = 0;
                        default_desc.header.type = PACKET_TYPE_HELLO;
                        default_desc.header.id = 0;
                        tx_desc (&default_desc, sr, target_ip);

                    }
                    else
                    {
                        default_desc.header.size = 0;
                        default_desc.header.type = PACKET_TYPE_BUSY;
                        default_desc.header.id = 0;
                        tx_desc (&default_desc, sr, from_ip);
                    }
                }
                else if (header_type == PACKET_TYPE_DATA)	// this is the ticket to start state two.
                {
                    if (from_ip == target_ip)
                    {
                        int id = default_desc.header.id;
                        window[id].header.size = default_desc.header.size;
                        window[id].header.type = default_desc.header.type;
                        window[id].header.id = default_desc.header.id;
                        memcpy (window[id].p_data, default_desc.p_data,
                        MAX_DATA_SIZE);
                        ack_idx[id] = 1;
                        state = 2;
                    }
                }
                else if (header_type == PACKET_TYPE_EOF)
                {
                    fwrite (default_desc.p_data, 1, default_desc.header.size, fw);
                    state = 10;
                }
            }
        }
        else if (state == 2)
        {
            struct timeval transfer_start;
    		gettimeofday( &transfer_start, NULL );
            int number_of_bytes_written = 0;
            printf ("entering state 2");
            while (state == 2)
            {
                int rec_num, proc_num;
                for (rec_num = 0; rec_num < SUB_WIN_SIZE; rec_num++)
                {
                    int from_ip = rx_desc_probe (&default_desc, sr);
                    int header_type = default_desc.header.type;
                    if (header_type == PACKET_TYPE_HELLO)
                    {
                        if (from_ip == target_ip)
                        {
                            fclose (fw);
                            window_pos = 0;
                            start_pos = 0;
                            state = 0;
                            break;
                        }
                        else	//send busy back to new client.
                        {
                            default_desc.header.size = 0;
                            default_desc.header.type = PACKET_TYPE_BUSY;
                            default_desc.header.id = 0;
                            tx_desc (&default_desc, sr, from_ip);
                        }
                    }
                    else if (header_type == PACKET_TYPE_DATA || header_type == PACKET_TYPE_EOF)	// this is the ticket to start state two.
                    {
                        if (from_ip == target_ip)
                        {
                            int id = default_desc.header.id;
                            window[id].header.size = default_desc.header.size;
                            window[id].header.type = default_desc.header.type;
                            window[id].header.id = default_desc.header.id;
                            memcpy (window[id].p_data, default_desc.p_data,
                            MAX_DATA_SIZE);
                            ack_idx[id] = 1;
                            ack_idx[(id + WINDOW_SIZE / 2) % WINDOW_SIZE] = 0;
                        }
                    }
                }
                int nacking = 0;
                int cur_win_pos = window_pos;
                for (int i = window_pos; i < window_pos + rec_num; i++)
                {
                    int array_pos = (i % WINDOW_SIZE);
                    if (ack_idx[array_pos] && !nacking)
                    {
                        int size = window[array_pos].header.size;
                        fwrite (window[array_pos].p_data, 1, size, fw);
                        number_of_bytes_written += size;
                        if (number_of_bytes_written >= BYTE_WRITE_LIM)
                        {
                            struct timeval transfer_current;
                            gettimeofday( &transfer_current, NULL );
                            printf("Processed 100MB. Time elapsed %i", transfer_current.tv_sec - transfer_start.tv_sec );
                            number_of_bytes_written = 0;
                        }
                        ack_idx[array_pos] = 0;
                        if (window[array_pos].header.type == PACKET_TYPE_EOF)
                        {
                            printf ("Final packet processed.\n");
                            fflush(stdout);
                            state = 10;
                            i = window_pos + rec_num;
                        }
                        window_pos = (window_pos + 1) % WINDOW_SIZE;
                    }
                    else
                    {
                        if (!nacking && i != cur_win_pos)	//send Ack for all contiguous
                        {
                            default_desc.header.size = 0;
                            default_desc.header.type = PACKET_TYPE_ACK;
                            default_desc.header.id = (i - 1) % WINDOW_SIZE;
                            tx_desc (&default_desc, sr, target_ip);
                        }
                        nacking = 1;
                        default_desc.header.size = 0;
                        default_desc.header.type = PACKET_TYPE_NACK;
                        default_desc.header.id = array_pos;
                        tx_desc (&default_desc, sr, target_ip);
                    }
                }
            }
        }

        else if (state == 10)
        {
            for (int i = 0; i < 5; i++)
            {
                default_desc.header.size = 0;
                default_desc.header.type = PACKET_TYPE_BYE;
                default_desc.header.id = 0;
                tx_desc (&default_desc, sr, target_ip);
            }
            fclose (fw);
            memset(&window, 0, sizeof(desc_t)*WINDOW_SIZE);
            printf ("Final packet sent to the client.\n");
            memset(&ack_idx, 0, sizeof(int)*WINDOW_SIZE);
            window_pos = 0;
            start_pos = 0;
            state = 0;
            target_ip = 0;
        }

    }

    return 0;

}
