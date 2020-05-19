struct Queue {
	int front, rear, size;
	unsigned capacity;
	int * array;
};

struct Queue * createQueue(unsigned capacity);
bool isFull(struct Queue * queue);
bool isEmpty(struct Queue * queue);
void enqueue(struct Queue * queue, int item);
int dequeue(struct Queue * queue);
int front(struct Queue * queue);