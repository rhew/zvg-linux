/*****************************************************************************
* Demo code used to test the ZVG SDK.
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
#include	<conio.h>
#include	<sys\farptr.h>
#include	"zstddef.h"
#include	"zvgport.h"
#include	"zvgenc.h"
#include	"timer.h"
#include	"zvgframe.h"

#define	RANDLOGO					// let the DEMO logo randomly drift, else move it with keyboard

// If manually moved logo, then allow a dump of the ZVG buffer using the zvg disassembler.

#ifndef RANDLOGO
uchar			ZvgDsmBfr[4096];
extern void zvgResetDsm( void);
extern void	zvgDsm( uchar *bfr, uint size);
#endif

// Starting with a print out of the file 64x48.txt I drew the ZEKTOR logo
// centered on the paper.  Then numbered each vector.
//
// The smallest X value is -17, the largest is 17.
// The smallest Y value is  -5, the largest is  5.
//
// This array is each of those numbered vectors written as: xStart,yStart,xEnd,yEnd

#define	X_LOGO_MIN		-17L		// smallest X in logo
#define	Y_LOGO_MIN		-5L		// smallest Y in logo
#define	X_LOGO_MAX		17L		// largest X in logo
#define	Y_LOGO_MAX		5L			// largest Y in logo

#define	LOGO_SCALE_X		8L		// size multiplier for LOGO
#define	LOGO_SCALE_Y		4L		// size multiplier for LOGO

#define	X_MIN_EDGE		((X_MIN_O - (X_LOGO_MIN * LOGO_SCALE_X)) << 8)
#define	X_MAX_EDGE		((X_MAX_O - (X_LOGO_MAX * LOGO_SCALE_X)) << 8)
#define	Y_MIN_EDGE		((Y_MIN_O - (Y_LOGO_MIN * LOGO_SCALE_Y)) << 8)
#define	Y_MAX_EDGE		((Y_MAX_O - (Y_LOGO_MAX * LOGO_SCALE_Y)) << 8)

#define	FRAMES_PER_SEC	60			// number of frames drawn in a second
#define	MAX_SPEED		2048
#define	MIN_SPEED		128

#define	NewSpeed()		(random() % (MAX_SPEED - MIN_SPEED) + MIN_SPEED)
#define	NewSpeedDir()	(NewSpeed() * ((random() % 100 < 50) ? -1 : 1))

#define	FLICKER_FRAMES	4			// number of frames to brighten when collision detected

#define	EDGE_BRI			25			// Bright intensity for edge collision
#define	EDGE_NRM			15			// Normal edge intensity

int ZektorLogo[] =
{
	// xStart, yStart, xEnd, yEnd

	// Z

	-17,  3, -17,  5,			// 1	
	-17,  5, -13,  5,			// 2	
	-13,  5, -17, -5,			// 3	
	-17, -5, -13, -5,			// 4	
	-13, -5, -13, -3,			// !5	

	// E

	 -7,  3,  -7,  5,			// 6	
	 -7,  5, -11,  5,			// 7	
	-11,  5,  -8,  0,			// 8	
	 -8,  0, -11, -5,			// 9	
	-11, -5,  -7, -5,			// 10
	 -7, -5,  -7, -3,			// 11

	// K

	 -5, -5,  -5,  5,			// 12
	 -1,  5,  -5,  0,			// 13
	 -5,  0,  -1, -5,			// 14

	// T

	  3, -5,   3,  5,			// 15
	  1,  3,   1,  5,			// 16
	  1,  5,   5,  5,			// 17
	  5,  5,   5,  3,			// 18

	// O

	  9, -5,   7,  0,			// 19
	  7,  0,   9,  5,			// 20
	  9,  5,  11,  0,			// 21
	 11,  0,   9, -5,			// 22

	// R

	 13, -5,  13,  5,			// 23
	 13,  5,  17,  3,			// 24
	 17,  3,  13,  0,			// 25
	 13,  0,  17, -5			// 26
};

// The floating logo of death demo code...

int main( void)
{
	uint	err, ii;
	char	cc;
	int	xOffset, yOffset;						// offsets for logo
	int	xSpeed, ySpeed;						// direction and speed values
	int	xStart, yStart, xEnd, yEnd;		// calculated vector values
	int	xLogo, yLogo;							// some temp values
	int	xMinCo, xMaxCo, yMinCo, yMaxCo;	// edge color counters
	uint	borderFlag;

	err = zvgFrameOpen();						// initialize everything

	if (err)
	{	zvgError( err);							// print error
		exit( 0);									// return to DOS
	}

	// print out a ZVG banner, indicating version etc.

	zvgBanner( ZvgSpeeds, &ZvgID);

	// initialize the logo position to the center of the screen

	xOffset = 0;
	yOffset = 0;
	xMinCo = 0;
	xMaxCo = 0;
	yMinCo = 0;
	yMaxCo = 0;

#ifdef RANDLOGO

	// start the logo moving randomly

	srand( 1);										// always start the same way

	xSpeed = NewSpeedDir();						// pick a value between MIN_SPEED and MAX_SPEED, and a dir
	ySpeed = NewSpeedDir();						// pick a value between MIN_SPEED and MAX_SPEED, and a dir

#else

	xSpeed = 0;
	ySpeed = 0;

#endif

	borderFlag = zTrue;

	// The timer has been initialized by the call to 'zvgFrameOpen()', we just need to
	// set the frame rate.

	tmrSetFrameRate( FRAMES_PER_SEC);

	zvgFrameSetClipWin( X_MIN, Y_MIN, X_MAX, Y_MAX);

	// Start draw loop

	while (1)
	{
		// move the logo

		xOffset += xSpeed;
		yOffset += ySpeed;

		// test for limits

		if (xOffset >= X_MAX_EDGE)
		{	xOffset = X_MAX_EDGE;
			xSpeed = -NewSpeed();			// pick a value between MIN_SPEED and MAX_SPEED
			xMaxCo = FLICKER_FRAMES;
		}
		else if (xOffset <= X_MIN_EDGE)
		{	xOffset = X_MIN_EDGE;
			xSpeed = NewSpeed();				// pick a value between MIN_SPEED and MAX_SPEED
			xMinCo = FLICKER_FRAMES;
		}

		if (yOffset >= Y_MAX_EDGE)
		{	yOffset = Y_MAX_EDGE;
			ySpeed = -NewSpeed();			// pick a value between MIN_SPEED and MAX_SPEED
			yMaxCo = FLICKER_FRAMES;
		}
		else if (yOffset <= Y_MIN_EDGE)
		{	yOffset = Y_MIN_EDGE;
			ySpeed = NewSpeed();				// pick a value between MIN_SPEED and MAX_SPEED
			yMinCo = FLICKER_FRAMES;
		}

		// check for edge collision, brighten up vectors when hit

		if (yMaxCo > 0)
		{	zvgFrameSetRGB15( EDGE_BRI, EDGE_BRI, EDGE_BRI);
			yMaxCo--;
		}
		else
			zvgFrameSetRGB15( EDGE_NRM, EDGE_NRM, EDGE_NRM);

		if (borderFlag)
		{	zvgFrameVector( X_MIN,  Y_MAX,  X_MAX,  Y_MAX);
//			zvgFrameVector( X_MIN_O,  Y_MAX_O,  X_MAX_O,  Y_MAX_O);
		}

		if (xMaxCo > 0)
		{	zvgFrameSetRGB15( EDGE_BRI, EDGE_BRI, EDGE_BRI);
			xMaxCo--;
		}
		else
			zvgFrameSetRGB15( EDGE_NRM, EDGE_NRM, EDGE_NRM);

		if (borderFlag)
		{	zvgFrameVector(  X_MAX,  Y_MAX,  X_MAX, Y_MIN);
//			zvgFrameVector(  X_MAX_O,  Y_MAX_O,  X_MAX_O, Y_MIN_O);
		}

		if (yMinCo > 0)
		{	zvgFrameSetRGB15( EDGE_BRI, EDGE_BRI, EDGE_BRI);
			yMinCo--;
		}
		else
			zvgFrameSetRGB15( EDGE_NRM, EDGE_NRM, EDGE_NRM);

		if (borderFlag)
		{	zvgFrameVector(  X_MAX, Y_MIN, X_MIN, Y_MIN);
//			zvgFrameVector(  X_MAX_O, Y_MIN_O, X_MIN_O, Y_MIN_O);
		}

		if (xMinCo > 0)
		{	zvgFrameSetRGB15( EDGE_BRI, EDGE_BRI, EDGE_BRI);
			xMinCo--;
		}
		else
			zvgFrameSetRGB15( EDGE_NRM, EDGE_NRM, EDGE_NRM);

		if (borderFlag)
		{	zvgFrameVector( X_MIN, Y_MIN, X_MIN,  Y_MAX);
//			zvgFrameVector( X_MIN_O, Y_MIN_O, X_MIN_O,  Y_MAX_O);
		}
	
		// light up logo on collision

		if (xMaxCo != 0 || xMinCo != 0 || yMaxCo != 0 || yMinCo != 0)
			zvgFrameSetRGB15( 31, 31, 31);

		else
			zvgFrameSetRGB15( 15, 15, 15);

		// draw the ZEKTOR logo, at set LOGO_SCALE, and new offsets

		xLogo = xOffset / 256;					
		yLogo = yOffset / 256;

		for (ii = 0; ii < sizeof( ZektorLogo) / sizeof( *ZektorLogo); ii += 4)
		{
			// draw properly scaled and offset logo

			xStart =	ZektorLogo[ii] * LOGO_SCALE_X + xLogo;
			yStart =	ZektorLogo[ii+1] * LOGO_SCALE_Y + yLogo;
			xEnd = ZektorLogo[ii+2] * LOGO_SCALE_X + xLogo;
			yEnd = ZektorLogo[ii+3] * LOGO_SCALE_Y + yLogo;

			// send vector info to ZVG drivers

			zvgFrameVector( xStart, yStart, xEnd, yEnd);
		}

#ifndef RANDLOGO

		if (xSpeed != 0 || ySpeed != 0)
		{	fprintf( stdout, "\nxLogo = %d, yLogo = %d, xPos=%d, yPos=%d, DmaCount=%u", xLogo, yLogo, ZvgENC.xPos, ZvgENC.yPos,
					ZvgIO.dmaCurCount);

			fflush( stdout);
		}

		xSpeed = 0;
		ySpeed = 0;
#endif

//	zvgEncEOF();
//	zvgDmaPutMem( EncodeBfr, zvgEncSize());

	// Clear the encode buffer

//	zvgEncClearBfr();


		// check for ESC key

		if (kbhit())
		{	cc = getch();					// read keyboard

			if (cc == 0x1B)
				break;						// if ESC pressed, leave loop

			if (cc == 'B' || cc == 'b')
				borderFlag = !borderFlag;

#ifndef RANDLOGO
			switch (cc)
			{
			case '8':
				ySpeed = -256;
				break;

			case '4':
				xSpeed = -256;
				break;

			case '6':
				xSpeed = 256;
				break;

			case '2':
				ySpeed = 256;
				break;

			case 'd':
			case 'D':


				// loop through buffer sending bytes to the ZVG

				for (ii = 0; ii < ZvgIO.dmaCurCount; ii++)
					ZvgDsmBfr[ii] = _farpeekb( ZvgIO.dmaMemSel, ZvgIO.dmaCurPMode + ii);


				zvgResetDsm();				// reset the DSM since the zcCENTER command is not seen by it
				zvgDsm( ZvgDsmBfr, ZvgIO.dmaCurCount);
				break;
			}
#endif

		}

		// wait for next frame time

		tmrWaitFrame();

		// send next frame

		err = zvgFrameSend();

		if (err)
		{	zvgError( err);
			break;
		}
	}

	// if loop exited, return to DOS

	zvgFrameClose();		// fix up all the ZVG stuff
	return (0);
}
