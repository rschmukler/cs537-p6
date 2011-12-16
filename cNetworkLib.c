#include <sys/select.h>
#include "packets.h"

int sendPacket(char* hostname, int port, Net_Packet* sendPacket, Net_Packet* responsePacket, int maxTries)
{
	if(maxTries == 0)
	{
		perror("Error connecting to Server\n");
		return -1;
	}
	int sd = UDP_Open(0);
    if(sd < -1)
    {
    	perror("Error opening connection.\n");
    	return -1
    }

    struct sockaddr_in addr, addr2;
    int rc = UDP_FillSockAddr(&addr, hostname, port);
    if(rc < 1)
    {
    	perror("Error looking up host.\n");
    	return -1;
    }

    fd_set rfds;
    struct timeval tv;
    tv.tv_sec=3;
    tv.tv_usec=0;
    FD_ZERO(&rfds);
    FD_SET(sd,&rfds);
    rc = UDP_Write(sd, &addr, sendPacket, sizeof(Net_Packet));
    if (rc > 0) {
	    while(select(fd+1,&rfds,0,0,&tv))
	    {
	    	int rc = UDP_Read(sd, &addr2, responsePacket, sizeof(Net_Packet));
	    }

	    if(rc == -1)
	    {
	    	return sendPacket(hostname, port, sendPacket, responsePacket, maxTries - 1);
	    }
	    return 0;
	}	
}