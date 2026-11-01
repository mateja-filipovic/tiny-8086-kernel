#include "thread.h"
#include "PCB.h"
#include "kernel.h"
#include "SCHEDULE.H"

Thread::Thread(StackSize stackSize, Time timeSlice) {
	lock();
	myPCB = new PCB(stackSize, timeSlice, this);
	unlock();
}

Thread::~Thread() {
	waitToComplete();
	lock();
	delete myPCB;
	unlock();
}

void Thread::start() {
	lock();
	if (myPCB->state == PCB::NEW) {
		myPCB->state = PCB::READY;
		Scheduler::put(myPCB);
	}
	unlock();
}

void Thread::waitToComplete() {
	lock();
	if (myPCB == running || myPCB->state == PCB::NEW || myPCB->state == PCB::FINISHED) {
		unlock();
		return;
	}
	running->state = PCB::BLOCKED;
	myPCB->waitingList.add(running);
	dispatch();
	unlock();
}

ID Thread::getId() {
	return myPCB->id;
}

ID Thread::getRunningId() {
	return running->id;
}

Thread* Thread::getThreadById(ID id) {
	PCB* pcb = allPCBs()->find(id);
	return pcb != 0 ? pcb->myThread : 0;
}
