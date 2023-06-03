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
}

void destroyQueue(void) {
    // Destroy the queue and free resources
}

void enqueue(const void* data) {
    // Add an item to the queue
}

void* dequeue(void)  {
    // Remove and return an item from the queue
}

bool tryDequeue(void** data) {
    // Try to remove and return an item from the queue, return true if successful
}

size_t size(void) {
    // Get the current size of the queue
}

size_t size(void) {
    // Get the current number of waiting threads
}

size_t visited(void) {
    // Get the number of items that have passed inside the queue
}