
typedef enum {
	EXPRESSION_BLOCK, // multiple expressions in a row: has any number of children >1
	
	EXPRESSION,
	
	CONSTANT, // has a value string, no children
	
	ATOM, // immutable atom: has a name, no children
	
	VARIABLE, // variable: has a name, no children
	
	U_OPERATION, // operation applied to only one expression: has a operation, one child
	
	B_OPERATION, // operation applied to two expressions: has an operation, two children
	
	FUNCTION_CALL, // has 
	
	FOLLOW, // follow a nested attribute (as in a period): has two children, before and after the period
	
	ASSIGNMENT, // assign a value to a variable: has two children, the variable and value
	
	BRANCH, // if-then-else: has 2 or 3 children depending on if the branch has an else
	
	TRYCATCH, // has 1-3 children, depending on if the try has a catch and then finally blocks
	
	LOOP, // has 4 children, for the initial, conditional, increment, and body expressions
	
	RETURN, // has up to 32 children (for 32 return values)
} Node_Type;


typedef enum {
	PUBLIC,
	OVERRIDE
} Rivr_Flag;

struct _AST_Node;

typedef struct _AST_Node {
	Node_Type type;
	
	struct _AST_Node* children; // array of children
	int n_children;
	
	char* data; // a string of data; could be a constant, or identifier, or operator, etc.
	
	Rivr_Flag* flags; // array of compiler flags
	int n_flags;
	
} AST_Node;