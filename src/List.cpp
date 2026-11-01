#include "List.h"
#include "PCB.h"

List::List() {
	head = 0;
	tail = 0;
}

List::~List() {
	while (head != 0) {
		Node* next = head->next;
		delete head;
		head = next;
	}
	tail = 0;
}

void List::add(PCB* pcb) {
	Node* node = new Node;
	node->pcb = pcb;
	node->next = 0;
	if (head == 0)
		head = node;
	else
		tail->next = node;
	tail = node;
}

void List::remove(PCB* pcb) {
	Node* prev = 0;
	Node* cur = head;
	while (cur != 0 && cur->pcb != pcb) {
		prev = cur;
		cur = cur->next;
	}
	if (cur == 0)
		return;
	if (prev == 0)
		head = cur->next;
	else
		prev->next = cur->next;
	if (cur == tail)
		tail = prev;
	delete cur;
}

PCB* List::find(ID id) const {
	for (Node* cur = head; cur != 0; cur = cur->next)
		if (cur->pcb->id == id)
			return cur->pcb;
	return 0;
}

int List::isEmpty() const {
	return head == 0;
}

void List::wakeAll() {
	while (head != 0) {
		Node* next = head->next;
		head->pcb->wakeUp();
		delete head;
		head = next;
	}
	tail = 0;
}
