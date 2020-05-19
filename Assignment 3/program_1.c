/*********************************************************
   ----- 48450 -- week 8 lab handout 3 ------
This is a program to practice FIFO (named pipe) in multi threads.

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

/*------------------- variables ------------------------*/
//Averages calculated
float avg_wait_t = 0.0, avg_turnaround_t = 0.0;
//Semaphore
sem_t sem_SRTF;
//Pthreads
pthread_t processor, writer;

/*------------------- functions ------------------------*/
//Simple calculate average wait time and turnaround time function
void calculate_average();
//Read average wait time and turnaround time from fifo then write to output.txt
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
int main(){
	if(sem_init(&sem_SRTF, 0, 0)!=0)
	{
	    printf("semaphore initialize erro \n");
	    return(-10);
	}

	if(pthread_create(&processor, NULL, (void *)processor_routine, NULL)!=0)
 	{
	    printf("Processor Thread created error\n");
	    return -1;
	}
	if(pthread_create(&writer, NULL, (void *)writer_routine, NULL)!=0)
	{
	    printf("Writer thread created error\n");
	    return -2;
	}

	if(pthread_join(processor, NULL)!=0)
	{
	    printf("join processor thread error\n");
	    return -3;
	}
	if(pthread_join(writer, NULL)!=0)
	{
	    printf("join writer thread error\n");
	    return -4;
	}

	if(sem_destroy(&sem_SRTF)!=0)
	{
	    printf("Semaphore destroy error\n");
	    return -5;
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
	sem_wait(&sem_SRTF);
	read_FIFO();
}

//Simple calculate average wait time and turnaround time function
void calculate_average() {
	avg_wait_t=100;
	avg_turnaround_t=390;
	int PROCESSNUM=7;

	avg_wait_t /= PROCESSNUM;
	avg_turnaround_t /= PROCESSNUM;
}

//Print results, taken from sample
void print_results() {
	printf("\nWrite to FIFO: Average wait time: %fs\n", avg_wait_t);
	printf("\nWrite to FIFO: Average turnaround time: %fs\n", avg_turnaround_t);
}

//Send and write average wait time and turnaround time to fifo
void send_FIFO(){
	int res, fifofd;
	char * myfifo = "/tmp/myfifo1";

	if ((res = mkfifo(myfifo, 0777)) < 0) {
		printf("mkfifo error\n");
		exit(0);
	}

	sem_post(&sem_SRTF);

	if ((fifofd = open(myfifo, O_WRONLY)) < 0) {
		printf("fifo open send error\n");
		exit(0);
	}

	write(fifofd, &avg_wait_t, sizeof(avg_wait_t));
	write(fifofd, &avg_turnaround_t, sizeof(avg_turnaround_t));

	close(fifofd);
	unlink(myfifo);
}

//Read average wait time and turnaround time from fifo then write to output.txt
void read_FIFO() {
	int fifofd;
	float fifo_avg_turnaround_t, fifo_avg_wait_t;
	char * myfifo = "/tmp/myfifo1";

	if ((fifofd = open(myfifo, O_RDONLY)) < 0){
		printf("fifo open read error\n");
		exit(0);
	}

	read(fifofd, &fifo_avg_wait_t, sizeof(int));
	read(fifofd, &fifo_avg_turnaround_t, sizeof(int));

	close(fifofd);
	remove(myfifo);

	printf("\nRead from FIFO: %fs Average wait time\n", fifo_avg_wait_t);
	printf("\nRead from FIFO: %fs Average turnaround time\n", fifo_avg_turnaround_t);

	// Write to file
	FILE *file_to_write;
	char * file_name = "output.txt";

	if((file_to_write = fopen(file_name, "w")) == NULL) {
		printf("Error! opening file");
		exit(1);
	}

	fprintf(file_to_write, "Read from FIFO: %fs Average wait time\n", fifo_avg_wait_t);
	fprintf(file_to_write, "Read from FIFO: %fs Average turnaround time\n", fifo_avg_turnaround_t);
	fclose(file_to_write);
}
