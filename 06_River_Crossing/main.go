/*
 * River Crossing - camac kapaciteta 4
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   pthread_cond_broadcast(&c)  ->  cond.Broadcast()
 *   pthread_cond_wait(&c, &m)   ->  cond.Wait()
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
	TOTAL_PASSENGERS = 12
	BOAT_CAPACITY    = 4
)

var (
	passengersInLine int
	passengersOnBoat int
	boatIsRowing     bool

	mu            sync.Mutex
	condPassenger = sync.NewCond(&mu)
	condDisembark = sync.NewCond(&mu)
)

func passengerThread(id int, wg *sync.WaitGroup) {
	defer wg.Done()

	time.Sleep(time.Duration(10+rand.Intn(290)) * time.Millisecond)

	mu.Lock()
	passengersInLine++
	fmt.Printf("Putnik %02d je stigao na obalu. (U redu: %d)\n", id, passengersInLine)

	for boatIsRowing {
		condPassenger.Wait()
	}

	passengersInLine--
	passengersOnBoat++
	fmt.Printf("Putnik %02d se ukrcao u camac. (Na camcu: %d/%d)\n", id, passengersOnBoat, BOAT_CAPACITY)

	if passengersOnBoat == BOAT_CAPACITY {
		boatIsRowing = true
		fmt.Printf(">>> Camac je pun! Putnik %02d pokrece camac i vozi preko reke... <<<\n", id)

		mu.Unlock()
		time.Sleep(time.Duration(150+rand.Intn(100)) * time.Millisecond)
		mu.Lock()

		fmt.Println(">>> Camac je stigao na drugu obalu. <<<")
		condDisembark.Broadcast()
	} else {
		for !boatIsRowing || passengersOnBoat == BOAT_CAPACITY {
			condDisembark.Wait()
			if boatIsRowing {
				break
			}
		}
	}

	passengersOnBoat--
	fmt.Printf("Putnik %02d je izasao iz camca.\n", id)

	if passengersOnBoat == 0 {
		boatIsRowing = false
		fmt.Println(">>> Camac je prazan i vraca se nazad. <<<\n")
		condPassenger.Broadcast()
	}

	mu.Unlock()
}

func main() {
	var wg sync.WaitGroup

	for i := 1; i <= TOTAL_PASSENGERS; i++ {
		wg.Add(1)
		go passengerThread(i, &wg)
	}

	wg.Wait()
	fmt.Println("Simulacija uspesno zavrsena. Svi putnici su presli reku.")
}
