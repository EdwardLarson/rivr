#include "vm.h"

#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include <errno.h>

#ifdef VM_ONLY
int main(int argc, char** argv){
	printf("Hello World!\n");
	
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
	
	op.bytes[0] = bytes[pc];
	op.bytes[1] = bytes[pc + 1];
	
	return op;
}

byte get_opcode(Operation op){
	// first 6 bits (bits 0 to 6)
	return (op.bytes[0] & 0xFC) >> 2;
}

byte get_subop(Operation op){
	// bits 6 to 9
	return ((op.bytes[0] & 0x03) << 2) | ((op.bytes[1] & 0xC0) >> 6);
}

Operation encode_operation(byte opcode, byte subop){
	Operation op;
	op.bytes[0] = 0xFC & (opcode << 2);
	op.bytes[0] |= 0x03 & (subop >> 2);
	op.bytes[1] = 0xC0 & (subop << 6);
	
	return op;
}


int init_Register_File(Register_File* rf){
	int i;
	
	rf->prev_frame = NULL;
	rf->curr_frame = NULL;
	rf->next_frame = NULL;
	
	rf->references = 0;
	
	return 0;
}

Register_Frame* next_free_frame(Register_File* rf){
	
	return NULL;
}

Register_Frame* alloc_frame(Register_File* rf, Register_Frame* prv){
	Register_Frame* frame = calloc(1, sizeof(Register_Frame));
	frame->prv_frame = prv;
	frame->nxt_frame = NULL;
	
	return frame;
}

void dealloc_frames(Register_Frame* init_frame, Register_File* rf){
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
	
	th->rf->prev_frame = alloc_frame(rf, NULL);
	th->rf->curr_frame = alloc_frame(rf, th->rf->prev_frame);
	th->rf->next_frame = alloc_frame(rf, th->rf->curr_frame);
	
	th->rf->curr_frame->nxt_frame = th->rf->next_frame;
	
	
	init_spec_registers(rf->register_cache.s_registers);
	
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
	dealloc_frames(th->rf->curr_frame, th->rf);
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

void push_frame(Thread* th){
	
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
	src = th->rf->register_cache.v_registers;
	dest = original_curr_frame->v_registers;
	width = 64;
	memcpy(dest, src, width * sizeof(Data));
	
	// copy arg_read registers
	src = th->rf->register_cache.a_read_registers;
	dest = original_curr_frame->a_read_registers;
	width = 32;
	memcpy(dest, src, width * sizeof(Data));
	
	// copy ret_read registers
	src = th->rf->register_cache.r_read_registers;
	dest = original_curr_frame->r_read_registers;
	width = 32;
	memcpy(dest, src, width * sizeof(Data));
	
	// copy ret_write_registers
	src = th->rf->register_cache.r_write_registers;
	dest = original_curr_frame->r_write_registers;
	width = 32;
	memcpy(dest, src, width * sizeof(Data));
	
	// copy written args to arg_read registers
	src = th->rf->register_cache.a_write_registers;
	dest = th->rf->register_cache.a_read_registers;
	width = 32;
	memcpy(dest, src, width * sizeof(Data));
	
	th->rf->prev_frame = original_curr_frame;
	th->rf->curr_frame = original_next_frame;
	if (th->rf->curr_frame->nxt_frame == NULL){
		th->rf->curr_frame->nxt_frame = alloc_frame(th->rf, th->rf->curr_frame);
	}
	th->rf->next_frame = th->rf->curr_frame->nxt_frame;
	
	#ifdef DEBUG
	printf("push_frame: a_prev<%p>, a_curr<%p>, a_next<%p>\n", 
		th->rf->prev_frame,
	    th->rf->curr_frame,
	    th->rf->next_frame);
	#endif
}

void pop_frame(Thread* th){
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
	dest = th->rf->register_cache.v_registers;
	width = 64;
	memcpy(dest, src, width * sizeof(Data));
	
	// copy arg_read registers
	src = original_prev_frame->a_read_registers;
	dest = th->rf->register_cache.a_read_registers;
	width = 32;
	memcpy(dest, src, width * sizeof(Data));
	
	// copy ret_read registers
	src = original_prev_frame->r_read_registers;
	dest = th->rf->register_cache.r_read_registers;
	width = 32;
	memcpy(dest, src, width * sizeof(Data));
	
	// copy written returns to ret_read registers
	src = th->rf->register_cache.r_write_registers;
	dest = th->rf->register_cache.r_read_registers;
	width = 32;
	memcpy(dest, src, width * sizeof(Data));
	
	// copy ret_write_registers
	src = original_prev_frame->r_write_registers;
	dest = th->rf->register_cache.r_write_registers;
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

Data* access_register(PCType pc, const Thread* th){
	
	byte r = th->prog[pc];
	
	return &(th->rf->register_cache.all_registers[r]);
	
}

Data access_constant(PCType pc, const Thread* th){
	Data data;
	if (pc + sizeof(Data) >= th->prog_len){
		// error
	}else{
		union {Data data; byte bytes[sizeof(Data)];} data_union;
		int i;
		for (i = 0; i < sizeof(Data); i++){
			data_union.bytes[i] = th->prog[pc + i];
		}
		
		data = data_union.data;
	}
	
	return data;
}

byte read_byte(PCType pc, const Thread* th){
	return th->prog[pc];
}

void* run_thread(void* th_in){
	Thread* th = (Thread*) th_in;
	Operation op;
	byte opcode;
	byte subop;
	PCType pc_next;
	
	Data args[4]; // temporary storage for up to 4 args
	Data result;
	
	th->tid = pthread_self();
	th->status = TH_STAT_RUN;
	
	while ((th->status > 0) && (th->pc < th->prog_len)){
		
		CLEAR_DATA(result);
		
		pc_next = th->pc;
		
		op = read_op(th->prog, th->pc);
		opcode = get_opcode(op);
		subop = get_subop(op);
		
		#ifdef DEBUG
		printf("\tpc[%lu]-", th->pc);
		printf("Opcode[%x]-Subop[%x]-", opcode, subop);
		printf("Status[%d]\n", th->status);
		#endif
		
		pc_next += 2;
		
		switch(opcode){
			
		case I_ABS:
			switch(subop){
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_NONE, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					result.n = (args[0].n < 0) ? (-1 * args[0].n) : (args[0].n);
					
					break;
				case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_NONE, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					result.n = (args[0].n < 0) ? (-1 * args[0].n) : (args[0].n);
					
					break;
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_NONE, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					result.d = (args[0].d < 0) ? (-1 * args[0].d) : (args[0].d);
					
					break;
				case FORMAT1_SUBOP(SO_RATIONAL, SO_CONSTANT, SO_NONE, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					result.d = (args[0].d < 0) ? (-1 * args[0].d) : (args[0].d);
					
					break;
				default:
					break;
			}
			
			*access_register(pc_next, th) = result;
			
			break;
			
		case I_ADD:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
					
			switch(subop){
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n + args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.n = args[0].n + args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.d = args[0].d + args[1].d;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.d = args[0].d + args[1].d;
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_AND:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			switch(subop){
				case SO_REGISTER:
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.b = args[0].b && args[1].b;
					break;
					
				case SO_CONSTANT:
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.b = args[0].b && args[1].b;
					break;
				
				default:
					break;
			}
			break;
			
		case I_BITWISE:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			switch(subop){
				case FORMAT3_SUBOP(SO_REGISTER, SO_AND):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n & args[1].n;
					break;
					
				case FORMAT3_SUBOP(SO_REGISTER, SO_NOT):
					
					result.n = ~(args[0].n);
					break;
				
				case FORMAT3_SUBOP(SO_REGISTER, SO_OR):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n | args[1].n;
					break;
					
				case FORMAT3_SUBOP(SO_REGISTER, SO_XOR):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n ^ args[1].n;
					break;
					
				default:
					break;
			}
			break;
			
		case I_BRANCH:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			if (args[0].b){
				pc_next += sizeof(Data);
			}else{
				args[1] = access_constant(pc_next, th);
				pc_next = args[1].addr;
			}
			break;
			
		case I_DECR:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			result.n = args[0].n - 1;
			
			break;
			
		case I_DIV:
			switch(subop){
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
				pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n / args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.n = args[0].n / args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n / args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.d = args[0].d / args[1].d;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.d = args[0].d / args[1].d;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.d = args[0].d / args[1].d;
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_EQ:
			switch(subop){
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.b = args[0].n == args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.b = args[0].n == args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.b = args[0].d == args[1].d;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.b = args[0].d == args[1].d;
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_F_CALL:
			
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			switch(subop){
					
				case SO_PUSHFIRST:
					push_frame(th);
					// fall into next case
				case SO_DIRECT:
					pc_next = load_Function(args[0].f, th);
					break;
					
				default:
					break;
			}
			
			break;
			
		case I_F_CREATE:
			switch(subop){
				case FORMAT4_SUBOP(SO_NOCLOSURE, SO_RELATIVE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.f = create_Function(th->pc + args[0].addr, 0);
					
					break;
					
				case FORMAT4_SUBOP(SO_NOCLOSURE, SO_ABSOLUTE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					result.f = create_Function(args[0].addr, 0);
					
					break;
					
				case FORMAT4_SUBOP(SO_CLOSURE, SO_RELATIVE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1].b = read_byte(pc_next, th);
					pc_next += 1;
					
					result.f = create_Function(th->pc + args[0].addr, (int) args[1].b);
					
					for(int i = 0; i < args[1].b; i++){
						enclose_data_Function(result.f, access_register(pc_next, th), read_byte(pc_next, th));
						pc_next += 1;
					}
					
					break;
					
				case FORMAT4_SUBOP(SO_CLOSURE, SO_ABSOLUTE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					args[1].b = read_byte(pc_next, th);
					pc_next += 1;
					
					result.f = create_Function(args[0].addr, (int) args[1].b);
					
					
					for (int i = 0; i < args[1].b; i++){
						enclose_data_Function(result.f, access_register(pc_next, th), read_byte(pc_next, th));
						pc_next += 1;
					}
					
					break;
					
				case SO_CLONE_F:
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					
					args[1].b = read_byte(pc_next, th);
					pc_next += 1;
					
					result.f = copy_Function(args[0].f);
					
					for (int i = 0; i < args[1].b; i++){
						enclose_data_Function(result.f, access_register(pc_next, th), read_byte(pc_next, th));
						pc_next += 1;
					}
					
					break;
			}
			break;
			
		case I_GT:
			switch(subop){
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.b = args[0].n > args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.b = args[0].n > args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1] = *access_register(pc_next, th);
					pc_next += sizeof(1);
					
					result.b = args[0].n > args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.b = args[0].d > args[1].d;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.b = args[0].d > args[1].d;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1] = *access_register(pc_next, th);
					pc_next += sizeof(1);
					
					result.b = args[0].d > args[1].d;
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_HALT:
			#ifdef DEBUG
			printf("\tProgram halted.\n");
			#endif
			th->status = TH_STAT_FIN;
			break;
			
		case I_INCR:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			result.n = args[0].n + 1;
			break;
			
		case I_INPUT:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			switch(subop){
				case SO_NUMBER:
					fscanf((FILE*) args[0].n, "%ld", &result.n);
					while (getc((FILE*) args[0].n) != '\n'); // flush input buffer
					break;
				case SO_RATIONAL:
					fscanf((FILE*) args[0].n, "%lf", &result.d);
					while (getc((FILE*) args[0].n) != '\n'); // flush input buffer
					break;
				case SO_STRING:
					result.s = read_into_string((FILE*) args[0].n);
					break;
				case SO_BOOLEAN:
					result.b = read_into_bool((FILE*) args[0].n);
					break;
				default:
					break;
			}
			break;
			
		case I_JUMP:
		
			switch(subop){
				case FORMAT3_SUBOP(SO_CONSTANT, SO_ABSOLUTE):
					#ifdef DEBUG
					printf("Constant jump subop\n");
					#endif
					
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					pc_next = args[0].addr;
					break;
					
				case FORMAT3_SUBOP(SO_CONSTANT, SO_RELATIVE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					pc_next += args[0].addr;
					break;
					
				case FORMAT3_SUBOP(SO_REGISTER, SO_ABSOLUTE):
					#ifdef DEBUG
					printf("Register jump subop\n");
					#endif
				
				
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					pc_next = args[0].addr;
					break;
					
				case FORMAT3_SUBOP(SO_REGISTER, SO_RELATIVE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					pc_next += args[0].addr;
					break;
				
				default:
					break;
			}
			break;
			
		case I_LSH:
			switch(subop){
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
				pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n << args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.n = args[0].n << args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n << args[1].n;
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_LT:
			switch(subop){
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.b = args[0].n < args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.b = args[0].n < args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1] = *access_register(pc_next, th);
					pc_next += sizeof(1);
					
					result.b = args[0].n < args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.b = args[0].d < args[1].d;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.b = args[0].d < args[1].d;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1] = *access_register(pc_next, th);
					pc_next += sizeof(1);
					
					result.b = args[0].d < args[1].d;
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_M_ALLOC:
			switch(subop){
				case SO_REGISTER:
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.p = calloc(args[0].n, sizeof(Data));
					break;
				case SO_CONSTANT:
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					#ifdef DEBUG
					printf("\tallocating array with length %ld\n", args[0].n);
					#endif
					
					result.p = calloc(args[0].n, sizeof(Data));
					break;
				default:
					break;
			}
			break;
			
		case I_M_FREE:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			switch(subop){
				case SO_OBJECT:
					free(args[0].p);
					break;
					
				case SO_STRING:
					string_destroy(args[0].s);
					break;
					
				case SO_THREAD:
					teardown_Thread(args[0].t);
					break;
					
				case SO_FUNCTION:
					teardown_Function(args[0].f);
					break;
					
				default:
					break;
			}
			
			break;
			
		case I_M_LOAD:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			switch(subop){
				case SO_REGISTER:
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result = ( (Data*) args[0].p )[ args[1].n ];
					
					break;
					
				case SO_CONSTANT:
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result = ( (Data*) args[0].p )[ args[1].n ];
					
					break;
				
				default:
					break;
			}
			break;
			
		case I_M_STORE:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			switch(subop){
				case FORMAT1_SUBOP(SO_NONE, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					args[2] = *access_register(pc_next, th);
					pc_next += 1;
					
					( (Data*) args[0].p )[ args[2].n ] = args[1];
					
					break;
					
				case FORMAT1_SUBOP(SO_NONE, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					args[2] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					#ifdef DEBUG
					printf("\tpreparing to store to %p with offset %ld\n", args[0].p, args[2].n);
					#endif
					
					( (Data*) args[0].p )[ args[2].n ] = args[1];
					
					#ifdef DEBUG
					printf("\tstored\n");
					#endif
					
					break;
					
				case FORMAT1_SUBOP(SO_NONE, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[2] = *access_register(pc_next, th);
					pc_next += 1;
					
					( (Data*) args[0].p )[ (size_t) args[2].n ] = args[1];
					
					break;
					
				case FORMAT1_SUBOP(SO_NONE, SO_CONSTANT, SO_CONSTANT, SO_NONE):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[2] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					( (Data*) args[0].p )[ args[2].n ] = args[1];
					
					break;
				
				default:
					break;
			}
			break;
			
		case I_MOD:
			switch(subop){
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
				pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n % args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.n = args[0].n % args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n % args[1].n;
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_MOVE:
			switch(subop){
				case SO_REGISTER:
					result = *access_register(pc_next, th);
					pc_next += 1;
					
					break;
				case SO_CONSTANT:
					result = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_MUL:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
					
			switch(subop){
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n * args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.n = args[0].n * args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.d = args[0].d * args[1].d;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.d = args[0].d * args[1].d;
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_NOT:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			result.b = !(args[0].b);
			
			break;
			
		case I_NOOP:
			
			#ifdef DEBUG
			printf("\tNo-op, pc_next = %lu\n", pc_next);
			#endif
		
			switch(subop){
				#ifdef DEBUG
				case SO_NUMBER:
					printf("\t\tnumber subop\n");
					break;
				case SO_HASHTABLE:
					printf("\t\thashtable subop\n");
					break;
				#endif
				default:
					break;
			}
			
			break;
			
		case I_OR:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			switch(subop){
				case SO_REGISTER:
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.b = args[0].b || args[1].b;
					break;
					
				case SO_CONSTANT:
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.b = args[0].b || args[1].b;
					break;
				
				default:
					break;
			}
			break;
			
		case I_OUTPUT:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			switch(subop){
				case FORMAT2_SUBOP(SO_REGISTER, SO_NUMBER):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					fprintf((FILE*) args[0].n, "%ld", args[1].n);
					break;
				case FORMAT2_SUBOP(SO_REGISTER, SO_RATIONAL):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					fprintf((FILE*) args[0].n, "%f", args[1].d);
					break;
				case FORMAT2_SUBOP(SO_REGISTER, SO_OBJECT):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					fprintf((FILE*) args[0].n, "obj<%p>", args[1].p);
					break;
				case FORMAT2_SUBOP(SO_REGISTER, SO_STRING):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					string_print(args[1].s, (FILE*) args[0].n);
					break;
				case FORMAT2_SUBOP(SO_REGISTER, SO_THREAD):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					fprintf((FILE*) args[0].n, "thr<%p>", args[1].t);
					break;
				case FORMAT2_SUBOP(SO_REGISTER, SO_FUNCTION):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					fprintf((FILE*) args[0].n, "fun<%p>{%d enclosed}", args[1].f, args[1].f->n_enclosed);
					break;
				case FORMAT2_SUBOP(SO_REGISTER, SO_BOOLEAN):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					if (args[1].b){
						fprintf((FILE*) args[0].n, "True");
					}else{
						fprintf((FILE*) args[0].n, "False");
					}
					break;
				case FORMAT2_SUBOP(SO_REGISTER, SO_HASHTABLE):
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					fprintf((FILE*) args[0].n, "htb<%p>", args[1].h);
					break;
					
				case FORMAT2_SUBOP(SO_CONSTANT, SO_NUMBER):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					fprintf((FILE*) args[0].n, "%ld", args[1].n);
					break;
				case FORMAT2_SUBOP(SO_CONSTANT, SO_RATIONAL):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					fprintf((FILE*) args[0].n, "%f", args[1].d);
					break;
				case FORMAT2_SUBOP(SO_CONSTANT, SO_OBJECT):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					fprintf((FILE*) args[0].n, "obj<%p>", args[1].p);
					break;
				case FORMAT2_SUBOP(SO_CONSTANT, SO_STRING):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					fprintf((FILE*) args[0].n, "%.*s", 8, args[1].bytes);
					break;
				case FORMAT2_SUBOP(SO_CONSTANT, SO_THREAD):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					fprintf((FILE*) args[0].n, "thr<%p>", args[1].t);
					break;
				case FORMAT2_SUBOP(SO_CONSTANT, SO_FUNCTION):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					fprintf((FILE*) args[0].n, "fun<%lx>", args[1].addr);
					break;
				case FORMAT2_SUBOP(SO_CONSTANT, SO_BOOLEAN):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					if (args[1].b){
						fprintf((FILE*) args[0].n, "True");
					}else{
						fprintf((FILE*) args[0].n, "False");
					}
					break;
				case FORMAT2_SUBOP(SO_CONSTANT, SO_HASHTABLE):
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					fprintf((FILE*) args[0].n, "htb<%p>", args[1].h);
					break;
				
				default:
					break;
			}
			
			#ifdef DEBUG
			printf("\n\tprint accomplished\n");
			#endif
			break;
			
		case I_POPFRAME:
			pop_frame(th);
			break;
			
		case I_PUSHFRAME:
			push_frame(th);
			break;
			
		case I_POW:
			// Note: Power function is undefined for negative exponents
			switch(subop){
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
				pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = pow_num(args[0].n, args[1].n);
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.n = pow_num(args[0].n, args[1].n);
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = pow_num(args[0].n, args[1].n);
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.d = pow_rat(args[0].d, args[1].d);
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.d = pow_rat(args[0].d, args[1].d);
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.d = pow_rat(args[0].d, args[1].d);
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_RSH:
			switch(subop){
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
				pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n >> args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.n = args[0].n >> args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n >> args[1].n;
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_SUB:
			switch(subop){
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
				pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n - args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.n = args[0].n - args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n - args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.d = args[0].d - args[1].d;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.d = args[0].d - args[1].d;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_CONSTANT, SO_REGISTER, SO_NONE):
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.d = args[0].d - args[1].d;
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_TH_NEW:
			switch(subop){
				case SO_REGISTER:
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.t = fork_Thread(th, args[0].addr);
					break;
					
				case SO_CONSTANT:
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.t = fork_Thread(th, args[0].addr);
					break;
					
				case SO_FUNCTION:
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.t = create_child_Thread(th, args[0].f->addr);
					load_Function(args[0].f, result.t);
					start_Thread(result.t);
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_TH_JOIN:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			if(pthread_join(args[0].t->tid, NULL)){
				perror("Error: Unable to join thread:");
			}
			
			break;
			
		case I_TH_KILL:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			args[0].t->status = TH_STAT_KILLED;
			
			if (pthread_join(args[0].t->tid, NULL)){
				perror("Error: Unable to join thread\n");
			}
			
			break;
			
		case I_XOR:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			switch(subop){
				case SO_REGISTER:
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.b = (args[0].b && !args[1].b) || (!args[0].b && args[1].b);
					break;
					
				case SO_CONSTANT:
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.b = (args[0].b && !args[1].b) || (!args[0].b && args[1].b);
					break;
				
				default:
					break;
			}
			break;
			
		default:
			printf("Rivr Error: Unknown opcode\n");
			#ifdef DEBUG
			printf("\tInvalid opcode %x\t", opcode);
			#endif
		
		}
		
		if (HAS_RETURN(opcode)){
			#ifdef DEBUG
			printf("\tWriting result of <%x> to register\n", opcode);
			#endif
			*access_register(pc_next, th) = result;
			pc_next += 1;
		}
		
		
		th->pc = pc_next;
	}
	
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
