/*
 * Konkurentni obilazak stabla pomocu reda poslova
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   FILE *f = fopen("stablo.txt", "w")  ->  os.Create("stablo.txt")
 *   fprintf(f, "%d -> %d\n", ...)       ->  fmt.Fprintf(f, "%d -> %d\n", ...)
 *   static node_t job_queue[100]        ->  var jobQueue []*node  (slice)
 * -----------------------------------------------------------
 */

package main

import (
	"fmt"
	"os"
	"sync"
	"time"
)

const N = 4

type node struct {
	children   []*node
	childCount int
	value      int
}

var (
	jobQueue      []*node
	activeThreads int
	done          bool

	mu        sync.Mutex
	condQueue = sync.NewCond(&mu)
	fileOut   *os.File
)

func createSampleTree() *node {
	nodes := make([]node, 12)
	for i := range nodes {
		nodes[i].value = i
	}

	nodes[0].children = []*node{&nodes[1], &nodes[2], &nodes[3]}
	nodes[0].childCount = 3
	nodes[1].children = []*node{&nodes[4], &nodes[5]}
	nodes[1].childCount = 2
	nodes[2].children = []*node{&nodes[6]}
	nodes[2].childCount = 1
	nodes[3].children = []*node{&nodes[7], &nodes[8]}
	nodes[3].childCount = 2
	nodes[5].children = []*node{&nodes[9], &nodes[10]}
	nodes[5].childCount = 2
	nodes[8].children = []*node{&nodes[11]}
	nodes[8].childCount = 1

	return &nodes[0]
}

func enqueue(n *node) {
	jobQueue = append(jobQueue, n)
	condQueue.Signal()
}

func dequeue() *node {
	n := jobQueue[0]
	jobQueue = jobQueue[1:]
	return n
}

func workerThread(wg *sync.WaitGroup) {
	defer wg.Done()

	for {
		mu.Lock()

		for len(jobQueue) == 0 && !done {
			condQueue.Wait()
		}

		if done {
			mu.Unlock()
			break
		}

		current := dequeue()
		activeThreads++
		mu.Unlock()

		if current.childCount > 0 {
			mu.Lock()
			for i := 0; i < current.childCount; i++ {
				if fileOut != nil {
					fmt.Fprintf(fileOut, "%d -> %d\n", current.value, current.children[i].value)
				} else {
					fmt.Printf("[KONZOLA] Veza: %d -> %d\n", current.value, current.children[i].value)
				}
			}
			for i := 0; i < current.childCount; i++ {
				enqueue(current.children[i])
			}
			mu.Unlock()
		}

		time.Sleep(20 * time.Millisecond)

		mu.Lock()
		activeThreads--
		if len(jobQueue) == 0 && activeThreads == 0 {
			done = true
			condQueue.Broadcast()
		}
		mu.Unlock()
	}
}

func main() {
	root := createSampleTree()

	var err error
	fileOut, err = os.Create("stablo.txt")
	if err != nil {
		fmt.Println("[INFO] Ne mogu da otvorim stablo.txt, ispisujem na konzolu.\n")
	}

	mu.Lock()
	enqueue(root)
	mu.Unlock()

	var wg sync.WaitGroup
	for i := 0; i < N; i++ {
		wg.Add(1)
		go workerThread(&wg)
	}

	wg.Wait()

	if fileOut != nil {
		fileOut.Close()
	}
	fmt.Println("\nKonkurentni obilazak stabla zavrsen uspesno.")
}
