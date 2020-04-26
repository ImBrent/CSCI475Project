#include<stdlib.h>
#include<stdio.h>
#include "mpi.h"

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
	int myrank, grp_size,i;
	int *buf;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);		//find rank
	MPI_Comm_size(MPI_COMM_WORLD, &grp_size);	//find group size
	pivot = rand() % (nelements-1);			//select a pivot within the range of size
	int y = nelements / grp_size;			//y is the number of elements / number of processors
	int *send_count[y];				//The data that is being sent
	int x = nelements % grp_size;			//x is the number of elements % number of processors
	int *displs=(int* )malloc(grp_size*sizeof(int));//dspls has an index for each processor	
	buf = (int* )malloc(y*sizeof(int));	//buf has an index for each element
	
	for(i=0;i<grp_size;i++){
		if(x!=0 && myrank <= x)
			displs[i] = y+1;	//number of elements to send is counting for remainders.
		else
			displs[i] = y;	//Data to be sent
	}
	
	MPI_Scatterv(elements, send_count,displs,MPI_INT,buf,send_count,MPI_INT,0,MPI_COMM_WORLD);
}

void display(int *p,int size){
	int i;
	for(i=0;i<size;i++)
		printf(" %d", p[i]);
	printf("%d\n");
}

int main(int argc, char *argv[]){
	MPI_Init(&argc,&argv);
	int myrank,i, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);		//find rank
	size = 20;
	int array[20];
	if (myrank==0){
		for(i=0;i<size;i++)
			array[i] = rand()%size;
	}		
		
	//size = size, array array, pivot = 0(changes in pivot), MPI_Comm comm 
	//PQsort(size, array, 0, MPI_COMM_WORLD);
	if(myrank==0)
		display(array,size);
	
	MPI_Finalize();
	return 0;
}


