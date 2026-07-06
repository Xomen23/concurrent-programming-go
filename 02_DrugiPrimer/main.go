/*
 * Deljeno stanje:
 * Promenljiva counter koju sve goroutine uvecavaju.
 *
 * Tip race condition-a:
 * Vise goroutina istovremeno citaju i upisuju istu promenljivu
 * bez sinhronizacije, pa se neka uvecavanja izgube.
 *
 * Ocekivano ponasanje:
 * Program pokrece 2 goroutine, svaka uvecava counter 1000000 puta.
 * Ocekujemo da counter na kraju bude 2000000.
 * Sa mutexom, rezultat je uvek tacan.
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   pthread_mutex_t mutex               ->  var mu sync.Mutex
 *   pthread_mutex_init(&mutex, NULL)    ->  (automatski inicijalizovan)
 *   pthread_mutex_lock(&mutex)          ->  mu.Lock()
 *   pthread_mutex_unlock(&mutex)        ->  mu.Unlock()
 *   pthread_mutex_destroy(&mutex)       ->  (automatski, GC)
 * -----------------------------------------------------------
 */

package main

import (
	"fmt"
	"sync"
)

var counter int
var mu sync.Mutex

func posao(wg *sync.WaitGroup) {
	defer wg.Done()
	for i := 0; i < 1000000; i++ {
		mu.Lock()   // zakljucaj - ulaz u kriticnu sekciju
		counter++   // kriticna sekcija
		mu.Unlock() // otkljucaj - izlaz iz kriticne sekcije
	}
}

func main() {
	var wg sync.WaitGroup

	wg.Add(2)
	go posao(&wg)
	go posao(&wg)

	wg.Wait()

	fmt.Printf("counter = %d\n", counter)
}

/*
 * Zasto moze doci do race condition-a:
 * counter++ se izvrsava u tri koraka: ucitaj, uvecaj, upisi.
 * Ako dve goroutine istovremeno ucitaju istu vrednost, jedno
 * uvecanje se izgubi jer obe upisuju isti rezultat.
 *
 * Kriticna sekcija:
 * Linija counter++ jer tu dolazi do pristupa deljenoj promenljivoj.
 *
 * Kako je izvrsena sinhronizacija:
 * Koristimo sync.Mutex. Pre counter++ goroutina zakljuca mutex,
 * a posle otkljuca. Dok jedna goroutina drzi mutex, druga ceka.
 *
 * Promena ponasanja nakon sinhronizacije:
 * Bez mutexa: rezultat je razlicit pri svakom pokretanju.
 * Sa mutexom: rezultat je uvek tacno 2000000.
 */
