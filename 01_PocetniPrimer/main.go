/*
 * Ocekivano ponasanje:
 * Program pravi 3 goroutine i svaka uvecava counter 1000000 puta.
 * Ocekujemo da na kraju counter bude 3000000.
 *
 * Kada bi program bio deterministicki, svako pokretanje
 * bi ispisalo: "counter = 3000000"
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   pthread_create(&t, NULL, fn, NULL)  ->  go fn()
 *   pthread_join(t, NULL)               ->  wg.Wait()
 *   globalna int promenljiva            ->  var counter int
 * -----------------------------------------------------------
 */

package main

import (
	"fmt"
	"sync"
)

var counter int

func posao(wg *sync.WaitGroup) {
	defer wg.Done()
	for i := 0; i < 1000000; i++ {
		counter++ // race condition - nema zastite!
	}
}

func main() {
	var wg sync.WaitGroup

	wg.Add(3)
	go posao(&wg)
	go posao(&wg)
	go posao(&wg)

	wg.Wait()

	fmt.Printf("counter = %d\n", counter)
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
 * Moze se desiti da dve goroutine ucitaju istu vrednost
 * u isto vreme, pa jedno uvecanje bude izgubljeno.
 *
 * Deljena promenljiva koja pravi problem: counter
 * Problematicna linija: counter++
 */
