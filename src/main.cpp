#include "kernel.h"
#include "PCB.h"
#include "IdleTh.h"

extern int userMain(int argc, char* argv[]);

int main(int argc, char* argv[]) {
	running = new PCB(); // the main thread, already executing
	running->state = PCB::READY;

	Idle* idle = new Idle();
	idlePCB = allPCBs()->find(idle->getId());

	initTimer();
	int ret = userMain(argc, argv);
	restoreTimer();

	delete idle;
	delete running;
	// allPCBs()/allSems() stay alive for static destructors
	return ret;
}
