# Konkurentno programiranje u Go jeziku

Primjeri sa vježbi iz predmeta Konkurentno i distribuirano programiranje, prevedeni iz C u Go.

## Struktura projekta

| Folder | Tema |
|---|---|
| 01_PocetniPrimer | Race condition - counter bez zaštite |
| 02_DrugiPrimer | Mutex - zaštita kritične sekcije |
| 03_RW_no_starving | Readers-Writers bez starvation-a |
| 04_Cigarette_Smokers | Problem pušača |
| 04_Philosophers | Dining Philosophers - asimetrično rješenje |
| 04_Sleeping_Barber | Sleeping Barber |
| 05_Read_Write_Delete | Fine-grained locking na linked listi |
| 06_H2O | H2O - formiranje molekula vode |
| 06_River_Crossing | Prelaz rijeke čamcem |
| 07_Laboratorija | Laboratorija sa policom za kontrolu kvaliteta |
| 07_Oblizazak_Stabla | Konkurentni obilazak stabla |
| 07_Studentske_Konsultacije | Studentske konsultacije (FIFO) |
| 08_Radionica | Radionica - varijanta laboratorije |
| 08_Red_Na_Salteru | Red na šalteru (FIFO) |
| 09_Rok1_Jednosmeri_Most | Jednosmerni most |
| 09_Rok2_Studenti_U_Menzi | Studenti u menzi |

## Kako pokrenuti

Instalirati Go: https://go.dev/dl

```bash
cd 01_PocetniPrimer
go run main.go
```

## Ključne razlike C vs Go

Pogledati `C_vs_Go.md` za detaljna poređenja.
