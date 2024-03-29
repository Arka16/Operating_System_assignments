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

#include "threads/condvar.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <list.h>


bool condvar_priority_sort (const struct list_elem *a,
                             const struct list_elem *b,
                             void *aux);
/*
 * Initializes condition variable COND.  A condition variable
 * allows one piece of code to signal a condition and cooperating
 * code to receive the signal and act upon it.
 */
void condvar_init(struct condvar *cond)
{
  ASSERT(cond != NULL);
  list_init(&cond->waiters);
}

/*
 * Atomically releases LOCK and waits for COND to be signaled by
 * some other piece of code.  After COND is signaled, LOCK is
 * reacquired before returning.  LOCK must be held before calling
 * this function.
 *
 * The monitor implemented by this function is "Mesa" style, not
 * "Hoare" style, that is, sending and receiving a signal are not
 * an atomic operation.  Thus, typically the caller must recheck
 * the condition after the wait completes and, if necessary, wait
 * again.
 *
 * A given condition variable is associated with only a single
 * lock, but one lock may be associated with any number of
 * condition variables.  That is, there is a one-to-many mapping
 * from locks to condition variables.
 *
 * This function may sleep, so it must not be called within an
 * interrupt handler.  This function may be called with
 * interrupts disabled, but interrupts will be turned back on if
 * we need to sleep.
 */
void condvar_wait(struct condvar *cond, struct lock *lock)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  struct semaphore waiter;
  semaphore_init(&waiter, 0);
  // list_push_back(&cond->waiters, &waiter.condvarelem);
  list_insert_ordered(&cond->waiters, &waiter.condvarelem, condvar_priority_sort, NULL);
  lock_release(lock);
  semaphore_down(&waiter);
  lock_acquire(lock);
}

/*
 * If any threads are waiting on COND(protected by LOCK), then
 * this function signals one of them to wake up from its wait.
 * LOCK must be held before calling this function.
 *
 * An interrupt handler cannot acquire a lock, so it does not
 * make sense to try to signal a condition variable within an
 * interrupt handler.
 */
void condvar_signal(struct condvar *cond, struct lock *lock UNUSED)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  if (!list_empty(&cond->waiters))
  {
    //make sure cond waiters is sorted correctly before popping
    list_sort(&cond->waiters, condvar_priority_sort, NULL);
    semaphore_up(list_entry(
        list_pop_front(&cond->waiters), struct semaphore, condvarelem));
  }
}

/*
 * Wakes up all threads, if any, waiting on COND(protected by
 * LOCK).  LOCK must be held before calling this function.
 *
 * An interrupt handler cannot acquire a lock, so it does not
 * make sense to try to signal a condition variable within an
 * interrupt handler.
 */
void condvar_broadcast(struct condvar *cond, struct lock *lock)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);

  while (!list_empty(&cond->waiters))
  {
    condvar_signal(cond, lock);
  }
}

bool condvar_priority_sort (const struct list_elem *a,
                             const struct list_elem *b,
                             void *aux UNUSED)
    {
      //get condvar refereces of a and b
      struct semaphore *s1 = list_entry(a, struct semaphore, condvarelem);
      struct semaphore *s2 = list_entry(b, struct semaphore,  condvarelem);
      //get front element of each sem list so we can access the highest priority thread of each
      struct list_elem *l1;
      struct list_elem *l2;
      //if first sem has no threads in list
      if(list_empty(&s1->waiters)){
        return false;
      }
      l1 = list_front(&s1->waiters);
      //if second sem has no threads in list
      if(list_empty(&s2->waiters)){
        return true;
      }
      l2 = list_front(&s2->waiters);
       //get the front most thread of each sem list
      struct thread *t1 = list_entry(l1, struct thread, sharedelem);
      struct thread *t2 = list_entry(l2, struct thread, sharedelem);
      if(t1->priority > t2->priority){  //prefer  the thread with higher priority
        return true;
      }
      return false;
    }
