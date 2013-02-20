/*****************************************************************************
* Routines for encoding vectors into the ZVG command set.
*
* Author:  Zonn Moore
* Created: 11/06/02
*
* History:
*   07/02/03
*      Moved spot kill logic here.  Added 'zvgSOF()' to allow start a frame
*      with spot kill dots if needed.  Changed the spotkill algorith one
*      more time!  This time because of WotW.  WotW frames deflect to the
*      edge, but a B&W monitor running at 15us per inch, fills the screen
*      so fast that the spotkiller comes on while waiting for the next frame,
*      so the first few vectors were not being seen.  New logic always draws
*      spotkill dots if less than SK_THRESHOLD vectors were displayed in last
*      frame.
* 
*    07/01/03
*      Moved Color and B&W logic from 'zvgFrame.c' to here.
*    
*   06/27/03
*      Fixed absolute DOT color problem again! (It's snuck back in while
*      rolling back to a separate 'zvgEnc.c' and 'zvgPort.c'.  Doh!)
*
*   06/26/03
*      Separated 'zvgEnc.c' from 'zvgPort.c'.
*
*   06/25/03
*      Added clipping routines. Finally!
*
*   06/19/03
*      Fixed problems with color after the Center command,
*      and position updates after an absolute DOT command is sent.
*
* (c) Copyright 2002-2004, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#ifdef WIN32
	#include <windows.h>
	#include <winbase.h>

	#include "wrappers.h"

#elif defined(linux)
#else // DOS
#endif

#include	<stdlib.h>
#include	"zstddef.h"
#include	"zvgcmds.h"
#include	"zvgenc.h"
#include	"zvgport.h"

// Defines for sending command bytes to buffer

#define	sendColor( color) \
	ZvgENC.encBfr[ZvgENC.encCount++] = (uchar)(color >> 8), \
	ZvgENC.encBfr[ZvgENC.encCount++] = (uchar)color

#define	sendXY( xx, yy) \
	ZvgENC.encBfr[ZvgENC.encCount++] = (uchar)xx, \
	ZvgENC.encBfr[ZvgENC.encCount++] = (uchar)((xx >> 4) & 0xF0) | ((yy >> 8) & 0x0F), \
	ZvgENC.encBfr[ZvgENC.encCount++] = (uchar)yy

#define	sendLen8( len) \
	ZvgENC.encBfr[ZvgENC.encCount++] = (uchar)len

#define	sendLen12( len) \
	ZvgENC.encBfr[ZvgENC.encCount++] = (uchar)((len >> 8) & 0x0F), \
	ZvgENC.encBfr[ZvgENC.encCount++] = (uchar)len

#define	sendRatioLen8( ratio, len) \
	ZvgENC.encBfr[ZvgENC.encCount++] = (uchar)(ratio >> 8), \
	ZvgENC.encBfr[ZvgENC.encCount++] = (uchar)len \

#define	sendRatioLen12( ratio, len) \
	ZvgENC.encBfr[ZvgENC.encCount++] = (uchar)(ratio >> 8), \
	ZvgENC.encBfr[ZvgENC.encCount++] = (uchar)(ratio & 0xF0) | ((len >> 8) & 0x0F), \
	ZvgENC.encBfr[ZvgENC.encCount++] = (uchar)len

#define	CHECK_X_SPOT( xx) \
   { \
		if (xx < ZvgENC.xMinSpot) ZvgENC.xMinSpot = xx; \
	   if (xx > ZvgENC.xMaxSpot) ZvgENC.xMaxSpot = xx; \
	}

#define	CHECK_Y_SPOT( yy) \
	{ \
	   if (yy < ZvgENC.yMinSpot) ZvgENC.yMinSpot = yy; \
	   if (yy > ZvgENC.yMaxSpot) ZvgENC.yMaxSpot = yy; \
	}

ZvgEnc_s		ZvgENC;					// Encoder information structure

/*****************************************************************************
* This routine does one iteration of line clipping and is part of the
* Liang-Barsky algorithm described in the book "Computer Graphics - 
* 'C' Version".  It has been converted from floating point to integer,
* other than that it is directly from the book. (Including the extensive
* comments. ;-)
*****************************************************************************/
static int clipTest( long pp, long qq, long *u1, long *u2)
{
	long	rr;
	int	retVal;

	retVal = zTrue;

	if (pp < 0)
	{	rr = (qq << 16) / pp;

		if (rr > *u2)
			retVal = zFalse;

		else if (rr > *u1)
			*u1 = rr;
	}
	else if (pp > 0)
	{	rr = (qq << 16) / pp;

		if (rr < *u1)
			retVal = zFalse;

		else if (rr < *u2)
			*u2 = rr;
	}
	else
	{
		// pp = 0, so line is parallel to this clipping edge

		if (qq < 0)
			retVal = zFalse;	// line is outside clipping edge
	}
	return (retVal);
}

/*****************************************************************************
* Clip a vector using the Liang-Barsky line clip algorithm.
*
* This routine is a direct copy from the book "Computer Graphics -
* 'C' Version". The math has been converted to integer arithmetic, along
* with a few comsmetic changes, but other than that it's identical.
* For more info I refer to the book (since the code contains the same
* comments the book did -- none.)
*
* Called with:
*    xStart = Pointer to X start position of vector.
*    yStart = Pointer to Y start position of vector.
*    xEnd   = Pointer to X end position of vector.
*    yEnd   = Pointer to Y end position of vector.
*
* Returns:
*    TRUE  - If vector has been accepted.
*    FALSE - If vector has been rejected.
*
* The vector will be clipped to fit inside the clip window.
*****************************************************************************/
static int clipLine( int *xStart, int *yStart, int *xEnd, int *yEnd)
{
	int	retVal;
	long	u1, u2, dx, dy;
	long	xs, ys, xe, ye;

	// initially reject vector

	retVal = zFalse;

	// get local copies of vector variables

	xs = *xStart;
	ys = *yStart;
	xe = *xEnd;
	ye = *yEnd;

	// Initialize values.  Use fixed point 32 bit values for math.  Decimal point is
	// assumed between the 15 and 16 bit.  1.0000 = 0x10000.

	u1 = 0;				// 0.0
	u2 = 0x10000;		// 1.0
	dx = xe - xs;

	if (clipTest( -dx, xs - ZvgENC.xMinClip, &u1, &u2))
		if (clipTest( dx, ZvgENC.xMaxClip - xs, &u1, &u2))
		{	dy = ye - ys;

			if (clipTest( -dy, ys - ZvgENC.yMinClip, &u1, &u2))
				if (clipTest( dy, ZvgENC.yMaxClip - ys, &u1, &u2))
				{
					if (u2 < 0x10000)
					{	xe = xs + (((u2 * dx) + 0x8000) >> 16);
						ye = ys + (((u2 * dy) + 0x8000) >> 16);
						*xEnd = xe;										// clip end points
						*yEnd = ye;
					}
					if (u1 > 0)
					{	xs += ((u1 * dx) + 0x8000) >> 16;
						ys += ((u1 * dy) + 0x8000) >> 16;
						*xStart = xs;									// clip end points
						*yStart = ys;
					}
					retVal = zTrue;
				}
		}
	return (retVal);
}

/*****************************************************************************
* Reset globals to that of a just powered on ZVG.
*****************************************************************************/
void zvgEncReset( void)
{
	// Reset ZVG status to same as ZVG at power on.

	ZvgENC.xPos = 0;		  			// set to center
	ZvgENC.yPos = 0;		  			// set to center

	ZvgENC.zColor = zINIT_COLOR;	// initial color used by ZVG

	// Reset clipping window to maximum overscan

	ZvgENC.xMinClip = X_MIN_O;
	ZvgENC.yMinClip = Y_MIN_O;
	ZvgENC.xMaxClip = X_MAX_O;
	ZvgENC.yMaxClip = Y_MAX_O;

	// reset spot kill variables to center of screen

	ZvgENC.vecCount = 0;

	ZvgENC.xMinSpot = 0;
	ZvgENC.yMinSpot = 0;
	ZvgENC.xMaxSpot = 0;
	ZvgENC.yMaxSpot = 0;
}

/*****************************************************************************
* Set ZVG buffer pointer to the start of a buffer.
*****************************************************************************/
void zvgEncSetPtr( uchar *zvgBfr)
{
	// point to start of a command buffer

	ZvgENC.encBfr = zvgBfr;		// point to start of buffer
	ZvgENC.encCount = 0;			// reset index
}

/*****************************************************************************
* Return the number of bytes in the ZVG buffer.
*****************************************************************************/
uint	zvgEncSize( void)
{
	return (ZvgENC.encCount);
}

/*****************************************************************************
* Return the number of bytes in the ZVG buffer.
*****************************************************************************/
void	zvgEncClearBfr( void)
{
	ZvgENC.encCount = 0;
}

/*****************************************************************************
* Center the trace by sending the "Center" command.
*****************************************************************************/
void	zvgEncCenter( void)
{
	ZvgENC.encBfr[ZvgENC.encCount++] = zcCENTER;		// send center command
	ZvgENC.xPos = 0;											// center current position
	ZvgENC.yPos = 0;

	// The center command also resets the color, so reinitialize the
	// color as well.

	ZvgENC.zColor = zINIT_COLOR;							// re-initialize color
}

/*****************************************************************************
* End of Frame command.
*
* This is called at the end of a frame to center the trace and flush the ZVG
* buffer with NOP's so that the last few vectors sitting in the ZVG buffer
* get processed for this frame.
*****************************************************************************/
void	zvgEncEOF( void)
{
	zvgEncCenter();									// send center command

	// The ZVG will not process commands until at least 9 bytes are in its
	// command buffer.  Add 8 NOPs to end of frame so that last few vectors
	// are processed for this frame.

	ZvgENC.encBfr[ZvgENC.encCount++] = zcNOP;
	ZvgENC.encBfr[ZvgENC.encCount++] = zcNOP;
	ZvgENC.encBfr[ZvgENC.encCount++] = zcNOP;
	ZvgENC.encBfr[ZvgENC.encCount++] = zcNOP;
	ZvgENC.encBfr[ZvgENC.encCount++] = zcNOP;
	ZvgENC.encBfr[ZvgENC.encCount++] = zcNOP;
	ZvgENC.encBfr[ZvgENC.encCount++] = zcNOP;
	ZvgENC.encBfr[ZvgENC.encCount++] = zcNOP;
}

/*****************************************************************************
* Set a clip window.
*
* The all vectors encoded *after* this call will be clipped to fit within
* this window.
*
* The clip window cannot be set beyond the edges of maximum overscan which
* are set by the X_MIN_O - Y_MAX_O variables.
*
* You cannot set the clip window to less that zero, if min's are greater
* than max's, they will be swapped.
*
* The clip window will follow the FLIP_X and FLIP_Y flags as well.
*
* Clipping is always done by the encoder with the default being to clip
* at the overscan boudaries.
*****************************************************************************/
void zvgEncSetClipWin( int xMin, int yMin, int xMax, int yMax)
{
	int	tSwap;

	// start by flipping window if needed

	if (ZvgENC.encFlags & ENCF_FLIPX)
	{	xMin = ~xMin;
		xMax = ~xMax;
	}

	if (ZvgENC.encFlags & ENCF_FLIPY)
	{	yMin = ~yMin;
		yMax = ~yMax;
	}

	// clip, clip window

	if (ZvgENC.encFlags & ENCF_NOOVS)
	{
		if (xMin < X_MIN)
			xMin = X_MIN;

		else if (xMin > X_MAX)
			xMin = X_MAX;

		if (yMin < Y_MIN)
			yMin = Y_MIN;

		else if (yMin > Y_MAX)
			yMin = Y_MAX;

		if (xMax < X_MIN)
			xMax = X_MIN;

		else if (xMax > X_MAX)
			xMax = X_MAX;

		if (yMax < Y_MIN)
			yMax = Y_MIN;

		else if (yMax > Y_MAX)
			yMax = Y_MAX;
	}
	else
	{		
		if (xMin < X_MIN_O)
			xMin = X_MIN_O;

		else if (xMin > X_MAX_O)
			xMin = X_MAX_O;

		if (yMin < Y_MIN_O)
			yMin = Y_MIN_O;

		else if (yMin > Y_MAX_O)
			yMin = Y_MAX_O;

		if (xMax < X_MIN_O)
			xMax = X_MIN_O;

		else if (xMax > X_MAX_O)
			xMax = X_MAX_O;

		if (yMax < Y_MIN_O)
			yMax = Y_MIN_O;

		else if (yMax > Y_MAX_O)
			yMax = Y_MAX_O;
	}

	// make sure a negative box size is not being requested

	if (xMin > xMax)
	{	tSwap = xMax;
		xMax = xMin;
		xMin = tSwap;
	}

	if (yMin > yMax)
	{	tSwap = yMax;
		yMax = yMin;
		yMin = tSwap;
	}

	// set new clip window

	ZvgENC.xMinClip = xMin;
	ZvgENC.yMinClip = yMin;
	ZvgENC.xMaxClip = xMax;
	ZvgENC.yMaxClip = yMax;
}

/*****************************************************************************
* Opens up clip window to maximum size.
*
* Clipping is always done by the encoder with the default being to clip
* at the overscan boundaries.
*****************************************************************************/
void zvgEncSetClipOverscan( void)
{
	// set new clip window

	if (ZvgENC.encFlags & ENCF_NOOVS)
		zvgEncSetClipNoOverscan();

	else
	{	ZvgENC.xMinClip = X_MIN_O;
		ZvgENC.yMinClip = Y_MIN_O;
		ZvgENC.xMaxClip = X_MAX_O;
		ZvgENC.yMaxClip = Y_MAX_O;
	}
}
	
/*****************************************************************************
* Set the clip window to the overscan boundary. This prevents overscan.
*****************************************************************************/
void zvgEncSetClipNoOverscan( void)
{
	// set new clip window

	ZvgENC.xMinClip = X_MIN;
	ZvgENC.yMinClip = Y_MIN;
	ZvgENC.xMaxClip = X_MAX;
	ZvgENC.yMaxClip = Y_MAX;
}

/*****************************************************************************
* Low level routine to encode a ZVG point.
*
* No clipping or axis flip is done. The spot kill boundary is not checked.
* The color given here does not change the color of the next vector drawn.
*
* Called with:
*    zvgCmd = Current state of 'zvgCmd'.
*    xStart = Starting X position of point to be drawn.
*    yStart = Starting Y position of point to be drawn.
*    color  = Color of vector (zvgCmd indicates whether color is sent).
*
* Globals:
*    ZvgENC.encCount = Points to next position in ZvgBfr to place ZVG command.
*    ZvgENC.xPos     = Current X position.
*    ZvgENC.yPos     = Current Y position.
*    ZvgENC.zColor   = Current Z color.
*****************************************************************************/
static void _zvgEncPoint_( int xStart, int yStart, uint color)
{
	uint	xLen, yLen, len, zvgCmd;
	uint	xSign, ySign, vRatio;

	// Check to see if color has changed

	if (color != ZvgENC.zColor)
		zvgCmd = zbCOLOR;					// indicate color information is to be sent

	else
		zvgCmd = 0;							// reset command flags

	// get direction of X and Y axis, and their respective lengths
	// using the current position as the starting point

	if (xStart < ZvgENC.xPos)
	{	xSign = 1;							// moves from right to left
		xLen = ZvgENC.xPos - xStart;	// get length of X axis
	}
	else
	{	xSign = 0;							// moves from left to right
		xLen = xStart - ZvgENC.xPos;	// get length of X axis
	}

	if (yStart < ZvgENC.yPos)
	{	ySign = 1;							// moves downward
		yLen = ZvgENC.yPos - yStart;	// get length of Y axis
	}
	else
	{	ySign = 0;	 						// moves upward
		yLen = yStart - ZvgENC.yPos;	// get length of Y axis
	}

	// Check if NOT a 45 or 90 degree jump.  If it is a 45 or 90
	// degree angle from current position, or distance is less
	// than 128 points, then fall through to send relative command,
	// otherwise send an absolute position commmand and return.

	if (xLen != 0 && yLen != 0 && xLen != yLen)
	{
		// if jump is not 45 or 90 degree, then check length
		// start by finding largest length

		if (xLen > yLen)
			len = xLen;

		else
			len = yLen;

		// if jump would require sending a 12 bit ratio, send
		// absolute position command instead, since it takes
		// the same number of bytes, but is easier for the ZVG
		// to digest

		if (len > 127)
		{
			zvgCmd |= zbABS;				// indicate absolute positioning

			ZvgENC.encBfr[ZvgENC.encCount++] = zvgCmd;

			if (zvgCmd & zbCOLOR)
				sendColor( color);

			sendXY( xStart, yStart);	// send POINT position

			ZvgENC.xPos = xStart;		// save new position
			ZvgENC.yPos = yStart;
			ZvgENC.zColor = color;		// update color
			return;							// done sending point, return
		}
	}

	// If not absolute, draw a point relative to last end position

	// check for 45 degree angles

	if (yLen == xLen)
	{
		// if length fits in 8 bits, send short length

		if (xLen < 256)
			zvgCmd |= zbSHORT;			// use short length

		// send ZVG command, and direction

		ZvgENC.encBfr[ZvgENC.encCount++] = zvgCmd | (xSign << 1) | ySign;

		if (zvgCmd & zbCOLOR)
			sendColor( color);

		// if short length, send it

		if (zvgCmd & zbSHORT)
			sendLen8( xLen);
	
		// else, send long length

		else
			sendLen12( xLen);
	}

	// check for horizontal jump

	else if (yLen == 0)
	{
		// if length fits in 8 bits, use short version of command

		if (xLen < 256)
			zvgCmd |= zbSHORT;			// use short length

		// send command

		ZvgENC.encBfr[ZvgENC.encCount++] = zvgCmd | zbHZVT | xSign;

		// send color if needed

		if (zvgCmd & zbCOLOR)
			sendColor( color);

		// send length, short or long

		if (zvgCmd & zbSHORT)
			sendLen8( xLen);
	
		else
			sendLen12( xLen);
	}

	// check for vertical jump

	else if (xLen == 0)
	{
		// if length fits in 8 bits, use short version of command

		if (yLen < 256)
			zvgCmd |= zbSHORT;			// use short length

		// send command

		ZvgENC.encBfr[ZvgENC.encCount++] = zvgCmd | zbHZVT | zbVERT | ySign;

		// send color if needed

		if (zvgCmd & zbCOLOR)		
			sendColor( color);

		// send length, short or long

		if (zvgCmd & zbSHORT)
			sendLen8( yLen);
	
		else
			sendLen12( yLen);
	}

	// If not 45 degree angle, vertical, or horizontal, then ratio must be
	// calculated.

	else
	{	zvgCmd |= zbRATIO;					// indicate ratio being sent

		// is X length greater the Y length?

		if (yLen < xLen)
		{
			// if length can fit in 7 bits, send short version of command

			if (xLen < 128)
				zvgCmd |= zbSHORT;			// use short length

			// send command

			ZvgENC.encBfr[ZvgENC.encCount++] = zvgCmd | (xSign << 1) | ySign;

			// calculate integer ratio

			vRatio = (unsigned int)(((long)yLen << 16) / xLen);

			// send color if needed
		
			if (zvgCmd & zbCOLOR)
				sendColor( color);

			// send length and ratio, short or long

			if (zvgCmd & zbSHORT)
				sendRatioLen8( vRatio, xLen);
	
			else
				sendRatioLen12( vRatio, xLen);
		}

		// Else, Y length is greater than X length.

		else
		{
			// if length can fit in 7 bits, send short version of command

			if (yLen < 128)
				zvgCmd |= zbSHORT;			// use short length

			// send command

			ZvgENC.encBfr[ZvgENC.encCount++] = zvgCmd | zbYLEN | (xSign << 1) | ySign;

			// calculate ratio

			vRatio = (unsigned int)(((long)xLen << 16) / yLen);

			// send color if needed
		
			if (zvgCmd & zbCOLOR)
				sendColor( color);

			// send length and ratio, short or long

			if (zvgCmd & zbSHORT)
				sendRatioLen8( vRatio, yLen);
	
			else
				sendRatioLen12( vRatio, yLen);
		}
	}
	ZvgENC.xPos = xStart;						// new position is POINT
	ZvgENC.yPos = yStart;
	ZvgENC.zColor = color;						// save new color
}

/*****************************************************************************
* Low level routine to encode a ZVG point.
*
* No clipping or axis flip is done, spot kill *is* tested.
* The color given here does not change the color of the next vector drawn.
*
* Called with:
*    zvgCmd = Current state of 'zvgCmd'.
*    xStart = Starting X position of point to be drawn.
*    yStart = Starting Y position of point to be drawn.
*    color  = Color of vector (zvgCmd indicates whether color is sent).
*
* Globals:
*    ZvgENC.encCount = Points to next position in ZvgBfr to place ZVG command.
*    ZvgENC.xPos     = Current X position.
*    ZvgENC.yPos     = Current Y position.
*    ZvgENC.zColor   = Current Z color.
*****************************************************************************/
static void zvgEncPointSK( int xStart, int yStart, uint color)
{
	// do spotkill check if needed

	if (ZvgENC.encFlags & ENCF_SPOTKILL)
	{
		CHECK_X_SPOT( xStart)
		CHECK_Y_SPOT( yStart)
	}
	_zvgEncPoint_( xStart, yStart, color);
}

/*****************************************************************************
* Low level routine for display a POINT.  Used to send points to the ZVG
* outside the clipping area. Mostly used to send spotkiller points.
*
* No clipping is done, but X & Y flipping is done if needed.
* The Spot kill boundary will not be expanded by calls to this routine.
* The color given here does not change the color of the next vector drawn.
*
* Called with:
*    zvgCmd = Current state of 'zvgCmd'.
*    xStart = Starting X position of point to be drawn.
*    yStart = Starting Y position of point to be drawn.
*    color  = Color of vector (zvgCmd indicates whether color is sent).
*
* Globals:
*    ZvgENC.encCount = Points to next position in ZvgBfr to place ZVG command.
*    ZvgENC.xPos     = Current X position.
*    ZvgENC.yPos     = Current Y position.
*    ZvgENC.zColor   = Current Z color.
*****************************************************************************/
static void zvgEncPointFL( int xStart, int yStart, uint color)
{
	// check for axis flips

	if (ZvgENC.encFlags & ENCF_FLIPX)
		xStart = ~xStart;

	if (ZvgENC.encFlags & ENCF_FLIPY)
		yStart = ~yStart;

	_zvgEncPoint_( xStart, yStart, color);
}

/*****************************************************************************
* Routine to encode ZVG commands given vector coordinates.
*
* Called with:
*    xStart = Starting X position of vector to be drawn.
*    yStart = Starting Y position of vector to be drawn.
*    xEnd   = Ending X position of vector to be drawn.
*    yEnd   = Ending Y position of vector to be drawn.
*
* Globals:
*    ZvgENC.encCount = Points to next position in ZvgBfr to place ZVG command.
*    ZvgENC.xPos     = Current X position.
*    ZvgENC.yPos     = Current Y position.
*    ZvgENC.zColor   = Current Z color internal to the ZVG.
*    ZvgENC.encColor - Color of vector to be drawn.
*****************************************************************************/
void zvgEnc( int xStart, int yStart, int xEnd, int yEnd)
{
	uint	xLen, yLen;
	uint	xSign, ySign, vRatio;
	uint	zvgCmd;
	int	diff;

	ZvgENC.vecCount++;						// count number of vectors

	// check for axis flips

	if (ZvgENC.encFlags & ENCF_FLIPX)
	{	xStart = ~xStart;
		xEnd = ~xEnd;
	}

	if (ZvgENC.encFlags & ENCF_FLIPY)
	{	yStart = ~yStart;
		yEnd = ~yEnd;
	}

	// Check if NOT a point, vertical or horizontal line, then
	// clip the line the old fashion way.

	if (xStart != xEnd && yStart != yEnd)
		if (!clipLine( &xStart, &yStart, &xEnd, &yEnd))
			return;								// if vector rejected, just return

	// Check for point

	if (xStart == xEnd && yStart == yEnd)
	{
		// clip data point

		if (xStart < ZvgENC.xMinClip)
			return;								// do nothing if outside window

		if (xStart > ZvgENC.xMaxClip)
			return;								// do nothing if outside window

		if (yStart < ZvgENC.yMinClip)
			return;								// do nothing if outside window

		if (yStart > ZvgENC.yMaxClip)
			return;								// do nothing if outside window

		// encode data point

		zvgEncPointSK( xStart, yStart, ZvgENC.encColor);
		return;
	}

	// Handle vectors.

	zvgCmd = zbVECTOR;						// indicate a vector is being drawn

	// Check to see if color has changed

	if (ZvgENC.encColor != ZvgENC.zColor)
		zvgCmd |= zbCOLOR;					// indicate color information is to be sent

	// Check to see if start of this vector is same as current trace position
	// (if start point is the same a previous, then it does not need to be clipped)

	if (xStart != ZvgENC.xPos || yStart != ZvgENC.yPos)
		zvgCmd |= zbABS;						// if not, the starting points must be sent

	// get direction of X and Y axis, and their respective lengths

	if (xEnd < xStart)
	{	xSign = 1;								// moves from right to left
		xLen = xStart - xEnd;				// get length of X axis
	}
	else
	{	xSign = 0;								// moves from left to right
		xLen = xEnd - xStart;				// get length of X axis
	}

	if (yEnd < yStart)
	{	ySign = 1;								// moves downward
		yLen = yStart - yEnd;				// get length of Y axis
	}
	else
	{	ySign = 0;	 							// moves upward
		yLen = yEnd - yStart;				// get length of Y axis
	}

	// check for horizontal line

	if (yLen == 0)
	{
		// clip vector if needed

		if (yStart < ZvgENC.yMinClip || yStart > ZvgENC.yMaxClip)
			return; 						// reject vector

		// Does vector move from right to left?

		if (xSign)
		{
			if (xEnd > ZvgENC.xMaxClip || xStart < ZvgENC.xMinClip)
				return;					// reject vector

			diff = ZvgENC.xMinClip - xEnd;

			if (diff > 0)
			{	xEnd += diff;			// clip line
				xLen -= diff;
			}

			// if new starting points sent, they must be clipped

			if (zvgCmd & zbABS)
			{
				diff = xStart - ZvgENC.xMaxClip;

				if (diff > 0)
				{	xStart -= diff;	// clip line
					xLen -= diff;
				}
			}
		}

		// Else, vector moves from left to right

		else
		{
			if (xEnd < ZvgENC.xMinClip || xStart > ZvgENC.xMaxClip)
				return;					// reject vector

			diff = xEnd - ZvgENC.xMaxClip;

			if (diff > 0)
			{	xEnd -= diff;			// clip line
				xLen -= diff;
			}

			// if new starting points sent, they must be clipped

			if (zvgCmd & zbABS)
			{
				diff = ZvgENC.xMinClip - xStart;

				if (diff > 0)
				{	xStart += diff;	// clip line
					xLen -= diff;
				}
			}
		}

		// check if vector clipped to a point, if so, encode point

		if (xLen == 0)
		{	zvgEncPointSK( xStart, yStart, ZvgENC.encColor);
			return;						// done sending point, return
		}

		if (ZvgENC.encFlags & ENCF_SPOTKILL)
		{
			CHECK_X_SPOT( xStart)
			CHECK_X_SPOT( xEnd)
			CHECK_Y_SPOT( yStart)
		}

		// if length fits in 8 bits, use short version of command

		if (xLen < 256)
			zvgCmd |= zbSHORT;			// use short length

		// send command

		ZvgENC.encBfr[ZvgENC.encCount++] = zvgCmd | zbHZVT | xSign;

		// send color if needed

		if (zvgCmd & zbCOLOR)
			sendColor( ZvgENC.encColor);

		// send starting positions if needed

		if (zvgCmd & zbABS)
			sendXY( xStart, yStart);

		// send length, short or long

		if (zvgCmd & zbSHORT)
			sendLen8( xLen);
	
		else
			sendLen12( xLen);
	}

	// check for vertical line

	else if (xLen == 0)
	{
		// clip vector if needed

		if (xStart < ZvgENC.xMinClip || xStart > ZvgENC.xMaxClip)
			return; 						// reject vector

		// Does vector move downward?

		if (ySign)
		{
			if (yEnd > ZvgENC.yMaxClip || yStart < ZvgENC.yMinClip)
				return;					// reject vector

			diff = ZvgENC.yMinClip - yEnd;

			if (diff > 0)
			{	yEnd += diff;			// clip line
				yLen -= diff;
			}

			// if new starting points sent, they must be clipped

			if (zvgCmd & zbABS)
			{
				diff = yStart - ZvgENC.yMaxClip;

				if (diff > 0)
				{	yStart -= diff;	// clip line
					yLen -= diff;
				}
			}
		}

		// Else, vector moves upward

		else
		{
			if (yEnd < ZvgENC.yMinClip || yStart > ZvgENC.yMaxClip)
				return;					// reject vector

			diff = yEnd - ZvgENC.yMaxClip;

			if (diff > 0)
			{	yEnd -= diff;			// clip line
				yLen -= diff;
			}

			// if new starting points sent, they must be clipped

			if (zvgCmd & zbABS)
			{
				diff = ZvgENC.yMinClip - yStart;

				if (diff > 0)
				{	yStart += diff;	// clip line
					yLen -= diff;
				}
			}
		}

		// check if vector clipped to a point, if so, encode point

		if (yLen == 0)
		{	zvgEncPointSK( xStart, yStart, ZvgENC.encColor);
			return;						// done sending point, return
		}

		if (ZvgENC.encFlags & ENCF_SPOTKILL)
		{
			CHECK_X_SPOT( xStart)
			CHECK_Y_SPOT( yStart)
			CHECK_Y_SPOT( yEnd)
		}

		// if length fits in 8 bits, use short version of command

		if (yLen < 256)
			zvgCmd |= zbSHORT;			// use short length

		// send command

		ZvgENC.encBfr[ZvgENC.encCount++] = zvgCmd | zbHZVT | zbVERT | ySign;

		// send color if needed

		if (zvgCmd & zbCOLOR)		
			sendColor( ZvgENC.encColor);

		// send starting positions if needed

		if (zvgCmd & zbABS)
			sendXY( xStart, yStart);

		// send length, short or long

		if (zvgCmd & zbSHORT)
			sendLen8( yLen);
	
		else
			sendLen12( yLen);
	}

	// look for 45 degree angle

	else if (yLen == xLen)
	{
		// spot kill test

		if (ZvgENC.encFlags & ENCF_SPOTKILL)
		{
			CHECK_X_SPOT( xStart)
			CHECK_X_SPOT( xEnd)
			CHECK_Y_SPOT( yStart)
			CHECK_Y_SPOT( yEnd)
		}

		// if length fits in 8 bits, send short length

		if (xLen < 256)
			zvgCmd |= zbSHORT;				// use short length

		// send ZVG command, and direction

		ZvgENC.encBfr[ZvgENC.encCount++] = zvgCmd | (xSign << 1) | ySign;

		if (zvgCmd & zbCOLOR)
			sendColor( ZvgENC.encColor);

		// if starting points required, send them

		if (zvgCmd & zbABS)
			sendXY( xStart, yStart);

		// if short length, send it

		if (zvgCmd & zbSHORT)
			sendLen8( xLen);
	
		// else, send long length

		else
			sendLen12( xLen);
	}

	// If not 45 degree angle, vertical, or horizontal, then ratio must be
	// calculated.

	else
	{
		// spot kill test

		if (ZvgENC.encFlags & ENCF_SPOTKILL)
		{
			CHECK_X_SPOT( xStart)
			CHECK_X_SPOT( xEnd)
			CHECK_Y_SPOT( yStart)
			CHECK_Y_SPOT( yEnd)
		}

		zvgCmd |= zbRATIO;					// indicate ratio being sent

		if (xLen > yLen)
		{
			// calculate integer ratio

			vRatio = (uint)(((ulong)yLen << 16) / xLen);

			// if length can fit in 7 bits, send short version of command

			if (xLen < 128)
				zvgCmd |= zbSHORT;			// use short length

			// send command

			ZvgENC.encBfr[ZvgENC.encCount++] = zvgCmd | (xSign << 1) | ySign;

			// send color if needed
		
			if (zvgCmd & zbCOLOR)
				sendColor( ZvgENC.encColor);

			// send start positions if needed

			if (zvgCmd & zbABS)
				sendXY( xStart, yStart);

			// send length and ratio, short or long

			if (zvgCmd & zbSHORT)
				sendRatioLen8( vRatio, xLen);
	
			else
				sendRatioLen12( vRatio, xLen);
		}

		// Else, Y length is greater than X length.

		else
		{
			// calculate ratio

			vRatio = (unsigned int)(((long)xLen << 16) / yLen);

			// if length can fit in 7 bits, send short version of command

			if (yLen < 128)
				zvgCmd |= zbSHORT;			// use short length

			// send command

			ZvgENC.encBfr[ZvgENC.encCount++] = zvgCmd | zbYLEN | (xSign << 1) | ySign;

			// send color if needed
		
			if (zvgCmd & zbCOLOR)
				sendColor( ZvgENC.encColor);

			// send start positions if needed

			if (zvgCmd & zbABS)
				sendXY( xStart, yStart);

			// send length and ratio, short or long

			if (zvgCmd & zbSHORT)
				sendRatioLen8( vRatio, yLen);
	
			else
				sendRatioLen12( vRatio, yLen);
		}
	}
	ZvgENC.xPos = xEnd;						// new position is end of vector
	ZvgENC.yPos = yEnd;
	ZvgENC.zColor = ZvgENC.encColor;		// save new color
}

/*****************************************************************************
* Start of Frame command.
*
* This is called at the start of a frame to generate Spot Kill points if
* needed.
*****************************************************************************/
void	zvgEncSOF( void)
{
	uint	skFlag;

	if (ZvgENC.encFlags & ENCF_SPOTKILL)
	{
		skFlag = zFalse;

		// Check if max deflection was enough to disable the spotkiller

		if (ZvgENC.vecCount < SK_THRESHOLD)
			skFlag = zTrue;							// spotkill point needed

		else if (ZvgENC.xMinSpot > X_SK_MINB)
			skFlag = zTrue;							// spotkill point needed
			
		else if (ZvgENC.xMaxSpot < X_SK_MAXB)
			skFlag = zTrue;							// spotkill point needed

		else if (ZvgENC.yMinSpot > Y_SK_MINB)
			skFlag = zTrue;							// spotkill point needed

		else if (ZvgENC.yMaxSpot < Y_SK_MAXB)
			skFlag = zTrue;							// spotkill point needed

		// if spot kill active, disable it

		if (skFlag)
		{	zvgEncPointFL( X_SK_MAXP, Y_SK_MINP, SK_COLOR);
			zvgEncPointFL( X_SK_MINP, Y_SK_MAXP, SK_COLOR);
		}

		// Re-init deflection variables

		ZvgENC.vecCount = 0;

		ZvgENC.xMinSpot = 0;
		ZvgENC.yMinSpot = 0;
		ZvgENC.xMaxSpot = 0;
		ZvgENC.yMaxSpot = 0;
	}
}	

/*****************************************************************************
* Directly set the vector color.
*
* This routine set the color using the ZVG native 16 bit color encoding.
* 16 bit color word is encoded as: rrrrrggggggbbbbb
*
* No Color to B&W conversion is done here, this is simply a fast way set a
* previously calculated color word.
*
* Where:
*    rrrrr  = Red value (0-31)
*    gggggg = Green value (0-63)
*    bbbbb  = Blue value (0-31)
*
* For B&W monitors, only the GREEN channel is used to indicate intensity.
*****************************************************************************/
void zvgEncSetColor( uint newcolor)
{
	ZvgENC.encColor = newcolor;		// set new color
}

/*****************************************************************************
* Set color using 24 bit RGB mode.  Only the upper bits of each color are
* used, this routine makes it easy to do the 24 bit to 16 bit conversion.
*
* If a B&W monitor is connected to the ZVG, the colors will be mixed down
* to the green gun, using a "Brightest Color Wins" algorithm.
*
* Called with:
*    red   = Red value (0-255)
*    green = Green value (0-255)
*    blue  = Blue value (0-255)
*****************************************************************************/
void zvgEncSetRGB24( uint red, uint green, uint blue)
{
	// limit colors to 8 bits
	
	if (red > 255)
		red = 255;

	if (green > 255)
		green = 255;

	if (blue > 255)
		blue = 255;

	// If B&W, convert color to B&W using a "Brightest Color Wins" algorithm.

	if (ZvgENC.encFlags & ENCF_BW)
	{
		if (red > green)
			green = red;

		if (blue > green)
			green = blue;

		// Set all colors nearly equal.  Green has one more bit of resolution
		// which is retained, so Green can be 1/64 brighter than Red or Blue
		// on a color monitor that is being driven as a B&W monitor by a color
		// game.  Not the usual mode of operation.

		green &= 0xFC;
		red = green & 0xF8;
		blue = red;
	}
	else
	{	
		// Mask unused bits in each color and shift to proper location in
		// 16-bit color value.

		red &= 0xF8;	// keep only upper 5 bits
		green &= 0xFC;	// keep only upper 6 bits
//*	blue &= 0xF8;	// blue bits will be shifted out and don't need masking
	}

	// shift bits into proper position and create new color

	ZvgENC.encColor = (red << 8) | (green << 3) | (blue >> 3);
}

/*****************************************************************************
* Set color using 16 bit RGB mode.
*
* If a B&W monitor is connected to the ZVG, the colors will be mixed down
* to the green gun, using a "Brightest Color Wins" algorithm.
*
* Called with:
*    red   = Red value (0-31)
*    green = Green value (0-63)
*    blue  = Blue value (0-31)
*
* For B&W monitors, only the GREEN channel is used to indicate intensity.
*****************************************************************************/
void zvgEncSetRGB16( uint red, uint green, uint blue)
{
	// limit colors

	if (red > 31)
		red = 31;

	if (green > 63)
		green = 63;

	if (blue > 31)
		blue = 31;

	// If B&W, convert color to B&W using a "Brightest Color Wins" algorithm.

	if (ZvgENC.encFlags & ENCF_BW)
	{
		red <<= 1;										// convert to 6 bit colors
		blue <<= 1;

		// check for brightest color

		if (red > green)
			green = red;

		if (blue > green)
			green = blue;

		// Set all colors nearly equal.  Green has one more bit of resolution
		// which is retained, so Green can be 1/64 brighter than Red or Blue
		// on a color monitor that is being driven as a B&W monitor by a color
		// game.  Not the usual mode of operation.

		red = green >> 1;
		blue = red;
	}

	// shift bits into proper position and create new color

	ZvgENC.encColor = (red << 11) | (green << 5) | blue;
}

/*****************************************************************************
* Set color using 15 bit RGB mode.  The GREEN channel will be left shifted
* and have a trailing zero added to it.
*
* If a B&W monitor is connected to the ZVG, the colors will be mixed down
* to the green gun, using a "Brightest Color Wins" algorithm.
*
* Called with:
*    red   = Red value (0-31)
*    green = Green value (0-31)
*    blue  = Blue value (0-31)
*
* For B&W monitors, only the GREEN channel is used to indicate intensity.
*****************************************************************************/
void zvgEncSetRGB15( uint red, uint green, uint blue)
{	
	// limit colors

	if (red > 31)
		red = 31;

	if (green > 31)
		green = 31;

	if (blue > 31)
		blue = 31;

	// If B&W, convert color to B&W using a "Brightest Color Wins" algorithm.
	// Set all colors equal.

	if (ZvgENC.encFlags & ENCF_BW)
	{
		if (red > green)
			green = red;

		else
			red = green;

		if (blue > green)
			green = blue;

		else
			blue = green;
	}

	// shift bits into proper position and create new color
	
	ZvgENC.encColor = (red << 11) | (green << 6) | blue;
}
