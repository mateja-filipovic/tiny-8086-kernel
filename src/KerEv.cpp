#include "KerEv.h"
#include "PCB.h"
#include "kernel.h"

KernelEv::KernelEv(IVTNo ivtNo_) {
	owner = running;
	blocked = 0;
	ivtNo = ivtNo_;
	val = 0;
	IVTEntry* entry = IVTEntry::getEntry(ivtNo);
	if (entry != 0)
		entry->setEvent(this);
}

KernelEv::~KernelEv() {
	IVTEntry* entry = IVTEntry::getEntry(ivtNo);
	if (entry != 0 && entry->getEvent() == this)
		entry->setEvent(0);
}

void KernelEv::wait() {
	lock();
	if (running == owner) {
		if (val == 0) {
			owner->state = PCB::BLOCKED;
			blocked = owner;
			dispatch();
		} else {
			val = 0;
		}
	}
	unlock();
}

void KernelEv::signal() {
	lock();
	if (blocked != 0) {
		PCB* pcb = blocked;
		blocked = 0;
		pcb->wakeUp();
	} else {
		val = 1;
	}
	unlock();
	dispatch();
}
