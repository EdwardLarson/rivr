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


int main(int argc, char** argv){
	PCType proglen = 10;
	
	byte* prog = write_noops_halt();
	
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