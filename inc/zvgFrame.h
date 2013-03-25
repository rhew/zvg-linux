#ifndef _ZVGFRAME_H_
#define _ZVGFRAME_H_
/*****************************************************************************
* Header file for FRAME buffered ZVG driver routines.
*
* Author:       Zonn Moore
* Created:      05/20/03
*
* History:
*    07/30/03
*       Added "TIMER.H" to file.
*
*    06/27/03
*       Added "ZVGENC.H" and "ZVGPORT.H" to file, making "ZVGFRAME.H" the
*       only file needing to be included for those using ZVGFRAME routines.
*
* (c) Copyright 2003-2004, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#ifndef _ZVGENC_H_
#include	"zvgEnc.h"
#endif

#ifndef _ZVGPORT_H_
#include	"zvgPort.h"
#endif

#ifndef _TIMER_H_
#include	"timer.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Setup some aliases to make the API consistent.

#define	zvgFrameSetColor( newcolor) \
			zvgEncSetColor( newcolor)

#define	zvgFrameSetRGB24( red, green, blue) \
			zvgEncSetRGB24( red, green, blue)

#define	zvgFrameSetRGB16( red, green, blue) \
			zvgEncSetRGB16( red, green, blue)

#define	zvgFrameSetRGB15( red, green, blue) \
			zvgEncSetRGB15( red, green, blue)

#define	zvgFrameSetClipWin( xMin, yMin, xMax, yMax) \
			zvgEncSetClipWin( xMin, yMin, xMax, yMax)

#define	zvgFrameSetClipOverscan() \
			zvgEncSetClipOverscan()

#define	zvgFrameSetClipNoOverscan() \
			zvgEncSetClipNoOverscan()

// Prototypes
extern ZvgSpeeds_a	ZvgSpeeds;
extern ZvgMon_s		ZvgMon;
extern ZvgID_s			ZvgID;

extern uint zvgFrameOpen( void);
extern void zvgFrameClose( void);
extern uint zvgFrameVector( uint xStart, uint yStart, uint xEnd, uint yEnd);
extern uint zvgFrameSend(void);

#ifdef __cplusplus
}
#endif

#endif
