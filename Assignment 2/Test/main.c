/***********************************************************************************/
//***********************************************************************************
//            *************NOTE**************
// This is a template for the subject of RTOS in University of Technology Sydney(UTS)
// Please complete the code based on the assignment requirement.

//***********************************************************************************
/***********************************************************************************/

/*
  To compile prog_1 ensure that gcc is installed and run the following command:
  gcc prog_1.c -o prog_1 -lpthread -lrt

*/
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/time.h>

/* --- Structs --- */

typedef struct {
  int * pipePrt;
  sem_t sem_read, sem_justify, sem_write;
  char message[255];
  pthread_mutex_t lock;
} ThreadParams;

typedef enum fileRegion {
  Header,
  Content
} fileRegion;

//Each structure defines a line of the data.txt file
typedef struct FileLine {
  //An enum is used to determine whether the line is from the header or the content region
  enum fileRegion region;

  //The char array is used to store the content of the line read from the file
  char lineContent[255];
} FileLine;

/* This thread reads data from data.txt and writes each line to a pipe */
void *ThreadA(ThreadParams *params);

/* --- Main Code --- */
int main(int argc, char const *argv[])
{
  struct timeval t1;
  gettimeofday(&t1, NULL); // Start Timer
  int fd[2];               //File descriptor for creating a pipe
  pthread_t tid1;
  pthread_attr_t attr;

  ThreadParams params;
  params.pipePrt = fd;

  // Initialization
  pthread_attr_init(&attr);

  // Create pipe
  if (pipe(fd) < 0)
  {
    perror("pipe error");
    exit(EXIT_FAILURE);
  }

  // Create Threads
  if (pthread_create(&tid1, &attr, ThreadA, &params) != 0)
  {
    perror("Error creating threads: ");
    exit(EXIT_FAILURE);
  }

  pthread_join(tid1, NULL);
  return 0;
}


void *ThreadA(ThreadParams * params)
{
  printf("ThreadA\n");
  //struct ThreadParam *parameters = (struct ThreadParam *) params;

  &(params->pipePrt)[1];

  char line[100];
  int lineNo = 0;
  enum fileRegion region = Header;
  char check[12] = "end_header\n";

  FILE *fptr;
  struct FileLine fileContent[50];
  char file_name[10] = "data.txt";

  if ((fptr = fopen(file_name, "r")) == NULL){
    printf("Error! opening file\n");
    // Program exits if file pointer returns NULL.

    printf("Safely exiting\n");
    //need to ensure that garbage collection occurs
    exit(EXIT_FAILURE);
  }

  while (fgets(line, sizeof(line), fptr) != NULL)
  {
    printf("%i: ", region);
    fputs(line, stdout);
    printf("\n");


  }
  fclose(fptr);
  pthread_exit(NULL);
}
