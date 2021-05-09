/**
 * See scheduler.h for function details. All are callbacks; i.e. the simulator
 * calls you when something interesting happens.
 */
 //SOURCES: TA OH, lecture slides scheduling 1, secret sauce
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
  int tick_count;
  thread_t *t;
} thread_extra_t;

void *ready_queue;
void *all_threads;
int thread_count;
int alg;
int quant;
bool cpu_available;
thread_t *current_thread;
//queue find helper function
static bool inner_equalitor(void *outer, void *inner) {
    return ((thread_extra_t*)outer)->tid == ((thread_t*)inner)->tid;
  }

//helper function to sort
static int priority_sort(void *a, void *b) {
        return ((thread_t*)a)->priority - ((thread_t*)b)->priority;
    }
// static int rr_sort(void *a, void *b) {
//         return ((thread_extra_t*)a)->priority - ((thread_t*)b)->priority;
//     }

void scheduler(enum algorithm algorithm, unsigned int quantum) {
    current_thread = NULL;
    alg = algorithm; //store algorithm
    quant = quantum;
    ready_queue = queue_create(); //for scheduling purposes
    all_threads = queue_create(); //for stats
    cpu_available = true;
}

/**
 * Programmable clock interrupt handler. Call sim_time() to find out
 * what tick this is. Called after all calls to sys_exec() for this
 * tick have been made.
 */
void tick() {
    if(alg != FIRST_COME_FIRST_SERVED && alg != ROUND_ROBIN){
      queue_sort(ready_queue, priority_sort);
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
         cpu_available = false;
      }
    }
    if((alg == PREEMPTIVE_PRIORITY) && queue_size(ready_queue)!= 0){
      thread_t *t2 = (thread_t*)queue_head(ready_queue);
      if(current_thread != NULL && !cpu_available && current_thread->priority > t2->priority){
         thread_extra_t *t_extra = queue_find(all_threads,inner_equalitor,t2);
         thread_extra_t *t_extra2 = queue_find(all_threads,inner_equalitor,current_thread);
         t_extra->waiting_time += sim_time() - t_extra->start_time;
         t_extra2->start_time = sim_time();
         queue_enqueue(ready_queue, current_thread);
         queue_dequeue(ready_queue);
         sim_dispatch(t2);
         current_thread = t2;
         cpu_available = false;
    }
  }
  //if cpu available, then get next thread from ready queue, sim dispatch, and list is not empty
  if(cpu_available && queue_size(ready_queue)!=0){
    thread_t *t = (thread_t*)queue_dequeue(ready_queue);
    thread_extra_t *t_extra = queue_find(all_threads,inner_equalitor,t);
    t_extra->waiting_time += sim_time() - t_extra->start_time;
    if(alg == ROUND_ROBIN){
      t_extra->tick_count = 0;
    }
    sim_dispatch(t);
    current_thread = t;
    cpu_available = false;
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
  queue_enqueue(all_threads, t_extra1); //store thread with update stats
}

/**
 * Thread T has completed execution and should never again be scheduled.
 */
void sys_exit(thread_t *t) {
     thread_extra_t *t_extra2 =  queue_find(all_threads,inner_equalitor,t);  //store exit time
     t_extra2->finish_time = sim_time()+1; //set finish time
     current_thread = NULL;
     cpu_available = true;
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
  cpu_available = true;
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
  cpu_available = true;
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
  thread_count = queue_size(all_threads);
  stats_t *stats = malloc(sizeof(stats_t));
  stats->thread_count = thread_count;
  stats->tstats = malloc(sizeof(stats_t)*thread_count);
  int total_wait = 0, total_turnaround = 0;
  for(int i = 0; i< thread_count; i++){
    thread_extra_t *t =(thread_extra_t *)queue_dequeue(all_threads);
    stats->tstats[i].tid = t->tid;
    stats->tstats[i].waiting_time = t->waiting_time;
    stats->tstats[i].turnaround_time = t->finish_time - t->arrive_time;
    total_wait+= stats->tstats[i].waiting_time;
    total_turnaround += stats->tstats[i].turnaround_time;
    free(t);
  }
  stats->waiting_time = total_wait / thread_count;
  stats->turnaround_time = total_turnaround / thread_count;
  return stats;
}
