#ifndef _LIST_H_
#define _LIST_H_

#include "thread.h"

class PCB;

struct Node {
	PCB* pcb;
	Node* next;
};

class List {
public:
	List();
	~List();

	void add(PCB* pcb);
	void remove(PCB* pcb);
	PCB* find(ID id) const;
	void wakeAll();
	int isEmpty() const;

private:
	Node* head;
	Node* tail;
};

#endif
