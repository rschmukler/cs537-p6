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
	MFS_Creat(0, MFS_DIRECTORY, "TestDir");
	int inode = MFS_Lookup(0, "TestDir");
	int i = 0;
	for(;i < 1800; ++i)
	{
		char buffer[2000];
		sprintf(buffer, "file_%d", i);
		int ret = MFS_Creat(inode, MFS_REGULAR_FILE, buffer);
		if(ret == 0)
		{
			printf("Successfully created: %s", buffer);
		}else {
			printf("Failed to create: %s", buffer);
		}
	}
	/*printf("Got Inode #: %d\n", inode);
	strcpy(buffer, "This is some stuff!");
	MFS_Write(inode, buffer, 0);

	if(MFS_Read(inode, buffer, 0) > -1)
		printf("Read Message from server:\n%s\n", buffer);*/

	MFS_Shutdown();

	return 0;
}