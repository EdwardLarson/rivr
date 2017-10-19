# rivr

Programming language built on a register-based VM.

The river VM has a stack of frames, where each frame has 128 registers.
64 of these registers are for local variables, 32 are used for arguments written from the previous frame, and 32 are for returns written to from the next frame.

## Compilation & Operation
### Parser
gcc src/parser.c -o parse -D PARSER_ONLY

./parse <input-file> will parse the input file first into raw tokens, which are displayed. Pressing enter will process those tokens into typed tokens ready to be organized into an Abstract Syntax Tree. The typed tokens will be displayed as well. Try running it on meta/rivr_specs.txt

As of latest commit, no other source files have meaningful executables.
