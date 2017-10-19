#define LINE_BUFFER_S 1024
#include "keywords.h"

#define INDENT_PER_BLOCK 4

typedef enum {
	ABSTRACTION_BLOCK, // multiple abstractions in a row, to be executed sequentially: has any number of children
	DECLARATION_BLOCK, // multiple declrations in a row, no execution scheme: has any number of children
	
	//ABSTRACTIONS
	    DECLARATION, // has two children, for the identifier (either a variable or atom) and value (expression)
	
	//--CONTROL
	      BRANCH, // if-then-else: has 2 or 3 children depending on if the branch has an else
	      TRYCATCH, // has 1-3 children, depending on if the try has a catch and then finally blocks
	      LOOP, // has 4 children, for the initial, conditional, increment, and body expressions
	//----JUMPS
	        RETURN, // has up to 32 children (for 32 return values)
	        BREAK,
	        CONTINUE,
	//--EXPRESSIONS
	//----HARDCODED
	        CONSTANT, // has a value string, no children
	        FUNCTION,
	//------TYPE
	          PRIMITIVE,
	          COMPOSITION,
	//----IDENTIFIERS
	        ATOM, // immutable atom: has a name, no children
	        VARIABLE, // variable: has a name, no children
	//----OPERATIONS
	        ASSIGN, // assign a value to a variable: has two children, the variable and value (an expression)
	        CALL, // call a function
	//------INSTRUCTIONS
	          U_OPERATION, // operation applied to only one expression: has a operation, one child
	          B_OPERATION  // operation applied to two expressions: has an operation, two children
	
} Node_Type;

struct _Token;
typedef struct _Token{
	char* token;
	
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

struct _AST_Node;
typedef struct _AST_Node {
	Node_Type type;
	
	struct _AST_Node* children; // array of children
	int n_children;
	
	char* data; // a string of data; could be a constant, or identifier, or operator, etc.
	
	Compiler_Flag* flags; // list of compiler flags
	
} AST_Node;

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
	
	T_ATOM_DECLARE, // no data
	T_VAR_DECLAR, // no data
	
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
	struct _Typed_Token* next;
	
} Typed_Token;


Token* next_token(FILE* fp);
Token* finish_token(char* buffer, int i, char c, FILE* fp);
Token* create_token(const char* buffer);
Token* remove_comments(Token* list);
Typed_Token* convert_to_proto(Token* t, int prev_indent);
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