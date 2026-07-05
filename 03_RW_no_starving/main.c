#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define READER_COUNT 5
#define WRITER_COUNT 2
#define READER_LOOPS 3
#define WRITER_LOOPS 3

/* * Sinhronizacija:
 * - mutex:   štiti read_count
 * - rw_lock: štiti kritičnu sekciju (bilo jedan writer ili više readera)
 * - red:     sprečava writer starvation - kad writer stigne, zaključa "red" 
 * i novi čitaoci ne mogu da ga preskoče.
 */
int read_count = 0;
sem_t mutex;    /* Štiti read_count */
sem_t rw_lock;  /* Štiti pristup resursu */
sem_t red;      /* Red čekanja - sprečava starvation */

void *reader(void *arg) {
    int id = *(int *)arg;

    for (int i = 0; i < READER_LOOPS; i++) {
        /* Ulazak u red */
        sem_wait(&red);
        sem_post(&red);

        sem_wait(&mutex);
        read_count++;
        if (read_count == 1) {
            sem_wait(&rw_lock); /* Prvi čitalac zaključava bazu za pisce */
        }
        sem_post(&mutex);

        /* --- KRITIČNA SEKCIJA ZA ČITANJE --- */
        printf("[Reader %d] poceo citanje (Trenutno citaoca: %d)\n", id, read_count);
        fflush(stdout);
        
        usleep((50 + rand() % 100) * 1000); /* Simulacija čitanja */
        
        printf("[Reader %d] zavrsio citanje\n", id);
        fflush(stdout);
        /* ----------------------------------- */

        sem_wait(&mutex);
        read_count--;
        if (read_count == 0) {
            sem_post(&rw_lock); /* Poslednji čitalac otključava bazu za pisce */
        }
        sem_post(&mutex);

        usleep((100 + rand() % 300) * 1000);
    }
    return NULL;
}

void *writer(void *arg) {
    int id = *(int *)arg;

    for (int i = 0; i < WRITER_LOOPS; i++) {
        /* Pisac zaključava red kako bi sprečio nove čitaoce da ga preskoče */
        sem_wait(&red);
        sem_wait(&rw_lock); 
        sem_post(&red);    

        /* --- KRITIČNA SEKCIJA ZA PISANJE --- */
        printf("[Writer %d] počeo pisanje i dodavanje podataka...\n", id);
        fflush(stdout);
        
        usleep((80 + rand() % 150) * 1000); /* Simulacija pisanja */
        
        printf("[Writer %d] završio pisanje.\n", id);
        fflush(stdout);
        /* ----------------------------------- */

        sem_post(&rw_lock); 

        usleep((150 + rand() % 350) * 1000);
    }
    return NULL;
}

int main(void) {
    srand((unsigned)time(NULL));

    /* Inicijalizacija semafora na vrednost 1 */
    sem_init(&mutex,   0, 1);
    sem_init(&rw_lock, 0, 1);
    sem_init(&red,     0, 1);

    pthread_t readers[READER_COUNT];
    pthread_t writers[WRITER_COUNT];
    int reader_ids[READER_COUNT];
    int writer_ids[WRITER_COUNT];

    /* Kreiranje niti */
    for (int i = 0; i < READER_COUNT; i++) {
        reader_ids[i] = i + 1;
        pthread_create(&readers[i], NULL, reader, &reader_ids[i]);
    }
    for (int i = 0; i < WRITER_COUNT; i++) {
        writer_ids[i] = i + 1;
        pthread_create(&writers[i], NULL, writer, &writer_ids[i]);
    }

    /* Čekanje da se sve niti završe */
    for (int i = 0; i < READER_COUNT; i++) pthread_join(readers[i], NULL);
    for (int i = 0; i < WRITER_COUNT; i++) pthread_join(writers[i], NULL);

    /* Uništavanje semafora */
    sem_destroy(&mutex);
    sem_destroy(&rw_lock);
    sem_destroy(&red);

    printf("\n[USPEH] Svi readeri i writeri su uspesno zavrsili rad!\n");
    return 0;
}