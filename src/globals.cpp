#include "kernel.h"
#include "PCB.h"
#include "List.h"
#include "SemList.h"
#include "SCHEDULE.H"

extern void tick(); // provided by the test environment

PCB* running = 0;
PCB* idlePCB = 0;

static List* pcbList = 0;
static SemList* semList = 0;

List* allPCBs() {
	if (pcbList == 0)
		pcbList = new List();
	return pcbList;
}

SemList* allSems() {
	if (semList == 0)
		semList = new SemList();
	return semList;
}

void lock() {
	asm cli;
}

void unlock() {
	asm sti;
}

volatile int ticksLeft = 0;
volatile int contextSwitchRequested = 0;

unsigned tsp, tss, tbp;
unsigned oldTimerOFF, oldTimerSEG;

void interrupt timer() {
	if (!contextSwitchRequested) {
		ticksLeft--;
		allSems()->update();
		tick();
	}

	if (contextSwitchRequested || (running->timeSlice != 0 && ticksLeft <= 0)) {
		asm {
			mov tsp, sp
			mov tss, ss
			mov tbp, bp
		}
		running->sp = tsp;
		running->ss = tss;
		running->bp = tbp;

		if (running->state == PCB::READY && running != idlePCB)
			Scheduler::put(running);
		running = Scheduler::get();
		if (running == 0)
			running = idlePCB;

		tsp = running->sp;
		tss = running->ss;
		tbp = running->bp;
		ticksLeft = running->timeSlice;
		asm {
			mov sp, tsp
			mov ss, tss
			mov bp, tbp
		}
	}

	// old timer routine, moved to int 60h by initTimer()
	if (!contextSwitchRequested)
		asm int 60h;
	contextSwitchRequested = 0;
}

void dispatch() {
	asm cli;
	contextSwitchRequested = 1;
	timer();
	asm sti;
}

void initTimer() {
	ticksLeft = running->timeSlice;
	asm {
		cli
		push es
		push ax

		mov ax, 0
		mov es, ax

		mov ax, word ptr es:0022h
		mov word ptr oldTimerSEG, ax
		mov ax, word ptr es:0020h
		mov word ptr oldTimerOFF, ax

		mov word ptr es:0022h, seg timer
		mov word ptr es:0020h, offset timer

		mov ax, oldTimerSEG
		mov word ptr es:0182h, ax
		mov ax, oldTimerOFF
		mov word ptr es:0180h, ax

		pop ax
		pop es
		sti
	}
}

void restoreTimer() {
	asm {
		cli
		push es
		push ax

		mov ax, 0
		mov es, ax

		mov ax, word ptr oldTimerSEG
		mov word ptr es:0022h, ax
		mov ax, word ptr oldTimerOFF
		mov word ptr es:0020h, ax

		pop ax
		pop es
		sti
	}
}
