#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

/* Function to create an empty queue */
queue_t createQueue() {
    queue_t* queue = (queue_t*)malloc(sizeof(queue_t));
    queue->front = NULL;
    queue->rear = NULL;
    return (*queue);
}

/* Function to check if the queue is empty */
int isEmpty(queue_t* queue) {
    return (queue->front == NULL);
}

/* Function to add an element to the rear of the queue */
void enqueue(queue_t* queue, char* data) {
    node_t* newNode = (node_t*)malloc(sizeof(node_t));
    newNode->data = (char*)malloc(strlen(data) + 1);
    strcpy(newNode->data, data);
    newNode->next = NULL;
    
    if (isEmpty(queue)) {
        queue->front = newNode;
        queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
}

/* Function to remove an element from the front of the queue */
char* dequeue(queue_t* queue) {
    if (isEmpty(queue)) {
        printf("Queue is empty.\n");
        return NULL;
    }
    
    node_t* temp = queue->front;
    char* data = malloc(sizeof(temp->data));
    strcpy(data,temp->data);
    
    queue->front = queue->front->next;
    free(temp->data);
    free(temp);
    
    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    
    return data;
}

/* Function to print the contents of the queue */
void printQueue(queue_t* queue) {
    if (isEmpty(queue)) {
        printf("Queue is empty.\n");
        return;
    }
    
    node_t* temp = queue->front;
    
    while (temp != NULL) {
        printf("%s ", temp->data);
        temp = temp->next;
    }
    
    printf("\n");
}
