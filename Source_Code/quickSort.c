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
void prefixSumRearrangement(int group_size, int listSize, int *elements, int* sEndIndexList, int* lStartIndexList, int* elsAtEachProcess, int *sEndIndex, int *lStartIndex);
void partition(int* array, int size, int pivot, int *pstart, int *lstart);

void PQsort(int nelements, int *elements, MPI_Comm comm){

    if(nelements > 1){ //Base case, no sorting left to do
	int myRank, recCnt, grp_size, i, pivot, randProc, root = 0, currOffset = 0, splitIndex, smallGlobalArraySize, largeGlobalArraySize, color;
	int *localArray, *send_count, *displacements, *sEndIndexList, *lStartIndexList;
	int sEndIndex, lStartIndex, global_sEndIndex, global_lStartIndex;
	MPI_Comm splitComm;
	MPI_Comm_rank(comm, &myRank);		//find rank
	MPI_Comm_size(comm, &grp_size);	//find group size
	char outputStringTest[500];	

	if(myRank == root){
		int dataBlockSize = nelements / grp_size;		//dataBlockSize is the number of elements per processor
		int largeDataBlock = dataBlockSize + 1;			//largeDataBlock size is used when processors don't have the same number of elements
		send_count = (int*)malloc(grp_size * sizeof(int));	//Allocate space for the data that is being sent.
		int remainder = nelements % grp_size;			//remainder is the number of elements per process.
		displacements=(int* )malloc(grp_size*sizeof(int));	//displacements elements are the length for each processors array			
		
		//Determine size of data to send to each process
		for(i=0;i<grp_size;i++){
			displacements[i] = currOffset;			//Record where the data starts at
			//Record size of data, update where next data will start
			if(remainder!=0){ 				//Account for case that nelements % grp_size != 0
				send_count[i] = largeDataBlock;
				currOffset += largeDataBlock;
				remainder--;

			} else{ 					//No remainder left
				send_count[i] = dataBlockSize;
				currOffset += dataBlockSize;
			}//end else
		}//end for
	}//end if
	
	//Tell each process how much data to read
	MPI_Scatter(send_count, 1, MPI_INT, &recCnt, 1, MPI_INT, root, comm);	
	//Prepare a buffer large enough for that data
	localArray = (int*)malloc(recCnt * sizeof(int));
	
	//Distribute that data to each process
	MPI_Scatterv(elements, send_count,displacements,MPI_INT,localArray,recCnt,MPI_INT,root,comm);
	
	//Root: Select a random process
	if(myRank == root)
		randProc = rand() % grp_size;
	
	MPI_Bcast(&randProc, 1, MPI_INT, root, comm);
	
	//RandProc: Select a pivot
	if(myRank == randProc)
		pivot = localArray[rand() % recCnt];

	//Distribute the pivot to all processes
	MPI_Bcast(&pivot, 1, MPI_INT, randProc, comm);
//start testing code
	sprintf(outputStringTest, "My rank: %d\nMy initial values: ", myRank);
	for(i = 0; i < recCnt; i++)
		sprintf(outputStringTest, "%s %d", outputStringTest, localArray[i]);
//end testing code
	//Partition the arrays:
	partition(localArray, recCnt, pivot, &sEndIndex, &lStartIndex);

//Start testing code
	sprintf(outputStringTest, "%s\nMy partitioned values: ", outputStringTest);

	for(i = 0; i < recCnt; i++)
		sprintf(outputStringTest, "%s %d", outputStringTest, localArray[i]);
sleep(1);
//end testing code

	//Root gathers all partitioned values back up
	MPI_Gatherv(localArray, recCnt, MPI_INT,
				elements, send_count, displacements, MPI_INT,
				root, comm);

	//Root gathers the location of where each small subarray end
	if(myRank == root)
		sEndIndexList = (int*)malloc(grp_size*sizeof(int));
	MPI_Gather(&sEndIndex, 1, MPI_INT, sEndIndexList, 1, MPI_INT, root, comm);

	//Root gathers the location of where each large subarray starts
	if(myRank == root)
		lStartIndexList = (int*)malloc(grp_size*sizeof(int));
	MPI_Gather(&lStartIndex, 1, MPI_INT, lStartIndexList, 1, MPI_INT, root, comm);

	//Utilize the prefix sum operation(Figure 9.19) to perform global rearrangement
	if(myRank == root)
		prefixSumRearrangement(grp_size, nelements, elements, sEndIndexList, lStartIndexList, send_count, &global_sEndIndex, &global_lStartIndex);
	
//Start testing code
	sprintf(outputStringTest, "%s\nList after global rearrangement: ", outputStringTest);

	if(myRank == root){
		for(i = 0; i < nelements; i++)
			sprintf(outputStringTest, "%s %d", outputStringTest, elements[i]);
		printf("%s\n", outputStringTest);
	}
//end testing code

	//All processes need to know the indices of both the small array's end index and the large array's start index
	MPI_Bcast(&global_sEndIndex, 1, MPI_INT, root, comm);
	MPI_Bcast(&global_lStartIndex, 1, MPI_INT, root, comm);
	//Based upon these, compute the size of each array
	smallGlobalArraySize = global_sEndIndex + 1;
	largeGlobalArraySize = nelements - global_lStartIndex;
	if(grp_size > 1){
		//Determine who is going to which subgroup
		if(myRank < (((double)smallGlobalArraySize / (double)nelements) * grp_size))
			color = 0;
		else
			color = 1;
		
		MPI_Comm_split(comm, color, 0, &splitComm);

		if(color == 0){
			printf("Entering small PQsort\n");
			PQsort(smallGlobalArraySize, elements, splitComm);
			printf("Exiting small PQsort\n");
		}else{
			printf("Entering large PQsort");
			PQsort(largeGlobalArraySize, &(elements[global_lStartIndex]), splitComm);
			printf("Exiting large PQsort\n");
		}
		MPI_Barrier(splitComm);	
	 }else{
		PQsort(smallGlobalArraySize, elements, comm);
		PQsort(largeGlobalArraySize, &(elements[global_lStartIndex]), comm);	
	}
   }else{
		printf("Base case reached. Dropping out.\n");
	}
	MPI_Barrier(comm);
}
/***************************************************************************
partition
3-Way partition
Given an array, its size and a pivot
Partitions the array into three subarrays based upon the pivot value,
returns the index of where pivot and large subarrays start by reference
Passed array is modified by reference
****************************************************************************/
void partition(int* array, int size, int pivot, int *pstart, int *lstart){
	int leftIndex = -1, rightIndex = size;
	int p = -1, q = size, k;

    while(1){
	while(array[++leftIndex] < pivot) //find the first value >= pivot
		if(leftIndex == size - 1)	//If none exist, then break
			break;	//Because there is no way to partition using given pivot
		
	
	while(pivot < array[--rightIndex]) //Find first value <= pivot
		if(rightIndex == 0)
			break;
	
	if(leftIndex >= rightIndex) //No more partitioning need if this point is reached
		break;

	//Swap the contents, place greater value on right
	swap(array, leftIndex, rightIndex);

	//If pivot instance found on left index, move to start of array
	if(array[leftIndex] == pivot)
		swap(array, ++p, leftIndex);
	
	//If pivot instance found on right index, move to end of array
	if(array[rightIndex] == pivot)
		swap(array, --q, rightIndex);

    } //end while

	//Move all pivots at beginning to array[pstart]
	rightIndex = leftIndex - 1;
	for(k = -1; k < p; rightIndex--)
		swap(array, ++k, rightIndex);
	
	//Move all pivots at end to immediately follow previous set
	
	for(k = size; k > q; leftIndex++)
		swap(array, leftIndex, --k);
	printf("\n");
	
	*pstart = rightIndex;
	*lstart = leftIndex;
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
Returns the ending index of small elements and starting index of large elements by reference
************************************************************/
void prefixSumRearrangement(int group_size, int listSize, int *elements, int* sEndIndexList, int* lStartIndexList, int* elsAtEachProcess, int *sEndIndex, int *lStartIndex){
	int tempElementsList[listSize];
	int i, j, currPositionInNewArray = 0, currPositionInOldArray;
	//In order to rearrange the elements, we need to temporarily copy them elsewhere
	for(i = 0; i < listSize; i++)
		tempElementsList[i] = elements[i];
	//Now, go through each processor's array. First, put small values in the new array
	currPositionInOldArray = 0;
	for(i = 0; i < group_size; i++){
		for(j = 0; j < (sEndIndexList[i] + 1); j++)
			//Grab each smaller element
			elements[currPositionInNewArray++] = tempElementsList[currPositionInOldArray+j];
	
		//Move to next subArray
		currPositionInOldArray += elsAtEachProcess[i];
	}//end for(i)
	//All small elements packed into newElementsList now.
	//Record the size of the Smaller list
	*sEndIndex = currPositionInNewArray - 1;
	currPositionInOldArray = 0;
	//Repeat for pivots
	for(i = 0; i < group_size; i++){
		for(j = sEndIndexList[i] + 1; j < lStartIndexList[i]; j++)
			//Grab each pivot element
			elements[currPositionInNewArray++] = tempElementsList[currPositionInOldArray + j];
			
		currPositionInOldArray += elsAtEachProcess[i];
	}//end for(i)
	*lStartIndex = currPositionInNewArray;
	//Now, do the same thing for the larger elements
	currPositionInOldArray = 0;
	for(i = 0; i < group_size; i++){
		for(j = lStartIndexList[i]; j < elsAtEachProcess[i]; j++)
			//Grab each larger element
			elements[currPositionInNewArray++] = tempElementsList[currPositionInOldArray+j];
		
		//Move to next subArray
		currPositionInOldArray += elsAtEachProcess[i];
	}//end for(i)
	//elements now has expected output.
}//end prefixSumRearrangement

int main(int argc, char *argv[]){
	MPI_Init(&argc,&argv);
	srand(time(0)); //Seed rng
	int i, size, myrank;
	size = 20;
	int array [size];
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	if (myrank==0){
		for(i=0;i<size;i++)
			array[i] = rand()%size;
	}		

	if(myrank==0){
		display(array,size);
		printf("\n");
	}

	PQsort(size, array, MPI_COMM_WORLD);
	if(myrank == 0){
		printf("Final array: \n");
		display(array,size);
		printf("\n");
	}//end if
	MPI_Finalize();
}

