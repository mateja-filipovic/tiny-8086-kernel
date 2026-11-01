#ifndef _KERNEL_H_
#define _KERNEL_H_

class PCB;
class List;
class SemList;

extern PCB* running;
extern PCB* idlePCB;

List* allPCBs();
SemList* allSems();

void lock();
void unlock();
void dispatch();

void initTimer();
void restoreTimer();

#endif
