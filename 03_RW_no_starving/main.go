/*
 * Sinhronizacija:
 * - mu:      stiti read_count
 * - rwLock:  stiti pristup resursu (jedan writer ili vise readera)
 * - red:     sprecava writer starvation
 *
 * -----------------------------------------------------------
 * C vs Go:
 *   sem_wait(&s)  =  uzmi token  =  <-s         (prima iz kanala)
 *   sem_post(&s)  =  vrati token =  s <- struct{}{}  (salje u kanal)
 *
 *   Kanal kapaciteta 1, inicijalno sa jednom vrednoscu = otkljucan semafor
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
	READER_COUNT = 5
	WRITER_COUNT = 2
	READER_LOOPS = 3
	WRITER_LOOPS = 3
)

var readCount int
var mu sync.Mutex

var rwLock = make(chan struct{}, 1)
var red = make(chan struct{}, 1)

func reader(id int, wg *sync.WaitGroup) {
	defer wg.Done()

	for i := 0; i < READER_LOOPS; i++ {
		// Ulazak u red
		<-red             // sem_wait(&red)  - uzmi token
		red <- struct{}{} // sem_post(&red)  - odmah vrati (samo provera da writer ne ceka)

		mu.Lock()
		readCount++
		if readCount == 1 {
			<-rwLock // sem_wait(&rwLock) - prvi reader blokira writere
		}
		mu.Unlock()

		// KRITICNA SEKCIJA ZA CITANJE
		fmt.Printf("[Reader %d] poceo citanje (Trenutno citaoca: %d)\n", id, readCount)
		time.Sleep(time.Duration(50+rand.Intn(100)) * time.Millisecond)
		fmt.Printf("[Reader %d] zavrsio citanje\n", id)

		mu.Lock()
		readCount--
		if readCount == 0 {
			rwLock <- struct{}{} // sem_post(&rwLock) - poslednji reader pusta writere
		}
		mu.Unlock()

		time.Sleep(time.Duration(100+rand.Intn(300)) * time.Millisecond)
	}
}

func writer(id int, wg *sync.WaitGroup) {
	defer wg.Done()

	for i := 0; i < WRITER_LOOPS; i++ {
		// Writer zauzima red - novi readeri moraju da cekaju
		<-red             // sem_wait(&red)   - zauzmi red
		<-rwLock          // sem_wait(&rwLock) - cekaj da readeri zavrse
		red <- struct{}{} // sem_post(&red)   - pusti red tek kad dobijemo rwLock

		// KRITICNA SEKCIJA ZA PISANJE
		fmt.Printf("[Writer %d] poceo pisanje...\n", id)
		time.Sleep(time.Duration(80+rand.Intn(150)) * time.Millisecond)
		fmt.Printf("[Writer %d] zavrsio pisanje.\n", id)

		rwLock <- struct{}{} // sem_post(&rwLock)

		time.Sleep(time.Duration(150+rand.Intn(350)) * time.Millisecond)
	}
}

func main() {
	// Inicijalizacija - stavi jedan token da budu "otkljucani"
	rwLock <- struct{}{}
	red <- struct{}{}

	var wg sync.WaitGroup

	for i := 1; i <= READER_COUNT; i++ {
		wg.Add(1)
		go reader(i, &wg)
	}
	for i := 1; i <= WRITER_COUNT; i++ {
		wg.Add(1)
		go writer(i, &wg)
	}

	wg.Wait()
	fmt.Println("\n[USPEH] Svi readeri i writeri su uspesno zavrsili rad!")
}
