#include "vm.h"

#include <stdio.h>

// ALL OPERATIONS DEFINED HERE

#define	I_ABS			0x00
#define	I_ADD			0x01
#define	I_AND			0x02
#define	I_BRANCH		0x03
#define	I_BITNOT		0x04
#define	I_CALL			0x05
#define	I_DEV			0x06
#define	I_DIV			0x07
#define	I_EQ			0x08
#define	I_GT			0x09
#define	I_HALT			0x0A
#define	I_INPUT			0x0B
#define	I_JUMP			0x0C
#define	I_LSH			0x0D
#define	I_LT			0x0E
#define	I_M_ALLOC		0x0F
#define	I_M_FREE		0x10
#define	I_M_LOAD		0x11
#define	I_M_STORE		0x12
#define	I_MOD			0x13
#define	I_MOVE			0x14
#define	I_MUL			0x15
#define	I_NOT			0x16
#define	I_OR			0x17
#define	I_POPFRAME		0x18
#define	I_PUSHFRAME		0x19
#define	I_POW			0x1A
#define	I_PRINT			0x1B
#define	I_RETURN		0x1C
#define	I_RSH			0x1D
#define	I_SAVEFRAME		0x1E
#define	I_SUB			0x1F
#define	I_TH_NEW		0x20
#define	I_TH_JOIN		0x21
#define	I_TH_KILL		0x22
#define	I_XOR			0x23

#define SO_NUMBER		0x00
#define SO_RATIONAL		0x01
#define SO_REGISTER		0x02
#define SO_CONSTANT		0x00
#define SO_IF		0x04
#define SO_IFNOT		0x00

// vm operations have a signature of:
// operation_OPCODE(byte subop, byte* argstart, Thread* th, PCType* pc, PCType prog_len)

Data operation_ABS(byte subop, byte* prog, Thread* th, PCType* pc, PCType prog_len){
	Data result;
	
	switch(subop){
		case SO_NUMBER | SO_REGISTER: // one Number in a register
		
		long int tmp = access_register(prog[pc], th)->n;
		result.n = tmp >= 0? tmp : tmp * -1;
		break;
		
		case SO_RATIONAL | SO_REGISTER: // one Rational in a register
		
		double tmp = access_register(prog[pc], th)->d;
		result.d = tmp >= 0? tmp : tmp * -1;
		
		break;
		
		case SO_NUMBER | SO_CONSTANT: // one Number constant in the prog
		
		long int tmp = access_constant(prog, pc, prog_len);
		result.n = tmp >= 0? tmp : tmp * -1;
		
		break;
		
		case SO_RATIONAL | SO_CONSTANT: // one Rational constant in the prog
		
		double tmp = access_constant(prog, pc, prog_len);
		result.d = tmp >= 0? tmp : tmp * -1;
		
		break;
		
		default:
		break;
	}
	
	return result;
}

Data operation_ADD(byte subop, byte* prog, Thread* th, PCType* pc, PCType prog_len){
	Data result;
	
	switch(subop){
		case SO_NUMBER | SO_REGISTER:
			result.n = 
				access_register(prog[pc], th)->n + 
				access_register(prog[pc + 1], th)->n;
			
			break;
		case SO_NUMBER | SO_CONSTANT:
			result.n = 
				access_constant(prog, pc, prog_len) +
				access_constant(prog, pc + sizeof(Data), prog_len);
				
			break;
		case SO_RATIONAL | SO_REGISTER:
			result.d = 
				access_register(prog[pc], th)->d + 
				access_register(prog[pc + 1], th)->d;
			break;
		case SO_RATIONAL | SO_CONSTANT:
			result.d = 
				access_constant(prog, pc, prog_len) +
				access_constant(prog, pc + sizeof(Data), prog_len);
			break;
		default:
			break;
	}
	
	return result;
}
