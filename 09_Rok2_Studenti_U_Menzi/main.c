#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUM_STUDENTS 8
#define MEALS_PER_STUDENT 4
#define MENU_CAPACITY 5

typedef struct {
    int id;
} student_arg_t;

static int available_meals = 0;
static int meals_left = NUM_STUDENTS * MEALS_PER_STUDENT;
static int violation_detected = 0;

/* SINHRONIZACIJA */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_cook = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_students = PTHREAD_COND_INITIALIZER;

/* Pomocna promenljiva da samo jedan student u datom trenutku naruci dopunu */
static int refill_requested = 0; 

static void random_sleep_ms(int min_ms, int max_ms) {
    int span = max_ms - min_ms + 1;
    int delay = min_ms + rand() % span;
    usleep((useconds_t)delay * 1000);
}

static void check_menu_state(void) {
    if (available_meals < 0 || available_meals > MENU_CAPACITY) {
        fprintf(stderr, "GRESKA: neispravan broj obroka na liniji: %d\n", available_meals);
        violation_detected = 1;
    }
    if (meals_left < 0) {
        fprintf(stderr, "GRESKA: meals_left je negativan: %d\n", meals_left);
        violation_detected = 1;
    }
}

static void cook_refill_menu(void) {
    random_sleep_ms(80, 180);
    available_meals = MENU_CAPACITY;
    refill_requested = 0; /* Resetujemo zahtev nakon dopune */
    printf("KUVAR: izneo novu turu, na liniji je %d obroka\n", available_meals);
    fflush(stdout);
    check_menu_state();
}

static void take_meal(int student_id) {
    pthread_mutex_lock(&mutex);

    /* Ako nema obroka, student ceka dopunu */
    while (available_meals == 0) {
        /* Prvi student koji zatekne praznu liniju budi kuvara */
        if (!refill_requested) {
            refill_requested = 1;
            printf("STUDENT %02d: linija je prazna, treba obavestiti kuvara\n", student_id);
            fflush(stdout);
            pthread_cond_signal(&cond_cook);
        }
        pthread_cond_wait(&cond_students, &mutex);
    }

    available_meals--;
    meals_left--;

    printf("STUDENT %02d: uzima obrok | na liniji: %d | preostalo obroka: %d\n",
           student_id, available_meals, meals_left);
    fflush(stdout);
    check_menu_state();

    pthread_mutex_unlock(&mutex);
}

static void *cook_thread(void *arg) {
    (void)arg;

    pthread_mutex_lock(&mutex);
    while (1) {
        if (meals_left <= 0) {
            break;
        }

        /* Kuvar spava dok nema zahteva za dopunu i dok ima preostalih obroka uopste */
        while (!refill_requested && meals_left > 0) {
            pthread_cond_wait(&cond_cook, &mutex);
        }

        /* Provera uslova za izlazak nakon budjenja */
        if (meals_left <= 0) {
            break;
        }

        cook_refill_menu();
        
        /* Obaveštavamo sve studente koji čekaju u redu da je hrana stigla */
        pthread_cond_broadcast(&cond_students);
    }
    pthread_mutex_unlock(&mutex);

    printf("KUVAR: zavrsava rad\n");
    fflush(stdout);
    return NULL;
}

static void *student_thread(void *arg) {
    student_arg_t *student = (student_arg_t *)arg;

    for (int i = 0; i < MEALS_PER_STUDENT; i++) {
        random_sleep_ms(20, 160);
        take_meal(student->id);
        random_sleep_ms(30, 120);
    }

    printf("STUDENT %02d: zavrsio sve obroke\n", student->id);
    fflush(stdout);
    return NULL;
}

int main(void) {
    srand((unsigned)time(NULL));

    pthread_t cook;
    pthread_t students[NUM_STUDENTS];
    student_arg_t args[NUM_STUDENTS];

    if (pthread_create(&cook, NULL, cook_thread, NULL) != 0) {
        perror("pthread_create cook");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_STUDENTS; i++) {
        args[i].id = i + 1;
        if (pthread_create(&students[i], NULL, student_thread, &args[i]) != 0) {
            perror("pthread_create student");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_STUDENTS; i++) {
        if (pthread_join(students[i], NULL) != 0) {
            perror("pthread_join student");
            exit(EXIT_FAILURE);
        }
    }

    /* Svi studenti su zavrsili. Budimo kuvara da proveri meals_left <= 0 i izadje iz petlje */
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond_cook);
    pthread_mutex_unlock(&mutex);

    if (pthread_join(cook, NULL) != 0) {
        perror("pthread_join cook");
        exit(EXIT_FAILURE);
    }

    /* Unistavanje resursa */
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_cook);
    pthread_cond_destroy(&cond_students);

    if (violation_detected) {
        printf("\nSimulacija zavrsena, ali su detektovane greske u sinhronizaciji.\n");
        return EXIT_FAILURE;
    }

    printf("\nSimulacija zavrsena bez detektovanih gresaka.\n");
    return EXIT_SUCCESS;
}