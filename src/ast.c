#include "ast.h"


#define VECTOR_INIT_SIZE

void free_error_stack(AST_Error* err){
    if (!err) return;
    
    free_error_stack(err->next_err);
    free(err);
}

AST_Node* create_ast_node(Node_Type type, int n_children, void* data){
	AST_Node* node = malloc(sizeof(AST_Node));
	
	node->type = type;
	node->children = malloc(n_children * sizeof(AST_Node*));
	
	return node;
}

// parse a declaration block from a stream of tokens
AST_Node* tree_DECLARATION_BLOCK(const Typed_Token* token, Typed_Token** next_out, AST_Error* err_stack){
    AST_Node* block = malloc(sizeof(AST_Node));
    block->type = N_DECLARATION_BLOCK;
    block->n_children = 0;
    block->flags = 0;
    
    int block_children_size = VECTOR_INIT_SIZE;
    block->children = malloc(sizeof(AST_Node*) * block_children_size);
    
    /*
    [DECLARATION
     ...
     DECLARATION]
    [DECLARATION, ... , DECLARATION]
    */
    
    Typed_Token* next;
    const Typed_Token* curr = token;
    
    do{
        block->children[block->n_children] = tree_DECLARATION(curr, &next);
        
        // skip comma tokens in the case of a declaration block in a function definition
        if (next->type == T_COMMA){
            next = next->next;
        }
        
        curr = next;
        
        block->n_children++;
        // check if need to resize vector of children
        if (block->n_children >= block_children_size){
            block_children_size *= 2;
            block->children = realloc(block->children, sizeof(AST_Node) * block_children_size);
        }
        
    }while (next != NULL && !(next->type == T_CLOSE_PARA || next->type == T_EXIT_BLOCK));
    
    // after all declrations built, realloc to exactly the size needed
    
    block_children_size = block->n_children;
    block->children = realloc(block->children, block_children_size);
    
    if (next){
        *next_out = next->next;
    }
    
    return block;
}

AST_Node* tree_DECLARATION(const Typed_Token* token, Typed_Token** next_out, AST_Error* err_stack){
    
    AST_Node* decl = malloc(sizeof(AST_Node));
    decl->type = N_DECLARATION;
    decl->n_children = 2;
    decl->children = malloc(sizeof(AST_Node*) * 2);
    
    /*
    [IDENTIFIER :: EXPRESSION]
    [IDENTIFER := EXPRESSION]
    [TYPE IDENTIFIER]
    */
    
    AST_Error new_err; // 
    AST_Node* child1; // either the type or the identifier
    AST_Node* child2; // either the identifier or the expression
    AST_Node* next;
    
    // try to make a type node first
    
    child1 = tree_TYPE(token, &next, &new_err);
    
    // if that fails, make an identifier node
    if (!child1){
        // TO-DO: clear error stack
        child1 = tree_IDENTIFIER(token, &next, &new_err);
        
        // if making an identifier failed, the whole declaration has failed
        if (!child1){
            free (decl->children);
            free (decl);
            err_stack->next_err = new_err->next_err;
            
            return NULL;
        }
        
        child2 = tree_EXPRESSION(token, &next, &new_err);
    }
    
    child2 = tree_IDENTIFIER(token, &next, &new_err);
    
    if (!child2){
        free (decl->children);
        free (decl);
        err_stack->next_err = new_err->next_err;
            
        return NULL;
    }
    
    
    if (next){
        *next_out = next->next;
    }
    
    return decl;
}