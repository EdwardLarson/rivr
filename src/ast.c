#include <malloc.h>
#include "ast.h"

void free_error_stack(AST_Error* err){
    if (!err) return;
    
    free_error_stack(err->next_err);
    free(err);
}

AST_Node* create_ast_node(Node_Type type, int n_children, Typed_Token_Data data){
	AST_Node* node = malloc(sizeof(AST_Node));
	
	node->type = type;
	node->children = malloc(n_children * sizeof(AST_Node*));
	node->data = data;
	
	return node;
}