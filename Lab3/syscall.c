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
#include <syscall-nr.h>
#include <list.h>
#include <string.h>
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include "userprog/umem.h"


/****************** System Call Implementations ********************/

// typedef struct input_t{
//     char* cmd_line;
//   } childargs;

// void  child_helper(void * params);
/*
 * BUFFER+0 should be a valid user adresses
 */
void sys_exit(int exitcode)
{
  printf("%s: exit(%d)\n", thread_current()->name, exitcode);
  thread_exit();
}

static void exit_handler(struct intr_frame *f)
{
  int exitcode;
  umem_read(f->esp + 4, &exitcode, sizeof(exitcode));
  sys_exit(exitcode);
}

/*
 * BUFFER+0 and BUFFER+size should be valid user adresses
 */
static uint32_t sys_write(int fd, const void *buffer, unsigned size)
{
  // umem_check((const uint8_t*) buffer);
  // umem_check((const uint8_t*) buffer + size - 1);

  int ret = -1;
  if(size == 0){
    return 0;
  }
  int len = size;
  if(fd >len || fd < 0){ //checks if fd is valid
    return -1;
  }
  //check for bad pointer and if buffer is valid
  if(buffer== NULL || umem_get((uint8_t*)buffer) == -1){
    sys_exit(-1);
  }
  if (fd == 1) { // write to stdout
    putbuf(buffer, size);
    ret = size;
  }
  if(fd > 2){
     file_write(thread_current()->files[fd], (void *)buffer, size); //write contents to file
  }
  ret = size;
  return (uint32_t) ret;
}

static void write_handler(struct intr_frame *f)
{
    int fd;
    const void *buffer;
    unsigned size;

    umem_read(f->esp + 4, &fd, sizeof(fd));
    umem_read(f->esp + 8, &buffer, sizeof(buffer));
    umem_read(f->esp + 12, &size, sizeof(size));

    f->eax = sys_write(fd, buffer, size);
}

/****************** System Call Handler ********************/


/*For create reqs*/
static bool sys_create(char *fname, int isize){
  //If NULL or bad pointer, terminate thread
  if(fname == NULL || (umem_get((uint8_t*)fname) == -1)){
    sys_exit(-1);  //if NULL then exit
  }
  //create file
  bool create =  filesys_create ((const char *)fname, isize, false);
  return create; //status of created file
}

static void create_handler(struct intr_frame *f)
{
  const char *fname;
  int isize;

  umem_read(f->esp + 4, &fname, sizeof(fname));
  umem_read(f->esp + 8, &isize, sizeof(isize));

  f->eax = sys_create((char *)fname, isize);
}

/*FOR OPEN REQS*/
static uint32_t sys_open(char *fname, int fd){
  //If NULL or bad pointer, terminate thread
  if(fname == NULL || umem_get((uint8_t*)fname) == -1){
    sys_exit(-1);
  }
  if(strlen(fname)==0){ //if empty
    return -1;
  }

  struct file * f = filesys_open ((const char *) fname);

  if(f==NULL){ //if file doesn't exist
     return -1;
  }
  int len = sizeof(thread_current()->files);
  for(int i = 3; i < len; i++){
    if(thread_current()->files[i]== NULL){
      fd = i;
      thread_current()->files[fd] = f; //store file pointer
      break;
    }
  }
  return fd; //return file descriptor
}

static void open_handler(struct intr_frame *f)
{
  const char *fname;
  int fd;
  umem_read(f->esp + 4, &fname, sizeof(fname));
  umem_read(f->esp + 8, &fd, sizeof(fd));

  f->eax = sys_open((char *)fname, fd);
}

/*FOR READ REQS*/
static uint32_t sys_read(int fd, const void *buffer, unsigned size){
  int ret = -1;
  //if buffer empty
  if(size == 0){
    return 0;
  }
  int len = size;
  if(fd >len || fd < 0){ //checks if fd is valid
    return -1;
  }
  //check for bad pointer and if buffer is valid and file not null
  if(buffer== NULL || umem_get((uint8_t*)buffer) == -1 || thread_current()->files[fd] == NULL){
    sys_exit(-1);
  }
  //read the file
  file_read(thread_current()->files[fd], (void*)buffer, size);
  ret = size;
  return (uint32_t) ret;
}

static void read_handler(struct intr_frame *f)
{
    int fd;
    const void *buffer;
    unsigned size;

    umem_read(f->esp + 4, &fd, sizeof(fd));
    umem_read(f->esp + 8, &buffer, sizeof(buffer));
    umem_read(f->esp + 12, &size, sizeof(size));
    f->eax = sys_read(fd, buffer, size);
}
static void filesize_handler(struct intr_frame *f)
{
    int fd;
    umem_read(f->esp + 4, &fd, sizeof(fd));
    f->eax = file_length(thread_current()->files[fd]);
}

static void close_handler(struct intr_frame *f)
{
    int fd;
    const void *buffer;
    unsigned size;

    umem_read(f->esp + 4, &fd, sizeof(fd));
    umem_read(f->esp + 8, &buffer, sizeof(buffer));
    umem_read(f->esp + 12, &size, sizeof(size));
    int len = sizeof(thread_current()->files[fd]);
    if(fd <len && fd > 0){ //checks if fd is valid
      file_close(thread_current()->files[fd]);
      thread_current()->files[fd] = NULL; //delete file
    }
}

static int sys_exec(char * fname){
  //if bad pointer or missing
  if((umem_get((uint8_t*)fname) == -1)){
    sys_exit(-1);
  }
  process_execute(fname);
  struct file * f = filesys_open ((const char *) fname);
  if(f==NULL){ //check if the file existed
    return -1;
  }
  return 1;
}

static void exec_handler(struct intr_frame *f)
{
     const char *fname;
     umem_read(f->esp + 4, &fname, sizeof(fname));
     f->eax = sys_exec((char *)fname);
}

/*****************************************************************************************/
static void
syscall_handler(struct intr_frame *f)
{
  int syscall;
  ASSERT( sizeof(syscall) == 4 ); // assuming x86

  // The system call number is in the 32-bit word at the caller's stack pointer.
  umem_read(f->esp, &syscall, sizeof(syscall));

  // Store the stack pointer esp, which is needed in the page fault handler.
  // Do NOT remove this line
  thread_current()->current_esp = f->esp;

  switch (syscall) {
  case SYS_CREATE:
    create_handler(f);
    break;

  case SYS_OPEN:
    open_handler(f);
    break;

  case SYS_READ:
    read_handler(f);
    break;

  case SYS_HALT:
    shutdown_power_off();
    break;

  case SYS_EXIT:
    exit_handler(f);
    break;

  case SYS_WRITE:
    write_handler(f);
    break;

  case SYS_CLOSE:
    close_handler(f);
    break;

   case SYS_EXEC:
    exec_handler(f);
    break;

   case SYS_WAIT:
    // write_handler(f);
    break;
   case SYS_FILESIZE:
    filesize_handler(f);
    break;
  default:
    printf("[ERROR] system call %d is unimplemented!\n", syscall);
    thread_exit();
    break;
  }
}

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}
