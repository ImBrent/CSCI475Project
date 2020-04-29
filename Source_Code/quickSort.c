#include<stdlib.h>
#include<stdio.h>
#include<time.h>
#include <string.h>
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

void swap(int* array, int index1, int index2);

int PQsort(int nelements, int *elements, MPI_Comm comm){
	int myRank, recCnt, grp_size, i, pivot, randProc, root = 0, currOffset = 0, splitIndex;
	int *localArray, *send_count, *displacements;
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);		//find rank
	MPI_Comm_size(MPI_COMM_WORLD, &grp_size);	//find group size
	

	if(myRank == root){
		int dataBlockSize = nelements / grp_size;			//y is the number of elements / number of processors
		int largeDataBlock = dataBlockSize + 1;
		send_count = (int*)malloc(grp_size * sizeof(int));	//The data that is being sent
		int remainder = nelements % grp_size;			//x is the number of elements % number of processors
		displacements=(int* )malloc(grp_size*sizeof(int));//dspls has an index for each processor			
		
		//Determine size of data to send to each process
		for(i=0;i<grp_size;i++){
			displacements[i] = currOffset; //Record where the data starts at
			//Record size of data, update where next data will start
			if(remainder!=0){ //Account for case that nelements % grp_size != 0
				send_count[i] = largeDataBlock;
				currOffset += largeDataBlock;
				remainder--;
			} else{ //No remainder left
				send_count[i] = dataBlockSize;
				currOffset += dataBlockSize;
			}//end else
		}
	}//end if
	
	//Tell each process how much data to read
	MPI_Scatter(send_count, 1, MPI_INT, &recCnt, 1, MPI_INT, root, MPI_COMM_WORLD);	
	//Prepare a buffer large enough for that data
	localArray = (int*)malloc(recCnt * sizeof(int));
	
	//Distribute that data to each process
	MPI_Scatterv(elements, send_count,displacements,MPI_INT,localArray,recCnt,MPI_INT,0,MPI_COMM_WORLD);

	
	//Root: Select a random process
	if(myRank == root)
		randProc = rand() % grp_size;
	
	MPI_Bcast(&randProc, 1, MPI_INT, root, MPI_COMM_WORLD);
	
	//RandProc: Select a pivot
	if(myRank == randProc){
		pivot = localArray[rand() % recCnt];
		printf("Pivot selected: %d\n", pivot);
	}//end if
	//Distribute the pivot to all processes
	MPI_Bcast(&pivot, 1, MPI_INT, randProc, MPI_COMM_WORLD);


	if(myRank == root)
		printf("\nValues of all processes before and after");
MPI_Barrier(MPI_COMM_WORLD);
	char* outputStringTest = (char*)malloc(500);
	sprintf(outputStringTest, "My rank: %d\nMy initial values: ", myRank);
	for(i = 0; i < recCnt; i++)
		sprintf(outputStringTest, "%s %d", outputStringTest, localArray[i]);

	//Partition the arrays:
	splitIndex = partition(localArray, recCnt, pivot);


//TESTING: Everybody output their newly partitioned arrays:

MPI_Barrier(MPI_COMM_WORLD);
	sprintf(outputStringTest, "%s\nMy partitioned values: ", outputStringTest);

	for(i = 0; i < recCnt; i++)
		sprintf(outputStringTest, "%s %d", outputStringTest, localArray[i]);
	printf("%s\n", outputStringTest);
/*
not final result yet... still in progress
Gather
we need to put the elements back, but we need to keep the elements <=
than the pivot seperate from the > ones.

*/

	MPI_Barrier(MPI_COMM_WORLD);
	int j, size;
		int *buf;
		int sendCount;
		int* recvcounts;
		int* displs;
	if (myRank == 0) {
		
		recvcounts=(int* )malloc(splitIndex*sizeof(int));

		for(i=0; i<splitIndex; i++)
			recvcounts[i] = localArray[i];
	
		displs=(int* )malloc(grp_size*sizeof(int));
	
		for(i=0; i<grp_size; i++){
			displs[i] = 0;
			for(j=0; j<i; j++)
				displs[i] += recvcounts[j];
		}

		size=sendCount*sizeof(int);
		buf  = (int *)malloc(size); /*allocate receiving memory*/
	}
	 
	MPI_Gatherv(localArray, splitIndex, MPI_INT, buf, recvcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
	if(myRank==root)
		display(buf,20);
	//MPI_Gatherv(data,myRank+1,MPI_INT,buf,recvcounts, displs, MPI_INT,0,MPI_COMM_WORLD);



}

}
/***************************************************************************
partition
Given an array, its size and a pivot
Partitions the array into two subarrays based upon the pivot value,
returns the index of where the second subarray starts by value.
Passed array is modified by reference
****************************************************************************/
int partition(int* array, int size, int pivot){
	int leftIndex = 0, rightIndex;
	for(rightIndex = 0; rightIndex < size; rightIndex++)
		//If current element is smaller than or equal to pivot, place in left subarray
		if(array[rightIndex] <= pivot)
			swap(array, leftIndex++, rightIndex);
	return leftIndex; //leftIndex has location in which second subarray starts
}//end partition

/*************************************************************
swap
Given array, two indices, swap the values at the two indices
*************************************************************/
void swap(int* array, int index1, int index2){
	int temp = array[index1];
	array[index1] = array[index2];
	array[index2] = temp;
}//end swap

void display(int *p,int size){
	int i;
	for(i=0;i<size;i++)
		printf(" %d ", p[i]);
	//printf("%d\n");
}

int main(int argc, char *argv[]){
	MPI_Init(&argc,&argv);
	srand(time(0)); //Seed rng
	int myrank,i, size, grp_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);		//find rank
	MPI_Comm_size(MPI_COMM_WORLD, &grp_size);	//find group size
	size = 20;
	int array[20];
	if (myrank==0){
		for(i=0;i<size;i++)
			array[i] = rand()%size;
	}		
		
	if(myrank==0){
		display(array,size);
		printf("\n");
	}

	PQsort(size, array, MPI_COMM_WORLD);
	
	MPI_Finalize();
}

