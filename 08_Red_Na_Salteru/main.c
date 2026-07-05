#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define STUDENT_COUNT 10
#define WAITING_ROOM_SIZE 3

/* FIFO Red čekanja */
static int queue[WAITING_ROOM_SIZE];
static int head = 0;
static int tail = 0;
static int count = 0;

/* Sinhronizacija */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_clerk = PTHREAD_COND_INITIALIZER;
/* Niz uslovnih promenljivih - svaki student čeka na svojoj ličnoj promenljivoj */
static pthread_cond_t cond_students[STUDENT_COUNT + 1];

static int current_student_at_desk = 0;
static int clerk_called = 0;
static int request_processed = 0;
static int active_students = STUDENT_COUNT;

static void random_sleep_ms(int min_ms, int max_ms) {
    int span = max_ms - min_ms + 1;
    int delay = min_ms + rand() % span;
    usleep((useconds_t)delay * 1000);
}

static void *clerk_thread(void *arg) {
    (void)arg;

    pthread_mutex_lock(&mutex);
    while (active_students > 0) {
        /* Službenik spava ako je čekaonica prazna */
        while (count == 0 && active_students > 0) {
            pthread_cond_wait(&cond_clerk, &mutex);
        }

        if (active_students <= 0 && count == 0) {
            break;
        }

        /* Uzima prvog studenta iz FIFO reda */
        int student_id = queue[head];
        head = (head + 1) % WAITING_ROOM_SIZE;
        count--;

        current_student_at_desk = student_id;
        clerk_called = 1;
        request_processed = 0;

        printf("Sluzbenik proziva studenta %d.\n", student_id);
        fflush(stdout);

        /* Budi tačno tog prozvanog studenta */
        pthread_cond_signal(&cond_students[student_id]);

        /* Čeka da student priđe šalteru i da mu se obradi zahtev */
        while (!request_processed) {
            pthread_mutex_unlock(&mutex);
            random_sleep_ms(50, 150); /* Obrada zahteva */
            pthread_mutex_lock(&mutex);
            
            if (clerk_called == 0) { 
                /* Student je prišao i obrada je simulirana */
                request_processed = 1;
                printf("Sluzbenik zavrsava obradu zahteva studenta %d.\n", student_id);
                fflush(stdout);
                pthread_cond_signal(&cond_students[student_id]);
            }
        }
    }
    pthread_mutex_unlock(&mutex);

    printf("Sluzbenik zatvara salter.\n");
    fflush(stdout);
    return NULL;
}

static void *student_thread(void *arg) {
    int id = *(int *)arg;
    free(arg);

    random_sleep_ms(10, 200);

    pthread_mutex_lock(&mutex);
    printf("Student %d dolazi ispred studentske sluzbe.\n", id);
    fflush(stdout);

    if (count < WAITING_ROOM_SIZE) {
        /* Ima mesta, seda u čekaonicu i ulazi u FIFO niz */
        queue[tail] = id;
        tail = (tail + 1) % WAITING_ROOM_SIZE;
        count++;

        printf("Student %d seda u cekaonicu.\n", id);
        fflush(stdout);

        /* Obaveštava službenika da je neko ušao */
        pthread_cond_signal(&cond_clerk);

        /* Čeka dok ga službenik ne prozove */
        while (current_student_at_desk != id || !clerk_called) {
            pthread_cond_wait(&cond_students[id], &mutex);
        }

        printf("Student %d prilazi salteru.\n", id);
        printf("Sluzbenik obradjuje zahtev studenta %d.\n", id);
        fflush(stdout);

        clerk_called = 0; /* Signal službeniku da je student seo za šalter */

        /* Čeka znak od službenika da je sve gotovo */
        pthread_cond_wait(&cond_students[id], &mutex);
        active_students--;
    } else {
        /* Nema mesta u čekaonici, student odlazi */
        printf("Nema mesta u cekaonici. Student %d odlazi.\n", id);
        fflush(stdout);
        active_students--;
        /* Budi službenika za slučaj da je ovo bio poslednji student a službenik čeka praznu čekaonicu */
        pthread_cond_signal(&cond_clerk); 
    }

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main(void) {
    srand((unsigned)time(NULL));

    pthread_t clerk;
    pthread_t students[STUDENT_COUNT];

    /* Inicijalizacija niza uslovnih promenljivih za studente */
    for (int i = 1; i <= STUDENT_COUNT; i++) {
        pthread_cond_init(&cond_students[i], NULL);
    }

    if (pthread_create(&clerk, NULL, clerk_thread, NULL) != 0) {
        perror("pthread_create clerk");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < STUDENT_COUNT; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&students[i], NULL, student_thread, id) != 0) {
            perror("pthread_create student");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < STUDENT_COUNT; i++) {
        pthread_join(students[i], NULL);
    }

    /* Budimo službenika na kraju da proveri active_students i bezbedno završi nit */
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond_clerk);
    pthread_mutex_unlock(&mutex);

    pthread_join(clerk, NULL);

    /* Uništenje resursa */
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_clerk);
    for (int i = 1; i <= STUDENT_COUNT; i++) {
        pthread_cond_destroy(&cond_students[i]);
    }

    printf("\nSimulacija uspesno zavrsena.\n");
    return EXIT_SUCCESS;
}