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
	for (i = 0; i < FRAME_STACK_SIZE; i++){
		rf->frames[i].nxt_frame = NULL;
		rf->frames[i].prv_frame = NULL;
		rf->frames[i].used = 0;
	}
	
	rf->references = 0;
	
	return 0;
}

Register_Frame* next_free_frame(Register_File* rf){
	int i;
	for (i = 0; i < FRAME_STACK_SIZE; i++){
		if (!(rf->frames[i].used)){
			return &(rf->frames[i]);
		}
	}
	
	return NULL;
}

Register_Frame* alloc_frame(Register_File* rf, Register_Frame* prv){
	Register_Frame* frame = next_free_frame(rf);
	
	if (!frame){
		#ifdef DEBUG
		printf("\tWarning: dynamic frame allocation\n");
		#endif
		
		frame = malloc(sizeof(Register_Frame));
		if (!frame){
			// out of frame stack space and out of heap memory
			// TO-DO: ERROR out of memory
			return NULL;
		}
	}
	
	frame->used = 1;
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
	while(init_frame){
		tmp = init_frame->nxt_frame;
		if ( (init_frame >= rf->frames) && (init_frame < &(rf->frames[FRAME_STACK_SIZE - 1]) ) ){
			// frame is within bounds of register file's frame stack
			// no need to deallocate, will be freed when register file is
		}else{
			free(init_frame);
		}
		init_frame = tmp;
	}
}

void init_Thread(Thread* th, Register_File* rf, const byte* prog, PCType prog_len, PCType pc_start){
	th->rf = rf;
	th->rf->references += 1;
	
	th->frame = alloc_frame(rf, NULL);
	th->frame->nxt_frame = alloc_frame(rf, th->frame);
	
	init_spec_registers(rf->s_registers);
	
	th->prog = prog;
	th->prog_len = prog_len;
	th->pc = pc_start;
	
	th->status = TH_STAT_RDY;
	
	th->tid = pthread_self();
}

void init_spec_registers(Data* registers){
	Data data;
	
	// $!0 = 0
	data.n = 0;
	registers[0] = data;
	
	// $!1 = 1
	data.n = 1;
	registers[1] = data;
	
	// $!2 = stdout
	data.n = (long int) stdout;
	registers[2] = data;
	
	// $!3 = stdin
	data.n = (long int) stdin;
	registers[3] = data;
	
	// $!3 = stderr
	data.n = (long int) stderr;
	registers[3] = data;
}

Thread* fork_Thread(const Thread* parent, PCType pc_start){
	Thread* th = malloc(sizeof(Thread));
	
	th->rf = parent->rf;
	th->rf->references += 1;
	
	th->frame = alloc_frame(th->rf, NULL);
	th->frame->nxt_frame = alloc_frame(th->rf, th->frame);
	
	th->prog = parent->prog;
	th->pc = pc_start;
	th->prog_len = parent->prog_len;
	
	th->status = TH_STAT_RDY;
	
	int err = pthread_create(&th->tid, NULL, run_thread, th);
	if (err){
		printf("Error: Unable to create thread\n");
		return NULL;
	}
	
	return th;
}

void teardown_Thread(Thread* th){
	dealloc_frames(th->frame, th->rf);
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
	th->frame = th->frame->nxt_frame;
	
	if (!th->frame->nxt_frame) 
		th->frame->nxt_frame = alloc_frame(th->rf, th->frame);
}

void pop_frame(Thread* th){
	if (!th->frame->prv_frame){
		//TO-DO: Error when unable to pop further (stack underflow)
		#ifdef DEBUG
		printf("/tStack underflow!\n");
		#endif
	}else{
		th->frame->used = 0;
		th->frame = th->frame->prv_frame;
	}
}

Data* access_register(PCType pc, const Thread* th){
	
	byte r = th->prog[pc];
	
	// get first 3 bits for register type
	byte r_type = (r & 0xE0) >> 5;
	
	// get last 5 bits for register number
	byte n = (r & 0x1F);
	
	switch (r_type){
		
	case 0x00:
	case 0x01:
		// variable register
		n = (r & 0x3F); // twice as many variable registers as other types, so one more bit is needed to store their n
		
		return &(th->frame->v_registers[n]);
		break;
	case 0x02:
		// argument read register
		return &(th->frame->a_registers[n]);
		break;
	case 0x03:
		// return read register
		return &(th->frame->r_registers[n]);
		break;
	case 0x04:
		// argument virtual write register
		
		if (th->frame->nxt_frame){
			return &(th->frame->nxt_frame->a_registers[n]);
		}else{
			// TO-DO: ERROR when attempting to write to next frame from last frame
			return NULL;
		}
		break;
	case 0x05:
		// return virtual write register
		
		if (th->frame->prv_frame){
			return &(th->frame->prv_frame->r_registers[n]);
		}else{
			// TO-DO: ERROR when attempting to write to previous frame from first frame
			return NULL;
		}
		break;
	case 0x06:
		//  global register
		return &(th->rf->g_registers[n]);
		break;
	case 0x07:
		// special register
		return &(th->rf->s_registers[n]);
		break;
	default:
		// unknown register type
		return NULL;
		break;
	}
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

void* run_thread(void* th_in){
	Thread* th = (Thread*) th_in;
	Operation op;
	byte opcode;
	byte subop;
	PCType pc_next;
	
	Data args[4]; // temporary storage for up to 4 args
	Data result;
	
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
				pc_next = args[1].f;
			}
			break;
			
		case I_CALL:
			
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			pc_next = args[0].f;
		
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_DECR:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			result.n = args[0].n - 1;
			
			break;
			
		case I_DEV:
			switch(subop){
				
				default:
					break;
			}
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
			args[0] = access_constant(pc_next, th);
			pc_next += sizeof(Data);
		
			switch(subop){
				case SO_ABSOLUTE:
					pc_next = args[0].f;
					break;
					
				case SO_RELATIVE:
					pc_next = th->pc + args[0].f; // TO-DO: Verify that if .f is written as a signed number that sign will be used here
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
			
			free(args[0].p);
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
					fprintf((FILE*) args[0].n, "fun<%lx>", args[1].f);
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
					fprintf((FILE*) args[0].n, "fun<%lx>", args[1].f);
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
			
		case I_SAVEFRAME:
			switch(subop){
				
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
					
					result.t = fork_Thread(th, args[0].f);
					break;
					
				case SO_CONSTANT:
					args[0] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.t = fork_Thread(th, args[0].f);
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
	
	if (th->status < 0){
		teardown_Thread(th);
	}else{
		// status == 0 == TH_STAT_WAIT
		
	}
	
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
	
	if (strcmp(c_str, "true") == 0){
		return 1;
	}else{
		return 0;
	}
}

long int pow_num(long int b, long int e){
	// TO-DO: account for negative numbers?
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
