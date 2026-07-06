# Poređenje C i Go za konkurentno programiranje

## 1. Niti

| C (pthreads) | Go (goroutines) |
|---|---|
| `pthread_t t` | (nema deklaracije) |
| `pthread_create(&t, NULL, fn, arg)` | `go fn(arg)` |
| `pthread_join(t, NULL)` | `wg.Wait()` |
| Teže za kreiranje (~1MB stack) | Lakše (~2KB stack, raste po potrebi) |

```c
// C
pthread_t t;
pthread_create(&t, NULL, posao, NULL);
pthread_join(t, NULL);
```

```go
// Go
var wg sync.WaitGroup
wg.Add(1)
go posao(&wg)  // goroutine
wg.Wait()
```

---

## 2. Mutex

| C | Go |
|---|---|
| `pthread_mutex_t mu` | `var mu sync.Mutex` |
| `pthread_mutex_init(&mu, NULL)` | (automatski inicijalizovan) |
| `pthread_mutex_lock(&mu)` | `mu.Lock()` |
| `pthread_mutex_unlock(&mu)` | `mu.Unlock()` |
| `pthread_mutex_destroy(&mu)` | (GC, nema potrebe) |

```c
// C
pthread_mutex_lock(&mu);
counter++;
pthread_mutex_unlock(&mu);
```

```go
// Go
mu.Lock()
counter++
mu.Unlock()
// ili elegantnije:
mu.Lock()
defer mu.Unlock()
counter++
```

---

## 3. Semafori

Go nema ugrađene semafore - koristimo kanale kapaciteta 1 (binarni) ili N (counting).

| C | Go |
|---|---|
| `sem_t s; sem_init(&s, 0, 1)` | `s := make(chan struct{}, 1); s <- struct{}{}` |
| `sem_wait(&s)` | `<-s` |
| `sem_post(&s)` | `s <- struct{}{}` |

```c
// C - binarni semafor
sem_t s;
sem_init(&s, 0, 1);  // inicijalno otkljucan
sem_wait(&s);        // zakljucaj
// kriticna sekcija
sem_post(&s);        // otkljucaj
```

```go
// Go - kanal kao binarni semafor
s := make(chan struct{}, 1)
s <- struct{}{}  // inicijalno otkljucan

<-s              // sem_wait
// kriticna sekcija
s <- struct{}{}  // sem_post
```

---

## 4. Condition Variable

| C | Go |
|---|---|
| `pthread_cond_t c` | `c := sync.NewCond(&mu)` |
| `pthread_cond_wait(&c, &mu)` | `c.Wait()` |
| `pthread_cond_signal(&c)` | `c.Signal()` |
| `pthread_cond_broadcast(&c)` | `c.Broadcast()` |

```c
// C
pthread_mutex_lock(&mu);
while (!uslov) {
    pthread_cond_wait(&cond, &mu);
}
pthread_mutex_unlock(&mu);
```

```go
// Go
mu.Lock()
for !uslov {
    cond.Wait()  // automatski otpusta mu dok ceka
}
mu.Unlock()
```

---

## 5. WaitGroup (nema ekvivalenta u C)

Go ima `sync.WaitGroup` - elegantan nacin da sacekamo vise goroutina:

```go
var wg sync.WaitGroup

for i := 0; i < 5; i++ {
    wg.Add(1)
    go func() {
        defer wg.Done()
        // posao...
    }()
}

wg.Wait()  // cekaj da svih 5 zavrsi
```

U C bi ovo bio `pthread_join` u petlji.

---

## 6. Fajlovi

| C | Go |
|---|---|
| `FILE *f = fopen("f.txt", "w")` | `f, err := os.Create("f.txt")` |
| `fprintf(f, "tekst %d\n", n)` | `fmt.Fprintf(f, "tekst %d\n", n)` |
| `fclose(f)` | `f.Close()` |
| `fflush(stdout)` | (automatski u Go) |

---

## 7. Ključne prednosti Go-a

1. **Goroutine su jeftinije** - možeš pokrenuti hiljade, u C pthread je težak
2. **Nema sem_init/sem_destroy** - kanali su automatski
3. **defer** - garantuje unlock čak i ako dođe do greške
4. **Race detector** - `go run -race main.go` automatski detektuje race conditions!
5. **Garbage collector** - nema malloc/free, nema memory leaks
