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
#define CHILD_COUNT 2


//to store singleThreadedMergeSort params
typedef struct arg_t{
    int *mem;
    int L,M,R,num,t1_l, t1_r, t2_l, t2_r;
  } Arg;


pthread_t thread_id[THREAD_COUNT];  //initialize all threads required to merge/sort

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
void thread_sort(void * arg){
  Arg * args = (Arg *)arg;  //convert void* to Arg* so members can be accessed
  singleThreadedMergeSort(args->mem, args->L, args->R);  //3,4,5,6 sort corresponding parts of arr
}

void thread_create(void * arg){
  Arg * args = (Arg *)arg, args_arr2[2] = {{args->mem, args->t1_l, 0,args->t1_r}, {args->mem,args->t2_l,0,args->t2_r}};
  //create thread 1 and 2 to sort both halves; thread 1 creates 3 4, thread 2 creates 5 6
  pthread_create(&thread_id[2+args->num], NULL, (void *)&thread_sort, (void *)&args_arr2[0]);
  pthread_create(&thread_id[3+args->num], NULL, (void *)&thread_sort, (void *)&args_arr2[1]);
  //thread 1 waits for 3,4; thread 2 waits for 5,6
  pthread_join(thread_id[2+args->num],NULL);
  pthread_join(thread_id[3+args->num],NULL);
  //thread 1 merges left half, thread 2 merges right half
  merge(args->mem, args->L, args->M, args->R);
}

void multiThreadedMergeSort(int arr[], int left, int right)
{
  // Your code goes here

  //calculate midpoint, middle of left half, and middle of right half
  int mid = (left + right)/2, quart1 = (left + mid)/2, quart3 = (right + mid)/2;
  Arg args_arr[CHILD_COUNT] = {{arr, left, quart1, mid, 0, left, quart1, quart1+1, mid}, {arr, mid+1, quart3, right, 2, mid+1, quart3, quart3+1, right}};
  //Thread 1 creates 3,4, Thread 2 creates 5,6
  pthread_create(&thread_id[0], NULL, (void * )&thread_create, (void *) {&args_arr[0]});
  pthread_create(&thread_id[1], NULL, (void * )&thread_create, (void *) {&args_arr[1]});
  //Wait for threads to finish
  pthread_join(thread_id[0], NULL);
  pthread_join(thread_id[1], NULL);
  //Process merges left and right half
  merge(arr, left, mid, right);
}

/*
*HELPER FUNCTIONS
*/
// {{arr, (int)left, 0,(int)quart1}, {arr, (int) quart1+1,0,(int)mid}, {{arr, (int)mid+1, 0,(int)quart3}, {arr, (int) quart3+1,0,(int) right}}
/*For the threads to create other threads*/


/*For the threads to sort array*/
