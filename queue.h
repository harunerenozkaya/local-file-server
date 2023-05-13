#ifndef QUEUE_H
#define QUEUE_H

struct QueueNode {
   char* data;
   struct QueueNode* next;
};

struct Queue {
   struct QueueNode* front;
   struct QueueNode* rear;
};

typedef struct Queue queue_t;
typedef struct QueueNode node_t;

queue_t createQueue();
int isEmpty(queue_t* queue);
void enqueue(queue_t* queue, char* data);
char* dequeue(queue_t* queue);
void printQueue(queue_t* queue);

#endif
