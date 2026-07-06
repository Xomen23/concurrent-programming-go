/*
 * Studentske Konsultacije - Sleeping Barber varijanta sa FIFO
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   pthread_cond_t cond_students[N]  ->  []sync.Cond  (slice kondicionih promenljivih)
 *   pthread_cond_init(&c, NULL)      ->  sync.NewCond(&mu)
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
	STUDENT_COUNT     = 10
	WAITING_ROOM_SIZE = 3
)

var (
	waitingRoom            [WAITING_ROOM_SIZE]int
	head, tail, count      int
	currentStudentInOffice int
	assistantCalled        bool
	sessionFinished        bool
	remainingStudents      int

	mu            sync.Mutex
	condAssistant = sync.NewCond(&mu)
	condStudents  [STUDENT_COUNT + 1]*sync.Cond
)

func init() {
	remainingStudents = STUDENT_COUNT
	for i := 1; i <= STUDENT_COUNT; i++ {
		condStudents[i] = sync.NewCond(&mu)
	}
}

func randomSleep(min, max int) {
	time.Sleep(time.Duration(min+rand.Intn(max-min+1)) * time.Millisecond)
}

func assistantThread(wg *sync.WaitGroup) {
	defer wg.Done()

	mu.Lock()
	for remainingStudents > 0 {
		for count == 0 && remainingStudents > 0 {
			condAssistant.Wait()
		}

		if remainingStudents <= 0 && count == 0 {
			break
		}

		studentID := waitingRoom[head]
		head = (head + 1) % WAITING_ROOM_SIZE
		count--

		currentStudentInOffice = studentID
		assistantCalled = true
		sessionFinished = false

		fmt.Printf("Asistent proziva studenta %d.\n", studentID)
		condStudents[studentID].Signal()

		for !sessionFinished {
			mu.Unlock()
			randomSleep(60, 120)
			mu.Lock()

			if !assistantCalled {
				sessionFinished = true
				fmt.Printf("Asistent zavrsava konsultacije sa studentom %d.\n", studentID)
				condStudents[studentID].Signal()
			}
		}
	}
	mu.Unlock()
}

func studentThread(id int, wg *sync.WaitGroup) {
	defer wg.Done()

	randomSleep(10, 200)

	mu.Lock()
	fmt.Printf("Student %d dolazi na konsultacije.\n", id)

	if count < WAITING_ROOM_SIZE {
		waitingRoom[tail] = id
		tail = (tail + 1) % WAITING_ROOM_SIZE
		count++

		fmt.Printf("Student %d seda u cekaonicu.\n", id)
		condAssistant.Signal()

		for currentStudentInOffice != id || !assistantCalled {
			condStudents[id].Wait()
		}

		fmt.Printf("Student %d ulazi na konsultacije.\n", id)
		assistantCalled = false

		condStudents[id].Wait()
		remainingStudents--
	} else {
		fmt.Printf("Nema mesta u cekaonici. Student %d odlazi.\n", id)
		remainingStudents--
		condAssistant.Signal()
	}

	mu.Unlock()
}

func main() {
	var wg sync.WaitGroup

	wg.Add(1)
	go assistantThread(&wg)

	for i := 1; i <= STUDENT_COUNT; i++ {
		wg.Add(1)
		go studentThread(i, &wg)
	}

	wg.Wait()
	fmt.Println("\nKonsultacije su zavrsene. Asistent zakljucava kabinet.")
}
