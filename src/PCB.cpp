#include "PCB.h"
#include "kernel.h"
#include "SCHEDULE.H"
#include <dos.h>

ID PCB::nextID = 0;

PCB::PCB(StackSize stackSize, Time tSlice, Thread* thread) {
	id = nextID++;
	timeSlice = tSlice;
	myThread = thread;
	state = NEW;
	semStatus = 0;
	stack = 0;
	sp = ss = bp = 0;
	if (thread != 0)
		initStack(stackSize);
	allPCBs()->add(this);
}

PCB::~PCB() {
	allPCBs()->remove(this);
	delete[] stack;
}

void PCB::initStack(StackSize stackSize) {
	if (stackSize > maxStackSize)
		stackSize = maxStackSize;
	unsigned words = (unsigned)(stackSize / sizeof(unsigned));
	stack = new unsigned[words];
	// iret frame for the first switch: FLAGS (IF=1), CS, IP
	stack[words - 1] = 0x200;
	stack[words - 2] = FP_SEG(PCB::wrapper);
	stack[words - 3] = FP_OFF(PCB::wrapper);
	// room for the 9 registers the interrupt epilogue pops
	sp = FP_OFF(stack + words - 12);
	ss = FP_SEG(stack + words - 12);
	bp = sp;
}

void PCB::wrapper() {
	running->myThread->run();
	lock();
	running->state = FINISHED;
	running->waitingList.wakeAll();
	dispatch();
}

void PCB::wakeUp() {
	state = READY;
	Scheduler::put(this);
}
