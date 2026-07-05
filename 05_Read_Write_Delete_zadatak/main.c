#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define N 8

typedef struct node {
    int val;
    sem_t lock;
    struct node *next;
} node_t;

typedef struct {
    node_t *HEAD;
    sem_t list_lock; 
} LinkedList;

node_t *make_node(int val) {
    node_t *n = malloc(sizeof(node_t));
    n->val = val;
    n->next = NULL;
    sem_init(&n->lock, 0, 1);
    return n;
}

int contains(LinkedList *list, int val) {
    sem_wait(&list->list_lock);
    node_t *curr = list->HEAD;

    if (curr == NULL) {
        sem_post(&list->list_lock);
        return 0;
    }

    sem_wait(&curr->lock);
    sem_post(&list->list_lock); 

    while (curr->next != NULL && curr->val < val) {
        node_t *next = curr->next;
        sem_wait(&next->lock);
        sem_post(&curr->lock);
        curr = next;
    }

    int result = (curr->val == val);
    sem_post(&curr->lock);
    return result;
}

int insert(LinkedList *list, int val) {
    node_t *new_node = make_node(val);

    sem_wait(&list->list_lock);

    if (list->HEAD == NULL || list->HEAD->val > val) {
        new_node->next = list->HEAD;
        list->HEAD = new_node;
        sem_post(&list->list_lock);
        return 1;
    }

    if (list->HEAD->val == val) {
        sem_post(&list->list_lock);
        free(new_node);
        return 0;
    }

    node_t *prev = list->HEAD;
    sem_wait(&prev->lock);
    sem_post(&list->list_lock); 

    node_t *curr = prev->next;

    while (curr != NULL && curr->val < val) {
        sem_wait(&curr->lock);
        sem_post(&prev->lock);
        prev = curr;
        curr = curr->next;
    }

    if (curr != NULL && curr->val == val) {
        sem_post(&prev->lock);
        free(new_node);
        return 0;
    }

    new_node->next = curr;
    prev->next = new_node;
    sem_post(&prev->lock);
    return 1;
}

int delete(LinkedList *list, int val) {
    sem_wait(&list->list_lock);

    if (list->HEAD == NULL) {
        sem_post(&list->list_lock);
        return 0;
    }

    if (list->HEAD->val == val) {
        node_t *to_delete = list->HEAD;
        list->HEAD = list->HEAD->next;
        sem_post(&list->list_lock);
        sem_destroy(&to_delete->lock);
        free(to_delete);
        return 1;
    }

    node_t *prev = list->HEAD;
    sem_wait(&prev->lock);
    sem_post(&list->list_lock);

    node_t *curr = prev->next;

    while (curr != NULL && curr->val < val) {
        sem_wait(&curr->lock);
        sem_post(&prev->lock);
        prev = curr;
        curr = curr->next;
    }

    if (curr == NULL || curr->val != val) {
        sem_post(&prev->lock);
        return 0;
    }

    sem_wait(&curr->lock);
    prev->next = curr->next;
    sem_post(&curr->lock);
    sem_post(&prev->lock);
    sem_destroy(&curr->lock);
    free(curr);
    return 1;
}

void printList(LinkedList *list) {
    sem_wait(&list->list_lock);
    node_t *curr = list->HEAD;
    printf("[ ");
    while (curr != NULL) {
        printf("%d ", curr->val);
        curr = curr->next;
    }
    printf("]\n");
    sem_post(&list->list_lock);
}

typedef struct {
    LinkedList *list;
    int id;
} TArg;

void *worker(void *args) {
    TArg *targ = args;

    for (int i = 0; i < 20; i++) {
        int val    = rand() % 10;
        int op     = rand() % 3;
        int result;

        switch (op) {
        case 0:
            result = insert(targ->list, val);
            printf("[T-%d] insert %d -> %s\n", targ->id, val, result ? "ok" : "vec postoji");
            break;
        case 1:
            result = contains(targ->list, val);
            printf("[T-%d] contains %d -> %s\n", targ->id, val, result ? "nasao" : "nije");
            break;
        case 2:
            result = delete(targ->list, val);
            printf("[T-%d] delete %d -> %s\n", targ->id, val, result ? "obrisan" : "nije nadjen");
            break;
        }
    }
    return NULL;
}

int main(void) {
    LinkedList list;
    list.HEAD = NULL;
    sem_init(&list.list_lock, 0, 1);

    insert(&list, 2);
    insert(&list, 5);
    insert(&list, 7);

    printf("Pocetna lista: ");
    printList(&list);

    pthread_t t[N];
    TArg args[N];
    for (int i = 0; i < N; i++) {
        args[i].id   = i;
        args[i].list = &list;
        pthread_create(&t[i], NULL, worker, &args[i]);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(t[i], NULL);
    }

    printf("Krajnja lista: ");
    printList(&list);

    return 0;
}

/*
 * Objasnjenje logike:
 *
 * Deljeni resursi:
 * - LinkedList i njeni cvorovi - vise niti ih cita i menja
 *
 * Kriticne sekcije:
 * - insert: prev->next = new_node
 * - delete: prev->next = curr->next
 * - contains: citanje curr->val i curr->next
 *
 * Sinhronizacija - hand over hand locking:
 * Svaki cvor ima svoj semafor.
 * Uvek zakljucamo sledeci cvor PRE nego otkljucamo trenutni:
 *
 *   lock(prev)
 *   lock(curr)
 *   unlock(prev)
 *   prev = curr, curr = curr->next
 *
 * list_lock stiti HEAD pokazivac - koristi se samo
 * na pocetku dok ne zakljucamo prvi cvor, pa se otkljucava.
 *
 * Zbog toga dve niti mogu raditi na razlicitim delovima
 * liste u isto vreme.
 */