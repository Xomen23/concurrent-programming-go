#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#define NUM_CARS 24
#define MAX_ON_BRIDGE 3
#define NORTH 0
#define SOUTH 1

static const char *dir_name[] = {"SEVER", "JUG"};

typedef struct {
    int id;
    int dir;
} car_arg_t;

static int cars_on_bridge = 0;
static int current_dir = -1;          
static int violation_detected = 0;

/* SINHRONIZACIJA */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_north = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_south = PTHREAD_COND_INITIALIZER;

static void random_sleep_ms(int min_ms, int max_ms) {
    int span = max_ms - min_ms + 1;
    int delay = min_ms + rand() % span;
    usleep((useconds_t)delay * 1000);
}

static void check_bridge_state(void) {
    if (cars_on_bridge < 0 || cars_on_bridge > MAX_ON_BRIDGE) {
        fprintf(stderr, "GRESKA: broj automobila na mostu = %d\n", cars_on_bridge);
        violation_detected = 1;
    }
    if (cars_on_bridge == 0 && current_dir != -1) {
        fprintf(stderr, "GRESKA: most je prazan, ali current_dir nije resetovan\n");
        violation_detected = 1;
    }
}

static void bridge_enter(int id, int dir) {
    pthread_mutex_lock(&mutex);

    /* Ceka se ako je na mostu suprotan smer ILI ako je most pun */
    while ((current_dir != -1 && current_dir != dir) || cars_on_bridge >= MAX_ON_BRIDGE) {
        if (dir == NORTH) {
            pthread_cond_wait(&cond_north, &mutex);
        } else {
            pthread_cond_wait(&cond_south, &mutex);
        }
    }

    if (cars_on_bridge == 0) {
        current_dir = dir;
    }

    cars_on_bridge++;

    if (current_dir != dir || cars_on_bridge > MAX_ON_BRIDGE) {
        fprintf(stderr,
                "GRESKA: auto %02d iz smera %s je neispravno usao na most "
                "[current_dir=%d, cars_on_bridge=%d]\n",
                id, dir_name[dir], current_dir, cars_on_bridge);
        violation_detected = 1;
    }

    printf("AUTO %02d (%s) ULAZI  | na mostu: %d\n",
           id, dir_name[dir], cars_on_bridge);
    fflush(stdout);

    pthread_mutex_unlock(&mutex);
}

static void bridge_leave(int id, int dir) {
    pthread_mutex_lock(&mutex);

    cars_on_bridge--;
    if (cars_on_bridge == 0) {
        current_dir = -1;
        /* Most je prazan, budimo oba smera da vidimo ko ce pre doci */
        pthread_cond_broadcast(&cond_north);
        pthread_cond_broadcast(&cond_south);
    } else {
        /* Oslobodilo se mesto za auto iz ISTOG smera */
        if (dir == NORTH) {
            pthread_cond_signal(&cond_north);
        } else {
            pthread_cond_signal(&cond_south);
        }
    }

    printf("AUTO %02d (%s) IZLAZI | na mostu: %d\n",
           id, dir_name[dir], cars_on_bridge);
    fflush(stdout);

    check_bridge_state();

    pthread_mutex_unlock(&mutex);
}

static void *car_thread(void *arg) {
    car_arg_t *car = (car_arg_t *)arg;

    random_sleep_ms(10, 120);
    printf("AUTO %02d (%s) CEKA\n", car->id, dir_name[car->dir]);
    fflush(stdout);

    bridge_enter(car->id, car->dir);
    random_sleep_ms(80, 220);
    bridge_leave(car->id, car->dir);

    return NULL;
}

int main(void) {
    srand((unsigned)time(NULL));

    pthread_t threads[NUM_CARS];
    car_arg_t args[NUM_CARS];

    for (int i = 0; i < NUM_CARS; i++) {
        args[i].id = i + 1;
        args[i].dir = rand() % 2;

        if (pthread_create(&threads[i], NULL, car_thread, &args[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_CARS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    /* Unistavanje resursa na kraju simulacije */
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_north);
    pthread_cond_destroy(&cond_south);

    if (violation_detected) {
        printf("\nSimulacija zavrsena, ali su detektovane greske u sinhronizaciji.\n");
        return EXIT_FAILURE;
    }

    printf("\nSimulacija zavrsena bez detektovanih gresaka.\n");
    return EXIT_SUCCESS;
}