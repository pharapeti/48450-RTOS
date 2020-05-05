/*
  To compile main.c, ensure that gcc and make is installed. Then run the following command:
  make

  To delete the executable and output files created by this program, run the following command:
  make clean

  To run the program, you must use one of the following conventions:
  ./main
  ./main <input file name>
  ./main <input file name> <output file name>
  ./main <input file name> <output file name> <substring>
  note: arguments are restricted to a maximum of 100 characters each
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
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define MAX_ARGUMENT_LENGTH 100

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
  sem_t * sem_read;
  sem_t * sem_process;
  sem_t * sem_write;
} SemaphoreParams;

typedef struct
{
  char * inputFileName;
  int * pipePrt;
  sem_t * read;
  sem_t * process;
} ReadParams;

typedef struct
{
  char * substring;
  int *pipePrt;
  DataRow * sharedBuffer;
  sem_t * process;
  sem_t * write;
} ProcessorParams;

typedef struct
{
  char * outputFileName;
  DataRow * sharedBuffer;
  sem_t * write;
  sem_t * read;
} WriterParams;

/* --- Prototypes --- */

/* Initializes the three semaphores that are used to control the order of execution of the threads */
void initialiseSempahores(void * params);

/* Handles the Ctrl+C signal interrupt and safely exits the program */
void handleInterupt();

/* A thread which reads data from input file and writes each row to a pipe */
void *Reader(void * params);

/* A thread which reads data from pipe and writes it to a shared message */
void *Processor(void * params);

/* A thread which reads from shared message and writes non-header text to the output file */
void *Writer(void * params);

pthread_t readerThreadID, processorThreadID, writerThreadID;    //Thread ID

/* Global flags */
bool dataInFile; //To track whether the input file contains any data
bool substringFound; //To track whether the substring was found in the input file
bool safelyTerminate; //To track whether the user has interrupted the program

int main(int argc, char const *argv[])
{
  /* Handles the Ctrl+C signal interrupt and safely exits the program */
  signal(SIGINT, handleInterupt);

  // Ensure that the program has been invoked correctly
  if (argc < 1 || argc > 4) {
		fprintf(stderr, "USAGE:\n");
    fprintf(stderr, "./main.out\n");
    fprintf(stderr, "./main.out <input file>\n");
    fprintf(stderr, "./main.out <input file> <output file>\n");
    fprintf(stderr, "./main.out <input file> <output file> <substring>\n");
    exit(EXIT_FAILURE);
	}

  // Assign default input and output file names
  char inputFileName[MAX_ARGUMENT_LENGTH] = "data.txt";
  char outputFileName[MAX_ARGUMENT_LENGTH] = "output.txt";
  char substring[MAX_ARGUMENT_LENGTH] = "end_header";

  // Override the default input and output file names if they have been specified by the user 
  for(int i = 1; i < argc; i++){
    if(strlen(argv[i]) > MAX_ARGUMENT_LENGTH){
      fprintf(stderr, "Argument number %i exceeds %i characters.\n", i, MAX_ARGUMENT_LENGTH);
      fprintf(stderr, "Exiting program...\n");
      exit(EXIT_FAILURE);
    } else {
      switch(i){
        case 1:
          strncpy(inputFileName, argv[1], MAX_ARGUMENT_LENGTH);
          break;
        case 2:
          strncpy(outputFileName, argv[2], MAX_ARGUMENT_LENGTH);
          break;
        case 3:
          strncpy(substring, argv[3], MAX_ARGUMENT_LENGTH);
          break;
        default:
          break;
      }
    }
  }

  /* Initialisaton*/
  int pipeFileDescriptor[2];              //File descriptor for creating a pipe
  DataRow sharedBuffer;                   //Create shared memory buffer
  pthread_attr_t threadAttributes;        //Create pthread thread attributes object
  sem_t sem_read, sem_process, sem_write; //Create semaphores

  // Instantiate thread paramater structures for each thread
  SemaphoreParams semParams = {&sem_read, &sem_process, &sem_write};
  ReadParams readParams = {inputFileName, pipeFileDescriptor, &sem_read, &sem_process};
  ProcessorParams processorParams = {substring, pipeFileDescriptor, &sharedBuffer, &sem_process, &sem_write};
  WriterParams writerParams = {outputFileName, &sharedBuffer, &sem_write, &sem_read};

  initialiseSempahores(&semParams);
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
  if(pthread_join(readerThreadID, NULL) != 0){
    perror("Error joining Reader thread");
  }
  if(pthread_join(processorThreadID, NULL) != 0){
    perror("Error joining Processor thread");
  }
  if(pthread_join(writerThreadID, NULL) != 0){
    perror("Error joining Writer thread");
  }

  // If the program was not terminated by the user
  // Print a final message to indicate how the program performed
  if(!safelyTerminate){
    if(dataInFile){
      if(substringFound){
        printf("The content region of %s has been saved to %s\n", inputFileName, outputFileName);
      } else {
        printf("The substring '%s' was not found in %s\n", substring, inputFileName);
      }
    } else {
      printf("%s was empty\n", inputFileName);
    }
  }

  printf("Exiting program...\n");
  return 0;
}

void initialiseSempahores(void * params)
{
  SemaphoreParams * parameters = params;
  printf("Initialising program...\n");

  // Initialise Sempahores
  if (sem_init(parameters->sem_read, 0, 1)){
    perror("Error initializing read semaphore.");
    exit(EXIT_FAILURE);
  }

  if (sem_init(parameters->sem_process, 0, 0)){
    perror("Error initializing process semaphore.");
    exit(EXIT_FAILURE);
  }

  if (sem_init(parameters->sem_write, 0, 0)){
    perror("Error initializing write semaphore.");
    exit(EXIT_FAILURE);
  }
  return;
}

void handleInterupt(int signalNumber){
  // Termine the program the Ctrl+C interrupt is raised more than once
  if(safelyTerminate){
    exit(EXIT_FAILURE);
  } else {
    printf("Interrupt detected: safely exiting program...\n");
    safelyTerminate = true;
  }
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

  while (!safelyTerminate && !sem_wait(parameters->read) && fgets(row, BUFFER_SIZE, readFile) != NULL){
    dataInFile = true;
    //Write data from file to pipe between the Reader and Processor thread
    if ((write(parameters->pipePrt[1], row, strlen(row) + 1) < 1)){
      perror("Error writing to pipe");
      exit(EPIPE); /* Broken pipe */
    }
    sem_post(parameters->process);
  }

  if(close(parameters->pipePrt[1]) != 0){
    fprintf(stderr, "Error closing pipe: %s\n", strerror(errno));
  }
  if(fclose(readFile) == EOF){
    fprintf(stderr, "Error closing input file: %s\n", strerror(errno));
  }

  //Send a cancellation request to all other threads
  //This means that the threads will perform their cleanup tasks and then return
  if(pthread_cancel(readerThreadID) != 0){
    perror("Error cancelling Reader thread");
  }
  if(pthread_cancel(processorThreadID) != 0){
    perror("Error cancelling Processor thread");
  }
  if(pthread_cancel(writerThreadID) != 0){
    perror("Error cancelling Writer thread");
  }
  pthread_exit(0);
}

void *Processor(void *params)
{
  ProcessorParams * parameters = params;
  enum fileRegion region = Header;

  while (!safelyTerminate && !sem_wait(parameters->process)){
    char readBuffer[BUFFER_SIZE];

    // Read pipe and copy to readBuffer
    if ((read(parameters->pipePrt[0], &readBuffer, BUFFER_SIZE) < 1)){
      perror("Error reading from the pipe");
      exit(EPIPE); /* Broken pipe */
    }

    // Instantiate DataRow object with the current value of region
    struct DataRow dataRow = {region};

    //Copy data from read buffer to DataRow object
    strncpy(dataRow.content, readBuffer, sizeof(dataRow.content) - 1);

    // Copy DataRow object to shared memory that exists between processor and writer threads
    *(parameters->sharedBuffer) = dataRow;

    /* Check contains the substring. If it does, we update the region  */
    if (region == Header && strstr(readBuffer, parameters->substring) != NULL){
      region = Content;
      substringFound = true;
    }

    sem_post(parameters->write);
  }

  if(close(parameters->pipePrt[0]) != 0){
    fprintf(stderr, "Error closing pipe: %s\n", strerror(errno));
  }
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

  while (!safelyTerminate && !sem_wait(parameters->write)){
    /* Writes rows in the Content region to the output file */
    if (parameters->sharedBuffer->region == Content){
      fprintf(writeFile, "%s", parameters->sharedBuffer->content);
    }
    sem_post(parameters->read);
  }

  if(fclose(writeFile) == EOF){
    fprintf(stderr, "Error closing input file: %s\n", strerror(errno));
  }
  pthread_exit(NULL);
}
