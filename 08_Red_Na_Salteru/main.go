/*
 * Red na salteru - Sleeping Barber sa FIFO i sluzbenicom
 * Identicna logika kao 07_Studentske_Konsultacije
 */

package main

import (
	"fmt"
	"math/rand"
	"sync"
	"time"
)

const (
	STUDENT_COUNT     = 10
	WAITING_ROOM_SIZE = 3
)

var (
	queue                         [WAITING_ROOM_SIZE]int
	head, tail, count             int
	currentStudentAtDesk          int
	clerkCalled, requestProcessed bool
	activeStudents                int

	mu           sync.Mutex
	condClerk    = sync.NewCond(&mu)
	condStudents [STUDENT_COUNT + 1]*sync.Cond
)

func init() {
	activeStudents = STUDENT_COUNT
	for i := 1; i <= STUDENT_COUNT; i++ {
		condStudents[i] = sync.NewCond(&mu)
	}
}

func randomSleep(min, max int) {
	time.Sleep(time.Duration(min+rand.Intn(max-min+1)) * time.Millisecond)
}

func clerkThread(wg *sync.WaitGroup) {
	defer wg.Done()
	mu.Lock()
	for activeStudents > 0 {
		for count == 0 && activeStudents > 0 {
			condClerk.Wait()
		}
		if activeStudents <= 0 && count == 0 {
			break
		}

		studentID := queue[head]
		head = (head + 1) % WAITING_ROOM_SIZE
		count--
		currentStudentAtDesk = studentID
		clerkCalled = true
		requestProcessed = false

		fmt.Printf("Sluzbenik proziva studenta %d.\n", studentID)
		condStudents[studentID].Signal()

		for !requestProcessed {
			mu.Unlock()
			randomSleep(50, 150)
			mu.Lock()
			if !clerkCalled {
				requestProcessed = true
				fmt.Printf("Sluzbenik zavrsava obradu zahteva studenta %d.\n", studentID)
				condStudents[studentID].Signal()
			}
		}
	}
	mu.Unlock()
	fmt.Println("Sluzbenik zatvara salter.")
}

func studentThread(id int, wg *sync.WaitGroup) {
	defer wg.Done()
	randomSleep(10, 200)
	mu.Lock()
	fmt.Printf("Student %d dolazi ispred studentske sluzbe.\n", id)
	if count < WAITING_ROOM_SIZE {
		queue[tail] = id
		tail = (tail + 1) % WAITING_ROOM_SIZE
		count++
		fmt.Printf("Student %d seda u cekaonicu.\n", id)
		condClerk.Signal()
		for currentStudentAtDesk != id || !clerkCalled {
			condStudents[id].Wait()
		}
		fmt.Printf("Student %d prilazi salteru.\n", id)
		clerkCalled = false
		condStudents[id].Wait()
		activeStudents--
	} else {
		fmt.Printf("Nema mesta u cekaonici. Student %d odlazi.\n", id)
		activeStudents--
		condClerk.Signal()
	}
	mu.Unlock()
}

func main() {
	var wg sync.WaitGroup
	wg.Add(1)
	go clerkThread(&wg)
	for i := 1; i <= STUDENT_COUNT; i++ {
		wg.Add(1)
		go studentThread(i, &wg)
	}
	// Budimo sluzbenika na kraju
	go func() {
		time.Sleep(3 * time.Second)
		mu.Lock()
		condClerk.Signal()
		mu.Unlock()
	}()
	wg.Wait()
	fmt.Println("\nSimulacija uspesno zavrsena.")
}
