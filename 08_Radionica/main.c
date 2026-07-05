#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define ITERATIONS 20
#define SHELF_CAPACITY 3

/* Stanje zajednickog stola */
static int table_has_board = 0;
static int table_has_sensor = 0;
static int table_has_battery = 0;
static int table_busy = 0;

/* Stanje police i niti */
static int shelf_count = 0;
static int active_workers = 3; 
static int done = 0;

/* Sinhronizacija */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_manager = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_controller = PTHREAD_COND_INITIALIZER;

/* Posebne uslovne promenljive za svakog radnika */
static pthread_cond_t cond_worker_board = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_worker_sensor = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_worker_battery = PTHREAD_COND_INITIALIZER;

/* Polica za kontrolora */
static const char* shelf[SHELF_CAPACITY];
static int shelf_head = 0;
static int shelf_tail = 0;

static void random_sleep_ms(int min_ms, int max_ms) {
    int span = max_ms - min_ms + 1;
    int delay = min_ms + rand() % span;
    usleep((useconds_t)delay * 1000);
}

static void *manager_thread(void *arg) {
    (void)arg;

    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&mutex);

        while (table_busy) {
            pthread_cond_wait(&cond_manager, &mutex);
        }

        int kombinacija = rand() % 3;
        if (kombinacija == 0) {
            table_has_board = 1;
            table_has_sensor = 1;
            printf("Rukovodilac stavlja: plocica + senzor\n");
        } else if (kombinacija == 1) {
            table_has_sensor = 1;
            table_has_battery = 1;
            printf("Rukovodilac stavlja: senzor + baterija\n");
        } else {
            table_has_board = 1;
            table_has_battery = 1;
            printf("Rukovodilac stavlja: plocica + baterija\n");
        }
        table_busy = 1;
        fflush(stdout);

        if (table_has_board && table_has_sensor) {
            pthread_cond_signal(&cond_worker_battery);
        } else if (table_has_sensor && table_has_battery) {
            pthread_cond_signal(&cond_worker_board);
        } else if (table_has_board && table_has_battery) {
            pthread_cond_signal(&cond_worker_sensor);
        }

        pthread_mutex_unlock(&mutex);
        random_sleep_ms(50, 150);
    }

    pthread_mutex_lock(&mutex);
    done = 1;
    pthread_cond_broadcast(&cond_worker_board);
    pthread_cond_broadcast(&cond_worker_sensor);
    pthread_cond_broadcast(&cond_worker_battery);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

static void *worker_thread(void *arg) {
    int tip_resursa = *(int *)arg;
    free(arg);

    while (1) {
        pthread_mutex_lock(&mutex);

        if (tip_resursa == 0) { 
            while ((!table_has_sensor || !table_has_battery) && !done) 
                pthread_cond_wait(&cond_worker_board, &mutex);
        } else if (tip_resursa == 1) { 
            while ((!table_has_board || !table_has_battery) && !done) 
                pthread_cond_wait(&cond_worker_sensor, &mutex);
        } else { 
            while ((!table_has_board || !table_has_sensor) && !done) 
                pthread_cond_wait(&cond_worker_battery, &mutex);
        }

        if (done && !table_busy) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        table_has_board = 0;
        table_has_sensor = 0;
        table_has_battery = 0;
        table_busy = 0;

        const char* ime_radnika = (tip_resursa == 0) ? "Radnik sa plocicom" : 
                                  (tip_resursa == 1) ? "Radnik sa senzorom" : "Radnik sa baterijom";

        printf("%s sklapa uredjaj.\n", ime_radnika);
        fflush(stdout);

        pthread_cond_signal(&cond_manager);

        while (shelf_count >= SHELF_CAPACITY) {
            pthread_cond_wait(&cond_manager, &mutex); 
        }

        shelf[shelf_tail] = ime_radnika;
        shelf_tail = (shelf_tail + 1) % SHELF_CAPACITY;
        shelf_count++;

        printf("%s stavlja uredjaj na policu.\n", ime_radnika);
        fflush(stdout);

        pthread_cond_signal(&cond_controller);

        pthread_mutex_unlock(&mutex);
        random_sleep_ms(30, 100); 
    }

    pthread_mutex_lock(&mutex);
    active_workers--;
    pthread_cond_signal(&cond_controller); 
    pthread_mutex_unlock(&mutex);

    return NULL;
}

static void *controller_thread(void *arg) {
    (void)arg;
    
    /* Zastita: Pokusavamo da otvorimo fajl, ako ne moze, ispisujemo obavestenje ali NE KILUJEMO nit */
    FILE *f = fopen("proizvodi.txt", "w");
    if (!f) {
        printf("[INFO] fopen: Permission denied (Sajt blokira fajlove, radimo u memoriji)\n");
        fflush(stdout);
    }

    while (1) {
        pthread_mutex_lock(&mutex);

        while (shelf_count == 0 && active_workers > 0) {
            pthread_cond_wait(&cond_controller, &mutex);
        }

        if (shelf_count == 0 && active_workers == 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        const char* tvorac = shelf[shelf_head];
        shelf_head = (shelf_head + 1) % SHELF_CAPACITY;
        shelf_count--;

        printf("Kontrolor proverava uredjaj %s.\n", tvorac);
        fflush(stdout);

        /* Pisi u fajl samo ako je uspesno otvoren */
        if (f) {
            fprintf(f, "Proveren uredjaj koji je napravio: %s\n", tvorac);
            fflush(f);
        }

        pthread_cond_broadcast(&cond_manager);

        pthread_mutex_unlock(&mutex);
        random_sleep_ms(20, 80);
    }

    if (f) {
        fclose(f);
    }
    printf("Kontrolor zavrsava rad.\n");
    fflush(stdout);
    return NULL;
}

int main(void) {
    srand((unsigned)time(NULL));

    pthread_t manager, controller;
    pthread_t workers[3];

    pthread_create(&manager, NULL, manager_thread, NULL);
    pthread_create(&controller, NULL, controller_thread, NULL);

    for (int i = 0; i < 3; i++) {
        int *tip = malloc(sizeof(int));
        *tip = i;
        pthread_create(&workers[i], NULL, worker_thread, tip);
    }

    pthread_join(manager, NULL);
    
    for (int i = 0; i < 3; i++) {
        pthread_join(workers[i], NULL);
    }

    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond_controller);
    pthread_mutex_unlock(&mutex);

    pthread_join(controller, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_manager);
    pthread_cond_destroy(&cond_controller);
    pthread_cond_destroy(&cond_worker_board);
    pthread_cond_destroy(&cond_worker_sensor);
    pthread_cond_destroy(&cond_worker_battery);

    printf("\nRadionica je uspesno zatvorena.\n");
    return EXIT_SUCCESS;
}