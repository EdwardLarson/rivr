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
			printf("Opcode[0x%x]-Subop[0x%x]-", Op.opcode, Op.subop); \
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

#ifdef DEBUG
#define LOAD_NEXT_INSTRUCTION(PC_Next, Thread, Cache, Instr, Arg_Regs) \
	{ if ((Thread->status <= 0) || (PC_Next + INSTR_SIZE >= Thread->prog_len)) goto L_END_OF_PROGRAM; \
			Thread->pc = PC_Next; \
			memcpy(&Instr.bytes, &Thread->prog[PC_Next], INSTR_SIZE); \
			PC_Next += INSTR_SIZE; \
			\
			if(Instr.subop.arg1_const){  \
				memcpy(&Arg_Regs[0], &Thread->prog[PC_Next], sizeof(Data)); \
				PC_Next += sizeof(Data); \
			}else{ \
				memcpy(&Arg_Regs[0], &Cache.all_registers[ Instr.args[0] ], sizeof(Data)); \
			}\
			if(Instr.subop.arg2_const){  \
				memcpy(&Arg_Regs[1], &Thread->prog[PC_Next], sizeof(Data)); \
				PC_Next += sizeof(Data); \
			}else{ \
				memcpy(&Arg_Regs[1], &Cache.all_registers[ Instr.args[1] ], sizeof(Data)); \
			}\
			printf("\tpc[%lu]-", Thread->pc); \
			printf("Opcode[0x%x]-Subop[0x%x]-", Instr.opcode, Instr.subop.full); \
			printf("Status[%d]\n", Thread->status); \
	}
#else
#define LOAD_NEXT_INSTRUCTION(PC_Next, Thread, Cache, Instr, Arg_Regs) \
	{ if ((Thread->status <= 0) || (PC_Next + INSTR_SIZE >= Thread->prog_len)) goto L_END_OF_PROGRAM; \
			Thread->pc = PC_Next; \
			memcpy(&Instr.bytes, &Thread->prog[PC_Next], INSTR_SIZE); \
			PC_Next += INSTR_SIZE; \
			\
			if(Instr.subop.arg1_const){  \
				memcpy(&Arg_Regs[0], &Thread->prog[PC_Next], sizeof(Data)); \
				PC_Next += sizeof(Data); \
			}else{ \
				memcpy(&Arg_Regs[0], &Cache.all_registers[ Instr.args[0] ], sizeof(Data)); \
			}\
			if(Instr.subop.arg2_const){  \
				memcpy(&Arg_Regs[1], &Thread->prog[PC_Next], sizeof(Data)); \
				PC_Next += sizeof(Data); \
			}else{ \
				memcpy(&Arg_Regs[1], &Cache.all_registers[ Instr.args[1] ], sizeof(Data)); \
			}\
	}
#endif
	
#define JUMP_TO_NEXT_INSTRUCTION(ITable, Instruction) \
	goto *(ITable[Instruction.opcode]) 
	
#define REGISTER(Cache, Reg_Address) (Cache.all_registers[Reg_Address])

#define READ_PROG_BYTE(Prog, PC_Next) \
	Prog[(PC_Next)++]
	
#define READ_REGISTER_ARG(Arg_Regs, Arg_N, Prog, PC_Next) \
	{ Arg_Regs[Arg_N] = Prog[PC_Next]; PC_Next += 1; }
	
#define READ_CONSTANT_ARG(Arg_Regs, Arg_N, Prog, PC_Next, Cache, Const_N) \
	{ memcpy(&Cache.s_registers[ Const_N ], &Prog[PC_Next], sizeof(Data)); Arg_Regs[Arg_N] = Const_N; PC_Next += sizeof(Data); }

#define WRITE_OP_RESULT(Instruction, Cache, Result) \
	{ Cache.all_registers[Instruction.ret] = Result; }
	
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

# define LOAD_ARGUMENT(Instr, Iarg, ArgRegs, Cache, Bytes, PC_Next) \
	if(Instr.subop.full & (0x1 << (2 + Iarg))){  \
		memcpy(&ArgRegs[Iarg], &Bytes[PC_Next], sizeof(Data); \
		PC_Next += sizeof(Data); \
	}else{ \
		memcpy(&ArgRegs[Iarg], &Cache.all_registers[Instr.args[Iarg]], sizeof(Data)); \
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
	PCType pc_next;
	
	Register_Cache rc;
	init_Register_Cache(&rc, th->rf->curr_frame);
	
	Instruction instruction;
	
	Data iargs[5];
	Data result;
	
	th->tid = pthread_self();
	th->status = TH_STAT_RUN;
	
	void* instruction_table[] = {
		&&L_ABS,
		&&L_ADD,
		&&L_AND,
		&&L_BRANCH,
		&&L_BW_AND,
		&&L_BW_NOT,
		&&L_BW_OR,
		&&L_BW_XOR,
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
	
	LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
	JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
	
	L_ABS:
		
		switch (instruction.subop.arg_type){
			case SO_NUMBER:
				result.n = (iargs[0].n < 0) ? (-1 * iargs[0].n) : (iargs[0].n);
				break;
				
			case SO_RATIONAL:
				result.d = (iargs[0].n < 0) ? (-1 * iargs[0].d) : (iargs[0].d);
				break;
				
			default:
				break;
				
		}
	
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
		
	L_ADD:
		switch (instruction.subop.arg_type){
			case SO_NUMBER:
				result.n = iargs[0].n + iargs[1].n;
				break;
				
			case SO_RATIONAL:
				result.d = iargs[0].d + iargs[1].d;
				break;
				
			default:
				break;
				
		}
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_AND:	
		result.b = iargs[0].b && iargs[1].b;
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_BRANCH:
		if (iargs[0].b){
			pc_next += sizeof(Data);
			
		}else{
			pc_next = iargs[1].addr;
		}
		
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_BW_AND:
	
		result.n = iargs[0].n & iargs[1].n;
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_BW_NOT:
	
		result.n = ~(iargs[0].n);
	
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_BW_OR:
	
		result.n = iargs[0].n | iargs[1].n;
	
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_BW_XOR:
	
		result.n = iargs[0].n ^ iargs[1].n;
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_DECR:
		result.n = iargs[0].n - 1;
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_DIV:
	
		switch (instruction.subop.arg_type){
			case SO_NUMBER:
				result.n = iargs[0].n / iargs[1].n;
				break;
				
			case SO_RATIONAL:
				result.d = iargs[0].d / iargs[1].d;
				break;
				
			default:
				break;
				
		}
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_EQ:
		switch (instruction.subop.arg_type){
			case SO_NUMBER:
				result.b = iargs[0].n == iargs[1].n;
				break;
				
			case SO_RATIONAL:
				result.b = iargs[0].d == iargs[1].d;
				break;
				
			default:
				break;
				
		}
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_F_CALL: // TODO: REWORK FOR NEW INSTRUCTION FORMAT
		
		/*
		READ_REGISTER_ARG(iargs, 0, th->prog, pc_next);
		
		switch(instruction.subop.full){
				
			case SO_PUSHFIRST:
				push_frame(th, &rc);
				// fall into next case
			case SO_DIRECT:
				pc_next = load_Function(iargs[0].f, &rc);
				break;
				
			default:
				break;
		}
		*/
		
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_F_CREATE: // TODO: REWORK FOR NEW INSTRUCTION FORMAT
		/*
		switch(instruction.subop.full){
			case FORMAT4_SUBOP(SO_NOCLOSURE, SO_RELATIVE):
				READ_CONSTANT_ARG(iargs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				result.f = create_Function(th->pc + iargs[0].addr, 0);
				
				break;
				
			case FORMAT4_SUBOP(SO_NOCLOSURE, SO_ABSOLUTE):
				READ_CONSTANT_ARG(iargs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				result.f = create_Function(iargs[0].addr, 0);
				
				break;
				
			case FORMAT4_SUBOP(SO_CLOSURE, SO_RELATIVE):
				READ_CONSTANT_ARG(iargs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				tmp.b = READ_PROG_BYTE(th->prog, pc_next);
				
				result.f = create_Function(th->pc + iargs[0].addr, (int)tmp.b);
				
				for(int i = 0; i < tmp.b; i++){
					enclose_data_Function(result.f, &REGISTER(rc, th->prog[pc_next]), READ_PROG_BYTE(th->prog, pc_next));
				}
				
				break;
				
			case FORMAT4_SUBOP(SO_CLOSURE, SO_ABSOLUTE):
				READ_CONSTANT_ARG(iargs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				tmp.b = READ_PROG_BYTE(th->prog, pc_next);
				
				result.f = create_Function(iargs[0].addr, (int) tmp.b);
				
				for (int i = 0; i < tmp.b; i++){
					enclose_data_Function(result.f, &REGISTER(rc, th->prog[pc_next]), READ_PROG_BYTE(th->prog, pc_next));
				}
				
				break;
				
			case SO_CLONE_F:
				READ_REGISTER_ARG(iargs, 0, th->prog, pc_next);
				
				tmp.b = READ_PROG_BYTE(th->prog, pc_next);
				
				result.f = copy_Function(iargs[0].f);
				
				for (int i = 0; i < tmp.b; i++){
					enclose_data_Function(result.f, &REGISTER(rc, th->prog[pc_next]), READ_PROG_BYTE(th->prog, pc_next));
				}
				
				break;
		}
		*/
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_GT:
		switch (instruction.subop.arg_type){
			case SO_NUMBER:
				result.b = iargs[0].n > iargs[1].n;
				break;
				
			case SO_RATIONAL:
				result.b = iargs[0].d > iargs[1].d;
				break;
				
			default:
				break;
				
		}
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_HALT:
		#ifdef DEBUG
		printf("\tProgram halted.\n");
		#endif
		th->status = TH_STAT_FIN;
		
		goto L_END_OF_PROGRAM;
		
	L_INCR:
		result.n = iargs[0].n + 1;
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_INPUT:
		
		switch(instruction.subop.arg_type){
			case SO_NUMBER:
				fscanf((FILE*) iargs[0].n, "%ld", &result.n);
				while (getc((FILE*) iargs[0].n) != '\n'); // flush input buffer
				break;
			case SO_RATIONAL:
				fscanf((FILE*) iargs[0].n, "%lf", &result.d);
				while (getc((FILE*) iargs[0].n) != '\n'); // flush input buffer
				break;
			case SO_STRING:
				result.s = read_into_string((FILE*) iargs[0].n);
				break;
			case SO_BOOLEAN:
				result.b = read_into_bool((FILE*) iargs[0].n);
				break;
			default:
				break;
		}
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_JUMP:
	
		switch(instruction.subop.arg_type){
			case SO_ABSOLUTE:
			
				pc_next = iargs[0].addr;
				break;
				
			case SO_RELATIVE:
				
				pc_next += iargs[0].addr;
				break;
				
			default:
				break;
		}
		
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_LSH:
	
		result.n = iargs[0].n << iargs[1].n;
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_LT:
		switch (instruction.subop.arg_type){
			case SO_NUMBER:
				result.b = iargs[0].n < iargs[1].n;
				break;
				
			case SO_RATIONAL:
				result.b = iargs[0].d < iargs[1].d;
				break;
				
			default:
				break;
				
		}
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_M_ALLOC: // TODO: REWORK FOR NEW INSTRUCTION FORMAT
		/*
		switch(instruction.subop.full){
			case SO_REGISTER:
				READ_REGISTER_ARG(iargs, 0, th->prog, pc_next);
				
				result.p = calloc(iargs[0].n, sizeof(Data));
				break;
			case SO_CONSTANT:
				READ_CONSTANT_ARG(iargs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				#ifdef DEBUG
				printf("\tallocating array with length %ld\n", iargs[0].n);
				#endif
				
				result.p = calloc(iargs[0].n, sizeof(Data));
				break;
			default:
				break;
		}
		*/
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_M_FREE: // TODO: REWORK FOR NEW INSTRUCTION FORMAT
		/*
		READ_REGISTER_ARG(iargs, 0, th->prog, pc_next);
		
		switch(instruction.subop.full){
			case SO_OBJECT:
				free(iargs[0].p);
				break;
				
			case SO_STRING:
				string_destroy(iargs[0].s);
				break;
				
			case SO_THREAD:
				teardown_Thread(iargs[0].t);
				break;
				
			case SO_FUNCTION:
				teardown_Function(iargs[0].f);
				break;
				
			default:
				break;
		}
		*/
		
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_M_LOAD: // TODO: REWORK FOR NEW INSTRUCTION FORMAT
		/*
		READ_REGISTER_ARG(iargs, 0, th->prog, pc_next);
		
		switch(instruction.subop.full){
			case SO_REGISTER:
				READ_REGISTER_ARG(iargs, 1, th->prog, pc_next);
				
				result = ( (Data*) iargs[0].p )[ iargs[1].n ];
				
				break;
				
			case SO_CONSTANT:
				READ_CONSTANT_ARG(iargs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				result = ( (Data*) iargs[0].p )[ iargs[1].n ];
				
				break;
			
			default:
				break;
		}
		*/
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_M_STORE: // TODO: REWORK FOR NEW INSTRUCTION FORMAT
		/*
		READ_REGISTER_ARG(iargs, 0, th->prog, pc_next);
		
		switch(instruction.subop.full){
			case FORMAT1_SUBOP(SO_NONE, SO_REGISTER, SO_REGISTER, SO_NONE):
				READ_REGISTER_ARG(iargs, 1, th->prog, pc_next);
				
				READ_REGISTER_ARG(iargs, 2, th->prog, pc_next);
				
				( (Data*) iargs[0].p )[ iargs[2].n ] = iargs[1];
				
				break;
				
			case FORMAT1_SUBOP(SO_NONE, SO_REGISTER, SO_CONSTANT, SO_NONE):
				READ_REGISTER_ARG(iargs, 1, th->prog, pc_next);
				
				READ_CONSTANT_ARG(iargs, 2, th->prog, pc_next, rc, CONST_REG_0);
				
				#ifdef DEBUG
				printf("\tpreparing to store to %p with offset %ld\n", iargs[0].p, iargs[2].n);
				#endif
				
				( (Data*) iargs[0].p )[ iargs[2].n ] = iargs[1];
				
				#ifdef DEBUG
				printf("\tstored\n");
				#endif
				
				break;
				
			case FORMAT1_SUBOP(SO_NONE, SO_CONSTANT, SO_REGISTER, SO_NONE):
				READ_CONSTANT_ARG(iargs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_REGISTER_ARG(iargs, 2, th->prog, pc_next);
				
				( (Data*) iargs[0].p )[ (size_t) iargs[2].n ] = iargs[1];
				
				break;
				
			case FORMAT1_SUBOP(SO_NONE, SO_CONSTANT, SO_CONSTANT, SO_NONE):
				READ_CONSTANT_ARG(iargs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				READ_CONSTANT_ARG(iargs, 2, th->prog, pc_next, rc, CONST_REG_1);
				
				( (Data*) iargs[0].p )[ iargs[2].n ] = iargs[1];
				
				break;
			
			default:
				break;
		}
		*/
		
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_MOD:
		result.n = iargs[0].n % iargs[1].n;
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_MOVE:
		result = iargs[0];
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_MUL:
	
		switch (instruction.subop.arg_type){
			case SO_NUMBER:
				result.n = iargs[0].n * iargs[1].n;
				break;
				
			case SO_RATIONAL:
				result.d = iargs[0].d * iargs[1].d;
				break;
				
			default:
				break;
				
		}
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_NOT:
		
		result.b = !(iargs[0].b);
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_NOOP:
		
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_OR:
		
		result.b = iargs[0].b || iargs[1].b;
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_OUTPUT: // TODO: REWORK FOR NEW INSTRUCTION FORMAT
	
		switch(instruction.subop.arg_type){
			case SO_NUMBER:
				fprintf((FILE*) iargs[0].n, "%ld", iargs[1].n);
				break;
				
			case SO_RATIONAL:
				fprintf((FILE*) iargs[0].n, "%f", iargs[1].d);
				break;
				
			case SO_STRING:
				string_print(iargs[1].s, (FILE*) iargs[0].n);
				break;
				
			case SO_BOOLEAN:
				if (iargs[1].b){
					fprintf((FILE*) iargs[0].n, "True");
				}else{
					fprintf((FILE*) iargs[0].n, "False");
				}
				break;
			
			
		}
	
		/*
		READ_REGISTER_ARG(iargs, 0, th->prog, pc_next);
		
		switch(instruction.subop.full){
			case FORMAT2_SUBOP(SO_REGISTER, SO_NUMBER):
			
				READ_REGISTER_ARG(iargs, 1, th->prog, pc_next);
				
				fprintf((FILE*) iargs[0].n, "%ld", iargs[1].n);
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_RATIONAL):
			
				READ_REGISTER_ARG(iargs, 1, th->prog, pc_next);
				
				fprintf((FILE*) iargs[0].n, "%f", iargs[1].d);
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_OBJECT):
			
				READ_REGISTER_ARG(iargs, 1, th->prog, pc_next);
				
				fprintf((FILE*) iargs[0].n, "obj<%p>", iargs[1].p);
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_STRING):
			
				READ_REGISTER_ARG(iargs, 1, th->prog, pc_next);
				
				string_print(iargs[1].s, (FILE*) iargs[0].n);
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_THREAD):
			
				READ_REGISTER_ARG(iargs, 1, th->prog, pc_next);
				
				fprintf((FILE*) iargs[0].n, "thr<%p>", iargs[1].t);
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_FUNCTION):
				READ_REGISTER_ARG(iargs, 1, th->prog, pc_next);
				
				fprintf((FILE*) iargs[0].n, "fun<%p>{%d enclosed}", iargs[1].f, iargs[1].f->n_enclosed);
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_BOOLEAN):
			
				READ_REGISTER_ARG(iargs, 1, th->prog, pc_next);
				
				if (iargs[1].b){
					fprintf((FILE*) iargs[0].n, "True");
				}else{
					fprintf((FILE*) iargs[0].n, "False");
				}
				break;
				
			case FORMAT2_SUBOP(SO_REGISTER, SO_HASHTABLE):
				
				READ_REGISTER_ARG(iargs, 1, th->prog, pc_next);
				
				fprintf((FILE*) iargs[0].n, "htb<%p>", iargs[1].h);
				break;
				
			case FORMAT2_SUBOP(SO_CONSTANT, SO_NUMBER):
				
				READ_CONSTANT_ARG(iargs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) iargs[0].n, "%ld", iargs[1].n);
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_RATIONAL):
				
				READ_CONSTANT_ARG(iargs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) iargs[0].n, "%f", iargs[1].d);
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_OBJECT):
				
				READ_CONSTANT_ARG(iargs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) iargs[0].n, "obj<%p>", iargs[1].p);
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_STRING):
				
				READ_CONSTANT_ARG(iargs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) iargs[0].n, "%.*s", 8, iargs[1].bytes);
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_THREAD):
				
				READ_CONSTANT_ARG(iargs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) iargs[0].n, "thr<%p>", iargs[1].t);
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_FUNCTION):
				
				READ_CONSTANT_ARG(iargs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) iargs[0].n, "fun<%lx>", iargs[1].addr);
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_BOOLEAN):
				
				READ_CONSTANT_ARG(iargs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				if (iargs[1].b){
					fprintf((FILE*) iargs[0].n, "True");
				}else{
					fprintf((FILE*) iargs[0].n, "False");
				}
				break;
			case FORMAT2_SUBOP(SO_CONSTANT, SO_HASHTABLE):
				
				READ_CONSTANT_ARG(iargs, 1, th->prog, pc_next, rc, CONST_REG_0);
				
				fprintf((FILE*) iargs[0].n, "htb<%p>", iargs[1].h);
				break;
			
			default:
				break;
		}
		
		*/
		
		#ifdef DEBUG
		printf("\n\tprint accomplished\n");
		#endif
		
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_POPFRAME:
		pop_frame(th, &rc);
		
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_PUSHFRAME:
		push_frame(th, &rc);
		
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_POW:
		// Note: Power function is undefined for negative exponents
		
		switch (instruction.subop.arg_type){
			case SO_NUMBER:
				result.n = pow_num(iargs[0].n, iargs[1].n);
				break;
				
			case SO_RATIONAL:
				result.d = pow_rat(iargs[0].d, iargs[1].d);
				break;
				
			default:
				break;
				
		}
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_RSH: 
		result.n = iargs[0].n >> iargs[1].n;
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_SUB:
		
		switch (instruction.subop.arg_type){
			case SO_NUMBER:
				result.n = iargs[0].n - iargs[1].n;
				break;
				
			case SO_RATIONAL:
				result.d = iargs[0].d - iargs[1].d;
				break;
				
			default:
				break;
				
		}
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_TH_NEW: // TODO: REWORK FOR NEW INSTRUCTION FORMAT
		/*
		switch(instruction.subop.full){
			case SO_REGISTER:
				READ_REGISTER_ARG(iargs, 0, th->prog, pc_next);
				
				result.t = fork_Thread(th, iargs[0].addr);
				break;
				
			case SO_CONSTANT:
				READ_CONSTANT_ARG(iargs, 0, th->prog, pc_next, rc, CONST_REG_0);
				
				result.t = fork_Thread(th, iargs[0].addr);
				break;
				
			case SO_FUNCTION:
				
				READ_REGISTER_ARG(iargs, 0, th->prog, pc_next);
				
				result.t = create_child_Thread(th, iargs[0].f->addr);
				load_Function(iargs[0].f, &rc);
				start_Thread(result.t);
				
				break;
				
			default:
				break;
		}
		*/
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_TH_JOIN: // TODO: REWORK FOR NEW INSTRUCTION FORMAT
		/*
		READ_REGISTER_ARG(iargs, 0, th->prog, pc_next);
		
		if(pthread_join(iargs[0].t->tid, NULL)){
			perror("Error: Unable to join thread:");
		}
		*/
		
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_TH_KILL: // TODO: REWORK FOR NEW INSTRUCTION FORMAT
		/*
		READ_REGISTER_ARG(iargs, 0, th->prog, pc_next);
		
		iargs[0].t->status = TH_STAT_KILLED;
		
		if (pthread_join(iargs[0].t->tid, NULL)){
			perror("Error: Unable to join thread\n");
		}
		*/
		
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
		
	L_XOR:
		result.b = (iargs[0].b && !iargs[1].b) || (iargs[0].b && !iargs[1].b);
		
		WRITE_OP_RESULT(instruction, rc, result);
		LOAD_NEXT_INSTRUCTION(pc_next, th, rc, instruction, iargs);
		JUMP_TO_NEXT_INSTRUCTION(instruction_table, instruction);
	
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
