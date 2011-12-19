#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "udp.h"
#include "packets.h"

int sendPacket(char *hostname, int port, Net_Packet *sentPacket, Net_Packet *responsePacket, int maxTries)
{
    int sd = UDP_Open(0);
    if(sd < -1)
    {
        perror("Error opening connection.\n");
        return -1;
    }

    struct sockaddr_in addr, addr2;
    int rc = UDP_FillSockAddr(&addr, hostname, port);
    if(rc < 0)
    {
        perror("Error looking up host.\n");
        return -1;
    }

    fd_set rfds;
    struct timeval tv;
    tv.tv_sec=3;
    tv.tv_usec=0;


    do {
        FD_ZERO(&rfds);
        FD_SET(sd,&rfds);
        UDP_Write(sd, &addr, (char*)sentPacket, sizeof(Net_Packet));
        if(select(sd+1, &rfds, NULL, NULL, &tv))
        {
            rc = UDP_Read(sd, &addr2, (char*)responsePacket, sizeof(Net_Packet));
            if(rc > 0)
            {
                UDP_Close(sd);
                return 0;
            }
        }else {
            maxTries -= 1;
        }
    }while(1);
}
