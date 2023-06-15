#include <stdlib.h>
#include <stdbool.h>
#include <threads.h>
#include <stdatomic.h>

// Node structure for the queue
typedef struct Node {
    void *data;
    struct Node *next;
} Node;

// Thread structure
typedef struct Thread {
    cnd_t cnd;
    struct Thread *next;
} Thread;

// Queue structure
typedef struct {
    Node *head;
    Node *tail;
    atomic_size_t itemCount;
    atomic_size_t visitedCount;
    atomic_size_t waitingCount;
    Thread *waitingThreads;
    mtx_t lock;
} Queue;

// Global queue instance
Queue queue;


// Initialize the queue
void initQueue(void) {
    queue.head = NULL;
    queue.tail = NULL;
    queue.itemCount = 0;
    queue.visitedCount = 0;
    queue.waitingThreads = NULL;
    mtx_init(&queue.lock, mtx_plain);
}

// Destroy the queue
void destroyQueue(void) {
    mtx_destroy(&queue.lock);
}

// Add an item to the queue
void enqueue(void *data) {
    Node *newNode = malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;

    mtx_lock(&queue.lock);

    if (queue.head == NULL) {
        queue.head = newNode;
        queue.tail = newNode;
    } else {
        queue.tail->next = newNode;
        queue.tail = newNode;
    }

    queue.itemCount++;
    queue.visitedCount++;

    // Wake up a waiting thread if there is one
    if (queue.waitingThreads != NULL) {
        Thread *thread = queue.waitingThreads;
        queue.waitingThreads = thread->next;
        cnd_signal(&thread->cnd);
    }

    mtx_unlock(&queue.lock);
}

// Remove an item from the queue (block if empty)
void *dequeue(void) {
    mtx_lock(&queue.lock);

    while (queue.itemCount == 0) {
        Thread thread;
        cnd_init(&thread.cnd);
        thread.next = queue.waitingThreads;
        queue.waitingThreads = &thread;

        queue.waitingCount++;

        cnd_wait(&thread.cnd, &queue.lock);

        queue.waitingCount--;
        cnd_destroy(&thread.cnd);
    }

    Node *node = queue.head;
    queue.head = node->next;

    if (queue.head == NULL) {
        queue.tail = NULL;
    }

    void *data = node->data;
    free(node);

    queue.itemCount--;

    mtx_unlock(&queue.lock);

    return data;
}

// Try to remove an item from the queue (return false if empty)
bool tryDequeue(void **data) {
    mtx_lock(&queue.lock);

    if (queue.itemCount == 0) {
        mtx_unlock(&queue.lock);
        return false;
    }

    Node *node = queue.head;
    queue.head = node->next;

    if (queue.head == NULL) {
        queue.tail = NULL;
    }

    *data = node->data;
    free(node);

    queue.itemCount--;

    mtx_unlock(&queue.lock);

    return true;
}

// Return the current amount of items in the queue
size_t size(void) {
    return queue.itemCount;
}

// Return the current amount of threads waiting for the queue to fill
size_t waiting(void) {
    return queue.waitingCount;
}

// Return the amount of items that have passed inside the queue
size_t visited(void) {
    return queue.visitedCount;
}