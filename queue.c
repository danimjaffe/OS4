#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

typedef struct Node {
    const void* data;
    struct Node* next;
} Node;

typedef struct Queue {
    Node* head;
    Node* tail;
    atomic_size_t itemCount;
    atomic_size_t waitingCount;
    atomic_size_t visitedCount;
    mtx_t lock;
    cnd_t itemAvailable;
} Queue;

Queue queue;

void initQueue(void) {
    // Initialize the queue
    queue.head = NULL;
    queue.tail = NULL;
    queue.itemCount=0;
    queue.waitingCount=0;
    queue.visitedCount=0;
    mtx_init(&queue.lock, mtx_plain);
    cnd_init(&queue.itemAvailable);
}

void destroyQueue(void) {
    // Destroy the queue and free resources
    mtx_lock(&queue.lock);
    while (queue.head != NULL) {
        Node* temp = queue.head;
        queue.head = queue.head->next;
        free(temp);
    }
    queue.tail = NULL;
    queue.itemCount= 0;
    queue.waitingCount=0;
    queue.visitedCount= 0;
    mtx_unlock(&queue.lock);

    mtx_destroy(&queue.lock);
    cnd_destroy(&queue.itemAvailable);
}

void enqueue(const void* data) {
    // Add an item to the queue
    mtx_lock(&queue.lock);

    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;

    if (queue.tail == NULL) {
        queue.head = newNode;
        queue.tail = newNode;
    } else {
        queue.tail->next = newNode;
        queue.tail = newNode;
    }

    queue.itemCount+=1;

    if (queue.waitingCount > 0) {
        cnd_signal(&queue.itemAvailable);
    }

    mtx_unlock(&queue.lock);
}

void* dequeue(void)  {
    // Remove and return an item from the queue
    mtx_lock(&queue.lock);

    while (queue.itemCount == 0) {
        queue.waitingCount+=1;
        cnd_wait(&queue.itemAvailable, &queue.lock);
        queue.waitingCount-=1;
    }

    Node* node = queue.head;
    queue.head = queue.head->next;
    if (queue.head == NULL) {
        queue.tail = NULL;
    }

    queue.itemCount-=1;
    queue.visitedCount+=1;

    mtx_unlock(&queue.lock);

    void* data = (void*)node->data;
    free(node);

    return data;
}

bool tryDequeue(void** data) {
    // Try to remove and return an item from the queue, return true if successful
    mtx_lock(&queue.lock);

    if (queue.itemCount == 0) {
        mtx_unlock(&queue.lock);
        return false;
    }

    Node* node = queue.head;
    queue.head = queue.head->next;
    if (queue.head == NULL) {
        queue.tail = NULL;
    }

    queue.itemCount-=1;
    queue.visitedCount+=1;

    mtx_unlock(&queue.lock);

    *data = (void*)node->data;
    free(node);

    return true;
}

size_t size(void) {
    // Get the current size of the queue
    return queue.itemCount;
}

size_t waiting(void) {
    // Get the current number of waiting threads
    return queue.waitingCount;
}

size_t visited(void) {
    // Get the number of items that have passed inside the queue
    return queue.visitedCount;
}