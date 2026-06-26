/*
 * wallCLock.c
 *
 *  Created on: Apr 20, 2022
 *      Author: mannk
 */

#include "wallClock.h"
#include "wallclock_tick.h"


#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )						// f=16 MHz -> 16 clock per Us	-- base clock cycle - NO PRESCALE
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )		// convert Machine cycle to Us
#define microsecondsToClockCycles(a) ( (a) * clockCyclesPerMicrosecond() )		// convert Us to Machine cycle





void initWallClock()
{
	// this needs to be called before setup() or some functions won't
	// work there
	sei();

	// on the ATmega168, timer 0 is also used for fast hardware pwm
	// (using phase-correct PWM would mean that timer 0 overflowed half as often
	// resulting in different millis() behavior on the ATmega8 and ATmega168)
#if defined(TCCR0A) && defined(WGM01)
	TCCR0A |= (1<<WGM01) | (1<<WGM00);
#endif

	// set timer 0 prescale factor to 64
#if defined(__AVR_ATmega128__)
	// CPU specific: different values for the ATmega128
	TCCR0 |= (1<<CS02);
#elif defined(TCCR0) && defined(CS01) && defined(CS00)
	// this combination is for the standard atmega8
	TCCR0 |= (1<<CS01) | (1<<CS00);
#elif defined(TCCR0B) && defined(CS01) && defined(CS00)
	// this combination is for the standard 168/328/1280/2560
	TCCR0B |= (1<<CS01) | (1<<CS00);
#elif defined(TCCR0A) && defined(CS01) && defined(CS00)
	// this combination is for the __AVR_ATmega645__ series
	TCCR0B |= (1<<CS01) | (1<<CS00);
#else
	#error Timer 0 prescale factor 64 not set correctly
#endif

	// enable timer 0 overflow interrupt
#if defined(TIMSK) && defined(TOIE0)
	TIMSK |= (1<<TOIE0);
#elif defined(TIMSK0) && defined(TOIE0)
	TIMSK0 |= (1<<TOIE0);
#else
	#error	Timer 0 overflow interrupt not set correctly
#endif
}


volatile uint32_t timer0_overflow_count = 0;
volatile uint32_t timer0_millis = 0;
static uint8_t timer0_fract = 0;

#if defined(TIM0_OVF_vect)
ISR(TIM0_OVF_vect)
#else
ISR(TIMER0_OVF_vect)
#endif
{
	wallclock_accum a = { timer0_millis, timer0_fract };
	wallclock_tick(&a);
	timer0_millis = a.millis;
	timer0_fract  = a.fract;
	timer0_overflow_count++;
}

uint32_t millis(){
	uint32_t m;
	uint8_t oldSREG = SREG;

	// disable interrupts while we read timer0_millis or we might get an
	// inconsistent value (e.g. in the middle of a write to timer0_millis)
	cli();
	m = timer0_millis;
	SREG = oldSREG;

	return m;
}

uint32_t micros(){
	uint32_t m;
	uint8_t oldSREG = SREG, t;

	cli();
	m = timer0_overflow_count;										// number of overflow time
#if defined(TCNT0)
	t = TCNT0;
#elif defined(TCNT0L)
	t = TCNT0L;
#else
	#error TIMER 0 not defined
#endif

#ifdef TIFR0
	if ((TIFR0 & _BV(TOV0)) && (t < 255))							//	TOV0 is auto cleared by hardware when interrupt exercute --> Use in case TOV0 is not cleared
		m++;
#else
	if ((TIFR & _BV(TOV0)) && (t < 255))
		m++;
#endif

	SREG = oldSREG;

	return ((m << 8) + t) * (64 / clockCyclesPerMicrosecond());		// number of Timer0 tick	/ (number tick of Micro Second)
																	// PRESCALE = 64 <-> 1 CLOCK <-> 64 CLOCK
}
