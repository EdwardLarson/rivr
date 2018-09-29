#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "lexer.h"

typedef enum Rivr_Command_{
	R_NONE,
	R_RUN, 
	R_PARSE
} Rivr_Command;

Rivr_Command parse_command(char* arg);

int rivr_run(char** argv, int argi, int argc);

int main(int argc, char** argv){
	int argi = 1;
	Rivr_Command command;
	while (argi < argc){
		command = parse_command(argv[argi]);
		switch(command){
			case R_RUN:
				argi = rivr_run(argv, argi, argc);
				break;
				
			case R_PARSE:
				break;
				
			case R_NONE:
				break;
		}
	}
	
	printf("Successful exit\n");
	
    return 0;
}

Rivr_Command parse_command(char* arg){
	const char cmd_run[4] = "run";
	const char cmd_parse[6] = "parse";
	
	if (strcmp(arg, cmd_run) == 0){
		return R_RUN;
		
	}else if (strcmp(arg, cmd_parse) == 0){
		return R_PARSE;
		
	}else{
		return R_NONE;
	}
}

int rivr_run(char** argv, int argi, int argc){
	#ifdef DEBUG
	printf("run command\n");
	#endif
	
	// argi points to 'run' command initially
	argi++;
	
	byte* mprog = NULL;
	PCType proglen = 0;
	
	while(argi < argc){
		if (argv[argi][0] == '-'){
			// argument
		}else{
			// filename
			FILE* fp = fopen(argv[argi], "r");
			
			if (!fp){
				printf("Error: unable to open file %s, aborting\n", argv[argc]);
				return argi;
			}
			
			fseek(fp, 0, SEEK_END);
			
			proglen = (PCType) ftell(fp);
			
			mprog = calloc(1, (size_t) proglen);
			
			fseek(fp, 0, SEEK_SET);
			
			fread(mprog, sizeof(char), (size_t) (proglen), fp);
			
			fclose(fp);
			
		}
		
		argi++;
	}
	
	if (mprog){
		Register_File* rfile = malloc(sizeof(Register_File));
		init_Register_File(rfile);
		
		Thread* thread = malloc(sizeof(Thread));
		init_Thread(thread, rfile, mprog, proglen, 0);
		
		#ifdef DEBUG
		printf("launching thread\n");
		#endif
	
		run_thread(thread);
	}else{
		printf("Error: no input file specificed\n");
	}
	
	
	
	return argi;
}