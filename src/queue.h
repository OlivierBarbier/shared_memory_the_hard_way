#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define SIGENQUEUE (SIGUSR1)

struct message {
   long type;
   char content[512];
};

struct queue {
    sem_t sem;
    unsigned int q_head;
    unsigned int q_tail;
    unsigned int q_length;
    struct message * q_store;
};

struct queue * queue_construct (unsigned int capacity);
void queue_destruct (struct queue * * qp);
unsigned char queue_is_full (struct queue * q);
unsigned char queue_is_empty (struct queue * q);
int queue_enqueue (struct queue * q, struct message x);
unsigned int queue_dequeue (struct queue * q);

struct queue * shared_queue_get (char * shm_name, int capacity);
unsigned int shared_queue_dequeue (struct queue * q);
int shared_queue_enqueue (struct queue * q, struct message x);
struct queue * shared_queue_create (char * shm_name, int capacity);
void shared_queue_send_msg (struct queue *chan, long type, char * msg, pid_t pid);
