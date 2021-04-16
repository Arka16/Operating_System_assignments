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
#define SORT_COUNT 4
#define MERGE_COUNT 2

//to store singleThreadedMergeSort params
typedef struct arg_t{
    int *mem;
    int L;
    int R;
  } Arg_S;

//to store merge params
typedef struct arg_t2{
    int *mem;
    int L;
    int M;
    int R;
  } Arg_M;

void * thread_sort(void * arg);
void * thread_merge(void * arg);


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


void * thread_sort(void * arg){
  Arg_S * args = (Arg_S *)arg;  //convert void* to Arg* so members can be accessed
  singleThreadedMergeSort(args->mem, args->L, args->R);  //sort arr
  return NULL;
}

void * thread_merge(void * arg){
  Arg_M * args = (Arg_M *)arg;  //convert void* to Arg_M so members can be accessed
  merge(args->mem, args->L, args->M, args->R);
  return NULL;
}

/*
 * This function stub needs to be completed
 */
void multiThreadedMergeSort(int arr[], int left, int right)
{
  // Delete this line, it's only here to fail the code quality check

  // Your code goes here
  int mid = (left + right)/2;   //middle part of the array for LEFT/RIGHT split
  int quart1 = (left + mid)/2;   //1/4 pos of the array
  int quart3 = (right + mid)/2;  //3/4 pos of the array

  // 4 arg structs for threads 1-4. contains respective arguments for singleThreadMergeSort
  Arg_S arg_s1, arg_s2, arg_s3, arg_s4;

  arg_s1.mem = arr; //store local array
  arg_s1.L = left;  //store left endpoint
  arg_s1.R = quart1; // store right endpoint

  arg_s2.mem = arr;
  arg_s2.L = quart1 + 1;
  arg_s2.R = mid;

  arg_s3.mem = arr;
  arg_s3.L = mid+1;
  arg_s3.R = quart3;

  arg_s4.mem = arr;
  arg_s4.L = quart3 + 1;
  arg_s4.R = right;

  //merge arguments for threads 5-6
  Arg_M arg_m1, arg_m2;
  arg_m1.mem = arr;    // merge first half
  arg_m1.L = left;
  arg_m1.M = quart1;
  arg_m1.R = mid;

  arg_m2.mem = arr;  //merge second half
  arg_m2.L = mid+1;
  arg_m2.M = quart3;
  arg_m2.R = right;

  Arg_S  args[SORT_COUNT]; //store sort args in an array for loop
  args[0] = arg_s1;
  args[1] = arg_s2;
  args[2] = arg_s3;
  args[3] = arg_s4;

  Arg_M args_m[MERGE_COUNT];  //store merge arguments in an array for loop
  args_m[0] = arg_m1;
  args_m[1] = arg_m2;

  //initialize all threads required to merge/sort
  pthread_t thread_id[THREAD_COUNT];

  int i = 0;  //loop counter

  /*create 4 threads where each sorts a quarter of arr*/
  for(i =0; i < SORT_COUNT; i++){
    pthread_create(&thread_id[i], NULL, thread_sort, (void *) &args[i]);
  }
  /*wait for sorting threads to finish*/
  for(i =0; i < SORT_COUNT; i++){
    pthread_join(thread_id[i], NULL);
  }
  /*create 2 threads to merge each half*/
  for(i =4; i < THREAD_COUNT; i++){
    pthread_create(&thread_id[i], NULL, thread_merge, (void *) &args_m[i-4]);
  }
  /*wait for merging threads to finish*/
  for(i =4; i < THREAD_COUNT; i++){
    pthread_join(thread_id[i], NULL);
  }
  //process merges to form sorted arr
  merge(arr, left, mid, right);
}
