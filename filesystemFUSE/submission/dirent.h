#ifndef _DIRENT_H_
#define _DIRENT_H_

#include "stdefs.h"

#define NAME_LEN 255



typedef struct __attribute__((packed, aligned(4)))dirEntry{
  unsigned int iNum;    // inode number
  unsigned char fileType;
  unsigned short dirLength; 
  char fileName[NAME_LEN];      // filename
} dirEntry;




void createDEntry(unsigned int inodeNumber, char * filename, unsigned int parentInodeNumber, mode_t mode);
void initializeDEntries(unsigned int dataBlockIndex, unsigned int inodeNum, int parentInodeNumber);

#endif