#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "./queue.h"

struct queue *srvin;
struct queue *clientin;
char in [100], out [100];

int srvpid;

void handler (int sig) {
    unsigned int i = 0;
    struct message m;
    
    if (sig == SIGENQUEUE) {
        sem_wait (&clientin->sem);
        while ( ! queue_is_empty (clientin)) {
            i = shared_queue_dequeue (clientin);

            m = clientin->q_store[i];

            printf ("%s", m.content);
        }
        sem_post (&clientin->sem);
    }
}

void quit (int sig) {
    char pid[7];
    sprintf(pid, "%d", getpid());
    sem_wait (&srvin->sem);
    shared_queue_send_msg (srvin, 3, pid, srvpid);
    sem_post(&srvin->sem);
    kill (srvpid, SIGENQUEUE);
    printf ("Good bye!");
    shm_unlink (in);
    shm_unlink (out);
    exit (0);
}

int main (int argc, char * argv[]) {
    char pid[7];
    char str[100];
    char *line = NULL;
    size_t len = 0;
    /* serveur input (via shared queue) */


    struct queue *clientout;

    signal (SIGENQUEUE, handler);
    signal (SIGINT, quit);

    printf ("PID: %u\n", (unsigned) getpid());

    srvin = shared_queue_get ("/srv_shm_in", 10);

    strcpy (in, "/client_");
    sprintf(pid, "%d", getpid());
    strcat (in, pid);
    strcat (in, "_in");

    strcpy (out, "/client_");
    sprintf(pid, "%d", getpid());
    strcat (out, pid);
    strcat (out, "_out");

    // printf ("<%s>\n", in);
    // printf ("<%s>\n", out);

    clientin  = shared_queue_create (in, 10);
    clientout = shared_queue_create (out, 10);

    srvpid = atoi(argv[1]);

    /* cnx */
    sem_wait (&srvin->sem);
    shared_queue_send_msg (srvin, 1, pid, srvpid);
    sem_post(&srvin->sem);
    
    kill (srvpid, SIGENQUEUE);
    
    do {
        getline (&line, &len, stdin);
        sem_wait (&clientout->sem);
        shared_queue_send_msg (clientout, 2, line, srvpid);
        sem_post(&clientout->sem);

        kill (srvpid, SIGENQUEUE);
    }
    while (1);
}
