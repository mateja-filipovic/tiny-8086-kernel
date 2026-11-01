#include "IVTEntry.h"
#include "KerEv.h"
#include "kernel.h"
#include <dos.h>

IVTEntry* IVTEntry::table[256];

IVTEntry::IVTEntry(IVTNo entry_, interruptPointer newRoutine) {
	entry = entry_;
	event = 0;
	lock();
	oldRoutine = getvect(entry);
	setvect(entry, newRoutine);
	table[entry] = this;
	unlock();
}

IVTEntry::~IVTEntry() {
	lock();
	setvect(entry, oldRoutine);
	table[entry] = 0;
	unlock();
}

void IVTEntry::callOldRoutine() {
	oldRoutine();
}

void IVTEntry::signal() {
	if (event != 0)
		event->signal();
}

void IVTEntry::setEvent(KernelEv* ev) {
	event = ev;
}

KernelEv* IVTEntry::getEvent() const {
	return event;
}

IVTEntry* IVTEntry::getEntry(IVTNo entry) {
	return table[entry];
}
