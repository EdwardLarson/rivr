#include "vm.h"

#include <stdio.h>
#include <malloc.h>

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

void init_Thread(Thread* th, Register_File* rf, const byte* prog, PCType prog_len, PCType pc_start){
	th->rf = rf;
	
	th->frame = alloc_frame(rf, NULL);
	th->frame->nxt_frame = alloc_frame(rf, th->frame);
	
	init_spec_registers(rf->s_registers);
	
	th->prog = prog;
	th->prog_len = prog_len;
	th->pc = pc_start;
	
	th->status = TH_STAT_RDY;
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

void run_thread(Thread* th){
	Operation op;
	byte opcode;
	byte subop;
	PCType pc_next;
	
	Data args[4]; // temporary storage for up to 4 args
	Data result;
	
	while ((th->status > 0) && (th->pc < th->prog_len)){
		
		CLEAR_DATA(result);
		
		pc_next = th->pc;
		
		op = read_op(th->prog, th->pc);
		opcode = get_opcode(op);
		subop = get_subop(op);
		
		#ifdef DEBUG
		printf("Opcode[%x], Subop[%x]\n", opcode, subop);
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
			switch(subop){
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.n = args[0].n + args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.n = args[0].n + args[1].n;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = *access_register(pc_next, th);
					pc_next += 1;
					
					result.d = args[0].d + args[1].d;
					
					break;
					
				case FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_CONSTANT, SO_NONE):
					args[0] = *access_register(pc_next, th);
					pc_next += 1;
					args[1] = access_constant(pc_next, th);
					pc_next += sizeof(Data);
					
					result.d = args[0].d + args[1].d;
					
					break;
					
				default:
					break;
			}
			break;
			
		case I_AND:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_BRANCH:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			if (args[0].b){
				args[1] = access_constant(pc_next, th);
				pc_next = args[1].f;
			}else{
				pc_next += sizeof(Data);
			}
			break;
			
		case I_BITWISE:
			switch(subop){
				
				default:
					break;
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
			
		case I_DEV:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_DIV:
			switch(subop){
				
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
			
		case I_INPUT:
			switch(subop){
				
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
					
				default:
					break;
			}
			break;
			
		case I_M_ALLOC:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_M_FREE:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_M_LOAD:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_M_STORE:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_MOD:
			switch(subop){
				
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
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_NOT:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_NOOP:
			
			#ifdef DEBUG
			printf("\tNo-op\n");
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
			
		case I_OR:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_POPFRAME:
			pop_frame(th);
			break;
			
		case I_PUSHFRAME:
			push_frame(th);
			break;
			
		case I_POW:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_PRINT:
			args[0] = *access_register(pc_next, th);
			pc_next += 1;
			
			switch(subop){
				case SO_NUMBER:
					printf("%ld", args[0].n);
					break;
				case SO_RATIONAL:
					printf("%f", args[0].d);
					break;
				case SO_OBJECT:
					printf("obj<%p>", args[0].p);
					break;
				case SO_STRING:
					if(!args[0].s){
						#ifdef DEBUG
						printf("{in null string branch}");
						#endif
						putc('\n', stdout);
					}else{
						string_print(args[0].s);
					}
					break;
				case SO_THREAD:
					printf("thr<%p>", args[0].t);
					break;
				case SO_FUNCTION:
					printf("fun<%lx>", args[0].f);
					break;
				case SO_BOOLEAN:
					if (args[0].b){
						printf("True");
					}else{
						printf("False");
					}
					break;
				case SO_HASHTABLE:
					printf("htb<%p>", args[0].h);
					break;
				
				default:
					break;
			}
			
			#ifdef DEBUG
			printf("\n\tprint accomplished\n");
			#endif
			break;
			
		case I_SAVEFRAME:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_SUB:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_TH_NEW:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_TH_JOIN:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_TH_KILL:
			switch(subop){
				
				default:
					break;
			}
			break;
			
		case I_XOR:
			switch(subop){
				
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
			printf("\tWriting instruction result to register\n");
			#endif
			*access_register(pc_next, th) = result;
			pc_next += 1;
		}
		
		#ifdef DEBUG
		printf("\tpc at <%lu>\n", th->pc);
		#endif
		
		th->pc = pc_next;
	}
	
	
}


