#include <malloc.h>

#include "rv_strings.h"
#include "opcodes.h"

#define FRAME_STACK_SIZE 256

#define TH_STAT_FIN -1
#define TH_STAT_WAIT 0
#define TH_STAT_RDY 1

#define CLEAR_DATA(data) data.n = 0;


typedef unsigned char byte;

typedef unsigned long PCType;

// opcodes are 6 bits (64 possible) in length
// subop is 4 bits (32 possible) in length
// for 10 bits used
// therefore entire operations will be 16 bits (2 bytes) in length
// 6 bits are left over
// these may in the future be used to expand the number of opcodes or subfunctions

typedef struct Operation_{
	byte bytes[2];
} Operation;

Operation read_op(const byte* bytes, PCType pc);


byte get_opcode(Operation op);

byte get_subop(Operation op);

Operation encode_operation(byte opcode, byte subop);

// data is stored as a union
// size is 64 bits (8 bytes)

struct Thread_;

typedef union Data_
{
	long int n; // 64 bit number: Number / num
	double d; // 64 bit floating-point number: Rational / rat
	void* p; // pointer to arbitrary data: Object
	Rivr_String* s; // pointer to some string data: String
	struct Thread_* t; // pointer to some thread data: Thread
	PCType f; // pointer to some function data: Function / f
	byte b; // boolean value: Boolean / bool
	void* h; // pointer to a hash table
	
	byte bytes[8];
} Data;


// data is stored in registers:

typedef struct Register_Frame_{
	// 64 variable registers
	// 32 argument read-only registers
	// 32 return read-only registers
	// from these 3 groups there are 128 registers to a frame
	// (therefore each frame is just over 1 kb of memory, taking into account the two pointers and extra byte)

	Data v_registers[64];
	Data a_registers[32];
	Data r_registers[32];
	
	struct Register_Frame_* nxt_frame;
	struct Register_Frame_* prv_frame;
	byte used;
} Register_Frame;

// registers are held in a container of register files
// along with an accompanying set of global and special registers

typedef struct Register_File_ {
	Register_Frame frames[FRAME_STACK_SIZE];
	Data g_registers[32];
	Data s_registers[32];
} Register_File;

int init_Register_File(Register_File* rf);
void init_spec_registers(Data* registers);

Register_Frame* next_free_frame(Register_File* rf);

Register_Frame* alloc_frame(Register_File* rf, Register_Frame* prv);

// multiple threads may run concurrently
// threads share a register file, but have separate framestacks

typedef struct Thread_ {
	Register_File* rf;
	
	Register_Frame* frame;
	
	const byte* prog;
	PCType pc;
	PCType prog_len;
	
	int status;
} Thread;

void init_Thread(Thread* th, Register_File* rf, const byte* prog, PCType prog_len, PCType pc_start);
void push_frame(Thread* th);
void pop_frame(Thread* th);

Data* access_register(PCType pc, const Thread* th);
Data access_constant(PCType pc, const Thread* th);

// function which performs actual execution of code
void run_thread(Thread* th);

Rivr_String* read_into_string(FILE* fp);
byte read_into_bool(FILE* fp);
long int pow_num(long int b, long int e);
double pow_rat(double b, double e);

// function semantics:
// f(arg0, ... ):<expression> returns an anonymous function
// very similar to " \(args) . (expression)"

// f name(arg0, ... ) returns <expression>
// f name(arg0, ... ) returns <type>: <expression-block>
// both define functions in the current space

typedef struct Function_ {
	PCType pc;
	
	Register_Frame state;
	
	
	
} Function;


