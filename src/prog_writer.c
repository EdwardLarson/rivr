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

void record_pc(byte* prog, int pc, int* refs, int nrefs);

int write_noops_halt(byte* prog);
int write_addition(byte* prog);
int write_input(byte* prog);
int write_memory(byte* prog);
int write_pow(byte* prog);
int write_branch(byte* prog);


int main(int argc, char** argv){
	int proglen = write_branch(NULL);
	byte* prog = calloc(proglen, sizeof(byte));
	int actual_proglen = write_branch(prog);
	
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

int write_pow(byte* prog){
	int pc = 0;
	
	Operation npow_op = encode_operation(I_POW, FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE));
	Operation rpow_op = encode_operation(I_POW, FORMAT1_SUBOP(SO_RATIONAL, SO_REGISTER, SO_REGISTER, SO_NONE));
	Operation load_op = encode_operation(I_MOVE, SO_CONSTANT);
	Operation increment_op = encode_operation(I_INCR, SO_NONE);
	Operation num_print_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_REGISTER, SO_NUMBER));
	Operation rat_print_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_REGISTER, SO_RATIONAL));
	Operation nwl_print_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_CONSTANT, SO_STRING));
	Operation halt_op = encode_operation(I_HALT, SO_NONE);
	
	Data newline;
	newline.bytes[0] = '\n';
	newline.bytes[1] = '\0';
	
	Data done;
	done.bytes[0] = 'D';
	done.bytes[1] = 'o';
	done.bytes[2] = 'n';
	done.bytes[3] = 'e';
	done.bytes[4] = '\0';
	
	Data onef;
	onef.d = 1.0D;
	
	Data twof;
	twof.d = 2.0D;
	
	Data threef;
	threef.d = -3.0D;
	
	
	// INCR $!1 > $0
	pc = write_opcode(prog, pc, increment_op);
	pc = write_register(prog, pc, 1, REG_SPEC);
	pc = write_register(prog, pc, 0, REG_VAR);
	
	// INCR $0 > $1
	pc = write_opcode(prog, pc, increment_op);
	pc = write_register(prog, pc, 0, REG_VAR);
	pc = write_register(prog, pc, 1, REG_VAR);
	
	// POW $0 $1 > $2
	pc = write_opcode(prog, pc, npow_op);
	pc = write_register(prog, pc, 0, REG_VAR);
	pc = write_register(prog, pc, 1, REG_VAR);
	pc = write_register(prog, pc, 2, REG_VAR);
	
	// OUTPUT $!2 $2
	pc = write_opcode(prog, pc, num_print_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_register(prog, pc, 2, REG_VAR);
	
	// OUTPUT $!2 '\n'
	pc = write_opcode(prog, pc, nwl_print_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_constant(prog, pc, newline);
	
	// MOVE twof > $0
	pc = write_opcode(prog, pc, load_op);
	pc = write_constant(prog, pc, twof);
	pc = write_register(prog, pc, 0, REG_VAR);
	
	// MOVE onef > $1
	pc = write_opcode(prog, pc, load_op);
	pc = write_constant(prog, pc, onef);
	pc = write_register(prog, pc, 1, REG_VAR);
	
	// POW $0 $1 > $2
	pc = write_opcode(prog, pc, rpow_op);
	pc = write_register(prog, pc, 0, REG_VAR);
	pc = write_register(prog, pc, 1, REG_VAR);
	pc = write_register(prog, pc, 2, REG_VAR);
	
	// OUTPUT $!2 $2
	pc = write_opcode(prog, pc, rat_print_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_register(prog, pc, 2, REG_VAR);
	
	// OUTPUT $!2 '\n'
	pc = write_opcode(prog, pc, nwl_print_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_constant(prog, pc, newline);
	
	// MOVE threef > $1
	pc = write_opcode(prog, pc, load_op);
	pc = write_constant(prog, pc, threef);
	pc = write_register(prog, pc, 1, REG_VAR);
	
	// POW $0 $1 > $2
	pc = write_opcode(prog, pc, rpow_op);
	pc = write_register(prog, pc, 0, REG_VAR);
	pc = write_register(prog, pc, 1, REG_VAR);
	pc = write_register(prog, pc, 2, REG_VAR);
	
	// OUTPUT $!2 $2
	pc = write_opcode(prog, pc, rat_print_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_register(prog, pc, 2, REG_VAR);
	
	// OUTPUT $!2 '\n'
	pc = write_opcode(prog, pc, nwl_print_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_constant(prog, pc, newline);
	
	// OUTPUT $!2 "Done"
	pc = write_opcode(prog, pc, nwl_print_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_constant(prog, pc, done);
	
	// HALT 
	pc = write_opcode(prog, pc, halt_op);
	
	return pc;
}

int write_branch(byte* prog){
	int pc = 0;
	Operation copy_op = encode_operation(I_MOVE, SO_REGISTER);
	Operation num_input_op = encode_operation(I_INPUT, SO_NUMBER);
	Operation gt_op = encode_operation(I_GT, FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE));
	Operation branch_op = encode_operation(I_BRANCH, SO_NONE);
	Operation cons_str_output_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_CONSTANT, SO_STRING));
	Operation incr_op = encode_operation(I_INCR, SO_NONE);
	Operation jump_op = encode_operation(I_JUMP, SO_ABSOLUTE);
	Operation num_output_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_REGISTER, SO_NUMBER));
	Operation halt_op = encode_operation(I_HALT, SO_NONE);
	
	Data str_greater_than_one;
	str_greater_than_one.bytes[0] = '>';
	str_greater_than_one.bytes[1] = '1';
	str_greater_than_one.bytes[2] = '\n';
	str_greater_than_one.bytes[3] = '\0';
	
	Data str_finished;
	str_finished.bytes[0] = 'f';
	str_finished.bytes[1] = 'i';
	str_finished.bytes[2] = 'n';
	str_finished.bytes[3] = '\n';
	str_finished.bytes[4] = '\0';
	
	Data str_newline;
	str_newline.bytes[0] = '\n';
	str_newline.bytes[1] = '\0';
	
	int label_input_loop_loc;
	int label_input_loop_ref;
	int label_else_loc;
	int label_else_ref;
	
	// MOVE $!0 > $0
	pc = write_opcode(prog, pc, copy_op);
	pc = write_register(prog, pc, 0, REG_SPEC);
	pc = write_register(prog, pc, 0, REG_VAR);
	
	// :INPUT_LOOP:
	label_input_loop_loc = pc;
	
	// INPUT $!3 > $1
	pc = write_opcode(prog, pc, num_input_op);
	pc = write_register(prog, pc, 3, REG_SPEC);
	pc = write_register(prog, pc, 1, REG_VAR);
	
	// GT $1 $!1 > $2
	pc = write_opcode(prog, pc, gt_op);
	pc = write_register(prog, pc, 1, REG_VAR);
	pc = write_register(prog, pc, 1, REG_SPEC);
	pc = write_register(prog, pc, 2, REG_VAR);
	
	// BRANCH $2 :ELSE:
	pc = write_opcode(prog, pc, branch_op);
	pc = write_register(prog, pc, 2, REG_VAR);
	label_else_ref = pc;
	pc += sizeof(Data);
	
	// OUTPUT $!2 ">1\n"
	pc = write_opcode(prog, pc, cons_str_output_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_constant(prog, pc, str_greater_than_one);
	
	// INCR $0 > $0
	pc = write_opcode(prog, pc, incr_op);
	pc = write_register(prog, pc, 0, REG_VAR);
	pc = write_register(prog, pc, 0, REG_VAR);
	
	// JUMP :INPUT_LOOP:
	pc = write_opcode(prog, pc, jump_op);
	label_input_loop_ref = pc;
	pc += sizeof(Data);
	
	// :ELSE:
	label_else_loc = pc;
	
	// OUTPUT $!2 "fin\n"
	pc = write_opcode(prog, pc, cons_str_output_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_constant(prog, pc, str_finished);
	
	// OUTPUT $!2 $0
	pc = write_opcode(prog, pc, num_output_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_register(prog, pc, 0, REG_VAR);
	
	// OUTPUT $!2 "\n"
	pc = write_opcode(prog, pc, cons_str_output_op);
	pc = write_register(prog, pc, 2, REG_SPEC);
	pc = write_constant(prog, pc, str_newline);
	
	// HALT
	pc = write_opcode(prog, pc, halt_op);
	
	record_pc(prog, label_else_loc, &label_else_ref, 1);
	record_pc(prog, label_input_loop_loc, &label_input_loop_ref, 1);
	
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

void record_pc(byte* prog, int pc, int* refs, int nrefs){
	Data location;
	location.f = (PCType) pc;
	int i;
	for (i = 0; i < nrefs; i++){
		write_constant(prog, refs[i], location);
	}
}