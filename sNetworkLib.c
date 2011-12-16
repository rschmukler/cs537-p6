#include <stdio.h>
#include "udp.h"
#include "packets.h"
#include "lfs.h"

void serverListen(int port)
{
	int sd = UDP_Open(port);
	if(sd < 0)
	{
		printf("Error opening socket on port %d\n", port);
		exit(1);
	}

    printf("Starting server...\n");
    while (1) {
		struct sockaddr_in s;
		Net_Packet packet;
		int rc = UDP_Read(sd, &s, (char *)&packet, sizeof(Net_Packet));
		if (rc > 0) {
		    Net_Packet responsePacket;

		    switch(packet.message){
		    		
		    	case PAK_LOOKUP :
		    		responsePacket.inum = Server_Lookup(packet.inum, packet.name);
		    		break;

		    	case PAK_STAT :
		    		responsePacket.inum = Server_Stat(packet.inum, &(responsePacket.stat));
		    		break;

		    	case PAK_WRITE :
		    		responsePacket.inum = Server_Write(packet.inum, packet.buffer, packet.block);
		    		break;

		    	case PAK_READ:
		    		responsePacket.inum = Server_Read(packet.inum, responsePacket.buffer, packet.block);
		    		break;

		    	case PAK_CREAT:
		    		responsePacket.inum = Server_Creat(packet.inum, packet.type, packet.name);
		    		break;

		    	case PAK_UNLINK:
		    		responsePacket.inum = Server_Unlink(packet.inum, packet.name);
		    		break;

		    	case PAK_SHUTDOWN:
		    		Server_Shutdown();
		    	
		    	case PAK_RESPONSE:
		    		break;
		    }

		    responsePacket.message = PAK_RESPONSE;

		    rc = UDP_Write(sd, &s, (char*)&responsePacket, sizeof(Net_Packet));
		}
	}
}