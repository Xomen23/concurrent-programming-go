#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 4
#define QUEUE_MAX 100

typedef struct node {
    struct node **children;
    int child_count;
    int value;
} node_t;

/* FIFO Red poslova za cvorove */
static node_t *job_queue[QUEUE_MAX];
static int queue_head = 0;
static int queue_tail = 0;
static int queue_count = 0;

/* Brojac niti koje trenutno aktivno obradjuju neki cvor */
static int active_threads = 0;
static int done = 0;

/* Sinhronizacija i fajl */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_queue = PTHREAD_COND_INITIALIZER;
static FILE *file_out = NULL;

node_t *create_sample_tree(void) {
    static node_t nodes[12];
    static node_t *children_0[3];
    static node_t *children_1[2];
    static node_t *children_2[1];
    static node_t *children_3[2];
    static node_t *children_5[2];
    static node_t *children_8[1];
    static int initialized = 0;

    if (!initialized) {
        for (int i = 0; i < 12; i++) {
            nodes[i].value = i;
            nodes[i].child_count = 0;
            nodes[i].children = NULL;
        }

        children_0[0] = &nodes[1];
        children_0[1] = &nodes[2];
        children_0[2] = &nodes[3];
        nodes[0].children = children_0;
        nodes[0].child_count = 3;

        children_1[0] = &nodes[4];
        children_1[1] = &nodes[5];
        nodes[1].children = children_1;
        nodes[1].child_count = 2;

        children_2[0] = &nodes[6];
        nodes[2].children = children_2;
        nodes[2].child_count = 1;

        children_3[0] = &nodes[7];
        children_3[1] = &nodes[8];
        nodes[3].children = children_3;
        nodes[3].child_count = 2;

        children_5[0] = &nodes[9];
        children_5[1] = &nodes[10];
        nodes[5].children = children_5;
        nodes[5].child_count = 2;

        children_8[0] = &nodes[11];
        nodes[8].children = children_8;
        nodes[8].child_count = 1;

        initialized = 1;
    }
    return &nodes[0];
}

static void enqueue(node_t *node) {
    job_queue[queue_tail] = node;
    queue_tail = (queue_tail + 1) % QUEUE_MAX;
    queue_count++;
    pthread_cond_signal(&cond_queue);
}

static node_t *dequeue(void) {
    node_t *node = job_queue[queue_head];
    queue_head = (queue_head + 1) % QUEUE_MAX;
    queue_count--;
    return node;
}

static void *worker_thread(void *arg) {
    (void)arg;

    while (1) {
        pthread_mutex_lock(&mutex);

        /* Uslov za cekanje: red je prazan, ali posao nije gotov jer neko jos radi */
        while (queue_count == 0 && !done) {
            pthread_cond_wait(&cond_queue, &mutex);
        }

        if (done) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        /* Uzimanje posla i inkrementiranje aktivnih radnika */
        node_t *current = dequeue();
        active_threads++;
        pthread_mutex_unlock(&mutex);

        /* Obrada cvora i upis u fajl */
        if (current->child_count > 0 && current->children != NULL) {
            pthread_mutex_lock(&mutex);
            
            /* KLJUČNA IZMENA: Pišemo u fajl samo ako je uspešno otvoren, inače samo ispisujemo na ekran */
            for (int i = 0; i < current->child_count; i++) {
                if (file_out) {
                    fprintf(file_out, "%d -> %d\n", current->value, current->children[i]->value);
                    fflush(file_out);
                } else {
                    printf("[KONZOLA] Veza: %d -> %d\n", current->value, current->children[i]->value);
                    fflush(stdout);
                }
            }
            
            /* Dodavanje dece u red poslova */
            for (int i = 0; i < current->child_count; i++) {
                enqueue(current->children[i]);
            }
            pthread_mutex_unlock(&mutex);
        }

        /* Simulacija obrade */
        usleep(20000); 

        pthread_mutex_lock(&mutex);
        active_threads--;

        /* USLOV ZAVRSETKA: Red je prazan i nijedna nit ne obradjuje cvor */
        if (queue_count == 0 && active_threads == 0) {
            done = 1;
            pthread_cond_broadcast(&cond_queue);
        }
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main(void) {
    node_t *root = create_sample_tree();
    pthread_t workers[N];

    /* Zastita za online kompajlere */
    file_out = fopen("stablo.txt", "w");
    if (!file_out) {
        printf("[INFO] Nemam permisiju za fajl stablo.txt, ispis radim na standardni izlaz.\n\n");
        fflush(stdout);
    }

    /* Na pocetku ubacujemo samo koren */
    pthread_mutex_lock(&mutex);
    enqueue(root);
    pthread_mutex_unlock(&mutex);

    for (int i = 0; i < N; i++) {
        pthread_create(&workers[i], NULL, worker_thread, NULL);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(workers[i], NULL);
    }

    if (file_out) {
        fclose(file_out);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_queue);

    printf("\nKonkurentni obilazak stabla zavrsen uspesno.\n");
    return 0;
}