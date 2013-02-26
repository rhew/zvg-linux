#ifndef _ZVGENC_H_
#define _ZVGENC_H_
/*****************************************************************************
* Header file for ZVGENC.C
*
* Author:       Zonn Moore
* Created:      11/06/02
* Last Updated: 05/21/03
*
* History:
*    06/30/03
*       Add ENCF_BW flag for B&W monitors.
*
* (c) Copyright 2002-2004, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ZSTDDEF_H_
#include	"zstddef.h"
#endif

// Absolute boundaries of ZVG display's veiwable area

#define	X_MIN			(-512)
#define	X_MAX			511
#define	Y_MIN			(-384)
#define	Y_MAX			383

// Absolute boundaries of ZVG display's overscan area

#define	X_MIN_O		(-600)
#define	X_MAX_O		599
#define	Y_MIN_O		(-472)
#define	Y_MAX_O		471

// Setup spotkiller boundaries.
//
// If the deflection is within one of these boundaries
// a spotkiller DOT will be generated.
//
// The spot killers on the WG6100 / Amplifone monitors are a proper 3/4 ratio
// test.  Meaning deflection must travel farther in the X direction than the Y
// direction.
//
// On the G05 and 19V2000 monitors, the spot killer looks at X and Y deflection
// equally so the Spot killer triggers on an equal lack of deflection in the X
// and Y directions.
//
// The boundary box set here is for the worst case B&W monitor.

#define	X_SK_MINB	(-384)
#define	X_SK_MAXB	384
#define	Y_SK_MINB	(-300)
#define	Y_SK_MAXB	300

// If less than the threshold number of vectors being drawn, generate spot kill
// points, regardless of boundary tests.

#define	SK_THRESHOLD	600

// The spotkill disable dots are sent in a full screen 3/4 ratio to disable
// a worst case color monitor properly.

// Spotkiller DOT placement positions

#define	X_SK_MINP	(X_MIN_O)
#define	X_SK_MAXP 	(X_MAX_O)
#define	Y_SK_MINP	(Y_MIN_O)
#define	Y_SK_MAXP	(Y_MAX_O)

#define	SK_COLOR		0x0000		// Color of spot used in the spotkiller killer (0=Invisible)

typedef struct ZVGENC_S
{
	// Variables used for encoding vectors

	int		xPos;							// current X position of ZVG encoder
	int		yPos;							// current Y position of ZVG encoder
	uint		zColor;						// current color used by ZVG encoder Z-axis
	uchar		*encBfr;						// pointer to start of encode buffer
	uint		encCount;					// number of bytes in encode buffer (also used as index)
	uint		encFlags;					// flags to keep track of various encoder settings
	uint		encColor;					// current color used by the encoder

	int		xMinClip;
	int		yMinClip;
	int		xMaxClip;
	int		yMaxClip;

	// threshold counts used to trigger spotkill killer

	int		vecCount;

	int		xMinSpot;
	int		yMinSpot;
	int		xMaxSpot;
	int		yMaxSpot;
} ZvgEnc_s;

// Define flags for 'ZvgEnc.encFlags'

#define	ENCF_FLIPX		0x01			// flip the display in the X direction
#define	ENCF_FLIPY		0x02			// flip the display in the Y direction
#define	ENCF_SPOTKILL	0x04			// if set, handle the spot killer
#define	ENCF_BW			0x08			// if set, mix colors down to B&W
#define	ENCF_NOOVS		0x10			// if set, no overscanning is allow (1024x768 max clip)

// Maximum number of bytes used by one ZVG vector command

#define	zENC_CMD_SIZE	9				// Max number of bytes needed to encode one command

extern ZvgEnc_s	ZvgENC;				// Encoder information structure

extern void zvgEncReset( void);
extern void zvgEncSetPtr( uchar *zvgBfr);
extern uint	zvgEncSize( void);
extern void zvgEncClearBfr( void);
extern void	zvgEncCenter( void);
extern void	zvgEncSOF( void);
extern void	zvgEncEOF( void);
extern void zvgEnc( int xStart, int yStart, int xEnd, int yEnd);
extern void zvgEncSetColor( uint newcolor);
extern void zvgEncSetRGB24( uint red, uint green, uint blue);
extern void zvgEncSetRGB16( uint red, uint green, uint blue);
extern void zvgEncSetRGB15( uint red, uint green, uint blue);
extern void zvgEncSetClipWin( int xMin, int yMin, int xMax, int yMax);
extern void zvgEncSetClipOverscan( void);
extern void zvgEncSetClipNoOverscan( void);

#ifdef __cplusplus
}
#endif

#endif
