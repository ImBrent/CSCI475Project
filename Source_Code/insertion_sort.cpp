#include "mpi.h"
#include <iostream>
#include <cstdlib>
using namespace std;
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
int PQsort(int nelements, int *elements, int pivot, MPI_Comm comm){
	int myrank, grp_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);		//find rank
	MPI_Comm_size(MPI_COMM_WORLD, &grp_size);	//find group size
	pivot = rand() % (nelements-1);			//select a pivot within the range of size
	int x = nelements % grp_size;			//x is the number of elements % number of processors
	int send_count = nelements / grp_size;		//send_count is the number of elements / number of processors
	
	if(x!=0 && myrank <= x){
		int data[send_count+1]	//number of elements to send is counting for remainders.
	}
	else{
		int data[send_count];	//Data to be sent
	}
	
	buf = (int* )malloc(send_count*sizeof(int));
	MPI_scatterv(elements, send_count,MPI_INT,buf,send_count,MPI_INT,0,MPI_COMM_WORLD);
}

int main(int argc, char *argv[]){
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);		//find rank
	MPI_Init(&argc,&argv);
	int myrank;
	int array;
	size = 20;
	if (myrank==0){
		for(int i=0;i<20;i++)
			array[i] = rand;
	}		
		
	//size = 20, array *array, pivot = 0(changes in pivot), MPI_Comm comm 
	PQsort(20, array, 0, MPI_COMM_WORLD);
	MPI_Finalize();
}


