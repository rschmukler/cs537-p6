#include "mfs.h"
#include "packets.h"
#include "cNetworkLib.h"
#include "stdlib.h"

char* myHostname;
int myPort;
int initialized = 0;

int MFS_Init(char *hostname, int port) {
	myHostname = malloc(strlen(hostname) + 1);
	strcpy(myHostname, hostname);
	myPort = port;
	initialized = 1;
}

int MFS_Lookup(int pinum, char *name){
	if(!initialized)
		return -1;
	
	
}

int MFS_Stat(int inum, MFS_Stat_t *m) {
	
}

int MFS_Write(int inum, char *buffer, int block){
	
}

int MFS_Read(int inum, char *buffer, int block){
	
}

int MFS_Creat(int pinum, int type, char *name){
	
}

int MFS_Unlink(int pinum, char *name){
	
}