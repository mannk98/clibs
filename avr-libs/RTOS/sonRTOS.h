// ******************************************************************
//
// File Name:     sonRTOS.h
// Author:        thanhson.rf@gmail.com
// Date revised:  3/1/2015
// Version:       1.0
// Target MCU:    All AVR
//
// ******************************************************************

#ifndef sonRTOS_H
#define sonRTOS_H
#include "port.h"

/*
 #define USE_SYSTIME  1
 enum {
 usbTaskNumber, blinkTaskNumber, clockTaskNumber, adcTaskNumber, nTasks
 };

 enum {
 terminal_Mutex, usbtx_Mutex, nMutex
 };

 */

#define MASK_STACK 'S'

// mutexOwner: uint8_t --> max: 8 mutex
enum {
	mutex0, mutex1, mutex2, mutex3, mutex4, mutex5, mutex6, nMutex
};

enum {
	task0, task1, task2, task3, task4, task5, task6, nTasks
};

#define ACTUAL_NTASK 8
#define ACTUAL_NMUTEX 8

extern void *os_pointer[nTasks];

//******************** API ****** for USER
// function initTimer2() and allocate memory for task().
#define Init_SystemClock1MS		port_InitOSTimer2
#define osINIT_TASK             port_OS_TASK_INIT

//convert to milliseconds to ticks
#define M2T(ms) ((((ms) / MSPERTICKS) == 0) ? 1 : ((ms) / MSPERTICKS))

void osWaitTicks(u16 ticks);
void osWaitMs(u16 ms);

#define osWaitWhile(statement) while((statement))osWaitTicks(1)
#define osWaitUtil(statement) while(!(statement))osWaitTicks(1)

void osYield(void);
void osSuspend(void);

void osClearRTR(u08 task);
void osSetRTR(u08 task);

s08 osGetMutex(u08 mutexNumber, u16 ticks);
void osReleaseMutex(u08 mutexNumber);
//*************************************
//*************************************

// configuration osTYPE_t	(8/16/32 bits)
#if ACTUAL_NTASK > 16
   #define MAX_NTASK 32
   typedef u32 osTYPE_t;
#elif ACTUAL_NTASK > 8
   #define MAX_NTASK 16
   typedef u16 osTYPE_t;
#elif ACTUAL_NTASK <= 8
#define MAX_NTASK 8
typedef u08 osTYPE_t;
#endif

// create mask for tasks
osTYPE_t two2n(u08 n);
extern volatile osTYPE_t readyToRun;

// used in the os macros above
extern volatile u08 thisTask;
extern u16 ticks[];
extern u32 clockTicks_ms;
extern osTYPE_t wantMutex[];
extern u08 mutexOwner[];

#if USE_SYSTIME == 1
typedef struct {
	u08 SECS;
	u08 MINS;
	u08 HRS;
	u08 DAY;
	u08 MONTH;
	u08 YEAR;
} DATE_T;

extern DATE_T date_r;
#define secs date_r.SECS
#define mins date_r.MINS
#define hrs date_r.HRS
#define day date_r.DAY
#define month date_r.MONTH
#define year date_r.YEAR

#endif

// function use by user's API
void __schedule(void);
void __osWaitTicks(u16 nTicks);
//inline void __getMutex(u08 semaNumber, u16 ticks_tmo);
//inline void __releaseMutex(u08 semaNumber);
//void __osProcesstime(); // called every 1ms to process delay(timeout)

#endif

