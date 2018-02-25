#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "parser.h"

int main(int argc, char** argv){
    printf("rivr is running\n");
	
	Register_File rfile;
	init_Register_File(&rfile);
	
	Operation halt_op = encode_operation(I_HALT, 0x0);
	
	const byte* mprog = (const byte*) &(halt_op.bytes);
	
	Thread thread;
	init_Thread(&thread, &rfile, mprog, 2, 0);
	
	run_thread(&thread);
	
	printf("Successful exit\n");
	
    return 0;
}