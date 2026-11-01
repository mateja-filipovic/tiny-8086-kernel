#include "KerSem.h"
#include "PCB.h"
#include "SemList.h"
#include "kernel.h"

KernelSem::KernelSem(int init) {
	val = init;
	allSems()->add(this);
}

KernelSem::~KernelSem() {
	allSems()->remove(this);
}

int KernelSem::wait(Time maxTimeToWait) {
	int result = 1;
	lock();
	if (--val < 0) {
		running->semStatus = 0;
		running->state = PCB::BLOCKED;
		blocked.add(running, maxTimeToWait);
		dispatch();
		result = running->semStatus;
	}
	unlock();
	return result;
}

int KernelSem::signal(int n) {
	if (n < 0)
		return n;
	int toWake = (n == 0) ? 1 : n;
	int woken = 0;
	lock();
	while (woken < toWake && val < 0) {
		PCB* pcb = blocked.removeFirst();
		if (pcb == 0)
			break;
		pcb->semStatus = 1;
		pcb->wakeUp();
		val++;
		woken++;
	}
	val += toWake - woken;
	unlock();
	return (n == 0) ? 0 : woken;
}

int KernelSem::value() const {
	return val;
}

void KernelSem::update() {
	val += blocked.wakeExpired();
}
