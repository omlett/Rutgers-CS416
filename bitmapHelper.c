/* Helper file for creating bitmap */
//http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html


// Combined set and clear. If bit given in K, cleark that bit. Else set that bit
void setandClearBit(int * bitmap, int bitK){
	
	int i = abs(k/32);
	int pos = (k%32);
	unsigned int flag = 1;

	if(k < 0){
		flag = ~flag;
		bitmap[i] = bitmap[i] & flag;
	}
	else{
		i *= -1;
		bitmap[i] | | flag;
	}
	
}

int testBit(int * bitmap, int bitK){
	  int i = k/32;
      int pos = k%32;

      unsigned int flag = 1;  /

      flag = flag << pos;     

      if ( bitmap[i] & flag ){
      	return 1;
      }     
         
      else{
      	return 0;
      }

}

