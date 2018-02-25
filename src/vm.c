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
	return ((op.bytes[0] & 0x03) << 6) | (op.bytes[1] & 0xC0);
}

Operation encode_operation(byte opcode, byte subop){
	Operation op;
	op.bytes[0] = 0xFC & (opcode << 2);
	op.bytes[0] |= 0x03 & (subop >> 6);
	op.bytes[1] = 0xC0 & (subop << 2);
	
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
	
	th->prog = prog;
	th->prog_len = prog_len;
	th->pc = pc_start;
	th->pc_next = pc_start;
	
	th->status = TH_STAT_RDY;
}

Data* access_register(byte r, Thread* th){
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

Data access_constant(const byte* prog, PCType pc, PCType prog_len){
	Data data;
	if (pc + sizeof(Data) >= prog_len){
		// error
	}else{
		union {Data data, byte bytes[sizeof(Data)} data_union;
		int i;
		for (i = 0; i < sizeof(Data); i++){
			data_union.bytes[i] = prog[pc + i];
		}
		
		data = data_union.data;
	}
	
	return data;
}

void run_thread(Thread* th){
	Operation op;
	byte opcode;
	byte subop;
	
	while ((th->status > 0) && (th->pc < th->prog_len)){
		
		th->pc_next = th->pc;
		
		op = read_op(th->prog, th->pc);
		opcode = get_opcode(op);
		subop = get_subop(op);
		
		th->pc_next += 2;
		
		switch(opcode){
		case I_HALT:
			#ifdef DEBUG
			printf("\tProgram halted.\n");
			#endif
			break;
		default:
			#ifdef DEBUG
			printf("\tError: Unknown opcode %u...\n", opcode);
			#endif
			break;
		}
		
		printf("thread exited with status <%d>\n", th->status);
		printf("pc at <%lu>\n", th->pc);
		
		
		printf("Some data in this frame: %ld\n", access_register(0x01, th)->n);
		
		th->frame = th->frame->nxt_frame;
		th->frame->nxt_frame = alloc_frame(th->rf, th->frame);
		
		th->pc = th->pc_next;
	}
	
	
}


