# rivr

Programming language built on a register-based VM.

The river VM has a stack of frames, where each frame has 128 registers.
64 of these registers are for local variables, 32 are used for arguments written from the previous frame, and 32 are for returns written to from the next frame.
