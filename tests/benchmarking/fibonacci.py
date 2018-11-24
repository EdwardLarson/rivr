N = int(raw_input())

if N < 2:
	result = 1.0
	
else:
	array = [0 for i in range(0, N+1)]
	
	array[0] = 1.0
	array[1] = 1.0
	
	i = 2
	
	while True:
		x = i-1
		y = x-1
		nxt = array[x] + array[y]
		array[i] = nxt
		
		i += 1
		
		if i > N:
			break
		
	result = array[i-1]
	
print result