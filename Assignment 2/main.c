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

#define BUFFER_SIZE 1024
#define INPUT_FILE_NAME "data.txt"
#define OUTPUT_FILE_NAME "output.txt"
#define END_OF_HEADER "end_header"

/* --- Structs --- */
typedef enum fileRegion
{
  Header,
  Content
} fileRegion;

//Each structure defines a line of a file
typedef struct FileLine
{
  //An enum is used to determine whether the line is from the header or the content region
  enum fileRegion region;

  //The char array is used to store the content of the line read from the file
  char lineContent[255];
} FileLine;

typedef struct
{
  int lineNo;
  int *pipePrt;
} ReadParams;

typedef struct
{
  int lineNo;
  int *pipePrt;
  FileLine *shared_memory[50];
} ProcessorParams;

typedef struct
{
  int lineNo;
  FileLine *shared_memory[50];
} WriterParams;

/* --- Prototypes --- */

/* Initializes data and utilities used in thread params */
void initialiseData();

/* This thread reads data from INPUT_FILE_NAME and writes each line to a pipe */
void *Reader(ReadParams *params);

/* This thread reads data from pipe used in Reader and writes it to a shared variable */
void *Processor(ProcessorParams *params);

/* This thread reads from shared variable and outputs non-header text to output.txt */
void *Writer(WriterParams *params);

pthread_t tid1, tid2, tid3;             // Thread ID
sem_t sem_read, sem_process, sem_write; //Create semaphores

/* --- Main Code --- */
int main(int argc, char const *argv[])
{
  struct timeval t1;
  gettimeofday(&t1, NULL); // Start Timer
  pthread_attr_t attr;

  int lineNo = 0;             //Track line number
  int pipeFileDescriptor[2];  //File descriptor for creating a pipe
  FileLine shared_memory[50]; //Create shared memory buffer

  ReadParams readParams = {lineNo, pipeFileDescriptor};
  ProcessorParams processorParams = {lineNo, pipeFileDescriptor, &shared_memory};
  WriterParams writerParams = {lineNo, &shared_memory};

  // Initialisation
  initialiseData(NULL);
  pthread_attr_init(&attr);

  // Create pipe
  if (pipe(pipeFileDescriptor) < 0)
  {
    perror("Pipe creation error");
    exit(EXIT_FAILURE);
  }

  // Create Threads
  if (pthread_create(&tid1, &attr, Reader, &readParams) != 0)
  {
    perror("Error creating Reader thread");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&tid2, &attr, Processor, &processorParams) != 0)
  {
    perror("Error creating Processor thread");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&tid3, &attr, Writer, &writerParams) != 0)
  {
    perror("Error creating Writer thread");
    exit(EXIT_FAILURE);
  }

  // Wait on threads to finish
  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);
  pthread_join(tid3, NULL);

  //TODO: add your code
  close(pipeFileDescriptor[0]);
  close(pipeFileDescriptor[1]);
  return 0;
}

void initialiseData()
{
  // Initialise Sempahores
  if (sem_init(&sem_read, 0, 1))
  {
    perror("Error initializing read semaphore.");
    exit(EXIT_FAILURE);
  }

  if (sem_init(&sem_process, 0, 0))
  {
    perror("Error initializing process semaphore.");
    exit(EXIT_FAILURE);
  }

  if (sem_init(&sem_write, 0, 0))
  {
    perror("Error initializing write semaphore.");
    exit(EXIT_FAILURE);
  }

  return;
}

void *Reader(ReadParams *params)
{
  printf("Reader\n");
  char line[BUFFER_SIZE];
  FILE *readFile;
  char fileName[10] = INPUT_FILE_NAME;

  //Open INPUT_FILE_NAME
  if ((readFile = fopen(fileName, "r")) == NULL)
  {
    printf("Error! opening %s\n", fileName);
    exit(EXIT_FAILURE);
  }

  while (!sem_wait(&sem_read) && fgets(line, BUFFER_SIZE, readFile) != NULL)
  {
    if ((write(params->pipePrt[1], line, strlen(line) + 1) < 1))
    {
      perror("Write");
      exit(EXIT_FAILURE);
    }

    params->lineNo++;
    sem_post(&sem_process);
  }

  close(params->pipePrt[1]);
  fclose(readFile);

  //Cancel threads - might not be the best way to do this
  pthread_cancel(tid1);
  pthread_cancel(tid2);
  pthread_cancel(tid3);
  pthread_exit(0);
}

void *Processor(ProcessorParams *params)
{
  printf("Processor\n");
  enum fileRegion region = Header;
  char check[sizeof(char) * 13] = END_OF_HEADER;

  while (!sem_wait(&sem_process))
  {
    char readBuffer[BUFFER_SIZE];

    // Read pipe and copy to readBuffer
    if ((read(params->pipePrt[0], &readBuffer, BUFFER_SIZE) < 1))
    {
      perror("Read");
      exit(EXIT_FAILURE);
    }

    // Construct FileLine object
    struct FileLine fileLine = {region};
    strncpy(fileLine.lineContent, readBuffer, sizeof(fileLine.lineContent) - 1);

    // Copy FileLine object to shared memory (between processor and writer)
    *(params->shared_memory[params->lineNo]) = fileLine;

    /* check whether this line is the end of header,
    the new line in array c contains "end_header\n"*/
    if (region == Header && strstr(readBuffer, check) != NULL)
    {
      region = Content;
    }

    sem_post(&sem_write);
  }

  close(params->pipePrt[0]);
  pthread_exit(NULL);
}

void *Writer(WriterParams *params)
{
  printf("Writer\n");
  FILE *writeFile;
  char outputFileName[sizeof(char) * 11] = OUTPUT_FILE_NAME;

  // Create or open the file we want to output the content to
  if ((writeFile = fopen(outputFileName, "w")) == NULL)
  {
    printf("Error! opening creating or opening existing output file\n");
    exit(EXIT_FAILURE);
  }

  while (!sem_wait(&sem_write))
  {
    /* Writes lines in the Content region to the output file */
    if (params->shared_memory[params->lineNo]->region == Content)
    {
      fprintf(writeFile, "%s", params->shared_memory[params->lineNo]->lineContent);
    }

    sem_post(&sem_read);
  }

  fclose(writeFile);
  pthread_exit(NULL);
}
