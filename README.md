<h1 align="center">OS1 Kernel</h1>

<p align="center">
  A small multithreaded kernel for the 8086 in C++: threads, time sharing, semaphores with timed waits and events bound to hardware interrupts.
</p>

<p align="center">
  <b>English</b> | <a href="README.sr.md">Srpski</a>
</p>

<p align="center">
  <a href="#build">Build</a> ·
  <a href="#run">Run</a> ·
  <a href="#usage">Usage</a> ·
  <a href="#design">Design</a> ·
  <a href="#tests">Tests</a> ·
  <a href="#project-layout">Project layout</a>
</p>

---

This is the project from the Operating Systems 1 course at the School of Electrical Engineering, University of Belgrade. The assignment asks for the thread subsystem of a kernel that runs as a single real-mode DOS program together with the user code: the kernel and the application are compiled and linked into one executable, and every thread shares its address space. The kernel provides threads with their own stacks, context switching on request, on time-slice expiry and on interrupts, counting semaphores whose wait can time out, and events on which a thread blocks until a chosen hardware interrupt fires. The ready-queue scheduler is supplied by the course as a library and is not part of this repository. The second task of the assignment, asynchronous signals between threads, is not implemented.

## Build

The code targets Borland C++ 3.1 under DOS. It relies on the compiler's `interrupt` functions and inline assembly, so it does not build with a modern compiler. DOSBox runs the toolchain fine on a current machine.

1. Create a project with the **Huge** memory model. The model matters: every function then reloads `DS` on entry, which is what lets a freshly created thread start from a stack full of garbage registers.
2. Add every `.cpp` from `src/` and from `public test/`, and add `scheduler/APPLICAT.LIB`. In the file dialog switch the name filter to `*.*` to see the library.
3. Add `h/`, `scheduler/` and `public test/` to the include path.
4. Build All. The executable takes the name of the project file.

Whatever user program is linked in has to define two functions: `int userMain(int argc, char* argv[])`, which the kernel calls as the main thread once the timer is installed, and `void tick()`, which the kernel calls on every timer interrupt. The public test defines both.

## Run

The public test is a producer and consumer system. It takes three arguments.

```
TEST.EXE <bufferSize> <producers> <timeSlice>
```

| Argument | Meaning |
|---|---|
| `bufferSize` | Capacity of the bounded buffer, in characters; must be at least the number of producers |
| `producers` | Number of producer threads, 1 to 19 |
| `timeSlice` | Time slice of each producer in timer ticks of 55 ms; `0` means the thread is never preempted on time |

Each producer repeatedly puts its own digit into the buffer, a keyboard thread puts the character of every key you press, and a single consumer takes characters out and prints them. Pressing Escape stops the producers; the consumer drains what is left, prints `Happy End`, and the program returns to the DOS prompt. The assignment tests the program in two variants, `timeSlice` 0 and `timeSlice` 2.

```
> TEST.EXE 10 3 2
KeyebordListener started!
0 1 2 0 1 2 0 1 2 a b c 0 1 2 ...
ESC pressed - empty the buffer!
0 1 2
Happy End

KeyebordListener stopped!
userMain finished!
```

## Usage

The user program sees three classes, each declared in a header whose text is fixed by the assignment: `Thread` in `thread.h`, `Semaphore` in `semaphor.h` and `Event` in `event.h`. Everything else in `h/` is kernel internals.

**Threads.** A thread is an object of a class derived from `Thread` that overrides `run()`. Creating the object creates the thread; `start()` puts it in the ready queue; the thread ends when `run()` returns. The constructor takes the stack size in bytes, 4096 by default and at most 64000, and the time slice in ticks, 2 by default, where 0 means unlimited.

```cpp
#include "thread.h"

class Worker : public Thread {
public:
	Worker(int count) : Thread(defaultStackSize, 1), n(count) {}
	virtual ~Worker() { waitToComplete(); }
protected:
	virtual void run() {
		for (int i = 0; i < n; i++)
			dispatch(); // give the processor away voluntarily
	}
private:
	int n;
};

int userMain(int argc, char* argv[]) {
	Worker* w = new Worker(10);
	w->start();
	delete w; // the destructor waits for run() to finish
	return 0;
}
```

| Member | Effect |
|---|---|
| `start()` | Make the thread ready; does nothing if it was already started |
| `waitToComplete()` | Block the caller until this thread's `run()` returns; returns at once for the running thread, a finished thread, or one that was never started |
| `getId()`, `getRunningId()` | The thread's id and the id of the running thread; ids are assigned from 0 in creation order, so the main thread is 0 and the idle thread is 1 |
| `getThreadById(id)` | The `Thread` with that id, or 0 if there is none |
| `dispatch()` | Free function: give up the processor and let the scheduler pick the next thread, possibly the same one |

A derived class should call `waitToComplete()` in its destructor, as the example does, because `~Thread` runs after the derived part has already been destroyed and a `run()` still executing at that point would touch a half-destroyed object.

**Semaphores.** `Semaphore(init)` is a counting semaphore with the usual `wait` and `signal`, both extended with an integer as the assignment requires.

| Call | Effect |
|---|---|
| `wait(maxTimeToWait)` | Decrement; block if the value became negative. `maxTimeToWait` is a limit in ticks, 0 for no limit. Returns 1 if released by `signal`, 0 if released because the time ran out |
| `signal(0)` | Increment and release one blocked thread if any; returns 0 |
| `signal(n)`, n > 0 | Increment by `n` and release up to `n` blocked threads; returns how many were released |
| `signal(n)`, n < 0 | No effect; returns `n` |
| `val()` | The current value; negative means that many threads are blocked |

**Events.** An event is a binary semaphore signaled by an interrupt. The interrupt routine is generated by the `PREPAREENTRY(entry, callOld)` macro at file scope; `entry` is the IVT entry and `callOld` is 1 if the previous handler of that entry should still run. A thread then creates an `Event` on the same entry and calls `wait()` on it. Only the thread that created the event can wait on it; a `wait()` from any other thread returns immediately. If the interrupt arrives while nobody waits, the next `wait()` returns without blocking.

```cpp
#include "event.h"

PREPAREENTRY(9, 0); // keyboard, replace the BIOS handler

void KeyboardThread::run() {
	Event keyPressed(9);
	while (!done) {
		keyPressed.wait();
		// read the scancode from port 60h and send the end-of-interrupt yourself,
		// because the BIOS handler was not called
	}
}
```

## Design

**Threads and PCBs.** `Thread` is a thin wrapper that holds a pointer to a `PCB`, the kernel's record of the thread: the stack, the saved `SS`, `SP` and `BP`, the time slice, the state (`NEW`, `READY`, `BLOCKED`, `FINISHED`), the id, a result flag for timed waits and a list of threads waiting for this one to finish. `Semaphore` and `Event` wrap `KernelSem` and `KernelEv` the same way. The split exists so the public headers can stay exactly as the assignment prints them while the kernel side changes freely.

A new thread's stack is prepared to look as if the thread had just been interrupted: the top three words are `FLAGS` with interrupts enabled, and the `CS:IP` of `PCB::wrapper`; below them are nine words for the registers the compiler's interrupt epilogue pops. The first time the scheduler picks the thread, the timer routine switches to that stack and returns with `iret` straight into `wrapper`, which calls the thread's `run()`, marks the PCB finished, wakes everything on its waiting list and dispatches for the last time. The main thread has a PCB too, created before the timer is installed, with no stack of its own since it keeps the one DOS gave the program.

**Timer and context switch.** The kernel takes over `int 08h`, the hardware timer, and moves the BIOS routine to entry `60h` so the system clock keeps running. On each hardware tick the routine decrements the running thread's remaining ticks, lets every semaphore age its timed waits and calls `tick()`. Then, if the slice is spent or a switch was requested explicitly, it saves `SS:SP:BP` into the running PCB, puts the thread back into the scheduler if it is still ready, takes the next one, falls back to the idle thread if there is none, restores the registers and sets the new thread's remaining ticks. The compiler's interrupt epilogue then pops the new thread's registers and executes `iret` on the new stack. A thread with time slice 0 is never preempted on time; it runs until it blocks, dispatches or an interrupt causes a switch.

`dispatch()` reuses the same routine for an explicit switch: it disables interrupts, sets a flag and calls `timer()` directly. Since the function is declared `interrupt`, Borland pushes the flags before the call, so the frame on the stack is the one `iret` expects. The flag tells the routine not to count a tick and not to chain to the BIOS handler, because no hardware interrupt happened.

**Critical sections.** `lock()` and `unlock()` disable and enable interrupts. Every kernel operation that touches shared structures, which means every list, queue and scheduler call, runs between them, so the timer routine can never see a half-linked node. The sections are a handful of pointer operations long and are never nested, so there is no need for a nesting counter. A thread that blocks does so by calling `dispatch()` inside its critical section; when it is resumed, it continues right after that call and unlocks.

**Semaphores and timed waits.** `KernelSem` keeps the value and a queue of blocked PCBs, each with its remaining ticks, where 0 stands for an unlimited wait. `wait` decrements the value and, if it went negative, marks the PCB blocked, enqueues it with the limit and dispatches. `signal` dequeues the oldest threads, marks each as released by a signal, and makes them ready. Every semaphore is registered in a global list that the timer routine walks on each tick; each one decrements the finite counters in its queue, and a thread whose counter reaches zero is removed from the queue, made ready and left marked as timed out, while the value is incremented for it. The result of `wait` is read from that mark once the thread runs again.

**Events.** `IVTEntry` represents one entry of the interrupt vector table and keeps the old handler and the `KernelEv` currently bound to it, in a static table of 256 entries. The object and the interrupt routine for an entry are generated by `PREPAREENTRY`; the object's constructor runs at program start, saves the old vector and installs the new routine, and its destructor restores the old vector at exit. Creating an `Event` creates a `KernelEv` owned by the running thread and binds it to the entry. `wait` blocks the owner if the event is not pending, or consumes the pending state. `signal`, called from the interrupt routine, makes the blocked owner ready or records the event as pending, and then dispatches so a thread waiting on hardware gets the processor as soon as possible. The routine runs the old handler first, when asked to, so that the dispatch cannot delay it. An interrupt on an entry with no bound event is ignored.

**Idle thread.** `Idle` is a thread that loops on `dispatch()`. It is created in `main` and never started through the scheduler; the timer routine picks it whenever the scheduler returns nothing, and never puts it back into the scheduler.

**Startup and shutdown.** `main` creates the main thread's PCB and the idle thread, installs the timer, runs `userMain`, restores the timer, and frees the idle thread and the main PCB. The global lists of PCBs and semaphores are created on first use rather than as static objects, so a `Semaphore` declared at file scope in the user program works regardless of the order in which the compiler initializes globals, and they are never freed, so such a semaphore can also be destroyed after `main` returns.

## Tests

The assignment ships a public test, in `public test/`, which is the program described under [Run](#run). There is no automated runner: build it, run it with a few sizes and time slices, type while it runs, and press Escape. The expected ending is `Happy End` followed by `userMain finished!` and a return to the prompt. The course graded the project with additional private tests that are not available.

| File | Role |
|---|---|
| `USER.CPP` | `userMain`, the producer and consumer threads, `tick()` |
| `bounded.h`, `bounded.cpp` | Bounded buffer guarded by four semaphores |
| `keyevent.h`, `keyevent.cpp` | Keyboard thread: `PREPAREENTRY(9, 0)`, an `Event` on entry 9, scancode reading |
| `intLock.h`, `intLock.cpp` | Spin lock around `cout`, which is not reentrant |
| `user.h` | The `theEnd` flag shared by the threads |

## Project layout

```
h/thread.h        public Thread class; fixed by the assignment
h/semaphor.h      public Semaphore class; fixed by the assignment
h/event.h         public Event class and the PREPAREENTRY macro
h/PCB.h           kernel record of a thread
h/KerSem.h        kernel semaphore
h/KerEv.h         kernel event
h/IVTEntry.h      one interrupt vector table entry and the table of them
h/IdleTh.h        idle thread
h/List.h          list of PCBs
h/QUEUE.h         queue of blocked PCBs with remaining wait ticks
h/SemList.h       list of all semaphores, aged by the timer
h/kernel.h        kernel globals shared across source files
src/globals.cpp   timer interrupt routine, context switch, dispatch, lock and unlock
src/main.cpp      startup and shutdown around userMain
src/*.cpp         one file per header
scheduler/        SCHEDULE.H and APPLICAT.LIB, the course's scheduler
public test/      the course's public test program
docs/             the assignment and the project report, in Serbian
```

## Known limits

- Signals, the second task of the assignment, are not implemented.
- `operator new` and `cout` are not reentrant. The kernel calls them only inside its critical sections; user code that calls them from several threads has to guard them itself, as the public test does with `intLock`.
- One `Event` can be bound to an IVT entry at a time; creating a second one on the same entry rebinds the entry to it.
- A thread stack is capped at 64000 bytes, so that it fits in one 64 KB segment together with the allocator's header.
