/*********************************************************
   ----- 48450 -- Program 1 by Patrice Harapeti ------

Description:

This program calculates the average wait time and average turnaround time
of processes in the ready state when using the Shortest Remaining Time First (SRTF)
CPU scheduling algorithm.

Compilation instructions:

gcc -Wall -pthread -O2 -o program_1 program_1.c

Usage:

./program_1
./program_1 <output filename>

*********************************************************/

#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* a struct to store data about each process */
typedef struct {
  // process id
  int pid;

  // time when the process arrives enters into the ready state and can be executed
  float arrive_t;

  // total time spent by the process in the ready state waiting for the CPU
  float wait_t;

  // total time taken by the process for its execution
  float burst_t;

  // total time spent by the process from being in the ready state for the first time to it's completion
  float turnaround_t;

  // time when the process beings execution
  float start_t;
} process;

/* Sorts the processes in burst time order (bubble sort) */
void bubble_sort(process p[]);

// SRTF variables
float avg_wait_t = 0.0, avg_turnaround_t = 0.0;
int Process_start = 0;
float time_residue;
int processNum = 7;
process *processes;

// Semaphore
sem_t sem_SRTF;

// Threads
pthread_t processor, writer;

//IO
char *outputFileName = "output.txt";
char *namedFIFOname = "/tmp/myfifo1";

/*------------------- functions ------------------------*/
// Performs the SRTF CPU Scheduling Algorithm to calculate average wait time and turnaround time
void perform_srtf();

// Send and write average wait time and turnaround time to fifo
void send_FIFO();

// Print results of SRTF algorithm to the console
void print_results();

// Read average wait time and turnaround time from the named fifo then write to the output file
void read_FIFO();

// Routine for Processor Thread
void processor_routine();

// Routine for Writer thread
void writer_routine();

/*------------------- implementation ------------------------*/
int main(int argc, char *argv[]) {
  // Ensure that the program has been invoked correctly
  if (argc < 1 || argc > 2) {
    fprintf(stderr, "USAGE:\n");
    fprintf(stderr, "./program_1.out\n");
    fprintf(stderr, "./program_1.out <output filename>\n");
    exit(EXIT_FAILURE);
  }

  // Override default output filename if argument is specified
  if (argv[1] != NULL) {
    outputFileName = argv[1];
  }

  processes = malloc(sizeof(process) * processNum);

  if(processes == NULL){
	fprintf("error allocating memory\n");
	exit(EXIT_FAILURE);
  }

  processes[0].pid = 1; processes[0].arrive_t = 8; processes[0].burst_t = 10;
  processes[1].pid = 2; processes[1].arrive_t = 10; processes[1].burst_t = 3;
  processes[2].pid = 3; processes[2].arrive_t = 14; processes[2].burst_t = 7;
  processes[3].pid = 4; processes[3].arrive_t = 9; processes[3].burst_t = 5;
  processes[4].pid = 5; processes[4].arrive_t = 16; processes[4].burst_t = 4;
  processes[5].pid = 6; processes[5].arrive_t = 21; processes[5].burst_t = 6;
  processes[6].pid = 7; processes[6].arrive_t = 26; processes[6].burst_t = 2;

  if (sem_init(&sem_SRTF, 0, 0) != 0) {
    fprintf(stderr, "semaphore initialize error \n");
    exit(EXIT_FAILURE);
  }

  if (pthread_create(&processor, NULL, (void *)processor_routine, NULL) != 0) {
    fprintf(stderr, "Processor Thread created error\n");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&writer, NULL, (void *)writer_routine, NULL) != 0) {
    fprintf(stderr, "Writer thread created error\n");
    exit(EXIT_FAILURE);
  }

  if (pthread_join(processor, NULL) != 0) {
    fprintf(stderr, "join processor thread error\n");
    exit(EXIT_FAILURE);
  }
  if (pthread_join(writer, NULL) != 0) {
    fprintf(stderr, "join writer thread error\n");
    exit(EXIT_FAILURE);
  }

  if (sem_destroy(&sem_SRTF) != 0) {
    fprintf(stderr, "Semaphore destroy error\n");
    exit(EXIT_FAILURE);
  }

  return 0;
}

// Processor Thread of assignment
void processor_routine() {
  perform_srtf();
  print_results();
  send_FIFO();
}

// Writer thread of assignment
void writer_routine() {
  if (sem_wait(&sem_SRTF) == -1) {
    fprintf(stderr, "semaphore lock error\n");
    exit(EXIT_FAILURE);
  }
  read_FIFO();
}

// Performs the SRTF CPU Scheduling Algorithm to calculate average wait time and turnaround time
void perform_srtf() {
  time_residue = processes[0].arrive_t + 1;
  bubble_sort(processes);

  for (int i = 0; i < processNum; i++) {
    if (processes[i].arrive_t <= Process_start) { // set the rest process' start time
      if (processes[i].arrive_t == 0) {
        Process_start -= time_residue;
      }
      processes[i].start_t = Process_start; // set up the current process' start time
    } else {
		// set the shortest burst process' start time
      	processes[i].start_t = processes[i].arrive_t;
      	if (processes[i].arrive_t > 0) {
        	Process_start += time_residue;
      	}
    }

    /* set the global start time to the end of the process done time */
    Process_start += processes[i].burst_t;

	/* set the wait time as CPU start time minus process arrive time */
    processes[i].wait_t = processes[i].start_t - processes[i].arrive_t;

    /* set turn around time as bust time plus wait time */
    processes[i].turnaround_t = processes[i].burst_t + processes[i].wait_t;

	avg_wait_t += processes[i].wait_t;
    avg_turnaround_t += processes[i].turnaround_t;
  }

	avg_wait_t /= processNum;
	avg_turnaround_t /= processNum;
}

// Print results, taken from sample
void print_results() {
  printf("Process Schedule Table: \n");
  printf("\tProcess ID\tArrival Time\tBurst Time\tWait Time\tTurnaround Time\n");
  for (int i = 0; i < processNum; i++) {
    printf(
		"\t%d\t\t%f\t%f\t%f\t%f\n",
		processes[i].pid,
		processes[i].arrive_t,
        processes[i].burst_t,
		processes[i].wait_t,
		processes[i].turnaround_t
	);
  }

  printf("Average wait time of each process: %fs\n", avg_wait_t);
  printf("Average turnaround time of each process: %fs\n", avg_turnaround_t);
}

// Send and write average wait time and turnaround time to fifo
void send_FIFO() {
  int res, fifofd;

  if ((res = mkfifo(namedFIFOname, 0777)) < 0) {
    fprintf(stderr, "mkfifo error\n");
    exit(EXIT_FAILURE);
  }

  if (sem_post(&sem_SRTF) == -1) {
    fprintf(stderr, "semaphore unlock error\n");
    exit(EXIT_FAILURE);
  }

  if ((fifofd = open(namedFIFOname, O_WRONLY)) < 0) {
    fprintf(stderr, "fifo open send error\n");
    exit(EXIT_FAILURE);
  }

  if (write(fifofd, &avg_wait_t, sizeof(avg_wait_t)) == -1) {
    fprintf(stderr, "Cannot write to FIFO\n");
    exit(EXIT_FAILURE);
  }

  if (write(fifofd, &avg_turnaround_t, sizeof(avg_turnaround_t)) == -1) {
    fprintf(stderr, "Cannot write to FIFO\n");
    exit(EXIT_FAILURE);
  }

  if (close(fifofd) == -1) {
    fprintf(stderr, "Cannot close FIFO\n");
    exit(EXIT_FAILURE);
  }
}

// Read average wait time and turnaround time from fifo then write to the output file
void read_FIFO() {
  int fifofd;
  float fifo_avg_turnaround_t, fifo_avg_wait_t;

  if ((fifofd = open(namedFIFOname, O_RDONLY)) < 0) {
    fprintf(stderr, "fifo open read error\n");
    exit(EXIT_FAILURE);
  }

  if (read(fifofd, &fifo_avg_wait_t, sizeof(int)) == -1) {
    fprintf(stderr, "Cannot read from FIFO\n");
    exit(EXIT_FAILURE);
  }

  if (read(fifofd, &fifo_avg_turnaround_t, sizeof(int)) == -1) {
    fprintf(stderr, "Cannot read from FIFO\n");
    exit(EXIT_FAILURE);
  }

  if (close(fifofd) == -1) {
    fprintf(stderr, "Cannot close named FIFO\n");
    exit(EXIT_FAILURE);
  }

  if (remove(namedFIFOname) == -1) {
    fprintf(stderr, "Cannot remove named FIFO\n");
    exit(EXIT_FAILURE);
  }

  // Write to file
  FILE *file_to_write;

  if ((file_to_write = fopen(outputFileName, "w")) == NULL) {
    fprintf(stderr, "Error! opening file");
    exit(EXIT_FAILURE);
  }

  fprintf(file_to_write, "Read from FIFO: %fs Average wait time\n", fifo_avg_wait_t);
  fprintf(file_to_write, "Read from FIFO: %fs Average turnaround time\n", fifo_avg_turnaround_t);

  if (fclose(file_to_write) != 0) {
    fprintf(stderr, "Error closing stream");
    exit(EXIT_FAILURE);
  }
}

void bubble_sort(process p[]) {
  process temp;

  // Sort processes in order of smallest to largest burst
  for (int i = 0; i < processNum; i++) {
    for (int j = i + 1; j < processNum; j++) {
      if (p[i].burst_t > p[j].burst_t) {
        temp = p[i];
        p[i] = p[j];
        p[j] = temp;
      }
    }
  }
}
