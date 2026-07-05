/*
 * Deljeno stanje:
 * Promenljiva counter koju sve niti uvecavaju.
 *
 * Tip race condition-a:
 * Vise niti istovremeno citaju i upisuju istu promenljivu
 * bez sinhronizacije, pa se neka uvecavanja izgube.
 *
 * Ocekivano ponasanje:
 * Program pokrece 2 niti, svaka uvecava counter 1000000 puta.
 * Ocekujemo da counter na kraju bude 2000000.
 * Sa mutexom, rezultat je uvek tacan.
 * Bez muteksa, rezultat bi bio manji od 2000000.
 */

#include <stdio.h>
#include <pthread.h>

int counter = 0;
pthread_mutex_t mutex;

void *posao(void *args) {
    for (int i = 0; i < 1000000; i++) {
        pthread_mutex_lock(&mutex);   /* zakljucaj - ulaz u kriticnu sekciju */
        counter++;                     /* kriticna sekcija */
        pthread_mutex_unlock(&mutex); /* otkljucaj - izlaz iz kriticne sekcije */
    }
    return NULL;
}

int main() {
    pthread_mutex_init(&mutex, NULL);

    pthread_t t1, t2;
    pthread_create(&t1, NULL, posao, NULL);
    pthread_create(&t2, NULL, posao, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("counter = %d\n", counter);

    pthread_mutex_destroy(&mutex);
    return 0;
}

/*
 * Zasto moze doci do race condition-a:
 * counter++ se izvrsava u tri koraka: ucitaj, uvecaj, upisi.
 * Ako dve niti istovremeno ucitaju istu vrednost, jedno
 * uvecanje se izgubi jer obe upisuju isti rezultat.
 *
 * Kriticna sekcija:
 * Linija counter++ jer tu dolazi do pristupa deljenoj promenljivoj.
 *
 * Kako je izvrsena sinhronizacija:
 * Koristimo pthread_mutex_t. Pre counter++ nit zakljuca mutex,
 * a posle otkljuca. Dok jedna nit drzi mutex, druga ceka.
 * Tako u svakom trenutku samo jedna nit izvrsava counter++.
 *
 * Promena ponasanja nakon sinhronizacije:
 * Bez mutexa: rezultat je razlicit pri svakom pokretanju i manji od 2000000.
 * Sa mutexom: rezultat je uvek tacno 2000000.
 * Program je sporiji jer niti cekaju jedna drugu, ali je korektan.
 */