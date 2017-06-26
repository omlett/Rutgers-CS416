/*
  Simple File System

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.

*/

#include "stdefs.h"
#include "params.h"
#include "block.h"
#include "inode.h"
#include "dirent.h"

#include <ctype.h>
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
#include <time.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif
#include "log.h"


int writeResult;
int readResult;
char * blockBuffer;
int i, j, k;

void *sfs_init(struct fuse_conn_info *conn){
    log_msg("\nsfs_init()\n");
    log_conn(conn);
    log_fuse_context(fuse_get_context());
    log_msg("\ndisk_open(path=%s) TOTAL_FS_SIZE: %i\n", SFS_DATA->diskfile, TOTAL_FS_SIZE);

    // Opens disk simply returns if disk already open
    disk_open(SFS_DATA->diskfile);

    //log_msg("\nGod dammit\n", SFS_DATA->diskfile);

    
    char * buffer = (char *) malloc(BLOCK_SIZE);
    // Check if there is a SUPERBLOCK set using block_read
    // Read first block (if set first block is super block
    // Read should return (1) exactly @BLOCK_SIZE when succeeded, or (2) 0 when the requested block has never been touched before, or (3) a negtive value when failed. 
    int readResult = block_read(SBLOCK_IDX, buffer); 
    log_msg("readResullt : %i\n", readResult);

    if(readResult < 0) exit(EXIT_FAILURE);

    sblock * super = (sblock *)buffer;

    if((super->magic_number != MAGIC)){ // block has never been touched
        // Create and initialize SBlock
        //int * blockFreeList = (int *)malloc(TOTAL_BLOCKS/sizeof(int));
        sblock * superBlock = initializeSuperBlock();

        // Write the SBlock to to first block in FS using block_write
        // Write should return exactly @BLOCK_SIZE except on error. 

        int writeResult = block_write(SBLOCK_IDX, superBlock);

        if(writeResult < 0) exit(EXIT_FAILURE);

        log_msg("\nblock_write(0, &superBlock) was successful\n");
        //log_msg("\nsize of superBlock = %i\n", sizeof(sblock));


        unsigned int * inodeBitmap = (unsigned int *)malloc(BLOCK_SIZE);
        memset(inodeBitmap, 0, BLOCK_SIZE);
        setBit(inodeBitmap, 0); // inode 0 = root so set it as used;
        writeResult = block_write(IBITMAP_IDX, inodeBitmap);
        if(writeResult < 0) exit(EXIT_FAILURE);

        log_msg("\nblock_write(0, &inodeBitmap) was successful");

        int * blockBitmap = malloc(BLOCK_SIZE);
        int i = 0;
        for(; i < NBIT_BLOCK; i++){
         memset(blockBitmap, 0,  BLOCK_SIZE);
         writeResult = block_write(BBITMAP_IDX + i, blockBitmap);
         blockBitmap = malloc(BLOCK_SIZE);
        }

           

        writeResult = block_write(BBITMAP_IDX, blockBitmap);

        if(writeResult < 0) exit(EXIT_FAILURE);

        log_msg("\nblock_write(0, &blockBitmap) was successful");


        inode * inodeTable = malloc(TOTAL_INODES * sizeof(inode));
        i = 0;

        for(; i< TOTAL_INODES; i++){
        inodeTable[i].iNum = i;
        inodeTable[i].size = -1;         
        }

        log_msg("\nInodes set\n");
        createRoot(inodeTable);
        log_msg("\nRoot created\n");
        
        i = ITABLE_IDX;
        char * buffer;
        inode * ptr = (inode *)inodeTable;
        for(; i < (ITABLE_IDX + NUM_INODE_BLOCKS); i++){
            buffer = malloc(BLOCK_SIZE);
            memset(buffer, 0, BLOCK_SIZE);
            memcpy(buffer, &inodeTable[NINODES_BLOCK * (i- ITABLE_IDX)], BLOCK_SIZE);
            writeResult = block_write(i, buffer);
        }
}
     
    else if(readResult > 0 ){ // first block has been accessed before parse superBlock information
      char* superBlock = (char *)malloc(BLOCK_SIZE);
      sblock * test = (sblock *)superBlock;
      int superBlockRead = block_read(SBLOCK_IDX, superBlock);

      log_msg("superBlockRead1: %i Block SIze: %i\n", superBlockRead, BLOCK_SIZE);
      
      if(superBlockRead > 0){ // superBlock read sucessfully
        log_msg("FS Contains Valid Super Block\n");
        log_msg("SuperBlock: TOTAL_FS_SIZE: %i\n NumI: %i \n", test->fs_size, test->num_inodes);
        log_msg("\nsize of superBlock = %i\n", sizeof(sblock));
      }

      else {
        log_msg("block read failed\n");
      }
    }

    else{
        log_msg("block read failed\n");
    }


 log_msg("\nRile System Successfully Initialized SFS_STATE ---> DISKFILE: %s\n", SFS_DATA->diskfile);

  return SFS_DATA;
  
}


/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata)
{
    log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
    disk_close();
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf)
{
    int retstat = 0;
    log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n",path, statbuf);
    char fpath[NAME_LEN];
    memset(fpath, 0, NAME_LEN);
    fprintf(stderr, "in getatt\n");
    int i = 0;
    strcpy(fpath, path);

    blockBuffer = malloc(BLOCK_SIZE);
    memset(blockBuffer, 0, BLOCK_SIZE);
    readResult = block_read(ITABLE_IDX, blockBuffer);
   

    inode * ptr = (inode *)blockBuffer;
    if(strcmp(path, "/") == 0){ // ROOT DIRECTORY
        fillStat(statbuf, ptr);
        return retstat;    
    }
    else if(*path == '/'){
      inode * _inode = malloc(sizeof(inode));
      int parentNum = 0;
      char * iPath = strtok(fpath, "/");

    while(iPath != NULL){
        parentNum = ptr->iNum;
        findInode(iPath, _inode, ptr->iNum);
        ptr = _inode;
        log_msg("\nInode: %i size: %i\n", ptr->iNum, ptr->size);

        if(ptr->size < 0){
            retstat = -ENOENT;
            break;
        }

        iPath = strtok(NULL, "/");
        log_msg("\nIPath: %s\n",iPath) ;
    }
    
    log_msg("\nParent INUM: %i PTR INUM: %i\n", parentNum, ptr->iNum);

    if((ptr->iNum > 0)  && (ptr->iNum != parentNum) && (ptr->iNum < TOTAL_INODES)){
        fillStat(statbuf, ptr);
        return retstat;
    }
  }
    retstat = -ENOENT;
    return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
   // mode_t  st_mode  mode of file

  int retstat = 0;
  log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
      path, mode, fi);

 

    char fpath[NAME_LEN];
    strcpy(fpath, path);

   
    retstat = createInode(fpath, mode);
    
    return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path)
{
    int retstat = 0;
    log_msg("sfs_unlink(path=\"%s\")\n", path);
    char fpath[NAME_LEN];
    strcpy(fpath, path);

    fprintf(stderr, "in getatt\n");
    int fileInode = inodeFromPath(fpath);
    int parentInode = getParentInodeFP(fpath);
    if(fileInode > 0){
        inode * inodeRm = malloc(sizeof(inode));
        getInode(fileInode, inodeRm);
        if(inodeRm->iNum){
          if(!removeInode(inodeRm->iNum, inodeRm->fileMode))
            retstat = -1;
          
          if(!deleteInodeEntry(fileInode, parentInode))
            retstat = -1;

        }
    }

    log_msg("\nInode not found\n");

    return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",
      path, fi);

    inode * inodeReq = malloc(sizeof(inode));

    int inodeNumber = inodeFromPath(path);
    if (inodeNumber != -1){
      getInode(inodeNumber,inodeReq);
      if (S_ISREG(inodeReq->fileMode)){
        free(inodeReq);
        log_msg("\nFile Successfully found for path: %s\nInode Number: %i\n", path, inodeNumber);
        return retstat;
      }
      else{
        free(inodeReq);
        log_msg("\nError: File not found for path %s\n", path);
      return ENOENT;
      }
    }
    else{
      free(inodeReq);
      log_msg("\nError: File not found for path %s\n", path);
      return ENOENT;
    }
    free(inodeReq);
    return -1;

}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int sfs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",
    path, fi);
    

    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
    path, buf, size, offset, fi);


    char * dataBlockBuffer;
    int blockidx, bitesLeft, bitesRed, readResult, blockoff;
    inode * file_ino;

    int file_inum = inodeFromPath(path);

    if (file_inum != -1)
    {
        file_ino = (inode *) malloc(sizeof(inode));
        getInode(file_inum, file_ino);

        bitesLeft = (file_ino->size < size) ? file_ino->size : size;
        bitesRed = 0;
        blockidx = offset / BLOCK_SIZE;
        blockoff = offset % BLOCK_SIZE;
        //log_msg("Starting sfs_read bitesLeft %i blockidx = %i, blockoff %i\n", bitesLeft, blockidx, blockoff);

        dataBlockBuffer = malloc(BLOCK_SIZE);
        log_msg("Starting sfs_read bitesLeft1 %i blockidx = %i, [blockidx] %i, blockoff %i\n", bitesLeft, blockidx, file_ino->directBlockPtr[blockidx], blockoff);
        while(bitesLeft > 0 && (bitesRed <= (file_ino->size)))
        {
            log_msg("Starting sfs_read bitesLeft %i blockidx = %i, [blockidx] %i, blockoff %i\n", bitesLeft, blockidx, file_ino->directBlockPtr[blockidx],
             blockoff);
            memset(dataBlockBuffer, 0, BLOCK_SIZE);

            readResult = block_read(file_ino->directBlockPtr[blockidx] + DBLOCK_IDX, dataBlockBuffer);
            
            if (readResult < 0){
                log_msg("Failed to Read Data Block %i", file_ino->directBlockPtr[blockidx] + DBLOCK_IDX);

                // Free Mallocs
                free(dataBlockBuffer);
                free(file_ino);

                return 0;
            }

            if (bitesLeft <= (BLOCK_SIZE - blockoff)){
                memcpy((void *) (buf + bitesRed), (void *) (dataBlockBuffer + blockoff), bitesLeft);
                bitesRed += bitesLeft;
                bitesLeft = 0;

                log_msg("Read %i bytes from file. // bitesLeft <=(Block_Size - blockofff) \n", bitesRed);

                // Free Mallocs
                free(dataBlockBuffer);
                free(file_ino);

                return bitesRed;
            }

            else{
                if (bitesRed == 0){
                    memcpy((void *) (buf + bitesRed), (void *) (dataBlockBuffer + blockoff), BLOCK_SIZE - blockoff);
                    bitesRed += (BLOCK_SIZE - blockoff);
                    bitesLeft -= bitesRed;
                    blockidx++;

                    log_msg("Read %i bytes from file. First Read\n", bitesRed);

                    // Change Block Offset to 0
                    blockoff = 0;
                }
                else{
                    memcpy((void *) (buf + bitesRed), (void *) (dataBlockBuffer), BLOCK_SIZE);
                    bitesRed += BLOCK_SIZE;
                    bitesLeft -= bitesRed;
                    blockidx++;

                    log_msg("Read %i bytes from file. Not first\n", bitesRed);
                }
            }
        }
    }
    // Free Mallocs
    free(dataBlockBuffer);
    free(file_ino);

    return bitesRed;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int sfs_write(const char *path, const char *buf, size_t size, off_t offset,
       struct fuse_file_info *fi)
{

    log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
      path, buf, size, offset, fi);
    
    char * dataBlockBuffer;
    unsigned int bitesLeft, bitesWrote, readResult, writeResult;
    unsigned int startBlockIdx, endBlockIdx;
    inode * file_ino;
    int file_inum = inodeFromPath(path);
    
    if (file_inum != -1)
    {
      log_msg("Found inode number: %i\n", file_inum);

      // Copy the actual inode into file_ino
      file_ino = (inode *) malloc(sizeof(inode));
      getInode(file_inum, file_ino);
      log_msg("Inode number from getInode: %i\n", file_ino->iNum);
      bitesLeft = size;
      bitesWrote = 0;
      startBlockIdx = (offset / BLOCK_SIZE);
      //int startBlockOff = (unsigned int)((unsigned int)offset - (unsigned long)(BLOCK_SIZE * startBlockIdx));
      int startBlockOff = (offset % BLOCK_SIZE);
      endBlockIdx =  ((size + offset)/BLOCK_SIZE);
      endBlockIdx = (((size + offset) % BLOCK_SIZE) == 0) ? (endBlockIdx -1) : (endBlockIdx);
 

      log_msg("startBlockIdx = %i\n", startBlockIdx);
      log_msg("startBlockOff = %lld\n", startBlockOff);
      log_msg("endBlockIdx = %i\n", endBlockIdx);


      // Detect if additional blocks need to be allocated
      // I.E. startBlockIdx > numBlocks
      if (endBlockIdx > NDBLOCK_PTRS){
        if((size + offset) % BLOCK_SIZE != 0){
          log_msg("File system cant handle file that big\n");
          return -1;
        }
        
      }

      if (endBlockIdx >= file_ino->numBlocks)
      {
        unsigned int neededBlocks = endBlockIdx - file_ino->numBlocks ;
        unsigned int currBlockIdx = file_ino->size / BLOCK_SIZE;

        if(!file_ino->directBlockPtr[currBlockIdx])
          file_ino->directBlockPtr[currBlockIdx] = findFreeBlock();
          log_msg("Found free block: %i inodeDB INdex: %i\n",  file_ino->directBlockPtr[currBlockIdx], currBlockIdx );

        unsigned int x = 0;
        log_msg("Need %i Blocks start at Current Block : %i CurrBlock Value: %i \n", neededBlocks, currBlockIdx,file_ino->directBlockPtr[currBlockIdx]);
        for (; x < neededBlocks; x++)
        {
          currBlockIdx++;
          file_ino->directBlockPtr[currBlockIdx] = findFreeBlock();
          log_msg("CurrBlockIdx %i Allocate DB: %i\n", currBlockIdx, file_ino->directBlockPtr[currBlockIdx]);
        }
        updateInode(file_ino);
      }
      

      // Begin write operations
      dataBlockBuffer = malloc(BLOCK_SIZE);
      while (bitesLeft > 0)
      {
        log_msg("Attempting to write %i bytes.\n", bitesLeft);
        log_msg("startBlockIdx = %i.\n", startBlockIdx);
        log_msg("startBlockOff = %i.\n", startBlockOff);
        log_msg("endBlockIdx = %i.\n", endBlockIdx);

        memset(dataBlockBuffer, 0, BLOCK_SIZE);
        readResult = block_read(file_ino->directBlockPtr[startBlockIdx] + DBLOCK_IDX, dataBlockBuffer);
        if (readResult < 0){
          log_msg("Failed to Read Data Block %i", file_ino->directBlockPtr[startBlockIdx] + DBLOCK_IDX);

          // Free Mallocs
          free(dataBlockBuffer);
          free(file_ino);

          return 0;
        }

        // First, if needed, finish writing until first block to be written to is finished
        // Then, Write one full block at a time
        if (bitesLeft <= (BLOCK_SIZE - startBlockOff))
        {
          
          log_msg("bitesLeft can fit on the rest of the current block bitesLeft: %i startBlockOff %i\n", bitesLeft, startBlockOff);
          memcpy((void *) (dataBlockBuffer + startBlockOff), (void *) (buf + bitesWrote), bitesLeft);
          writeResult = block_write(file_ino->directBlockPtr[startBlockIdx] + DBLOCK_IDX, dataBlockBuffer);
          if (writeResult < BLOCK_SIZE){
            log_msg("Failed to Write to Data Block %i", (file_ino->directBlockPtr[startBlockIdx] + DBLOCK_IDX));

            // Free Mallocs
            free(dataBlockBuffer);
            free(file_ino);

            return writeResult;
          }
          
          bitesWrote += (bitesLeft);
          file_ino->size += bitesLeft;
          bitesLeft -= (bitesLeft);
          updateInode(file_ino);
          log_msg("Written %i bytes to data block %i.\n", bitesWrote, (file_ino->directBlockPtr[startBlockIdx] + DBLOCK_IDX));

          // Free Mallocs
          free(dataBlockBuffer);
          free(file_ino);

          return bitesWrote;
        }
        else
        {
          if (bitesWrote == 0)
          {
            
            memcpy((void *) (dataBlockBuffer + startBlockOff), (void *) (buf + bitesWrote), BLOCK_SIZE - startBlockOff);
            writeResult = block_write(file_ino->directBlockPtr[startBlockIdx] + DBLOCK_IDX, dataBlockBuffer);
            
            if (writeResult < BLOCK_SIZE){
              log_msg("Failed to Write to Data Block %i", file_ino->directBlockPtr[startBlockIdx] + DBLOCK_IDX);

              // Free Mallocs
              free(dataBlockBuffer);
              free(file_ino);

              return writeResult;
            }

            bitesWrote += (BLOCK_SIZE - startBlockOff);
            bitesLeft -= (BLOCK_SIZE - startBlockOff);
            file_ino->size += (BLOCK_SIZE - startBlockOff);
            startBlockOff = 0;
            log_msg("Written %i bytes to data block %i. -> %i Bytes left to write\n", bitesWrote, file_ino->directBlockPtr[startBlockIdx] + DBLOCK_IDX, bitesLeft);
          
            startBlockIdx ++;
            file_ino->numBlocks++;
            updateInode(file_ino);
            }
          else
          {
            memcpy((void *) (dataBlockBuffer + startBlockOff), (void *) (buf + bitesWrote), BLOCK_SIZE);
            writeResult = block_write(file_ino->directBlockPtr[startBlockIdx] + DBLOCK_IDX, dataBlockBuffer);
            if (writeResult < BLOCK_SIZE){
              log_msg("Failed to Write to Data Block %i", file_ino->directBlockPtr[startBlockIdx] + DBLOCK_IDX);

              // Free Mallocs
              free(dataBlockBuffer);
              free(file_ino);

              return writeResult;
            }

            bitesWrote += BLOCK_SIZE;
            bitesLeft -= BLOCK_SIZE;
            startBlockIdx++;
            file_ino->size += BLOCK_SIZE;
            file_ino->numBlocks++;
            updateInode(file_ino);
            log_msg("Written %i bytes to data block %i Index: %i\n", bitesWrote, file_ino->directBlockPtr[startBlockIdx] + DBLOCK_IDX, startBlockIdx);
          }
        }
      }
    }
    // Free Mallocs
    free(dataBlockBuffer);
    free(file_ino);

    return -1;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode)
{
    // Seems would be same as create file. 
    log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n", path, mode);
    int retstat = 0;
    char fpath[NAME_LEN];
    strcpy(fpath, path);

    retstat = createInode(fpath, (S_IFDIR | 0755));
   
    
    return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path)
{
    int retstat = 0;
    log_msg("sfs_rmdir(path=\"%s\")\n",
      path);

    char fpath[NAME_LEN];
    strcpy(fpath, path);


    int dirInode = inodeFromPath(fpath);
    int dirInodeIndex = getInodeIndex(dirInode);

    int parentNum = getParentInodeFP(fpath);
    int parentInodeIndex = getInodeIndex(parentNum);
    
    if(dirInode == 0){
      log_msg("nCan not delete Root!!!\n");
      retstat = EFAULT;
    }
    else if(dirInode < 0){
        log_msg("\nDirectory not found\n");
        retstat = EFAULT;
    }
    else{
        inode * directory = malloc(sizeof(inode));
        getInode(dirInode, directory);
        if(removeInode(dirInode, S_IFDIR ) == -1){
            log_msg("Unable to remove Inode: %i\n", dirInode);
            return -1;
        }

        if(deleteInodeEntry(dirInode, parentNum) == -1){
            log_msg("Could not fine Inode %i entry in Directory Inode: %i\n", dirInode, parentNum);
            retstat = -1;
        }
        else{
            log_msg("Deleting all entrys for Directory Inode:%i\n", dirInode);
            deleteAllInodeEntries(directory);
        }


    }
    return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi)
{

    int retstat = 0;
    log_msg("\nsfs_opendir(path=\"%s\", fi=0x%08x)\n",
    path, fi);
    
    inode * inodeReq = malloc(sizeof(inode));

    int inodeNumber = inodeFromPath(path);
    if (inodeNumber != -1){
      getInode(inodeNumber,inodeReq);
      if (S_ISDIR(inodeReq->fileMode)){
        free(inodeReq);
        log_msg("\nDirectory Successfully found for path: %s\nInode Number: %i\n", path, inodeNumber);
        return retstat;
      }
      else{
        free(inodeReq);
        log_msg("\nError: Directory not found for path %s\n", path);
      return ENOENT;
      }
    }
    else{
      free(inodeReq);
      log_msg("\nError: Directory not found for path %s\n", path);
      return ENOENT;
    }
    free(inodeReq);
    return -1;

    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
         struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_readdir(path=\"%s\")\n", path);
    int directoryNum;
    if(strcmp(path, "/") == 0){
      directoryNum = 0;
    }
    else{
      directoryNum = inodeFromPath(path);
    } 
    if(directoryNum >= 0){
      log_msg("\nFound Pat Inode NUM: %i\n", directoryNum);
      inode * directoryInode = malloc(sizeof(inode));
      getInode(directoryNum, directoryInode);
      int i = 0;
      if(directoryInode->iNum < 0 || !S_ISDIR(directoryInode->fileMode)) return -1;
      int bytesChecked = 0;
      char * dataBlockBuffer = malloc(BLOCK_SIZE);
      char * ptr;
      int readResult;
      dirEntry * entry;
      for(; i < directoryInode->numBlocks; i++){
          memset(dataBlockBuffer, 0, BLOCK_SIZE);
          readResult = block_read(DBLOCK_IDX + directoryInode->directBlockPtr[i], dataBlockBuffer);
          ptr = dataBlockBuffer;
          entry = (dirEntry *)ptr;
          //log_msg("Checking entry: %i which has %i Inode and %s filename\n", entryCounter, entry->iNum, entry->fileName);
          while(entry->dirLength > 0 && bytesChecked <= BLOCK_SIZE){
            log_msg("Entry with Inode: %i which has %s filename\n Bytes Checked: %i\n BLOCK_SIZE: %i\n Entry->dirLength: %i\n\n", entry->iNum, entry->fileName, bytesChecked, BLOCK_SIZE, entry->dirLength);
            
            if((entry->iNum) != -1){
                log_msg("Inode %i Valid to List\n", entry->iNum);
                filler(buf, entry->fileName, NULL, 0);
            }
                
            
            bytesChecked += entry->dirLength;
            ptr+= entry->dirLength;
            entry = (dirEntry *)ptr;
          }
     }
  }
  else{
    log_msg("Not valid Directory Path");
    retstat = -ENOENT;
  }
  return retstat;
}


    


/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;

    
    return retstat;
}

struct fuse_operations sfs_oper = {
  .init = sfs_init,
  .destroy = sfs_destroy,

  .getattr = sfs_getattr,
  .create = sfs_create,
  .unlink = sfs_unlink,
  .open = sfs_open,
  .release = sfs_release,
  .read = sfs_read,
  .write = sfs_write,

  .rmdir = sfs_rmdir,
  .mkdir = sfs_mkdir,

  .opendir = sfs_opendir,
  .readdir = sfs_readdir,
  .releasedir = sfs_releasedir
};

void sfs_usage()
{
    fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{
    int fuse_stat;
    struct sfs_state *sfs_data;
    
    // sanity checking on the command line
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
  sfs_usage();

    sfs_data = malloc(sizeof(struct sfs_state));
    if (sfs_data == NULL) {
  perror("main calloc");
  abort();
    }

    // Pull the diskfile and save it in internal data
    sfs_data->diskfile = argv[argc-2];
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    sfs_data->logfile = log_open();
    
    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main, %s \n", sfs_data->diskfile);
    fuse_stat = fuse_main(argc, argv, &sfs_oper, sfs_data);
       fprintf(stderr, "fuse_main returned \n");
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}
