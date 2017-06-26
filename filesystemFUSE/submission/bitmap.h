
#ifndef _BITMAP_H_
#define _BITMAP_H_

#include "stdefs.h"

/* Bitmap: one bit for each block on the disk
 * Good to find a contiguous group of free blocks
 * Files are often accessed sequentially
 * For 1TB disk and 4KB blocks, 32MB for the bitmap
 * Chained free portions: pointer to the next one
 * Not so good for sequential access (hard to find sequential
 * blocks of appropriate size)
 * Index: treats free space as a file
 */

/* Bitmap methods for Setting Bits
 * Find free bit index  and clearing bits
 */

int testBit(int * bitmap, int bitK);
int findFirstFree(int * bitmap);
void clearBit(int * bitmap, int bitK);
void setBit(int * bitmap, int bitK);

#endif