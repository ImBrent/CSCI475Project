#include<stdlib.h>
#include <unistd.h>
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

void display(int *p,int size);
void swap(int* array, int index1, int index2);
int prefixSumRearrangement(int group_size, int listSize, int **elements, int* splitLocations, int* elsAtEachProcess);

int PQsort(int nelements, int *elements, MPI_Comm comm){
	int myRank, recCnt, grp_size, i, pivot, randProc, root = 0, currOffset = 0, splitIndex, largeArrayStartIndex;
	int *localArray, *send_count, *displacements, *splitLocations;
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
		printf("Pivot selected: %d\n", pivot); //**** TESTING CODE
	}//end if
	//Distribute the pivot to all processes
	MPI_Bcast(&pivot, 1, MPI_INT, randProc, MPI_COMM_WORLD);

//Start testing code
	if(myRank == root)
		printf("\nValues of all processes before and after\n");
sleep(1);
	char* outputStringTest = (char*)malloc(500);
	sprintf(outputStringTest, "My rank: %d\nMy initial values: ", myRank);
	for(i = 0; i < recCnt; i++)
		sprintf(outputStringTest, "%s %d", outputStringTest, localArray[i]);
//end testing code
	//Partition the arrays:
	splitIndex = partition(localArray, recCnt, pivot);

//Start testing code
	sprintf(outputStringTest, "%s\nMy partitioned values: ", outputStringTest);

	for(i = 0; i < recCnt; i++)
		sprintf(outputStringTest, "%s %d", outputStringTest, localArray[i]);
	printf("%s\n", outputStringTest);

//end testing code

	//Root gathers all partitioned values back up
	MPI_Gatherv(localArray, recCnt, MPI_INT,
				elements, send_count, displacements, MPI_INT,
				root, MPI_COMM_WORLD);

	//Root gathers the location of where each subarray "splits" at
	if(myRank == root)
		splitLocations = (int*)malloc(grp_size*sizeof(int));
	MPI_Gather(&splitIndex, 1, MPI_INT, splitLocations, 1, MPI_INT, root, MPI_COMM_WORLD);

	//Utilize the prefix sum operation(Figure 9.19) to perform global rearrangement
	if(myRank == root)
		largeArrayStartIndex = prefixSumRearrangement(grp_size, nelements, &elements, splitLocations, send_count);
	
	sleep(1);
	if(myRank == root){
		printf("New list of elements: ");
		display(elements, nelements);
		printf("\n");
	}//end if
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

/*********************************************************
prefixSumRearrangement
Places the smaller segmenets at the beginning of the list of elements,
and places the larger segments at the end of the list of elements
Returns the starting index of the segment of larger elements.
************************************************************/
int prefixSumRearrangement(int group_size, int listSize, int **elements, int* splitLocations, int* elsAtEachProcess){
	int *newElementsList;
	int i, j, currPositionInNewArray = 0, currPositionInOldArray, smallListSize;
	newElementsList = (int*)malloc(listSize * sizeof(int));

	
	//Now, go through each processor's array. First, put small values in the new array
	currPositionInOldArray = 0;
	for(i = 0; i < group_size; i++){
		for(j = 0; j < splitLocations[i]; j++){
			//Grab each smaller element
			newElementsList[currPositionInNewArray] = (*elements)[currPositionInOldArray+j];
			currPositionInNewArray++;
		}//end for(j)
		//Move to next subArray
		currPositionInOldArray += elsAtEachProcess[i];
	}//end for(i)
	//All small elements packed into newElementsList now.
	//Record the size of the Smaller list
	smallListSize = currPositionInNewArray;
	currPositionInOldArray = 0;
	//Now, do the same thing for the larger elements
	for(i = 0; i < group_size; i++){
		for(j = splitLocations[i]; j < elsAtEachProcess[i]; j++){
			//Grab each smaller element
			newElementsList[currPositionInNewArray] = (*elements)[currPositionInOldArray+j];
			currPositionInNewArray++;
		}//end for(j)
		//Move to next subArray
		currPositionInOldArray += elsAtEachProcess[i];
	}//end for(i)
	//newElementsList now has the expected output. Return the size of the smaller half.
	*elements = newElementsList;
	return smallListSize;
}//end prefixSumRearrangement

int main(int argc, char *argv[]){
	MPI_Init(&argc,&argv);
	srand(time(0)); //Seed rng
	int myrank,i, size, grp_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);		//find rank
	MPI_Comm_size(MPI_COMM_WORLD, &grp_size);	//find group size
	size = 20;
	int array[size];
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

