/************************************************************************
 *
 * CSE130 Assignment 1
 *
 * POSIX Shared Memory Multi-Process Merge Sort
 *
 * Copyright (C) 2020-2021 David C. Harrison. All right reserved.
 *
 * You may not use, distribute, publish, or modify this code without
 * the express written permission of the copyright holder.
 *
 ************************************************************************/

//SOURCES USED: https://pubs.opengroup.org/onlinepubs/007904875/functions/shm_open.html,
//              https://www.geeksforgeeks.org/posix-shared-memory-api/
//              https://www.geeksforgeeks.org/input-output-system-calls-c-create-open-close-read-write/
//              https://www.ibm.com/docs/en/i/7.4?topic=ssw_ibm_i_74/apis/munmap.htm
//used the above sources to get familiar with posix documentation and look up which .h files to include
//quick google search for other minor documentation references/header files


#include "merge.h"
#include <sys/mman.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>

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
  int buffer_size = (right+1) * sizeof(int);  //size of local array
  int mid = (left + right)/2;   //middle part of the array for LEFT/RIGHT split
  //create shared memory
  int shmfd = shm_open("shmseg", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  ftruncate(shmfd, buffer_size);  //set size of shared memory
  //Attach to shared memory
  int *ptr =  mmap(0, buffer_size, PROT_WRITE, MAP_SHARED, shmfd, 0);
  //copy array to shared memory
  memcpy(ptr, arr, buffer_size);
  switch(fork()){
    case -1:        //error
      exit(-1);
    case 0:        //child
      ptr =  mmap(0, buffer_size, PROT_WRITE, MAP_SHARED, shmfd, 0);   //attach shared memory
      singleProcessMergeSort(ptr, left, mid);                         //sort left side
      munmap(ptr, buffer_size);                                       //detach memory
      exit(-1);     //exit
    default:       //parent
      singleProcessMergeSort(ptr, mid+1, right); //sort right side
      wait(NULL);
      merge(ptr, left, mid, right);          // merge both sorted sides
      ptr =  mmap(0, buffer_size, PROT_WRITE, MAP_SHARED, shmfd, 0);   //attach shared memory
      memcpy(arr, ptr, buffer_size);  //copy shared memory to array
      munmap(ptr, buffer_size);      //detach memory
      //destroy shared memory
      shm_unlink("shmseg");
      close(shmfd);
  }
}
