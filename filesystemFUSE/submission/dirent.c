#include "dirent.h"

dirEntry * entry; 

void initializeDEntries(unsigned int dataBlockIndex, unsigned int inodeNum, int parentNum){
   log_msg("Initializing DEntries for Inode: %i AT DataBlock_IDX %i\n", inodeNum, dataBlockIndex);

    char * buffer = malloc(BLOCK_SIZE);
    memset(buffer, 0, BLOCK_SIZE);
    char * ptr = buffer;

    entry = (dirEntry *)buffer;
    entry->iNum = inodeNum;
    entry->fileType = 'd';
    strcpy(entry->fileName, ".");
    entry->dirLength = DENTRY_MIN + strlen(entry->fileName);

    ptr += entry->dirLength;

    entry = (dirEntry *)ptr;
    entry->iNum = parentNum;
    entry->fileType = 'd';
    strcpy(entry->fileName, "..");
    entry->dirLength = DENTRY_MIN + strlen(entry->fileName);

    block_write(dataBlockIndex + DBLOCK_IDX, buffer);
    log_msg("\nFinished Initializing DEntries for Inode: %i Parent: %i\n", inodeNum, parentNum);
    free(buffer);


}


void createDEntry(unsigned int inodeNumber, char * filename, unsigned int parentInodeNumber, mode_t mode){
   log_msg("\nCreating Directory Entry for Inode: %i With Filename: %s Inside Directory: %i\n", inodeNumber, filename, parentInodeNumber);
    char * buffer = malloc(BLOCK_SIZE);
    memset(buffer, 0, BLOCK_SIZE);
    int i = 0;
    char * ptr;
    int bytesChecked;
    char * dataBlockBuffer = malloc(BLOCK_SIZE);
    int entryCounter = 0;

    int parentInodeBlock = (parentInodeNumber / NINODES_BLOCK) + ITABLE_IDX;
    int parentIndex = parentInodeNumber;

    if(parentInodeBlock >= NINODES_BLOCK) 
      parentIndex = parentInodeNumber % NINODES_BLOCK;

    readResult = block_read(parentInodeBlock, buffer);

    inode * parentInode = (inode *) buffer;
    int numBlocks = parentInode[parentIndex].numBlocks;

    for(; i < numBlocks; i++){
        bytesChecked = 0;
        memset(dataBlockBuffer, 0, BLOCK_SIZE);
        readResult = block_read(DBLOCK_IDX + parentInode[parentIndex].directBlockPtr[i], dataBlockBuffer);
     
        ptr = dataBlockBuffer;
        entry = (dirEntry *)ptr;
   
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
            strcpy(entry->fileName, filename);
            writeResult = block_write(DBLOCK_IDX + parentInode[parentIndex].directBlockPtr[i], dataBlockBuffer);
            log_msg("Wrote entry to block Inode: %i, fileName: %s Entrylength: %i fileType: %c", entry->iNum, entry->fileName, entry->dirLength, entry->fileType);
            return;
        }
    }

    log_msg("\nCould not find space to create entry checking if data block can be allocated\n");
                 
    int freeBlock; 

    while(numBlocks < NDBLOCK_PTRS){      
        
        freeBlock = findFreeBlock();      
        
        if(freeBlock > 0){        
            parentInode[parentIndex].directBlockPtr[numBlocks] = freeBlock;       
            parentInode[parentIndex].numBlocks++;
            block_write(parentInodeBlock, buffer);     
            numBlocks = parentInode[parentIndex].numBlocks;       
            bytesChecked = 0;     
            memset(dataBlockBuffer, 0, BLOCK_SIZE);       
            readResult = block_read(DBLOCK_IDX + numBlocks, dataBlockBuffer);     
            ptr = dataBlockBuffer;        
            entry = (dirEntry *)ptr;      
       
            while(entry->dirLength > 0 && bytesChecked < BLOCK_SIZE && (entry->iNum > 0)){        
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
                strcpy(entry->fileName, filename);        
                writeResult = block_write(DBLOCK_IDX + parentInode[parentIndex].directBlockPtr[i], dataBlockBuffer);      
                log_msg("Wrote entry to block Inode: %i, fileName: %s Entrylength: %i fileType: %c", entry->iNum, entry->fileName, entry->dirLength, entry->fileType);        
                return;       
            }     
        }              
    }

    log_msg("No space for Entry in direct Blocks looking in single indirection\n");
    
   /* if(!parentInode[parentIndex].indirectBlockPtr){
        memset(buffer, 0, BLOCK_SIZE);
        block_read(parentInodeBlock, buffer);
        parentInode[parentIndex].indirectBlockPtr = findFreeBlock();
        
        if(!parentInode[parentIndex].indirectBlockPtr){
            log_msg("Could not allocate indirect block\n");
            if(deleteInodeEntry(inodeNumber, parentInodeNumber)){     
                log_msg("Successfully Deleted Inode %i for lack of entry space\n", inodeNumber);       
                return;       
            }

            log_msg("Fuck couldnt Deleted Inode %i for lack of entry space\n", inodeNumber);
            return;            
        }

        block_write(parentInodeBlock, buffer);
    }
        
    char * indirectBuffer = malloc(BLOCK_SIZE);
    block_read(parentInode[parentIndex].indirectBlockPtr + DBLOCK_IDX, indirectBuffer);
    ptr = indirectBuffer;
    char * dataPtr;
    int inDirectBytesChecked = 0;
    bytesChecked = 0;
    dirEntry * entry;

    while(indirectBytesChecked < BLOCK_SIZE){
        int blockNum = *(int *) ptr;
        
        if(blockNum){
            memset(dataBlockBuffer, 0, BLOCK_SIZE);
            block_read(DBLOCK_IDX + blockNum, dataBlockBuffer);
            dataPtr = dataBlockBuffer;
            entry = dataPtr;

            while(entry->dirLength > 0 && bytesChecked < BLOCK_SIZE && (entry->iNum > 0)){        
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
                strcpy(entry->fileName, filename);        
                writeResult = block_write(DBLOCK_IDX + parentInode[parentIndex].directBlockPtr[i], dataBlockBuffer);      
                log_msg("Wrote entry to block Inode: %i, fileName: %s Entrylength: %i fileType: %c", entry->iNum, entry->fileName, entry->dirLength, entry->fileType);        
                return;       
            } 
        }
        
        if((BLOCK_SIZE - indirectBytesChecked) > sizeof(int)){      
            *(int *)ptr = findFreeBlock();
            
            if(!*(int *)ptr){
                log_msg("\nCould not sucessfully allocated data block for on single indirection block\n");
                return;
            }
            
            block_write(parentInode[parentIndex].indirectBlockPtr + DBLOCK_IDX, indirectBuffer); 
        }
    }*/
         
    log_msg("No Space for entry Deleting Inode: %i\n", inodeNumber);      
           
    if(deleteInodeEntry(inodeNumber, parentInodeNumber)){     
        log_msg("Successfully Deleted Inode %i for lack of entry space\n");       
        return;       
    }     
       
    log_msg("Couldnt delete Inode: %i So Fuck your screwed\n", inodeNumber);      
       
}
