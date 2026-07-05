#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#define NUM_MOLECULES 10
#define NUM_H (NUM_MOLECULES * 2)
#define NUM_O NUM_MOLECULES

static int h_count = 0;
static int o_count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t sem_h;
sem_t sem_o;
pthread_barrier_t barrier;

void bond(const char* atom_type, int id) {
    printf("Atom %s [%02d] je usao u vezu. Stvara se H2O...\n", atom_type, id);
    fflush(stdout);
}

void* h_thread(void* arg) {
    int id = *(int*)arg;
    free(arg);

    pthread_mutex_lock(&mutex);
    h_count++;

    // Provera da li imamo punu ekipu (2H i 1O)
    if (h_count >= 2 && o_count >= 1) {
        sem_post(&sem_h);
        sem_post(&sem_o);
        h_count -= 2;
        o_count -= 1;
        pthread_mutex_unlock(&mutex);
    }
    else {
        pthread_mutex_unlock(&mutex);
        sem_wait(&sem_h); // Ceka se ostatak ekipe
    }

    bond("H", id);
    pthread_barrier_wait(&barrier); // Sva tri atoma moraju da se sretnu ovde

    return NULL;
}

void* o_thread(void* arg) {
    int id = *(int*)arg;
    free(arg);

    pthread_mutex_lock(&mutex);
    o_count++;

    // Provera da li imamo punu ekipu (2H i 1O)
    if (h_count >= 2 && o_count >= 1) {
        sem_post(&sem_h);
        sem_post(&sem_h);
        h_count -= 2;
        o_count -= 1;
        pthread_mutex_unlock(&mutex);
    }
    else {
        pthread_mutex_unlock(&mutex);
        sem_wait(&sem_o); // Ceka se ostatak ekipe
    }

    bond("O", id);
    pthread_barrier_wait(&barrier); // Sva tri atoma moraju da se sretnu ovde

    return NULL;
}

int main(void) {
    // Inicijalizacija semafora na 0 i barijere na 3 niti
    sem_init(&sem_h, 0, 0);
    sem_init(&sem_o, 0, 0);
    pthread_barrier_init(&barrier, NULL, 3);

    pthread_t h_threads[NUM_H];
    pthread_t o_threads[NUM_O];

    for (int i = 0; i < NUM_O; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&o_threads[i], NULL, o_thread, id);
    }

    for (int i = 0; i < NUM_H; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&h_threads[i], NULL, h_thread, id);
    }

    for (int i = 0; i < NUM_O; i++) pthread_join(o_threads[i], NULL);
    for (int i = 0; i < NUM_H; i++) pthread_join(h_threads[i], NULL);

    sem_destroy(&sem_h);
    sem_destroy(&sem_o);
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);

    printf("\nSimulacija uspesno zavrsena. Formirano je %d molekula vode!\n", NUM_MOLECULES);
    return 0;
}