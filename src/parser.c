#include <stdlib.h>
#include "parser.h"

/*
Reduction_Pattern patterns[] = {
	{ {S_DECLARATION, S_ENDSTATEMENT, S_DECL_BLOCK, S_NONE, S_NONE, S_NONE, S_NONE, S_NONE}, {}, S_DECL_BLOCK, N_DECLARATION_BLOCK }
	
	{ {S_IDENTIFIER, S_DECL_ASSIGN, S_EXPRESSION, S_ENDSTATEMENT, S_NONE, S_NONE, S_NONE, S_NONE}, {}, S_DECLARATION, N_DECLARATION }
};

*/

#define MAP_TO_ALL_SYMBOLS(FUNC, SYMBOLS, RESULT) {    \
	CFG_Symbol* last = SYMBOLS;                        \
	while (SYMBOLS){                                   \
		SYMBOLS = FUNC(SYMBOLS);                       \
		                                               \
		last = SYMBOLS;                                \
		SYMBOLS = SYMBOLS->next;                       \
	}                                                  \
	                                                   \
	*RESULT = last;                                    \
}

const char* global_symbol_names[] = {
	[S_NONE           ] = "NONE",
	[S_DECL_BLOCK     ] = "DECL_BLOCK",
	[S_DECLARATION    ] = "DECLARATION",
	[S_TYPE           ] = "TYPE",
	[S_ID_CHAIN       ] = "ID_CHAIN",
	[S_EXPRESSION     ] = "EXPRESSION",
	[S_INHERIT        ] = "INHERIT",
	[S_ID_LIST        ] = "ID_LIST",
	[S_FUNC_DECL      ] = "FUNC_DECL",
	[S_EXPR_IMPERATIVE] = "EXPR_IMPERATIVE",
	[S_EXPR_BLOCK     ] = "EXPR_BLOCK",
	[S_FUNC_ARGS      ] = "FUNC_ARGS",
	[S_TYPE_LIST      ] = "TYPE_LIST",
	[S_STATEMENT      ] = "STATEMENT",
	[S_OPERABLE       ] = "OPERABLE",
	[S_CALL_ARGS      ] = "CALL_ARGS",
	[S_ELSE_F         ] = "ELSE_IF",
	[S_FLAG_ACC       ] = "FLAG_ACC",
	[S_ENDSTATEMENT   ] = "ENDSTATMENT",
	[S_IDENTIFIER     ] = "IDENTIFIER",
	[S_IDENTIFIER_ACC ] = "IDENTIFIER_ACC",
	[S_PRIMITIVE      ] = "PRIMITIVE",
	[S_OPEN_BRACK     ] = "OPEN_BRACK",
	[S_CLOSE_BRACK    ] = "CLOSE_BRACK",
	[S_KW_F           ] = "KW_F",
	[S_KW_F_RET       ] = "FW_F_RET",
	[S_OPEN_PARA      ] = "OPEN_PARA",
	[S_CLOSE_PARA     ] = "CLOSE_PARA",
	[S_OPEN_BRACE     ] = "OPEN_BRACE",
	[S_CLOSE_BRACE    ] = "CLOSE_BRACE",
	[S_COMMA          ] = "COMMA",
	[S_DOT            ] = "DOT",
	[S_COLON          ] = "COLON",
	[S_KW_CLASS       ] = "KW_CLASS",
	[S_KW_IS          ] = "KW_IS",
	[S_KW_NEW         ] = "KW_NEW",
	[S_KW_OF          ] = "KW_OF",
	[S_ENTERBLOCK     ] = "ENTERBLOCK",
	[S_EXITBLOCK      ] = "EXITBLOCK",
	[S_CONSTANT       ] = "CONSTANT",
	[S_BINARY_OP      ] = "BINARY_OP",
	[S_UNARY_OP_R     ] = "UNARY_OP_R",
	[S_UNARY_OP_L     ] = "UNARY_OP_L",
	[S_ASSIGN         ] = "ASSIGN",
	[S_DECL_ASSIGN    ] = "DECL_ASSIGN",
	[S_DECL           ] = "DECL",
	[S_KW_IF          ] = "KW_IF",
	[S_KW_WHILE       ] = "KW_WHILE",
	[S_KW_FOR         ] = "KW_FOR",
	[S_KW_FOREACH     ] = "KW_FOREACH",
	[S_KW_ELSE        ] = "KW_ELSE",
	[S_CONTROL_FLOW   ] = "CONTROL_FLOW",
	[S_RETURN         ] = "RETURN",
	[S_FLAG           ] = "FLAG",
	[S_END            ] = "END",
};

void print_cfg_symbol(CFG_Symbol* symbol){
	if (symbol) printf("%s\n", global_symbol_names[symbol->symbol_id]);
	
	
}


/* Convert a linked list of typed tokens to a doubly linked list of cfg symbols
 * Each symbol may have either an underlying typed token or AST node
 * (a loop is used instead of recursion due to stack overflow concerns)
**/
CFG_Symbol* convert_to_cfg_symbols(Typed_Token* token){
	CFG_Symbol dummy_head; // <- not an insult!
	dummy_head.next = NULL;
	CFG_Symbol* converted_symbol = &dummy_head;
	
	while (token){
		converted_symbol->next = token_to_symbol(token);
		
		converted_symbol->next->prev = converted_symbol;
		converted_symbol = converted_symbol->next;
		
		printf("\tConvert a token{%u} to a symbol{%u}\n", token->type, converted_symbol->symbol_id);
		
		token = token->next;
	}
	
	converted_symbol->next = NULL;
	
	if (dummy_head.next){
		dummy_head.next->prev = NULL;
	}
	
	return dummy_head.next;
}

/* Convert a token to a symbol
 * This may be as simple as assigning a symbol id and wrapping in a CFG_Symbol struct,
 * or the token may be converted to an AST_Node and discarded. The new node is then
 * wrapped in a CFG_Symbol struct.
**/
CFG_Symbol* token_to_symbol(Typed_Token* token){
	
	// new node may or may not be created
	AST_Node* new_node = NULL;
	Node_Type node_type = N_NONE;
	Typed_Token_Data node_data = token->data;
	int node_n_children = 0;
	
	CFG_Symbol_Id sym_id;
	
	switch(token->type){
		case T_IDENTIFIER:
			node_type = N_VARIABLE;
			sym_id = S_IDENTIFIER;
			break;
			
		case T_KEYWORD:
			switch(token->data.keyword)
			{
				case K_INT:
				case K_RAT:
				case K_BOOL:
				case K_STR:
					node_type = N_PRIMITIVE;
					sym_id = S_TYPE;
					break;
					
				case K_F:
					sym_id = S_KW_F;
					break;
					
				case K_RETURN:
					sym_id = S_RETURN;
					break;
					
				case K_BREAK:
				case K_CONTINUE:
					sym_id = S_CONTROL_FLOW;
					break;
					
				case K_WHILE:
					sym_id = S_KW_WHILE;
					break;
					
				case K_FOR:
					sym_id = S_KW_FOR;
					break;
					
				case K_FOREACH:
					sym_id = S_KW_FOREACH;
					break;
					
				case K_IF:
					sym_id = S_KW_IF;
					break;
					
				case K_ELSE:
					sym_id = S_KW_ELSE;
					break;
					
				case K_IS:
					sym_id = S_KW_IS;
					break;
					
				case K_CLASS:
					sym_id = S_KW_CLASS;
					break;
					
				case K_ACTOR:
					///TO-DO
					sym_id = S_NONE;
					break;
					
				case K_AND:
					sym_id = S_BINARY_OP;
					node_data.oper = O_LOG_AND;
					break;
					
				case K_OR:
					sym_id = S_BINARY_OP;
					node_data.oper = O_LOG_OR;
					break;
					
				case K_XOR:
					sym_id = S_BINARY_OP;
					node_data.oper = O_LOG_XOR;
					break;
					
				case K_TRUE:
					node_type = N_BOOLEAN;
					node_data.integer = 1;
					
					sym_id = S_CONSTANT;
					break;
					
				case K_FALSE:
					node_type = N_BOOLEAN;
					node_data.integer = 0;
					
					sym_id = S_CONSTANT;
					break;
					
				case K_NEW:
					sym_id = S_KW_NEW;
					break;
				
				case K_OF:
					sym_id = S_KW_OF;
					break;
					
				default:
					fprintf(stderr, "token_to_symbol(): Unrecognized keyword token %u\n", token->data.keyword);
					exit(1);
			}
			break;
			
		case T_FLAG:
			sym_id = S_FLAG;
			break;
			
		case T_VAR_DECLARE:
			sym_id = S_DECL;
			break;
			
		case T_VAR_DEFINE:
			sym_id = S_DECL_ASSIGN;
			break;
			
		case T_INTEGER:
			node_type = N_INTEGER;
			sym_id = S_CONSTANT;
			break;
			
		case T_RATIONAL:
			node_type = N_RATIONAL;
			sym_id = S_CONSTANT;
			break;
			
		case T_STRING:
			node_type = N_STRING;
			sym_id = S_CONSTANT;
			break;
			
		case T_FUNC_RETURN:
			sym_id = S_KW_F_RET;
			break;
			
		case T_OPERATOR:
			switch(token->data.oper){
				case O_ADD:
				case O_SUB:
				case O_DIV:
				case O_MUL:
				case O_MOD:
				case O_BIT_AND:
				case O_BIT_OR:
				case O_BIT_XOR:
				case O_POW:
				case O_RSH:
				case O_LSH:
				case O_LT:
				case O_LTE:
				case O_GT:
				case O_GTE:
				case O_EQU:
				case O_NEQ:
				case O_LOG_AND:
				case O_LOG_OR:
				case O_LOG_XOR:
					sym_id = S_BINARY_OP;
					break;
					
				case O_INCR:
				case O_DECR:
					// in Rivr, increment and decrement are not operations with a return value
					// they are shorthand for '+= 1' and '-= 1' respectively
					sym_id = S_ASSIGN;
					break;
					
				case O_NOT:
					/// TO-DO: '-' can also be a left unary op but without context (future reductions needed) it is impossible to know
					sym_id = S_UNARY_OP_L;
					break;
					
				case O_ADD_ASSIGN:
				case O_SUB_ASSIGN:
				case O_DIV_ASSIGN:
				case O_MUL_ASSIGN:
				case O_MOD_ASSIGN:
				case O_BIT_AND_ASSIGN:
				case O_BIT_OR_ASSIGN:
				case O_BIT_XOR_ASSIGN:
				case O_POW_ASSIGN:
				case O_RSH_ASSIGN:
				case O_LSH_ASSIGN:
					sym_id = S_ASSIGN;
					break;
				
			}
			
			break;
			
		case T_ASSIGNMENT:	
			sym_id = S_ASSIGN;
			break;
			
		case T_COMMA:
			sym_id = S_COMMA;
			break;
			
		case T_PERIOD:
			sym_id = S_DOT;
			break;
			
		case T_COLON:
			sym_id = S_COLON;
			break;
			
		case T_OPEN_PARA:
			sym_id = S_OPEN_PARA;
			break;
			
		case T_CLOSE_PARA:
			sym_id = S_CLOSE_PARA;
			break;
			
		case T_OPEN_BRACE:
			sym_id = S_OPEN_BRACE;
			break;
			
		case T_CLOSE_BRACE:
			sym_id = S_CLOSE_BRACE;
			break;
			
		case T_OPEN_BRACK:
			sym_id = S_OPEN_BRACK;
			break;
			
		case T_CLOSE_BRACK:
			sym_id = S_CLOSE_BRACK;
			break;
		
		case T_END_STATEMENT:
			sym_id = S_ENDSTATEMENT;
			break;
			
		case T_ENTER_BLOCK:
			sym_id = S_ENTERBLOCK;
			break;
			
		case T_EXIT_BLOCK:
			sym_id = S_EXITBLOCK;
			break;
			
		default:
			fprintf(stderr, "token_to_symbol(): Unrecognized token type %u\n", token->type);
			exit(1);
			
	}
	
	
	CFG_Symbol* symbol = malloc(sizeof(CFG_Symbol));
	symbol->symbol_id = sym_id;
	
	if (node_type == N_NONE){
		symbol->underlying.token = token;
	}else{
		symbol->underlying.node = create_ast_node(node_type, node_n_children, node_data);
		
		// token data is not freed
		// if token data is discarded (not reused by node),
		// it must be discarded in above switch block
		free(symbol->underlying.token);
	}
	
	return symbol;
}