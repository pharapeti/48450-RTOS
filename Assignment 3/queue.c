#include <stdbool.h>
#include <stdlib.h>

struct Queue {
	int front, rear, size;
	unsigned capacity;
	int * array;
};

struct Queue * createQueue(unsigned capacity){
	struct Queue * queue = (struct Queue*) malloc(sizeof(struct Queue));
	queue->capacity = capacity;
	queue->front = 0;
	queue->size = 0;
	queue->rear = capacity - 1;
	queue->array = (int*) malloc(queue->capacity * sizeof(int));
	return queue;
}

bool isFull(struct Queue * queue){
	return queue->size == queue->capacity;
}

bool isEmpty(struct Queue * queue){
	return queue->size == 0;
}

void enqueue(struct Queue * queue, int item){
	if (isFull(queue)) { return; }

	queue->rear = (queue->rear + 1) % queue->capacity;
	queue->array[queue->rear] = item;
	queue->size = queue->size + 1;
} 

int dequeue(struct Queue * queue){
	//Return -1 as error if queue is empty
	if(isEmpty(queue)){ return -1; }

	//Get front value
	int item = queue->array[queue->front];

	//Update front
	queue->front = (queue->front + 1) % queue->capacity;

	//Decrement size of queue
	queue->size = queue->size - 1;

	//Return front value
	return item;
}

int front(struct Queue * queue){
	if(isEmpty(queue)) { return -1; }

	return queue->array[queue->front];
}
