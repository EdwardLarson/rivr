#ifndef LEXER_H
#define LEXER_H

#define LINE_BUFFER_S 1024

#include <stdio.h>
#include "keywords.h"

#define INDENT_PER_BLOCK 4

struct _Token;
typedef struct _Token{
	char* token;
	int pos;
	
	struct _Token* next;
} Token;

struct _Compiler_Flag;
typedef struct _Compiler_Flag{
	enum {
		PUBLIC,
		OVERRIDE
	} flag;
	
	struct _Compiler_Flag* next_flag;
} Compiler_Flag;

struct _Parse_Error;
typedef struct _Parse_Error{
	int error_code;
	int line;
	int col;
	
	struct _Parse_Error* next_err;
} Parse_Error;

typedef enum {
	T_IDENTIFIER, // string data
	
	T_KEYWORD, // string data
	T_FLAG, // string data
	
	T_VAR_DECLARE, // no data
	T_VAR_DEFINE, // no data
	
	T_INTEGER, // long int
	T_RATIONAL, // double
	T_STRING, // string data
	
	T_FUNC_RETURN, // no data necessary
	T_OPERATOR, // enum for what type of operator this is, maybe a flag for if it is also an assignment?
	T_ASSIGNMENT, // no data necessary
	
// structure tokens
	T_COMMA, // no data
	T_PERIOD, // no data
	T_COLON,
	
	T_OPEN_PARA, // no data
	T_CLOSE_PARA, // no data
	T_OPEN_BRACE, // no data
	T_CLOSE_BRACE, // no data
	T_OPEN_BRACK, // no data
	T_CLOSE_BRACK, // no data
	
	T_END_STATEMENT, // no data
	T_ENTER_BLOCK, // no data
	T_EXIT_BLOCK // no data
} Token_Type;

typedef union{
	KEYWORD keyword;
	OPERATOR oper;
	FLAG flag;
	long int integer;
	double rational;
	char* string;
	
	int indent;
} Typed_Token_Data;

struct _Typed_Token;
typedef struct _Typed_Token{
	Token_Type type;
	Typed_Token_Data data;
	int pos;
	
	struct _Typed_Token* next;
	
} Typed_Token;

typedef struct Line_Vector_{
	int* array;
	int size;
	int n;
} Line_Vector;


Token* next_token(FILE* fp);
Token* finish_token(char* buffer, int i, char c, FILE* fp);
Token* create_token(const char* buffer, int pos_end);
Token* remove_comments(Token* list);
Typed_Token* convert_to_proto(Token* t, int prev_indent);

Typed_Token* generate_exit_blocks(Token* t, Typed_Token* typed_token, int prev_indent);

Line_Vector get_line_numbers(FILE* fp);
void add_line(Line_Vector* lv, int lineno);


int is_keyword(const char* token, Token_Type* type, Typed_Token_Data* data);
int is_operator(const char* token, Token_Type* type, Typed_Token_Data* data);
int is_structural(const char* token, Token_Type* type, Typed_Token_Data* data);
int is_assignment(const char* token, Token_Type* type, Typed_Token_Data* data);
int is_declarator(const char* token, Token_Type* type, Typed_Token_Data* data);
int is_number(const char* token, Token_Type* type, Typed_Token_Data* data);
int is_string(const char* token, Token_Type* type, Typed_Token_Data* data);
int is_func_return(const char* token, Token_Type* type, Typed_Token_Data* data);
int is_flag(const char* token, Token_Type* type, Typed_Token_Data* data);
int is_valid_identifier(const char* token, Token_Type* type, Typed_Token_Data* data);

#endif