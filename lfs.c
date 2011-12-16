#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "mfs.h"
#include "lfs.h"

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

// Returns block number of new block
// pinum is unused if firstBlock == 0
int build_dir_block(int firstBlock, int inum, int pinum)
{
	dirBlock db;
	int i;
	for(i = 0; i < NENTRIES; i++)
	{
		db.inums[i] = -1;
	}

	if(firstBlock)
	{
		db.inums[0] = inum;
		strcpy(db.names[0], ".");
		db.inums[1] = pinum;
		strcpy(db.names[1], "..");
	}

	// write new block
	lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
	write(fd, &db, BLOCKSIZE);
	nextBlock++;

	return nextBlock-1;
}

void update_CR(int dirty_inum)
{
	if(dirty_inum != -1)
	{
		lseek(fd, dirty_inum*sizeof(int), SEEK_SET);		// update inode table
		write(fd, &imap[dirty_inum], sizeof(int));
	}

	lseek(fd, NINODES*sizeof(int), SEEK_SET);	// update nextBlock
	write(fd, &nextBlock, sizeof(int));
}


int Server_Startup(int port, char* path) {
	
	if((fd = open(path, O_RDWR)) == -1)
	{
		// create new file system
		fd = creat(path, O_RDWR);
		nextBlock = CRSIZE*BLOCKSIZE;
		
		int i;
		for(i = 0; i < NINODES; i++)
		{
			imap[i] = -1;
		}

		lseek(fd, 0, SEEK_SET);
		write(fd, imap, sizeof(int)*NINODES);
		write(fd, &nextBlock, sizeof(int));

		return 0;
	}
	else
	{
		lseek(fd, 0, SEEK_SET);
		read(fd, imap, sizeof(int)*NINODES);
		read(fd, &nextBlock, sizeof(int));
	}

	serverListen(port);
}

int Server_Lookup(int pinum, char *name) {
	
	inode parent;
	if(get_inode(pinum, &parent) == -1)
		return -1;

	int b;
	for(b = 0; b < NBLOCKS; b++)
	{
		if(parent.used[b])
		{
			dirBlock block;
			lseek(fd, parent.blocks[b], SEEK_SET);
			read(fd, &block, BLOCKSIZE);

			int e;
			for(e = 0; e < NENTRIES; e++)
			{
				if(block.inums[e] != -1)
				{
					if(strcmp(name, block.names[e]) == 0)
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
	
	if(n.type != MFS_REGULAR_FILE)								// can't write to directory
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
	update_CR(inum);

	// update file size
	if(!n.used[block])
	{
		n.used[block] = 1;
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
	if(n.type == MFS_REGULAR_FILE)																		// read regular file
	{
		lseek(fd, n.blocks[block], SEEK_SET);
		read(fd, buffer, BLOCKSIZE);
	}
	else																									// read directory
	{
		dirBlock db;																				// read dirBlock
		lseek(fd, n.blocks[block], SEEK_SET);
		read(fd, &db, BLOCKSIZE);

		MFS_DirEnt_t entries[NENTRIES];											// convert dirBlock to MRS_DirEnt_t
		int i;
		for(i = 0; i < NENTRIES; i++)
		{
			MFS_DirEnt_t* entry;
			strcpy(entry->name, db.names[i]);
			entry->inum = db.inums[i];
			entries[i] = *entry;
		}

		buffer = (void*)entries;
	}

	return 0;
}

int Server_Creat(int pinum, int type, char *name){
	
	if(Server_Lookup(pinum, name) != -1)					// if the file already exists, return success
		return 0;

	inode parent;
	if(get_inode(pinum, &parent) == -1)
		return -1;

	if(parent.type != MFS_DIRECTORY)												// if parent directory is not a directory, return failure
		return -1;
	
	// find lowest available inum
	int inum = -1;
	int i;
	for(i = 0; i < NINODES; i++)
	{
		if(imap[i] == -1)
		{
			inum = i;
			break;
		}
	}

	if(inum == -1)			// if more than NINODES inodes exist, return failure
		return -1;

	// put inode into parent directory
	int b, e; dirBlock block;
	for(b = 0; b < NBLOCKS; b++)
	{
		if(parent.used[b])
		{
			lseek(fd, parent.blocks[b], SEEK_SET);
			read(fd, &block, BLOCKSIZE);

			for(e = 0; e < NENTRIES; e++)
			{
				if(block.inums[e] == -1)
				{
					goto found_parent_slot;
				}
			}
		}
		else
		{
			// make new block, then repeat loop on this block
			int block = build_dir_block(0, inum, -1);
			parent.size += BLOCKSIZE;

			parent.used[b] = true;
			parent.blocks[b] = block;
			b--;
		}
	}
	found_parent_slot:
	
	if(b == NBLOCKS)			// directory is full
		return -1;

	block.inums[e] = inum;
	strcpy(block.names[e], name);
	lseek(fd, parent.blocks[b], SEEK_SET);
	write(fd, &block, BLOCKSIZE);

	// create inode
	inode* n;
	n->inum = inum;
	n->size = 0;
	for(i = 0; i < NBLOCKS; i++)
	{
		n->used[i] = 0;
		n->blocks[i] = -1;
	}

	if(type == MFS_REGULAR_FILE)
	{
		n->type = type;	
	}
	else if(type == MFS_DIRECTORY)
	{
		n->type = type;
		n->used[0] = 1;
		n->blocks[0] = nextBlock;
		
		dirBlock baseBlock;
		dirBlock.inum[0] = inum;
		dirBlock.inum[1] = pinum;
		dirBlock.names[0] = ".";
		dirBlock.names[1] = "..";


		// write baseBlock
		lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
		write(fd, &baseBlock, BLOCKSIZE);
		nextBlock++;
		
		// update file size
		n->size += BLOCKSIZE;
	}
	else
	{
		return -1;
	}

	// update imap
	imap[inum] = nextBlock;

	// write inode
	lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
	write(fd, n, sizeof(inode));
	nextBlock++;

	// write checkpoint region
	update_CR(inum);

	return 0;
}

int Server_Unlink(int pinum, char *name){
	
	inode toRemove;					// to be removed
	inode parent;						// parent of toRemove

	if(get_inode(pinum, &parent) == -1)			// parent directory doesn't exist; return failure
		return -1;

	int inum = Server_Lookup(pinum, name);	// inum of toRemove
	if(get_inode(inum, &toRemove) == -1)		// toRemove doesn't exist; return success
		return 0;

	// if toRemove is a directory, make sure it's empty
	if(toRemove.type == MFS_DIRECTORY)
	{
		int b;
		for(b = 0; b < NBLOCKS; b++)
		{
			if(toRemove.used[b])
			{
				dirBlock block;
				lseek(fd, toRemove.blocks[b], SEEK_SET);
				read(fd, &block, BLOCKSIZE);

				int e;
				for(e = 0; e < NENTRIES; e++)
				{
					if(block.inums[e] != -1 && strcmp(block.names[e], ".") != 0 && strcmp(block.names[e], "..") != 0)
					{
						return -1;	// found file in toRemove
					}
				}
			}
		}
	}
	
	// remove toRemove from parent
	int b;
	for(b = 0; b < NBLOCKS; b++)
	{
		if(parent.used[b])
		{
			dirBlock block;
			lseek(fd, toRemove.blocks[b], SEEK_SET);
			read(fd, &block, BLOCKSIZE);

			int e;
			for(e = 0; e < NENTRIES; e++)
			{
				if(block.inums[e] != -1)
				{
					if(strcmp(name, block.names[e]) = 0)
					{
						block.inums[e] = -1;
					}
				}
			}
		}
	}

	// remove toRemove from CR
	imap[inum] = -1;
	update_CR(inum);


	return 0;
}

int MFS_Shutdown()
{
	fsync(fd);			// not sure if this is necessary
	exit(0);
	return -1;	// if we reach this line of code, there was an error
}
