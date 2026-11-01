#ifndef _QUEUE_H_
#define _QUEUE_H_

class PCB;

struct QNode {
	PCB* pcb;
	unsigned ticksLeft;
	QNode* next;
};

class Queue {
public:
	Queue();
	~Queue();

	void add(PCB* pcb, unsigned maxTicks); // maxTicks 0: wait forever
	PCB* removeFirst();
	int wakeExpired();
	int isEmpty() const;

private:
	QNode* head;
	QNode* tail;
};

#endif
