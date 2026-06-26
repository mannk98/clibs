/*
 *sonRTOS
 *Athor: Nguyen Thanh Son
 *Email: thanhsoon.rf@gmail.com
 *Phone: +840973505781
 *Skype: nts2201
 */
#include "sonRTOS.h"
#include <util/delay.h>
#include  <util/atomic.h>
#include <avr/pgmspace.h>

#if USE_SYSTIME == 1
DATE_T date_r;
#endif

// variables used by the whole project:
volatile osTYPE_t readyToRun = 0;
volatile u08 thisTask;				// The number of the currently running task.
u16 ticks[nTasks]; 					  // If a task is suspended for ticks
u32 clockTicks_ms; 					// for keeping clock time

void *os_pointer[nTasks]; // address point to end of task ( os_pointer (2 bytes)-> [][] + context memory  + stack  memory)

// maybe many tasks want mutex, but just one task is mutex's owner at time.
osTYPE_t wantMutex[nMutex]; // each element is a mask, high for each task waiting for the mutex.
u08 mutexOwner[nMutex]; // Task number of semaphore owner, or 255 if mutex is available.

// convert a task number to the corresponding bit in readyToRun.
osTYPE_t two2n(u08 n) {
	return (1 << n);

	// use to save ram memory
#if (MAX_NTASK == 8)
	const osTYPE_t static t2n[ACTUAL_NTASK] PROGMEM = { 1, 0x2, 0x4, 0x8, 0x10,
			0x20, 0x40, 0x8 };
	return pgm_read_byte(t2n + n);
#elif (MAX_NTASK == 16)
	const osTYPE_t static t2n[ACTUAL_NTASK] PROGMEM = {1,0x2,0x4,0x8,0x10,0x20,0x40,0x80,0x100,0x200,0x400,0x800,0x1000,0x2000,0x4000,0x8000};
	return pgm_read_word(t2n + n);
#elif  (MAX_NTASK == 32)
	const osTYPE_t static t2n[ACTUAL_NTASK] PROGMEM = {1,0x2,0x4,0x8,0x10,0x20,0x40,0x80,0x100,0x200,0x400,0x800,0x1000,0x2000,0x4000,0x8000,
		0x10000,0x20000,0x40000,0x80000,0x100000,0x200000,0x400000,0x800000,0x1000000,0x2000000,0x4000000,0x8000000,0x10000000,0x20000000,0x40000000,0x80000000};
	return pgm_read_dword(t2n + n);
#else
#error "MAX_NTASK is too large"
#endif
//   return (1 << n);
}

typedef uint8_t ignored_ticks_t;
ignored_ticks_t volatile cntTickIsIgnored = 0;		 //max detected is 255 ticks

//inline cause it is called only one time
void __osCheckResources(ignored_ticks_t elapsed_ticks) __attribute__ ((always_inline));
void __osCheckResources(ignored_ticks_t elapsed_ticks) {
	for (u08 i = 0; i < nTasks; i++) {// check each task, if tick[i] = 0 because suspend(), don't check it.
		 // don't check task timeout(0xFFFF) or task with tick[thisTask]==0 (cause by wait(0), getMutex(mutex, 0), release(mutex)).
		if (ticks[i] && (ticks[i] != 0XFFFF)) {
			u16 temp_ticks = ticks[i];

			if (ticks[i] <= elapsed_ticks)
				ticks[i] = 0;					// --> task timeout
			else
				ticks[i] -= elapsed_ticks;		// if task isn't timeout yet
			// this if,else above make tick[i] turn to = 0

			if (readyToRun & two2n(i)) {				// if task ready to run
				ticks[i] = temp_ticks;
				continue;// skip --> check next task because tick[task] is not master.
			} // a Son: task is running and process ticks

			// when task is not ready to run and tick[task] == 0;
			if (ticks[i] == 0) { // this task is now ready to run cause timeout or wait
				u08 mutex_index;
				for (mutex_index = 0; mutex_index < nMutex; mutex_index++) 				// verify that is cause timeout of getMutex
						{
					if (wantMutex[mutex_index] & two2n(i)) { 			// ask if task i is waiting for the mutex_n ? if yes:
						// make this task don't wait for mutex anymore, and make tick[thisTask] = 0xFFFF --> mean this task is block)
						wantMutex[mutex_index] &= ~two2n(i);
						ticks[i] = -1; 						  //indicate timtickseout -> tick[i] = OxFFFF
						break;
					}
				}
				osSetRTR(i);			// set task i ready to run
			}
		}
	}
}

// find highest priority task and restore_context of this task
void __schedule(void) {
	osTYPE_t osTemp;

	// Do this when NO task is ready to run.
	do {
		ignored_ticks_t temp;
		ATOMIC_BLOCK(ATOMIC_FORCEON)
		{
			temp = cntTickIsIgnored;
			cntTickIsIgnored = 0;
		}
		if (temp) {
			__osCheckResources(temp);
		}
	} while (readyToRun == 0);

	osTemp = readyToRun;
	thisTask = 0;

	// find highest priority (0 is highest priority, n is lowest priority)
	while ((osTemp & 1) == 0) {
		osTemp >>= 1;
		thisTask += 1;
	}

	// check if stack of task overflow.
	if (*((u08*) ((u08*) os_pointer[thisTask] - 1)) != MASK_STACK) {// if stack grown up, it overwrite data of address os_pointer[thisTask]
		while (1) {
#ifdef HIDSerial_h
			hidWriteStringPGM("Task Overflow: ");
			hidWrite(' ');
			hidWriteUint16(thisTask, 10);
			hidWrite('\n');
#else
			_delay_ms(50);
#endif
			DDRC |= 0x03;
			PORTC ^= 0x03;
		}
	}

	__restoreContex(os_pointer[thisTask], 1); // resume the new task.
}

// Suspend the calling task for some number of ticks.
// Calling this with nTicks=0 will result in a
// suspension that will not be resumed by ticks,
// but can only be resumed by an ISR or another
// task setting the readyToRun bit.
void __osWaitTicks(u16 nTicks) {
	ticks[thisTask] = nTicks; //need atomic, must before osClearRTR() to indicate that task is using ticks
	osClearRTR(thisTask);
	__schedule();
}


/* acquire a semaphore, or suspend*/
// inline cause it is called only one time
// sleep(ticks_tmo) task when mutex is not ready, or gan' owner for this mutex.
void __getMutex(u08 semaNumber, u16 ticks_tmo) __attribute__ ((always_inline));
void __getMutex(u08 semaNumber, u16 ticks_tmo) {
	if (mutexOwner[semaNumber] == 0xFF) 			// no task is owner of this mutex --> this task take this mutex
	{
		mutexOwner[semaNumber] = thisTask;
	} else {										// some task already take this mutex before --> save to want mutex mask, sleep task for ticks_tmo (ticks)
		wantMutex[semaNumber] |= two2n(thisTask);	// and wait for mutex is released.
		osClearRTR(thisTask);
		ticks[thisTask] = ticks_tmo;
	}

	__schedule();									// find readyToRun task -> do other work.
}

// clear mutexOwner of this mutex, find highest priority task want mutex --> make it readyToRun or do nothing if there are no task want mutex.
void __releaseMutex(u08 mutexNumber) __attribute__ ((always_inline));
void __releaseMutex(u08 mutexNumber) {				/*inline cause it is called only one time*/
	osTYPE_t osTemp;

	osTemp = two2n(thisTask);
	wantMutex[mutexNumber] &= ~osTemp;				// clear bit mask want[mutex] -> make this task do not want mutex anymore

	// find highest priority task want mutex --> change it as mutex owner --> make it ready to run(tisk = 0 and readyToRun bits mask).
	osTemp = wantMutex[mutexNumber];
	if (osTemp != 0) {								// there are other task want mutex( is waiting).

		u08 temp = 0;
		// IF many task WANT mutex, OS find highest priority task --> and give mutex to it.
		while ((osTemp & 1) == 0)  					// find highest priority task is waiting for mutex
		{
			osTemp >>= 1;
			temp += 1;
		}

		mutexOwner[mutexNumber] = temp;				// change owner of the mutex
		ticks[temp] = 0; 							//clear timeout if was set. need atomic
		osSetRTR(temp);								// make task ready to run
	}
	// if no task want this mutex
	else {
		mutexOwner[mutexNumber] = -1; 				//no one is owner this mutex --> -1 = 0xFFFF
	}
	__schedule();
}

/*API API API API API API API API API API API API API API API API API API API API API API */
void osWaitTicks(u16 tks) {
	if (__saveContex((os_pointer[thisTask])) == 0) {
		__osWaitTicks(tks);
	}
}

void osWaitMs(u16 ms) {
	if (ms) {
		ms = ms / MSPERTICKS;
		if (ms == 0)
			ms = 1;
	}
	osWaitTicks(ms);
}

void osYield(void) {
	if (__saveContex((os_pointer[thisTask])) == 0)
		__schedule();
}

void osSuspend(void) {
	if (__saveContex((os_pointer[thisTask])) == 0)
		__osWaitTicks(0);
}

void osClearRTR(u08 task) {
	readyToRun = readyToRun & ~two2n(task);
}

void osSetRTR(u08 task) {
	readyToRun |= two2n(task);
}

s08 osGetMutex(u08 mutexNumber, u16 ms) {
	if (ms) {
		ms = (ms) / MSPERTICKS; //convert to number tick
		if (ms == 0)
			ms = 1;
	}
	if (__saveContex((os_pointer[thisTask])) == 0)
		__getMutex(mutexNumber, ms);

	if (ticks[thisTask] == 0XFF) {
		ticks[thisTask] = 0;
		return -1;
	} else
		return 0;
}

void osReleaseMutex(u08 mutexNumber) {
	if (__saveContex((os_pointer[thisTask])) == 0)
		__releaseMutex(mutexNumber);
}
/*ENDAPI ENDAPI ENDAPI ENDAPI ENDAPI ENDAPI ENDAPI ENDAPI ENDAPI ENDAPI ENDAPI ENDAPI */

//void __osProcesstime(u08 nTicks)  __attribute__ ((always_inline));
ISR_TIMER // timer2 overflows every 1 mSec.
{
#if USE_SYSTIME == 1
	static uint16_t cntsecs = 0;
	cntsecs += MSPERTICKS;
	if (cntsecs >= 1000) {
		secs++;
		cntsecs = 0;
	}
#endif
	sei();
	clockTicks_ms += MSPERTICKS;
	cntTickIsIgnored++;
}

//#include "port.c"
