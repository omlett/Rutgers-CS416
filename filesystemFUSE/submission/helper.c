#include "stdefs.h"
#include "params.h"
#include "block.h"
  
#include "stdefs.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h> 
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>


// Refactor to encapsulate helper methods


sblock * initializeSuperBlock(int debug){
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

	 return superBlock;
}



int createInode(char * path, mode_t mode, int debug){
    int freeInodeNum = findFreeInode();
    log_msg("Find free inode %i\n", freeInodeNum);
    int freeInodeIndex = freeInodeNum;
    if(freeInodeIndex < 0){
      log_msg("Could not find free inode");
      return -ENOENT;
    }
    int inodeBlockIndex = ITABLE_IDX + (freeInodeNum/ NINODES_BLOCK);
    inode * inodeTable = malloc(BLOCK_SIZE);
    log_msg("\nInode Block Index %i \n\n",  inodeBlockIndex);
    log_msg("\nNumber of Inodes per Block %i \n", (BLOCK_SIZE/sizeof(inode)));
    int readResult = block_read(inodeBlockIndex, inodeTable);
    if(readResult < 0) return -1;

    if(freeInodeIndex >= NINODES_BLOCK) freeInodeIndex = freeInodeNum % NINODES_BLOCK;
    log_msg("\nInode Index %i in Block %i Size: %i\n", freeInodeIndex, (freeInodeNum / NINODES_BLOCK),inodeTable[freeInodeIndex].size );
    if(inodeTable[freeInodeIndex].size == -1){ // Inode is free
    inode * ptr = inodeTable;
    ptr += freeInodeIndex;
    if(debug){
      log_msg("Inode %i - Size: %i NumBlocks: %i\n", inodeTable->iNum, inodeTable->size, inodeTable->numBlocks);
      log_msg("MemAddress inodeTable: %p, MemAddres Inode: %p  Difference %p\n", inodeTable, ptr, (inode *)(ptr-inodeTable) );
      log_msg("\nCreating inode at index %i\n BLOCK: %i address: %p start: %p", freeInodeIndex, inodeBlockIndex, ptr, inodeTable);
    }
    inode * inode = ptr;
    log_msg("\nInode MenAddres: %p\n", inode);
    inode->iNum = freeInodeNum;
    inode->size = 0;            // data block size (bytes) | 0 = free | Around 4 GB
    inode->atime = time(NULL);
    inode->ctime = time(NULL); 
    inode->mtime = time(NULL); 
    inode->userID = getuid();
    inode->fileMode = mode;
    inode->nLinks = (S_ISDIR(mode)) ? 2: 1;
    inode->numBlocks = (S_ISDIR(mode)) ? 2: 0;
    inode->bitPos = freeInodeIndex; 
    inode->directBlockPtr[0] = findFreeBlock();
    if(inode->directBlockPtr[0] < 0) log_msg("Could not allocate a datablock\n");
    char * parentDirectory = malloc(NAME_LEN);
    log_msg("Makes it ere\n");
    char * fileName = strrchr(path, '/') + 1;
    int lengthParentName = strlen(path) - strlen(fileName);
    strncpy(parentDirectory, path, lengthParentName);
    parentDirectory[lengthParentName + 1] = '\0';
    int parentInodeNumber = inodeFromPath(parentDirectory);
    if(S_ISDIR(mode)) initializeDEntries(inode->directBlockPtr[0], inode->iNum, parentInodeNumber);
    log_msg("Creating DENTRY for Inode %i FILENAME: %s ParentInode: %i\n", freeInodeIndex, fileName, parentInodeNumber);
    createDEntry(inode->iNum, fileName, parentInodeNumber, mode);
    int writeResult = block_write(inodeBlockIndex, inodeTable);
    if(writeResult < 0) return -1;
    }
    else{
      log_msg("\nInode is not free size %i\n",inodeTable[freeInodeIndex% NINODES_BLOCK].size);
      return -1;
    }
    free(inodeTable);
    //free(parentDirectory);
    return 0;

}

void initializeDEntries(unsigned int dataBlockIndex, unsigned int inodeNum, int parentNum){
  log_msg("Initializing DEntries for Inode: %i AT DataBlock_IDX %i\n", inodeNum, dataBlockIndex);
  char * buffer = malloc(BLOCK_SIZE);
  memset(buffer, 0, BLOCK_SIZE);
  char * ptr = buffer;
  dirEntry * entry = (dirEntry *)buffer;
  entry->iNum = inodeNum;
  entry->fileType = 'd';
  strcpy(entry->fileName, ".");
  entry->dirLength = DENTRY_MIN + strlen(entry->fileName);
  entry->dirLength += entry->dirLength % 4;
  ptr += entry->dirLength;
  entry = (dirEntry *)ptr;
  entry->iNum = parentNum;
  entry->fileType = 'd';
  strcpy(entry->fileName, "..");
  entry->dirLength = DENTRY_MIN + strlen(entry->fileName);
  entry->dirLength += entry->dirLength % 4;
  block_write(dataBlockIndex + DBLOCK_IDX, buffer);
  log_msg("Finished Initializing DEntries for Inode: %i\n", inodeNum);
  free(buffer);

}

void createDEntry(unsigned int inodeNumber, char * filename, unsigned int parentInodeNumber, mode_t mode){
    char * buffer = malloc(BLOCK_SIZE);
    memset(buffer, 0, BLOCK_SIZE);
    int parentInodeBlock = (parentInodeNumber / NINODES_BLOCK) + ITABLE_IDX;
    int parentIndex = parentInodeNumber;
    int readResult;
    int writeResult;

    if(parentInodeBlock >= NINODES_BLOCK) parentIndex = parentInodeNumber % NINODES_BLOCK;
    readResult = block_read(parentInodeBlock, buffer);
    if(readResult < 0){
      log_msg("CreateDEntry couldnt read block\n");
      exit(0);
    }
    inode * parentInode = (inode *) buffer;
    int numBlocks = parentInode[parentIndex].numBlocks;
    int i = 0;
    char * ptr;
    dirEntry * entry;
    dirEntry * entry2;
    int bytesChecked;
    char * dataBlockBuffer = malloc(BLOCK_SIZE);
    int entryCounter = 0;
    for(; i < numBlocks; i++){
      bytesChecked = 0;
      memset(dataBlockBuffer, 0, BLOCK_SIZE);
      readResult = block_read(DBLOCK_IDX + parentInode[parentIndex].directBlockPtr[i], dataBlockBuffer);
      if(readResult < 1){
        log_msg("CreateDEntry couldnt read block\n");
        exit(0);
      }
      ptr = dataBlockBuffer;
      entry = (dirEntry *)ptr;
      //log_msg("Checking entry: %i which has %i Inode and %s filename\n", entryCounter, entry->iNum, entry->fileName);
      while(entry->dirLength > 0 && bytesChecked < BLOCK_SIZE){
        log_msg("Checking entry: %i Distance %pwhich has %i Inode and %s filename\n", entryCounter, entry, entry->iNum, entry->fileName);
        bytesChecked += entry->dirLength;
        ptr+= entry->dirLength;
        entry = (dirEntry *)ptr;
      }
      if((BLOCK_SIZE - bytesChecked) > (DENTRY_MIN + strlen(filename))){
        log_msg("\nFound free entry at %p\n", entry);
        entry->iNum = inodeNumber;
        entry->fileType = S_ISDIR(mode) ? 'd' : 'f';
        entry->dirLength = DENTRY_MIN + strlen(filename);
        entry->dirLength += (entry->dirLength % 4);
        strcpy(entry->fileName, filename);
        writeResult = block_write(DBLOCK_IDX + parentInode[parentIndex].directBlockPtr[i], dataBlockBuffer);
<<<<<<< HEAD
        log_msg("Wrote entry to block Inode: %i, fileName: %s Entrylength: %i fileType: %c", entry->iNum, entry->fileName, entry->dirLength, entry->fileType);
        break;
=======
        log_msg("Wrote entry to block Inode: %i, fileName: %s Entrylength: %i fileType: %c\n", entry->iNum, entry->fileName, entry->dirLength, entry->fileType);
        return;
>>>>>>> 163ce7770a3cd7d6bde4c3e234543e13568bb972
      }

    }
}

/* Tester Driver for Creating files */
void testFSCreate(){
  sfs_create("hello.txt", "r");
  
}

/* Helper file for creating bitmap */
//http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html

int findFirstFree(int * bitmap){
  int i = 0;
  int l = 1;
  unsigned int flag = 1;

  for(; i< (BLOCK_SIZE / sizeof(int)); i++){  //Go through every integer in bitmap
    for(; l < 32; l++){ // GO through each bit in the integer;
      if(!((bitmap[i] >> l) & 1)){
        bitmap[i] |=  1 << l;
        return ((i * 32) + l);
      }
    }
  }
  return -1; // If no 
}
//Need to add to make file
// Combined set and clear. If bit given in K, cleark that bit. Else set that bit
void setBit(int * bitmap, int bitK){
  int i = abs(bitK/32);
  int pos = abs(bitK%32);
  unsigned int flag = 1;
  log_msg("\nSettimg bit position %i \n", bitK);
  bitmap[i] | flag;
  
}

void clearBit(int * bitmap, int bitK){
  int i = abs(bitK/32);
  int pos = abs(bitK%32);
  bitmap[i] &= ~(1<<pos);
  
  
}

int testBit(int * bitmap, int bitK){
    int i = bitK/32;
    int pos = (bitK < 32) ?  bitK : bitK % 32;
    return (bitmap[i] >> pos) & 1;

}

void createRoot(inode * inodeTable){
      inodeTable[0].fileMode = S_IFDIR | 0755;
      inodeTable[0].nLinks = 2;
      inodeTable[0].ctime = time(&(inodeTable[0].ctime));
      inodeTable[0].mtime = time(&(inodeTable[0].mtime));
      inodeTable[0].groupID = getegid();
      inodeTable[0].bitPos = 0;
      inodeTable[0].userID = getuid();
      inodeTable[0].size = 0;
      inodeTable[0].directBlockPtr[0] = findFreeBlock();
      inodeTable[0].numBlocks = 1;
      inodeTable[0].iNum = 0;
      initializeDEntries(inodeTable[0].directBlockPtr[0], inodeTable[0].iNum, inodeTable[0].iNum);
}


/*int getInode(char * buffer, const char * path){
  int i = 0;
  int k = 0;
  inode * ptr;
  int readResult;
  for(; i < NUM_INODE_BLOCKS; i++){
      memset(buffer, 0, BLOCK_SIZE);
      readResult = block_read((i+4), buffer);
      ptr = (inode *)buffer;
         log_msg("BlOCK %i Filepath = %i\n", (i+4), ptr->iNum);
      for(; k < 2; k++){
        //if(debug)log_msg("Inode: %i Filepath = %s\n", ptr->iNum, ptr->name);
        if(strcmp(ptr->name, path)== 0){
             return ptr->iNum;
        }

        ptr++;
    }

}
return -1;
}*/

int findFreeBlock(){
  int * blockBitmap = malloc(BLOCK_SIZE);
  int readResult = block_read(BBITMAP_IDX, blockBitmap);
  int blockIndex = findFirstFree(blockBitmap);
  int writeResult = block_write(BBITMAP_IDX, blockBitmap);
  return blockIndex;
}

int findFreeInode(){
  int * inodeBitmap = malloc(BLOCK_SIZE);
  int readResult = block_read(IBITMAP_IDX, inodeBitmap);
  int inodeIndex = findFirstFree(inodeBitmap);
  int writeResult = block_write(IBITMAP_IDX, inodeBitmap);
  free(inodeBitmap);
  log_msg("\nFound Free Inode at index %i in Bitmap\n", inodeIndex);
  return inodeIndex;
}

void getInode(unsigned int inodeNumber, inode * inodeData){
  char * buffer = malloc(BLOCK_SIZE);
  int inodeIndex;
  int inodeBlockIndex = inodeNumber / NINODES_BLOCK;
  int readResult = block_read(ITABLE_IDX + inodeBlockIndex, buffer);
  if(readResult < 0){
     log_msg("Cant Read Inode Table Block\n");
     exit(0);
  }
  inodeIndex = inodeNumber;
  if(inodeNumber >= NINODES_BLOCK) inodeIndex = inodeNumber % NINODES_BLOCK;

  inode * inodeTable = (inode *)buffer;
  inode * _inode = &inodeTable[inodeIndex];
  memcpy(inodeData, _inode, sizeof(inode));
  log_msg("getInode: %i  inodeNuber: %i size = %i\n", _inode->iNum, inodeNumber, _inode->size);

  //memcpy(inodeData, _inode, sizeof(inode));
  free(buffer);
}

void findInode(const char * iPath, inode * _inode, unsigned int iParent){
  int i = 0;
  char * buffer = malloc(BLOCK_SIZE);
  log_msg("\nfindInode IPATH: %s Parent: %i\n", iPath, iParent);
  int readResult = block_read((ITABLE_IDX + (iParent/NINODES_BLOCK)), buffer);
  inode * parent = (inode *)buffer;
  char * ptr;
  dirEntry * entry;
  char * dataBlockBuffer = malloc(BLOCK_SIZE);
  int bytesChecked = 0;
  if(iParent >= NINODES_BLOCK) iParent = iParent%NINODES_BLOCK;
  parent = &parent[iParent];
  for(; i< parent->numBlocks; i++){
    readResult = block_read((parent->directBlockPtr[i] + DBLOCK_IDX), dataBlockBuffer);
    log_msg("Makes it inside forloop for entries check in findnode\n");
    if(readResult < 0){
      log_msg("Failed to Read Data Block %i", parent->directBlockPtr);
      exit(1);
    }
    ptr = dataBlockBuffer;
    entry = (dirEntry *)ptr;
    while(entry->dirLength && bytesChecked < BLOCK_SIZE){
      log_msg("\nChecking entry for Inode: %i with filename : %s", entry->iNum, entry->fileName);
      if(strcmp(entry->fileName, iPath) == 0){
        log_msg("\nFound entry %s Inode: %i\n", entry->fileName, entry->iNum);
        getInode(entry->iNum, _inode);
        log_msg("getInode return with %i\n size: %i", _inode->iNum, _inode->size);
        return;
      }
      bytesChecked += entry->dirLength;
      ptr+= entry->dirLength;
      entry = (dirEntry *)ptr;
    }
  }
}



int inodeFromPath(const char * path){

 if(strcmp(path, "/") == 0){ // ROOT DIRECTORY
     return 0;     
  }
  char fpath[NAME_LEN];
  strcpy(fpath, path);
  char * buffer = malloc(BLOCK_SIZE);
  int readResult = block_read(ITABLE_IDX, buffer);
   if(readResult < 0){
      log_msg("Unable to Read INODE TABLE BLOCK \n");
      return EFAULT;
    } 
  inode * ptr = (inode *)buffer;
 if(*fpath == '/'){
    inode * _inode = malloc(sizeof(inode));
    int parentNum = 0;
    char * iPath = strtok(fpath, "/");
    while(iPath != NULL){
      parentNum = ptr->iNum;
      findInode(iPath, _inode, ptr->iNum);
      ptr = _inode;
      log_msg("\nInode: %i size: %i\n", ptr->iNum, ptr->size);
      if(!(ptr->iNum)){
        return -1;
      }
      iPath = strtok(NULL, "/");
      log_msg("\nIPath: %s\n",iPath) ;
    }
    log_msg("\nParent INUM: %i PTR INUM: %i\n", parentNum, ptr->iNum);
    if((ptr->iNum > 0)  && (ptr->iNum != parentNum) && (ptr->iNum < TOTAL_INODES)){
      return ptr->iNum;
    }
    return -1;
  }
}

int getChild(char * iPath, unsigned int parent){
  char * buffer = malloc(BLOCK_SIZE);
  int inodeBlockIndex = parent / NINODES_BLOCK;
  int inodeIndex = (parent >= NINODES_BLOCK) ? parent%NINODES_BLOCK : parent;
  int readResult = block_read(inodeBlockIndex + ITABLE_IDX, buffer);
  if(readResult < 0){
    log_msg("\nCould not read ITABLE BLOCK\n");
    exit(0);
  }
  inode parentInode;
  inode * ptr = (inode *) buffer;
  dirEntry * entry;
  char * ptr1; 
  memcpy(&parentInode, &ptr[inodeIndex], sizeof(inode));
  unsigned int bytesChecked = 0;
  int i = 0;
  for(; i < parentInode.numBlocks; i++){
    memset(buffer, 0, BLOCK_SIZE);
    readResult = block_read(parentInode.directBlockPtr[i], buffer);
    entry = (dirEntry *)buffer;
    ptr1 = (char *) entry;
    while(entry->iNum && bytesChecked < BLOCK_SIZE){
      entry = (dirEntry *)ptr;
      if(strcmp(entry->fileName, iPath) == 0){
        return entry->iNum;
      }
      bytesChecked += entry->dirLength;
      ptr1 += entry->dirLength;
    }
  }
  return -1;

}

void fillStat(struct stat *statbuf, inode * inode){
  statbuf->st_ino = inode->iNum;
  log_msg("\nInode: %i Same file path \n", inode->iNum);
  statbuf->st_mode =  inode->fileMode;
  statbuf->st_nlink = inode->nLinks;
  statbuf->st_uid = inode->userID;
  statbuf->st_gid = inode->groupID;
  statbuf->st_size = inode->size;
  statbuf->st_blksize = BLOCK_SIZE;
  statbuf->st_blocks = inode->numBlocks;
  statbuf->st_atime = inode->atime;
  statbuf->st_mtime = inode->mtime;
  statbuf->st_ctime = inode->ctime;
  log_stat(statbuf);
}

