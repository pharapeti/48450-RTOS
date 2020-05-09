/**********************************************************************************

This is a template for the subject of RTOS in University of Technology Sydney(UTS)
Please complete the code based on the assignment requirement.

Assignment 3 Program_2 template

**********************************************************************************/
/*
  To compile prog_1 ensure that gcc is installed and run the following command:
  gcc -Wall -O2 program_2.c -o prog_2 -lpthread -lrt

*/
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#define REFERENCE_STRING_LENGTH 24

//Function prototypes
void SignalHandler(int signal);

void printFrame(int frame[], int frameSize, int noOfFaults);


// data structures
struct FIFO {
	int front, rear, size;
	unsigned capacity;
	int * array;
};

struct FIFO * createFIFO(unsigned capacity){
	struct FIFO * fifo = (struct FIFO*) malloc(sizeof(struct FIFO));
	fifo->capacity = capacity;
	fifo->front = 0;
	fifo->size = 0;
	fifo->rear = capacity - 1;
	fifo->array = (int*) malloc(fifo->capacity * sizeof(int));
	return fifo;
}

bool isFull(struct FIFO * fifo){
	return fifo->size == fifo->capacity;
}

bool isEmpty(struct FIFO * fifo){
	return fifo->size == 0;
}

void enqueue(struct FIFO * fifo, int item){
	if (isFull(fifo)) { return; }

	fifo->rear = (fifo->rear + 1) % fifo->capacity;
	fifo->array[fifo->rear] = item;
	fifo->size = fifo->size + 1;
} 

int dequeue(struct FIFO * fifo){
	//Return -1 as error if fifo is empty
	if(isEmpty(fifo)){ return -1; }

	//Get front value
	int item = fifo->array[fifo->front];

	//Update front
	fifo->front = (fifo->front + 1) % fifo->capacity;

	//Decrement size of fifo
	fifo->size = fifo->size - 1;

	//Return front value
	return item;
}

int front(struct FIFO * fifo){
	if(isEmpty(fifo)) { return -1; }

	return fifo->array[fifo->front];
}

//Current number of page faults in the program
int pageFaults = 0;

//Flag to track whether the program should terminate
bool terminateProgram = false;

int main(int argc, char* argv[])
{
	// Ensure that the program has been invoked correctly
  	if (argc < 1 || argc > 2) {
		fprintf(stderr, "USAGE:\n");
		fprintf(stderr, "./Program_2.out\n");
		fprintf(stderr, "./Program_2.out <frame size>\n");
		exit(EXIT_FAILURE);
	}

	//Register Ctrl+c(SIGINT) signal and call the signal handler for the function.
	signal(SIGINT, SignalHandler);
	
	// Assign a default page number
	int frameSize = 4;

	// Override default if argument is specified
	if(argv[1] != NULL){ 
		// Convert argument to integer (atio returns 0 if the argument is a string)
		frameSize = atoi(argv[1]);

		// Ensure that the frame size is greater than 1
		if(frameSize < 1){
			fprintf(stderr, "The specifed frame size must be a positive integer\n");
			exit(EXIT_FAILURE);
		}
	}

	// Create fifo for algorithm
	struct FIFO * fifo = createFIFO(frameSize);
	
	//Reference string from the assignment outline
	int referenceString[REFERENCE_STRING_LENGTH] = {7,0,1,2,0,3,0,4,2,3,0,3,0,3,2,1,2,0,1,7,0,1,7,5};
	
	//Boolean value for whether there is a match or not.
	bool match = false;

	//Integer for whether to determine if there is a free position in the frame
	int emptyFramePos = -1;
	
	//Current value of the reference string.
	int currentValue;

	//Frame where we will be storing the references. -1 is equivalent to an empty value
	int frame[REFERENCE_STRING_LENGTH];

	for(int x = 0; x < REFERENCE_STRING_LENGTH; x++){
		frame[x] = -1;
	}

	//Print empty frame
	printFrame(frame, frameSize, pageFaults);

	//Loop through the reference string values.
	for(int i = 0; i < REFERENCE_STRING_LENGTH; i++)
	{
		// Set search variables to default
		match = false;
		emptyFramePos = -1;
		currentValue = referenceString[i];

		for(int j = 0; j < frameSize; j++){
			if(currentValue == frame[j]){ match = true; }
			if(frame[j] == -1) { emptyFramePos = j; }
		}

		if(match){
			// Don't need to do anything - the page is already in the frame
			// just continue onto the next page from the reference string
		} else{
			// Increment page fault as the page is not in the frame
			pageFaults++;

			// If there is no space in the frame for a new page
			if(emptyFramePos == -1){
				//Get the page at the front of the FIFO
				int frontOfFIFO = front(fifo);

				//Remove the page at the front of the FIFO
				dequeue(fifo);

				//Determine where to write new frame to
				int writePos = -1;
				for(int pos = 0; pos < frameSize; pos++){
					if(frame[pos] == frontOfFIFO) { writePos = pos; }
				}

				//Add new page to the frame
				frame[writePos] = currentValue;
			} else {
				// Add the page to the frame
				frame[emptyFramePos] = currentValue;
			}

			// Add the page to the back of the queue
			enqueue(fifo, currentValue);
		}

		printFrame(frame, frameSize, pageFaults);
	}

	//Sit here until the ctrl+c signal is given by the user.
	while(!terminateProgram)
	{
		sleep(1);
	}

	// Print out results to user
	printf("\nNumber of page faults: %i\n", pageFaults);
	printf("Frame size: %i\n", frameSize);
	printf("Reference string length: %i\n", REFERENCE_STRING_LENGTH);

	float faultPercentage = ((float) pageFaults / (float) REFERENCE_STRING_LENGTH) * 100;
	printf("Fault percentage: %.2f%%\n", ((signed long)(faultPercentage * 100) * 0.01f));
	return 0;
}

void SignalHandler(int signal)
{
	terminateProgram = true;
}

void printFrame(int frame[], int frameSize, int noOfFaults){
	char * message = malloc(255 * sizeof(char));
	strcat(message, "|");

	int val = -1;
	for(int i = 0; i < frameSize; i++){
		val = frame[i];

		if(val == -1){
			strcat(message, " |");
		} else {
			char str[64];
			sprintf(str, "%i|", val);
			strcat(message, str);
		}
	}

	char * pageFaultMessage = malloc(128 * sizeof(char));
	sprintf(pageFaultMessage, " Page fault count: %i\n", noOfFaults);
	strcat(message, pageFaultMessage);
	printf("%s", message);

	free(message);
	free(pageFaultMessage);
	return;
}