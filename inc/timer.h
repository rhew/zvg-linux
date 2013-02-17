#ifndef _TIMER_H_
#define _TIMER_H_
/*****************************************************************************
* ZVG timer routines.
*
* Author:       Zonn Moore
* Last Updated: 05/21/03
*
* History:
*
* (c) Copyright 2003-2004, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#ifndef _ZSTDDEF_H_
#include	"zstddef.h"
#endif

// The clock ticks are times 1000 to allow better resolution during tick
// calculations.

#define	TICK_RATE1000		1193181667		// tick rate * 1000
#define	TICKS_PER_MS		1193				// ticks in a millisecond

// Define timer type

typedef unsigned long		Timer_t;

// External frame counter, incremented each frame

extern uint		TmrFrameCounter;

// Define to remove redundant call

#define	tmrReadFrameCount()	TmrFrameCounter

// Prototypes

extern void		tmrInit( void);
extern void		tmrRestore( void);
extern void		tmrSetFrameRate( int fps);
extern uint		tmrTestFrame( void);
extern uint		tmrWaitFrame( void);
extern Timer_t	tmrRead( void);
extern bool		tmrTestTicks( Timer_t timer, ulong ticks);
extern bool		tmrTestms( Timer_t timer, ulong ms);
extern bool		tmrTestFrameCount( uint frameCount, uint frames);
#endif
