#ifndef _IVTENTRY_H_
#define _IVTENTRY_H_

typedef unsigned char IVTNo;
typedef void interrupt (*interruptPointer)(...);

class KernelEv;

class IVTEntry {
public:
	IVTEntry(IVTNo entry, interruptPointer newRoutine);
	~IVTEntry();

	void callOldRoutine();
	void signal(); // called from the new interrupt routine

	void setEvent(KernelEv* ev);
	KernelEv* getEvent() const;

	static IVTEntry* getEntry(IVTNo entry);

private:
	IVTNo entry;
	interruptPointer oldRoutine;
	KernelEv* event;

	static IVTEntry* table[256];
};

#endif
