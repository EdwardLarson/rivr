#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv){
	long int N;
	fscanf(stdin, "%ld", &N);
	
	double result;
	
	if (N < 2){
		result = 1.0d;
		
	}else{
		double* array = calloc(N+1, sizeof(long int));
		array[0] = 1.0d;
		array[1] = 1.0d;
		
		int i, x, y;
		double nxt;
		
		i = 2;
		
		do{
			x = i-1;
			y = x-1;
			nxt = array[x] + array[y];
			array[i] = nxt;
			
			i++;
			
		}while (i <= N);
		
		result = array[i-1];
		
	}
	
	printf("%lf", result);
	
	return 0;
}