#include "lfs.h"
#include "sNetworkLib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	if(argc != 3) {
		printf("Usage: server [portnum] [file-system image]\n");
		exit(1);
	}

	int portNumber = atoi(argv[1]);
	char *fileSysPath = argv[2];

	Server_Startup(portNumber, fileSysPath);

	return 0;
}


/*//REMOVE AFTER TESTING
int Server_Startup() {
	return 0;
}
int Server_Lookup(int pinum, char *name){
	return 0;	
}
int Server_Stat(int inum, MFS_Stat_t *m){
	return 0;
}
int Server_Write(int inum, char *buffer, int block){
	return 0;
}
int Server_Read(int inum, char *buffer, int block){
	printf("Got read request: \n\tInum: %d\n\tBlock: %d\n", inum, block);
	char toReturn[] = "HelloWorld from the wonderful world of sockets!";
	strcpy(buffer, toReturn);
	return 0;
}
int Server_Creat(int pinum, int type, char *name){
	return 0;
}
int Server_Unlink(int pinum, char *name){
	return 0;
}
int Server_Shutdown(){
	return 0;
}*/