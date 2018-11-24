fibonacci.c, fibonacci.b, fibonacci.java, and fibonacci.py all implement the same Nth-fibonacci algorithm:

get N from stdin
if N < 2:
	print 1
else:
	allocate an array of size N+1
	array[0] = 1
	array[1] = 1
	for i from 0 to N:
		array[i] = array[i-1] + array[i-2]
	print array[N]

fibonacci.c must be compiled by a C compiler.
fibonacci.b is already valid Rivr bytecode.
fibonacci.java must be compiled by a Java compiler.
fibonacci.py can be run by a Python2 interpreter.

fib_in.txt contains the string "10000000" and can be used as stdin for any of the above programs, so that each will compute the ten-millionth member of fibonacci series
In reality, this number is far larger than a double-precision floating point number can hold, so the result will be 'Inf'.
However we are only interested in how long the program takes to complete, not the actual result.

One way to time each of these programs is the 'time' terminal command, e.g.

>> time ./rivr run tests/benchmarking/fibonacci.b < tests/benchmarking/fib_in.txt
