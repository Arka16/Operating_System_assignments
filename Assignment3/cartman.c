
#include "cartman.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>

//to store singleThreadedMergeSort params
typedef struct arg_t{
    unsigned int C;
    enum track T;
    enum junction J;
  } Arg;

 int tracknum = 0; //total number of tracks
 sem_t sems[17];
 pthread_t *tids;



void * make_cart(void * param){
  Arg* arg = param;
  //go backwards or forwards
  int offset = arg->J == arg->T + 1 ? -1:1;
  //J1 is left if odd, right if even
  int  J1 = arg->C % 2 != 0 ? arg->J : (arg->J + offset)%tracknum;
  //J2 is right if odd, left if even
  int  J2 = arg->C % 2 != 0 ? (arg->J + offset)%tracknum : arg->J;
  if(J1 == (arg->J + offset)%tracknum && offset == -1){
    J1 = arg->J + offset;
  }
  if(J1 < 0){
    J1 = tracknum -1;
  }
  if(J2 < 0){
    J2 = tracknum -1;
  }
  if(J1 > tracknum -1){
    J1 = 0;
  }
  if(J2 > tracknum -1){
    J2 = 0;
  }
  //entry section
  sem_wait(&sems[J1]);
  sem_wait(&sems[J2]);
  reserve(arg->C, J1);
  reserve(arg->C, J2);
  //critical section
  cross(arg->C, arg->T, arg->J);
  //Exit section
  release(arg->C, J1);
  release(arg->C, J2);
  sem_post(&sems[J1]);
  sem_post(&sems[J2]);
  //free memory
  free(arg);
  pthread_exit(NULL);
}

/*
 * Callback when CART on TRACK arrives at JUNCTION in preparation for
 * crossing a critical section of track.
 */

void arrive(unsigned int cart, enum track track, enum junction junction)
{
  Arg *args = malloc(sizeof(Arg));
  args->C = cart;
  args->T = track;
  args->J = junction;
  pthread_create(&tids[cart], NULL, make_cart, args); //create thread for cart
  pthread_detach(tids[cart]); //detach thread
}

/*
 * Start the CART Manager.
 *
 * Return is optional, i.e. entering an infinite loop is allowed, but not
 * recommended. Some implementations will do nothing, most will initialise
 * necessary concurrency primitives.
 */
void cartman(unsigned int tracks)
{
  tracknum = tracks; //store the total number of tracks
  tids = malloc(sizeof(pthread_t)*tracks);
  //initialize semaphores
  for(int i = 0; i < tracks; i++){
    sem_init(&sems[i],0,1); //create all binary semaphores
  }
}
