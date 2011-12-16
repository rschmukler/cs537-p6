#ifndef __LFS_H__
#define __LFS_H__
#define NBLOCKS 	14				// max number of blocks per inode
#define NINODES 	4096			// max number of inodes in system
#define CRSIZE		999999		// size (in blocks) of checkpoint region TODO
#define BLOCKSIZE	4096			// size (in bytes) of one block
#define DIRENTRYSIZE	32		// size (in bytes) of a directory entry
#define NENTRIES	(BLOCKSIZE/DIRENTRYSIZE)	// number of entries per block in a directory
#define NAMELENGTH	28			// length (in bytes) of a directory entry name

typedef struct __inode {
	int inum;
	int size;								// number of bytes in the file. a multiple of BLOCKSIZE
	int type;
	int used[NBLOCKS];			// used[i] is true if blocks[i] is used
	int blocks[NBLOCKS];		// address in memory of each block
} inode;

typedef struct __dirBlock {
	char names[NENTRIES][NAMELENGTH];
	int  inums[NENTRIES];
} dirBlock;

#ifndef __MFS_H__
#ifndef __PACKETS_H__
typedef struct __MFS_Stat_t {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    // note: no permissions, access times, etc.
} MFS_Stat_t;
typedef struct __MFS_DirEnt_t {
    char name[28];  // up to 28 bytes of name in directory (including \0)
    int  inum;      // inode number of entry (-1 means entry not used)
} MFS_DirEnt_t;
#endif
#endif

int get_inode(int inum, inode* n);
void build_dir_block(int firstBlock, int inum, int pinum);
void update_CR(int dirty_inum);
int Server_Startup();
int Server_Lookup(int pinum, char *name);
int Server_Stat(int inum, MFS_Stat_t *m);
int Server_Write(int inum, char *buffer, int block);
int Server_Read(int inum, char *buffer, int block);
int Server_Creat(int pinum, int type, char *name);
int Server_Unlink(int pinum, char *name);
int Server_Shutdown();
#endif
