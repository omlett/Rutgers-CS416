/****************************************************************/
// Created by Pranav Katkamwar, Andrew Khaznakovic, Karl Xu
// Spring 2017 - CS416
/****************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef _STDEFS_H
#define _STDEFS_H

/****************************************************************/
// Structure Definitions
/****************************************************************/
typedef struct inode{
	short itype;			// dir or file
	int ino;			// inode number
	long int size;			// data block size (bytes)
	int fileMode;			// Permissions
	time_t ctime;			// inode change time
	time_t mtime;			// inode modification time
	time_t atime;			// inode access time
} inode;

typedef struct inodeTable{
	table inode[240000];
} iTable;



#endif
