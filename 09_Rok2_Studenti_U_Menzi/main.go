/*
 * Studenti u menzi - Producer-Consumer varijanta
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   static int available_meals  ->  var availableMeals int
 *   static int refill_requested ->  var refillRequested bool
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
	NUM_STUDENTS      = 8
	MEALS_PER_STUDENT = 4
	MENU_CAPACITY     = 5
)

var (
	availableMeals    int
	mealsLeft         = NUM_STUDENTS * MEALS_PER_STUDENT
	refillRequested   bool
	violationDetected bool

	mu           sync.Mutex
	condCook     = sync.NewCond(&mu)
	condStudents = sync.NewCond(&mu)
)

func randomSleep(min, max int) {
	time.Sleep(time.Duration(min+rand.Intn(max-min+1)) * time.Millisecond)
}

func cookRefill() {
	randomSleep(80, 180)
	availableMeals = MENU_CAPACITY
	refillRequested = false
	fmt.Printf("KUVAR: izneo novu turu, na liniji je %d obroka\n", availableMeals)
}

func takeMeal(studentID int) {
	mu.Lock()
	for availableMeals == 0 {
		if !refillRequested {
			refillRequested = true
			fmt.Printf("STUDENT %02d: linija je prazna, treba obavestiti kuvara\n", studentID)
			condCook.Signal()
		}
		condStudents.Wait()
	}
	availableMeals--
	mealsLeft--
	fmt.Printf("STUDENT %02d: uzima obrok | na liniji: %d | preostalo: %d\n", studentID, availableMeals, mealsLeft)
	mu.Unlock()
}

func cookThread(wg *sync.WaitGroup) {
	defer wg.Done()
	mu.Lock()
	for {
		if mealsLeft <= 0 {
			break
		}
		for !refillRequested && mealsLeft > 0 {
			condCook.Wait()
		}
		if mealsLeft <= 0 {
			break
		}
		cookRefill()
		condStudents.Broadcast()
	}
	mu.Unlock()
	fmt.Println("KUVAR: zavrsava rad")
}

func studentThread(id int, wg *sync.WaitGroup) {
	defer wg.Done()
	for i := 0; i < MEALS_PER_STUDENT; i++ {
		randomSleep(20, 160)
		takeMeal(id)
		randomSleep(30, 120)
	}
	fmt.Printf("STUDENT %02d: zavrsio sve obroke\n", id)
}

func main() {
	var wg sync.WaitGroup

	wg.Add(1)
	go cookThread(&wg)

	for i := 1; i <= NUM_STUDENTS; i++ {
		wg.Add(1)
		go studentThread(i, &wg)
	}

	// Budimo kuvara kad svi studenti zavrse
	go func() {
		time.Sleep(5 * time.Second)
		mu.Lock()
		condCook.Signal()
		mu.Unlock()
	}()

	wg.Wait()

	if violationDetected {
		fmt.Println("\nSimulacija zavrsena, ali su detektovane greske.")
	} else {
		fmt.Println("\nSimulacija zavrsena bez detektovanih gresaka.")
	}
}
