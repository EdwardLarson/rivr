#include "vm.h"
#include <stdio.h>

int main(int argc, char** argv){
	PCType proglen;
	
	Operation halt_op = encode_operation(I_HALT, SO_NONE);
	proglen = 2;
	
	FILE* fp;
	
	if (argc > 1){
		fp = fopen(argv[1], "w+");
		if (!fp){
			printf("Error: unable to open %s\n", argv[1]);
		}
	}else{
		fp = fopen("rivr_prog.b", "w");
		if (!fp){
			printf("Error: unable to open rivr_prog.b\n");
		}
	}
	
	fwrite(halt_op.bytes, sizeof(byte), (size_t) proglen, fp);
	
	fclose(fp);
}