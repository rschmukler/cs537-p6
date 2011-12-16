#include <stdio.h>
#include "udp.h"
#include "packets.h"

void serverListen(int port)
{
	int sd = UDP_Open(port);
	if(sd < 0)
	{
		perror("Error opening socket on port %d\n", port)
		exit(1);
	}

    printf("Starting server...\n");

    while (1) {
		struct sockaddr_in s;
		Net_Packet packet;
		int rc = UDP_Read(sd, &s, packet, sizeof(Net_Packet));
		if (rc > 0) {
		    Net_Packet responsePacket;

		    switch(packet.message){
		    	case init:
		    		break;
		    		
		    	case lookup:
		    		responsePacket.inum = Server_Lookup(packet.inum, packet.name);
		    		break;

		    	case stat:
		    		responsePacket.inum = Server_Stat(packet.inum, &(responsePacket.stat);
		    		break;

		    	case write:
		    		responsePacket.inum = Server_Write(packet.inum, packet.buffer, packet.block);
		    		break;

		    	case read:
		    		responsePacket.inum = Server_Read(packet.inum, responsePacket.buffer, packet.block);
		    		break;

		    	case creat:
		    		responsePacket.inum = Server_Creat(packet.inum, packet.type, packet.name);
		    		break;

		    	case unlink:
		    		responsePacket.inum = Server_Unlink(packet.inum, packet.name);
		    		break;

		    }
		    responsePacket.message = PAK_RESPONSE;
		    if(packet.message = read)
		    	

		    rc = UDP_Write(sd, &s, responsePacket, sizeof(Net_Packet));
		}
	}
}