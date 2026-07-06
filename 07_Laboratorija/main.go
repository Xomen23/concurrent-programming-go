/*
 * Laboratorija - Cigarette Smokers varijanta sa policom
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   static const char* shelf[N]  ->  var shelf [N]string
 *   FILE *f = fopen(...)         ->  os.Create(...)  ili  os.OpenFile(...)
 *   fprintf(f, ...)              ->  fmt.Fprintf(f, ...)
 * -----------------------------------------------------------
 */

package main

import (
	"fmt"
	"math/rand"
	"os"
	"sync"
	"time"
)

const (
	ITERATIONS     = 20
	SHELF_CAPACITY = 3
)

var (
	tableBoard, tableSensor, tableBattery, tableFull int
	shelfCount, activeWorkers, done                  int

	mu             sync.Mutex
	condManager    = sync.NewCond(&mu)
	condController = sync.NewCond(&mu)
	condWBoard     = sync.NewCond(&mu)
	condWSensor    = sync.NewCond(&mu)
	condWBattery   = sync.NewCond(&mu)

	shelf     [SHELF_CAPACITY]string
	shelfHead int
	shelfTail int
)

func init() {
	activeWorkers = 3
}

func randomSleep(min, max int) {
	time.Sleep(time.Duration(min+rand.Intn(max-min+1)) * time.Millisecond)
}

func managerThread(wg *sync.WaitGroup) {
	defer wg.Done()

	for i := 0; i < ITERATIONS; i++ {
		mu.Lock()
		for tableFull == 1 {
			condManager.Wait()
		}

		choice := rand.Intn(3)
		if choice == 0 {
			tableBoard = 1
			tableSensor = 1
			fmt.Println("Rukovodilac stavlja: plocica + senzor")
		} else if choice == 1 {
			tableSensor = 1
			tableBattery = 1
			fmt.Println("Rukovodilac stavlja: senzor + baterija")
		} else {
			tableBoard = 1
			tableBattery = 1
			fmt.Println("Rukovodilac stavlja: plocica + baterija")
		}
		tableFull = 1

		if tableBoard == 1 && tableSensor == 1 {
			condWBattery.Signal()
		} else if tableSensor == 1 && tableBattery == 1 {
			condWBoard.Signal()
		} else if tableBoard == 1 && tableBattery == 1 {
			condWSensor.Signal()
		}
		mu.Unlock()
		randomSleep(40, 100)
	}

	mu.Lock()
	done = 1
	condWBoard.Broadcast()
	condWSensor.Broadcast()
	condWBattery.Broadcast()
	mu.Unlock()
}

func workerThread(tip int, wg *sync.WaitGroup) {
	defer wg.Done()
	names := []string{"Radnik sa plocicom", "Radnik sa senzorom", "Radnik sa baterijom"}

	for {
		mu.Lock()

		if tip == 0 {
			for (tableSensor == 0 || tableBattery == 0) && done == 0 {
				condWBoard.Wait()
			}
		} else if tip == 1 {
			for (tableBoard == 0 || tableBattery == 0) && done == 0 {
				condWSensor.Wait()
			}
		} else {
			for (tableBoard == 0 || tableSensor == 0) && done == 0 {
				condWBattery.Wait()
			}
		}

		if done == 1 && tableFull == 0 {
			mu.Unlock()
			break
		}

		tableBoard = 0
		tableSensor = 0
		tableBattery = 0
		tableFull = 0
		name := names[tip]
		fmt.Printf("%s sklapa proizvod.\n", name)
		condManager.Signal()

		for shelfCount >= SHELF_CAPACITY {
			condManager.Wait()
		}

		shelf[shelfTail] = name
		shelfTail = (shelfTail + 1) % SHELF_CAPACITY
		shelfCount++

		fmt.Printf("%s stavlja proizvod na policu.\n", name)
		condController.Signal()
		mu.Unlock()
		randomSleep(30, 80)
	}

	mu.Lock()
	activeWorkers--
	condController.Signal()
	mu.Unlock()
}

func controllerThread(wg *sync.WaitGroup) {
	defer wg.Done()

	f, err := os.Create("proizvodi.txt")
	if err != nil {
		fmt.Println("[INFO] Ne mogu da otvorim fajl, radim u memoriji.")
	}

	for {
		mu.Lock()
		for shelfCount == 0 && activeWorkers > 0 {
			condController.Wait()
		}

		if shelfCount == 0 && activeWorkers == 0 {
			mu.Unlock()
			break
		}

		maker := shelf[shelfHead]
		shelfHead = (shelfHead + 1) % SHELF_CAPACITY
		shelfCount--

		fmt.Printf("Kontrolor kvaliteta proverava proizvod %s.\n", maker)
		if f != nil {
			fmt.Fprintf(f, "Proveren proizvod radnika: %s\n", maker)
		}
		condManager.Broadcast()
		mu.Unlock()
		randomSleep(20, 60)
	}

	if f != nil {
		f.Close()
	}
}

func main() {
	var wg sync.WaitGroup

	wg.Add(1)
	go managerThread(&wg)
	wg.Add(1)
	go controllerThread(&wg)
	for i := 0; i < 3; i++ {
		wg.Add(1)
		go workerThread(i, &wg)
	}

	wg.Wait()
	fmt.Println("\nLaboratorija je uspesno zatvorena nakon svih iteracija.")
}
