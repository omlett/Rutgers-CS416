
#include "bitmap.h"

/* Helper file for creating bitmap */
//http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html

int findFirstFree(int * bitmap){
    int i = 0;
    int l = 1;
    unsigned int flag = 1;

    for(; i< (BLOCK_SIZE / sizeof(int)); i++){      // Go through every integer in bitmap
        for(; l < 32; l++){                         // Go through each bit in the integer;
            if(!((bitmap[i] >> l) & 1)){
                bitmap[i] |=  1 << l;               // Set bit and return index
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
