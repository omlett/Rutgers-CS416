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
typedef struct inode{
	short iType;			// dir or file //AK: Why short, if either dir or file we can just use char short is 2 bytes, we dont need 2^16 optiomns
	int ino;				// inode number
	long int size;			// data block size (bytes) | 0 = free | Around 4 GB
	int fileMode;			// Permissions
	time_t atime;			// inode access time
	time_t ctime;			// inode change time
	time_t mtime;			// inode modification time
	uid_t userID;			// user id
	gid_t groupID;			// group id
} inode;

inode *inodeTable;			// Global inode table
char initTracker;			// Make sure init doesn't get called more than once 



#endif


/*
Diskfile created with command:
dd if=/dev/zero of=diskfile bs=1024 count=24576
*/
