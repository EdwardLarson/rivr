import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

class Main{
	public static void main(String argv[]){
		int N;
		try{
			BufferedReader bufferRead = new BufferedReader(new InputStreamReader(System.in));
			N = Integer.parseInt(bufferRead.readLine());
		}catch (IOException e){
			e.printStackTrace();
			return;
		}
		
		double result;
		
		if (N < 2){
			result = 1.0;
			
		}else{
			double[] array = new double[N+1];
			
			array[0] = 1.0;
			array[1] = 1.0;
			
			int i, x, y;
			double nxt;
			
			i = 2;
			
			do{
				x = i-1;
				y = x-1;
				nxt = array[x] + array[y];
				array[i] = nxt;
				
				i++;
				
			}while(i <= N);
				
			result = array[i-1];
			
		}
		
		System.out.println(Double.toString(result));
		
	}

}