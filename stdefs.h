/****************************************************************/
// Created by Pranav Katkamwar, Andrew Khaznakovic, Karl Xu
// Spring 2017 - CS416
/****************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef _STDEFS_H
#define _STDEFS_H

#define	TOTAL_INODES	240000
/****************************************************************/
// Structure Definitions
/****************************************************************/
typedef struct inode{
	short iType;			// dir or file
	int ino;			// inode number
	long int size;			// data block size (bytes) | 0 = free
	int fileMode;			// Permissions
	time_t atime;			// inode access time
	time_t ctime;			// inode change time
	time_t mtime;			// inode modification time
} inode;

inode *inodeTable;			// Global inode table
char initTracker;			// Make sure init doesn't get called more than once 


#endif


/*
Diskfile created with command:
dd if=/dev/zero of=diskfile bs=1024 count=24576
*/
