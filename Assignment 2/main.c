/***********************************************************************************/
//***********************************************************************************
//            *************NOTE**************
// This is a template for the subject of RTOS in University of Technology Sydney(UTS)
// Please complete the code based on the assignment requirement.

//***********************************************************************************
/***********************************************************************************/

/*
  To compile main.c ensure that gcc is installed and run the following command:
  gcc main.c -o main -pthread
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
#include <errno.h>

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

//Each structure defines a row of a file
typedef struct DataRow
{
  //An enum is used to determine whether the row is from the header or the content region
  enum fileRegion region;

  //The char array is used to store the content of the row read from the file
  char content[255];
} DataRow;

typedef struct
{
  int rowNumber;
  int * pipePrt;
} ReadParams;

typedef struct
{
  int rowNumber;
  int *pipePrt;
  DataRow * shared_memory[50];
} ProcessorParams;

typedef struct
{
  int rowNumber;
  DataRow * shared_memory[50];
} WriterParams;

/* --- Prototypes --- */

/* Initializes data and utilities used in thread params */
void initialiseData();

/* This thread reads data from INPUT_FILE_NAME and writes each row to a pipe */
void *Reader(void * params);

/* This thread reads data from pipe used in Reader and writes it to a shared variable */
void *Processor(void * params);

/* This thread reads from shared variable and outputs non-header text to output.txt */
void *Writer(void * params);

pthread_t readerThreadID, processorThreadID, writerThreadID;             // Thread ID
sem_t sem_read, sem_process, sem_write; //Create semaphores

/* --- Main Code --- */
int main(int argc, char const *argv[])
{
  struct timeval t1;
  gettimeofday(&t1, NULL); // Start Timer
  pthread_attr_t threadAttributes;

  int rowNumber = 0;             //Track row number
  int pipeFileDescriptor[2];  //File descriptor for creating a pipe
  DataRow shared_memory[50]; //Create shared memory buffer

  ReadParams readParams = {rowNumber, pipeFileDescriptor};
  ProcessorParams processorParams = {rowNumber, pipeFileDescriptor, shared_memory};
  WriterParams writerParams = {rowNumber, shared_memory};

  // Initialisation
  initialiseData(NULL);
  pthread_attr_init(&threadAttributes);

  // Create pipe
  if (pipe(pipeFileDescriptor) < 0){
    perror("Pipe creation error");
    exit(EXIT_FAILURE);
  }

  // Create Threads
  if (pthread_create(&readerThreadID, &threadAttributes, Reader, &readParams) != 0){
    perror("Error creating Reader thread");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&processorThreadID, &threadAttributes, Processor, &processorParams) != 0){
    perror("Error creating Processor thread");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&writerThreadID, &threadAttributes, Writer, &writerParams) != 0){
    perror("Error creating Writer thread");
    exit(EXIT_FAILURE);
  }

  // Wait on threads to finish
  pthread_join(readerThreadID, NULL);
  pthread_join(processorThreadID, NULL);
  pthread_join(writerThreadID, NULL);

  printf("Output saved to %s\n", OUTPUT_FILE_NAME);

  //TODO: add your code
  close(pipeFileDescriptor[0]);
  close(pipeFileDescriptor[1]);

  printf("Done\n");
  return 0;
}

void initialiseData()
{
  printf("Initialising...\n");

  // Initialise Sempahores
  if (sem_init(&sem_read, 0, 1)){
    perror("Error initializing read semaphore.");
    exit(EXIT_FAILURE);
  }

  if (sem_init(&sem_process, 0, 0)){
    perror("Error initializing process semaphore.");
    exit(EXIT_FAILURE);
  }

  if (sem_init(&sem_write, 0, 0)){
    perror("Error initializing write semaphore.");
    exit(EXIT_FAILURE);
  }

  return;
}

void *Reader(void * params)
{
  ReadParams * parameters = params;
  char row[BUFFER_SIZE];
  FILE *readFile;
  char fileName[10] = INPUT_FILE_NAME;

  printf("Reading from %s\n", fileName);

  //Open INPUT_FILE_NAME
  if ((readFile = fopen(fileName, "r")) == NULL){
    printf("Error! opening %s\n", fileName);
    exit(ENOENT); /* No such file or directory */
  }

  while (!sem_wait(&sem_read) && fgets(row, BUFFER_SIZE, readFile) != NULL){
    if ((write(parameters->pipePrt[1], row, strlen(row) + 1) < 1)){
      perror("Write");
      exit(EPIPE); /* Broken pipe */
    }

    parameters->rowNumber++;
    sem_post(&sem_process);
  }

  close(parameters->pipePrt[1]);
  fclose(readFile);

  //Cancel threads - might not be the best way to do this
  pthread_cancel(readerThreadID);
  pthread_cancel(processorThreadID);
  pthread_cancel(writerThreadID);
  pthread_exit(0);
}

void *Processor(void *params)
{
  ProcessorParams * parameters = params;
  enum fileRegion region = Header;
  char headerRow[sizeof(END_OF_HEADER)] = END_OF_HEADER;

  while (!sem_wait(&sem_process)){
    char readBuffer[BUFFER_SIZE];

    // Read pipe and copy to readBuffer
    if ((read(parameters->pipePrt[0], &readBuffer, BUFFER_SIZE) < 1)){
      perror("Read");
      exit(EPIPE); /* Broken pipe */
    }

    // Instantiate DataRow object
    struct DataRow dataRow = {region};
    strncpy(dataRow.content, readBuffer, sizeof(dataRow.content) - 1);

    // Copy DataRow object to shared memory that exists between processor and writer threads
    *(parameters->shared_memory[parameters->rowNumber]) = dataRow;

    /* Check whether this row is the end of header,
    the new row in array c contains "end_header\n"*/
    if (region == Header && strstr(readBuffer, headerRow) != NULL){
      region = Content;
    }

    sem_post(&sem_write);
  }

  close(parameters->pipePrt[0]);
  pthread_exit(NULL);
}

void *Writer(void * params)
{
  WriterParams * parameters = params;
  FILE *writeFile;
  char outputFileName[sizeof(OUTPUT_FILE_NAME)] = OUTPUT_FILE_NAME;

  // Create or open the file we want to output the content to
  if ((writeFile = fopen(outputFileName, "w")) == NULL){
    printf("Error! opening creating or opening existing output file\n");
    exit(EXIT_FAILURE);
  }

  while (!sem_wait(&sem_write)){
    /* Writes rows in the Content region to the output file */
    if (parameters->shared_memory[parameters->rowNumber]->region == Content){
      fprintf(writeFile, "%s", parameters->shared_memory[parameters->rowNumber]->content);
    }

    sem_post(&sem_read);
  }

  fclose(writeFile);
  pthread_exit(NULL);
}
