#if defined(__AVR__)

#include "port.h"
#include "sonRTOS.h"

//extern u16 nextAddTask;
u16 nextAddTask;
void port_OS_TASK_INIT(u16 pfunction, u16 sizeStack, u08 priorityTask) {
	u08 *p08;
	u16 temp;
	temp = nextAddTask;
	nextAddTask -= ((sizeStack) + sizeof(port_jmp_buf[0]) + 1);	// nextAddTask -= ((sizeStack) + sizeof(port_jmp_buf[0]));
	//if(nextAddTask <  __heap_start)while(1);
	os_pointer[priorityTask] = (u08*) nextAddTask + 2;// os_pointer[priorityTask] = (u08*) nextAddTask + 1;
	p08 = (u08*) (os_pointer[priorityTask]);
	p08[17] = p08[19] = temp >> 8; //YH  = SPH
	p08[16] = p08[18] = temp & 0xFF; //YL = SPL
	p08[20] = 0x80; //SSEG
	p08[21] = (pfunction) & 0XFF; //PCL
	p08[22] = (pfunction) >> 8;   //PCH
	p08 = (u08*) ((u08*) os_pointer[priorityTask] - 1);
	*p08 = MASK_STACK;
}

void port_InitOSTimer2() {
	nextAddTask = RAMEND;
	readyToRun = 0;	// no task ready to run

	// init: no task want semaphore, no task owner semaphore
	for (thisTask = 0; thisTask < nMutex; thisTask++) { // using thisTask for a different purpose here
		wantMutex[thisTask] = 0; // no task wants the semaphore. NOTE: not required as
		mutexOwner[thisTask] = -1; // c initializes variables to 0xFFFF.
	}

	// SP =  RAMEND - sizeof(env[0][0]) - 10;

	// setting Timer2
	TCCR2A |= (1 << WGM21) | (1 << WGM20);		// top = OCR2A
	TCCR2B |= (1 << CS22);
	TCNT2 = 0;

	OCR2A = 250;
	//OCR1BH=0x00;
	//OCR1BL=0x00;
	TIMSK2 |= (1 << OCIE2A);
	// sei();
}

#elif defined(__CODEVISIONAVR__)

#define port_OS_TASK_INIT(sizeStack) if(__saveContex(env[thisTask]) == 0)\
                                                    {\
                                                         env[thisTask][3] = nextAddTask >> 8;\
                                                         env[thisTask][2] = nextAddTask & 0xFF;\
                                                         nextAddTask -= sizeStack;\
                                                         env[thisTask][1] = nextAddTask >> 8;\
                                                         env[thisTask][0] = nextAddTask & 0xFF;\
                                                         return;\
                                                     }
#endif
