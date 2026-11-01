#include "QUEUE.h"
#include "PCB.h"

Queue::Queue() {
	head = 0;
	tail = 0;
}

Queue::~Queue() {
	while (head != 0) {
		QNode* next = head->next;
		delete head;
		head = next;
	}
	tail = 0;
}

void Queue::add(PCB* pcb, unsigned maxTicks) {
	QNode* node = new QNode;
	node->pcb = pcb;
	node->ticksLeft = maxTicks;
	node->next = 0;
	if (head == 0)
		head = node;
	else
		tail->next = node;
	tail = node;
}

PCB* Queue::removeFirst() {
	if (head == 0)
		return 0;
	QNode* node = head;
	PCB* pcb = node->pcb;
	head = node->next;
	if (head == 0)
		tail = 0;
	delete node;
	return pcb;
}

int Queue::wakeExpired() {
	int woken = 0;
	QNode* prev = 0;
	QNode* cur = head;
	while (cur != 0) {
		QNode* next = cur->next;
		if (cur->ticksLeft != 0 && --cur->ticksLeft == 0) {
			cur->pcb->wakeUp();
			if (prev == 0)
				head = next;
			else
				prev->next = next;
			if (cur == tail)
				tail = prev;
			delete cur;
			woken++;
		} else {
			prev = cur;
		}
		cur = next;
	}
	return woken;
}

int Queue::isEmpty() const {
	return head == 0;
}
