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

byte* write_noops_halt();
byte* write_addition();


int main(int argc, char** argv){
	PCType proglen = 14 + sizeof(Data);
	
	byte* prog = write_addition();
	
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

byte* write_noops_halt(){
	byte* prog = malloc(sizeof(Operation) * 10);
	Operation no_op = encode_operation(I_NOOP, SO_NUMBER);
	Operation halt = encode_operation(I_HALT, SO_NONE);
	
	int i;
	int pc = 0;
	int pc_nxt = 0;
	for (i = 0; i < 10; i++){
		pc = pc_nxt;
		pc_nxt = write_opcode(prog, pc, no_op);
	}
	
	write_opcode(prog, pc, halt);
	
	return prog;
}

byte* write_addition(){
	int proglen = 14 + sizeof(Data);
	byte* prog = malloc( sizeof(byte) * (proglen) );
	
	Operation add_op = encode_operation(I_ADD, FORMAT1_SUBOP(SO_NUMBER, SO_REGISTER, SO_REGISTER, SO_NONE));
	Operation print_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_REGISTER, SO_NUMBER));
	Operation newline_op = encode_operation(I_OUTPUT, FORMAT2_SUBOP(SO_CONSTANT, SO_STRING));
	Operation halt_op = encode_operation(I_HALT, SO_NONE);
	
	printf("add_op raw: %x %x\n", add_op.bytes[0], add_op.bytes[1]);
	
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
	// HALT
	pc = write_opcode(prog, pc, halt_op); // 2
	
	printf("pc=<%d>, proglen=<%d>\n", pc, proglen);
	
	return prog;
}


int write_opcode(byte* prog, int pc, Operation op){
	
	prog[pc] = op.bytes[0];
	prog[pc + 1] = op.bytes[1];
	
	return pc + 2;
}

int write_register(byte* prog, int pc, int reg_id, int r_type){
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
	int i;
	
	for (i = 0; i < sizeof(Data); i++){
		prog[pc + i] = data.bytes[i];
	}
	
	return pc + sizeof(Data);
}