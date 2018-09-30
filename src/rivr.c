#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"

#define LOG_CRIT	0
#define LOG_ERR		1
#define LOG_WARN	2
#define LOG_STATUS	3
#define LOG_DEBUG	4

#ifdef DEBUG
int global_log_level = LOG_DEBUG;
#else
int global_log_level = LOG_ERR;
#endif

#define G_LOG_OUTPUT stdout

#define RIVR_LOG(LEVEL, ...) if (LEVEL <= global_log_level) fprintf(G_LOG_OUTPUT, __VA_ARGS__)

typedef enum Rivr_Command_{
	R_NONE,
	R_RUN, 
	R_LEX,
	R_PARSE
} Rivr_Command;

Rivr_Command parse_command(char* arg);

int rivr_run(char** argv, int argi, int argc);

extern void print_typed_token(const Typed_Token* tt);

int main(int argc, char** argv){
	int argi = 1;
	Rivr_Command command;
	if (argc > 1){
		command = parse_command(argv[1]);
		
		switch(command){
			case R_RUN:
				argi = rivr_run(argv, 1, argc);
				return 0;
				
			case R_LEX:
				RIVR_LOG(LOG_STATUS, "Lexing...\n");
				if (argc > 2){
					
					Typed_Token* tt = NULL;
					int n_tokens;
					FILE* f = fopen(argv[2], "r");
					
					if (f){
						tt = lex_file(f, &n_tokens);
						
						fclose(f);
					}else{
						RIVR_LOG(LOG_CRIT, "Unable to open %s for reading!\n", argv[2]);
					}
					
					RIVR_LOG(LOG_STATUS, "Parsed %d tokens\n", n_tokens);
					
					while(tt){
						
						print_typed_token(tt);
						
						tt = tt->next;
					}
					
					
				}else{
					fprintf(stderr, "No files to lex!\n");
				}
				return 0;
				
			case R_PARSE:
				RIVR_LOG(LOG_STATUS, "Parsing...\n");
				if (argc > 2){
					
					Typed_Token* tt = NULL;
					int n_tokens;
					FILE* f = fopen(argv[2], "r");
					
					if (f){
						tt = lex_file(f, &n_tokens);
						
						fclose(f);
					}else{
						RIVR_LOG(LOG_CRIT, "Unable to open %s for reading!\n", argv[2]);
					}
					
					RIVR_LOG(LOG_STATUS, "Parsed %d tokens\n", n_tokens);
					
					CFG_Symbol* symbols = convert_to_cfg_symbols(tt);
					
					RIVR_LOG(LOG_STATUS, "Converted tokens to cfg_symbols\n", n_tokens);
					
					while(symbols){
						
						print_cfg_symbol(symbols);
						symbols = symbols->next;
					}
				}
				return 0;
				
			case R_NONE:
			default:
				RIVR_LOG(LOG_STATUS, "Unknown command: '%s'\n", argv[1]);
				return 0;
		}
	}else{
		RIVR_LOG(LOG_STATUS, "No command specified\n");
	}
	
    return 0;
}

Rivr_Command parse_command(char* arg){
	const char cmd_run[4] = "run";
	const char cmd_lex[4] = "lex";
	const char cmd_parse[6] = "parse";
	
	if (strcmp(arg, cmd_run) == 0){
		return R_RUN;
		
	}else if (strcmp(arg, cmd_lex) == 0){
		return R_LEX;
		
	}else if (strcmp(arg, cmd_parse) == 0){
		return R_PARSE;
		
	}else{
		return R_NONE;
	}
}

int rivr_run(char** argv, int argi, int argc){
	
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
				RIVR_LOG(LOG_CRIT, "Error: unable to open file %s, aborting\n", argv[argc]);
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

		RIVR_LOG(LOG_STATUS, "launching thread\n");
	
		run_thread(thread);
	}else{
		RIVR_LOG(LOG_CRIT, "Error: no input file specificed\n");
	}
	
	
	
	return argi;
}