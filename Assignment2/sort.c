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
    int L,M,R,id;
  } Arg;



int thread_num, thread_num2;

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

void thread_create2(void * arg){
  Arg * args = (Arg *)arg;
  multiThreadedMergeSort(args->mem, args->L, args->R);
}

void multiThreadedMergeSort(int arr[], int left, int right)
{
  // Your code goes here
  pthread_t thread_id[THREAD_COUNT];  //initialize all threads required to merge/sort
  int mid = (left + right)/2;
  void * f = thread_num <= 1 ? &thread_create2 : &thread_sort;
  Arg arg1 = {arr, left, (left + mid)/2, mid, thread_num};
  //P creates t1, t1 creates t3, t2 creates t5
  pthread_create(&thread_id[thread_num++], NULL, f, (void *) &arg1);
  Arg arg2 = {arr, mid + 1, (mid + right)/2, right, thread_num};
  //P creates t2, t1 creates t4, t2 creates t6
  pthread_create(&thread_id[thread_num++], NULL, f, (void *) &arg2);
  //P waits for t1, t1 waits for t3, t2 waits for t5
  pthread_join(thread_id[arg1.id], NULL);
  //P waits for t2, t1 waits for t4, t2 waits for t6
  pthread_join(thread_id[arg2.id], NULL);
  //Process merges left and right half, t1 merges left-left, left-right, t2 merges right-left, right-right
  merge(arr, left, mid, right);
}

/*
*HELPER FUNCTIONS
*/
// {{arr, (int)left, 0,(int)quart1}, {arr, (int) quart1+1,0,(int)mid}, {{arr, (int)mid+1, 0,(int)quart3}, {arr, (int) quart3+1,0,(int) right}}
/*For the threads to create other threads*/


/*For the threads to sort array*/
