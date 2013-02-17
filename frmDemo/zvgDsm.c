/*****************************************************************************
* A debug routine used to disassemble a ZVG buffer.
*
* Author:  Zonn Moore
* Created: 06/30/03
*
* History:
*
* (c) Copyright 2002-2004, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#include	<stdio.h>
#include	<stdlib.h>
#include	"zstddef.h"
#include	"zvgcmds.h"

#include	<conio.h>

static short	XLast = 0, YLast = 0;
static ushort	LastColor = 0;

void	zvgResetDsm( void)
{
	XLast = 0;
	YLast = 0;
	LastColor = 0x7BCF;
}

ushort zvgDecode( uchar *zvgPtr, short *pxStart, short *pyStart, short *pxEnd, short *pyEnd, ushort *pcolor)
{
	unsigned	char	cmd;
	int				xStart,yStart,xEnd,yEnd,vLen,ratio,color;
	uchar				*cmdStart;

	cmdStart = zvgPtr;
	cmd = *zvgPtr++;						// get command

	// check for center command, the only extended command supported

	switch (cmd)
	{
	case zcCENTER:
		xStart = 0;
		xEnd = 0;
		yStart = 0;
		yEnd = 0;
		color = zINIT_COLOR;
		LastColor = color;

		// save endpoints as last points

		XLast = xEnd;
		YLast = yEnd;

		*pxStart = xStart;
		*pyStart = yStart;
		*pxEnd = xEnd;
		*pyEnd = yEnd;
		*pcolor = color;
		return (1);

	case zcNOP:
		return (1);
	}
		
	if ((cmd & 0xB0) == 0xA0)
		return (0);							// reserve or extended command

	xStart = XLast;						// start with previous end values
	yStart = YLast;
	color = LastColor;

	// check for color bytes

	if (cmd & zbCOLOR)
	{	color = (int)*zvgPtr++ << 8;		// get upper byte
		color |= *zvgPtr++;					// get lower byte
	}

	*pcolor = color;
	LastColor = color;

	// check for absolute positions

	if (cmd & zbABS)
	{	xStart = *zvgPtr++;
		xStart |= ((int)*zvgPtr << 4) & 0x0F00;
		yStart = ((int)*zvgPtr++ << 8) & 0x0F00;
		yStart |= *zvgPtr++;

		// sign extend

		if (xStart & 0x0800)
			xStart |= 0xF000;

		if (yStart & 0x0800)
			yStart |= 0xF000;

		// check for absolute point

		if (!(cmd & zbVECTOR))
		{	xEnd = xStart;							// end points and start are the same
			yEnd = yStart;

			XLast = xEnd;							// save end points as last points
			YLast = yEnd;

			// return end points

			*pxStart = xStart;
			*pyStart = yStart;
			*pxEnd = xEnd;
			*pyEnd = yEnd;

			// done

			return (zvgPtr - cmdStart);
		}
	}

	// check for short length commands

	if (cmd & zbSHORT)
	{
		// check for ratio

		if (cmd & zbRATIO)
			ratio = (int)*zvgPtr++ << 8;

		vLen = *zvgPtr++;
	}

	// long length commands

	else
	{
		// check for ratio

		if (cmd & zbRATIO)
		{	ratio = (int)*zvgPtr++ << 8;
			ratio |= *zvgPtr & 0xF0;
		}
		vLen = ((int)*zvgPtr++ << 4) & 0x0F00;
		vLen |= *zvgPtr++;
	}

	xEnd = xStart;
	yEnd = yStart;

	// based on angle bits, recreate original endpoints

	switch (cmd & 0x0F)
	{
	case 0x00:			// 45 deg +X, +Y
		xEnd += vLen;
		yEnd += vLen;
		break;

	case 0x01:			// 45 deg +X, -Y
		xEnd += vLen;
		yEnd -= vLen;
		break;

	case 0x02:			// 45 deg -X, +Y
		xEnd -= vLen;
		yEnd += vLen;
		break;

	case 0x03:			// 45 deg -X, -Y
		xEnd -= vLen;
		yEnd -= vLen;
		break;

	case 0x04:			// 90 deg +X, 0Y
		xEnd += vLen;
		break;

	case 0x05:			// 90 deg -X, 0Y
		xEnd -= vLen;
		break;

	case 0x06:			// 90 deg 0X, +Y
		yEnd += vLen;
		break;

	case 0x07:			// 90 deg 0X, -Y
		yEnd -= vLen;
		break;

	case 0x08:			// Len=X, +X, +Y
		xEnd += vLen;
		yEnd += (int)((((long)vLen * ratio) + 0x8000) >> 16);
		break;

	case 0x09:			// Len=X, +X, -Y
		xEnd += vLen;
		yEnd -= (int)((((long)vLen * ratio) + 0x8000) >> 16);
		break;

	case 0x0A:			// Len=X, -X, +Y
		xEnd -= vLen;
		yEnd += (int)((((long)vLen * ratio) + 0x8000) >> 16);
		break;

	case 0x0B:			// Len=X, -X, -Y
		xEnd -= vLen;
		yEnd -= (int)((((long)vLen * ratio) + 0x8000) >> 16);
		break;

	case 0x0C:			// Len=Y, +X, +Y
		xEnd += (int)((((long)vLen * ratio) + 0x8000) >> 16);
		yEnd += vLen;
		break;

	case 0x0D:			// Len=Y, +X, -Y
		xEnd += (int)((((long)vLen * ratio) + 0x8000) >> 16);
		yEnd -= vLen;
		break;

	case 0x0E:			// Len=Y, -X, +Y
		xEnd -= (int)((((long)vLen * ratio) + 0x8000) >> 16);
		yEnd += vLen;
		break;

	case 0x0F:			// Len=Y, -X, -Y
		xEnd -= (int)((((long)vLen * ratio) + 0x8000) >> 16);
		yEnd -= vLen;
		break;
	}

	// check for relative point

	if (!(cmd & zbVECTOR))
	{	xStart = xEnd;						// start and end are the same
		yStart = yEnd;
	}

	// save endpoints as last points

	XLast = xEnd;
	YLast = yEnd;

	*pxStart = xStart;
	*pyStart = yStart;
	*pxEnd = xEnd;
	*pyEnd = yEnd;
	return (zvgPtr - cmdStart);
}

// Disassemble a ZVG buffer

void zvgDsm( uchar *bfr, uint size)
{
	unsigned char	cmd;
	short				xStart, yStart, xEnd, yEnd, ii;
	ushort			color, csize;
	uchar				*zvgPtr;

	zvgPtr = bfr;							// point to buffer

	fputc( '\n', stdout);

	while (zvgPtr < (bfr + size))
	{
		// Decode next instruction

		csize = zvgDecode( zvgPtr, &xStart, &yStart, &xEnd, &yEnd, &color);
		cmd = *zvgPtr;

		fputc( '\n', stdout);

		for (ii = 0; ii < csize; ii++)
			fprintf( stdout, "%02X ", *zvgPtr++);

		for (; ii < 9; ii++)
			fputs( "   ", stdout);

		fprintf( stdout, "XS=%5d,YS=%5d,XE=%5d,YE=%5d,CO=%04X", 
				xStart, yStart, xEnd, yEnd, color);

		fflush( stdout);

		getch();

		// check for end of command stream (assumes 'zcCENTER' ends stream)

		if (cmd == zcCENTER || csize == 0)
			break;			// exit loop
	}
}


