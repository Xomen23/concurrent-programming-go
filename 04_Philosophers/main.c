#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_PHILOSOPHERS 5
#define ITERATIONS 3

static pthread_mutex_t sticks[NUM_PHILOSOPHERS];

static void *philosopher(void *arg) {
    int id = *(int *)arg;
    free(arg);

    int left = id;
    int right = (id + 1) % NUM_PHILOSOPHERS;

    for (int i = 0; i < ITERATIONS; i++) {
        printf("Filozof %d razmislja.\n", id);
        fflush(stdout);
        usleep((10 + rand() % 50) * 1000);

        printf("Filozof %d pokusava da uzme stapice.\n", id);
        fflush(stdout);

        /* ASIMETRIČNO REŠENJE: Neparni filozofi uzimaju kontra redosledom */
        if (id % 2 != 0) {
            pthread_mutex_lock(&sticks[right]);
            pthread_mutex_lock(&sticks[left]);
        } else {
            pthread_mutex_lock(&sticks[left]);
            pthread_mutex_lock(&sticks[right]);
        }

        printf("Filozof %d je uspesno uzeo oba stapica i jede.\n", id);
        fflush(stdout);
        usleep((10 + rand() % 50) * 1000);

        /* Vraćanje štapića */
        pthread_mutex_unlock(&sticks[left]);
        pthread_mutex_unlock(&sticks[right]);

        printf("Filozof %d je vratio stapice.\n", id);
        fflush(stdout);
    }

    return NULL;
}

int main(void) {
    srand(time(NULL));
    pthread_t phils[NUM_PHILOSOPHERS];

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_init(&sticks[i], NULL);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&phils[i], NULL, philosopher, id);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_join(phils[i], NULL);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_destroy(&sticks[i]);
    }

    printf("\n[USPEH] Svi filozofi su zavrsili sa jelom i razmisljanjem!\n");
    return 0;
}