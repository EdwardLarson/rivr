#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "keywords.h"
#include "ast.h"

typedef enum {
	S_NONE,
	
	// nonterminal symbols
	S_DECL_BLOCK,
	S_DECLARATION,
	S_TYPE,
	S_ID_CHAIN,
	S_EXPRESSION,
	S_INHERIT,
	S_ID_LIST,
	S_FUNC_DECL,
	S_EXPR_IMPERATIVE,
	S_EXPR_BLOCK,
	S_FUNC_ARGS,
	S_TYPE_LIST,
	S_STATEMENT,
	S_OPERABLE,
	S_CALL_ARGS,
	S_ELSE_F,
	S_FLAG_ACC,
	
	// terminal symbols
	S_ENDSTATEMENT,
	S_IDENTIFIER,
	S_IDENTIFIER_ACC,
	S_PRIMITIVE,
	S_OPEN_BRACK,
	S_CLOSE_BRACK,
	S_KW_F,
	S_KW_F_RET,
	S_OPEN_PARA,
	S_CLOSE_PARA,
	S_OPEN_BRACE,
	S_CLOSE_BRACE,
	S_COMMA,
	S_DOT,
	S_COLON,
	S_KW_CLASS,
	S_KW_IS,
	S_KW_NEW,
	S_KW_OF,
	S_ENTERBLOCK,
	S_EXITBLOCK,
	S_CONSTANT,
	S_BINARY_OP,
	S_UNARY_OP_R,
	S_UNARY_OP_L,
	S_ASSIGN,
	S_DECL_ASSIGN,
	S_DECL,
	S_KW_IF,
	S_KW_WHILE,
	S_KW_FOR,
	S_KW_FOREACH,
	S_KW_ELSE,
	S_CONTROL_FLOW,
	S_RETURN,
	S_FLAG,
	
	S_END
	
} CFG_Symbol_Id;

#define CFG_SYM_IS_TERMINAL(symbol) (symbol >= S_ENDSTATEMENT)

struct _CFG_Symbol;
typedef struct _CFG_Symbol{
	CFG_Symbol_Id symbol_id;
	
	union{
		Typed_Token* token;
		AST_Node* node;
	} underlying;
	
	struct _CFG_Symbol* prev;
	struct _CFG_Symbol* next;
	
} CFG_Symbol;

typedef struct {
	CFG_Symbol pattern[8];
	int node_children[8];
	
	// identifiers for what this pattern reduces to
	CFG_Symbol_Id symbol_type;
	Node_Type node_type;
	
} Reduction_Pattern;



CFG_Symbol* convert_to_cfg_symbols(Typed_Token* token);
CFG_Symbol* token_to_symbol(Typed_Token* token);

void print_cfg_symbol(CFG_Symbol* symbol);


#endif