/*CSCI 475 
insertion sort
Donivan Anderson
Brent Clapp*/

/* Steps to take for insertion sort
1. We need an array, select the pivot, divide into processes
2. we separate the elements <= pivot into front of each processes
3. move all elements <= pivot into front of entire array
4. assign elements to processes (but we need to keep processes separate from last pivot)
5. repeat to 1 until each process is sorted.
*/
PQsort(int nelements, int *elements, int pivot, MPI_Comm comm){
}

int main(int argc, char *argv[]){
	int myrank;
	int array{7,13,18,2,17,1,14,20,6,10,15,9,3,16,19,4,11,12,5,8};//This array is the same one in the book
	//is is only temp
	PQsort(20,
}