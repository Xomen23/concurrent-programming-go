#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define ITERATIONS 20
#define SHELF_CAPACITY 3

static int table_board = 0;
static int table_sensor = 0;
static int table_battery = 0;
static int table_full = 0;

static int shelf_count = 0;
static int active_workers = 3;
static int done = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_manager = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_controller = PTHREAD_COND_INITIALIZER;

static pthread_cond_t cond_w_board = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_w_sensor = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_w_battery = PTHREAD_COND_INITIALIZER;

static const char* shelf[SHELF_CAPACITY];
static int shelf_head = 0;
static int shelf_tail = 0;

static void random_sleep_ms(int min_ms, int max_ms) {
    int delay = min_ms + rand() % (max_ms - min_ms + 1);
    usleep(delay * 1000);
}

static void *manager_thread(void *arg) {
    (void)arg;

    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&mutex);

        while (table_full) {
            pthread_cond_wait(&cond_manager, &mutex);
        }

        int choice = rand() % 3;
        if (choice == 0) {
            table_board = 1; table_sensor = 1;
            printf("Rukovodilac stavlja: plocica + senzor\n");
        } else if (choice == 1) {
            table_sensor = 1; table_battery = 1;
            printf("Rukovodilac stavlja: senzor + baterija\n");
        } else {
            table_board = 1; table_battery = 1;
            printf("Rukovodilac stavlja: plocica + baterija\n");
        }
        table_full = 1;
        fflush(stdout);

        /* Logika posrednika u kriticnoj sekciji bira koga budi */
        if (table_board && table_sensor) pthread_cond_signal(&cond_w_battery);
        else if (table_sensor && table_battery) pthread_cond_signal(&cond_w_board);
        else if (table_board && table_battery) pthread_cond_signal(&cond_w_sensor);

        pthread_mutex_unlock(&mutex);
        random_sleep_ms(40, 100);
    }

    pthread_mutex_lock(&mutex);
    done = 1;
    pthread_cond_broadcast(&cond_w_board);
    pthread_cond_broadcast(&cond_w_sensor);
    pthread_cond_broadcast(&cond_w_battery);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

static void *worker_thread(void *arg) {
    int type = *(int *)arg;
    free(arg);

    while (1) {
        pthread_mutex_lock(&mutex);

        if (type == 0) {
            while ((!table_sensor || !table_battery) && !done) pthread_cond_wait(&cond_w_board, &mutex);
        } else if (type == 1) {
            while ((!table_board || !table_battery) && !done) pthread_cond_wait(&cond_w_sensor, &mutex);
        } else {
            while ((!table_board || !table_sensor) && !done) pthread_cond_wait(&cond_w_battery, &mutex);
        }

        if (done && !table_full) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        table_board = 0; table_sensor = 0; table_battery = 0; table_full = 0;

        const char* name = (type == 0) ? "Radnik sa plocicom" : 
                            (type == 1) ? "Radnik sa senzorom" : "Radnik sa baterijom";

        printf("%s sklapa proizvod.\n", name);
        fflush(stdout);

        pthread_cond_signal(&cond_manager);

        while (shelf_count >= SHELF_CAPACITY) {
            pthread_cond_wait(&cond_manager, &mutex);
        }

        shelf[shelf_tail] = name;
        shelf_tail = (shelf_tail + 1) % SHELF_CAPACITY;
        shelf_count++;

        printf("%s stavlja proizvod na policu.\n", name);
        fflush(stdout);

        pthread_cond_signal(&cond_controller);
        pthread_mutex_unlock(&mutex);
        random_sleep_ms(30, 80);
    }

    pthread_mutex_lock(&mutex);
    active_workers--;
    pthread_cond_signal(&cond_controller);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

static void *controller_thread(void *arg) {
    (void)arg;
    FILE *f = fopen("proizvodi.txt", "w");
    if (!f) {
        printf("[INFO] Bezbednosna blokada za fopen, radim simulaciju u memoriji.\n");
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

        const char* maker = shelf[shelf_head];
        shelf_head = (shelf_head + 1) % SHELF_CAPACITY;
        shelf_count--;

        printf("Kontrolor kvaliteta proverava proizvod %s.\n", maker);
        fflush(stdout);

        if (f) {
            fprintf(f, "Proveren proizvod radnika: %s\n", maker);
            fflush(f);
        }

        pthread_cond_broadcast(&cond_manager);
        pthread_mutex_unlock(&mutex);
        random_sleep_ms(20, 60);
    }

    if (f) fclose(f);
    return NULL;
}

int main(void) {
    srand((unsigned)time(NULL));
    pthread_t manager, controller;
    pthread_t workers[3];

    pthread_create(&manager, NULL, manager_thread, NULL);
    pthread_create(&controller, NULL, controller_thread, NULL);

    for (int i = 0; i < 3; i++) {
        int *t = malloc(sizeof(int));
        *t = i;
        pthread_create(&workers[i], NULL, worker_thread, t);
    }

    pthread_join(manager, NULL);
    for (int i = 0; i < 3; i++) pthread_join(workers[i], NULL);

    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond_controller);
    pthread_mutex_unlock(&mutex);

    pthread_join(controller, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_manager);
    pthread_cond_destroy(&cond_controller);
    pthread_cond_destroy(&cond_w_board);
    pthread_cond_destroy(&cond_w_sensor);
    pthread_cond_destroy(&cond_w_battery);
    
    printf("\nLaboratorija je uspesno zatvorena nakon svih iteracija.\n");

    return 0;
}