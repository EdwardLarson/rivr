#include "rv_types.h"

typedef struct Function_ {
	PCType addr;
	
	// the enclosure
	union Data_* local_data;
	byte* registers;
	int n_enclosed;
	int enclosure_size;
	
} Function;

Function* create_Function(PCType f_start);
void init_Function(Function* f, PCType f_start);
void teardown_Function(Function* f);

PCType load_Function(const Function* f, struct Thread_* th);