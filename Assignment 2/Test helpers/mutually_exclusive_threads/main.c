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

void *ThreadA(ThreadParams *params);
void *ThreadB(ThreadParams *params);
void *ThreadC(ThreadParams *params);

/* --- Main Code --- */
int main(int argc, char const *argv[])
{
  pthread_t tid1, tid2, tid3;
  pthread_attr_t attr;
  ThreadParams params;

  // Initialization
  pthread_attr_init(&attr);

  // Initialise sempahores
  if(sem_init(&params.sem_read, 0, 1)){
    perror("Error initializing semaphore.");
    exit(0);
  }

  if(sem_init(&params.sem_justify, 0, 0)){
    perror("Error initializing semaphore.");
    exit(0);
  }

  if(sem_init(&params.sem_write, 0, 0))
  {
    perror("Error initializing semaphore.");
    exit(0);
  }

  // Create Threads
  if (pthread_create(&tid1, &attr, ThreadA, &params) != 0)
  {
    perror("Error creating threads: ");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&tid2, &attr, ThreadB, &params) != 0)
  {
    perror("Error creating threads: ");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&tid3, &attr, ThreadC, &params) != 0)
  {
    perror("Error creating threads: ");
    exit(EXIT_FAILURE);
  }

  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);
  pthread_join(tid3, NULL);
  return 0;
}


void *ThreadA(ThreadParams * params)
{
  int counter = 0;
  while(!sem_wait(&params->sem_read) && counter <= 5000){
    printf("ThreadA with count %i\n", counter);
    sem_post(&params->sem_justify);
    counter++;
  }

  printf("Exiting ThreadA\n");
  pthread_exit(NULL);
}

void *ThreadB(ThreadParams * params)
{
  while(!sem_wait(&params->sem_justify)){
    printf("ThreadB\n");
    sem_post(&params->sem_write);
  }

  printf("Exiting ThreadB\n");
  pthread_exit(NULL);
}

void *ThreadC(ThreadParams * params)
{
  while(!sem_wait(&params->sem_write)){
    printf("ThreadC\n");
    sem_post(&params->sem_read);
  }

  printf("Exiting ThreadC\n");
  pthread_exit(NULL);
}