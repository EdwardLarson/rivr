#include "lexer.h"

typedef enum {
	N_STATEMENT_BLOCK, // multiple abstractions in a row, to be executed sequentially: has any number of children
	N_DECLARATION_BLOCK, // multiple declrations in a row, no execution scheme: has any number of children
	
	N_TYPELIST, // list of types: has n children
	
	
//IMPERATIVES
//----DECLARATIONS
	N_TDECLARE, // variable type declare: has two children, for the identifier and type
	N_VDECLARE, // variable declare: has two children, for the identifier and value expression
	N_CDECLARE, // constant declare: has two children, for the identifier and value expression
	
	N_COMPOSE, // class definition/composition: has 3 children for name composition, 'inheritance' composition typelist, and body declaration block
	
//----STATEMENTS
//--------CONTROL
	    N_IF, // if-then: has 2 children for conditional expression and body expression block
		N_IFELSE, // if-then-else: has 3 children for conditional expression and body expression blocks
	    N_TRYCATCH, // try, n>0 catches: has n+1 children  for 
		N_TRYCATCHFINALLY, // try, n>0 catches, finally: has n+2 children for try expression, catch expression blocks, and 
//------------LOOPS
			N_WHILE, // has 2 children, for the conditional expression and body expression block
			N_FOR, // has 4 children, for the initial, conditional, and increment expressions and body expression block
			N_FOREACH, // has 3 children, for the identifier loop variable, iterable expression, and body expression block
//------------JUMPS
	        N_RETURN, // has up to 32 children (for 32 return values)
	        N_BREAK,
	        N_CONTINUE,
//--------EXPRESSIONS
//------------RVALUES
//----------------CONSTANTS
				N_INTEGER, // has a int value, no children
				N_RATIONAL, // has a double value, no children
				N_STRING, // has a string value, no children
				N_FUNCTION, // has 3 children, for arguments, returns, and body expression block
				N_BOOLEAN,
//----------------OPERATIONS
				N_ASSIGN, // assign a value to a variable: has two children, the variable left value and value right value
				N_CALL, // call a function: 1+n children for function right value and a number of argument right values
				N_U_OPERATION, // operation applied to only one expression: has a operation, one child
				N_B_OPERATION  // operation applied to two expressions: has an operation, two children
	
//----------------LVALUES
//--------------------IDENTIFIERS
					N_VARIABLE, // variable: has a name, no children
//TYPE
	N_PRIMITIVE,
	N_COMPOSITION,
	
	N_NONE
	
} Node_Type;

struct _AST_Node;
typedef struct _AST_Node {
	Node_Type type;
	
	struct _AST_Node** children; // array of children
	int n_children;
	
	//void* data; // a string of data; could be a constant, or identifier, or operator, etc.
	Typed_Token_Data data;
	
	Compiler_Flag* flags; // list of compiler flags
	
} AST_Node;

struct _AST_Error;
typedef struct _AST_Error {
    int errort;
    
    struct _AST_Error* next_err;
} AST_Error;

void free_error_stack(AST_Error* err);

AST_Node* create_ast_node(Node_Type type, int n_children, void* data);

AST_Node* tree_DECLARATION_BLOCK(const Token* token, Token** next_out, AST_Error* err_stack);

AST_Node* tree_DECLARATION(const Token* token, Token** next_out, AST_Error* err_stack);