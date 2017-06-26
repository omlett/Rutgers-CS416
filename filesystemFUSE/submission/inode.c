#include "inode.h"
#include "stdefs.h"
#include <errno.h>
#include "dirent.h"  
//#include "dirent.h"

#define ROOT_IDX 0

int inodeDebug = 1;
int getAttDebug = 1;
char dataBlockBuffer[BLOCK_SIZE];


int createInode(char * path, mode_t mode){
    int freeInodeNum = findFreeInode();
    int freeInodeIndex = freeInodeNum;
    log_msg("\nFound free inode: %i\n for file %s", freeInodeNum, path);

    if(freeInodeIndex < 0){
      log_msg("Could not find free inode");
      return -ENOENT;
    }

    int inodeBlockIndex = getInodeBlockIndex(freeInodeIndex);
    inode * inodeTable = malloc(BLOCK_SIZE);

    if(inodeDebug){
        log_msg("\nInode Block Index %i \n\n",  inodeBlockIndex);
        log_msg("\nNumber of Inodes per Block %i \n", NINODES_BLOCK);
    }
 
    readResult = block_read(inodeBlockIndex, inodeTable);

    freeInodeIndex = getInodeIndex(freeInodeIndex);

    log_msg("\nInode Index %i in Block %i Size: %i\n", freeInodeIndex, (freeInodeNum / NINODES_BLOCK),inodeTable[freeInodeIndex].size );
    
    if(inodeTable[freeInodeIndex].size == -1){ // Inode is free
        inode * inode = inodeTable;
        inode += freeInodeIndex;
        
        if(inodeDebug){
          log_msg("Inode %i - Size: %i NumBlocks: %i\n", inode->iNum, inode->size, inode->numBlocks);
          log_msg("MemAddress inodeTable: %p, MemAddres Inode: %p  Difference %p\n", inodeTable, inode, (inode-inodeTable) );
          log_msg("\nCreating inode at index %i\n BLOCK: %i address: %p start: %p", freeInodeIndex, inodeBlockIndex, inode, inodeTable);
        }

        inode->iNum = freeInodeNum;
        inode->size = 0;            
        inode->atime = time(NULL);
        inode->mtime = time(NULL); 
        inode->userID = getuid();
        inode->fileMode = mode;
        inode->nLinks = (S_ISDIR(mode)) ? 2: 1;
        inode->numBlocks = 1;
        inode->bitPos = freeInodeIndex; 
        inode->directBlockPtr[0] = findFreeBlock();
        
        if(inode->directBlockPtr[0] < 0){
            log_msg("\nCould not allocate a datablock for Inode: %i\n", freeInodeNum);  
            exit(0); 
        } 

        if(inodeDebug)
            log_msg("\nInode %i Created with Allocated Data Block: %i\n", inode->iNum, inode->directBlockPtr[0]);

        char * parentDirectory = malloc(NAME_LEN);
        char * fileName = strrchr(path, '/') + 1;
        int lengthParentName = strlen(path) - strlen(fileName);
        lengthParentName = (lengthParentName < NAME_LEN) ? lengthParentName : NAME_LEN - 1;
        strncpy(parentDirectory, path, lengthParentName);
        parentDirectory[lengthParentName] = '\0';

        int parentInodeNumber = inodeFromPath(parentDirectory);
        if(inodeDebug)
            log_msg("ParentInodeNumber:%i\n", parentInodeNumber);
        
        if(S_ISDIR(mode)) 
            initializeDEntries(inode->directBlockPtr[0], inode->iNum, parentInodeNumber);

        if(inodeDebug)
            log_msg("Made Initial Directory Entries Inode: %i iParentInodeNumber:%i\n", inode->iNum, parentInodeNumber);

        createDEntry(inode->iNum, fileName, parentInodeNumber, mode);
        writeResult = block_write(inodeBlockIndex, inodeTable);
        free(parentDirectory);
        free(inodeTable);
    }

    else{
        log_msg("\nInode is not free size %i\n",inodeTable[freeInodeIndex% NINODES_BLOCK].size);
        free(inodeTable);
        return EFAULT;
    }
    
    return 0;

}

void createRoot(inode * inodeTable){
    log_msg("\nCreating Root for FileSystem\n");

    inodeTable[ROOT_IDX].fileMode = S_IFDIR | 0755;
    inodeTable[ROOT_IDX].nLinks = 2;
    inodeTable[ROOT_IDX].ctime = time(&(inodeTable[0].ctime));
    inodeTable[ROOT_IDX].mtime = time(&(inodeTable[0].mtime));
    inodeTable[ROOT_IDX].groupID = getegid();
    inodeTable[ROOT_IDX].bitPos = 0;
    inodeTable[ROOT_IDX].userID = getuid();
    inodeTable[ROOT_IDX].size = 0;
    inodeTable[ROOT_IDX].directBlockPtr[0] = findFreeBlock();
    inodeTable[ROOT_IDX].numBlocks = 1;
    inodeTable[ROOT_IDX].iNum = 0;

    if(inodeTable[ROOT_IDX].directBlockPtr[0] < 0){
        log_msg("\nCould not allocated Data Block for Root\n");
        exit(0);
    } 

    initializeDEntries(inodeTable[ROOT_IDX].directBlockPtr[0], inodeTable[ROOT_IDX].iNum, inodeTable[ROOT_IDX].iNum);
}

int findFreeInode(){
    int * inodeBitmap = malloc(BLOCK_SIZE);
    readResult = block_read(IBITMAP_IDX, inodeBitmap);
    int inodeIndex = findFirstFree(inodeBitmap);

    if(inodeIndex < 1){
        log_msg("\nCould Not Find Free Inode\n");
        free(inodeBitmap);
        return inodeIndex;
    }

    writeResult = block_write(IBITMAP_IDX, inodeBitmap);
    free(inodeBitmap);

    log_msg("\nFound Free Inode at index %i in Bitmap\n", inodeIndex);
    return inodeIndex;
}

void getInode(unsigned int inodeNumber, inode * inodeData){
    log_msg("\nGet Inode: %i\n", inodeNumber);

    inode * inodeTable = malloc(BLOCK_SIZE);
    
    int inodeBlockIndex = getInodeBlockIndex(inodeNumber); 
    readResult = block_read(inodeBlockIndex, inodeTable);
    
    int inodeIndex = getInodeIndex(inodeNumber);
    memcpy(inodeData, &(inodeTable[inodeIndex]), sizeof(inode));

    log_msg("\nGot Inode: %i  inodeNumber: %i size = %i\n", inodeTable[inodeIndex].iNum, inodeNumber, inodeTable[inodeIndex].size);

    free(inodeTable);
}

void findInode(const char * iPath, inode * _inode, unsigned int iParent){
    int i = 0;
    int bytesChecked = 0;
    inode * inodeTable = (inode *)malloc(BLOCK_SIZE);
    char * ptr;

    log_msg("\nfindInode IPATH: %s Parent: %i\n", iPath, iParent);
    readResult = block_read((ITABLE_IDX + (iParent/NINODES_BLOCK)), inodeTable);
    iParent = getInodeIndex(iParent);
    inode * parentInode  = inodeTable + iParent;
    char * dataBlockBuffer = malloc(BLOCK_SIZE);

    for(; i< parentInode->numBlocks; i++){
        memset(dataBlockBuffer, 0, BLOCK_SIZE);
        readResult = block_read((parentInode->directBlockPtr[i] + DBLOCK_IDX), dataBlockBuffer);
        ptr = dataBlockBuffer;
        dirEntry * entry = (dirEntry *)ptr;

        while(entry->dirLength && bytesChecked <= BLOCK_SIZE){
            log_msg("\nChecking Entry for Inode: %i with Filename : %s", entry->iNum, entry->fileName);

            if(strcmp(entry->fileName, iPath) == 0){
                log_msg("\nFound entry %s Inode: %i\n", entry->fileName, entry->iNum);
                getInode(entry->iNum, _inode);
                log_msg("\ngetInode return with Inode: %i size: %i\n", _inode->iNum, _inode->size);
                free(inodeTable);
                free(dataBlockBuffer);
                return;
            }

            bytesChecked += entry->dirLength;
            ptr+= entry->dirLength;
            entry = (dirEntry *)ptr;
        }
    }

    log_msg("\n findInode could not find Inode\n");
    free(inodeTable);
    free(dataBlockBuffer);
    _inode->iNum= -1;
}



int inodeFromPath(const char * path){
    int parentNum = 0;

    if(inodeDebug)
        log_msg("Find inode for path %s\n", path);

    if(strcmp(path, "/") == 0){ // ROOT DIRECTORY
        return ROOT_IDX;     
    }
    
    char fpath[NAME_LEN];
    memset(fpath, 0, NAME_LEN);
    strcpy(fpath, path);
    char * buffer = malloc(BLOCK_SIZE);
    int readResult = block_read(ITABLE_IDX, buffer);
    
    inode * ptr = (inode *)buffer;

    if(*fpath == '/'){
        inode * _inode = malloc(sizeof(inode));
        char * iPath = strtok(fpath, "/");
        log_msg("\nIPath: %s\n",iPath);
        while(iPath != NULL){
            parentNum = ptr->iNum;
            findInode(iPath, _inode, ptr->iNum);
            ptr = _inode;
            log_msg("\nInode: %i size: %i\n", ptr->iNum, ptr->size);

            if(!(ptr->iNum))
                return -1;
            
            iPath = strtok(NULL, "/");
            log_msg("\nIPath: %s\n",iPath) ;
        }

        log_msg("\nParent INUM: %i PTR INUM: %i\n", parentNum, ptr->iNum);

        if((ptr->iNum > 0)  && (ptr->iNum != parentNum) && (ptr->iNum < TOTAL_INODES)){
            int ptrNum = ptr->iNum;
            free(_inode);
            return ptrNum;
        }

        free(_inode);
        return -1;
    }
}

int getChild(char * iPath, unsigned int parent){
    inode parentInode;
    int inodeNumber;
    int i = 0;
    char * buffer = malloc(BLOCK_SIZE);
    int inodeBlockIndex = parent / NINODES_BLOCK;
    int inodeIndex = (parent >= NINODES_BLOCK) ? parent%NINODES_BLOCK : parent;
    readResult = block_read(inodeBlockIndex + ITABLE_IDX, buffer);
    inode * ptr = (inode *) buffer;
    char * ptr1; 
    memcpy(&parentInode, &ptr[inodeIndex], sizeof(inode));
    unsigned int bytesChecked = 0;

    for(; i < parentInode.numBlocks; i++){
        memset(buffer, 0, BLOCK_SIZE);
        readResult = block_read(parentInode.directBlockPtr[i], buffer);
        dirEntry * entry = (dirEntry *)buffer;
        ptr1 = (char *) entry;
        while(entry->iNum && bytesChecked < BLOCK_SIZE){
            entry = (dirEntry *)ptr;

            if(strcmp(entry->fileName, iPath) == 0){
                inodeNumber = entry->iNum;
                free(buffer);
                return inodeNumber;
            }

            bytesChecked += entry->dirLength;
            ptr1 += entry->dirLength;
        }
    }

    return -1;

}

void updateInode(inode * _inode)
{
  char * buffer = malloc(BLOCK_SIZE);
  int inodeIndex;
  int inodeBlockIndex = _inode->iNum / NINODES_BLOCK;
  int readResult = block_read(ITABLE_IDX + inodeBlockIndex, buffer);
  if(readResult < 0){
     log_msg("Cant Read Inode Table Block\n");
     exit(0);
  }
  inodeIndex = _inode->iNum;
  if(_inode->iNum >= NINODES_BLOCK) inodeIndex = _inode->iNum % NINODES_BLOCK;

  inode * inodeTable = (inode *)buffer;
  inode * inodeData = &inodeTable[inodeIndex];
  memcpy(inodeData, _inode, sizeof(inode));
  log_msg("updateInode: %i\n", _inode->iNum);

  int writeResult = block_write(ITABLE_IDX + inodeBlockIndex, buffer);
  if (writeResult < 0){
    log_msg("updateInode failed to write to block!\n");
  }
  free(buffer);
}

void fillStat(struct stat *statbuf, inode * inode){
    //log_msg("\nInode: %i Found with Same file path  \n", inode->iNum);
    statbuf->st_ino = inode->iNum;
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
    
    if(getAttDebug)
        log_stat(statbuf);
}

int getParentInodeFP(char * filepath){
    log_msg("Filepath: %s\n", filepath);
    char * parentDirectory = malloc(NAME_LEN);
    char * fileName = strrchr(filepath, '/') + 1;
    int lengthParentName = strlen(filepath) - strlen(fileName);
    strncpy(parentDirectory, filepath, lengthParentName);
    parentDirectory[lengthParentName] = '\0';
    if(inodeDebug)
        log_msg("ParentDirectory: %s", parentDirectory);
    
    return inodeFromPath(parentDirectory);

}

int getParentInode(inode * childInode){
    int entryBlock = childInode->directBlockPtr[0] + DBLOCK_IDX;
    char * buffer = malloc(BLOCK_SIZE);
    int readResult = block_read(entryBlock, buffer);
    char * ptr = buffer;
    dirEntry * entry = (dirEntry *) ptr;
    ptr += entry->dirLength;
    return ((dirEntry *)ptr)->iNum;
}

int getInodeIndex(unsigned int inodeIndex){
    return (inodeIndex >= NINODES_BLOCK) ? (inodeIndex % NINODES_BLOCK) : inodeIndex;
}

int getInodeBlockIndex(unsigned int inodeIndex){
    return (ITABLE_IDX + (inodeIndex / NINODES_BLOCK));
}

int removeInode(unsigned int inodeNum, mode_t mode){
    int inodeBlockIndex = ITABLE_IDX + (inodeNum / NINODES_BLOCK);
    inode * inodeTable = malloc(BLOCK_SIZE);
    readResult = block_read(inodeBlockIndex, inodeTable);
    int inodeIndex = getInodeIndex(inodeNum);
   
    inode * _inode = inodeTable + inodeIndex;
    if(_inode->size == -1){
        log_msg("Not a valid Inode Can not Delete\n");

        if(inodeDebug){
            log_msg("Inode %i - Size: %i NumBlocks: %i\n", _inode->iNum, _inode->size, _inode->numBlocks);
            log_msg("MemAddress inodeTable: %p, MemAddres Inode: %p  Difference %p\n", inodeTable, _inode, (_inode-inodeTable) );
            log_msg("\nDeleting inode at index %i\n BLOCK: %i address: %p start: %p", _inode, inodeBlockIndex, _inode, inodeTable);
        }

        free(inodeTable);
        return -1;
     }
    else if (!((S_ISDIR(_inode->fileMode)) == S_ISDIR(mode))){
        log_msg("\nThis is not the right fileMode..... cant not remove \n");
        free(inodeTable);
        return -1;
    }
    else{
        log_msg("\nRemoving Inode: %i IndexT: %i Block: %i\n", _inode->iNum, inodeIndex, inodeBlockIndex); 
        memset(_inode, 0, sizeof(inode));
        _inode->size = -1;
        _inode->iNum = inodeNum;
        writeResult = block_write(inodeBlockIndex, inodeTable);
        free(inodeTable);
        return 1;
    }
}

int deleteInodeEntry(int inodeNum, int parentNum){
    inode * parentInode = malloc(sizeof(inode));
    getInode(parentNum, parentInode);
    char * dataBlockBuffer = malloc(BLOCK_SIZE);
    int i = 0;
    char * ptr;
    int bytesChecked; 
    for(i = 0; i< parentInode->numBlocks; i++){
        bytesChecked = 0;
        memset(dataBlockBuffer, 0, BLOCK_SIZE);
        readResult = block_read((parentInode->directBlockPtr[i] + DBLOCK_IDX), dataBlockBuffer);
        ptr = dataBlockBuffer;
        dirEntry * entry = (dirEntry *)ptr;
        char * frontOfBlock = ptr;
        char * backOfBlock = ptr + BLOCK_SIZE;
        log_msg("Removing Entry for Inode: %i In Directory: %i\n", inodeNum, parentNum);
        while((entry->dirLength > 0) && (bytesChecked < BLOCK_SIZE)){
            log_msg("\nChecking Entry for Inode: %i with Filename : %s", entry->iNum, entry->fileName);

            if(entry->iNum == inodeNum){
                log_msg("\nFound Directory entry %s Inode: %i\n", entry->fileName, entry->iNum);
                int deletedLength = entry->dirLength;
                memcpy(entry, (ptr+=deletedLength), (BLOCK_SIZE - bytesChecked - deletedLength));
                memset((backOfBlock - deletedLength), 0, deletedLength);
                block_write((parentInode->directBlockPtr[i] + DBLOCK_IDX), dataBlockBuffer);
                log_msg("Removed Entry\n");
                free(dataBlockBuffer);
                free(parentInode);
                return 1;
            }

            bytesChecked += entry->dirLength;
            ptr+= entry->dirLength;
            entry = (dirEntry *)ptr;
        }
    }

    log_msg("Could not find Entry to Delete\n");
    //free(dataBlockBuffer);
    free(parentInode);
    return -1;

}

void deleteAllInodeEntries(inode * _inode){
    //char * dataBlockBuffer = malloc(BLOCK_SIZE);
    int i = 0;
    char * ptr;
    int bytesChecked;
    for(i = 0; i< _inode->numBlocks; i++){
        bytesChecked = 0;
        memset(dataBlockBuffer, 0, BLOCK_SIZE);
        readResult = block_read((_inode->directBlockPtr[i] + DBLOCK_IDX), dataBlockBuffer);
        ptr = dataBlockBuffer;
        dirEntry * entry = (dirEntry *)ptr;
        char * frontOfBlock = ptr;
        char * backOfBlock = ptr + BLOCK_SIZE;
        int entryInodeNum;
        log_msg("Removing All Entries in Inode: %i\n",  _inode->iNum);
        
        while(bytesChecked < BLOCK_SIZE && (entry->dirLength > 0)){
            if(((entry->iNum) > 0) && (strcmp(entry->fileName, "..") != 0) && (strcmp(entry->fileName, ".") != 0)){
                log_msg("\nDeleting Entry for Inode: %i with Filename : %s EntryLength: %i\n", entry->iNum, entry->fileName, entry->dirLength);
                inode * currInode = malloc(sizeof(inode));
                getInode(entry->iNum, currInode);
                removeInode(currInode->iNum, currInode->fileMode);
                int deletedLength = entry->dirLength;
                memcpy(entry, (ptr+=deletedLength), (BLOCK_SIZE - bytesChecked - deletedLength));
                memset((backOfBlock - deletedLength), 0, deletedLength);
                deleteAllInodeEntries(currInode);
                free(currInode);
            }

            bytesChecked += entry->dirLength;
            ptr+= entry->dirLength;
            entry = (dirEntry *)ptr;
        }
    }
    
    //free(dataBlockBuffer);
    free(_inode);
}
    


