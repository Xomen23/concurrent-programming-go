#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define TOTAL_PASSENGERS 12
#define BOAT_CAPACITY 4

static int passengers_in_line = 0;
static int passengers_on_boat = 0;
static int boat_is_rowing = 0;

/* Sinhronizacija */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_passenger = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_boat_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_disembark = PTHREAD_COND_INITIALIZER;

static void random_sleep_ms(int min_ms, int max_ms) {
    int span = max_ms - min_ms + 1;
    int delay = min_ms + rand() % span;
    usleep((useconds_t)delay * 1000);
}

static void *passenger_thread(void *arg) {
    int id = *(int *)arg;
    free(arg);

    random_sleep_ms(10, 300);

    pthread_mutex_lock(&mutex);
    passengers_in_line++;
    printf("Putnik %02d je stigao na obalu. (U redu: %d)\n", id, passengers_in_line);
    fflush(stdout);

    /* Ceka se ako je camac trenutno na reci (plovi) */
    while (boat_is_rowing) {
        pthread_cond_wait(&cond_passenger, &mutex);
    }

    /* Ukrcavanje u camac */
    passengers_in_line--;
    passengers_on_boat++;
    printf("Putnik %02d se ukrcao u camac. (Na camcu: %d/%d)\n", id, passengers_on_boat, BOAT_CAPACITY);
    fflush(stdout);

    if (passengers_on_boat == BOAT_CAPACITY) {
        /* Poslednji putnik signalizira da je camac pun i vozi */
        boat_is_rowing = 1;
        printf(">>> Camac je pun! Putnik %02d pokrece camac i vozi preko reke... <<<\n", id);
        fflush(stdout);
        
        pthread_mutex_unlock(&mutex);
        random_sleep_ms(150, 250); /* Simulacija plovidbe */
        pthread_mutex_lock(&mutex);

        printf(">>> Camac je stigao na drugu obalu. <<<\n");
        fflush(stdout);
        
        /* Budi ostale putnike u camcu da mogu da izadju */
        pthread_cond_broadcast(&cond_disembark);
    } else {
        /* Putnici koji su se ukrcali cekaju dok camac ne stigne na drugu obalu */
        while (!boat_is_rowing || passengers_on_boat == BOAT_CAPACITY) {
            pthread_cond_wait(&cond_disembark, &mutex);
            if (boat_is_rowing) break; 
        }
    }

    /* Putnik izlazi iz camca na drugoj obali */
    passengers_on_boat--;
    printf("Putnik %02d je izasao iz camca.\n", id);
    fflush(stdout);

    /* Kada i poslednji putnik izadje na drugoj strani, resetuje se camac */
    if (passengers_on_boat == 0) {
        boat_is_rowing = 0;
        printf(">>> Camac je prazan i vraca se nazad. <<<\n\n");
        fflush(stdout);
        /* Budi ljude na obali koji su cekali novu turu */
        pthread_cond_broadcast(&cond_passenger);
    }

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main(void) {
    srand((unsigned)time(NULL));

    pthread_t threads[TOTAL_PASSENGERS];

    for (int i = 0; i < TOTAL_PASSENGERS; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&threads[i], NULL, passenger_thread, id) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < TOTAL_PASSENGERS; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Unistavanje resursa */
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_passenger);
    pthread_cond_destroy(&cond_boat_full);
    pthread_cond_destroy(&cond_disembark);

    printf("Simulacija uspesno zavrsena. Svi putnici su presli reku.\n");
    return EXIT_SUCCESS;
}