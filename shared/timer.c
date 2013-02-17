/*****************************************************************************
* Timer routines.
*
* Author:  Zonn Moore
* Created: 11/06/02
*
* History:
*
* (c) Copyright 2002-2004, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#include	<go32.h>
#include	<pc.h>
#include	<sys\farptr.h>

#include	"zstddef.h"
#include	"timer.h"

#define	io8254		0x40		// address of the 8254
#define	BIOS_CLOCK	0x046C	// real address of the BIOS clock ticks

uint				TmrFrameCounter;						// increment for each frame
static Timer_t	TmrFrameTicks, TmrFrameClock;

/*****************************************************************************
* Initialize timer routines, must be called before timers will work properly.
*
* Initialize 8254 to mode 2 instead of mode 3.  This should only be	     
* called once, and can cause a one time error, in the real-time clock, of    
* no more than 55ms.  Except for the above single time error, this routine   
* does not effect the real-time clock.
*
* Overview:
*
* The 8254 is supplied by with a (14.31818mhz / 12 = 1193181.667hz) clock.   
*
* The BIOS uses mode 3 with a count of 0000h, when programming the the 8254
* which results in a square wave with a count of 65536.  This causes the
* 8254 to decrement by 2 each clock time, which is how the count is really
* the cylce time of the full square wave.
*
* This routine re-initializes the 8254 to mode 2 with a count of 0000h.
* This causes the 8254 to decrement by 1 each clock time, with a single
* pulse at the zero count to signal the 8259.  The time between pulses is
* the same as the time of the square wave, so nothing is changed in
* real-time clock accuracy.
*
* Since the 8254 is re-initialized, up to 55ms of time can be lost.
*
* Since this routine is called from the protected mode, a selector is
* needed to access the BIOS timer information.  A flag indicates whether
* the selector is present to prevent GPF's from occuring.
*
* Called with:
*    NONE
*
* Returns:
*    NONE
*****************************************************************************/
void tmrInit( void)
{
	uint	mode;

	// Initialize the 8254 timer

	outportb( io8254+3, 0xE2);			// setup for reading mode
	mode = inportb( io8254);			// read mode
	mode &= 0x0E;							// get only mode bits

	// check if already in MODE 2

	if (mode != 0x04)
	{
		// if not in mode 2, set it

		outportb( io8254+3, 0x34);		// set to MODE 2
		outportb( io8254, 0x00);		// reset counter to max count
		outportb( io8254, 0x00);
	}
}

/*****************************************************************************
* Restore the 8254 timer to mode 3.
*
* Sets the 8254 back to it's original DOS programming mode.
* ZVG Timer routines will stop functioning properly after this call.
*
* Called with:
*    NONE
*
* Returns:
*    NONE
*****************************************************************************/
void tmrRestore( void)
{
	uint	mode;

	// Restore the 8254 timer

	outportb( io8254+3, 0xE2);			// setup for reading mode
	mode = inportb( io8254);			// read mode
	mode &= 0x0E;							// get only mode bits

	// check if already in MODE 3

	if (mode != 0x03)
	{
		// if not in mode 3, set it

		outportb( io8254+3, 0x36);		// set to MODE 3
		outportb( io8254, 0x00);		// reset counter to max count
		outportb( io8254, 0x00);
	}
}

/*****************************************************************************
* Set frame rate.
*
* This frame rate timer is a special time that times out once a framerate.
* This routine is called with the desired frames per second.  After this
* routine is called, subsequent calls to 'tmrTestFrame()', or 'tmrWaitFrame()'
* can be called to check for end of frame timing.
*
* Called with:
*    rate = Frame rate in frames per second.
*****************************************************************************/
void tmrSetFrameRate( int fps)
{
	if (fps == 0)
		fps = 1;								// if zero, set to one for error

	// calculate the number of ticks needed for given frame rate

	TmrFrameTicks = (TICK_RATE1000 / fps) / 1000;
	TmrFrameClock = tmrRead();			// reset the framerate clock
}

/*****************************************************************************
* Test for end of frame.
*
* Returns zero if not end of frame.  If not zero, the returns the number of
* frames skipped, normally this is one, indicating one frame has passed.
*
* If the return value is greater than one, it indicates more than one frame
* has passed since last called.
*
* Resets the frame timer so that subsequent calls will return zero until the
* next frame time has passed.
*
* Called with:
*    NONE
*
* Returns with:
*    0 = Not end of frame. If not zero, then returns frames skipped.
*****************************************************************************/
uint tmrTestFrame( void)
{
	Timer_t	timer;
	uint		frame;

	frame = TmrFrameCounter;				// get current frame count
	timer = tmrRead();						// read current time

	// Calculate number of frames that have passed

	while ((timer - TmrFrameClock) >= TmrFrameTicks)
	{	TmrFrameCounter++;					// increment frame counter
		TmrFrameClock += TmrFrameTicks;	// reset framerate clock
	}
	return (TmrFrameCounter - frame);	// return number of frames that have passed
}

/*****************************************************************************
* Wait for end of frame.
*
* Returns when end of frame is reached.
*
* Called with:
*    NONE
*
* Returns with:
*    Number of frames that have passed since last called.
*****************************************************************************/
uint tmrWaitFrame( void)
{
	uint	frames;

	while ((frames = tmrTestFrame()) == 0)
		;

	return (frames);
}

#if 0 	// replaced with #define for speed
/*****************************************************************************
* Read the frame counter
*****************************************************************************/
uint	tmrReadFramesCount( void)
{
	return (TmrFrameCounter);
}
#endif

/*****************************************************************************
* Test to see if a number of frames have passed.
*
* Returns TRUE if number of frames have passed since the 'frameCount' was
* read.  FALSE if number of frames have not passed.
*
* This routine does not update the 'frameCount', so it will continue to
* return TRUE, until 'frameCount' is updated by the calling program.
*
* Called with:
*    frameCount = Frame counter, read at start of delay loop.
*    frames     = Number of frames to test for.
*
* Returns with:
*    TRUE  - if number of 'frames' have passed since 'frameCount' was read.
*    FALSE - if number of 'frames' have not yet passed.
*****************************************************************************/
bool	tmrTestFrameCount( uint frameCount, uint frames)
{
	if (TmrFrameCounter - frameCount < frames)
		return (zFalse);

	return (zTrue);
}

/*****************************************************************************
* Read a timer.
*
* Reads a high resolution timer.  Used to start a background timer.
*
* Called with:
*    NONE
*
* Returns with:
*    Timer Ticks
*****************************************************************************/
Timer_t tmrRead( void)
{
		uint	lsb, msb, biosTicks, oBiosTicks;

		biosTicks = _farpeekw( _dos_ds, BIOS_CLOCK);			// get BIOS ticks

		// if BIOS ticks changes during a read, then a timer interrupt
		// occured and we must re-read the 8254 to be in sync

		do
		{	oBiosTicks = biosTicks;									// remember current ticks
			outportb( io8254+3, 0);									// latch timer 0
			lsb = inportb( io8254);									// get LSB
			msb = inportb( io8254);									// get MSB
			biosTicks = _farpeekw( _dos_ds, BIOS_CLOCK);		// re-read BIOS ticks
		} while (oBiosTicks != biosTicks);						// if different, IRQ occurred, try again

		lsb += msb << 8;												// get full 16 bit count
		lsb--;															// Convert '[1]0000h thru 0001h' to 'FFFFh thru 0000h'
		lsb = -lsb;														// count upwards instead of downwards

		return ((biosTicks << 16) + lsb);						// return full 32 bit timer count
}

/*****************************************************************************
* Test a timer using 8254 ticks.
*
* Returns a non-zero value when 'ticks' number of 8254 ticks have passed
* since the given timer value was read.
*
* There are	1,193,181.667 8254 ticks in a second.
*
* Called with:
*    timer = Timer value obtained by a previous call to 'tmrRead()'.
*    ticks = Number of 8254 ticks to test for.
*
* Returns with:
*    0 if timer has not timed out, not zero indicates timer has timed out.
*****************************************************************************/
bool tmrTestTicks( Timer_t timer, Timer_t ticks)
{
	// if time passed is less than number of ticks, then return 0

	if ((tmrRead() - timer) < ticks)
		return (zFalse);

	return (zTrue);
}

/*****************************************************************************
* Test a timer using milliseconds.
*
* Returns a non-zero value when 'ms' number of milliseconds have passed
* since the given timer value was read.
*
* Called with:
*    timer = Timer value obtained by a previous call to 'tmrRead()'.
*    ms    = Number of milliseconds to test for.
*
* Returns with:
*    0 if timer has not timed out, not zero indicates timer has timed out.
*****************************************************************************/
bool tmrTestms( Timer_t timer, ulong ms)
{
	// if time passed is less than number of ticks, then return 0

	if ((tmrRead() - timer) < (ms * TICKS_PER_MS))
		return (zFalse);

	return (zTrue);
}

