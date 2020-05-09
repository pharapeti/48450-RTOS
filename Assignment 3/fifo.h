struct FIFO {
	int front, rear, size;
	unsigned capacity;
	int * array;
};

struct FIFO * createFIFO(unsigned capacity);
bool isFull(struct FIFO * fifo);
bool isEmpty(struct FIFO * fifo);
void enqueue(struct FIFO * fifo, int item);
int dequeue(struct FIFO * fifo);
int front(struct FIFO * fifo);