/*
 * SIZE OF TESTER FOR CALCULATIONS
 */

 #include "stdefs.h"
 //#include <bitmap.h>

 int main(int argc, char **argv){
 	printf("Size of SBlock: %i\n", sizeof(sblock));
 	printf("Size of Inode: %i\n", sizeof(inode));
 	printf("Inodes per Block: %i\n", (4096)/sizeof(inode));
 	printf("Number of Blocks in Filesystem: %i\n", (TOTAL_FS_SIZE / BLOCK_SIZE));
 	printf("(Blocksize: %i * 64 Blocks)/ sizeof(inode) %i = %i\n", BLOCK_SIZE, sizeof(inode), (BLOCK_SIZE * 64)/ sizeof(inode));
 	printf("Sizeof InodeBitmap: %i\n", (BLOCK_SIZE * 64)/ sizeof(inode)/32);
 	return 0;
 }