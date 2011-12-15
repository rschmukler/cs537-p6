#ifndef __PACKETS_H__
#define __PACKETS_H__
#define BUFFER_SIZE (4096)
#define MAX_NAME_SIZE (28)
#ifndef __MFS_h__
typedef struct __MFS_Stat_t {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    // note: no permissions, access times, etc.
} MFS_Stat_t;
#endif


enum message {
	init,
	lookup,
	stat,
	write,
	read,
	creat,
	unlink,
	response
};

typedef struct __Net_Packet {
	message message;
	int inum;
	int block;
	int type;

	char name[MAX_NAME_SIZE];
	char buffer[BUFFER_SIZE];
	MFS_Stat_t stat;
} Net_Packet;
#endif