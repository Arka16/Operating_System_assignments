/*
 * This file is derived from source code for the Pintos
 * instructional operating system which is itself derived
 * from the Nachos instructional operating system. The
 * Nachos copyright notice is reproduced in full below.
 *
 * Copyright (C) 1992-1996 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose, without fee, and
 * without written agreement is hereby granted, provided that the
 * above copyright notice and the following two paragraphs appear
 * in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
 * AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
 * HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 * BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 * MODIFICATIONS.
 *
 * Modifications Copyright (C) 2017-2021 David C. Harrison.
 * All rights reserved.
 */

#include <stdio.h>
#include <string.h>

#include "threads/semaphore.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/lock.h"
bool sem_priority_sort (const struct list_elem *a,
                             const struct list_elem *b,
                             void *aux);
/*
 * Initializes semaphore SEMA to VALUE.  A semaphore is a
 * nonnegative integer along with two atomic operators for
 * manipulating it:
 *
 * - down or Dijkstra's "P": wait for the value to become positive,
 * 	then decrement it.
 * - up or Dijkstra's "V": increment the value(and wake up one waiting
 * 	thread, if any).
 */
void semaphore_init(struct semaphore *sema, unsigned value)
{
  ASSERT(sema != NULL);

  sema->value = value;
  list_init(&sema->waiters);
}

/*
 * Down or Dijkstra's "P" operation on a semaphore.  Waits for SEMA's
 * value to become positive and then atomically decrements it.
 *
 * This function may sleep, so it must not be called within an
 * interrupt handler.  This function may be called with
 * interrupts disabled, but if it sleeps then the next scheduled
 * thread will probably turn interrupts back on.
 */
void semaphore_down(struct semaphore *sema)
{
  ASSERT(sema != NULL);
  ASSERT(!intr_context());
  enum intr_level old_level = intr_disable();
  while (sema->value == 0)
  {
    //if the current thread has a higher priority than the lock holder
    if(thread_current()->lock_temp!=NULL && thread_current()->lock_temp->holder->priority < thread_get_priority()){
      struct thread *cur = thread_current(); //allows us to change the current thread to be the lock holder
      cur->lock_temp->holder->priority = cur->priority; //preserve current thread priority
      cur = cur->lock_temp->holder; //make current thread the lock holder
    }
    list_insert_ordered(&sema->waiters, &thread_current()->sharedelem, sem_priority_sort, NULL);
    thread_block();
  }
  sema->value--;
  intr_set_level(old_level);
}

/*
 * Up or Dijkstra's "V" operation on a semaphore.  Increments SEMA's value
 * and wakes up one thread of those waiting for SEMA, if any.
 *
 * This function may be called from an interrupt handler.
 */
void semaphore_up(struct semaphore *semaphore)
{
  enum intr_level old_level;

  ASSERT(semaphore != NULL);
  old_level = intr_disable();
  struct thread *t1 = thread_current();
  if (!list_empty(&semaphore->waiters))
  {
      list_sort(&semaphore->waiters, sem_priority_sort, NULL);
      t1 = list_entry(list_pop_front(&semaphore->waiters), struct thread, sharedelem);
      thread_unblock(t1); //wake up next waiting thread
  }
  semaphore->value++;
  //check whether if a thread with higher priority than new priority exists
  intr_set_level(old_level);
  //if current thread has a lower priority than
  if(thread_get_priority() < t1->priority){  //preempt if unwoke thread has higher thread than current
    thread_yield();
  }
  //yield after incrementing the semaphore value
  struct list_elem *e;
  for (e = list_begin (&semaphore->waiters); e != list_end (&semaphore->waiters);
      e = list_next (e))
      {
        struct thread *t = list_entry (e, struct thread, sharedelem);
        if(t->priority >= thread_get_priority()){
          thread_yield();  //yield to higher priority thread
          break;
        }
     }
     if(thread_get_priority()!= thread_current()->prev_priority){
       thread_set_priority(thread_current()->prev_priority);
     }
    thread_yield();  //yield to higher priority thread again
}

/*function that keeps the threads in sem waiters in order of priority*/
bool sem_priority_sort (const struct list_elem *a,
                             const struct list_elem *b,
                             void *aux UNUSED)
{
  //get thread refereces of a and b
  struct thread *t1 = list_entry(a, struct thread, sharedelem);
  struct thread *t2 = list_entry(b, struct thread, sharedelem);

  if(t1->priority > t2->priority){    //prefer the thread with higher priority
    return true;
  }
  return false;
}
