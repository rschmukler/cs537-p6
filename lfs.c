#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "mfs.h"
#include "lfs.h"
#include <stdio.h>
#include <assert.h>
#include <errno.h>


void print_inode(inode *n);
void print_dirBlock(int block);
void print_CR();
typedef struct __buf {
	char string [BLOCKSIZE/sizeof(char)];
} buf;
int newFS;

int imap[NINODES];			// block number of each inode
int nextBlock;					// next block in the address space to be written
int fd;										// the file descriptor of the LFS

int get_inode(int inum, inode* n) {
	
	if(inum < 0 || inum >= NINODES)		// check for invalid inum
	{
		printf("get_inode: invalid inum\n");
		return -1;
	}
	
	int iblock = imap[inum];					// block where desired inode is written
	
	lseek(fd, iblock*BLOCKSIZE, SEEK_SET);
	read(fd, n, sizeof(inode));

	//printf("get_inode: returning inode--\n");
	//print_inode(n);
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
		strcpy(db.names[i], "DNE\0");
	}

	if(firstBlock)
	{
		db.inums[0] = inum;
		strcpy(db.names[0], ".\0");
		db.inums[1] = pinum;
		strcpy(db.names[1], "..\0");
	}
	
	//print_dirBlock(nextBlock);
	// write new block
	lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
	write(fd, &db, BLOCKSIZE);
	nextBlock++;

	//print_dirBlock(nextBlock-1);

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
		newFS = 1;
		printf("CREATING NEW FILE SYSTEM\n\n");

		// create new file system
		fd = open(path, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU);
		if(fd == -1)
			return -1;
		nextBlock = CRSIZE;
		
		//printf("fd = %d\n", fd);

		int i;
		for(i = 0; i < NINODES; i++)
		{
			imap[i] = -1;
		}

		lseek(fd, 0, SEEK_SET);
		write(fd, imap, sizeof(int)*NINODES);
		write(fd, &nextBlock, sizeof(int));

		// create root
		//
		//printf("=================================\nRoot before doing anything:\n\n");
		//print_dirBlock(7);
		//printf("=================================\n\n\n\n\n");

		inode n;
		n.inum = 0;
		n.size = BLOCKSIZE;
		n.type = MFS_DIRECTORY;
		n.used[0] = 1;
		n.blocks[0] = nextBlock;
		for(i = 1; i < NBLOCKS; i++)
		{
			n.used[i] = 0;
			n.blocks[i] = -1;
		}

		dirBlock baseBlock;
		baseBlock.inums[0] = 0;
		baseBlock.inums[1] = 0;
		strcpy(baseBlock.names[0], ".\0");
		strcpy(baseBlock.names[1], "..\0");

		for(i = 2; i < NENTRIES; i++)
		{
			baseBlock.inums[i] = -1;
			strcpy(baseBlock.names[i], "DNE\0");
		}

		// write baseBlock
		lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
		write(fd, &baseBlock, sizeof(dirBlock));
		nextBlock++;
		
		// update imap
		imap[0] = nextBlock;

		// write inode
		lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
		write(fd, &n, sizeof(inode));
		nextBlock++;

		// write checkpoint region
		update_CR(0);

		//printf("=======================================\nRoot (block number %d) after creation:\n\n\n\n", imap[0]);
		//print_dirBlock(imap[0]);
		//printf("========================================\n\n\n\n");
	}
	else
	{
		newFS = 0;
		printf("USING OLD FILE SYSTEM\n\n");

		lseek(fd, 0, SEEK_SET);
		read(fd, imap, sizeof(int)*NINODES);
		read(fd, &nextBlock, sizeof(int));
	}

	//	TODO: remove comment here
	serverListen(port);
	//inode root;
	//get_inode(0, &root);
	//print_inode(&root);
	//print_dirBlock(root.blocks[0]);
	return 0;
}

int Server_Lookup(int pinum, char *name) {
	
	inode parent;
	if(get_inode(pinum, &parent) == -1)
	{
		////printf("get_inode(%d, &parent) failed in Server_Lookup.\n", pinum);
		return -1;
	}

	//print_inode(&parent);

	int b;
	for(b = 0; b < NBLOCKS; b++)
	{
		if(parent.used[b])
		{
			//printf("Checking block %d\n", b);
			dirBlock block;
			lseek(fd, parent.blocks[b]*BLOCKSIZE, SEEK_SET);
			read(fd, &block, BLOCKSIZE);

			//print_dirBlock(parent.blocks[b]);

			int e;
			for(e = 0; e < NENTRIES; e++)
			{
				//printf("File %s has inum %d\n", "***", block.inums[e]);
				if(block.inums[e] != -1)
				{
					////printf("Checking %s == %s\n", name, block.names[e]);
					if(strcmp(name, block.names[e]) == 0)
					{
						return block.inums[e];
					}
					//printf("Check failed.\n");
				}
			}
		}
	}

	//printf("Server_Lookup couldn't find file of name %s in directory with inum %d.\n", name, pinum);
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
	
	// update file size
	n.size = (block+1)*BLOCKSIZE > n.size ? (block+1)*BLOCKSIZE : n.size;
	n.used[block] = 1;

	/*if(!n.used[block])
	{
		n.used[block] = 1;
		n.size += BLOCKSIZE;
	}*/
	
	// inform inode of location of new block
	n.blocks[block] = nextBlock+1;

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
	return 0;
}

int Server_Read(int inum, char *buffer, int block){
	inode n;
	if(get_inode(inum, &n) == -1)
	{
		printf("get_inode failed for inum %d.\n", inum);
		return -1;
	}

	if(block < 0 || block >= NBLOCKS || !n.used[block])		// check for invalid block
	{
		printf("invalid block.\n");
		return -1;
	}

	// read
	if(n.type == MFS_REGULAR_FILE)																		// read regular file
	{
		//printf("Reading a file\n");
		if(lseek(fd, n.blocks[block]*BLOCKSIZE, SEEK_SET) == -1)
		{
			perror("Server_Read: lseek:");
			printf("Server_Read: lseek failed\n");
		}
		
		if(read(fd, buffer, BLOCKSIZE) == -1)
		{
			perror("Server_Read: read:");
			printf("Server_Read: read failed\n");
		}
		//printf("File reads: %s\n", buffer);
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
			MFS_DirEnt_t entry ;
			strcpy(entry.name, db.names[i]);
			entry.inum = db.inums[i];
			entries[i] = entry;
		}

		memcpy(buffer, entries, sizeof(MFS_DirEnt_t)*NENTRIES);
	}
	return 0;
}

int Server_Creat(int pinum, int type, char *name){
	//printf("Got Request with Type: %d\n", type);
	
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
			lseek(fd, parent.blocks[b]*BLOCKSIZE, SEEK_SET);
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

			parent.used[b] = 1;
			parent.blocks[b] = block;
			b--;
		}
	}
	found_parent_slot:
	
	if(b == NBLOCKS)			// directory is full
		return -1;

	block.inums[e] = inum;
	strcpy(block.names[e], name);
	lseek(fd, parent.blocks[b]*BLOCKSIZE, SEEK_SET);
	write(fd, &block, BLOCKSIZE);

	// create inode
	inode n;
	n.inum = inum;
	n.size = 0;
	for(i = 0; i < NBLOCKS; i++)
	{
		n.used[i] = 0;
		n.blocks[i] = -1;
	}
	n.type = type;	
	if(type == MFS_DIRECTORY)
	{
		n.used[0] = 1;
		n.blocks[0] = nextBlock;
		
		build_dir_block(1, inum, pinum);

		//dirBlock baseBlock;
		//baseBlock.inums[0] = inum;
		//baseBlock.inums[1] = pinum;
		//strcpy(baseBlock.names[0], ".");
		//strcpy(baseBlock.names[1], "..");


		//// write baseBlock
		//lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
		//write(fd, &baseBlock, BLOCKSIZE);
		//nextBlock++;
		//
		// update file size
		n.size += BLOCKSIZE;
	}
	else if (type != MFS_DIRECTORY && type != MFS_REGULAR_FILE)
	{
		return -1;
	}

	// update imap
	imap[inum] = nextBlock;

	// write inode
	lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
	write(fd, &n, sizeof(inode));
	nextBlock++;

	// write checkpoint region
	update_CR(inum);

	return 0;
}

int Server_Unlink(int pinum, char *name){
	
	inode toRemove;					// to be removed
	inode parent;						// parent of toRemove

	//printf("Checking for parent inode\n");
	if(get_inode(pinum, &parent) == -1)			// parent directory doesn't exist; return failure
		return -1;

	int inum = Server_Lookup(pinum, name);	// inum of toRemove
	//printf("Checking for toRemove, inum = %d\n", inum);
	if(get_inode(inum, &toRemove) == -1)		// toRemove doesn't exist; return success
		return 0;

	// if toRemove is a directory, make sure it's empty
	//printf("toRemove.type = %d\n", toRemove.type);
	if(toRemove.type == MFS_DIRECTORY)
	{
		printf("inum %d is a directory!\n", pinum);
		int b;
		for(b = 0; b < NBLOCKS; b++)
		{
			if(toRemove.used[b])
			{
				//printf("inum %d uses block %d\n", pinum, b);
				dirBlock block;
				lseek(fd, toRemove.blocks[b]*BLOCKSIZE, SEEK_SET);
				read(fd, &block, BLOCKSIZE);

				int e;
				for(e = 0; e < NENTRIES; e++)
				{
					//printf("Checking entry number %d with inum %d and name %s\n", e, block.inums[e], block.names[e]);
					if(block.inums[e] != -1 && strcmp(block.names[e], ".") != 0 && strcmp(block.names[e], "..") != 0)
					{
						return -1;	// found file in toRemove
					}
				}
			}
		}
	}
	
	// remove toRemove from parent
	int found = 0;
	int b;
	for(b = 0; b < NBLOCKS && !found; b++)
	{
		if(parent.used[b])
		{
			//printf("Block number %d is used in inum %d\n", b, pinum);
			dirBlock block;
			lseek(fd, parent.blocks[b]*BLOCKSIZE, SEEK_SET);
			read(fd, &block, BLOCKSIZE);

			int e;
			for(e = 0; e < NENTRIES && !found; e++)
			{
				if(block.inums[e] != -1)
				{
					//printf("Entry number %d is used in block number %d in inum %d\n", e, b, pinum);
					if(strcmp(name, block.names[e]) == 0)
					{
						block.inums[e] = -1;
						strcpy(block.names[e], "DNE");
						found = 1;
					}
				}
			}

			if(found)
			{
				//printf("***************************Found file to delete in inum %d\n", pinum);
				// rewrite this block of parent
				lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
				write(fd, &block, BLOCKSIZE);
				nextBlock++;

				// inform parent inode of new block location
				parent.blocks[b] = nextBlock-1;
				lseek(fd, nextBlock*BLOCKSIZE, SEEK_SET);
				write(fd, &parent, BLOCKSIZE);
				nextBlock++;

				// update imap
				imap[pinum] = nextBlock-1;
				update_CR(pinum);
			}
		}
	}

	// remove toRemove from CR
	imap[inum] = -1;
	update_CR(inum);

	//printf("Removed %s from inum %d.\n", name, pinum);
	//print_CR();
	return 0;
}

int Server_Shutdown()
{
	fsync(fd);			// not sure if this is necessary
	exit(0);
	return -1;	// if we reach this line of code, there was an error
}

// TODO: remove test code
void print_stat(MFS_Stat_t *m)
{
	printf("The size is %d and the type is %d.\n", m->size, m->type);
}

void print_dirBlock(int block)
{
	dirBlock db;
	lseek(fd, block*BLOCKSIZE, SEEK_SET);
	printf("Reading from address %d\n", block*BLOCKSIZE);
	read(fd, &db, sizeof(dirBlock));
	
	int i;
	for(i = 0; i < NENTRIES; i++)
	{
		printf("%d: File %s has inode number %d.\n", i, db.names[i], db.inums[i]);
	}

}

void print_CR()
{
	lseek(fd, 0, SEEK_SET);
	read(fd, &imap, NINODES*sizeof(int));
	read(fd, &nextBlock, sizeof(int));

	int i;
	for(i = 0; i < NINODES; i++)
	{
		printf("inum:%d block:%d    \t", i, imap[i]);
		if(i%5 == 0)
			printf("\n");
	}

	printf("\nNext free block is %d.\n", nextBlock);
}

void print_inode(inode *n)
{
	printf("inode number: %d, type: %d, size: %d\n", n->inum, n->type, n->size);

	int i;
	for(i = 0; i < NBLOCKS; i++)
	{
		printf("%d: block %d, used = %d\n", i, n->blocks[i], n->used[i]);
	}
}

/*int main()
{
	int val;
	int inum;
	int block;
	inode n;

	if(Server_Startup(0, "testLFS") == -1)
	{
		perror("open(): ");
		printf("Startup error: %d\n", errno);
		return -1;
	}
	inum = Server_Lookup(0, "Second level");
	printf("Does Second level exist? %d\n", inum >= 0);
	inum = Server_Lookup(inum, "Third level");
	printf("Does Third level exist? %d\n", inum >= 0);
	
	//printf("\n\n\n\n\n\n/////////////////////////////////////\nStartup successful.\n/////////////////////////////////////////\n\n\n\n\n\n\n");
	
	//printf("fd = %d\n", fd);
	//printf("imap[0] = %d\n", imap[0]);
	//inode root;
	//get_inode(0, &root);
	//print_inode(&root);
	//print_dirBlock(root.blocks[0]);
	//print_CR();
	//print_dirBlock(imap[0]);

	//printf("fd = %d\n", fd);

	//int inum = Server_Lookup(0, ".");		// should return root's inum, 0.
	//printf("Self of root has inode number %d\n", inum);
	//inum = Server_Lookup(0, "..");
	//printf("Parent of root has inode number %d\n", inum);
	//MFS_Stat_t m;
	//assert(Server_Stat(inum, &m) == 0);
	//printf("Server_Stat on inode %d successful.\n", inum);
	//print_stat(&m);

	if(newFS)
	{
		if(Server_Creat(0, MFS_REGULAR_FILE, "FILE!!!!") == -1)
		{
			printf("Server_Creat failed.\n");
			return -1;
		}

		//print_CR();
		//
		inum = Server_Lookup(0, "FILE!!!!");
		//printf("inum of new file is: %d\n", inum);
		get_inode(inum, &n);
		//print_inode(&n);
	//
		//get_inode(0, &n);
		//print_dirBlock(n.blocks[0]);

		buf b;
		strcpy(b.string, "BUFFER BUFFER BUFFER!");
		if(Server_Write(inum, b.string, 0) == -1)
		{
			printf("Server_Write failed.\n");
			return -1;
		}
		strcpy(b.string, "NUMERO ONCE");
		if(Server_Write(inum, b.string, 11) == -1)
		{
			printf("Server_Write failed.\n");
			return -1;
		}
		
		strcpy(b.string, "YOU HAVE NO COPY\n");
		//printf("You have no copy reads: %s\n", b.string);
		if(Server_Read(inum, b.string, 0) == -1)
		{
			printf("Server_Read failed.\n");
			return -1;
		}
		
		//printf("The first block of the file reads: %s\n", b.string);
		if(Server_Read(inum, b.string, 11) == -1)
		{
			printf("Server_Read failed.\n");
			return -1;
		}

		//printf("The eleventh block of the file reads: %s\n", b.string);

		Server_Creat(0, MFS_DIRECTORY, "Second level");
		inum = Server_Lookup(0, "Second level");
		Server_Creat(inum, MFS_DIRECTORY, "Third level");
		inum = Server_Lookup(inum, "Third level");
		
		//print_CR();

		//printf("\n\n\n\nPrint root\n");
		//inum = Server_Lookup(0, ".");
		//printf("inum = %d\n", inum);
		//get_inode(inum, &n);
		//block = n.blocks[0];
		//printf("block = %d\n", block);
		//print_dirBlock(block);
	//
	//
		//printf("\n\n\n\nPrint Second level\n");
		//inum = Server_Lookup(0, "Second level");
		//printf("inum = %d\n", inum);
		//get_inode(inum, &n);
		//block = n.blocks[0];
		//printf("block = %d\n", block);
		//print_dirBlock(block);
	//
		//printf("\n\n\n\nPrint Third level\n");
		//inum = Server_Lookup(inum, "Third level");
		//printf("inum = %d\n", inum);
		//get_inode(inum, &n);
		//block = n.blocks[0];
		//printf("block = %d\n", block);
		//print_dirBlock(block);

		//printf("\n\n\n\nTry (and hopefully fail) to delete Second level\n");
		//if(Server_Unlink(0, "Second level") != -1)
		//{
			//get_inode(0, &n);
			//block = n.blocks[0];
			//print_dirBlock(block);
			//printf("Incorrectly deleted Second level :(\n");
			//return -1;
		//}
	}
	else
	{
		inum = Server_Lookup(0, "Second level");
		printf("inum of Second level is %d\n", inum);
		inum = Server_Lookup(inum, "Third level");
		printf("inum of Third level is %d\n", inum);

		Server_Creat(inum, MFS_REGULAR_FILE, "Final file!!");
		get_inode(inum, &n);
		print_inode(&n);
		print_dirBlock(n.blocks[0]);
	}

	print_CR();

	return 0;
}*/
