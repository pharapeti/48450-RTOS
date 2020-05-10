/*********************************************************
   ----- 48450 -- week 8 lab handout 2 ------ 
This is a program to practice CPU Scheduling algorithm about shortest remaining time first

/* The input data of the cpu scheduling algorithm is:
--------------------------------------------------------
Process ID           Arrive time          Burst time
    1					0	    	    	10
    2                   7                   31
    3                   12                  12
    4                   3                   8
    5                   16                  5
    6                   30                  19
    7                   15                  6
    8                   30                  13
--------------------------------------------------------

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

/*---------------------------------- Variables -------------------------------*/
//a struct storing the information of each process
typedef struct
{
     int pid;//process id
     int arrive_t, wait_t, burst_t, turnaround_t, remain_t;//process time
}process;

//Max number of processes
#define PROCESSNUM 8
//Array of processes with 1 extra for placeholder remain_t
process processes[9];
//Index variable
int i;
//Averages calculated
float avg_wait_t = 0.0, avg_turnaround_t = 0.0;
//Semaphore
sem_t sem_SRTF;
//Pthreads
pthread_t thread1, thread2;

/*---------------------------------- Functions -------------------------------*/
//Create process arrive times and burst times, taken from assignment details
void input_processes();
//Schedule processes according to SRTF rule
void process_SRTF();
//Simple calculate average wait time and turnaround time function
void calculate_average();
//Print results, taken from sample
void print_results();
//Thread 1 of assignment
void thread1_routine();
//Thread 2 of assignment
void thread2_routine();

/*---------------------------------- Implementation -------------------------------*/
//Create semaphores, run threads then end threads and semaphores when finished
int main() {
	
	if(sem_init(&sem_SRTF, 0, 0)!=0)
	{
	    printf("semaphore initialize erro \n");
	    return(-10);
	}
	
	if(pthread_create(&thread1, NULL, (void *)thread1_routine, NULL)!=0)
 	{
	    printf("Thread 1 created error\n");
	    return -1;
	}
	if(pthread_create(&thread2, NULL, (void *)thread2_routine, NULL)!=0)
	{
	    printf("Thread 2 created error\n");
	    return -2;
	}
	
	if(pthread_join(thread1, NULL)!=0)
	{
	    printf("join thread 1 error\n");
	    return -3;
	}
	if(pthread_join(thread2, NULL)!=0)
	{
	    printf("join thread 2 error\n");
	    return -4;
	}
	
	if(sem_destroy(&sem_SRTF)!=0)
	{
	    printf("Semaphore destroy error\n");
	    return -5;
	}
	
	return 0;
}

//Thread 1 of assignment
void thread1_routine() {
	input_processes();
	process_SRTF();
	calculate_average();
	sem_post(&sem_SRTF);	
}

//Thread 2 of assignment
void thread2_routine() {
	sem_wait(&sem_SRTF);
	print_results();
}

/* The input data of the cpu scheduling algorithm is:
--------------------------------------------------------
Process ID           Arrive time          Burst time
    1					0		    		10
    2                   7                   31
    3                   12                  12
    4                   3                   8
    5                   16                  5
    6                   30                  19
    7                   15                  6
    8                   30                  13
--------------------------------------------------------
*/
//Create process arrive times and burst times, taken from assignment details
void input_processes() {
	processes[0].pid = 1; processes[0].arrive_t = 0; processes[0].burst_t = 10;
	processes[1].pid = 2; processes[1].arrive_t = 7; processes[1].burst_t = 31;
	processes[2].pid = 3; processes[2].arrive_t = 12; processes[2].burst_t = 12;
	processes[3].pid = 4; processes[3].arrive_t = 3; processes[3].burst_t = 8;
	processes[4].pid = 5; processes[4].arrive_t = 16; processes[4].burst_t = 5;
	processes[5].pid = 6; processes[5].arrive_t = 30; processes[5].burst_t = 19;
	processes[6].pid = 7; processes[6].arrive_t = 15; processes[6].burst_t = 6;
	processes[7].pid = 8; processes[7].arrive_t = 30; processes[7].burst_t = 13;
	
	//Initialise remaining time to be same as burst time
	for (i = 0; i < PROCESSNUM; i++) {
		processes[i].remain_t = processes[i].burst_t;
	}
}

//Schedule processes according to SRTF rule
void process_SRTF() {
	
    int endTime, smallest, time, remain = 0;
	
    //Placeholder remaining time to be replaced
    processes[8].remain_t=9999;
	
    //Run function until remain is equal to number of processes
    for (time = 0; remain != PROCESSNUM; time++) {
		
	//Assign placeholder remaining time as smallest
        smallest = 8;
		
	//Check all processes that have arrived for lowest remain time then set the lowest to be smallest
        for (i=0;i<PROCESSNUM;i++) {
            if (processes[i].arrive_t <= time && processes[i].remain_t < processes[smallest].remain_t && processes[i].remain_t > 0) {
                smallest = i;
            }
        }
		
	//Decrease remaining time as time increases
        processes[smallest].remain_t--;
		
	//If process is finished, save time information, add to average totals and increase remain
        if (processes[smallest].remain_t == 0) {
			
            remain++;
			
            endTime=time+1;
			
	    processes[smallest].turnaround_t = endTime-processes[smallest].arrive_t;
			
	    processes[smallest].wait_t = endTime-processes[smallest].burst_t-processes[smallest].arrive_t;
			
	    avg_wait_t += processes[smallest].wait_t;
			
	    avg_turnaround_t += processes[smallest].turnaround_t;
        }
    }
	
}

//Simple calculate average wait time and turnaround time function
void calculate_average() {
	avg_wait_t /= PROCESSNUM;
	avg_turnaround_t /= PROCESSNUM;
}

//Print results, taken from sample
void print_results() {
	
	printf("Process Schedule Table: \n");
	
	printf("\tProcess ID\tArrival Time\tBurst Time\tWait Time\tTurnaround Time\n");
	
	for (i = 0; i<PROCESSNUM; i++) {
	  	printf("\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n", processes[i].pid,processes[i].arrive_t, processes[i].burst_t, processes[i].wait_t, processes[i].turnaround_t);
	}
	
	printf("\nAverage wait time: %fs\n", avg_wait_t);
	
	printf("\nAverage turnaround time: %fs\n", avg_turnaround_t);
}


