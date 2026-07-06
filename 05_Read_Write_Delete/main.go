/*
 * Read-Write-Delete - Fine-grained locking na linked listi
 * Hand-over-hand locking: svaki cvor ima svoj mutex
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   sem_t lock; sem_init(&lock,0,1)  ->  var mu sync.Mutex  (u struct-u)
 *   sem_wait(&lock)                  ->  mu.Lock()
 *   sem_post(&lock)                  ->  mu.Unlock()
 *   malloc(sizeof(node_t))           ->  &node{} (Go GC, nema free)
 * -----------------------------------------------------------
 */

package main

import (
	"fmt"
	"math/rand"
	"sync"
)

const N = 8

type node struct {
	val  int
	mu   sync.Mutex
	next *node
}

type LinkedList struct {
	head     *node
	listLock sync.Mutex
}

func makeNode(val int) *node {
	return &node{val: val}
}

func (l *LinkedList) contains(val int) bool {
	l.listLock.Lock()
	curr := l.head

	if curr == nil {
		l.listLock.Unlock()
		return false
	}

	curr.mu.Lock()
	l.listLock.Unlock()

	for curr.next != nil && curr.val < val {
		next := curr.next
		next.mu.Lock()
		curr.mu.Unlock()
		curr = next
	}

	result := curr.val == val
	curr.mu.Unlock()
	return result
}

func (l *LinkedList) insert(val int) bool {
	newNode := makeNode(val)

	l.listLock.Lock()

	if l.head == nil || l.head.val > val {
		newNode.next = l.head
		l.head = newNode
		l.listLock.Unlock()
		return true
	}

	if l.head.val == val {
		l.listLock.Unlock()
		return false
	}

	prev := l.head
	prev.mu.Lock()
	l.listLock.Unlock()

	curr := prev.next

	for curr != nil && curr.val < val {
		curr.mu.Lock()
		prev.mu.Unlock()
		prev = curr
		curr = curr.next
	}

	if curr != nil && curr.val == val {
		prev.mu.Unlock()
		return false
	}

	newNode.next = curr
	prev.next = newNode
	prev.mu.Unlock()
	return true
}

func (l *LinkedList) delete(val int) bool {
	l.listLock.Lock()

	if l.head == nil {
		l.listLock.Unlock()
		return false
	}

	if l.head.val == val {
		l.head = l.head.next
		l.listLock.Unlock()
		return true
	}

	prev := l.head
	prev.mu.Lock()
	l.listLock.Unlock()

	curr := prev.next

	for curr != nil && curr.val < val {
		curr.mu.Lock()
		prev.mu.Unlock()
		prev = curr
		curr = curr.next
	}

	if curr == nil || curr.val != val {
		prev.mu.Unlock()
		return false
	}

	curr.mu.Lock()
	prev.next = curr.next
	curr.mu.Unlock()
	prev.mu.Unlock()
	return true
}

func (l *LinkedList) printList() {
	l.listLock.Lock()
	defer l.listLock.Unlock()
	curr := l.head
	fmt.Print("[ ")
	for curr != nil {
		fmt.Printf("%d ", curr.val)
		curr = curr.next
	}
	fmt.Println("]")
}

type tArg struct {
	list *LinkedList
	id   int
}

func worker(arg tArg, wg *sync.WaitGroup) {
	defer wg.Done()

	for i := 0; i < 20; i++ {
		val := rand.Intn(10)
		op := rand.Intn(3)

		switch op {
		case 0:
			result := arg.list.insert(val)
			if result {
				fmt.Printf("[T-%d] insert %d -> ok\n", arg.id, val)
			} else {
				fmt.Printf("[T-%d] insert %d -> vec postoji\n", arg.id, val)
			}
		case 1:
			result := arg.list.contains(val)
			if result {
				fmt.Printf("[T-%d] contains %d -> nasao\n", arg.id, val)
			} else {
				fmt.Printf("[T-%d] contains %d -> nije\n", arg.id, val)
			}
		case 2:
			result := arg.list.delete(val)
			if result {
				fmt.Printf("[T-%d] delete %d -> obrisan\n", arg.id, val)
			} else {
				fmt.Printf("[T-%d] delete %d -> nije nadjen\n", arg.id, val)
			}
		}
	}
}

func main() {
	list := &LinkedList{}

	list.insert(2)
	list.insert(5)
	list.insert(7)

	fmt.Print("Pocetna lista: ")
	list.printList()

	var wg sync.WaitGroup
	for i := 0; i < N; i++ {
		wg.Add(1)
		go worker(tArg{list: list, id: i}, &wg)
	}

	wg.Wait()

	fmt.Print("Krajnja lista: ")
	list.printList()
}
