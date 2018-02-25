#include "parser.h"

typedef enum {
	N_ABSTRACTION_BLOCK, // multiple abstractions in a row, to be executed sequentially: has any number of children
	N_DECLARATION_BLOCK, // multiple declrations in a row, no execution scheme: has any number of children
	
	//ABSTRACTIONS
	    N_DECLARATION, // has two children, for the identifier (either a variable or atom) and value (expression)
	
	//--CONTROL
	      N_BRANCH, // if-then-else: has 2 or 3 children depending on if the branch has an else
	      N_TRYCATCH, // has 1-3 children, depending on if the try has a catch and then finally blocks
	      N_LOOP, // has 4 children, for the initial, conditional, increment, and body expressions
	//----JUMPS
	        N_RETURN, // has up to 32 children (for 32 return values)
	        N_BREAK,
	        N_CONTINUE,
	//--EXPRESSIONS
	//----HARDCODED
	        N_CONSTANT, // has a value string, no children
	        N_FUNCTION,
	//------TYPE
	          N_PRIMITIVE,
	          N_COMPOSITION,
	//----IDENTIFIERS
	        N_ATOM, // immutable atom: has a name, no children
	        N_VARIABLE, // variable: has a name, no children
	//----OPERATIONS
	        N_ASSIGN, // assign a value to a variable: has two children, the variable and value (an expression)
	        N_CALL, // call a function
	//------INSTRUCTIONS
	          N_U_OPERATION, // operation applied to only one expression: has a operation, one child
	          N_B_OPERATION  // operation applied to two expressions: has an operation, two children
	
} Node_Type;

struct _AST_Node;
typedef struct _AST_Node {
	Node_Type type;
	
	struct _AST_Node* children; // array of children
	int n_children;
	
	void* data; // a string of data; could be a constant, or identifier, or operator, etc.
	
	Compiler_Flag* flags; // list of compiler flags
	
} AST_Node;

struct _AST_Error;
typedef struct _AST_Error {
    int errort;
    
    struct _AST_Error* next_err;
} AST_Error;

void free_error_stack(AST_Error* err);

AST_Node* tree_DECLARATION_BLOCK(const Token* token, Token** next_out, AST_Error* err_stack);

AST_Node* tree_DECLARATION(const Token* token, Token** next_out, AST_Error* err_stack);