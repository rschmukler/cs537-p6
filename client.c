#include "mfs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	if(argc != 3)
		exit(1);
	
	char *hostname = argv[1];
	int port = atoi(argv[2]);

	MFS_Init(hostname, port);
	char buffer[MFS_BLOCK_SIZE];
	MFS_Creat(0, MFS_REGULAR_FILE, "TestFile");
	int inode = MFS_Lookup(0, "TestFile");
	printf("Got Inode #: %d\n", inode);
	strcpy(buffer, "This is some stuff!");
	MFS_Write(inode, buffer, 0);

	if(MFS_Read(inode, buffer, 0) > -1)
		printf("Read Message from server:\n%s\n", buffer);

	MFS_Shutdown();

	return 0;
}