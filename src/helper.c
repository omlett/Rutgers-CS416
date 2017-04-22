// Refactor to encapsulate helper methods

int initalizeSuperBlock(sblock * sBlock, int debug){
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

	if(debug){
		log_msg("TOTAL_FS_SIZE: %i\n", superBlock->fs_size);
		log_msg("BLOCK_SIZE: %i\n", superBlock->block_size);
		log_msg("NUM_INODES: %i\n", superBlock->num_inodes);
		log_msg("TOTAL_BLOCKS: %i\n", superBlock->num_blocks);
		log_msg("NUM_FREE_BLOCKS: %i\n", superBlock->num_free_blocks);
		log_msg("INDEX_NEXT_FREE_BLOCK: %i\n", superBlock->index_next_free_block);
		log_msg("ROOT_INODE_NUM: %i\n", superBlock->root_inode_num);
	}	

	 return 1;
}

int write_Sblock(superblock * superBlock){
	 int writeResult = block_write(1, superBlock);
     
      if(writeResult < 0){ //write failed
        log_msg("\nblock_write(0, &superBlock) failed\n");
        return -1;
        exit(0);
      }
      else{
        log_msg("\nblock_write(0, &superBlock) was successful");
        log_msg("\nsize of superBlock = %i\n", sizeof(sblock));
        return 1;
      }

      free(superBlock);
}

int writeInodeBitmap(int * bitmap, int blockNum){
	 writeResult = block_write(blockNum, inodeBitmap);
      
      if(writeResult < 0){ //write failed
        log_msg("\nblock_write(0, &inodeBitmapk) failed\n");
        return -1;
      }
      else{
        log_msg("\nblock_write(0, &inodeBitmap) was successful");
        return 1;
        //log_msg("\nsize of inodeBitmap = %i\n", sizeof(sblock));
      }
}


