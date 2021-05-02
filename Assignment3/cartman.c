
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
    //go backwards
    int offset = arg->J == arg->T + 1 ? -1:1;
    //entry section
    if(arg->C % 2 == 0){
      //pick up left
      sem_wait(&sems[arg->J]);
      reserve(arg->C, arg->J);
      //pick up right
      sem_wait(&sems[(arg->J+ offset)%tracknum]);
      reserve(arg->C, (arg->J + offset)%tracknum);
      //critical section
      cross(arg->C, arg->T, arg->J);
      //Exit section
      release(arg->C, arg->J);
      sem_post(&sems[arg->J]);
      release(arg->C, (arg->J + offset)%tracknum);
      sem_post(&sems[(arg->J + offset)% tracknum]);
    }
    else{
      //pick up right
      sem_wait(&sems[(arg->J+ offset)%tracknum]);
      reserve(arg->C, (arg->J + offset)%tracknum);
      //pick up left
      sem_wait(&sems[arg->J]);
      reserve(arg->C, arg->J);
      //critical section
      cross(arg->C, arg->T, arg->J);
      //Exit section
      release(arg->C, (arg->J + offset)%tracknum);
      sem_post(&sems[(arg->J + offset)% tracknum]);
      release(arg->C, arg->J);
      sem_post(&sems[arg->J]);
    }
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
  tids = malloc(sizeof(pthread_t)*tracks + 1);
  //initialize semaphores
  for(int i = 0; i < tracks; i++){
    sem_init(&sems[i],0,1); //create all binary semaphores
  }
}
