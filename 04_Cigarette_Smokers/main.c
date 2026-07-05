#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define ITERATIONS 6

/* Stanje stola: 1 ako je resurs tu, 0 ako nije */
static int table_tobacco = 0;
static int table_paper = 0;
static int table_matches = 0;

static int table_busy = 0;     /* Da li ima neceg na stolu */
static int agent_ready = 1;    /* Da li agent sme da stavlja */
static int done = 0;           /* Zastavica za kraj programa */

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_agent = PTHREAD_COND_INITIALIZER;

/* Uslovne promenljive za svakog pusaca pojedinacno */
static pthread_cond_t cond_smoker_tobacco = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_smoker_paper = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_smoker_matches = PTHREAD_COND_INITIALIZER;

static void *agent_thread(void *arg) {
    (void)arg;

    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&mutex);
        
        while (!agent_ready) {
            pthread_cond_wait(&cond_agent, &mutex);
        }

        int r = rand() % 3;
        if (r == 0) {
            table_paper = 1; 
            table_matches = 1;
            printf("Agent stavlja na sto: PAPIR + SIBICE\n");
        } else if (r == 1) {
            table_tobacco = 1; 
            table_matches = 1;
            printf("Agent stavlja na sto: DUVAN + SIBICE\n");
        } else {
            table_tobacco = 1; 
            table_paper = 1;
            printf("Agent stavlja na sto: DUVAN + PAPIR\n");
        }

        table_busy = 1;
        agent_ready = 0;
        fflush(stdout); /* Odmah guramo ispis u konzolu */

        /* Posrednicka logika: Agent salje signal, ali pusaci proveravaju i logicke promenljive */
        if (table_paper && table_matches) pthread_cond_signal(&cond_smoker_tobacco);
        else if (table_tobacco && table_matches) pthread_cond_signal(&cond_smoker_paper);
        else if (table_tobacco && table_paper) pthread_cond_signal(&cond_smoker_matches);

        pthread_mutex_unlock(&mutex);
        usleep(50000); /* Pauza izmedju rundi */
    }

    /* Kraj rada: Obavestavamo sve pusace da se gase */
    pthread_mutex_lock(&mutex);
    done = 1;
    pthread_cond_broadcast(&cond_smoker_tobacco);
    pthread_cond_broadcast(&cond_smoker_paper);
    pthread_cond_broadcast(&cond_smoker_matches);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

static void *smoker_thread(void *arg) {
    int type = *(int *)arg;
    free(arg);

    while (1) {
        pthread_mutex_lock(&mutex);

        /* Kljuco resenje: Dokle god na stolu nema TAČNO onoga sto pusacu treba, on spava.
           Ako su resursi vec tu, uopste nece uci u while i program nece zabagovati! */
        if (type == 0) { // Ima duvan, ceka papir i sibice
            while ((!table_paper || !table_matches) && !done) {
                pthread_cond_wait(&cond_smoker_tobacco, &mutex);
            }
        } else if (type == 1) { // Ima papir, ceka duvan i sibice
            while ((!table_tobacco || !table_matches) && !done) {
                pthread_cond_wait(&cond_smoker_paper, &mutex);
            }
        } else { // Ima sibice, ceka duvan i papir
            while ((!table_tobacco || !table_paper) && !done) {
                pthread_cond_wait(&cond_smoker_matches, &mutex);
            }
        }

        if (done && !table_busy) {
            pthread_mutex_unlock(&mutex);
            break; 
        }

        const char *name = (type == 0) ? "Pusac sa duvanom" : (type == 1) ? "Pusac sa papirom" : "Pusac sa sibicama";
        printf("%s prepoznaje sastojke, uzima ih i pravi cigaretu.\n", name);
        fflush(stdout);
        
        /* Sklanjamo sa stola */
        table_tobacco = 0; 
        table_paper = 0; 
        table_matches = 0;
        table_busy = 0;

        pthread_mutex_unlock(&mutex);
        
        /* Simulacija pusenja van kriticne sekcije */
        printf("%s pusi...\n", name);
        fflush(stdout);
        usleep(50000);

        pthread_mutex_lock(&mutex);
        printf("%s je zavrsio i signalizira agentu.\n", name);
        fflush(stdout);
        
        agent_ready = 1;
        pthread_cond_signal(&cond_agent);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(void) {
    srand((unsigned)time(NULL));
    pthread_t agent;
    pthread_t smokers[3];

    /* Kreiramo pusace */
    for (int i = 0; i < 3; i++) {
        int *type = malloc(sizeof(int));
        *type = i;
        pthread_create(&smokers[i], NULL, smoker_thread, type);
    }

    /* Kreiramo agenta */
    pthread_create(&agent, NULL, agent_thread, NULL);

    /* Cekamo kraj niti */
    pthread_join(agent, NULL);

    for (int i = 0; i < 3; i++) {
        pthread_join(smokers[i], NULL);
    }

    /* Unistavanje resursa */
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_agent);
    pthread_cond_destroy(&cond_smoker_tobacco);
    pthread_cond_destroy(&cond_smoker_paper);
    pthread_cond_destroy(&cond_smoker_matches);

    printf("\n[USPEH] Svi sastojci potroseni, sesija zavrsena!\n");
    fflush(stdout);
    return 0;
}