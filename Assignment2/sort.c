//SOURCES: https://www.educative.io/edpresso/how-to-create-a-simple-thread-in-c
//https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html#BASICS
//https://www.geeksforgeeks.org/multithreading-c-2/
//links from assignment2 handout

//All the sources above used to get familiar with the p_thread functions and know the purpose
//of each
//Also used TA OH for debugging


#include "merge.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define THREAD_COUNT 6
#define POS_COUNT 8
#define CHILD_COUNT 2

//to store singleThreadedMergeSort params
typedef struct arg_t{
    int *mem;
    int L,M,R,num;
  } Arg;

void * thread_create(void * args);
void * thread_sort(void * arg);
pthread_t thread_id[THREAD_COUNT];  //initialize all threads required to merge/sort
Arg args_arr[THREAD_COUNT];       //store sort args in an array for loop


/* LEFT index and RIGHT index of the sub-array of ARR[] to be sorted */
void singleThreadedMergeSort(int arr[], int left, int right)
{
  if (left < right) {
    int middle = (left+right)/2;
    singleThreadedMergeSort(arr, left, middle);
    singleThreadedMergeSort(arr, middle+1, right);
    merge(arr, left, middle, right);
  }
}

/*
 * This function stub needs to be completed
 */
void multiThreadedMergeSort(int arr[], int left, int right)
{
  // Your code goes here

  //calculate midpoint, middle of left half, and middle of right half
  int mid = (left + right)/2, quart1 = (left + mid)/2, quart3 = (right + mid)/2;
  int Pos[POS_COUNT] = {left, quart1, mid, quart3, right, left, mid+1, left};

  for(int i = 0; i < THREAD_COUNT; i++){
    args_arr[i].mem = arr;    //store local memory
    args_arr[i].L = i < 3 ? Pos[i+5] : Pos[i-2]+1;  //store left endpoint
    args_arr[i].R = i < 2 ? Pos[2*i+2] : Pos[i-1]; //store right endpoint
    args_arr[i].M = i < 2 ? Pos[2*i+1] : 0;  //store midpoint for merging
    args_arr[i].num = 2*i;   //offest for grandchild threads
  }

  for(int i = 0; i < CHILD_COUNT; i++){
     //Thread 1 creates 3,4, Thread 2 creates 5,6
     pthread_create(&thread_id[i], NULL, thread_create, (void *) &args_arr[i]);
  }
  for(int i = 0; i < CHILD_COUNT; i++){
    //Process waits for Thread 1 and 2
    pthread_join(thread_id[i], NULL);
  }
  //Process merges left and right half
  merge(arr, left, mid, right);
}

/*
*HELPER FUNCTIONS
*/

/*For the threads to create other threads*/
void * thread_create(void * arg){
  Arg * args = (Arg *)arg;
  for(int i = CHILD_COUNT; i < 4; i++){
    //create thread 1 and 2 to sort both halves; thread 1 creates 3 4, thread 2 creates 5 6
    pthread_create(&thread_id[args->num+i], NULL, thread_sort, (void *)&args_arr[args->num+i]);
  }
  for(int i = CHILD_COUNT; i < 4; i++){
    //thread 1 waits for 3,4; thread 2 waits for 5,6
    pthread_join(thread_id[args->num+i],NULL);
  }
  //thread 1 merges left half, thread 2 merges right half
  merge(args->mem, args->L, args->M, args->R);
  return NULL;
}

/*For the threads to sort array*/
void * thread_sort(void * arg){
  Arg * args = (Arg *)arg;  //convert void* to Arg* so members can be accessed
  singleThreadedMergeSort(args->mem, args->L, args->R);  //3,4,5,6 sort corresponding parts of arr
  return NULL;
}
