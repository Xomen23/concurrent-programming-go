#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define STUDENT_COUNT 10
#define WAITING_ROOM_SIZE 3

/* FIFO Niz za pracenje ID-eva studenata u cekaonici */
static int waiting_room[WAITING_ROOM_SIZE];
static int head = 0;
static int tail = 0;
static int count = 0;

/* Sinhronizacija rada */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_assistant = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_students[STUDENT_COUNT + 1];

static int current_student_in_office = 0;
static int assistant_called = 0;
static int session_finished = 0;
static int remaining_students = STUDENT_COUNT;

static void random_sleep_ms(int min_ms, int max_ms) {
    int delay = min_ms + rand() % (max_ms - min_ms + 1);
    usleep(delay * 1000);
}

static void *assistant_thread(void *arg) {
    (void)arg;

    pthread_mutex_lock(&mutex);
    while (remaining_students > 0) {
        /* Asistent spava ako nema nikoga ispred kabineta */
        while (count == 0 && remaining_students > 0) {
            pthread_cond_wait(&cond_assistant, &mutex);
        }

        if (remaining_students <= 0 && count == 0) break;

        /* FIFO izbacivanje studenta iz reda */
        int student_id = waiting_room[head];
        head = (head + 1) % WAITING_ROOM_SIZE;
        count--;

        current_student_in_office = student_id;
        assistant_called = 1;
        session_finished = 0;

        printf("Asistent proziva studenta %d.\n", student_id);
        fflush(stdout);

        /* Budi se tacno odredjeni student */
        pthread_cond_signal(&cond_students[student_id]);

        /* Asistent ceka da student udje i da obavi konsultacije */
        while (!session_finished) {
            pthread_mutex_unlock(&mutex);
            random_sleep_ms(60, 120); /* Konsultacije */
            pthread_mutex_lock(&mutex);

            if (assistant_called == 0) {
                session_finished = 1;
                printf("Asistent zavrsava konsultacije sa studentom %d.\n", student_id);
                fflush(stdout);
                pthread_cond_signal(&cond_students[student_id]);
            }
        }
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

static void *student_thread(void *arg) {
    int id = *(int *)arg;
    free(arg);

    random_sleep_ms(10, 200);

    pthread_mutex_lock(&mutex);
    printf("Student %d dolazi na konsultacije.\n", id);
    fflush(stdout);

    if (count < WAITING_ROOM_SIZE) {
        /* Ima mesta, student seda u cekaonicu */
        waiting_room[tail] = id;
        tail = (tail + 1) % WAITING_ROOM_SIZE;
        count++;

        printf("Student %d seda u cekaonicu.\n", id);
        fflush(stdout);

        pthread_cond_signal(&cond_assistant);

        /* Ceka dok ga asistent ne prozove po imenu */
        while (current_student_in_office != id || !assistant_called) {
            pthread_cond_wait(&cond_students[id], &mutex);
        }

        printf("Student %d ulazi na konsultacije.\n", id);
        fflush(stdout);

        assistant_called = 0; /* Javlja asistentu da je seo */

        /* Ceka signal da su konsultacije gotove */
        pthread_cond_wait(&cond_students[id], &mutex);
        remaining_students--;
    } else {
        printf("Nema mesta u cekaonici. Student %d odlazi.\n", id);
        fflush(stdout);
        remaining_students--;
        pthread_cond_signal(&cond_assistant);
    }

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main(void) {
    srand((unsigned)time(NULL));
    pthread_t assistant;
    pthread_t students[STUDENT_COUNT];

    for (int i = 1; i <= STUDENT_COUNT; i++) {
        pthread_cond_init(&cond_students[i], NULL);
    }

    pthread_create(&assistant, NULL, assistant_thread, NULL);

    for (int i = 0; i < STUDENT_COUNT; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&students[i], NULL, student_thread, id);
    }

    for (int i = 0; i < STUDENT_COUNT; i++) {
        pthread_join(students[i], NULL);
    }

    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond_assistant);
    pthread_mutex_unlock(&mutex);

    pthread_join(assistant, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_assistant);
    for (int i = 1; i <= STUDENT_COUNT; i++) {
        pthread_cond_destroy(&cond_students[i]);
    }
    printf("\nKonsultacije su zavrsene. Asistent zakljucava kabinet.\n");
    return 0;
}