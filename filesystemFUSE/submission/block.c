/*
  Copyright (C) 2015 CS416/CS516

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/
#include "block.h"
#include "stdefs.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>


int diskfile = -1;
int superDebug = 0;
int readResult;
int writeResult;

void disk_open(const char* diskfile_path)
{
    if(diskfile >= 0)
        return;
    
    
    diskfile = open(diskfile_path, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
    
    if (diskfile < 0) {
        perror("disk_open failed");
        log_msg("\ndisk_open( FAILEDDDDD\n");
        //exit(EXIT_FAILURE);
    }
}

void disk_close()
{
    if(diskfile >= 0)
        close(diskfile);
    
}

/** Read a block from an open file
 *
 * Read should return (1) exactly @BLOCK_SIZE when succeeded, or (2) 0 when the requested block has never been touched before, or (3) a negtive value when failed. 
 * In cases of error or return value equals to 0, the content of the @buf is set to 0.
 */
int block_read(const int block_num, void *buf)
{
    int retstat = 0;
    retstat = pread(diskfile, buf, BLOCK_SIZE, block_num*BLOCK_SIZE);
    if (retstat <= 0){
	   memset(buf, 0, BLOCK_SIZE);

	   if(retstat<0)
	       perror("block_read failed");

       log_msg("\nUnable to Read Block %i\n", block_num);
       exit(0);
    }

    return retstat;
}

/** Write a block to an open file
 *
 * Write should return exactly @BLOCK_SIZE except on error. 
 */
int block_write(const int block_num, const void *buf)
{
    int retstat = 0;
    retstat = pwrite(diskfile, buf, BLOCK_SIZE, block_num*BLOCK_SIZE);
    
    if (retstat < 0){
        perror("block_write failed");
        log_msg("\nUnable to Read Block %i\n", block_num);
        exit(0);
    }
    
    return retstat;
}


int findFreeBlock(){
    int * blockBitmap = malloc(BLOCK_SIZE);
    int i = 0;
    int blockIndex;

    for(; i < NBIT_BLOCK; i++){
       memset(blockBitmap, 0, BLOCK_SIZE); 
       readResult = block_read(BBITMAP_IDX + i, blockBitmap);
       blockIndex = findFirstFree(blockBitmap);

       if(blockIndex > 0){
            writeResult = block_write(BBITMAP_IDX + i, blockBitmap);
            break;
       }
    }

  free(blockBitmap);
  return blockIndex;
}

sblock * initializeSuperBlock(){
    sblock * superBlock = (sblock *)malloc(sizeof(sblock));
    superBlock->fs_size = TOTAL_FS_SIZE;
    superBlock->block_size = BLOCK_SIZE;
    superBlock->num_inodes = TOTAL_INODES;
    superBlock->num_blocks = TOTAL_BLOCKS;
    superBlock->num_free_blocks =  TOTAL_BLOCKS - NUM_RESERVED_BLOCKS;
    superBlock->index_next_free_block = TOTAL_BLOCKS - superBlock->num_free_blocks - 1; // first free block right after superblock
    superBlock->free_block_list = NULL; // Not sure if should keep this 
    superBlock->num_free_inodes = TOTAL_INODES - 1; // ALl inodes free except first one (which will be set to root)
    superBlock->index_next_free_inode = 1; // First free inode should be root
    superBlock->free_inode_list = NULL; // Not sure if will keeep 
    superBlock->root_inode_num = 0; // Inode number for root directory made up for now  
    superBlock->magic_number = 90210;

    if(superDebug){
        log_msg("TOTAL_FS_SIZE: %i\n", superBlock->fs_size);
        log_msg("BLOCK_SIZE: %i\n", superBlock->block_size);
        log_msg("NUM_INODES: %i\n", superBlock->num_inodes);
        log_msg("TOTAL_BLOCKS: %i\n", superBlock->num_blocks);
        log_msg("NUM_FREE_BLOCKS: %i\n", superBlock->num_free_blocks);
        log_msg("INDEX_NEXT_FREE_BLOCK: %i\n", superBlock->index_next_free_block);
        log_msg("ROOT_INODE_NUM: %i\n", superBlock->root_inode_num);
    }   

     return superBlock;
}


