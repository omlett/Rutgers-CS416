#ifndef _INODE_H_
#define _INODE_H_
#include <sys/stat.h>
#include "stdefs.h"
//#include "dirent.h"

#define ROOT_IDX 0

int writeResult;
int readResult;
//https://web.cs.wpi.edu/~rek/DCS/D04/UnixFileSystems.html
// Where im getting most of information

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


/* According to lecutre notes 
 * I-node contains: owner, type (directory, file, device), last modified
 * time, last accessed time, last I-node modified time, access
 * permissions, number of links to the file, size, and block pointers 
 */

typedef struct inode{
    int iNum;         // inode number
    long size;          // data block size (bytes) | 0 = free | Around 4 GB
    mode_t fileMode;        // Permissions (Dont think we need to worry about this)
    time_t atime;         // inode access time
    time_t ctime;         // inode change time
    time_t mtime;         // inode modification time
    uid_t userID;         // user id
    gid_t groupID;        // group id
    unsigned int nLinks;
    unsigned int directBlockPtr [10];     // 10 direct pointers to the datablocks for the file (pointers are indexs of the block)
    unsigned int indirectBlockPtr;     // Single Doubly indirect block pointer
    unsigned int bitPos;
    unsigned int numBlocks;
    char padding[140];
} inode;

int writeInodeBitmap(int * bitmap, int blockNum);
int createInode(char * path, mode_t mode);
void getInode(unsigned int inodeNumber, inode * inodeData);
void createRoot(inode * inodeTable);
void fillStat(struct stat *statbuf, inode * inode);
int getInodeBlockIndex(unsigned int inodeIndex);
int getInodeIndex(unsigned int inodeIndex);
int removeInode(unsigned int inodeNum, mode_t mode);
int getParentInodeFP(char * filepath);
int getParentInode(inode * childInode);
void deleteAllInodeEntries(inode * _inode);

#endif