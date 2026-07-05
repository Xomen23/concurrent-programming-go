/*
 * Ocekivano ponasanje:
 * Program pravi 3 niti i svaka uvecava counter 1000000 puta.
 * Ocekujemo da na kraju counter bude 3000000.
 * 
 * Kada bi program bio deterministicki, svako pokretanje
 * bi ispisalo: "counter = 3000000"
 */

#include <stdio.h>
#include <pthread.h>

int counter = 0;

void *posao(void *args) {
    for (int i = 0; i < 1000000; i++) {
        counter++;
    }
    return NULL;
}

int main() {
    pthread_t t1, t2, t3;

    pthread_create(&t1, NULL, posao, NULL);
    pthread_create(&t2, NULL, posao, NULL);
    pthread_create(&t3, NULL, posao, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    printf("counter = %d\n", counter);

    return 0;
}

/*
 * Sta se stvarno desava:
 * Rezultat nije uvek 3000000. Ponekad je 2150000, ponekad 1900000, itd.
 * 
 * Zasto:
 * counter++ nije jedna operacija, nego tri:
 *   - ucitaj counter
 *   - uvecaj za 1
 *   - upisi nazad
 * 
 * Moze se desiti da dve niti ucitaju istu vrednost
 * u isto vreme, pa jedno uvecanje bude izgubljeno.
 * 
 * Na primer:
 *   Nit1 ucita counter = 5
 *   Nit2 ucita counter = 5
 *   Nit1 upise counter = 6
 *   Nit2 upise counter = 6  (trebalo je biti 7!)
 * 
 * Deljena promenljiva koja pravi problem: counter
 * Problematicna linija: counter++
 */