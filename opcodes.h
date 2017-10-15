#include "vm.h"

#include <stdio.h>

// ALL OPERATIONS DEFINED HERE
/*

 0	ABS,
 1	ADD,
 2	AND,
 3	BRANCH,
 4	BITNOT,
 5	CALL,
 6	DEV,
 7	DIV,
 8	EQ,
 9	GT,
10	HALT,
11	INPUT,
12	JUMP,
13	LSH,
14	LT,
15	M_ALLOC,
16	M_FREE,
17	M_LOAD,
18	M_STORE,
19	MOD,
20	MOVE,
21	MUL,
22	NOT,
23	OR,
24	POPFRAME,
25	PUSHFRAME,
26	POW,
27	PRINT,
28	RETURN,
29	RSH,
30	SAVEFRAME,
31	SUB,
32	TH_NEW,
33	TH_JOIN,
34	TH_KILL,
35	XOR

*/

// vm operations have a signature of:
// operation_OPCODE(byte subop, byte* argstart, PCType* pc, PCType prog_len)

Data operation_ABS(byte subop, byte* prog, PCType* pc, PCType prog_len){
	switch(subop){
		case 0: // one Number in a register
		// -
		break;
		
		case 1: // one Rational in a register
		// -
		break;
		
		case 2: // one Number constant in the prog
		// -
		break;
		
		case 3: // one Rational constant in the prog
		// -
		break;
		
		default:
		Data data;
		return data;
	}
}

