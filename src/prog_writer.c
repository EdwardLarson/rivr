#include "vm.h"
#include <stdio.h>

#define REG_VAR		0x00
#define REG_GLOB	0x01
#define REG_RARG	0x02
#define REG_RRET	0x03
#define REG_WARG	0x04
#define REG_WRET	0x05
#define REG_SPEC	0x06

int write_opcode(byte* prog, int pc, Operation op);
int write_register(byte* prog, int pc, int reg_id, int r_type);
int write_constant(byte* prog, int pc, Data data);

int write_noops_halt(byte* prog);
int write_addition(byte* prog);
int write_input(byte* prog);
int write_memory(byte* prog);


int main(int argc, char** argv){
	int proglen = write_memory(NULL);
	byte* prog = calloc(proglen, sizeof(byte));
	int actual_proglen = write_memory(prog);
	
	printf("proglen initially counted as %d, was %d when writing\n", proglen, actual_proglen);
	
	FILE* fp;
	
	if (argc > 1){
		fp = fopen(argv[1], "w");
		if (!fp){
			printf("Error: unable to open %s\n", argv[1]);
			return 1;
		}
	}else{
		fp = fopen("rivr_prog.b", "w");
		if (!fp){
			printf("Error: unable to open rivr_prog.b\n");
		}
	}
	
	fwrite(prog, sizeof(byte), (size_t) proglen, fp);
	
	fclose(fp);
	
	return 0;
}

int write_noops_halt(byte* prog){
	Operation no_op = encode_operation(I_NOOP, SO_NUMBER);
	Operation halt = encode_operation(I_HALT, SO_NONE);
	
	int i;
	int pc = 0;
	int pc_nxt = 0;
	for (i = 0; i < 10; i++){
		pc = pc_nxt;
		pc_nxt = write_opcode(prog, pc, no_op);
	}
	
	pc = write_opcode(prog, pc, halt);
	
	return pc;
}

int write_addition(byte* prog){
	
	Operation add_op = encode_operation(I_ADD, FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE));
	Operation print_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_REGISTER, SO_NUMBER));
	Operation newline_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_CONSTANT, SO_STRING));
	Operation halt_op = encode_operation(I_HALT, SO_NONE);
	Operation increment_op = encode_operation(I_INCR, SO_NONE);
	
	Data newline;
	newline.bytes[0] = '\n';
	newline.bytes[1] = '\0';
	
	int pc = 0;
	// ADD $!1 $!1 > $0
	pc = write_opcode(prog, pc, add_op); // 2
	pc = write_register(prog, pc, 1, REG_SPEC); // 1
	pc = write_register(prog, pc, 1, REG_SPEC); // 1
	pc = write_register(prog, pc, 0, REG_VAR); // 1
	// PRINT $0
	pc = write_opcode(prog, pc, print_op); // 2
	pc = write_register(prog, pc, 2, REG_SPEC); // 1
	pc = write_register(prog, pc, 0, REG_VAR); // 1
	// PRINT newline
	pc = write_opcode(prog, pc, newline_op); // 2
	pc = write_register(prog, pc, 2, REG_SPEC); // 1
	pc = write_constant(prog, pc, newline); // _data
	// INCR $0 > $0
	pc = write_opcode(prog, pc, increment_op);
	pc = write_register(prog, pc, 0, REG_VAR);
	pc = write_register(prog, pc, 0, REG_VAR);
	// PRINT $0
	pc = write_opcode(prog, pc, print_op); // 2
	pc = write_register(prog, pc, 2, REG_SPEC); // 1
	pc = write_register(prog, pc, 0, REG_VAR); // 1
	// PRINT newline
	pc = write_opcode(prog, pc, newline_op); // 2
	pc = write_register(prog, pc, 2, REG_SPEC); // 1
	pc = write_constant(prog, pc, newline); // _data
	// HALT
	pc = write_opcode(prog, pc, halt_op); // 2
	
	return pc;
}

int write_input(byte* prog){
	int proglen = 52 + (sizeof(Data)*6);
	int pc = 0;
	
	Operation num_input_op = encode_operation(I_INPUT, SO_NUMBER);
	Operation str_input_op = encode_operation(I_INPUT, SO_STRING);
	Operation num_print_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_REGISTER, SO_NUMBER));
	Operation str_print_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_REGISTER, SO_STRING));
	Operation nwl_print_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_CONSTANT, SO_STRING));
	Operation copy_op = encode_operation(I_MOVE, SO_REGISTER);
	Operation halt_op = encode_operation(I_HALT, SO_NONE);
	
	Data newline;
	newline.bytes[0] = '\n';
	newline.bytes[1] = '\0';
	
	// MOVE $!0 > $0
	pc = write_opcode(prog, pc, copy_op); // 2
	pc = write_register(prog, pc, 0, REG_SPEC); // 1
	pc = write_register(prog, pc, 0, REG_VAR); // 1
	// MOVE $!0 > $1
	pc = write_opcode(prog, pc, copy_op); // 2
	pc = write_register(prog, pc, 0, REG_SPEC); // 1
	pc = write_register(prog, pc, 1, REG_VAR); // 1
	// INPUT $!3 > $0
	pc = write_opcode(prog, pc, num_input_op); // 2
	pc = write_register(prog, pc, 3, REG_SPEC); // 1
	pc = write_register(prog, pc, 0, REG_VAR); // 1
	// OUTPUT '/n'
	pc = write_opcode(prog, pc, nwl_print_op); // 2
	pc = write_register(prog, pc, 2, REG_SPEC); // 1
	pc = write_constant(prog, pc, newline); // _data
	// OUTPUT $!2 $0
	pc = write_opcode(prog, pc, num_print_op); // 2
	pc = write_register(prog, pc, 2, REG_SPEC); // 1
	pc = write_register(prog, pc, 0, REG_VAR); // 1
	// OUTPUT '/n'
	pc = write_opcode(prog, pc, nwl_print_op); // 2
	pc = write_register(prog, pc, 2, REG_SPEC); // 1
	pc = write_constant(prog, pc, newline); // _data
	
	// INPUT $!3 > $1
	pc = write_opcode(prog, pc, str_input_op); // 2
	pc = write_register(prog, pc, 3, REG_SPEC); // 1
	pc = write_register(prog, pc, 1, REG_VAR); // 1
	// OUTPUT '/n'
	pc = write_opcode(prog, pc, nwl_print_op); // 2
	pc = write_register(prog, pc, 2, REG_SPEC); // 1
	pc = write_constant(prog, pc, newline); // _data
	// OUTPUT $!2 $1
	pc = write_opcode(prog, pc, str_print_op); // 2
	pc = write_register(prog, pc, 2, REG_SPEC); // 1
	pc = write_register(prog, pc, 1, REG_VAR); // 1
	// OUTPUT '/n'
	pc = write_opcode(prog, pc, nwl_print_op); // 2
	pc = write_register(prog, pc, 2, REG_SPEC); // 1
	pc = write_constant(prog, pc, newline); // _data
	
	// INPUT $!3 > $1
	pc = write_opcode(prog, pc, str_input_op); // 2
	pc = write_register(prog, pc, 3, REG_SPEC); // 1
	pc = write_register(prog, pc, 1, REG_VAR); // 1
	// OUTPUT '/n'
	pc = write_opcode(prog, pc, nwl_print_op); // 2
	pc = write_register(prog, pc, 2, REG_SPEC); // 1
	pc = write_constant(prog, pc, newline); // _data
	// OUTPUT $!2 $1
	pc = write_opcode(prog, pc, str_print_op); // 2
	pc = write_register(prog, pc, 2, REG_SPEC); // 1
	pc = write_register(prog, pc, 1, REG_VAR); // 1
	// OUTPUT '/n'
	pc = write_opcode(prog, pc, nwl_print_op); // 2
	pc = write_register(prog, pc, 2, REG_SPEC); // 1
	pc = write_constant(prog, pc, newline); // _data
	
	// HALT
	pc = write_opcode(prog, pc, halt_op); // 2
	
	printf("pc=<%d>, proglen=<%d>\n", pc, proglen);
	
	return pc;
}

int write_memory(byte* prog){
	int pc = 0;
	Operation alloc_op = encode_operation(I_M_ALLOC, SO_CONSTANT);
	Operation store_op = encode_operation(I_M_STORE, FORMAT1_SUBOP(SO_NONE, SO_REGISTER, SO_CONSTANT, SO_NONE));
	Operation free_op = encode_operation(I_M_FREE, SO_NONE);
	Operation load_op = encode_operation(I_M_LOAD, SO_CONSTANT);
	Operation copy_op = encode_operation(I_MOVE, SO_REGISTER);
	Operation increment_op = encode_operation(I_INCR, SO_NONE);
	Operation num_print_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_REGISTER, SO_NUMBER));
	Operation obj_print_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_REGISTER, SO_OBJECT));
	Operation nwl_print_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_CONSTANT, SO_STRING));
	Operation halt_op = encode_operation(I_HALT, SO_NONE);
	
	Data newline;
	newline.bytes[0] = '\n';
	newline.bytes[1] = '\0';
	
	Data zero;
	zero.n = 0;
	Data one;
	one.n = 1;
	Data two;
	two.n = 2;
	Data three;
	three.n = 3;
	Data four;
	four.n = 4;
	Data five;
	five.n = 5;
	Data six;
	six.n = 6;
	
	// MOVE $!0 > $0
	pc = write_opcode(prog, pc, copy_op);
	pc = write_register(prog, pc, 0, REG_SPEC);
	pc = write_register(prog, pc, 0, REG_VAR);
	
	// MOVE $!1 > $1
	pc = write_opcode(prog, pc, copy_op);
	pc = write_register(prog, pc, 1, REG_SPEC);
	pc = write_register(prog, pc, 1, REG_VAR);
	
	// INCR $1 > $2
	pc = write_opcode(prog, pc, increment_op);
	pc = write_register(prog, pc, 1, REG_VAR);
	pc = write_register(prog, pc, 2, REG_VAR);
	
	// INCR $2 > $3
	pc = write_opcode(prog, pc, increment_op);
	pc = write_register(prog, pc, 2, REG_VAR);
	pc = write_register(prog, pc, 3, REG_VAR);
	
	// INCR $3 > $4
	pc = write_opcode(prog, pc, increment_op);
	pc = write_register(prog, pc, 3, REG_VAR);
	pc = write_register(prog, pc, 4, REG_VAR);
	
	// INCR $4 > $5
	pc = write_opcode(prog, pc, increment_op);
	pc = write_register(prog, pc, 4, REG_VAR);
	pc = write_register(prog, pc, 5, REG_VAR);
	
	// M_ALLOC 6 > $6
	pc = write_opcode(prog, pc, alloc_op);
	pc = write_constant(prog, pc, six);
	pc = write_register(prog, pc, 6, REG_VAR);
	
	// OUTPUT $!2 $6
	pc = write_opcode(prog, pc, obj_print_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_register(prog, pc, 6, REG_VAR);
	
	// OUTPUT $!2 '\n'
	pc = write_opcode(prog, pc, nwl_print_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_constant(prog, pc, newline);
	
	// M_STORE $6 0 $3
	pc = write_opcode(prog, pc, store_op);
	pc = write_register(prog, pc, 6, REG_VAR);
	pc = write_register(prog, pc, 3, REG_VAR);
	pc = write_constant(prog, pc, zero);
	
	// M_STORE $6 1 $4
	pc = write_opcode(prog, pc, store_op);
	pc = write_register(prog, pc, 6, REG_VAR);
	pc = write_register(prog, pc, 4, REG_VAR);
	pc = write_constant(prog, pc, one);
	
	// M_STORE $6 2 $5
	pc = write_opcode(prog, pc, store_op);
	pc = write_register(prog, pc, 6, REG_VAR);
	pc = write_register(prog, pc, 5, REG_VAR);
	pc = write_constant(prog, pc, two);
	
	// M_STORE $6 3 $0
	pc = write_opcode(prog, pc, store_op);
	pc = write_register(prog, pc, 6, REG_VAR);
	pc = write_register(prog, pc, 0, REG_VAR);
	pc = write_constant(prog, pc, three);
	
	// M_STORE $6 4 $1
	pc = write_opcode(prog, pc, store_op);
	pc = write_register(prog, pc, 6, REG_VAR);
	pc = write_register(prog, pc, 1, REG_VAR);
	pc = write_constant(prog, pc, four);
	
	// M_STORE $6 5 $2
	pc = write_opcode(prog, pc, store_op);
	pc = write_register(prog, pc, 6, REG_VAR);
	pc = write_register(prog, pc, 2, REG_VAR);
	pc = write_constant(prog, pc, five);
	
	// M_LOAD $6 0 > $0
	pc = write_opcode(prog, pc, load_op);
	pc = write_register(prog, pc, 6, REG_VAR);
	pc = write_constant(prog, pc, zero);
	pc = write_register(prog, pc, 0, REG_VAR);
	
	// M_LOAD $6 1 > $1
	pc = write_opcode(prog, pc, load_op);
	pc = write_register(prog, pc, 6, REG_VAR);
	pc = write_constant(prog, pc, one);
	pc = write_register(prog, pc, 1, REG_VAR);
	
	// M_LOAD $6 2 > $2
	pc = write_opcode(prog, pc, load_op);
	pc = write_register(prog, pc, 6, REG_VAR);
	pc = write_constant(prog, pc, two);
	pc = write_register(prog, pc, 2, REG_VAR);
	
	// M_LOAD $6 3 > $3
	pc = write_opcode(prog, pc, load_op);
	pc = write_register(prog, pc, 6, REG_VAR);
	pc = write_constant(prog, pc, three);
	pc = write_register(prog, pc, 3, REG_VAR);
	
	// M_LOAD $6 4 > $4
	pc = write_opcode(prog, pc, load_op);
	pc = write_register(prog, pc, 6, REG_VAR);
	pc = write_constant(prog, pc, four);
	pc = write_register(prog, pc, 4, REG_VAR);
	
	// M_LOAD $6 5 > $5
	pc = write_opcode(prog, pc, load_op);
	pc = write_register(prog, pc, 6, REG_VAR);
	pc = write_constant(prog, pc, five);
	pc = write_register(prog, pc, 5, REG_VAR);
	
	// M_FREE $6
	pc = write_opcode(prog, pc, free_op);
	pc = write_register(prog, pc, 6, REG_VAR);
	
	// OUTPUT $!2 $5
	pc = write_opcode(prog, pc, num_print_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_register(prog, pc, 5, REG_VAR);
	
	// OUTPUT $!2 '\n'
	pc = write_opcode(prog, pc, nwl_print_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_constant(prog, pc, newline);
	
	// HALT
	pc = write_opcode(prog, pc, halt_op);
	
	
	return pc;
}


int write_opcode(byte* prog, int pc, Operation op){
	
	if (!prog) return pc + 2;
	
	prog[pc] = op.bytes[0];
	prog[pc + 1] = op.bytes[1];
	
	return pc + 2;
}

int write_register(byte* prog, int pc, int reg_id, int r_type){
	
	if (!prog) return pc + 1;
	
	reg_id = reg_id & 0xFF;
	byte reg = 0;
	
	switch(r_type){
		case REG_VAR:
			reg = (reg_id & 0x3F) | (0x00 << 5);
			break;
		case REG_GLOB:
			reg = (reg_id & 0x1F) | (0x06 << 5);
			break;
		case REG_RARG:
			reg = (reg_id & 0x1F) | (0x02 << 5);
			break;
		case REG_RRET:
			reg = (reg_id & 0x1F) | (0x03 << 5);
			break;
		case REG_WARG:
			reg = (reg_id & 0x1F) | (0x04 << 5);
			break;
		case REG_WRET:
			reg = (reg_id & 0x1F) | (0x05 << 5);
			break;
		case REG_SPEC:
			reg = (reg_id & 0x1F) | (0x07 << 5);
			break;
	}
	
	prog[pc] = reg;
	
	return pc + 1;
}

int write_constant(byte* prog, int pc, Data data){
	if (!prog) return pc + sizeof(Data);
	
	int i;
	
	for (i = 0; i < sizeof(Data); i++){
		prog[pc + i] = data.bytes[i];
	}
	
	return pc + sizeof(Data);
}