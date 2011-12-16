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
	PAK_LOOKUP,
	PAK_STAT,
	PAK_WRITE,
	PAK_READ,
	PAK_CREAT,
	PAK_UNLINK,
	PAK_RESPONSE,
	PAK_SHUTDOWN
};

typedef struct __Net_Packet {
	enum message message;
	int inum;
	int block;
	int type;

	char name[MAX_NAME_SIZE];
	char buffer[BUFFER_SIZE];
	MFS_Stat_t stat;
} Net_Packet;
#endif