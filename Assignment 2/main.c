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
#include <signal.h>

#define BUFFER_SIZE 1024
#define MAX_ARGUMENT_LENGTH 100
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
  char content[BUFFER_SIZE];
} DataRow;

typedef struct
{
  int rowNumber;
  char * inputFileName;
  int * pipePrt;
} ReadParams;

typedef struct
{
  int rowNumber;
  int *pipePrt;
  DataRow * sharedBuffer;
} ProcessorParams;

typedef struct
{
  int rowNumber;
  char * outputFileName;
  DataRow * sharedBuffer;
} WriterParams;

/* --- Prototypes --- */

/* Initializes the three semaphores that are used to control the order of execution of the threads */
void initialiseSempahores();

/* Handles the Ctrl+C signal interrupt and safely exits the program */
void handleInterupt();

/* A thread which reads data from input file and writes each row to a pipe */
void *Reader(void * params);

/* A thread which reads data from pipe and writes it to a shared message */
void *Processor(void * params);

/* A thread which reads from shared message and writes non-header text to the output file */
void *Writer(void * params);

pthread_t readerThreadID, processorThreadID, writerThreadID;    //Thread ID
sem_t sem_read, sem_process, sem_write;                         //Create semaphores

int main(int argc, char const *argv[])
{
  /* Handles the Ctrl+C signal interrupt and safely exits the program */
  signal(SIGINT, handleInterupt);

  // Ensure that the program has been invoked correctly
  if (argc < 1 || argc > 3) {
		fprintf(stderr, "USAGE:\n./main.out\n./main.out <input file>\n./main.out <input file> <output file>\n");
    exit(EXIT_FAILURE);
	}

  // Assign default input and output file names
  char inputFileName[MAX_ARGUMENT_LENGTH] = "data.txt";
  char outputFileName[MAX_ARGUMENT_LENGTH] = "output.txt";

  // Override the default input and output file names if they have been specified by the user
  switch(argc) {
    case 2:
      if(strlen(argv[1]) > MAX_ARGUMENT_LENGTH){
        fprintf(stderr, "The <output file> specified exceeded %i characters.\n", MAX_ARGUMENT_LENGTH);
        fprintf(stderr, "Exiting program...\n");
        exit(EXIT_FAILURE);
      }

      strncpy(inputFileName, argv[1], MAX_ARGUMENT_LENGTH);
      break;
    case 3:
      if(strlen(argv[1]) > MAX_ARGUMENT_LENGTH){
        fprintf(stderr, "The <input file> specified exceeded %i characters.\n", MAX_ARGUMENT_LENGTH);
        fprintf(stderr, "Exiting program...\n");
        exit(EXIT_FAILURE);
      } else if(strlen(argv[2]) > MAX_ARGUMENT_LENGTH){
        fprintf(stderr, "The <output file> specified exceeded %i characters.\n", MAX_ARGUMENT_LENGTH);
        fprintf(stderr, "Exiting program...\n");
        exit(EXIT_FAILURE);
      }

      strncpy(inputFileName, argv[1], MAX_ARGUMENT_LENGTH);
      strncpy(outputFileName, argv[2], MAX_ARGUMENT_LENGTH);
      break;
    default:
      break;
  }

  /* Initialisaton*/
  int rowNumber = 0;                //Track row number
  int pipeFileDescriptor[2];        //File descriptor for creating a pipe
  DataRow sharedBuffer;        //Create shared memory buffer
  pthread_attr_t threadAttributes;  //Create pthread thread attributes object

  // Instantiate thread paramater structures for each thread
  ReadParams readParams = {rowNumber, inputFileName, pipeFileDescriptor};
  ProcessorParams processorParams = {rowNumber, pipeFileDescriptor, &sharedBuffer};
  WriterParams writerParams = {rowNumber, outputFileName, &sharedBuffer};

  initialiseSempahores(NULL);
  pthread_attr_init(&threadAttributes);

  // Create pipe between Processor and Writer thread
  if (pipe(pipeFileDescriptor) < 0){
    perror("Pipe creation error");
    exit(EXIT_FAILURE);
  }

  // Create the Reader, Processor and Writer thread
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

  printf("The content region of %s has been saved to %s\n", inputFileName, outputFileName);

  //Close pipes
  close(pipeFileDescriptor[0]);
  close(pipeFileDescriptor[1]);

  printf("Exiting program...\n");
  return 0;
}

void initialiseSempahores()
{
  printf("Initialising program...\n");

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

void handleInterupt(int signalNumber){
  printf("Interrupt deteced: safely exiting program...\n");
  exit(EXIT_FAILURE);
}

void *Reader(void * params)
{
  ReadParams * parameters = params;
  char row[BUFFER_SIZE];
  FILE *readFile;

  //Open the input file for reading
  if ((readFile = fopen(parameters->inputFileName, "r")) == NULL){
    printf(
      "Error: Could not find or open %s. Ensure that a file named %s exists within the same directory.\n",
      parameters->inputFileName, parameters->inputFileName
    );
    printf("Exiting program...\n");
    exit(ENOENT); /* No such file or directory */
  }

  printf("Reading from %s\n", parameters->inputFileName);

  while (!sem_wait(&sem_read) && fgets(row, BUFFER_SIZE, readFile) != NULL){
    //Write data from file to pipe between the Reader and Processor thread
    if ((write(parameters->pipePrt[1], row, strlen(row) + 1) < 1)){
      perror("Write");
      exit(EPIPE); /* Broken pipe */
    }

    parameters->rowNumber++;
    sem_post(&sem_process);
  }

  printf("Finished reading %s\n", parameters->inputFileName);

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

    // Instantiate DataRow object with the current value of region
    struct DataRow dataRow = {region};

    //Copy data from read buffer to DataRow object
    strncpy(dataRow.content, readBuffer, sizeof(dataRow.content) - 1);

    // Copy DataRow object to shared memory that exists between processor and writer threads
    *(parameters->sharedBuffer) = dataRow;

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

  // Create or open the output file we want to output the content to
  if ((writeFile = fopen(parameters->outputFileName, "w")) == NULL){
    printf("Error! opening creating or opening existing output file\n");
    exit(EXIT_FAILURE);
  }

  while (!sem_wait(&sem_write)){
    /* Writes rows in the Content region to the output file */
    if (parameters->sharedBuffer->region == Content){
      fprintf(writeFile, "%s", parameters->sharedBuffer->content);
    }
    sem_post(&sem_read);
  }

  fclose(writeFile);
  pthread_exit(NULL);
}
