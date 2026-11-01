#ifndef _KEREV_H_
#define _KEREV_H_

#include "IVTEntry.h"

class PCB;

class KernelEv {
public:
	KernelEv(IVTNo ivtNo); // owner is the thread creating it
	~KernelEv();

	void wait();
	void signal();

private:
	PCB* owner;
	PCB* blocked; // owner while it waits, otherwise 0
	IVTNo ivtNo;
	int val;
};

#endif
