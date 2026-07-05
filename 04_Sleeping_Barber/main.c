#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CHAIRS 3
#define TOTAL_CUSTOMERS 8

static int waiting_customers = 0;
static int barber_sleeping = 0;
static int all_done = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_barber = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_customers = PTHREAD_COND_INITIALIZER;

static void *barber_thread(void *arg) {
    (void)arg;

    while (1) {
        pthread_mutex_lock(&mutex);

        while (waiting_customers == 0 && !all_done) {
            printf("Nema musterija. Berberin spava...\n");
            barber_sleeping = 1;
            pthread_cond_wait(&cond_barber, &mutex);
            barber_sleeping = 0;
        }

        if (waiting_customers == 0 && all_done) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        /* Berberin preuzima jednu musteriju iz cekaonice */
        waiting_customers--;
        printf("Berberin poziva musteriju. (U cekaonici ostaje: %d)\n", waiting_customers);
        
        /* Budimo sisanje za musteriju */
        pthread_cond_signal(&cond_customers);
        pthread_mutex_unlock(&mutex);

        /* Simulacija sisanja van kriticne sekcije */
        printf("Berberin sisa musteriju...\n");
        usleep(40000);
        printf("Berberin je zavrsio sisanje.\n");
    }

    printf("Berberin zatvara radnju za danas.\n");
    return NULL;
}

static void *customer_thread(void *arg) {
    int id = *(int *)arg;
    free(arg);

    usleep((rand() % 150) * 1000);

    pthread_mutex_lock(&mutex);
    printf("Musterija %d stize u berbernicu.\n", id);

    if (waiting_customers < CHAIRS) {
        waiting_customers++;
        printf("Musterija %d seda u cekaonicu. (Ukupno ceka: %d)\n", id, waiting_customers);

        if (barber_sleeping) {
            printf("Musterija %d budi berberina!\n", id);
            pthread_cond_signal(&cond_barber);
        }

        /* Musterija ceka da je berberin prozove */
        pthread_cond_wait(&cond_customers, &mutex);
        printf("Musterija %d ulazi na stolicu za sisanje.\n", id);
    } else {
        printf("Cekaonica je puna! Musterija %d odlazi bez sisanja.\n", id);
    }

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main(void) {
    srand(time(NULL));
    pthread_t barber;
    pthread_t customers[TOTAL_CUSTOMERS];

    pthread_create(&barber, NULL, barber_thread, NULL);

    for (int i = 0; i < TOTAL_CUSTOMERS; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&customers[i], NULL, customer_thread, id);
    }

    for (int i = 0; i < TOTAL_CUSTOMERS; i++) {
        pthread_join(customers[i], NULL);
    }

    pthread_mutex_lock(&mutex);
    all_done = 1;
    pthread_cond_signal(&cond_barber);
    pthread_mutex_unlock(&mutex);

    pthread_join(barber, NULL);

    printf("\n[USPEH] Svi klijenti procesuirani, program uspesno zatvoren!\n");
    return 0;
}