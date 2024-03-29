/*********************************************************************
 *
 * Copyright (C) 2020-2021 David C. Harrison. All right reserved.
 *
 * You may not use, distribute, publish, or modify this code without
 * the express written permission of the copyright holder.
 *
 ***********************************************************************/

/*******SOURCES*************

*****************************BASIC*************************************************************************:

Checking whether/not file is missing(using access(name, F_OK))
https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c

For file_seek() to take care of Offset logic
https://www.tutorialspoint.com/c_standard_library/c_function_fseek.htm

knowing how to use fopen to open a file for different purposes
https://www.tutorialspoint.com/c_standard_library/c_function_fopen.htm

for fread syntax and following a example
https://www.tutorialspoint.com/c_standard_library/c_function_fread.htm

for fwrite syntax and following a example
https://www.tutorialspoint.com/c_standard_library/c_function_fwrite.htm

For knowing how to close a file
https://www.tutorialspoint.com/c_standard_library/c_function_fclose.htm

*************************ADVANCE*****************************************************************************:

how to use write() with file descriptor
https://man7.org/linux/man-pages/man2/read.2.html

how to use read() with file descriptor
https://man7.org/linux/man-pages/man2/open.2.html

to look up all the flags to pass to open()
https://www.gnu.org/software/libc/manual/html_node/Open_002dtime-Flags.html
https://www.gnu.org/software/libc/manual/html_node/Access-Modes.html

************************STRETCH*******************************************************************************:
Learned what scandir is, what it returns, and how it works. use this to get the list of directories/sort/iterate through list

https://www.gnu.org/software/libc/manual/html_node/Scanning-Directory-Content.html

More specifics on iterating thru list after scandir is called. Used this to make my looping condition, learn how to use the ->d_name
field, and knew how to free the list properly to avoid mem errors. Also learned how to initialize dirent** struct list
https://stackoverflow.com/questions/18402428/how-to-properly-use-scandir-in-c
https://pubs.opengroup.org/onlinepubs/9699919799/functions/alphasort.html

//use this to learn how to filer the stars out of output(filter helper function)
https://stackoverflow.com/questions/17616362/filtering-scandir-for-filenames-in-folder-c-language

//use this for checking if a file was a directory
https://man7.org/linux/man-pages/man3/readdir.3.html

//used this to know how to conviniently write to dir using fd
https://linux.die.net/man/3/dprintf


EXTREME:
Used Supraja's help in TA OH for bitmask logic(expecially for handliling VER and space)

*/





#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>

/*
 * Extended ASCII box drawing characters:
 *
 * The following code:
 *
 * printf("CSE130\n");
 * printf("%s%s Assignments\n", TEE, HOR);
 * printf("%s  %s%s Assignment 1\n", VER, TEE, HOR);
 * printf("%s  %s%s Assignment 2\n", VER, TEE, HOR);
 * printf("%s  %s%s Assignment 3\n", VER, TEE, HOR);
 * printf("%s  %s%s Assignment 4\n", VER, TEE, HOR);
 * printf("%s  %s%s Assignment 5\n", VER, TEE, HOR);
 * printf("%s  %s%s Assignment 6\n", VER, ELB, HOR);
 * printf("%s%s Labs\n", ELB, HOR);
 * printf("   %s%s Lab 1\n", TEE, HOR);
 * printf("   %s%s Lab 2\n", TEE, HOR);
 * printf("   %s%s Lab 3\n", ELB, HOR);
 * printf();
 *
 * Shows this tree:
 *
 * CSE130
 * ├─ Assignments
 * │  ├─ Assignment 1
 * │  ├─ Assignment 2
 * │  ├─ Assignment 3
 * │  ├─ Assignment 4
 * │  ├─ Assignment 5
 * │  └─ Assignment 6
 * └─ Labs
 *    ├─ Lab 1
 *    ├─ Lab 2
 *    └─ Lab 3
 */
#define TEE "\u251C"  // ├
#define HOR "\u2500"  // ─
#define VER "\u2502"  // │
#define ELB "\u2514"  // └

/*
 * Read at most SIZE bytes from FNAME starting at FOFFSET into BUF starting
 * at BOFFSET.
 *
 * RETURN number of bytes read from FNAME into BUFF, -1 on error.
 */
int fileman_read(
    const char *fname,
    const size_t foffset,
    const char *buf,
    const size_t boffset,
    const size_t size)
{
  if(access(fname, F_OK) != 0){
    return -1;
  }
  FILE *fp = fopen(fname, "r");
  fseek(fp,foffset,SEEK_SET);  //set pointer to begining of file
  void * offset =  (void*) (buf + boffset);  //set offset
  fread((char*)offset,size, 1,fp);
  fclose(fp);
  return size;
}

/*
 * Create FILE and Write SIZE bytes from BUF starting at BOFFSET into FILE
 * starting at FOFFSET.
 *
 * RETURN number of bytes from BUFF written to FNAME, -1 on error or if FILE
 * already exists
 */
int fileman_write(
    const char *fname,
    const size_t foffset,
    const char *buf,
    const size_t boffset,
    const size_t size)
{
  if(access(fname, F_OK) == 0){
    return -1;
  }
  FILE *fp = fopen(fname, "w");
  fseek(fp,foffset,SEEK_SET);  //set pointer to begining of file
  void * offset =  (void*) (buf + boffset);  //set offset
  fwrite((char*)offset,size, 1,fp);  //write to file
  fclose(fp);
  return size;
}

/*
 * Append SIZE bytes from BUF to existing FILE.
 *
 * RETURN number of bytes from BUFF appended to FNAME, -1 on error or if FILE
 * does not exist
 */
int fileman_append(const char *fname, const char *buf, const size_t size)
{
  if(access(fname, F_OK) != 0){
    return -1;
  }
  int fd = open(fname, O_APPEND|O_WRONLY);  //give permission to write and set pointer to end
  int num_bytes = write(fd, buf, size);
  close(fd);
  return num_bytes;
}

/*
 * Copy existing file FSRC to new file FDEST.
 *
 * Do not assume anything about the size of FSRC.
 *
 * RETURN number of bytes from FSRC written to FDEST, -1 on error, or if FSRC
 * does not exists, or if SDEST already exists.
 */
int fileman_copy(const char *fsrc, const char *fdest)
{
  if(access(fsrc, F_OK) != 0 || access(fdest,F_OK)==0){
    return -1;
  }
  //have a char buf[], and array size can be 100, copy into buff each iteration till EOF is reach,
  //do while loop, read frc to buff, write from buff to dest, keep count of number of bytes read,
  //when return == 100, continue

  //if files can't be opened
  if(!fopen(fsrc,"r") || !fopen(fdest, "w")){
    return -1;
  }
  int fd1 = open(fsrc, O_RDONLY);    //read from source
  int fd2 = open(fdest, O_WRONLY|O_CREAT);   //write to dest
  int num_bytes;
  int count = 0;
  char buf[100]; //allocate buf
  do{
    num_bytes = read(fd1, &buf, 100); //read length of buf bytes at a time
    count+=num_bytes;
    write(fd2, buf, num_bytes);      //write to dest
  }
  while(num_bytes == 100);
  close(fd1);
  close(fd2);
  return count;
}

/*
 * Print a hierachival directory view starting at DNAME to file descriptor FD
 * as shown in an example below where DNAME == 'data.dir'
 *
 *   data.dir
 *       blbcbuvjjko
 *           lgvoz
 *               jfwbv
 *                   jqlbbb
 *                   yfgwpvax
 *           tcx
 *               jbjfwbv
 *                   demvlgq
 *                   us
 *               zss
 *                   jfwbv
 *                       ahfamnz
 *       vkhqmgwsgd
 *           agmugje
 *               surxeb
 *                   dyjxfseur
 *                   wy
 *           tcx
 */
int filter(const struct dirent * entry){
  return strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."); //fiter out the dots
}

void get_dir(int fd, const char *dname, int indent, char* path) {
  struct dirent** namelist;
  int numFiles = scandir(path, &namelist, filter, alphasort); //get list and sort alphabetically
  for(int i = 0; i< numFiles; i++){
    for(int j = 0; j < indent+4; j++){
       dprintf(fd, " "); //write indents
    }
    dprintf(fd, "%s\n", namelist[i]->d_name); //write entry names and new line
    if(namelist[i]->d_type == DT_DIR){  //if directory
      //allocate space for previous path length as well as space for new child file length
      char *path_temp = malloc((strlen(path)+1 + strlen(namelist[i]->d_name)+1));
      //make new path
      strcpy(path_temp,  (char *) path);
      strcat(path_temp,  "/");
      strcat(path_temp, namelist[i]->d_name);
      get_dir(fd, namelist[i]->d_name, indent+4, path_temp);
      free(path_temp);
    }
    free(namelist[i]);
  }
  if(numFiles != -1){   //only free if scandir doesn't result in an error
    free(namelist);
   }
}

void fileman_dir(const int fd, const char *dname)
{
  dprintf(fd, "%s\n", dname); //write to fd
  get_dir(fd,dname,0, (char*)dname);
}

/*
 * Print a hierachival directory tree view starting at DNAME to file descriptor
 * FD as shown in an example below where DNAME == 'data.dir'.
 *
 * Use the extended ASCII box drawing characters TEE, HOR, VER, and ELB.
 *
 *   data.dir
 *   ├── blbcbuvjjko
 *   │   ├── lgvoz
 *   │   │   └── jfwbv
 *   │   │       ├── jqlbbb
 *   │   │       └── yfgwpvax
 *   │   └── tcx
 *   │       ├── jbjfwbv
 *   │       │   ├── demvlgq
 *   │       │   └── us
 *   │       └── zss
 *   │           └── jfwbv
 *   │               └── ahfamnz
 *   └── vkhqmgwsgd
 *       ├── agmugje
 *       │   └── surxeb
 *       │       ├── dyjxfseur
 *       │       └── wy
 *       └── tcx
 */


//global bitmask array to handle VER/space logic
bool bitmask [100];

void drawTree(int fd, const char *dname, int indent, char* path) {
  struct dirent** namelist;
  int numFiles = scandir(path, &namelist, filter, alphasort); //get list and sort alphabetically
  for(int i = 0; i< numFiles; i++){
    for(int j = 0; j <= indent; j++){  // j greater than equal to indent, take care of TEE and ELB, else, know VERT/SPACE
       if(j == indent){    //last 4 spaces check TEE vs ELB logic
            if(i== numFiles-1){    //if last entry then ELB
              dprintf(fd, ELB);
            }
            else{
               dprintf(fd, TEE); //else print TEE
            }
            //print 2 HORs and a space to match style
            dprintf(fd, "%s%s ", HOR, HOR);
        }
        else{
          if(bitmask[j]== true){ //if not last file vertical line
            dprintf(fd, VER);
          }
          else{
            dprintf(fd, " "); //if last file and more directories incoming, add spaces
          }
        }
    }
    dprintf(fd, "%s\n", namelist[i]->d_name); //write entry names and new line
    if(namelist[i]->d_type == DT_DIR){  //if directory
      char *path_temp = malloc((strlen(path)+1 + strlen(namelist[i]->d_name)+1) * sizeof(char*));
      //make new path
      strcpy(path_temp,  (char *) path);
      strcat(path_temp,  "/");
      strcat(path_temp, namelist[i]->d_name);
      if(i == numFiles -1){
        bitmask[indent] = false;  //space if last file
      }
      else{
        bitmask[indent] = true;   //vertical otherwise
      }
      drawTree(fd, namelist[i]->d_name, indent+4, path_temp);
      free(path_temp); //free malloced path
    }
    free(namelist[i]); //free every malloced elem
  }
  if(numFiles != -1){
    free(namelist); //only free namelist if it's created
   }
}

void fileman_tree(const int fd, const char *dname)
{
  dprintf(fd, "%s\n", dname); //write very first file to fd
  drawTree(fd,dname,0, (char*)dname);
}
