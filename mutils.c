////////////////////////////////////////////////////////////////////////////////
//
//    James D. Browne
//    Eric
//    Distributed Assignment 2
//
//    mutils.c
//    Functions required by mcast.c
//    Date: 2016/10/10
//
////////////////////////////////////////////////////////////////////////////////

void tx_desc(desc_t* desc, int sr, int addr)
{
    struct sockaddr_in send_addr;
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = addr;
    send_addr.sin_port = htons (PORT);

    int32_t header_pack = 0;
    header_pack |= (desc->header.type << PACKET_TYPE_BIT) | (desc->header.size << PACKET_SIZE_BIT) |
            (desc->header.id << PACKET_ID_BIT);
    char buff[sizeof(desc->header) + desc->header.size];
    memcpy(buff, &header_pack, 4); //copy the header, 4 is size of int32_t
    memcpy(buff+4, desc->p_data, desc->header.size); //copy the data
    sendto_dbg(sr, buff, BUFF_SIZE, 0, (struct sockaddr *) &send_addr,
                sizeof (send_addr));
}

int rx_desc_probe(desc_t* desc, int sr)
{

    fd_set mask, temp_mask, dummy_mask;
    FD_ZERO (&mask);
    FD_ZERO (&dummy_mask);
    FD_SET (sr, &mask);
    struct timeval timeout;
    struct sockaddr_in from_addr;
    char msg_buff[BUFF_SIZE];

    while (1)
    {
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;
        temp_mask = mask;
        int num = select (FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0)
        {
            if (FD_ISSET (sr, &temp_mask))
            {
                socklen_t from_len = sizeof (from_addr);
                int bytes = recvfrom (sr, msg_buff, sizeof (msg_buff), 0,
                        (struct sockaddr *) &from_addr, &from_len);
                int32_t header = ((int32_t*) msg_buff)[0];
                desc->header.type = (header & PACKET_TYPE) >> PACKET_TYPE_BIT;
                desc->header.size = (header & PACKET_SIZE) >> PACKET_SIZE_BIT;
                desc->header.id = (header & PACKET_ID) >> PACKET_ID_BIT;
                memcpy(desc->p_data, msg_buff+4, desc->header.size);
                return from_addr.sin_addr.s_addr;
            }
        }
        else
        {
            return 0;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//  Revsion History
//
//
////////////////////////////////////////////////////////////////////////////////
