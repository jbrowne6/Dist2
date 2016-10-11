#include "proto.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>

#define BUSY_TIMEOUT 3		//seconds
//sender
int main (int argc, char **argv)
{
    /* required for reading from file. */
    FILE *fr;			/* Pointer to source file, which we read */
    char buf[BUFF_SIZE];
    int state = 0;
    int eof = 0;
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
    char input_buf[80];
    int window_pos = 0;
    int start_pos = 0;
    //int packets_to_fill = WINDOW_SIZE;
    struct timeval timeout;

    desc_t default_desc;
    memset (&default_desc, 0, sizeof (desc_t));

    if (argc < 5)
    {
    perror
    ("ncp <loss_rate_percent> <source_file_name> <dest_file_name> <host_name>\n \
    where <comp_name> is the name of the computer where the server runs \
    (ugrad1, ugrad2, etc.)");
    exit (1);
    }

    // grab CLI
    int loss_rate = (int) strtol (argv[1], NULL, 0);
    char *source_file_name = argv[2];
    char *dest_file_name = argv[3];
    char *host_name = argv[4];

    sendto_dbg_init(loss_rate);

    /* Open the source file for reading */
    if ((fr = fopen (source_file_name, "r")) == NULL)
    {
    perror ("fopen");
    exit (0);
    }
    printf ("Opened %s for reading...\n", source_file_name);

    // create a socket to rcv and send data on
    sr = socket (AF_INET, SOCK_DGRAM, 0);	/* socket for receiving and sending(udp) */
    if (sr < 0)
    {
    perror ("Ucast: socket");
    exit (1);
    }

    // set essential socket properties
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons (PORT);

    // bind our receive socket to PORT
    if (bind (sr, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
    perror ("Ucast: bind");
    exit (1);
    }

    // do a dns lookup on host_name
    p_h_ent = gethostbyname (host_name);
    if (p_h_ent == NULL)
    {
    printf ("Ucast: gethostbyname error.\n");
    exit (1);
    }

    // copy over essential data from our dns lookup
    memcpy (&h_ent, p_h_ent, sizeof (h_ent));
    memcpy (&host_num, h_ent.h_addr_list[0], sizeof (host_num));

    // set our tx socket properties
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = host_num;
    send_addr.sin_port = htons (PORT);

    // zero out our masks later used for select
    // and set our receive socket in the mask
    FD_ZERO (&mask);
    FD_ZERO (&dummy_mask);
    FD_SET (sr, &mask);

    desc_t hello_descriptor = default_desc;
    memcpy (hello_descriptor.p_data, dest_file_name, strlen (dest_file_name));
    hello_descriptor.header.size = strlen (dest_file_name);
    hello_descriptor.header.type = PACKET_TYPE_HELLO;


    desc_t window[WINDOW_SIZE];
    memset (window, 0, WINDOW_SIZE * sizeof (desc_t));

    int finished = 0;

    // main app loop
    while (!finished)
    {

        /*STATE_0 sends a hello then waits for (1) A Hello back.  (2) a busy.  (3) time to run out. */
        if (state == 0)		//current_state == STATE_0)      //needs strcmp instead.  int compare is better
        {
            printf("Checking if host is running...");
            tx_desc (&hello_descriptor, sr, host_num);
            state = 1;
        }
        else if (state == 1)
        {
            printf("Host found!\nBeginning transfer...");
            fflush(stdout);
            desc_t rx_desc = default_desc;
            int rx = rx_desc_probe (&rx_desc, sr);
            if (rx)
            {
                if (rx_desc.header.type == PACKET_TYPE_HELLO)
                {
                    state = 2;
                }
                else if (rx_desc.header.type == PACKET_TYPE_BUSY)
                {
                    //should make sure hello is from the right address.
                    printf ("recieved a busy trying again in 10 seconds");
                    sleep (10);
                    state = 0;
                }
            }
            else
            {
                //we've timed out
                printf ("timed out, or received from another host.");
                fclose(fr);
                fflush (stdout);
                return 0;
            }
        }
        else if (state == 2)
        {
            //loop over the whole file, reading MAX_BUFF_SIZE
            // int i;
            char buffer[BUFF_SIZE];
            while (((window_pos + 1) % WINDOW_SIZE != (start_pos + WINDOW_SIZE / 2)) && !eof)
            {
                int type = PACKET_TYPE_DATA;
                nread = fread (buf, 1, MAX_DATA_SIZE, fr);	//pull in a chunk of the file
                if (nread < MAX_DATA_SIZE)
                {
                    if (feof (fr))
                    {
                        eof = 1;
                        type = PACKET_TYPE_EOF;
                        fclose(fr);
                        printf ("EOF reached");
                    }
                    else
                    {
                        printf ("error while reading file");
                        fclose(fr);
                        exit (0);
                    }
                }
                window[window_pos].header.type = type;
                window[window_pos].header.size = nread;
                window[window_pos].header.id = window_pos;
                memcpy (window[window_pos].p_data, buf, nread);
                tx_desc (&window[window_pos], sr, host_num);
                window_pos = (window_pos + 1) % WINDOW_SIZE;
            }
            int num_timed_out = 0;
            for (int m = 0; m < 100; m++)
            {

                desc_t rx_desc = default_desc;
                int rx = rx_desc_probe (&rx_desc, sr);
                if (rx)
                {
                    num_timed_out = 0;
                    if (rx_desc.header.type == PACKET_TYPE_ACK)
                    {
                        start_pos = (rx_desc.header.id + 1) % WINDOW_SIZE;
                    }
                    else if (rx_desc.header.type == PACKET_TYPE_NACK)
                    {
                        default_desc.header.type =
                        window[rx_desc.header.id].header.type;
                        default_desc.header.size =
                        window[rx_desc.header.id].header.size;
                        default_desc.header.id =
                        window[rx_desc.header.id].header.id;
                        memcpy (default_desc.p_data,
                        window[rx_desc.header.id].p_data,
                        MAX_DATA_SIZE);
                        tx_desc (&default_desc, sr, host_num);
                    }
                    else if (rx_desc.header.type == PACKET_TYPE_BYE)
                    {
                        state = 3;
                        break;
                    }
                }
                else num_timed_out++;



                if (num_timed_out == 3)
                {
                    //we've timed out
                    printf ("Server reliable timeout, quitting!\n");
                    fflush(stdout);
                    eof = 0;
                    finished = 1;
                    return 0;
                }
            }

        }
        else if (state == 3)
        {
            break;
        }
    }

    printf ("Finished file transfer\n");
    FD_CLR (sr, &mask);
    close (sr);
    return 0;

}
