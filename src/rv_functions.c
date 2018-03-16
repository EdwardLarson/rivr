#include "vm.h"

#include <malloc.h>

Function* create_Function(PCType f_start, int ndata){
	Function* f = malloc(sizeof(Function));
	
	init_Function(f, f_start, ndata);
	
	return f;
}

void init_Function(Function* f, PCType f_start, int ndata){
	f->addr = f_start;
	
	f->enclosure_size = (ndata > 0)? ndata : 4;
	
	if (ndata == 0){
		f->enclosure_size = 0;
		f->local_data = NULL;
		f->registers = NULL;
		
	}else{
		f->enclosure_size = (ndata > 0)? ndata : 4;
		f->local_data = calloc(f->enclosure_size, sizeof(Data));
		f->registers = calloc(f->enclosure_size, sizeof(byte));
		
	}
	
	f->n_enclosed = 0;
	
}

Function* copy_Function(const Function* f){
	Function* f_cpy = malloc(sizeof(Function));
	f_cpy->addr = f->addr;
	
	// Most of the time a function is being copied, it is to add data to enclosure
	// Anticipating this, the copy allocates slightly more space than the original
	// Risk of repetitive copies requesting larger and larger mallocs
	// However, repetitive copying is liable to rip through memory anyway
	f_cpy->enclosure_size = f->enclosure_size + CPY_NDATA_GUESS;  
	
	if (f_cpy->enclosure_size > 0){
		f_cpy->local_data = calloc(f_cpy->enclosure_size, sizeof(Data));
		f_cpy->registers = calloc(f_cpy->enclosure_size, sizeof(byte));
	}else{
		f_cpy->enclosure_size = 0;
		f_cpy->local_data = NULL;
		f_cpy->registers = NULL;
	}
	
	f_cpy->n_enclosed = f->n_enclosed;
	
	for (int i = 0; i < f_cpy->n_enclosed; i++){
		f_cpy->local_data[i] = f->local_data[i];
		f_cpy->registers[i] = f->registers[i];
	}
	
	return f_cpy;
}

void teardown_Function(Function* f){
	
	free(f->local_data);
	free(f->registers);
	
	free(f);
}

void enclose_data_Function(Function* f, Data* data, byte reg){
	if (f->n_enclosed == f->enclosure_size){
		f->enclosure_size *= 2;
		f->local_data = realloc(f->local_data, f->enclosure_size);
		f->registers = realloc(f->registers, f->enclosure_size);
	}
	
	f->local_data[f->n_enclosed] = *data;
	
	if (((reg & 0xE0) >> 5) == 0x04){
		// reg is a virtual argument write register
		// the only time a register of this type should be enclosed 
		// is during a partial function call: an enclosure is being
		// created in which one or more arguments are 'pre-passed'
		// in. Notice the distinction between this and an in-scope
		// variable being captured.
		
		// convert argument write register to argument read register
		f->registers[f->n_enclosed] = (reg & 0x1F) | 0x40;
		
	}else{
		f->registers[f->n_enclosed] = reg;
	}
	
	
	f->n_enclosed++;
}

PCType load_Function(const Function* f, Thread* th){
	
	Register_Frame* frame = th->frame;
	byte r_type;
	byte n;
	
	for (int i = 0; i < f->n_enclosed; i++){
		r_type = (f->registers[i] & 0xE0) >> 5;
		n = f->registers[i] & 0x1F;
		
		switch(r_type){
			case 0x00:
			case 0x01:
				
				n = f->registers[i] & 0x3F;
				frame->v_registers[n] = f->local_data[i];
				break;
			case 0x02:
				n = f->registers[i] & 0x1F;
				
				// shift argument registers outward to make room for arg in closure
				for (int j = 31; j > n; j--){
					// WARNING 1: this could break closure layout if 
					// a_registers in f are not ordered (l to g)
					// so make sure that always happens
					// WARNING 2: Will overwrite the last _x_ argument
					// registers, where _x_ is the number of argument
					// registers in the closure
					// This process is expensive so it may become optional in the future
					
					frame->a_registers[j] = frame->a_registers[j-1];
				}
				
				frame->a_registers[n] = f->local_data[i];
				break;
			case 0x03:
				n = f->registers[i] & 0x1F;
				frame->r_registers[n] = f->local_data[i];
				break;
		}
	}
	
	return f->addr;
}




