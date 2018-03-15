#include "rv_types.h"

#define CPY_NDATA_GUESS 1

typedef struct Function_ {
	PCType addr;
	
	// the enclosure
	union Data_* local_data;
	byte* registers;
	int n_enclosed;
	int enclosure_size;
	
} Function;

Function* create_Function(PCType f_start, int ndata);
void init_Function(Function* f, PCType f_start, int ndata);
Function* copy_Function(const Function* f);
void teardown_Function(Function* f);

void enclose_data_Function(Function* f, union Data_* data, byte reg);
PCType load_Function(const Function* f, struct Thread_* th);