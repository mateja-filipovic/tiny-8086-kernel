#include "SemList.h"
#include "KerSem.h"

SemList::SemList() {
	head = 0;
	tail = 0;
}

SemList::~SemList() {
	while (head != 0) {
		SNode* next = head->next;
		delete head;
		head = next;
	}
	tail = 0;
}

void SemList::add(KernelSem* sem) {
	SNode* node = new SNode;
	node->sem = sem;
	node->next = 0;
	if (head == 0)
		head = node;
	else
		tail->next = node;
	tail = node;
}

void SemList::remove(KernelSem* sem) {
	SNode* prev = 0;
	SNode* cur = head;
	while (cur != 0 && cur->sem != sem) {
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

void SemList::update() {
	for (SNode* cur = head; cur != 0; cur = cur->next)
		cur->sem->update();
}
