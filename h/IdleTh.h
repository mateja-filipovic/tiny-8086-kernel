#ifndef _IDLETH_H_
#define _IDLETH_H_

#include "thread.h"

// runs only while every other thread is blocked
class Idle : public Thread {
public:
	Idle();
	virtual void run();
};

#endif
