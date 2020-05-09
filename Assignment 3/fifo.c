#include <stdbool.h>
#include <stdlib.h>

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
