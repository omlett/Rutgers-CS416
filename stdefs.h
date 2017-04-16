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

// Superblock to describe filesystem
/* Struct
 * s_list: superblock LL
 * s_dev: = Device indentifer
 * s_blocksize = block size in bytes
 * s_dir = dirty (modified) flag
 * s_op Superblock methods
 * s_lock = superblock semaphore
 * s_inodes = list of all inodes
 * s_io = inodes waiting for write
 * s_files = list of file objects
 */
typdef struct sblock {
	int num_nodes;
	int 



#endif


/*
Diskfile created with command:
dd if=/dev/zero of=diskfile bs=1024 count=24576
*/
