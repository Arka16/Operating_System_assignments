/************************************************************************
 *
 * CSE130 Assignment 1
 *
 * UNIX Shared Memory Multi-Process Merge Sort
 *
 * Copyright (C) 2020-2021 David C. Harrison. All right reserved.
 *
 * You may not use, distribute, publish, or modify this code without
 * the express written permission of the copyright holder.
 *
 ************************************************************************/
//SOURCES: https://www.tutorialspoint.com/inter_process_communication/inter_process_communication_shared_memory.htm
//         https://www.geeksforgeeks.org/ipc-shared-memory/
//         used the to get familiar with documentation
//         shm code examples from Thursdays lecture and sauce pseudocode for structure
//         quick google search for other minor documentation references/header files (no specific websites)

#include "merge.h"
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
/*
 * Merge Sort in the current process a sub-array of ARR[] defined by the
 * LEFT and RIGHT indexes.
 */
void singleProcessMergeSort(int arr[], int left, int right)
{
  if (left < right) {
    int middle = (left+right)/2;
    singleProcessMergeSort(arr, left, middle);
    singleProcessMergeSort(arr, middle+1, right);
    merge(arr, left, middle, right);
  }
}

/*
 * Merge Sort in the current process and at least one child process a
 * sub-array of ARR[] defined by the LEFT and RIGHT indexes.
 */
void multiProcessMergeSort(int arr[], int left, int right)
{
  // Your code goes here
  int mid = (left + right)/2;   //middle part of the array for LEFT/RIGHT split
  int buffer_size = (right+1) * sizeof(int);   //number of bytes of local array
  //create shared memory
  key_t key = ftok("shmseg", 34);
  int shmid = shmget(key, buffer_size, IPC_CREAT|0666);
  //Attach to shared memory
  int *str = (int *) shmat(shmid, (void *)0, 0);
  //copy array to shared memory
  memcpy(str, arr, buffer_size);
  switch(fork()){
    case -1:        //error
      exit(-1);
    case 0:        //child
      str = (int *) shmat(shmid, (void *)0, 0);   //attach shared memory
      singleProcessMergeSort(str, left, mid);    //sort left side
      shmdt(str);   //detach memory
      exit(-1);
    default:       //parent
      singleProcessMergeSort(str, mid+1, right); //sort right side
      wait(NULL);
      merge(str, left, mid, right);                // merge both sorted sides
      str = (int *) shmat(shmid, (void *)0, 0);   //attach shared memory
      memcpy(arr, str, buffer_size);             //copy shared memory to array
      shmdt(str);   //detach memory
      shmctl(shmid, IPC_RMID, NULL); //destroy shared memory
  }
}
