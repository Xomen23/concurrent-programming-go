/*
 * Dining Philosophers - asimetricno resenje
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   pthread_mutex_t sticks[N]         ->  var sticks [N]sync.Mutex
 *   pthread_mutex_lock(&sticks[i])    ->  sticks[i].Lock()
 *   pthread_mutex_unlock(&sticks[i])  ->  sticks[i].Unlock()
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
	NUM_PHILOSOPHERS = 5
	ITERATIONS       = 3
)

var sticks [NUM_PHILOSOPHERS]sync.Mutex

func philosopher(id int, wg *sync.WaitGroup) {
	defer wg.Done()

	left := id
	right := (id + 1) % NUM_PHILOSOPHERS

	for i := 0; i < ITERATIONS; i++ {
		fmt.Printf("Filozof %d razmislja.\n", id)
		time.Sleep(time.Duration(10+rand.Intn(50)) * time.Millisecond)

		fmt.Printf("Filozof %d pokusava da uzme stapice.\n", id)

		// ASIMETRICNO RESENJE: neparni filozofi uzimaju kontra redosledom
		if id%2 != 0 {
			sticks[right].Lock()
			sticks[left].Lock()
		} else {
			sticks[left].Lock()
			sticks[right].Lock()
		}

		fmt.Printf("Filozof %d je uspesno uzeo oba stapica i jede.\n", id)
		time.Sleep(time.Duration(10+rand.Intn(50)) * time.Millisecond)

		sticks[left].Unlock()
		sticks[right].Unlock()

		fmt.Printf("Filozof %d je vratio stapice.\n", id)
	}
}

func main() {
	var wg sync.WaitGroup

	for i := 0; i < NUM_PHILOSOPHERS; i++ {
		wg.Add(1)
		go philosopher(i, &wg)
	}

	wg.Wait()
	fmt.Println("\n[USPEH] Svi filozofi su zavrsili sa jelom i razmisljanjem!")
}
