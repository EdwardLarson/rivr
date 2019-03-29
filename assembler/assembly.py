import struct
import sys
import io

label_references = {} # label : [(line_no, pc)...]
label_declarations = {} # label : (line_no, pc)

# so a label DECLARATION needs to mark an instruction
# when that instruction is written, then the label becomes associated with that written location
# label REFERENCES are within instructions, that is, they are known to instructions
# they will be constants, so outside the 8-byte instruction itself


SREG_NIL = 31
SREG_CONST_CACHE_START = 8 + 224

line_no = 0
pc = 0

opcode_LUT = {
	"ABS"		: 0x00,
	"ADD"		: 0x01,
	"AND"		: 0x02,
	"BRANCH"	: 0x03,
	"BW_AND"	: 0X04,
	"BW_NOT"	: 0x05,
	"BW_OR"		: 0x06,
	"BW_XOR"	: 0x07,
	"DECR"		: 0x08,
	"DIV"		: 0x09,
	"EQ"		: 0x0A,
	"F_CALL"	: 0x0B,
	"F_CREATE"	: 0x0C,
	"GT"		: 0x0D,
	"HALT"		: 0x0E,
	"INCR"		: 0x0F,
	"INPUT"		: 0x10,
	"JUMP"		: 0x11,
	"LSH"		: 0x12,
	"LT"		: 0x13,
	"M_ALLOC"	: 0x14,
	"M_FREE"	: 0x15,
	"M_LOAD"	: 0x16,
	"M_STORE"	: 0x17,
	"MOD"		: 0x18,
	"MOVE"		: 0x19,
	"MUL"		: 0x1A,
	"NOOP"		: 0x1B,
	"NOT"		: 0x1C,
	"OR"		: 0x1D,
	"OUTPUT"	: 0x1E,
	"POPFRAME"	: 0x1F,
	"PUSHFRAME"	: 0x20,
	"POW"		: 0x21,
	"RSH"		: 0x22,
	"SUB"		: 0x23,
	"TH_NEW"	: 0x24,
	"TH_JOIN"	: 0X25,
	"TH_KILL"	: 0X26,
	"XOR"		: 0X27,
}

argtype_LUT = {
	"NUMBER"	: 0x0,
	"RATIONAL"	: 0x1,
	"STRING"	: 0x2,
	"BOOLEAN"	: 0x3,
}

def str_is_int(s):
	try:
		int(s)
		return True
	except:
		return False
		
def str_is_float(s):
	try:
		float(s)
		return True
	except:
		return False
		

"""
Takes a register argument <reg_arg> and converts it to a register numbered 0-255
This is the register that is accessed via Cache.all_registers[x]
"""
def get_literal_register(reg_arg):
	reg_begins = {
		'!': 224, # special
		'%': 96, # return reads
		'^': 160, # return writes
		'@': 192, # globals
		'&': 64, # arg reads
		'*': 128 # arg writes
	}
	
	if reg_arg[0] in reg_begins:
		n = int (reg_arg[1:])
		
		literal = n + reg_begins[reg_arg[0]]
		
	else:
		literal = int(reg_arg)
		
	if literal < 256:
		return literal
	else:
		print "ERROR line {}: argument {} becomes register {}, which is beyond max register 255".format(line_no, reg_arg, literal)
		exit(0)

"""
Determine the type of an argument and parse its value as such
Adds register numbers to the list of register arguments for the instruction
Adds constant values to the instruction's tail which contains constants referenced by the instruction
Returns the bit to be stored in the subop for this instruction (1 for constants, 0 for registers)
"""
def parse_argument(arg, arg_reg_bytes, instruction_tail, local_label_references):
	global pc
	
	if arg[0] == '$': #register
		reg_arg = get_literal_register(arg[1:])
		
		arg_reg_bytes.append(struct.pack("=B", reg_arg))
		
		return 0
		
	elif arg[0] == ':': #label reference
		label_name = arg[1:].strip()
		
		"""
		if label_name in local_label_references:
			local_label_references[label_name].append(len(instruction_tail))
		else:
			local_label_references[label_name] = [len(instruction_tail)]
			"""
		arg_reg_bytes.append(struct.pack("=B", SREG_CONST_CACHE_START + len(arg_reg_bytes)))
		instruction_tail.append( (None, label_name) )
		
		return 1 # for now let all label usages be absolute addressed jumps
		
	elif arg[0] == '"': # String constant
		str_const = arg[1:-2]
		
		arg_reg_bytes.append(struct.pack("=B", SREG_CONST_CACHE_START + len(arg_reg_bytes)))
		instruction_tail.append(str_const)
		
		return 1
		
	elif str_is_int(arg): # Number constant
		num_const = int(arg)
		
		arg_reg_bytes.append(struct.pack("=B", SREG_CONST_CACHE_START + len(arg_reg_bytes)))
		instruction_tail.append(struct.pack("=q", num_const))
		
		print "-- line {} found num constant {}, gave it register {}".format(line_no, num_const, ord(arg_reg_bytes[-1]))
		
		return 1
		
	elif str_is_float(arg): # Rational constant
		rat_const = float(arg)
		
		arg_reg_bytes.append(struct.pack("=B", SREG_CONST_CACHE_START + len(arg_reg_bytes)))
		instruction_tail.append(struct.pack("=d", rat_const))
		
		return 1
		
	elif arg == "True": # Boolean constant (True)
		arg_reg_bytes.append(struct.pack("=B", SREG_CONST_CACHE_START + len(arg_reg_bytes)))
		instruction_tail.append(struct.pack("=B", 1))
		
		return 1
		
	elif arg == "False": # Boolean constant (False)
		arg_reg_bytes.append(struct.pack("=B", SREG_CONST_CACHE_START + len(arg_reg_bytes)))
		instruction_tail.append(struct.pack("=B", 0))
		
		return 1
		
"""
Returns a tuple (opcode_num, arg_type)

"""
def parse_opcode(opcode):
	split_by_parenth = [item.strip() for item in opcode.split('(')]
	
	if len(split_by_parenth) > 1:
		# has an arg_type
		
		arg_type_str = split_by_parenth[1].rstrip(')')
		
		print "-- instruction on line {} has an arg type {}".format(line_no, arg_type_str)
		
		return opcode_LUT[split_by_parenth[0]], argtype_LUT[arg_type_str]
		
	else:
		return opcode_LUT[opcode], 0x0
		
def parse_instruction(line, byte_queue):
	global pc
	
	instr_and_return = [item.strip() for item in line.split("->")]
	
	has_return = len(instr_and_return) > 1
	split_instr = [item.strip() for item in instr_and_return[0].split(",")]
	
	opcode = split_instr[0]
	args = split_instr[1:]
	
	if len(args) > 5:
		print "ERROR line {}: Too many arguments for {}".format(line_no, line)
		
	ret_reg = None
	
	if has_return:
		# second member of instr_and_return should be instruction in format '$x'
		# don't care about first character '$'
		ret_reg = get_literal_register(instr_and_return[1][1:])
		
	else:
		ret_reg = SREG_NIL
		
	arg_reg_bytes = []
	instruction_tail = []
	local_label_references = {}
	
	opcode, subop = parse_opcode(opcode)
	
	for i in range(len(args)):
		is_const = parse_argument(args[i], arg_reg_bytes, instruction_tail, local_label_references)
		
		subop = subop | (is_const << (i + 2))
		
	# pad out the rest of the argument bytes
	while (len(arg_reg_bytes) < 5):
		arg_reg_bytes.append(struct.pack("=B", SREG_NIL))
		
	# register the local_label_references as global label_references
	for label, references in local_label_references.iteritems():
		for tail_offset in references:
			# pc at start of instruction + instruction bytes + offset from end of instruction
			absolute_reference_loc = outstream.tell() + 8 + tail_offset
			
			if label in label_references:
				label_references[label].append(absolute_reference_loc)
				
			else:
				label_references[label] = [absolute_reference_loc]
		
	# now output full instruction bytes
	
	opcode_byte = struct.pack("=B", opcode)
	subop_byte = struct.pack("=B", subop)
	ret_reg_byte = struct.pack("=B", ret_reg)
	
	print "\t\tline {}: opcode = {!r}; subop = {!r}; pc = {}".format(line_no, opcode_byte, subop_byte, pc)
	
	byte_queue.append(opcode_byte)
	byte_queue.append(subop_byte)
	pc += 2
	
	for arg_reg in arg_reg_bytes:
		byte_queue.append(arg_reg)
		pc += 1
		
	byte_queue.append(ret_reg_byte)
	pc += 1
	
	for constant in instruction_tail:
		if type(constant) == tuple: # flags a reference!
			# add a reference at this offset to the label
			label = constant[1]
			
			print "-- placing label ref {} at {}".format(label, pc)
			
			if label in label_references:
				label_references[label].append(len(byte_queue))
				
			else:
				label_references[label] = [len(byte_queue)]
				
			byte_queue.append(label)
			pc += 8
				
		else:
			print "-- outputting constant {!r} with length {} at {}".format(constant , len(constant), pc)
			byte_queue.append(constant)
			pc += len(constant)
	
	
if __name__ == "__main__":

	assembly_in = sys.argv[1]
	bytecode_out = "out.b" if len(sys.argv) < 3 else sys.argv[2]
	
	
	with open(assembly_in, "r") as f:
		
		# first pass, 3 things happen:
		# 1. Instructions turn themselves and args into bytecode, write that to an output string
		# 2. Label references are written as 0s, but the location is recorded
		# 3. Label declarations: label_declarations[label] = output_i
		
		out_queue = []

		for instruction_line in f:
			line_no += 1
			
			instruction_line = instruction_line.split(';')[0].strip() # discard comments after ';'
			
			if len(instruction_line) == 0:
				continue
			
			if instruction_line[0] == ':':
				# label
				label = instruction_line[1:].strip()
				
				if label in label_declarations:
					#error! multiple labels
					print "ERROR line {}: Multiple declarations for label {}: Declared line {} and line {}".format(
						line_no, label, label_declarations[label][0], line_no)
					exit()
				else:
					label_declarations[label] = pc
					
			else:
				# instruction
				parse_instruction(instruction_line, out_queue)
			
		# second pass: fill in labels
		# for label, refs in label_references.iteritems(): for ref in refs: output[ref] = label_declarations[label]
		
		for label, references in label_references.iteritems():
			if label in label_declarations:
				label_loc = struct.pack("=Q", label_declarations[label])
				
				for ref in references:
					print "-- overwriting {} reference at {} with {} (was {})".format(label, ref, label_loc, out_queue[ref])
					out_queue[ref] = label_loc
				
			else:
				print "ERROR: Label {} undeclared!".format(label)
				
			
		with open (bytecode_out, "wb") as out:
			for chunk in out_queue:
				out.write(chunk)
				
			print "Output size: {} bytes".format(out.tell())