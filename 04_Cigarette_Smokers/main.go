/*
 * Cigarette Smokers Problem
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   pthread_mutex_t + pthread_cond_t  ->  sync.Mutex + sync.Cond
 *   pthread_cond_wait(&c, &m)         ->  cond.Wait()
 *   pthread_cond_signal(&c)           ->  cond.Signal()
 *   pthread_cond_broadcast(&c)        ->  cond.Broadcast()
 * -----------------------------------------------------------
 */

package main

import (
	"fmt"
	"math/rand"
	"sync"
	"time"
)

const ITERATIONS = 6

var (
	tableTobacco, tablePaper, tableMatches int
	tableBusy, agentReady, done            int

	mu          sync.Mutex
	condAgent   = sync.NewCond(&mu)
	condTobacco = sync.NewCond(&mu)
	condPaper   = sync.NewCond(&mu)
	condMatches = sync.NewCond(&mu)
)

func init() {
	agentReady = 1
}

func agentThread(wg *sync.WaitGroup) {
	defer wg.Done()

	for i := 0; i < ITERATIONS; i++ {
		mu.Lock()
		for agentReady == 0 {
			condAgent.Wait()
		}

		r := rand.Intn(3)
		if r == 0 {
			tablePaper = 1
			tableMatches = 1
			fmt.Println("Agent stavlja na sto: PAPIR + SIBICE")
		} else if r == 1 {
			tableTobacco = 1
			tableMatches = 1
			fmt.Println("Agent stavlja na sto: DUVAN + SIBICE")
		} else {
			tableTobacco = 1
			tablePaper = 1
			fmt.Println("Agent stavlja na sto: DUVAN + PAPIR")
		}

		tableBusy = 1
		agentReady = 0

		if tablePaper == 1 && tableMatches == 1 {
			condTobacco.Signal()
		} else if tableTobacco == 1 && tableMatches == 1 {
			condPaper.Signal()
		} else if tableTobacco == 1 && tablePaper == 1 {
			condMatches.Signal()
		}

		mu.Unlock()
		time.Sleep(50 * time.Millisecond)
	}

	mu.Lock()
	done = 1
	condTobacco.Broadcast()
	condPaper.Broadcast()
	condMatches.Broadcast()
	mu.Unlock()
}

func smokerThread(tip int, wg *sync.WaitGroup) {
	defer wg.Done()

	names := []string{"Pusac sa duvanom", "Pusac sa papirom", "Pusac sa sibicama"}

	for {
		mu.Lock()

		if tip == 0 {
			for (tablePaper == 0 || tableMatches == 0) && done == 0 {
				condTobacco.Wait()
			}
		} else if tip == 1 {
			for (tableTobacco == 0 || tableMatches == 0) && done == 0 {
				condPaper.Wait()
			}
		} else {
			for (tableTobacco == 0 || tablePaper == 0) && done == 0 {
				condMatches.Wait()
			}
		}

		if done == 1 && tableBusy == 0 {
			mu.Unlock()
			break
		}

		fmt.Printf("%s prepoznaje sastojke, uzima ih i pravi cigaretu.\n", names[tip])
		tableTobacco = 0
		tablePaper = 0
		tableMatches = 0
		tableBusy = 0
		mu.Unlock()

		fmt.Printf("%s pusi...\n", names[tip])
		time.Sleep(50 * time.Millisecond)

		mu.Lock()
		fmt.Printf("%s je zavrsio i signalizira agentu.\n", names[tip])
		agentReady = 1
		condAgent.Signal()
		mu.Unlock()
	}
}

func main() {
	var wg sync.WaitGroup

	for i := 0; i < 3; i++ {
		wg.Add(1)
		go smokerThread(i, &wg)
	}

	wg.Add(1)
	go agentThread(&wg)

	wg.Wait()
	fmt.Println("\n[USPEH] Svi sastojci potroseni, sesija zavrsena!")
}
