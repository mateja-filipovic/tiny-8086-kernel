#include "IdleTh.h"

Idle::Idle() : Thread(defaultStackSize, 1) {}

void Idle::run() {
	for (;;)
		dispatch();
}
