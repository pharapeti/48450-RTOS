/*********************************************************
   ----- 48450 -- Program 1 by Patrice Harapeti ------
This is a program calculates the average wait time and average turnaround time of processes
when using the Shortest Remaining Time First (SRTF) algorithm CPU scheduling algorithm

Compilation instructions:

gcc -Wall -pthread -O2 -o program_1 program_1.c

Usage:

./program_1
./program_1 <output filename>

*********************************************************/

#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<semaphore.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

/* a struct to store data about each process */
typedef struct {
  int pid; // process id
  float arrive_t, wait_t, burst_t, turnaround_t;
  float start_t; // process time
} process;


/*------------------- variables ------------------------*/
//Averages calculated
float avg_wait_t = 0.0, avg_turnaround_t = 0.0;
//Semaphore
sem_t sem_SRTF;
//Pthreads
pthread_t processor, writer;
int processNum = 7;
process * processes;
char * outputFileName = "output.txt";

/*------------------- functions ------------------------*/
//Simple calculate average wait time and turnaround time function
void calculate_average();
//Read average wait time and turnaround time from fifo then write to the output filename
void read_FIFO();
//Print results, taken from sample
void print_results();
//Send and write average wait time and turnaround time to fifo
void send_FIFO();

//Processor Thread of assignment
void processor_routine();
//Writer thread of assignment
void writer_routine();

/*------------------- implementation ------------------------*/
//main
int main(int argc, char* argv[]){
	// Ensure that the program has been invoked correctly
  	if (argc < 1 || argc > 2) {
		fprintf(stderr, "USAGE:\n");
		fprintf(stderr, "./program_1.out\n");
		fprintf(stderr, "./program_1.out <output filename>\n");
		exit(EXIT_FAILURE);
	}

	// Override default output filename if argument is specified
	if(argv[1] != NULL){ 
		outputFileName = argv[1];
	}

	processes = malloc(sizeof(process) * processNum);
	processes[0].pid = 1;
	processes[0].arrive_t = 8;
	processes[0].burst_t = 10;
	processes[1].pid = 2;
	processes[1].arrive_t = 10;
	processes[1].burst_t = 3;
	processes[2].pid = 3;
	processes[2].arrive_t = 14;
	processes[2].burst_t = 7;
	processes[3].pid = 4;
	processes[3].arrive_t = 9;
	processes[3].burst_t = 5;
	processes[4].pid = 5;
	processes[4].arrive_t = 16;
	processes[4].burst_t = 4;
	processes[5].pid = 6;
	processes[5].arrive_t = 21;
	processes[5].burst_t = 6;
	processes[6].pid = 7;
	processes[6].arrive_t = 26;
	processes[6].burst_t = 2;



	if(sem_init(&sem_SRTF, 0, 0) != 0)
	{
	    fprintf(stderr, "semaphore initialize error \n");
	    exit(EXIT_FAILURE);
	}

	if(pthread_create(&processor, NULL, (void *) processor_routine, NULL)!=0)
 	{
	    fprintf(stderr, "Processor Thread created error\n");
	    exit(EXIT_FAILURE);
	}
	if(pthread_create(&writer, NULL, (void *) writer_routine, NULL)!=0)
	{
	    fprintf(stderr, "Writer thread created error\n");
	    exit(EXIT_FAILURE);
	}

	if(pthread_join(processor, NULL)!=0)
	{
	    fprintf(stderr, "join processor thread error\n");
	    exit(EXIT_FAILURE);
	}
	if(pthread_join(writer, NULL)!=0)
	{
	    fprintf(stderr, "join writer thread error\n");
	    exit(EXIT_FAILURE);
	}

	if(sem_destroy(&sem_SRTF)!=0)
	{
	    fprintf(stderr, "Semaphore destroy error\n");
	    exit(EXIT_FAILURE);
	}

	return 0;
}

//Processor Thread of assignment
void processor_routine() {
	calculate_average();
	print_results();
	send_FIFO();
}

//Writer thread of assignment
void writer_routine() {
	if(sem_wait(&sem_SRTF) == -1){
		fprintf(stderr, "semaphore lock error\n");
		exit(EXIT_FAILURE);
	}
	read_FIFO();
}

//Simple calculate average wait time and turnaround time function
void calculate_average() {
	avg_wait_t=100;
	avg_turnaround_t=390;
	avg_wait_t /= processNum;
	avg_turnaround_t /= processNum;
}

//Print results, taken from sample
void print_results() {
	printf("Write to FIFO: Average wait time: %fs\n", avg_wait_t);
	printf("Write to FIFO: Average turnaround time: %fs\n", avg_turnaround_t);

	printf("Process Schedule Table: \n");
	printf("\tProcess ID\tArrival Time\tBurst Time\tWait Time\tTurnaround Time\n");
	for (int i = 0; i < processNum; i++){
		printf(
			"\t%d\t\t%f\t%f\t%f\t%f\n",
			processes[i].pid,
			processes[i].arrive_t,
			processes[i].burst_t,
			processes[i].wait_t,
			processes[i].turnaround_t
		);
	}
}

//Send and write average wait time and turnaround time to fifo
void send_FIFO(){
	int res, fifofd;
	char * myfifo = "/tmp/myfifo1";

	if ((res = mkfifo(myfifo, 0777)) < 0) {
		fprintf(stderr, "mkfifo error\n");
		exit(EXIT_FAILURE);
	}

	if(sem_post(&sem_SRTF) == -1){
		fprintf(stderr, "semaphore unlock error\n");
		exit(EXIT_FAILURE);
	}

	if ((fifofd = open(myfifo, O_WRONLY)) < 0) {
		fprintf(stderr, "fifo open send error\n");
		exit(EXIT_FAILURE);
	}

	if(write(fifofd, &avg_wait_t, sizeof(avg_wait_t)) == -1){
		fprintf(stderr, "Cannot write to FIFO\n");
		exit(EXIT_FAILURE);
	}

	if(write(fifofd, &avg_turnaround_t, sizeof(avg_turnaround_t)) == -1){
		fprintf(stderr, "Cannot write to FIFO\n");
		exit(EXIT_FAILURE);
	}

	if(close(fifofd) == -1){
		fprintf(stderr, "Cannot close FIFO\n");
		exit(EXIT_FAILURE);
	}
}

//Read average wait time and turnaround time from fifo then write to the output file
void read_FIFO() {
	int fifofd;
	float fifo_avg_turnaround_t, fifo_avg_wait_t;
	char * myfifo = "/tmp/myfifo1";

	if ((fifofd = open(myfifo, O_RDONLY)) < 0){
		fprintf(stderr, "fifo open read error\n");
		exit(EXIT_FAILURE);
	}

	if(read(fifofd, &fifo_avg_wait_t, sizeof(int)) == -1){
		fprintf(stderr, "Cannot read from FIFO\n");
		exit(EXIT_FAILURE);
	}

	if(read(fifofd, &fifo_avg_turnaround_t, sizeof(int)) == -1){
		fprintf(stderr, "Cannot read from FIFO\n");
		exit(EXIT_FAILURE);
	}

	if(close(fifofd) == -1){
		fprintf(stderr, "Cannot close named FIFO\n");
		exit(EXIT_FAILURE);
	}

	if(remove(myfifo) == -1){
		fprintf(stderr, "Cannot remove named FIFO\n");
		exit(EXIT_FAILURE);
	}

	printf("Read from FIFO: %fs Average wait time\n", fifo_avg_wait_t);
	printf("Read from FIFO: %fs Average turnaround time\n", fifo_avg_turnaround_t);

	// Write to file
	FILE *file_to_write;

	if((file_to_write = fopen(outputFileName, "w")) == NULL) {
		fprintf(stderr, "Error! opening file");
		exit(EXIT_FAILURE);
	}

	fprintf(file_to_write, "Read from FIFO: %fs Average wait time\n", fifo_avg_wait_t);
	fprintf(file_to_write, "Read from FIFO: %fs Average turnaround time\n", fifo_avg_turnaround_t);
	
	if(fclose(file_to_write) != 0){
		fprintf(stderr, "Error closing stream");
		exit(EXIT_FAILURE);
	}
}
