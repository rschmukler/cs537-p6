
#include "mfs.h"

#define NBLOCKS 	14				// max number of blocks per inode
#define NINODES 	4096			// max number of inodes in system
#define CRSIZE		999999		// size (in blocks) of checkpoint region
#define BLOCKSIZE	4096			// size (in bytes) of one block
#define DIRENTRYSIZE	32		// size (in bytes) of a directory entry
#define ENTRIESPERBLOCK	(BLOCKSIZE/DIRENTRYSIZE)	// number of entries per block in a directory
#define NAMELENGTH	28			// length (in bytes) of a directory entry name

int FILE = 0;
int DIR = 1;
int INVALID = 2;

typedef struct __inode {
	int inum;
	int size;
	int type;
	int blocks[NBLOCKS];		// address in memory of each block
} inode;

typedef struct __dirBlock {
	char names[ENTRIESPERBLOCK][NAMELENGTH];
	int  inums[ENTRIESPERBLOCK];
}

int imap[NINODES];				// the block number of each inode
int nextBlock;							// the next block in the file to be written
int fd;										// the file descriptor of the LFS

int get_inode(int inum, inode* n) {
	
	if(inum < 0 || inum >= NINODES)		// check for invalid inum
		return -1;
	
	int iblock = imap[inum];					// block where desired inode is written
	
	lseek(fd, CRSIZE+iblock*BLOCKSIZE, SEEK_SET);
	read(fd, n, sizeof(inode));

	return 0;
}

int Server_Startup() {
	
}

int Server_Lookup(int pinum, char *name) {
	
}

int Server_Stat(int inum, MFS_Stat_t *m) {
	
}

int Server_Write(int inum, char *buffer, int block) {
	
	inode n = get_inode(inum);
	
	if(n.type != FILE)								// check for unused inum or directory
		return -1;

	if(block < 0 || block >= NBLOCKS)	// check for invalid block
		return -1;
	
	// write inode chunk
	lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
	write(fd, n, BLOCKSIZE);
	imap[inum] = nextBlock;
	nextBlock++;
	
	// write buffer
	lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
	write(fd, buffer, BLOCKSIZE);
	nextBlock++;

	// write checkpoint region
	lseek(fd, inum*sizeof(int), SEEK_SET);
	write(fd, &imap[inum], sizeof(int));
	return 0;
}

int Server_Read(int inum, char *buffer, int block){
	
}

int Server_Creat(int pinum, int type, char *name){
	
}

int Server_Unlink(int pinum, char *name){
	
}
