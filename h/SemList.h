#ifndef _SEMLIST_H_
#define _SEMLIST_H_

class KernelSem;

struct SNode {
	KernelSem* sem;
	SNode* next;
};

class SemList {
public:
	SemList();
	~SemList();

	void add(KernelSem* sem);
	void remove(KernelSem* sem);
	void update();

private:
	SNode* head;
	SNode* tail;
};

#endif
