// ALL OPERATIONS DEFINED HERE
// 64 possible instructions
#define	I_ABS			0x00
#define	I_ADD			0x01
#define	I_AND			0x02
#define	I_BRANCH		0x03
#define	I_BITWISE		0x04
#define	I_CALL			0x05
#define I_DECR			0x06
#define	I_DEV			0x07
#define	I_DIV			0x08
#define	I_EQ			0x09
#define	I_GT			0x0A
#define	I_HALT			0x0B
#define I_INCR			0x0C
#define	I_INPUT			0x0D
#define	I_JUMP			0x0E
#define	I_LSH			0x0F
#define	I_LT			0x10
#define	I_M_ALLOC		0x11
#define	I_M_FREE		0x12
#define	I_M_LOAD		0x13
#define	I_M_STORE		0x14
#define	I_MOD			0x15
#define	I_MOVE			0x16
#define	I_MUL			0x17
#define I_NOOP			0x18
#define	I_NOT			0x19
#define	I_OR			0x1A
#define I_OUTPUT		0x1B
#define	I_POPFRAME		0x1C
#define	I_PUSHFRAME		0x1D
#define	I_POW			0x1E
#define	I_RSH			0x1F
#define	I_SAVEFRAME		0x20
#define	I_SUB			0x21
#define	I_TH_NEW		0x22
#define	I_TH_JOIN		0x23
#define	I_TH_KILL		0x24
#define	I_XOR			0x25

#define HAS_RETURN(opcpde) (\
	(opcode == I_ABS) | \
	(opcode == I_ADD) | \
	(opcode == I_AND) | \
	(opcode == I_BITWISE) | \
	(opcode == I_DECR) | \
	(opcode == I_DIV) | \
	(opcode == I_EQ) | \
	(opcode == I_INCR) | \
	(opcode == I_INPUT) | \
	(opcode == I_GT) | \
	(opcode == I_LSH) | \
	(opcode == I_LT) | \
	(opcode == I_M_ALLOC) | \
	(opcode == I_M_FREE) | \
	(opcode == I_M_STORE) | \
	(opcode == I_MOD) | \
	(opcode == I_MOVE) | \
	(opcode == I_MUL) | \
	(opcode == I_NOT) | \
	(opcode == I_OR) | \
	(opcode == I_POW) | \
	(opcode == I_RSH) | \
	(opcode == I_SUB) | \
	(opcode == I_XOR) \
)


// subop format:
// Format 1:
//   bit0: data type (integer or decimal)
//   bit1: arg 1 type (register or constant)
//   bit2: arg 2 type (register or constant)
//   bit3: arg 3 type (register or constant)
// Format 2:
//   bit 0: arg type (register or constant)
//   bits 1-3: data type (any of the 8 rivr data types)
// Format 3:
//   bit 0: arg type (register or constant)
//   bits 1-3: subop type (boolean operation)

#define FORMAT1_SUBOP(datatype, arg1_type, arg2_type, arg3_type) (datatype | (arg1_type << 1) | (arg2_type << 2) | (arg3_type << 3))
#define FORMAT2_SUBOP(arg_type, datatype) (arg_type | datatype << 1)
#define FORMAT3_SUBOP(arg_type, optype) (arg_type | optype << 1)

#define SO_NONE			0
#define SO_NUMBER		0
#define SO_RATIONAL		1
#define SO_OBJECT		2
#define SO_STRING		3
#define SO_THREAD		4
#define SO_FUNCTION		5
#define SO_BOOLEAN		6
#define SO_HASHTABLE	7

#define SO_REGISTER		0
#define SO_CONSTANT		1

#define SO_ABSOLUTE		0
#define SO_RELATIVE		1

#define SO_AND			0
#define SO_NOT			1
#define SO_OR			2
#define SO_XOR			3

// vm operations have a signature of:
// operation_OPCODE(byte subop, byte* argstart, Thread* th, PCType* pc, PCType prog_len)
