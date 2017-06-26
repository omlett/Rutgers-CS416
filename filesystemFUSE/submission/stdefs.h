/****************************************************************/
// Created by Pranav Katkamwar, Andrew Khaznakovic, Karl Xu
// Spring 2017 - CS416
/****************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inode.h"

#ifndef _STDEFS_H
#define _STDEFS_H


#define NUM_INODE_BLOCKS 64
#define BLOCK_SIZE 512
#define	TOTAL_INODES ((BLOCK_SIZE * NUM_INODE_BLOCKS)/sizeof(inode))
#define TOTAL_FS_SIZE ((8 + 16) * 1024 * 1024)
#define TOTAL_BLOCKS TOTAL_FS_SIZE/BLOCK_SIZE
#define NUM_RESERVED_BLOCKS (NUM_INODE_BLOCKS + 4)
#define TOTAL_DBLOCKS (TOTAL_BLOCKS - NUM_RESERVED_BLOCKS)
#define INVALID_INO TOTAL_INODES
#define NBIT_BLOCK 3
#define NINODES_BLOCK (BLOCK_SIZE/sizeof(inode))
#define BOOTBLOCK_IDX 0
#define SBLOCK_IDX 1
#define IBITMAP_IDX 2
#define BBITMAP_IDX 3
#define ITABLE_IDX 6
#define DBLOCK_IDX 70
#define NDBLOCK_PTRS 10
#define NAME_LEN 255
#define DENTRY_MIN (sizeof(unsigned int) + sizeof(unsigned char) + sizeof(unsigned short) + 1)
#define NBIT_BLOCK 3
#define MAGIC 90210
/****************************************************************/
// Structure Definitions
/****************************************************************/

//https://web.cs.wpi.edu/~rek/DCS/D04/UnixFileSystems.html
// Where im getting most of information

/* Inode Basics
 * An Inode number points to an Inode. An Inode is a data structure that stores the following information about a file :
 * Size of file
 * Device ID
 * User ID of the file
 * Group ID of the file
 * The file mode information and access privileges for owner, group and others
 * File protection flags
 * The timestamps for file creation, modification etc
 * link counter to determine the number of hard links
 * Pointers to the blocks storing file’s contents
 */


/* According to lecutre notes 
 * I-node contains: owner, type (directory, file, device), last modified
 * time, last accessed time, last I-node modified time, access
 * permissions, number of links to the file, size, and block pointers 
 */
/*typedef struct inode{
	int	iNum;					// inode number
	long size;					// data block size (bytes) | 0 = free | Around 4 GB
	mode_t fileMode;				// Permissions (Dont think we need to worry about this)
	time_t atime;					// inode access time
	time_t ctime;					// inode change time
	time_t mtime;					// inode modification time
	uid_t userID;					// user id
	gid_t groupID;				// group id
	unsigned int nLinks;
	unsigned int directBlockPtr [10];			// 10 direct pointers to the datablocks for the file (pointers are indexs of the block)
	unsigned int dIndirectBlockPtr;			// Single Doubly indirect block pointer
	unsigned int bitPos;
	unsigned int numBlocks;
	//char name[MAX_PATH];
	char padding[140];
	// does not contain file path
} inode;

inode * inodeTable;		*/	// Global inode table

/*typedef struct dirEntry{
	unsigned int iNum;		// inode number
	unsigned char fileType;
	unsigned short dirLength;	
	char fileName[NAME_LEN];			// filename
} dirEntry;*/





// Not sure if be necessary but bitmap to keep track of blocks as in slide 13 of filesystem 
// slides

/* Bitmap: one bit for each block on the disk
 * Good to find a contiguous group of free blocks
 * Files are often accessed sequentially
 * For 1TB disk and 4KB blocks, 32MB for the bitmap
 * Chained free portions: pointer to the next one
 * Not so good for sequential access (hard to find sequential
 * blocks of appropriate size)
 * Index: treats free space as a file
 */


/*int testBit(int * bitmap, int bitK);
int findFirstFree(int * bitmap);
void clearBit(int * bitmap, int bitK);

void setBit(int * bitmap, int bitK);
int writeInodeBitmap(int * bitmap, int blockNum);*/
//int createInode(char * path, mode_t mode, int debug);
//void createRoot(inode * inodeTable);
//void createDEntry(unsigned int inodeNumber, char * filename, unsigned int parentInodeNumber, mode_t mode);
//void initializeDEntries(unsigned int dataBlockIndex, unsigned int inodeNum, int parentNum);
//void getInode(unsigned int inodeNumber, inode * inodeData);

#endif


/*
Diskfile created with command:
dd if=/dev/zero of=diskfile bs=1024 count=24576
dd if=/dev/zero of=testsys bs=1024 count=2048
*/