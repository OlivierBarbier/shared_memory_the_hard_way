#define _GNU_SOURCE

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

int next_client = 0;

struct queue *svrin;

struct cnx_t {
    struct queue *in;
    struct queue *out;
    char pid[100];
} cnx[10];

void accept (char * from_pid) {
    char in [100], out [100];

    strcpy (in, "/client_");
    strcat (in, from_pid);
    strcat (in, "_in");

    strcpy (out, "/client_");
    strcat (out, from_pid);
    strcat (out, "_out");

    printf ("<%s>\n", in);
    printf ("<%s>\n", out);

    cnx[next_client].in  = shared_queue_get (in, 10);
    cnx[next_client].out = shared_queue_get (out, 10);
    strcpy(cnx[next_client].pid, from_pid);

    next_client++;
}

int flag = 0;
void handler (int sig) {

    unsigned int i = 0, j=0, k;
    struct message m;
    char from_pid[7];

    if (sig == SIGENQUEUE) {
        printf ("\n\n\n>>> srv\n");
        sem_wait (&svrin->sem);
        while ( ! queue_is_empty (svrin)) {
            i = shared_queue_dequeue (svrin);

            m = svrin->q_store[i];

            if (m.type == 1) {
                printf ("Demande de cnx pour PID %s\n", m.content);
                accept (m.content);

                sem_wait (&cnx[next_client-1].out->sem);
                shared_queue_send_msg (cnx[next_client-1].in, 1, "Bienvenue sur le chat :-) !\n", atoi(m.content));
                sem_post (&cnx[next_client-1].out->sem);

                kill (atoi(m.content), SIGENQUEUE);
            }

            if (m.type == 3) {
                printf ("Demande de Dcnx pour PID %s\n", m.content);
                for (i=0; i<10; i++) {
                    if (cnx[i].in == (void *)0) {
                        continue;
                    }
                    if (strcmp(cnx[i].pid, m.content)) {
                        cnx[i].in == (void *)0;
                    }
                }
            }
        }
        sem_post (&svrin->sem);
        printf ("<<< srv\n");

        printf ("\n\n\n>>> diffusion\n");
        for (i=0; i<10; i++) {
            if (cnx[i].in == (void *)0) {
                continue;
            }
 
            do {
                sem_wait (&cnx[i].out->sem);
                if (queue_is_empty (cnx[i].out)) {
                    sem_post (&cnx[i].out->sem);
                    break;
                }
                k = shared_queue_dequeue (cnx[i].out);
                m = cnx[i].out->q_store[k];
                sem_post (&cnx[i].out->sem);

                // shared_queue_send_msg (cnx[i].in, 2, "Ack.", atoi(cnx[i].pid));
                for (j=0; j<10; j++) {
                    if (j==i) continue;
                    if (cnx[j].in == (void *)0) continue;
                    printf ("diff message %s to %s\n", m.content, cnx[j].pid);

                    sem_wait (&cnx[j].out->sem);
                    shared_queue_send_msg (cnx[j].in, 2, m.content, atoi(cnx[j].pid));
                    sem_post (&cnx[j].out->sem);
                    
                    kill (atoi(cnx[j].pid), SIGENQUEUE);
                }
            } while (1);
            
        }
        printf ("<<< diffusion\n");
    }
}

int main () {
    signal (SIGENQUEUE, handler);

    printf ("PID: %u\n", (unsigned) getpid());

    svrin = shared_queue_create ("/srv_shm_in", 10);
    
    handler (SIGENQUEUE);

    do {
        pause ();
    }
    while (1);
}
