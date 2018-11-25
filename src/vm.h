#include <malloc.h>
#include <pthread.h>

#include "rv_strings.h"
#include "rv_functions.h"
#include "opcodes.h"
#include "rv_types.h"

#define FRAME_STACK_SIZE 256

# define TH_STAT_TOPRETURN	-3
#define TH_STAT_KILLED		-2
#define TH_STAT_FIN			-1
#define TH_STAT_WAIT		 0
#define TH_STAT_RDY			 1
#define TH_STAT_RUN			 2

#define CLEAR_DATA(data) data.n = 0;

#define SREG_ZERO_N		0
#define SREG_ONE_N		1
#define SREG_STDOUT		2
#define SREG_STDIN		3
#define SREG_STDERR		4
#define SREG_ZERO_R		5
#define SREG_ONE_R		6
#define SREG_HALF_R		7



typedef struct Operation_{
	byte opcode; // only 6 bits planned to be used
	byte subop; // only 4 bits planned to be used
} Operation;

Operation read_op(const byte* bytes, PCType pc);

Operation encode_operation(byte opcode, byte subop);

// data is stored as a union
// size is 64 bits (8 bytes)

typedef union Data_
{
	long int n; // 64 bit number: Number / num
	double d; // 64 bit floating-point number: Rational / rat
	void* p; // pointer to arbitrary data: Object
	Rivr_String* s; // pointer to some string data: String
	struct Thread_* t; // pointer to some thread data: Thread
	struct Function_* f; // pointer to some function data: Function / f
	byte b; // boolean value: Boolean / bool
	void* h; // pointer to a hash table
	
	PCType addr; // not an actual data type, but useful for jump/branch constants
	
	byte bytes[8];
} Data;

// data is stored in registers:

typedef struct Register_Frame_{
	// stored in frame:
	// 64 variable registers
	// 32 argument read-only registers
	// 32 return write-only registers
	// not stored because they don't need to be:
	// 32 argument write-only registers (discarded on pop, copied to next frame on push)
	// 32 return read-only registers (discarded on pop, become stale and may be overwritten on push)
	// 32 global registers (do not change on push/pop)
	// 32 special registers (do not change on push/pop)
	
	// 128 registers to a frame
	// (therefore each frame is just over 1 kb of memory, taking into account the two pointers and extra byte)

	Data v_registers[64];
	Data a_read_registers[32];
	Data r_write_registers[32];
	
	struct Register_Frame_* nxt_frame;
	struct Register_Frame_* prv_frame;
} Register_Frame;

Register_Frame* alloc_frame(Register_Frame* prv);
void dealloc_frames(Register_Frame* init_frame);

typedef union Register_Cache_{
	Data all_registers[256];
		
	struct{
		Data v_registers[64];
		Data a_read_registers[32];
		Data r_read_registers[32];
		Data a_write_registers[32];
		Data r_write_registers[32];
		Data g_registers[32];
		Data s_registers[32];
		
	};
} Register_Cache;

void init_Register_Cache(Register_Cache* rc, Register_Frame* reference_frame);

// registers are held in a container of register files
// along with an accompanying set of global and special registers

typedef struct Register_File_ {
	Register_Frame* prev_frame;
	Register_Frame* curr_frame;
	Register_Frame* next_frame;
	
	int references;
} Register_File;

int init_Register_File(Register_File* rf);
void init_spec_registers(Data* registers);

// multiple threads may run concurrently
// threads share a register file, but have separate framestacks

typedef struct Thread_ {
	Register_File* rf;
	
	const byte* prog;
	PCType pc;
	PCType prog_len;
	
	int status;
	
	pthread_t tid;
} Thread;

void init_Thread(Thread* th, Register_File* rf, const byte* prog, PCType prog_len, PCType pc_start);
Thread* fork_Thread(const Thread* parent, PCType pc_start);
Thread* create_child_Thread(const Thread* parent, PCType pc_start);
void start_Thread(Thread* th);
void teardown_Thread(Thread* th);

void push_frame(Thread* th, Register_Cache* local_rc);
void pop_frame(Thread* th, Register_Cache* local_rc);

byte read_byte(PCType pc, const Thread* th);

// function which performs actual execution of code
void* run_thread(void* th_in);

Rivr_String* read_into_string(FILE* fp);
byte read_into_bool(FILE* fp);
long int pow_num(long int b, long int e);
double pow_rat(double b, double e);


