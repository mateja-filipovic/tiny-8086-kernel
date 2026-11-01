#ifndef _PCB_H_
#define _PCB_H_

#include "thread.h"
#include "List.h"

const StackSize maxStackSize = 64000;

class PCB {
public:
	enum State { NEW, READY, BLOCKED, FINISHED };

	PCB(StackSize stackSize = defaultStackSize, Time timeSlice = defaultTimeSlice, Thread* thread = 0);
	~PCB();

	static void wrapper();
	void wakeUp();

	unsigned* stack;
	unsigned sp, ss, bp;

	Time timeSlice;
	Thread* myThread;
	List waitingList;
	ID id;
	State state;
	int semStatus; // 1 signaled, 0 timed out

	static ID nextID;

private:
	void initStack(StackSize stackSize);
};

#endif
