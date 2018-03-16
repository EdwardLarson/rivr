typedef enum {
	K_F,
	K_INT,
	K_RAT,
	K_BOOL,
	K_STR,
	K_RETURN,
	K_BREAK,
	K_CONTINUE,
	K_WHILE,
	K_FOR,
	K_FOREACH,
	K_IF,
	K_ELSE,
	K_IS,
	K_CLASS,
	K_ACTOR,
	AND,
	OR
} KEYWORD;

#define N_KEYWORDS 18
const char* const keywords[N_KEYWORDS];

typedef enum {
	O_ADD,
	O_SUB,
	O_DIV,
	O_MUL,
	O_MOD,
	O_BIT_AND,
	O_BIT_OR,
	O_BIT_XOR,
	O_POW,
	O_RSH,
	O_LSH,
	O_INCR,
	O_DECR,
	O_LT,
	O_LTE,
	O_GT,
	O_GTE,
	O_NOT,
	O_EQU,
	O_NEQ,
	O_ADD_ASSIGN,
	O_SUB_ASSIGN,
	O_DIV_ASSIGN,
	O_MUL_ASSIGN,
	O_MOD_ASSIGN,
	O_BIT_AND_ASSIGN,
	O_BIT_OR_ASSIGN,
	O_BIT_XOR_ASSIGN,
	O_POW_ASSIGN,
	O_RSH_ASSIGN,
	O_LSH_ASSIGN,
	
} OPERATOR;

#define N_OPERATORS 31

const char* const operators[N_OPERATORS];

typedef enum {
	F_PUBLIC,
	F_PRIVATE,
	F_OVERRIDE,
	F_PURE
} FLAG;

#define N_FLAGS 4

const char* const flags[N_FLAGS];