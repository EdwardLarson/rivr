#include "vm.h"

#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include <errno.h>

#ifdef DEBUG
#define LOAD_NEXT_OPCODE(PC_Next, Thread, Op) \
	{ if ((Thread->status <= 0) || (PC_Next >= Thread->prog_len)) goto L_END_OF_PROGRAM; \
			Thread->pc = PC_Next; \
			memcpy(&Op, &Thread->prog[PC_Next], 2); \
			PC_Next += 2; \
			printf("\tpc[%lu]-", Thread->pc); \
			printf("Opcode[%x]-Subop[%x]-", Op.opcode, Op.subop); \
			printf("Status[%d]\n", Thread->status); \
	}
#else
#define LOAD_NEXT_OPCODE(PC_Next, Thread, Op) \
	{ if ((Thread->status <= 0) || (PC_Next >= Thread->prog_len)) goto L_END_OF_PROGRAM; \
		Thread->pc = PC_Next; \
		memcpy(&Op, &Thread->prog[PC_Next], 2); \
		PC_Next += 2; \
	}
#endif
		
#define JUMP_TO_NEXT_INSTRUCTION(ITable, Op) \
	goto *(ITable[Op.opcode]) 
	
#define REGISTER(Cache, Reg_Address) (Cache.all_registers[Reg_Address])

#define READ_PROG_BYTE(Prog, PC_Next) \
	Prog[PC_Next++]
	
#define READ_REGISTER_ARG(Arg_Regs, Arg_N, Prog, PC_Next) \
	{ Arg_Regs[Arg_N] = Prog[PC_Next]; PC_Next += 1; }
	
#define READ_CONSTANT_ARG(Arg_Regs, Arg_N, Prog, PC_Next, Cache, Const_N) \
	{ memcpy(&Cache.all_registers[Const_N], &Prog[PC_Next], sizeof(Data)); Arg_Regs[Arg_N] = Const_N; PC_Next += sizeof(Data); }

#define WRITE_OP_RESULT(Prog, PC_Next, Cache, Result) \
	{ Cache.all_registers[Prog[PC_Next]] = Result; PC_Next += 1; }
	
// Last 4 registers of the Special registers are for holding constants
#define CONST_REG_0 0xFC
#define CONST_REG_1 0xFD
#define CONST_REG_2 0xFE
#define CONST_REG_3 0xFF

#ifdef VM_ONLY
int main(int argc, char** argv){
	Register_File rf;
	init_Register_File(&rf);
	
	byte test_prog[32];
	
	Thread th;
	init_Thread(&th, &rf, test_prog, 32, 0);
	
	run_thread(&th);
	
	printf("Sizeof data is %d\n", sizeof(Data));
	
	return 0;
}

#endif

Operation read_op(const byte* bytes, PCType pc){
	Operation op;
	
	memcpy(&op, &bytes[pc], 2);
	
	return op;
}

Operation encode_operation(byte opcode, byte subop){
	Operation op;
	op.opcode = opcode;
	op.subop = subop;
	
	return op;
}


int init_Register_File(Register_File* rf){
	rf->prev_frame = NULL;
	rf->curr_frame = NULL;
	rf->next_frame = NULL;
	
	rf->references = 0;
	
	return 0;
}

void init_Register_Cache(Register_Cache* rc, Register_Frame* reference_frame){
	if (reference_frame != NULL){
		memcpy(rc->v_registers, reference_frame->v_registers, 64 * sizeof(Data));
		memcpy(rc->a_read_registers, reference_frame->a_read_registers, 32 * sizeof(Data));
		memcpy(rc->r_write_registers, reference_frame->r_write_registers, 32 * sizeof(Data));
	}
	memset(rc->a_write_registers, 0, 32 * sizeof(Data));
	memset(rc->r_read_registers, 0, 32 * sizeof(Data));
	memset(rc->g_registers, 0, 32 * sizeof(Data));
	
	init_spec_registers(rc->s_registers);
}

Register_Frame* alloc_frame(Register_Frame* prv){
	Register_Frame* frame = calloc(1, sizeof(Register_Frame));
	frame->prv_frame = prv;
	frame->nxt_frame = NULL;
	
	return frame;
}

void dealloc_frames(Register_Frame* init_frame){
	if (!init_frame) return;
	
	while (init_frame->prv_frame){
		init_frame = init_frame->prv_frame;
	}
	
	Register_Frame* tmp;
	while (init_frame){
		tmp = init_frame->nxt_frame;
		free(init_frame);
		init_frame = tmp;
	}
}

void init_Thread(Thread* th, Register_File* rf, const byte* prog, PCType prog_len, PCType pc_start){
	th->rf = rf;
	th->rf->references += 1;
	
	th->rf->prev_frame = alloc_frame(NULL);
	th->rf->curr_frame = alloc_frame(th->rf->prev_frame);
	th->rf->next_frame = alloc_frame(th->rf->curr_frame);
	
	th->rf->curr_frame->nxt_frame = th->rf->next_frame;
	
	th->prog = prog;
	th->prog_len = prog_len;
	th->pc = pc_start;
	
	th->status = TH_STAT_RDY;
}

void init_spec_registers(Data* registers){
	
	registers[SREG_ZERO_N].n = 0;
	
	registers[SREG_ONE_N].n = 1;
	
	registers[SREG_STDOUT].n = (long int) stdout;
	
	registers[SREG_STDIN].n = (long int) stdin;
	
	registers[SREG_STDERR].n = (long int) stderr;
	
	registers[SREG_ZERO_R].d = 0.0D;
	
	registers[SREG_ONE_R].d = 1.0D;
	
	registers[SREG_HALF_R].d = 0.5D;
	
}

Thread* fork_Thread(const Thread* parent, PCType pc_start){
	Thread* th = create_child_Thread(parent, pc_start);
	
	start_Thread(th);
	
	return th;
	
}
	
Thread* create_child_Thread(const Thread* parent, PCType pc_start){
	Thread* th = malloc(sizeof(Thread));
	
	Register_File* rf = calloc(1, sizeof(Register_File));
	init_Register_File(th->rf);
	
	init_Thread(th, rf, parent->prog, parent->prog_len, pc_start);
	
	return th;
}

void start_Thread(Thread* th){
	int err = pthread_create(&th->tid, NULL, run_thread, th);
	if (err){
		printf("Error: Unable to create thread\n");
	}
}


void teardown_Thread(Thread* th){
	dealloc_frames(th->rf->curr_frame);
	th->rf->references -= 1;
	
	if (th->rf->references == 0){
		#ifdef DEBUG
		printf("\tFreed register file in thread teardown\n");
		#endif
		free (th->rf);
	}
	
	#ifdef DEBUG
	printf("\tAbout to free th\n");
	#endif
	
	free(th);
	
	#ifdef DEBUG
	printf("\tfreed th\n");
	#endif
}

void push_frame(Thread* th, Register_Cache* local_rc){
	
	Register_Frame* original_prev_frame = th->rf->prev_frame;
	Register_Frame* original_curr_frame = th->rf->curr_frame;
	Register_Frame* original_next_frame = th->rf->next_frame;
	
	#ifdef DEBUG
	printf("push_frame: o_prev<%p>, o_curr<%p>, o_next<%p>\n", 
		original_prev_frame,
	    original_curr_frame,
	    original_next_frame);
	#endif
	
	// copy cache registers to frame:
	Data* src;
	Data* dest;
	int width;
	// copy var registers
	src = local_rc->v_registers;
	dest = original_curr_frame->v_registers;
	width = 64;
	memcpy(dest, src, width * sizeof(Data));
	
	// copy arg_read registers
	src = local_rc->a_read_registers;
	dest = original_curr_frame->a_read_registers;
	width = 32;
	memcpy(dest, src, width * sizeof(Data));
	
	// copy ret_write_registers
	src = local_rc->r_write_registers;
	dest = original_curr_frame->r_write_registers;
	width = 32;
	memcpy(dest, src, width * sizeof(Data));
	
	// copy written args to arg_read registers
	src = local_rc->a_write_registers;
	dest = local_rc->a_read_registers;
	width = 32;
	memcpy(dest, src, width * sizeof(Data));
	
	th->rf->prev_frame = original_curr_frame;
	th->rf->curr_frame = original_next_frame;
	if (th->rf->curr_frame->nxt_frame == NULL){
		th->rf->curr_frame->nxt_frame = alloc_frame(th->rf->curr_frame);
	}
	th->rf->next_frame = th->rf->curr_frame->nxt_frame;
	
	#ifdef DEBUG
	printf("push_frame: a_prev<%p>, a_curr<%p>, a_next<%p>\n", 
		th->rf->prev_frame,
	    th->rf->curr_frame,
	    th->rf->next_frame);
	#endif
}

void pop_frame(Thread* th, Register_Cache* local_rc){
	Register_Frame* original_prev_frame = th->rf->prev_frame;
	Register_Frame* original_curr_frame = th->rf->curr_frame;
	Register_Frame* original_next_frame = th->rf->next_frame;
	
	#ifdef DEBUG
	printf("pop_frame: o_prev<%p>, o_curr<%p>, o_next<%p>\n", 
		original_prev_frame,
	    original_curr_frame,
	    original_next_frame);
	#endif
	
	// copy cache registers from frame:
	Data* src;
	Data* dest;
	int width;
	// copy var registers
	src = original_prev_frame->v_registers;
	dest = local_rc->v_registers;
	width = 64;
	memcpy(dest, src, width * sizeof(Data));
	
	// copy arg_read registers
	src = original_prev_frame->a_read_registers;
	dest = local_rc->a_read_registers;
	width = 32;
	memcpy(dest, src, width * sizeof(Data));
	
	
	// copy written returns to ret_read registers
	src = local_rc->r_write_registers;
	dest = local_rc->r_read_registers;
	width = 32;
	memcpy(dest, src, width * sizeof(Data));
	
	// copy ret_write_registers
	src = original_prev_frame->r_write_registers;
	dest = local_rc->r_write_registers;
	width = 32;
	memcpy(dest, src, width * sizeof(Data));
	
	th->rf->prev_frame = original_prev_frame->prv_frame;
	th->rf->curr_frame = original_prev_frame;
	th->rf->next_frame = original_curr_frame;
	
	free(original_next_frame);
	th->rf->next_frame->nxt_frame = NULL;
	
	#ifdef DEBUG
	printf("pop_frame: a_prev<%p>, a_curr<%p>, a_next<%p>\n", 
		th->rf->prev_frame,
	    th->rf->curr_frame,
	    th->rf->next_frame);
	#endif
	
	
	// program should automatically exit when top-level frame is popped (no prev)
	if (th->rf->prev_frame == NULL){
		th->status = TH_STAT_TOPRETURN;
	}
}

inline byte read_byte(PCType pc, const Thread* th){
	return th->prog[pc];
}

void* run_thread(void* th_in){
	Thread* th = (Thread*) th_in;
	Operation op;
	PCType pc_next;
	
	Register_Cache rc;
	init_Register_Cache(&rc, th->rf->curr_frame);
	
	byte arg_regs[8]; // store register values which are used as arguments
	Data tmp;
	Data result;
	
	th->tid = pthread_self();
	th->status = TH_STAT_RUN;
	
	void* instruction_table[] = {
		&&L_ABS,
		&&L_ADD,
		&&L_AND,
		&&L_BRANCH,
		&&L_BITWISE,
		&&L_DECR,
		&&L_DIV,
		&&L_EQ,
		&&L_F_CALL,
		&&L_F_CREATE,
		&&L_GT,
		&&L_HALT,
		&&L_INCR,
		&&L_INPUT,
		&&L_JUMP,
		&&L_LSH,
		&&L_LT,
		&&L_M_ALLOC,
		&&L_M_FREE,
		&&L_M_LOAD,
		&&L_M_STORE,
		&&L_MOD,
		&&L_MOVE,
		&&L_MUL,
		&&L_NOOP,
		&&L_NOT,
		&&L_OR,
		&&L_OUTPUT,
		&&L_POPFRAME,
		&&L_PUSHFRAME,
		&&L_POW,
		&&L_RSH,
		&&L_SUB,
		&&L_TH_NEW,
		&&L_TH_JOIN,
		&&L_TH_KILL,
		&&L_XOR
	};
		
	CLEAR_DATA(result);
	
	pc_next = th->pc;
	
	LOAD_NEXT_OPCODE(pc_next, th, op);
	JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
	
	L_ABS:
		switch(op.subop){
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_NONE, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				result.n = (REGISTER(rc, arg_regs[0]).n < 0) ? (-1 * REGISTER(rc, arg_regs[0]).n) : (REGISTER(rc, arg_regs[0]).n);
				
				break;
			case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_NONE, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				result.n = (REGISTER(rc, arg_regs[0]).n < 0) ? (-1 * REGISTER(rc, arg_regs[0]).n) : (REGISTER(rc, arg_regs[0]).n);
				
				break;
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_NONE, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				result.d = (REGISTER(rc, arg_regs[0]).d < 0) ? (-1 * REGISTER(rc, arg_regs[0]).d) : (REGISTER(rc, arg_regs[0]).d);
				
				break;
			case FORMAT1_SUBOP(SO_RATIONAL, SO_CONSTANT, SO_NONE, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				result.d = (REGISTER(rc, arg_regs[0]).n < 0) ? (-1 * REGISTER(rc, arg_regs[0]).d) : (REGISTER(rc, arg_regs[0]).d);
				
				break;
			default:
				break;
		}
	
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
		
	L_ADD:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
		switch(op.subop){
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n + REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.n = REGISTER(rc, arg_regs[0]).n + REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.d = REGISTER(rc, arg_regs[0]).d + REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.d = REGISTER(rc, arg_regs[0]).d + REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_AND:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		switch(op.subop){
			case SO_REGISTER:
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.b = REGISTER(rc, arg_regs[0]).b && REGISTER(rc, arg_regs[1]).b;
				break;
				
			case SO_CONSTANT:
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.b = REGISTER(rc, arg_regs[0]).b && REGISTER(rc, arg_regs[1]).b;
				break;
			
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_BITWISE:
		READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
		
		switch(op.subop){
			case FORMAT3_SUBOP(SO_REGISTER, SO_AND):
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n & REGISTER(rc, arg_regs[1]).n;
				break;
				
			case FORMAT3_SUBOP(SO_REGISTER, SO_NOT):
				
				result.n = ~(REGISTER(rc, arg_regs[0]).n);
				break;
			
			case FORMAT3_SUBOP(SO_REGISTER, SO_OR):
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n | REGISTER(rc, arg_regs[1]).n;
				break;
				
			case FORMAT3_SUBOP(SO_REGISTER, SO_XOR):
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n ^ REGISTER(rc, arg_regs[1]).n;
				break;
				
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_BRANCH:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		if (REGISTER(rc, arg_regs[0]).b){
			pc_next += sizeof(Data);
		}else{
			READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
			pc_next = REGISTER(rc, arg_regs[1]).addr;
		}
		
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_DECR:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		result.n = REGISTER(rc, arg_regs[0]).n - 1;
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_DIV:
		switch(op.subop){
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n / REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.n = REGISTER(rc, arg_regs[0]).n / REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n / REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.d = REGISTER(rc, arg_regs[0]).d / REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.d = REGISTER(rc, arg_regs[0]).d / REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.d = REGISTER(rc, arg_regs[0]).d / REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_EQ:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		switch(op.subop){
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.b = REGISTER(rc, arg_regs[0]).n == REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.b = REGISTER(rc, arg_regs[0]).n == REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.b = REGISTER(rc, arg_regs[0]).d == REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.b = REGISTER(rc, arg_regs[0]).d == REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_F_CALL:
		
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		switch(op.subop){
				
			case SO_PUSHFIRST:
				push_frame(th, &rc);
				// fall into next case
			case SO_DIRECT:
				pc_next = load_Function(REGISTER(rc, arg_regs[0]).f, &rc);
				break;
				
			default:
				break;
		}
		
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_F_CREATE:
		switch(op.subop){
			case FORMAT4_SUBOP(SO_NOCLOSURE, SO_RELATIVE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				result.f = create_Function(th->pc + REGISTER(rc, arg_regs[0]).addr, 0);
				
				break;
				
			case FORMAT4_SUBOP(SO_NOCLOSURE, SO_ABSOLUTE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				result.f = create_Function(REGISTER(rc, arg_regs[0]).addr, 0);
				
				break;
				
			case FORMAT4_SUBOP(SO_CLOSURE, SO_RELATIVE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				tmp.b = READ_PROG_BYTE(th->prog, pc_next);
				
				result.f = create_Function(th->pc + REGISTER(rc, arg_regs[0]).addr, (int)tmp.b);
				
				for(int i = 0; i < tmp.b; i++){
					enclose_data_Function(result.f, &REGISTER(rc, th->prog[pc_next]), READ_PROG_BYTE(th->prog, pc_next));
				}
				
				break;
				
			case FORMAT4_SUBOP(SO_CLOSURE, SO_ABSOLUTE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				tmp.b = READ_PROG_BYTE(th->prog, pc_next);
				
				result.f = create_Function(REGISTER(rc, arg_regs[0]).addr, (int) tmp.b);
				
				for (int i = 0; i < tmp.b; i++){
					enclose_data_Function(result.f, &REGISTER(rc, th->prog[pc_next]), READ_PROG_BYTE(th->prog, pc_next));
				}
				
				break;
				
			case SO_CLONE_F:
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				tmp.b = READ_PROG_BYTE(th->prog, pc_next);
				
				result.f = copy_Function(REGISTER(rc, arg_regs[0]).f);
				
				for (int i = 0; i < tmp.b; i++){
					enclose_data_Function(result.f, &REGISTER(rc, th->prog[pc_next]), READ_PROG_BYTE(th->prog, pc_next));
				}
				
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_GT:
		switch(op.subop){
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.b = REGISTER(rc, arg_regs[0]).n > REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.b = REGISTER(rc, arg_regs[0]).n > REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.b = REGISTER(rc, arg_regs[0]).n > REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.b = REGISTER(rc, arg_regs[0]).d > REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.b = REGISTER(rc, arg_regs[0]).d > REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.b = REGISTER(rc, arg_regs[0]).d > REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_HALT:
		#ifdef DEBUG
		printf("\tProgram halted.\n");
		#endif
		th->status = TH_STAT_FIN;
		
		goto L_END_OF_PROGRAM;
		
	L_INCR:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		result.n = REGISTER(rc, arg_regs[0]).n + 1;
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_INPUT:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		switch(op.subop){
			case SO_NUMBER:
				fscanf((FILE*) REGISTER(rc, arg_regs[0]).n, "%ld", &result.n);
				while (getc((FILE*) REGISTER(rc, arg_regs[0]).n) != '\n'); // flush input buffer
				break;
			case SO_RATIONAL:
				fscanf((FILE*) REGISTER(rc, arg_regs[0]).n, "%lf", &result.d);
				while (getc((FILE*) REGISTER(rc, arg_regs[0]).n) != '\n'); // flush input buffer
				break;
			case SO_STRING:
				result.s = read_into_string((FILE*) REGISTER(rc, arg_regs[0]).n);
				break;
			case SO_BOOLEAN:
				result.b = read_into_bool((FILE*) REGISTER(rc, arg_regs[0]).n);
				break;
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_JUMP:
	
		switch(op.subop){
			case FORMAT3_SUBOP(SO_CONSTANT, SO_ABSOLUTE):
				#ifdef DEBUG
				printf("Constant jump subop\n");
				#endif
				
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				pc_next = REGISTER(rc, arg_regs[0]).addr;
				break;
				
			case FORMAT3_SUBOP(SO_CONSTANT, SO_RELATIVE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				pc_next += REGISTER(rc, arg_regs[0]).addr;
				break;
				
			case FORMAT3_SUBOP(SO_REGISTER, SO_ABSOLUTE):
				#ifdef DEBUG
				printf("Register jump subop\n");
				#endif
			
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				pc_next = REGISTER(rc, arg_regs[0]).addr;
				break;
				
			case FORMAT3_SUBOP(SO_REGISTER, SO_RELATIVE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				pc_next += REGISTER(rc, arg_regs[0]).addr;
				break;
			
			default:
				break;
		}
		
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_LSH:
		switch(op.subop){
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n << REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.n = REGISTER(rc, arg_regs[0]).n << REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n << REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_LT:
		switch(op.subop){
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.b = REGISTER(rc, arg_regs[0]).n < REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.b = REGISTER(rc, arg_regs[0]).n < REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.b = REGISTER(rc, arg_regs[0]).n < REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.b = REGISTER(rc, arg_regs[0]).d < REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.b = REGISTER(rc, arg_regs[0]).d < REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.b = REGISTER(rc, arg_regs[0]).d < REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_M_ALLOC:
		switch(op.subop){
			case SO_REGISTER:
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				result.p = calloc(REGISTER(rc, arg_regs[0]).n, sizeof(Data));
				break;
			case SO_CONSTANT:
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				#ifdef DEBUG
				printf("\tallocating array with length %ld\n", REGISTER(rc, arg_regs[0]).n);
				#endif
				
				result.p = calloc(REGISTER(rc, arg_regs[0]).n, sizeof(Data));
				break;
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_M_FREE:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		switch(op.subop){
			case SO_OBJECT:
				free(REGISTER(rc, arg_regs[0]).p);
				break;
				
			case SO_STRING:
				string_destroy(REGISTER(rc, arg_regs[0]).s);
				break;
				
			case SO_THREAD:
				teardown_Thread(REGISTER(rc, arg_regs[0]).t);
				break;
				
			case SO_FUNCTION:
				teardown_Function(REGISTER(rc, arg_regs[0]).f);
				break;
				
			default:
				break;
		}
		
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_M_LOAD:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		switch(op.subop){
			case SO_REGISTER:
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result = ( (Data*) REGISTER(rc, arg_regs[0]).p )[ REGISTER(rc, arg_regs[1]).n ];
				
				break;
				
			case SO_CONSTANT:
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result = ( (Data*) REGISTER(rc, arg_regs[0]).p )[ REGISTER(rc, arg_regs[1]).n ];
				
				break;
			
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_M_STORE:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		switch(op.subop){
			case FORMAT1_SUBOP(SO_NONE, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 2, th->prog, pc_next);
				
				( (Data*) REGISTER(rc, arg_regs[0]).p )[ REGISTER(rc, arg_regs[2]).n ] = REGISTER(rc, arg_regs[1]);
				
				break;
				
			case FORMAT1_SUBOP(SO_NONE, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 2, th->prog, pc_next, rc, CONST_REG_0);
				
				#ifdef DEBUG
				printf("\tpreparing to store to %p with offset %ld\n", REGISTER(rc, arg_regs[0]).p, REGISTER(rc, arg_regs[2]).n);
				#endif
				
				( (Data*) REGISTER(rc, arg_regs[0]).p )[ REGISTER(rc, arg_regs[2]).n ] = REGISTER(rc, arg_regs[1]);
				
				#ifdef DEBUG
				printf("\tstored\n");
				#endif
				
				break;
				
			case FORMAT1_SUBOP(SO_NONE, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 2, th->prog, pc_next);
				
				( (Data*) REGISTER(rc, arg_regs[0]).p )[ (size_t) REGISTER(rc, arg_regs[2]).n ] = REGISTER(rc, arg_regs[1]);
				
				break;
				
			case FORMAT1_SUBOP(SO_NONE, SO_CONSTANT, SO_CONSTANT, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_CONSTANT_ARG(arg_regs, 2, th->prog, pc_next, rc, CONST_REG_1);
				
				( (Data*) REGISTER(rc, arg_regs[0]).p )[ REGISTER(rc, arg_regs[2]).n ] = REGISTER(rc, arg_regs[1]);
				
				break;
			
			default:
				break;
		}
		
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_MOD:
		switch(op.subop){
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n % REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.n = REGISTER(rc, arg_regs[0]).n % REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n % REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_MOVE:
		switch(op.subop){
			case SO_REGISTER:
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				break;
			case SO_CONSTANT:
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				break;
				
			default:
				break;
		}
		
		result = REGISTER(rc, arg_regs[0]);
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_MUL:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
		switch(op.subop){
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n * REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.n = REGISTER(rc, arg_regs[0]).n * REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.d = REGISTER(rc, arg_regs[0]).d * REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.d = REGISTER(rc, arg_regs[0]).d * REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_NOT:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		result.b = !(REGISTER(rc, arg_regs[0]).b);
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_NOOP:
		
		#ifdef DEBUG
		printf("\tNo-op, pc_next = %lu\n", pc_next);
		
		switch(op.subop){
			case SO_NUMBER:
				printf("\t\tnumber subop\n");
				break;
			case SO_HASHTABLE:
				printf("\t\thashtable subop\n");
				break;
			default:
				break;
		}
		#endif
		
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_OR:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		switch(op.subop){
			case SO_REGISTER:
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.b = REGISTER(rc, arg_regs[0]).b || REGISTER(rc, arg_regs[1]).b;
				break;
				
			case SO_CONSTANT:
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.b = REGISTER(rc, arg_regs[0]).b || REGISTER(rc, arg_regs[1]).b;
				break;
			
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_OUTPUT:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		switch(op.subop){
			case FORMAT2_SUBOP(SO_REGISTER, SO_NUMBER):
			
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "%ld", REGISTER(rc, arg_regs[1]).n);
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_RATIONAL):
			
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "%f", REGISTER(rc, arg_regs[1]).d);
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_OBJECT):
			
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "obj<%p>", REGISTER(rc, arg_regs[1]).p);
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_STRING):
			
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				string_print(REGISTER(rc, arg_regs[1]).s, (FILE*) REGISTER(rc, arg_regs[0]).n);
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_THREAD):
			
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "thr<%p>", REGISTER(rc, arg_regs[1]).t);
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_FUNCTION):
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "fun<%p>{%d enclosed}", REGISTER(rc, arg_regs[1]).f, REGISTER(rc, arg_regs[1]).f->n_enclosed);
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_BOOLEAN):
			
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				if (REGISTER(rc, arg_regs[1]).b){
					fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "True");
				}else{
					fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "False");
				}
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_HASHTABLE):
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "htb<%p>", REGISTER(rc, arg_regs[1]).h);
				break;
				
			case FORMAT2_SUBOP(SO_CONSTANT, SO_NUMBER):
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "%ld", REGISTER(rc, arg_regs[1]).n);
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_RATIONAL):
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "%f", REGISTER(rc, arg_regs[1]).d);
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_OBJECT):
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "obj<%p>", REGISTER(rc, arg_regs[1]).p);
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_STRING):
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "%.*s", 8, REGISTER(rc, arg_regs[1]).bytes);
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_THREAD):
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "thr<%p>", REGISTER(rc, arg_regs[1]).t);
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_FUNCTION):
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "fun<%lx>", REGISTER(rc, arg_regs[1]).addr);
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_BOOLEAN):
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				if (REGISTER(rc, arg_regs[1]).b){
					fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "True");
				}else{
					fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "False");
				}
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_HASHTABLE):
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) REGISTER(rc, arg_regs[0]).n, "htb<%p>", REGISTER(rc, arg_regs[1]).h);
				break;
			
			default:
				break;
		}
		
		#ifdef DEBUG
		printf("\n\tprint accomplished\n");
		#endif
		
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_POPFRAME:
		pop_frame(th, &rc);
		
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_PUSHFRAME:
		push_frame(th, &rc);
		
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_POW:
		// Note: Power function is undefined for negative exponents
		switch(op.subop){
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = pow_num(REGISTER(rc, arg_regs[0]).n, REGISTER(rc, arg_regs[1]).n);
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.n = pow_num(REGISTER(rc, arg_regs[0]).n, REGISTER(rc, arg_regs[1]).n);
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = pow_num(REGISTER(rc, arg_regs[0]).n, REGISTER(rc, arg_regs[1]).n);
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.d = pow_rat(REGISTER(rc, arg_regs[0]).d, REGISTER(rc, arg_regs[1]).d);
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.d = pow_rat(REGISTER(rc, arg_regs[0]).d, REGISTER(rc, arg_regs[1]).d);
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.d = pow_rat(REGISTER(rc, arg_regs[0]).d, REGISTER(rc, arg_regs[1]).d);
				
				break;
				
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_RSH:
		switch(op.subop){
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n >> REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.n = REGISTER(rc, arg_regs[0]).n >> REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n >> REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_SUB:
		switch(op.subop){
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n - REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.n = REGISTER(rc, arg_regs[0]).n - REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.n = REGISTER(rc, arg_regs[0]).n - REGISTER(rc, arg_regs[1]).n;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.d = REGISTER(rc, arg_regs[0]).d - REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.d = REGISTER(rc, arg_regs[0]).d - REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			case FORMAT1_SUBOP(SO_RATIONAL, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.d = REGISTER(rc, arg_regs[0]).d - REGISTER(rc, arg_regs[1]).d;
				
				break;
				
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_TH_NEW:
		switch(op.subop){
			case SO_REGISTER:
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				result.t = fork_Thread(th, REGISTER(rc, arg_regs[0]).addr);
				break;
				
			case SO_CONSTANT:
				READ_CONSTANT_ARG(arg_regs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				result.t = fork_Thread(th, REGISTER(rc, arg_regs[0]).addr);
				break;
				
			case SO_FUNCTION:
				READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
				
				result.t = create_child_Thread(th, REGISTER(rc, arg_regs[0]).f->addr);
				load_Function(REGISTER(rc, arg_regs[0]).f, &rc);
				start_Thread(result.t);
				
				break;
				
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_TH_JOIN:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		if(pthread_join(REGISTER(rc, arg_regs[0]).t->tid, NULL)){
			perror("Error: Unable to join thread:");
		}
		
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_TH_KILL:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		REGISTER(rc, arg_regs[0]).t->status = TH_STAT_KILLED;
		
		if (pthread_join(REGISTER(rc, arg_regs[0]).t->tid, NULL)){
			perror("Error: Unable to join thread\n");
		}
		
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
		
	L_XOR:
		READ_REGISTER_ARG(arg_regs, 0, th->prog, pc_next);
		
		switch(op.subop){
			case SO_REGISTER:
				READ_REGISTER_ARG(arg_regs, 1, th->prog, pc_next);
				
				result.b = (REGISTER(rc, arg_regs[0]).b && !REGISTER(rc, arg_regs[1]).b) || (REGISTER(rc, arg_regs[0]).b && !REGISTER(rc, arg_regs[1]).b);
				break;
				
			case SO_CONSTANT:
				READ_CONSTANT_ARG(arg_regs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result.b = (REGISTER(rc, arg_regs[0]).b && !REGISTER(rc, arg_regs[1]).b) || (REGISTER(rc, arg_regs[0]).b && !REGISTER(rc, arg_regs[1]).b);
				break;
			
			default:
				break;
		}
		
		WRITE_OP_RESULT(th->prog, pc_next, rc, result);
		LOAD_NEXT_OPCODE(pc_next, th, op);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, op);
	
	L_END_OF_PROGRAM:
	
	#ifdef DEBUG
	printf("Thread exiting\n");
	if (th->pc >= th->prog_len){
		printf("\nPC(%lu) outside of program bounds\n", th->pc);
	}else{
		printf("Exited with status %d\n", th->status);
	}
	#endif
	
	return NULL;
}

Rivr_String* read_into_string(FILE* fp){
	char buff[1024];
	fgets(buff, 1024, fp);
	int str_len = strlen(buff);
	
	Rivr_String* rv_str = string_create_from_seq(buff, str_len, STRING_NONE);
	#ifdef DEBUG
	printf("Read in string: %s\n", buff);
	string_print(rv_str, stdout);
	putc('\n', stdout);
	#endif
	
	return rv_str;
}

byte read_into_bool(FILE* fp){
	char* c_str;
	int str_len;
	fscanf(fp, "%ms%n", &c_str, &str_len);
	while (getc(fp) != '\n'); // flush input buffer
	
	if (strncmp(c_str, "true", 4) == 0){
		free (c_str);
		return 1;
	}else{
		free (c_str);
		return 0;
	}
}

long int pow_num(long int b, long int e){
	
	if (e <= 0) return 1;
	
	long int result = b;
	while (e > 1){
		result *= result;
		result += result * (e % 2);
		e /= 2;
	}
	
	return result;
}

double pow_rat(double b, double e){
	
	double natural_exp = e * log(b);
	
	#ifdef DEBUG
	printf("\t%lf ^ %lf: natural_exp=%lf\n", b, e, natural_exp);
	#endif
	
	return exp(natural_exp);
}
