//SOURCES: TA OH, lecture slides scheduling 1 to understand algorithms better, secret sauce,
/**
 * See scheduler.h for function details. All are callbacks; i.e. the simulator
 * calls you when something interesting happens.
 */
#include <stdlib.h>
#include "simulator.h"
#include "scheduler.h"
#include "queue.h"
#include <stdio.h>

/**
 * Initalise a scheduler implemeting the requested ALGORITHM. QUANTUM is only
 * meaningful for ROUND_ROBIN.
 */
 //to store relavant stats
typedef struct thread_extra_t {
  int tid;
  int arrive_time;
  int finish_time;
  int start_time;
  int waiting_time;
  int remaining_time;
  int tick_count;
  thread_t *t;
} thread_extra_t;

void *ready_queue;    //ready queue for scheduling
void *all_threads;   //keep track of every thread created
int alg;            //store algorithms globally
int quant;         //store quantum for RR globally
thread_t *current_thread;

//queue find helper function
static bool inner_equalitor(void *outer, void *inner) {
    return ((thread_extra_t*)outer)->tid == ((thread_t*)inner)->tid;
  }

//helper function to sort
static int priority_sort(void *a, void *b) {
        return ((thread_t*)a)->priority - ((thread_t*)b)->priority;
    }

//sort by shortest CPU time length
static int length_sort(void *a, void *b) {
        return ((thread_t*)a)->length - ((thread_t*)b)->length;
    }
//sort by remaining burst time left on CPU
static int remaining_sort(void *a, void *b) {
      thread_t* t1 = (thread_t*)a;
      thread_t* t2 = (thread_t*)b;
      thread_extra_t * a1 =  queue_find(all_threads,inner_equalitor, t1);
      thread_extra_t * b1 = queue_find(all_threads,inner_equalitor, t2 );
      return ((thread_extra_t*)a1)->remaining_time - ((thread_extra_t*)b1)->remaining_time;
    }

void scheduler(enum algorithm algorithm, unsigned int quantum) {
    current_thread = NULL;
    alg = algorithm; //store algorithm
    quant = quantum;
    ready_queue = queue_create(); //for scheduling purposes
    all_threads = queue_create(); //for stats
}

/**
 * Programmable clock interrupt handler. Call sim_time() to find out
 * what tick this is. Called after all calls to sys_exec() for this
 * tick have been made.
 */
void tick() {
    //sort by priority if algorithm is priority based
    if(alg == NON_PREEMPTIVE_PRIORITY || alg == PREEMPTIVE_PRIORITY){
      queue_sort(ready_queue, priority_sort);
    }
    //sort by job length if algorithm is based on SJ
    if(alg == NON_PREEMPTIVE_SHORTEST_JOB_FIRST || alg == PREEMPTIVE_SHORTEST_JOB_FIRST){
      queue_sort(ready_queue, length_sort);
    }
    //sort by remaining time if algorithm is based on SRT
    if(alg == NON_PREEMPTIVE_SHORTEST_REMAINING_TIME_FIRST || alg == PREEMPTIVE_SHORTEST_REMAINING_TIME_FIRST){
      queue_sort(ready_queue, remaining_sort);
      if(current_thread != NULL){
        thread_extra_t *current_rf = queue_find(all_threads,inner_equalitor,current_thread);
        current_rf->remaining_time--;  //decrement time remaining each tick it's on CPU
      }
    }
    if(alg == ROUND_ROBIN && current_thread != NULL){
      thread_extra_t *t_extra7 = queue_find(all_threads,inner_equalitor,current_thread);
      t_extra7->tick_count++;
      if(queue_size(ready_queue)==0 && t_extra7->tick_count == quant){     //if no threads in ready queue
        t_extra7->tick_count = 0; //reset tick
      }
      else if(t_extra7->tick_count == quant){  //if current thread expires quantum, preempt
         t_extra7->tick_count = 0;
         queue_enqueue(ready_queue, current_thread);
         thread_t * t_rr = (thread_t*)queue_dequeue(ready_queue);
         thread_extra_t *current_rr = queue_find(all_threads,inner_equalitor,t_rr);
         thread_extra_t *preempt_rr = queue_find(all_threads,inner_equalitor,current_thread);
         current_rr->tick_count = 0;
         current_rr->waiting_time += sim_time() - current_rr->start_time;
         preempt_rr->start_time = sim_time();
         sim_dispatch(t_rr);
         current_thread = t_rr;
      }
    }
    if((alg == PREEMPTIVE_PRIORITY) && queue_size(ready_queue)!= 0){
      thread_t *t2 = (thread_t*)queue_head(ready_queue);
      if(current_thread != NULL && current_thread->priority > t2->priority){
         thread_extra_t *t_extra = queue_find(all_threads,inner_equalitor,t2);
         thread_extra_t *t_extra2 = queue_find(all_threads,inner_equalitor,current_thread);
         t_extra->waiting_time += sim_time() - t_extra->start_time;
         t_extra2->start_time = sim_time();
         queue_enqueue(ready_queue, current_thread);
         queue_dequeue(ready_queue);
         sim_dispatch(t2);
         current_thread = t2;
    }
  }
  if((alg ==  PREEMPTIVE_SHORTEST_JOB_FIRST) && queue_size(ready_queue)!= 0){
      thread_t *t2 = (thread_t*)queue_head(ready_queue);
      if(current_thread != NULL && current_thread->length > t2->length){
         thread_extra_t *t_extra = queue_find(all_threads,inner_equalitor,t2);
         thread_extra_t *t_extra2 = queue_find(all_threads,inner_equalitor,current_thread);
         t_extra->waiting_time += sim_time() - t_extra->start_time;
         t_extra2->start_time = sim_time();
         queue_enqueue(ready_queue, current_thread);
         queue_dequeue(ready_queue);
         sim_dispatch(t2);
         current_thread = t2;
    }
  }
  if((alg ==  PREEMPTIVE_SHORTEST_REMAINING_TIME_FIRST) && queue_size(ready_queue)!= 0){
      thread_t *t2 = (thread_t*)queue_head(ready_queue);
      thread_extra_t *next_pjrt = queue_find(all_threads,inner_equalitor,t2);
      if(current_thread != NULL){
        thread_extra_t *cur_pjrt = queue_find(all_threads,inner_equalitor,current_thread);
        if(next_pjrt->remaining_time <= cur_pjrt->remaining_time){
           next_pjrt->waiting_time += sim_time() - next_pjrt->start_time;
           cur_pjrt->start_time = sim_time();
           queue_enqueue(ready_queue, current_thread);
           queue_dequeue(ready_queue);
           sim_dispatch(t2);
           next_pjrt->remaining_time--;
           cur_pjrt->remaining_time++;
           current_thread = t2;
        }
       }
      }
  //if cpu available, then get next thread from ready queue, sim dispatch, and list is not empty
  if(current_thread == NULL && queue_size(ready_queue)!=0){
    thread_t *t = (thread_t*)queue_dequeue(ready_queue);
    thread_extra_t *t_extra = queue_find(all_threads,inner_equalitor,t);
    t_extra->waiting_time += sim_time() - t_extra->start_time; //update waiting time
    if(alg == ROUND_ROBIN){
      t_extra->tick_count = 0;
    }
    sim_dispatch(t);
    current_thread = t;            //set new current thread
    //if an algorithm depending on remaining time is called
    if(alg == NON_PREEMPTIVE_SHORTEST_REMAINING_TIME_FIRST || alg == PREEMPTIVE_SHORTEST_REMAINING_TIME_FIRST){
      t_extra->remaining_time--;
    }
  }
}

/**
 * Tread T is ready to be scheduled for the first time.
 */
void sys_exec(thread_t *t) {
  queue_enqueue(ready_queue, t); //store thread on ready queue before dispatching
  thread_extra_t *t_extra1 = malloc(sizeof(thread_extra_t));
  t_extra1->tid = t->tid;
  t_extra1->arrive_time = sim_time();  //store arrival time
  t_extra1->start_time = sim_time();  //store start time once it gets on CPU
  t_extra1->waiting_time = 0;
  t_extra1->remaining_time = t->length;
  queue_enqueue(all_threads, t_extra1); //store thread with update stats
}

/**
 * Thread T has completed execution and should never again be scheduled.
 */
void sys_exit(thread_t *t) {
     thread_extra_t *t_extra2 =  queue_find(all_threads,inner_equalitor,t);  //store exit time
     t_extra2->finish_time = sim_time()+1; //set finish time
     current_thread = NULL; //set current thread to null to indicate CPU is empty
}

/**
 * Thread T has requested a read operation and is now in an I/O wait queue.
 * When the read operation starts, io_starting(T) will be called, when the
 * read operation completes, io_complete(T) will be called.
 */
void sys_read(thread_t *t) {
  thread_extra_t *t_extra3 = queue_find(all_threads,inner_equalitor,t);
  t_extra3->start_time = sim_time()+1;
  current_thread = NULL;
}

/**
 * Thread T has requested a write operation and is now in an I/O wait queue.
 * When the write operation starts, io_starting(T) will be called, when the
 * write operation completes, io_complete(T) will be called.
 */
void sys_write(thread_t *t) {
  thread_extra_t *t_extra4 = (thread_extra_t *) queue_find(all_threads,inner_equalitor,t);
  t_extra4->start_time = sim_time()+1;  //set start time
  current_thread = NULL;
}

/**
 * An I/O operation requested by T has completed; T is now ready to be
 * scheduled again.
 */
void io_complete(thread_t *t) {
  thread_extra_t *t_extra5 = (thread_extra_t *) queue_find(all_threads,inner_equalitor,t);
  t_extra5->start_time = sim_time()+1;
  queue_enqueue(ready_queue, t); //place back on ready queue
}

/**
 * An I/O operation requested by T is starting; T will not be ready for
 * scheduling until the operation completes.
 */
void io_starting(thread_t *t) {
  thread_extra_t *t_extra6 = (thread_extra_t *) queue_find(all_threads,inner_equalitor,t);
  t_extra6->waiting_time += sim_time() - t_extra6->start_time; //update waiting time
}

/**
 * Return stats for the scheduler simulation, see scheduler.h for details.
 */
stats_t *stats() {
  stats_t *stats = malloc(sizeof(stats_t));
  stats->thread_count = queue_size(all_threads); //number of every thread created
  stats->tstats = malloc(sizeof(stats_t)*stats->thread_count);
  int total_wait = 0, total_turnaround = 0;
  for(int i = 0; i< stats->thread_count; i++){
    thread_extra_t *t =(thread_extra_t *)queue_dequeue(all_threads);
    stats->tstats[i].tid = t->tid;
    stats->tstats[i].waiting_time = t->waiting_time;
    stats->tstats[i].turnaround_time = t->finish_time - t->arrive_time;
    total_wait+= stats->tstats[i].waiting_time;
    total_turnaround += stats->tstats[i].turnaround_time;
    free(t); //free mallocs
  }
  stats->waiting_time = total_wait / stats->thread_count;
  stats->turnaround_time = total_turnaround / stats->thread_count;
  return stats;
}
