<h1 align="center">OS1 Kernel</h1>

<p align="center">
  Malo višenitno jezgro za 8086 u C++-u: niti, deljenje vremena, semafori sa vremenski ograničenim čekanjem i događaji vezani za hardverske prekide.
</p>

<p align="center">
  <a href="README.md">English</a> | <b>Srpski</b>
</p>

<p align="center">
  <a href="#prevođenje">Prevođenje</a> ·
  <a href="#pokretanje">Pokretanje</a> ·
  <a href="#korišćenje">Korišćenje</a> ·
  <a href="#dizajn">Dizajn</a> ·
  <a href="#testovi">Testovi</a> ·
  <a href="#struktura-projekta">Struktura projekta</a>
</p>

---

Ovo je projekat iz predmeta Operativni sistemi 1 na Elektrotehničkom fakultetu Univerziteta u Beogradu. Zadatak traži podsistem jezgra za upravljanje nitima koji radi kao jedan DOS program u realnom režimu zajedno sa korisničkim kodom: jezgro i aplikacija se prevode i povezuju u jedan izvršni fajl, a sve niti dele isti adresni prostor. Jezgro obezbeđuje niti sa sopstvenim stekovima, promenu konteksta na zahtev, po isteku vremenskog intervala i na prekid, brojačke semafore čije čekanje može da istekne, i događaje na kojima nit blokira dok ne stigne izabrani hardverski prekid. Raspoređivač niti dolazi uz predmet kao biblioteka i nije deo ovog repozitorijuma. Drugi zadatak iz postavke, asinhroni signali između niti, nije realizovan.

## Prevođenje

Kod je namenjen prevodiocu Borland C++ 3.1 pod DOS-om. Oslanja se na `interrupt` funkcije i ugrađeni asembler tog prevodioca, pa se ne može prevesti modernim prevodiocem. DOSBox izvršava ceo alat bez problema na savremenom računaru.

1. Napraviti projekat sa memorijskim modelom **Huge**. Model je bitan: svaka funkcija tada na ulasku iznova učitava `DS`, što omogućava da nova nit krene sa steka na kome su registri još uvek nedefinisani.
2. Dodati sve `.cpp` fajlove iz `src/` i `public test/`, kao i `scheduler/APPLICAT.LIB`. U dijalogu za izbor fajlova promeniti filter na `*.*` da bi biblioteka bila vidljiva.
3. Dodati `h/`, `scheduler/` i `public test/` u putanje za zaglavlja.
4. Build All. Izvršni fajl dobija ime fajla projekta.

Korisnički program koji se povezuje sa jezgrom mora da definiše dve funkcije: `int userMain(int argc, char* argv[])`, koju jezgro poziva kao glavnu nit kada postavi tajmer, i `void tick()`, koju jezgro poziva na svaki prekid tajmera. Javni test definiše obe.

## Pokretanje

Javni test je sistem proizvođača i potrošača. Prima tri argumenta.

```
TEST.EXE <bufferSize> <producers> <timeSlice>
```

| Argument | Značenje |
|---|---|
| `bufferSize` | Kapacitet ograničenog bafera u znakovima; mora biti bar jednak broju proizvođača |
| `producers` | Broj niti proizvođača, od 1 do 19 |
| `timeSlice` | Vremenski interval svakog proizvođača u periodama tajmera od 55 ms; `0` znači da nit nikad ne gubi procesor zbog isteka vremena |

Svaki proizvođač stalno upisuje svoju cifru u bafer, nit za tastaturu upisuje znak svakog pritisnutog tastera, a jedan potrošač uzima znakove i ispisuje ih. Pritisak na Escape zaustavlja proizvođače; potrošač isprazni ostatak bafera, ispiše `Happy End` i program se vraća u DOS. Postavka testira program u dve varijante, sa `timeSlice` 0 i sa `timeSlice` 2.

```
> TEST.EXE 10 3 2
KeyebordListener started!
0 1 2 0 1 2 0 1 2 a b c 0 1 2 ...
ESC pressed - empty the buffer!
0 1 2
Happy End

KeyebordListener stopped!
userMain finished!
```

## Korišćenje

Korisnički program vidi tri klase, svaku u zaglavlju čiji je tekst zadat postavkom: `Thread` u `thread.h`, `Semaphore` u `semaphor.h` i `Event` u `event.h`. Sve ostalo u `h/` je unutrašnjost jezgra.

**Niti.** Nit je objekat klase izvedene iz `Thread` koja redefiniše `run()`. Stvaranje objekta stvara nit; `start()` je stavlja u red spremnih; nit se završava kada se `run()` vrati. Konstruktor prima veličinu steka u bajtovima, podrazumevano 4096 i najviše 64000, i vremenski interval u periodama tajmera, podrazumevano 2, gde 0 znači neograničen.

```cpp
#include "thread.h"

class Worker : public Thread {
public:
	Worker(int count) : Thread(defaultStackSize, 1), n(count) {}
	virtual ~Worker() { waitToComplete(); }
protected:
	virtual void run() {
		for (int i = 0; i < n; i++)
			dispatch(); // dobrovoljno ustupi procesor
	}
private:
	int n;
};

int userMain(int argc, char* argv[]) {
	Worker* w = new Worker(10);
	w->start();
	delete w; // destruktor čeka da se run() završi
	return 0;
}
```

| Član | Dejstvo |
|---|---|
| `start()` | Nit postaje spremna; nema dejstva ako je već pokrenuta |
| `waitToComplete()` | Blokira pozivaoca dok se `run()` ove niti ne završi; odmah se vraća za tekuću nit, završenu nit, ili nit koja nikad nije pokrenuta |
| `getId()`, `getRunningId()` | Identifikator niti i identifikator tekuće niti; dodeljuju se od 0 redom stvaranja, pa je glavna nit 0, a nit za uposleno čekanje 1 |
| `getThreadById(id)` | `Thread` sa tim identifikatorom, ili 0 ako ne postoji |
| `dispatch()` | Slobodna funkcija: ustupa procesor i raspoređivač bira sledeću nit, moguće i istu |

Izvedena klasa treba da pozove `waitToComplete()` u svom destruktoru, kao u primeru, jer se `~Thread` izvršava tek pošto je izvedeni deo objekta već uništen, pa bi `run()` koji još traje pristupao polurazrušenom objektu.

**Semafori.** `Semaphore(init)` je brojački semafor sa uobičajenim `wait` i `signal`, pri čemu obe operacije prema postavci primaju ceo broj.

| Poziv | Dejstvo |
|---|---|
| `wait(maxTimeToWait)` | Umanji vrednost; blokiraj ako je postala negativna. `maxTimeToWait` je granica u periodama tajmera, 0 za neograničeno čekanje. Vraća 1 ako je nit oslobodio `signal`, 0 ako je isteklo vreme |
| `signal(0)` | Uvećaj vrednost i oslobodi jednu blokiranu nit ako je ima; vraća 0 |
| `signal(n)`, n > 0 | Uvećaj vrednost za `n` i oslobodi najviše `n` blokiranih niti; vraća koliko ih je oslobođeno |
| `signal(n)`, n < 0 | Bez dejstva; vraća `n` |
| `val()` | Trenutna vrednost; negativna znači da je toliko niti blokirano |

**Događaji.** Događaj je binarni semafor koji signalizira prekid. Prekidnu rutinu generiše makro `PREPAREENTRY(entry, callOld)` na nivou fajla; `entry` je ulaz u IVT, a `callOld` je 1 ako i dalje treba pozivati staru rutinu tog ulaza. Nit zatim stvara `Event` nad istim ulazom i poziva `wait()`. Samo nit koja je stvorila događaj može da čeka na njemu; `wait()` iz bilo koje druge niti se odmah vraća. Ako prekid stigne dok niko ne čeka, sledeći `wait()` se vraća bez blokiranja.

```cpp
#include "event.h"

PREPAREENTRY(9, 0); // tastatura, zameni BIOS rutinu

void KeyboardThread::run() {
	Event keyPressed(9);
	while (!done) {
		keyPressed.wait();
		// pročitaj skan-kod sa porta 60h i sam pošalji end-of-interrupt,
		// jer BIOS rutina nije pozvana
	}
}
```

## Dizajn

**Niti i PCB.** `Thread` je tanak omotač koji drži pokazivač na `PCB`, zapis jezgra o niti: stek, sačuvane `SS`, `SP` i `BP`, vremenski interval, stanje (`NEW`, `READY`, `BLOCKED`, `FINISHED`), identifikator, zastavicu sa ishodom vremenski ograničenog čekanja i listu niti koje čekaju da se ova završi. `Semaphore` i `Event` na isti način omotavaju `KernelSem` i `KernelEv`. Podela postoji da bi javna zaglavlja ostala tačno onakva kakva ih postavka štampa dok se strana jezgra slobodno menja.

Stek nove niti se priprema tako da izgleda kao da je nit upravo prekinuta: gornje tri reči su `FLAGS` sa dozvoljenim prekidima i `CS:IP` funkcije `PCB::wrapper`; ispod njih je devet reči za registre koje epilog prekidne rutine skida sa steka. Kada raspoređivač prvi put izabere nit, rutina tajmera pređe na taj stek i sa `iret` se vrati pravo u `wrapper`, koji poziva `run()` niti, označava PCB kao završen, budi sve sa njegove liste čekanja i poslednji put traži promenu konteksta. Glavna nit takođe ima PCB, napravljen pre postavljanja tajmera, bez sopstvenog steka jer zadržava onaj koji je DOS dao programu.

**Tajmer i promena konteksta.** Jezgro preuzima `int 08h`, hardverski tajmer, i premešta BIOS rutinu na ulaz `60h` da bi sistemski sat nastavio da radi. Na svaki hardverski otkucaj rutina umanjuje preostale periode tekuće niti, pušta svaki semafor da ostari svoja vremenski ograničena čekanja i poziva `tick()`. Zatim, ako je interval istekao ili je promena eksplicitno zatražena, upisuje `SS:SP:BP` u tekući PCB, vraća nit u raspoređivač ako je još spremna, uzima sledeću, prelazi na nit za uposleno čekanje ako je nema, obnavlja registre i postavlja preostale periode nove niti. Epilog prekidne rutine zatim skida registre nove niti i izvršava `iret` na novom steku. Nit sa vremenskim intervalom 0 nikad ne gubi procesor zbog isteka vremena; radi dok ne blokira, ne pozove `dispatch()` ili dok prekid ne izazove promenu.

`dispatch()` koristi istu rutinu za eksplicitnu promenu: zabrani prekide, postavi zastavicu i direktno pozove `timer()`. Pošto je funkcija deklarisana kao `interrupt`, Borland pre poziva stavlja `FLAGS` na stek, pa je okvir na steku upravo onakav kakav `iret` očekuje. Zastavica govori rutini da ne broji otkucaj i da ne poziva BIOS rutinu, jer se hardverski prekid nije dogodio.

**Kritične sekcije.** `lock()` i `unlock()` zabranjuju i dozvoljavaju prekide. Svaka operacija jezgra koja dira deljene strukture, dakle svaka lista, red i poziv raspoređivača, izvršava se između njih, pa rutina tajmera nikad ne može da zatekne polupovezan čvor. Sekcije su dugačke svega nekoliko operacija nad pokazivačima i nikad se ne ugnežđuju, pa brojač ugnežđavanja nije potreban. Nit koja blokira to čini pozivom `dispatch()` unutar svoje kritične sekcije; kada se nastavi, nastavlja odmah iza tog poziva i izlazi iz sekcije.

**Semafori i vremenski ograničeno čekanje.** `KernelSem` čuva vrednost i red blokiranih PCB-ova, svaki sa preostalim periodama, gde 0 označava neograničeno čekanje. `wait` umanji vrednost i, ako je postala negativna, označi PCB kao blokiran, stavi ga u red sa granicom i zatraži promenu konteksta. `signal` uzima najstarije niti iz reda, svaku označi kao oslobođenu signalom i učini spremnom. Svaki semafor je upisan u globalnu listu koju rutina tajmera obilazi na svaki otkucaj; svaki umanjuje konačne brojače u svom redu, a nit čiji brojač stigne do nule izlazi iz reda, postaje spremna i ostaje označena kao istekla, dok se vrednost semafora za nju uveća. Rezultat `wait` se čita iz te oznake kada nit ponovo dobije procesor.

**Događaji.** `IVTEntry` predstavlja jedan ulaz tabele prekidnih vektora i čuva staru rutinu i `KernelEv` koji je trenutno vezan za njega, u statičkoj tabeli od 256 ulaza. Objekat i prekidnu rutinu za ulaz generiše `PREPAREENTRY`; konstruktor objekta se izvršava na početku programa, pamti stari vektor i postavlja novu rutinu, a destruktor vraća stari vektor na kraju. Stvaranje `Event`-a stvara `KernelEv` čiji je vlasnik tekuća nit i vezuje ga za ulaz. `wait` blokira vlasnika ako događaj nije na čekanju, ili troši stanje čekanja. `signal`, pozvan iz prekidne rutine, čini blokiranog vlasnika spremnim ili beleži događaj kao nastao, a zatim traži promenu konteksta da bi nit koja čeka na hardver dobila procesor što pre. Rutina prvo poziva staru rutinu, kada se to traži, da je promena konteksta ne bi odložila. Prekid na ulazu bez vezanog događaja se ignoriše.

**Nit za uposleno čekanje.** `Idle` je nit koja u petlji poziva `dispatch()`. Stvara se u `main` i nikad se ne pokreće kroz raspoređivač; rutina tajmera je bira kad god raspoređivač ne vrati ništa, i nikad je ne vraća u raspoređivač.

**Pokretanje i gašenje.** `main` stvara PCB glavne niti i nit za uposleno čekanje, postavlja tajmer, izvršava `userMain`, vraća tajmer i oslobađa nit za uposleno čekanje i glavni PCB. Globalne liste PCB-ova i semafora se stvaraju pri prvoj upotrebi, a ne kao statički objekti, pa `Semaphore` deklarisan na nivou fajla u korisničkom programu radi bez obzira na redosled kojim prevodilac inicijalizuje globalne promenljive; liste se nikad ne oslobađaju, pa takav semafor može da bude uništen i posle povratka iz `main`.

## Testovi

Uz postavku dolazi javni test, u `public test/`, a to je program opisan pod [Pokretanje](#pokretanje). Automatski pokretač ne postoji: prevesti ga, pokrenuti sa nekoliko veličina i vremenskih intervala, kucati dok radi i pritisnuti Escape. Očekivani kraj je `Happy End`, zatim `userMain finished!` i povratak u komandnu liniju. Predmet je projekat ocenjivao i dodatnim tajnim testovima koji nisu dostupni.

| Fajl | Uloga |
|---|---|
| `USER.CPP` | `userMain`, niti proizvođača i potrošača, `tick()` |
| `bounded.h`, `bounded.cpp` | Ograničeni bafer zaštićen sa četiri semafora |
| `keyevent.h`, `keyevent.cpp` | Nit za tastaturu: `PREPAREENTRY(9, 0)`, `Event` nad ulazom 9, čitanje skan-koda |
| `intLock.h`, `intLock.cpp` | Uposleno čekanje oko `cout`, koji nije reentrantan |
| `user.h` | Zastavica `theEnd` koju niti dele |

## Struktura projekta

```
h/thread.h        javna klasa Thread; zadata postavkom
h/semaphor.h      javna klasa Semaphore; zadata postavkom
h/event.h         javna klasa Event i makro PREPAREENTRY
h/PCB.h           zapis jezgra o niti
h/KerSem.h        semafor jezgra
h/KerEv.h         događaj jezgra
h/IVTEntry.h      jedan ulaz tabele prekidnih vektora i tabela ulaza
h/IdleTh.h        nit za uposleno čekanje
h/List.h          lista PCB-ova
h/QUEUE.h         red blokiranih PCB-ova sa preostalim periodama čekanja
h/SemList.h       lista svih semafora koju tajmer stari
h/kernel.h        globalne promenljive jezgra deljene između fajlova
src/globals.cpp   prekidna rutina tajmera, promena konteksta, dispatch, lock i unlock
src/main.cpp      pokretanje i gašenje oko userMain
src/*.cpp         po jedan fajl za svako zaglavlje
scheduler/        SCHEDULE.H i APPLICAT.LIB, raspoređivač sa predmeta
public test/      javni test program sa predmeta
docs/             postavka zadatka i dokumentacija projekta
```

## Poznata ograničenja

- Signali, drugi zadatak iz postavke, nisu realizovani.
- `operator new` i `cout` nisu reentrantni. Jezgro ih poziva samo unutar svojih kritičnih sekcija; korisnički kod koji ih poziva iz više niti mora sam da ih zaštiti, kao što javni test radi sa `intLock`.
- Za jedan ulaz IVT može da bude vezan jedan `Event` u datom trenutku; stvaranje drugog nad istim ulazom prevezuje ulaz na njega.
- Stek niti je ograničen na 64000 bajtova, da bi zajedno sa zaglavljem alokatora stao u jedan segment od 64 KB.
