#include "mfs.h"
#include "packets.h"
#include "cNetworkLib.h"
#include "stdlib.h"

char* serverHostname;
int serverPort;
int initialized = 0;

int MFS_Init(char *hostname, int port) {
	serverHostname = malloc(strlen(hostname) + 1);
	strcpy(serverHostname, hostname);
	serverPort = port;
	initialized = 1;
}

int MFS_Lookup(int pinum, char *name){
	if(!initialized)
		return -1;

	Net_Packet sendPacket;
	Net_Packet responsePacket;

	sendPacket.inum = pinum;
	strcpy(&(sendPacket.name), name);

	if(sendPacket(serverHostname, serverPort, &sendPacket, &responsePacket, 3) < 0)
		return -1;
	
	int rc = responsePacket.inum;
	return rc;
}

int MFS_Stat(int inum, MFS_Stat_t *m) {
	if(!initialized)
		return -1;

}

int MFS_Write(int inum, char *buffer, int block){
	if(!initialized)
		return -1;
}

int MFS_Read(int inum, char *buffer, int block){
	if(!initialized)
		return -1;
}

int MFS_Creat(int pinum, int type, char *name){
	if(!initialized)
		return -1;
}

int MFS_Unlink(int pinum, char *name){
	if(!initialized)
		return -1;
}
