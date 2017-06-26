/*
  Copyright (C) 2015 CS416/CS516

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#ifndef _BLOCK_H_
#define _BLOCK_H_

// Superblock to describe filesystem
// Each UNIX partition usually contains a special block called the superblock. 
// The superblock contains the basic information about the entire file system. 
// This includes the size of the file system, the list of free and allocated 
// blocks, the name of the partition, and the modification time of the filesystem.


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

// From class notes slide 24
/* Super Block contains
 * size of file system, number of free blocks, list of free blocks, 
 * index to the next free block, size of the I-node list,
 * number of free I-nodes, list of free I-nodes, index to the next free
 * I-node, locks for free block and free I-node lists, and flag to indicate
 * a modification to the SB
 */
typedef struct __attribute__((packed, aligned(4)))sblock {
	unsigned long fs_size;						// Size of File System (Ours in 24 MB)
	unsigned int num_inodes;					// Number inodes in the system
	unsigned int num_blocks;					// Number of total blocks
	unsigned int num_r_blocks;					// Number of reserved blocks (Boot, Super, InondeBitMap, DataBitMap, Inodetable)
	unsigned int first_data_block;
	unsigned long block_size;					// Systems blocksize (512 for Standard UNIX) .. PAGE = 8 Blocks
	unsigned char dirtyFlag;
	unsigned int num_free_blocks;				// Number of free blocks available (TOTAL - RESERVED) Starting
	unsigned int num_free_inodes;				// TOTAL - 1 (root)
	unsigned int index_next_free_block;			// TOAL BLOCKS - RESERVED  1				
	unsigned int * free_block_list;
	unsigned int * free_inode_list;				// Number of free inodes in file system
	unsigned int index_next_free_inode;			// Index of the free inode
	unsigned int magic_number;					// magic number
	unsigned int root_inode_num; 				// Inode number of root directory

} sblock;

void disk_open(const char* diskfile_path);
void disk_close();
int block_read(const int block_num, void *buf);
int block_write(const int block_num, const void *buf);
sblock * initializeSuperBlock();
int findFreeBlock();

#endif
