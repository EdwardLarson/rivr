#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

void print_typed_token(const Typed_Token* tt){
	switch (tt->type){
		case T_IDENTIFIER:
			printf("INDENTIFIER: %s\n", tt->data.string);
			break;
		case T_KEYWORD:
			printf("KEYWORD: %d\n", tt->data.keyword);
			break;
		case T_FLAG:
			printf("FLAG: %d\n", tt->data.flag);
			break;
		case T_ATOM_DECLARE:
			printf("ATOM_DECLARE\n");
			break;
		case T_VAR_DECLAR:
			printf("VAR_DECLARE\n");
			break;
		case T_INTEGER:
			printf("INTEGER: %ld\n", tt->data.integer);
			break;
		case T_RATIONAL:
			printf("RATIONAL: %f\n", tt->data.rational);
			break;
		case T_STRING:
			printf("STRING: %s\n", tt->data.string);
			break;
		case T_FUNC_RETURN:
			printf("FUNC_RETURN\n");
			break;
		case T_OPERATOR:
			printf("OPERATOR: %d\n", tt->data.oper);
			break;
		case T_ASSIGNMENT:
			printf("ASSIGNMENT\n");
			break;
		case T_COMMA:
			printf("COMMA\n");
			break;
		case T_PERIOD:
			printf("PERIOD\n");
			break;
		case T_COLON:
			printf("COLON\n");
			break;
		case T_OPEN_PARA:
			printf("OPEN_PARA\n");
			break;
		case T_CLOSE_PARA:
			printf("CLOSE_PARA\n");
			break;
		case T_OPEN_BRACE:
			printf("OPEN_BRACE\n");
			break;
		case T_CLOSE_BRACE:
			printf("CLOSE_BRACE\n");
			break;
		case T_OPEN_BRACK:
			printf("OPEN_BRACK\n");
			break;
		case T_CLOSE_BRACK:
			printf("CLOSE_BRACK\n");
			break;
		case T_END_STATEMENT:
			printf("END_STATEMENT\n");
			break;
		case T_ENTER_BLOCK:
			printf("ENTER_BLOCK (%d)\n", tt->data.indent);
			break;
		case T_EXIT_BLOCK:
			printf("EXIT_BLOCK (%d)\n", tt->data.indent);
			break;
	}
}

void free_typed_token(Typed_Token* tt){
	if (!tt) return;
	
	if(tt->type == T_IDENTIFIER || tt->type == T_STRING){
		free(tt->data.string);
	}
	
	free_typed_token(tt->next);
	
	free(tt);
}

#ifdef PARSER_ONLY
int main(int argc, char** argv){
	printf("Hello World!\n");
	
	if (argc > 1){
		FILE* fp = fopen(argv[1], "r");
		if (fp){
			Token* current = next_token(fp);
			Token* list_start = current;
			
			while (current){
				current->next = next_token(fp);
				current = current->next;
			}
			
			printf("Done parsing %s:\n", argv[1]);
			/*
			current = list_start;
			int n_tokens = 0;
			
			while(current){
				printf("TOKEN:\t%s\n", current->token);
				current = current->next;
				++n_tokens;
			}
			
			printf("Finished printing %d tokens, now removing comment tokens:\n", n_tokens);*/
			
			//list_start = remove_comments(list_start);
			
			current = list_start;
			int n_ncomm_tokens = 0;
			
			while(current){
				printf("TOKEN:\t%s\n", current->token);
				current = current->next;
				++n_ncomm_tokens;
			}
			
			printf("Finished printing %d non-comment tokens\n", n_ncomm_tokens);
			
			
			getchar();
			
			Typed_Token* tt = convert_to_proto(list_start, 0);
			Typed_Token* initial_tt = tt;
			
			printf("Done converting to typed tokens (function could use a name change though)\n");
			
			while(tt){
				print_typed_token(tt);
				tt = tt->next;
			}
			
			free_typed_token(initial_tt);
			
		}else{
			printf("\tError: Unable to open %s for reading\n", argv[1]);
		}
	}
	
	return 0;
}
#endif

// parser procedure:
// parse a token
// add token to stack
// check stack for a token pattern match

void parse_file(FILE* fp, AST_Node* root, Parse_Error* errors){
	AST_Node* current_node = malloc(sizeof(AST_Node));
	
	current_node->type = DECLARATION_BLOCK;
	current_node->children = NULL;
	current_node->n_children = 0;
	current_node->data = NULL;
	current_node->flags = NULL;
	
	char line_buffer[LINE_BUFFER_S];
	
	while(fgets(line_buffer, LINE_BUFFER_S, fp)){
		
	}
}

/*
Typed_Token* get_next_typed_token(FILE* fp){
	// get next token
	
	// convert it to a 
}
*/

Token* create_token(const char* buffer){
	Token* token = malloc(sizeof(Token));
	token->token = malloc(sizeof(char) * (strlen(buffer) + 1));
	strcpy(token->token, buffer);
	
	return token;
}

Token* finish_token(char* buffer, int i, char c, FILE* fp){
	ungetc(c, fp);
	buffer[i] = '\0';
	return create_token(buffer);
}

Token* next_token(FILE* fp){
	
	/* we can know the end condition of a token by what the initial char is:
	| INITIAL CHAR		|END CONDITION										|TOKEN TYPE
	| ==================|===================================================|=========================
	| alphabetical		| non-alphanumeric, whitespace						| identifier, keyword
	| :					| anything but '=' or ':'							| declarator
	| digit				| anything but a digit or '.'						| number
	| /					| anything but '/', '*', '='						| operator
	| +					| anything but '=' or '+'							| operator
	| -					| anything but '=','-','>'							| operator, function_return
	| *					| anything but '=', '*', '/'						| operator
	| %					| anything but '='									| operator
	| "					| '"' (skipping any character preceded by '\')		| string
	| >					| anything but '>', '='								| operator
	| <					| anything but '<', '='								| operator
	| ^					| anything but '='									| operator
	| !					| anything but '='									| operator
	| &					| anything but '='									| operator
	| |					| anything but '='									| operator
	| =					| anything but '='									| assignment, operator
	| .					| immediately return								| period
	| ,					| immediately return								| comma
	| @					| non-alphanumeric									| flag
	| (					| immediately return								| open_expr
	| )					| immediately return								| close_expr
	| [					| immediately return								| open_arr
	| ]					| immediately return								| close_arr
	| {					| immediately return								| open_comp
	| }					| immediately return								| close_comp
	| \n				| non-whitespace									| end_statement, close_block, enter_block (depending on number of whitespaces relative to previous line)
	
	*/
	
	char buffer[1024];
	int i = 1;
	
	buffer[0] = getc(fp);
	
	if (buffer[0] == EOF){
		return NULL;
	}
	
	//advance past all non-newline whitespace
	while(isspace(buffer[0]) && buffer[0] != '\n'){
		buffer[0] = getc(fp);
	}
	
	char initial_char, c;
	
	if (isalpha(buffer[0])){
		initial_char = 'a'; // stand-in for any alphabetical character
		
	}else if (isdigit(buffer[0])){
		initial_char = 'd'; // stand-in for any digit
		
	}else{
		// take care of all immediate return characters at once
		switch(buffer[0]){
			case '.':
			case ',':
			case '(':
			case ')':
			case '[':
			case ']':
			case '{':
			case '}':
				buffer[1] = '\0';
				return create_token(buffer);
			default:
				initial_char = buffer[0];
		}
	}
	
	
	for (c = getc(fp); c != EOF; c = getc(fp)){
		switch(initial_char){
			case 'a':
				if (!(isalnum(c) || c == '_')){
					return finish_token(buffer, i, c, fp);
				}else{
					buffer[i] = c;
					i++;
				}
				break;
			case 'd':
				if (isdigit(c) || c == '.'){
					buffer[i] = c;
					i++;
				}else{
					return finish_token(buffer, i, c, fp);
				}
				break;
			case ':':
				if (c == ':' || c == '='){
					buffer[i] = c;
					i++;
				}else{
					return finish_token(buffer, i, c, fp);
				}
				break;
			case '/':
				if (i == 1){
					if (c == '/'){
						// skip to next newline and then recursively call next_token to skip the comment line
						while (c != '\n' && c != EOF) c = getc(fp);
						
						return next_token(fp);
					}else if(c == '*'){
						// skip to next "*/", and make recursive call
						while (c != EOF){
							c = getc(fp);
							if (c == '*' && getc(fp) == '/') return next_token(fp);
						}
						
						return NULL;
					}
				}
				
				if (c == '='){
					buffer[i] = c;
					i++;
				}else{
					return finish_token(buffer, i, c, fp);
				}
				break;
			case '+':
				if (c == '=' || c == '+'){
					buffer[i] = c;
					i++;
				}else{
					return finish_token(buffer, i, c, fp);
				}
				break;
			case '-':
				if (c == '=' || c == '-' || c == '>'){
					buffer[i] = c;
					i++;
				}else{
					return finish_token(buffer, i, c, fp);
				}
				break;
			case '*':
				if (c == '*' || c == '=' || c == '/'){
					buffer[i] = c;
					i++;
				}else{
					return finish_token(buffer, i, c, fp);
				}
				break;
			case '>':
			case '<':
				// c must be same as initial char or '='
				if (c == initial_char || c == '='){
					buffer[i] = c;
					i++;
				}else{
					return finish_token(buffer, i, c, fp);
				}
				break;
			case '^':
			case '!':
			case '&':
			case '|':
			case '%':
				// c must be '='
				if (c == '='){
					buffer[i] = c;
					i++;
				}else{
					return finish_token(buffer, i, c, fp);
				}
				break;
			case '"':
				if (c == '\\'){
					// skip to next character, handling it directly
					c = getc(fp);
					if (c != EOF){
						buffer[i] = c;
						i++;
					}else{
						printf("I knew this day would come... just Ctrl-F me, I'm too tried for proper error handling\n");
					}
				}else if (c == '"'){
					buffer[i] = '"';
					buffer[i+1] = '\0';
					return create_token(buffer);
				}else{
					buffer[i] = c;
					i++;
				}
				break;
			case '@':
				if (isalnum(c) || c == '.'){
					buffer[i] = c;
					i++;
				}else{
					return finish_token(buffer, i, c, fp);
				}
				break;
			case '\n':
				if (isspace(c) && c != '\n'){
					buffer[i] = c;
					i++;
				}else{
					return finish_token(buffer, i, c, fp);
				}
				break;
			default:
				// ERROR: Initial char should really be one of the above
				//printf("\tError: Unhandled initial char in token parser\n");
				if (isspace(c)){
					return finish_token(buffer, i, c, fp);
				}else{
					buffer[i] = c;
					i++;
				}
				break;
		}
	}
	
	buffer[i] = '\0';
	return create_token(buffer);
}

int is_comment_close(Token* t){
	return (strlen(t->token) > 1) && (t->token[0] == '*') && (t->token[1] == '/');
}

int is_newline(Token* t){
	return t->token[0] == '\n';
}

Typed_Token* convert_to_proto(Token* t, int prev_indent){
	if (!t){
		return NULL;
	}
	
	Typed_Token* typed_token = malloc(sizeof(Typed_Token));
	typed_token->next = NULL;
	typed_token->data.indent = prev_indent;
	
	//printf("\t1. data->indent is currently %d\n", typed_token->data.indent);
	
	typedef int (*Typed_Token_Generator)(const char*, Token_Type*, Typed_Token_Data*);
	Typed_Token_Generator const generators[10] = {
		is_structural,
		is_assignment,
		is_func_return,
		is_declarator,
		is_number,
		is_string,
		is_flag,
		is_keyword,
		is_operator,
		is_valid_identifier
	};
	
	// iterate over all token generating functions until one of them is successful
	int i;
	Typed_Token_Generator token_generator;
	for (i = 0; i < 10; i++){
		token_generator = generators[i];
		
		if (token_generator(t->token, &typed_token->type, &typed_token->data)){
			free(t->token); // freed ASAP because strings could take up a lot of space
			
			if (i == 0){
				// structural blocks require special handling, because they may be enter or exit blocks which generate more tokens
				
				if (typed_token->data.indent > prev_indent){
					// subtract INDENT_PER_BLOCK from data.indent until it matches previous indent
					// create an ENTER_BLOCK token for every subtraction
					
					if (typed_token->data.indent - prev_indent > INDENT_PER_BLOCK){
						printf("Parse error: Unexpected indentation\n");
						// TO-DO: Error handling
					}
					
					typed_token->next = convert_to_proto(t->next, typed_token->data.indent);
					
				}else if (typed_token->data.indent < prev_indent){
					// add INDENT_PER_BLOCK from data.indent until it matches previous indent
					// create an EXIT_BLOCK for every addition
					
					int tmp_indent;
					Typed_Token* curr_structure_token = typed_token;
					Typed_Token* original_exit_token = typed_token;
					
					for (tmp_indent = typed_token->data.indent + INDENT_PER_BLOCK; tmp_indent < prev_indent; tmp_indent += INDENT_PER_BLOCK){
						
						// insert new token before previously generated EXIT_BLOCK token
						typed_token = malloc(sizeof(Typed_Token));
						typed_token->next = curr_structure_token;
						typed_token->type = T_EXIT_BLOCK;
						typed_token->data.indent = tmp_indent;
						curr_structure_token = typed_token;
					}
					
					if (tmp_indent > prev_indent){
						printf("Parse error: bad indent when exiting block\n");
						// TO-DO: Error handling
					}
					
					
					
					original_exit_token->next = convert_to_proto(t->next, original_exit_token->data.indent);
				}else{
					typed_token->next = convert_to_proto(t->next, typed_token->data.indent);
				}
			}else{
				typed_token->next = convert_to_proto(t->next, prev_indent);
			}
			free(t);
			
			return typed_token;
		}
	}
	
	printf("Parse error: Unknown or invalid token %s\n", t->token);
	free(t->token);
	free(typed_token);
	Token* next = t->next;
	free(t);
	
	return convert_to_proto(next, prev_indent);
}

int is_keyword(const char* token, Token_Type* type, Typed_Token_Data* data){
	int i;
	for (i = 0; i < N_KEYWORDS; i++){
		if (!strcmp(token, keywords[i])){
			*type = T_KEYWORD;
			data->keyword = (KEYWORD) i;
			return 1;
		}
	}
	
	return 0;
}

int is_operator(const char* token, Token_Type* type, Typed_Token_Data* data){
	int i;
	for (i = 0; i < N_OPERATORS; i++){
		if (!strcmp(token, operators[i])){
			*type = T_OPERATOR;
			data->oper = (OPERATOR) i;
			return 1;
		}
	}
	
	return 0;
}

int is_structural(const char* token, Token_Type* type, Typed_Token_Data* data){
	//printf("\t2. data->indent is currently %d\n", data->indent);
	
	if (strlen(token) == 1 && token[0] != '\n'){
		switch(token[0]){
			case '.':
				*type = T_PERIOD;
				return 1;
			case ',':
				*type = T_COMMA;
				return 1;
			case '(':
				*type = T_OPEN_PARA;
				return 1;
			case ')':
				*type = T_CLOSE_PARA;
				return 1;
			case '[':
				*type = T_OPEN_BRACK;
				return 1;
			case ']':
				*type = T_CLOSE_BRACK;
				return 1;
			case '{':
				*type = T_OPEN_BRACE;
				return 1;
			case '}':
				*type = T_CLOSE_BRACE;
				return 1;
			case ':':
				*type = T_COLON;
				return 1;
			default:
				return 0;
		}
	}else if (token[0] == '\n'){
		// count whitespace after the newline
		int new_indent = strlen(token) - 1;
		// count tabs as multiple indents
		int i;
		for (i = 0; i < strlen(token); i++) 
			if (token[i] == '\t') 
				new_indent += INDENT_PER_BLOCK - 1;
		
		if (new_indent == data->indent){
			*type = T_END_STATEMENT;
		}else if (new_indent < data->indent){
			
			*type = T_EXIT_BLOCK; // TO_DO: enough exit_block tokens should be created to match the amount the indent changed
			data->indent = new_indent;
		}else{
			*type = T_ENTER_BLOCK;
			data->indent = new_indent;
		}
		
		return 1;
	}else{
		return 0;
	}
}

int is_assignment(const char* token, Token_Type* type, Typed_Token_Data* data){
	if (strlen(token) == 1 && token[0] == '='){
		*type = T_ASSIGNMENT;
		return 1;
	}else{
		return 0;
	}
}

int is_declarator(const char* token, Token_Type* type, Typed_Token_Data* data){
	if (strlen(token) == 2 && token[0] == ':'){
		if (token[1] == ':'){
			*type = T_ATOM_DECLARE;
			return 1;
		}else if (token[1] == '='){
			*type = T_VAR_DECLAR;
			return 1;
		}else{
			return 0;
		}
	}else{
		return 0;
	}
}

int is_number(const char* token, Token_Type* type, Typed_Token_Data* data){
	int is_rat = 0;
	int len = strlen(token);
	int i;
	for (i = 0; i < len; i++){
		if (token[i] == '.'){
			if (is_rat) return 0;
			
			is_rat = 1;
		}else if (!isdigit(token[i])){
			return 0;
		}
	}
	
	if (is_rat){
		*type = T_RATIONAL;
		data->rational = atof(token);
		return 1;
	}else{
		*type = T_INTEGER;
		data->integer = atol(token);
		return 1;
	}
}

int is_string(const char* token, Token_Type* type, Typed_Token_Data* data){
	int len = strlen(token);
	
	if (len > 1 && token[0] == '"' && token[len - 1] == '"'){
		
		// copy old token to a new string, without the " at the beginning and end
		// still need an null terminator, so new string should be size (len - 2 excluded "s) + 1 null terminator
		data->string = malloc(sizeof(char) * (len - 1));
		strncpy(data->string, &token[1], len - 2);
		data->string[len - 2] = '\0';
		
		*type = T_STRING;
		return 1;
	}else{
		return 0;
	}
}

int is_func_return(const char* token, Token_Type* type, Typed_Token_Data* data){
	if (!strcmp(token, "->")){
		*type = T_FUNC_RETURN;
		return 1;
	}else{
		return 0;
	}
}

int is_flag(const char* token, Token_Type* type, Typed_Token_Data* data){
	int i;
	for (i = 0; i < N_FLAGS; i++){
		if (!strcmp(token, flags[i])){
			*type = T_FLAG;
			data->flag = (FLAG) i;
			return 1;
		}
	}
	
	return 0;
}

int is_valid_identifier(const char* token, Token_Type* type, Typed_Token_Data* data){
	int len = strlen(token);
	if (len >= 1 && isalpha(token[0])){
		*type = T_IDENTIFIER;
		data->string = malloc(sizeof(char) * (len + 1));
		strcpy(data->string, token);
		data->string[len] = '\0';
		
		return 1;
	}else{
		return 0;
	}
}