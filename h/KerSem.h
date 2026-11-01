#ifndef _KERSEM_H_
#define _KERSEM_H_

#include "thread.h"
#include "QUEUE.h"

class KernelSem {
public:
	KernelSem(int init = 1);
	~KernelSem();

	int wait(Time maxTimeToWait);
	int signal(int n = 0);
	int value() const;

	void update(); // one timer tick

private:
	int val;
	Queue blocked;
};

#endif
