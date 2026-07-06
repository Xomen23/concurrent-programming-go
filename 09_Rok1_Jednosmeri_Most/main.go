/*
 * Jednosmerni most - max 3 auta, samo jedan smer
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   const char *dir_name[] = {...}  ->  var dirName = []string{...}
 *   struct car_arg_t                ->  struct carArg
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
	NUM_CARS      = 24
	MAX_ON_BRIDGE = 3
	NORTH         = 0
	SOUTH         = 1
)

var dirName = []string{"SEVER", "JUG"}

var (
	carsOnBridge      int
	currentDir        = -1
	violationDetected bool

	mu        sync.Mutex
	condNorth = sync.NewCond(&mu)
	condSouth = sync.NewCond(&mu)
)

func randomSleep(min, max int) {
	time.Sleep(time.Duration(min+rand.Intn(max-min+1)) * time.Millisecond)
}

func bridgeEnter(id, dir int) {
	mu.Lock()
	for (currentDir != -1 && currentDir != dir) || carsOnBridge >= MAX_ON_BRIDGE {
		if dir == NORTH {
			condNorth.Wait()
		} else {
			condSouth.Wait()
		}
	}
	if carsOnBridge == 0 {
		currentDir = dir
	}
	carsOnBridge++
	fmt.Printf("AUTO %02d (%s) ULAZI  | na mostu: %d\n", id, dirName[dir], carsOnBridge)
	mu.Unlock()
}

func bridgeLeave(id, dir int) {
	mu.Lock()
	carsOnBridge--
	if carsOnBridge == 0 {
		currentDir = -1
		condNorth.Broadcast()
		condSouth.Broadcast()
	} else {
		if dir == NORTH {
			condNorth.Signal()
		} else {
			condSouth.Signal()
		}
	}
	fmt.Printf("AUTO %02d (%s) IZLAZI | na mostu: %d\n", id, dirName[dir], carsOnBridge)
	mu.Unlock()
}

func carThread(id, dir int, wg *sync.WaitGroup) {
	defer wg.Done()
	randomSleep(10, 120)
	fmt.Printf("AUTO %02d (%s) CEKA\n", id, dirName[dir])
	bridgeEnter(id, dir)
	randomSleep(80, 220)
	bridgeLeave(id, dir)
}

func main() {
	var wg sync.WaitGroup

	for i := 1; i <= NUM_CARS; i++ {
		wg.Add(1)
		dir := rand.Intn(2)
		go carThread(i, dir, &wg)
	}

	wg.Wait()

	if violationDetected {
		fmt.Println("\nSimulacija zavrsena, ali su detektovane greske.")
	} else {
		fmt.Println("\nSimulacija zavrsena bez detektovanih gresaka.")
	}
}
