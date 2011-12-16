#include "mfs.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	if(argc != 3)
		exit(1);
	
	char *hostname = argv[1];
	int port = atoi(argv[2]);

	MFS_Init(hostname, port);
	char buffer[MFS_BLOCK_SIZE];
	if(MFS_Read(0, buffer, 100) > -1)
		printf("Read Message from server:\n %s\n", buffer);

	return 0;
}