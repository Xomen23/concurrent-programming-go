/*
 * Sleeping Barber Problem
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   pthread_mutex_t + pthread_cond_t  ->  sync.Mutex + sync.Cond
 *   pthread_cond_wait(&c, &m)         ->  cond.Wait()  (otpusta mutex dok ceka)
 *   pthread_cond_signal(&c)           ->  cond.Signal()
 * -----------------------------------------------------------
 */

package main

import (
	"fmt"
	"math/rand"
	"sync"
	"time"
)

const (
	CHAIRS          = 3
	TOTAL_CUSTOMERS = 8
)

var (
	waitingCustomers int
	barberSleeping   bool
	allDone          bool

	mu            sync.Mutex
	condBarber    = sync.NewCond(&mu)
	condCustomers = sync.NewCond(&mu)
)

func barberThread(wg *sync.WaitGroup) {
	defer wg.Done()

	for {
		mu.Lock()

		for waitingCustomers == 0 && !allDone {
			fmt.Println("Nema musterija. Berberin spava...")
			barberSleeping = true
			condBarber.Wait()
			barberSleeping = false
		}

		if waitingCustomers == 0 && allDone {
			mu.Unlock()
			break
		}

		waitingCustomers--
		fmt.Printf("Berberin poziva musteriju. (U cekaonici ostaje: %d)\n", waitingCustomers)
		condCustomers.Signal()
		mu.Unlock()

		fmt.Println("Berberin sisa musteriju...")
		time.Sleep(40 * time.Millisecond)
		fmt.Println("Berberin je zavrsio sisanje.")
	}

	fmt.Println("Berberin zatvara radnju za danas.")
}

func customerThread(id int, wg *sync.WaitGroup) {
	defer wg.Done()

	time.Sleep(time.Duration(rand.Intn(150)) * time.Millisecond)

	mu.Lock()
	fmt.Printf("Musterija %d stize u berbernicu.\n", id)

	if waitingCustomers < CHAIRS {
		waitingCustomers++
		fmt.Printf("Musterija %d seda u cekaonicu. (Ukupno ceka: %d)\n", id, waitingCustomers)

		if barberSleeping {
			fmt.Printf("Musterija %d budi berberina!\n", id)
			condBarber.Signal()
		}

		condCustomers.Wait()
		fmt.Printf("Musterija %d ulazi na stolicu za sisanje.\n", id)
	} else {
		fmt.Printf("Cekaonica je puna! Musterija %d odlazi bez sisanja.\n", id)
	}

	mu.Unlock()
}

func main() {
	var wg sync.WaitGroup

	wg.Add(1)
	go barberThread(&wg)

	for i := 1; i <= TOTAL_CUSTOMERS; i++ {
		wg.Add(1)
		go customerThread(i, &wg)
	}

	// Cekamo sve musterije
	// Napomena: moramo posebno da pratimo kraj musterija
	customerWg := sync.WaitGroup{}
	for i := 1; i <= TOTAL_CUSTOMERS; i++ {
		customerWg.Add(1)
		go func(id int) {
			defer customerWg.Done()
		}(i)
	}

	time.Sleep(3 * time.Second) // Cekamo da sve goroutine zavrse

	mu.Lock()
	allDone = true
	condBarber.Signal()
	mu.Unlock()

	wg.Wait()
	fmt.Println("\n[USPEH] Svi klijenti procesuirani, program uspesno zatvoren!")
}
