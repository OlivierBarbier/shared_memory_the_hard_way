#include "./queue.h"

struct queue * queue_construct (unsigned int capacity) {
    struct queue * q;
    if (capacity < 1) {
        return (void *) 0;
    }
    q = (struct queue *) malloc (sizeof (struct queue));
    q->q_length = capacity+1;
    q->q_head = 0;
    q->q_tail = 0;
    q->q_store = (struct message *) malloc (q->q_length * sizeof (struct message));
    return q;
}

void queue_destruct (struct queue * * qp) {
    struct queue * q = *qp;
    free (q->q_store);
    free (q);
    *qp = (struct queue *) 0;
}

unsigned char queue_is_full (struct queue * q) {
    return q->q_head == q->q_tail+1 || 
        (q->q_head == 0 && q->q_tail == q->q_length-1);
}

unsigned char queue_is_empty (struct queue * q) {
    return q->q_head == q->q_tail;
}

int queue_enqueue (struct queue * q, struct message x) {
    if (queue_is_full (q)) {
        return -1;
    }
    q->q_store[q->q_tail] = x;
    q->q_tail = (q->q_tail+1) % (q->q_length);
// printf ("enqueue done!\n");
    return 0;
}

unsigned int queue_dequeue (struct queue * q) {
    unsigned int i;
    if (queue_is_empty (q)) {
        return -1;
    }
    i = q->q_head;
    q->q_head = (q->q_head+1)%(q->q_length);
// printf ("dequeue done!\n");
    return i;
}

/* shared_queue extension */
struct queue * shared_queue_get (char * shm_name, int capacity) {
    int shm_fd;
    struct queue * shm_q;
    void * p;

    if ((shm_fd = shm_open (shm_name, O_RDWR|O_CREAT, 0644)) == -1) {
        perror ("shm_open");
        exit (-1);
    }

    if (ftruncate (shm_fd, getpagesize()) != 0) {
        perror ("ftruncate");
        exit (-1);
    }

    /* allocation du segment de mémoire partagée */
    p = mmap (NULL, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (p == MAP_FAILED) {
        perror ("mmap");
        exit (-1);       
    }
    shm_q = p;

    /* Prochaine case mémoire libre dans le segment de mémoire */
    shm_q->q_store = (void *)shm_q + sizeof (struct queue);

    if (shm_q->q_length == 0) {
        sem_init (&shm_q->sem, 1, 1);
        shm_q->q_head = 0;
        shm_q->q_tail = 0;
        shm_q->q_length = capacity + 1;
    }

    return shm_q;
}

unsigned int shared_queue_dequeue (struct queue * q) {
    q->q_store = (void *)q + sizeof (struct queue);
    return queue_dequeue (q);
}

int shared_queue_enqueue (struct queue * q, struct message x) {
    q->q_store = (void *)q + sizeof (struct queue);
    return queue_enqueue (q, x);
}

struct queue * shared_queue_create (char * shm_name, int capacity) {
    shm_unlink (shm_name);
    return shared_queue_get (shm_name, 1);
}

void shared_queue_send_msg (struct queue *chan, long type, char * msg, pid_t pid)
{
    int res;
    struct message m;
    m.type = type;
    strcpy(m.content, msg);
    // sem_wait (&chan->sem);
    res = shared_queue_enqueue (chan, m);
    // sem_post (&chan->sem);
    if (res == 0){
        // kill (pid, SIGENQUEUE);
    }
}
