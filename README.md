Ciao! Sono un ragazzo di 17 anni e questo è il mio progetto didattico scritto in C mentre seguo il corso di **Salvatore Sanfilippo (antirez)**. 

L'obiettivo di questo progetto è imparare come funziona un interprete, concentrandomi sulla gestione della memoria e sulla logica degli stack.

## Caratteristiche principali
- **Linguaggio a Stack:** Ispirato a Forth, i dati vengono manipolati su uno stack centrale.
- **Reference Counting:** Gestione manuale della memoria con `retain` e `release` per prevenire memory leak.
- **Tipi supportati:** Interi, Stringhe, Booleani e Liste.
- **Funzioni Utente:** Supporto per la definizione di funzioni personalizzate e **ricorsione**.
- **Parser con Error Reporting:** Il compilatore indica la colonna in caso di errore di sintassi.

## Esempio di Codice
Ecco come appare un programma nel mio linguaggio:

file testo:
'num[12] 'fact[dup 1 = ["finito" print] [dup 1 - fact * print] if] num fact

output:
-------------------------INIZIO PROGRAMMA----------------------------                                                                                 
list: 1 = [int: 12] -> Compiled
list: 2 = [str: finito, sym: print] -> Compiled
list: 6 = [sym: dup, int: 1, sym: -, sym: fact, sym: *, sym: print] -> Compiled
list: 6 = [sym: dup, int: 1, sym: =, list: 2 = [str: finito, sym: print], list: 6 = [sym: dup, int: 1, sym: -, sym: fact, sym: *, sym: print], sym: if] -> Compiled
list: 2 = [sym: num, sym: fact] -> Compiled

Exec:
str: finito
int: 2
int: 6
int: 24
int: 120
int: 720
int: 5040
int: 40320
int: 362880
int: 3628800
int: 39916800
int: 479001600
Code: 0

-------------------------FINE PROGRAMMA----------------------------
