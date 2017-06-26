Pranav Katkamwar (pranavk)
Andrew Khazanovich (akhaz)
Karl Xu (kx33)

----------------------------------------------------------------
----------------------------------------------------------------
Asst3 - A Simple File System in FUSE
----------------------------------------------------------------
----------------------------------------------------------------
Submission Details:

The programs were successfully tested on strategy.cs.rutgers.edu
The VM assigned to this group is classvm48.cs.rutgers.edu


Custom Files included:
	- bitmap.h
	- bitmap.c
	- dirent.h
	- dirent.c
	- inode.h
	- inode.c
	- stdefs.h

Original Files edited and included:
	- block.c
	- block.h
	- sfs.c

Makefile in /src/ edited to:
	Line 93:
	am_sfs_OBJECTS = sfs.$(OBJEXT) log.$(OBJEXT) block.$(OBJEXT) bitmap.$(OBJEXT) inode.$(OBJEXT) dirent.$(OBJEXT)

	Line 245:
	sfs_SOURCES = sfs.c  fuse.h  log.c	log.h  params.h  block.c  block.h bitmap.h bitmap.c inode.h inode.c dirent.h dirent.
----------------------------------------------------------------
----------------------------------------------------------------

Features Implemented:
	0. create
	1. delete
	2. stat
	3. open
	4. close
	5. read
	6. write
	7. readdir
	8. opendir
	9. closedir
	10. mkdir
	11. rmdir

----------------------------------------------------------------
----------------------------------------------------------------

The diskfile is organized like so:

	System Size:	((8 + 16) * 1024 * 1024)	=	25165824 Bytes
	Blocksize:		512 KB
	Total Blocks:	49152
	# Inode Blocks:	64
	Inodes/Blocks:	2
	# Data Blocks:	49082
	Free Space:		25129984 Bytes


	Block 0:		Boot Block
	Block 1:		Superblock
	Block 2:		Inode Bitmap
	Blocks 3-5:		Block Bitmap
	BLocks 6-69:	Inodes
	Blocks 70-end:	Data Blocks

----------------------------------------------------------------
----------------------------------------------------------------

Superblock Structure:
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

This tracks whether the filesystem needs to be initialized, and reports its status when accessed.



Inodes Structure:
typedef struct inode{
    int iNum;         		// inode number
    long size;          	// data block size (bytes) | 0 = free | Around 4 GB
    mode_t fileMode;        // Permissions (Dont think we need to worry about this)
    time_t atime;         	// inode access time
    time_t ctime;         	// inode change time
    time_t mtime;         	// inode modification time
    uid_t userID;         	// user id
    gid_t groupID;        	// group id
    unsigned int nLinks;	// Track the links to this file
    unsigned int directBlockPtr [10];    	// 10 direct pointers to the datablocks for the file (pointers are indexs of the block)
    unsigned int indirectBlockPtr;     		// Single Doubly indirect block pointer
    unsigned int bitPos;					// Used for bitmap operations
    unsigned int numBlocks;					// Number of blocks allocated to this inode
    char padding[140];						// Inodes are aligned to be divisible by BLOCK_SIZE
} inode;

The inode is accessed whenever sfs_getattr is called, which happens on almost every file system operation. The inode access time is changed.
The inode is created on calls to sfs_create and sfs_mkdir.
The inode has values changed in sfs_write and potentially sfs_mkdir.

Directory Entry Structure:
typedef struct __attribute__((packed, aligned(4)))dirEntry{
  unsigned int iNum;    // inode number
  unsigned char fileType;
  unsigned short dirLength; 
  char fileName[NAME_LEN];      // filename
} dirEntry;

The variable-length directory entries are stored in the datablocks of a DIR inode. They hold the file names associated with inodes, file type(file vs dir), and length of entry.
This structure is created on file or directory creation (sfs_mkdir, sfs_create).
This structure is accessed on calls to sfs_opendir, sfs_readdir, sfs_rmdir.
This structure is modified or deleted on calls to sfs_rmdir.

----------------------------------------------------------------
----------------------------------------------------------------

Implementations work as follows:

	- Bitmaps record whether inodes and datablocks are free or in use. They are used to find and track free blocks/inodes.
		int findFirstFree(int *bitmap)
			Purpose:
				Finds the first free bit in a given bitmap
			Return Value:
				Block index corresponding to the the first free bit found.


	- sfs_init
		The disk file is opened and its superblock read. If the superblock has not been initialized, the superblock is created and the disk organized as listed above. Else, the superblock is read from and logged.

	- sfs_create
		int createInode(char * path, mode_t mode)
			Purpose:
				Creates an inode, if possible. If the request is for a directory, its first data block is initialized for directory entries. Else, a directory entry is added to the parent directory.
			Return Value:
				0			Success
				-ENOENT		Unable to find free inode
				EFAULT		Inode found is not actually free

	- sfs_getttr
		Fills the statbuf with information in the requested inode.

	- sfs_open
		If a file exists and is able to be opened, 0 is returned.
		Else, ENOENT is returned.

	- sfs_read
		Reads the data blocks of an inode, one block at a time, and fills the buf until the request is fulfilled. Returns the number of bytes read successfully.

	- sfs_write
		Writes the requested bytes to the data blocks of an inode, filling one block at a time. The inode's size is adjusted and written to the disk as the request proceeds. Returns the number of bytes written successfully. If all data blocks are full, returns 

	- sfs_mkdir
		Creates an inode for a directory and initializes the first two directory entries (., ..)

	- sfs_rmdir
		Finds the parent inode, and recursively deletes all associated directory entries.

	- sfs_opendir
		Returns 0 if a directory is found at the given path. Else, returns ENOENT

	- sfs_readdir
		Calls filler to fill a buf with all directory entries of a valid directory path.
