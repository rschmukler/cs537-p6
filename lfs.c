#include <sys/types.h>
#include <unistd.h>
#include "mfs.h"

#define NBLOCKS 	14				// max number of blocks per inode
#define NINODES 	4096			// max number of inodes in system
#define CRSIZE		999999		// size (in blocks) of checkpoint region TODO
#define BLOCKSIZE	4096			// size (in bytes) of one block
#define DIRENTRYSIZE	32		// size (in bytes) of a directory entry
#define NENTRIES	(BLOCKSIZE/DIRENTRYSIZE)	// number of entries per block in a directory
#define NAMELENGTH	28			// length (in bytes) of a directory entry name

int FILE = 0;
int DIR = 1;
//int INVALID = 2;

typedef struct __inode {
	int inum;
	int size;								// number of bytes in the file. a multiple of BLOCKSIZE
	int type;
	bool used[NBLOCKS];			// used[i] is true if blocks[i] is used
	int blocks[NBLOCKS];		// address in memory of each block
} inode;

typedef struct __dirBlock {
	char names[ENTRIESPERBLOCK][NAMELENGTH];
	int  inums[ENTRIESPERBLOCK];
} dirBlock;

int imap[NINODES];			// block number of each inode
int nextBlock;					// next block in the address space to be written
int fd;										// the file descriptor of the LFS

int get_inode(int inum, inode* n) {
	
	if(inum < 0 || inum >= NINODES)		// check for invalid inum
		return -1;
	
	int iblock = imap[inum];					// block where desired inode is written
	
	lseek(fd, CRSIZE+iblock*BLOCKSIZE, SEEK_SET);
	read(fd, n, sizeof(inode));

	return 0;
}

int main()
{

}

int Server_Startup() {
	
}

int Server_Lookup(int pinum, char *name) {
	
	inode parent;
	if(get_inode(pinum, &parent) == -1)
		return -1;

	for(int b = 0; b < NBLOCKS; b++)
	{
		if(parent.used[b])
		{
			dirBlock block;
			lseek(fd, parent.block[b], SEEK_SET);
			read(fd, block, BLOCKSIZE);

			for(int e = 0; e < NENTRIES; e++)
			{
				if(block.inums[e] != -1)
				{
					if(strcmp(name, &block.names[e]) == 0)
					{
						return block.inums[e];
					}
				}
			}
		}
	}

	return -1;
}

int Server_Stat(int inum, MFS_Stat_t *m) {
	
	inode n;
	if(get_inode(inum, &n) == -1)
		return -1;

	m->type = n.type;
	m->size = n.size;

	return 0;
}

int Server_Write(int inum, char *buffer, int block) {
	
	inode n; 
	if(get_inode(inum, &n) == -1)
		return -1;
	
	if(n.type != FILE)								// can't write to directory
		return -1;

	if(block < 0 || block >= NBLOCKS)	// check for invalid block
		return -1;
	
	// write inode chunk
	lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
	write(fd, &n, BLOCKSIZE);
	imap[inum] = nextBlock;
	nextBlock++;
	
	// write buffer
	lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
	write(fd, buffer, BLOCKSIZE);
	nextBlock++;

	// write checkpoint region
	lseek(fd, inum*sizeof(int), SEEK_SET);		// update inode table
	write(fd, &imap[inum], sizeof(int));
	lseek(fd, NINODES*sizeof(int), SEEK_SET);	// update nextBlock
	write(fd, &nextBlock, sizeof(int));

	// update file size
	if(!n.used[block])
	{
		n.used[block] = true;
		n.size += BLOCKSIZE;
	}

	return 0;
}

int Server_Read(int inum, char *buffer, int block){
	
	inode n;
	if(get_inode(inum, &n) == 0)
		return -1;

	if(block < 0 || block >= NBLOCKS || !n.used[block])		// check for invalid block
		return -1;

	// read
	if(n.type == FILE)																		// read regular file
	{
		lseek(fd, n.blocks[block], SEEK_SET);
		read(fd, buffer, BLOCKSIZE);
	}
	else																									// read directory
	{
		dirBlock db;																				// read dirBlock
		lseek(fd, n.blocks[block], SEEK_SET);
		read(fd, db, BLOCKSIZE);

		MRS_DirEnt_t entries[NENTRIES];											// convert dirBlock to MRS_DirEnt_t
		for(int i = 0; i < NENTRIES; i++)
		{
			MRS_DirEnt_t* entry;
			entry->name = db.names[i];
			entry->inum = db.inums[i];
			entries[i] = entry;
		}
	}

	return 0;
}

int Server_Creat(int pinum, int type, char *name){
	
	if(Server_Lookup(pinum, name) != -1)					// if the file already exists, return success
		return 0;

	inode parent;
	if(get_inode(pinum, &parent) == -1)
		return -1;

	if(parent.type != DIR)												// if parent directory is not a directory, return failure
		return -1;
	
	
}

int Server_Unlink(int pinum, char *name){
	
}
