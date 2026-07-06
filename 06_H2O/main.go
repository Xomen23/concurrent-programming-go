/*
 * H2O Problem - formiranje molekula vode
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   sem_t s; sem_init(&s,0,0)    ->  make(chan struct{}, 0) = unbuffered channel
 *   sem_post(&s)                 ->  s <- struct{}{}
 *   sem_wait(&s)                 ->  <-s
 *   pthread_barrier_t b          ->  sync.WaitGroup ili CyclicBarrier
 *
 *   Za barijeru u Go koristimo sync.WaitGroup po grupi:
 *   svaka grupa od 2H+1O pravi svoju WaitGroup
 * -----------------------------------------------------------
 */

package main

import (
	"fmt"
	"sync"
)

const NUM_MOLECULES = 10

var (
	hCount, oCount int
	mu             sync.Mutex
	semH           = make(chan struct{}, NUM_MOLECULES*2)
	semO           = make(chan struct{}, NUM_MOLECULES)
)

func bond(atomType string, id int, barrier *sync.WaitGroup) {
	fmt.Printf("Atom %s [%02d] je usao u vezu. Stvara se H2O...\n", atomType, id)
	barrier.Done()
}

func hThread(id int, wg *sync.WaitGroup) {
	defer wg.Done()

	mu.Lock()
	hCount++

	if hCount >= 2 && oCount >= 1 {
		semH <- struct{}{}
		semH <- struct{}{}
		semO <- struct{}{}
		hCount -= 2
		oCount -= 1
		mu.Unlock()
	} else {
		mu.Unlock()
		<-semH
	}

	var barrier sync.WaitGroup
	barrier.Add(1)
	bond("H", id, &barrier)
}

func oThread(id int, wg *sync.WaitGroup) {
	defer wg.Done()

	mu.Lock()
	oCount++

	if hCount >= 2 && oCount >= 1 {
		semH <- struct{}{}
		semH <- struct{}{}
		semO <- struct{}{}
		hCount -= 2
		oCount -= 1
		mu.Unlock()
	} else {
		mu.Unlock()
		<-semO
	}

	var barrier sync.WaitGroup
	barrier.Add(1)
	bond("O", id, &barrier)
}

func main() {
	var wg sync.WaitGroup

	for i := 0; i < NUM_MOLECULES; i++ {
		wg.Add(1)
		go oThread(i+1, &wg)
	}

	for i := 0; i < NUM_MOLECULES*2; i++ {
		wg.Add(1)
		go hThread(i+1, &wg)
	}

	wg.Wait()
	fmt.Printf("\nSimulacija uspesno zavrsena. Formirano je %d molekula vode!\n", NUM_MOLECULES)
}
