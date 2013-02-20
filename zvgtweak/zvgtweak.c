/*****************************************************************************
* Program to "tweak" the ZVG vector board.
* 
* Used to adjust the internal ZVG registers that are used to time
* vector draws.
*
* Version:      1.0
* Author:       Zonn Moore
* Created:      11/06/02
*
* History:
*    07/30/03
*       Move the generation of the logo to ZVGTWEAK.C.  Removed MAKELOGO.C.
*
* (c) Copyright 2002-2003, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#if defined(WIN32)
#elif defined(linux)
#else // DOS
#endif // OS

#if defined(WIN32)
	#include <windows.h>
	#include <conio.h>
#elif defined(linux)
	#include <curses.h>
#else // DOS
	#include	<conio.h>
	#include	<keys.h>
	#include	<dos.h>
	#include	<bios.h>
	#include	<time.h>
	#include	<go32.h>
	#include	<sys\farptr.h>
	#include	<sys\movedata.h>
	#include	<sys\segments.h>
#endif // OS

#include	"zvgframe.h"
#include	"zvgcmds.h"

// For DEBUG_WITHOUT_ZEKTOR_DRIVER to work, also need to exclude the zekShr
// library from the link. 
//#define DEBUG_WITHOUT_ZEKTOR_DRIVER

// Set text attributes for DEFAULT, SELECTED and MESSAGES

#if defined(WIN32)
#elif defined(linux)
	// Macros to convert DOS calls to corresponding curses calls.  This will
	// reduce cluttering of the code with ifdefs.
	#define gotoxy(c,r)			move(r,c)
	#define cputs(s)			addstr(s)
	#define textattr(a)			attrset(a)
	#define cprintf(f,sc,v,ec)	printw(f,sc,v,ec)
	#define clreol				clrtoeol

	#define	K_Right		KEY_RIGHT
	#define	K_Left		KEY_LEFT
	#define	K_Down		KEY_DOWN
	#define	K_Up		KEY_UP

	#define DEF_PAIR		1
	#define SEL_PAIR		2
	#define MSG_PAIR		3

	#define	ATTR_DEF		(COLOR_PAIR(DEF_PAIR) | A_NORMAL)
	#define	ATTR_SEL		(COLOR_PAIR(SEL_PAIR) | A_NORMAL)
	#define	ATTR_MSG		(COLOR_PAIR(MSG_PAIR) | A_NORMAL)
#else // DOS
	#define	ATTR_DEF		((BLUE << 4) | CYAN)
	#define	ATTR_SEL		((BLACK << 4) | YELLOW)
	#define	ATTR_MSG		((RED << 4) | BLACK)
#endif // OS

#define	FRAMERATE	60								// frames per second

#ifdef DEBUG_WITHOUT_ZEKTOR_DRIVER
	// Define macros and function prototypes to support testing of this code
	// without having a real Zektor card atteched to the parallel port.
	#define zvgFrameOpenThreaded	yFrameOpen
	#define zvgFrameOpen			yFrameOpen
	#define zvgFrameCloseThreaded	yFrameClose
	#define zvgFrameClose			yFrameClose
	#define zvgFrameSendThreaded	yFrameSend
	#define zvgFrameSend			yFrameSend
	#define zvgGetABufferThreaded	yGetABufferThreaded
	#define zvgReadMonitorInfo		yReadMonitorInfo
	#define zvgEncSetColor			yEncSetColor
	#define zvgEncSetRGB15			yEncSetRGB15
	#define zvgDmaPutc				yDmaPutc
	#define zvgError				yError
	#define zvgFrameVector			yFrameVector
	#define tmrSetFrameRate			ytmrSetFrameRate
	#define tmrWaitFrame			ytmrWaitFrame
	#define zvgBanner				yBanner

	static uint yFrameOpen( void );
	static void yFrameClose( void );
	static uint yFrameSend( void);
	static uint yGetABufferThreaded( void);
	static uint yReadMonitorInfo( ZvgMon_s *mon);
	static void yEncSetColor( uint newcolor);
	static void yEncSetRGB15(unsigned int, unsigned int, unsigned int);
	static uint yDmaPutc( uchar cc);
	static void	yError( uint err);
	static uint yFrameVector( int xStart, int yStart, int xEnd, int yEnd);
	static void	ytmrSetFrameRate( int fps);
	static uint ytmrWaitFrame();
	static void yBanner( ZvgSpeeds_a, ZvgID_s *);

//	ZvgID_s		ZvgID;
//	ZvgSpeeds_a	ZvgSpeeds;
//	ZvgMon_s		ZvgMon;

#endif // DEBUG_WITHOUT_ZEKTOR_DRIVER

// Structure used to hold data read from the ZVG

extern ZvgID_s		ZvgID;
extern ZvgSpeeds_a	ZvgSpeeds;
extern ZvgMon_s		ZvgMon;

// Structure to hold local data, includes row and column address of
// where parameter is to be printed on the screen.

typedef struct PARAM_S
{	char	row;					// screen row to display desc of parameter
	char	col;					// screen column to display desc of parameter
	char	*title;				// description of parameter
	char	vrow;					// screen row to display value of parameter
	char	vcol;					// screen column to display value of parameter
	char	*fmt;					// "printf" format of parameter
	int	value;				// current local value of parameter
	char	*help;				// Help string associated with parameter
} Param_s;

// Fill up an array of the above structure, each array represents a
// ZVG parameter.

Param_s	Param[] =
{
	{	 3, 1, "Z-Axis Shift:",       3, 20, "%c%3u%c", 0, ""},
	{	 4, 1, "Z-Axis Overshoot:",   4, 20, "%c%3u%c", 0, ""},
	{	 6, 1, "Maximum Intensity:",  6, 20, "%c%3u%c", 0, ""},
	{	 7, 1, "Minimum Intensity:",  7, 20, "%c%3u%c", 0, ""},
	{	 8, 1, "Point Intensity:",    8, 20, "%c%3u%c", 0, ""},
	{	10, 1, "Screen Size:",       10, 20, "%c%3u%c", 0, ""},
	{	12, 1, "Jump Speed:",        12, 20, "%c%3u%c", 0, ""},
	{	13, 1, "Settling Time:",     13, 20, "%c%3u%c", 0, ""}
};

// Define indexes into the above array for each parameter

enum
{	ZSHIFT,
	ZOSHOOT,
	ZMAXINT,
	ZMININT,
	ZPOINT,
	ZSIZE,
	ZJUMP,
	ZSETTLE
};

// Starting with a print out of the file 64x48.txt I drew the ZEKTOR logo
// centered on the paper.  Then numbered each vector.
//
// This array is each of those numbered vectors written as: xStart,yStart,xEnd,yEnd

int zektorLogo[] =
{
	// xStart, yStart, xEnd, yEnd

	// Z

	-17,  3, -17,  5,			// 1	
	-17,  5, -13,  5,			// 2	
	-13,  5, -17, -5,			// 3	
	-17, -5, -13, -5,			// 4	
	-13, -5, -13, -3,			// 5	

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

// The .COM follows the ZEKTOR logo.

int dotComLogo[] =
{
	// .

	19, -5, 19, -5,

	// C

	24, -3, 22, -5,
	22, -5, 20,  0,
	20,  0, 22,  5,
	22,  5, 24,  3,

	// O

	28, -5, 26,  0,
	26,  0, 28,  5,
	28,  5, 30,  0,
	30,  0, 28, -5,

	// M

	32, -5, 32,  3,
	32,  3, 33,  5,
	33,  5, 34,  3,
	34,  3, 35,  5,
	35,  5, 36,  3,
	36,  3, 36, -5,
	34, -5, 34,  3
};

/*****************************************************************************
* If something goes wrong just call this routine to return to DOS
*****************************************************************************/
void exitProg( void)
{
#if defined(WIN32)
#elif defined(linux)

	endwin();

#else // DOS
#endif // OS

	zvgFrameClose();							// turn off DMA and free DMA buffers
	exit( 0);
}

/*****************************************************************************
*****************************************************************************/
int drawDisplay( void)
{
	uint	ii, xStart, yStart, xEnd, yEnd;

	zvgFrameSetRGB15( 7, 7, 7);

	// Draw a full size overscanned box, if setup properly, this will be drawn off the edges
	// of the screen.

	zvgFrameVector( X_MIN_O, Y_MAX_O, X_MAX_O, Y_MAX_O);
	zvgFrameVector( X_MAX_O, Y_MAX_O, X_MAX_O, Y_MIN_O);
	zvgFrameVector( X_MAX_O, Y_MIN_O, X_MIN_O, Y_MIN_O);
	zvgFrameVector( X_MIN_O, Y_MIN_O, X_MIN_O, Y_MAX_O);

	// Draw a box at the edges of the visible screen

	zvgFrameVector( X_MIN, Y_MAX, X_MAX, Y_MAX);
	zvgFrameVector( X_MAX, Y_MAX, X_MAX, Y_MIN);
	zvgFrameVector( X_MAX, Y_MIN, X_MIN, Y_MIN);
	zvgFrameVector( X_MIN, Y_MIN, X_MIN, Y_MAX);

	// Draw some intensity bars

	zvgFrameSetRGB15( 31, 31, 31);
	zvgFrameVector( -400, 300, -350, 300);

	zvgFrameSetRGB15( 27, 27, 27);
	zvgFrameVector( -400, 290, -350, 290);

	zvgFrameSetRGB15( 23, 23, 23);
	zvgFrameVector( -400, 280, -350, 280);

	zvgFrameSetRGB15( 19, 19, 19);
	zvgFrameVector( -400, 270, -350, 270);

	zvgFrameSetRGB15( 15, 15, 15);
	zvgFrameVector( -400, 260, -350, 260);

	zvgFrameSetRGB15( 11, 11, 11);
	zvgFrameVector( -400, 250, -350, 250);

	zvgFrameSetRGB15( 07, 07, 07);
	zvgFrameVector( -400, 240, -350, 240);

	zvgFrameSetRGB15( 03, 03, 03);
	zvgFrameVector( -400, 230, -350, 230);

	zvgFrameSetRGB15( 00, 00, 00);
	zvgFrameVector( -400, 220, -350, 220);

	zvgFrameSetRGB15( 7, 7, 7);

	// place a "normal" size ZEKTOR logo in middle of screen

	for (ii = 0; ii < sizeof( zektorLogo) / sizeof( *zektorLogo); ii += 4)
	{
		// place logo in center of screen, 16x larger than it was drawn.

		xStart =	zektorLogo[ii] * 16;
		yStart =	zektorLogo[ii+1] * 16;
		xEnd = zektorLogo[ii+2] * 16;
		yEnd = zektorLogo[ii+3] * 16;

		// encode vector into command buffer

		zvgFrameVector( xStart, yStart, xEnd, yEnd);
	}

	// place a smaller logo in the lower right corner followed by a .COM

	for (ii = 0; ii < sizeof( zektorLogo) / sizeof( *zektorLogo); ii += 4)
	{
		// draw logo on screen 6x size, and down 300 points and to the right 156 points

		xStart =	zektorLogo[ii] * 6 + 156;
		yStart =	zektorLogo[ii+1] * 6 - 300;
		xEnd = zektorLogo[ii+2] * 6 + 156;
		yEnd = zektorLogo[ii+3] * 6 - 300;

		// encode vector into command buffer

		zvgFrameVector( xStart, yStart, xEnd, yEnd);
	}

	// Add ".com" to above logo

	for (ii = 0; ii < sizeof( dotComLogo) / sizeof( *dotComLogo); ii += 4)
	{
		// draw .com on screen 6x size, and down 300 points and to the right 156 points

		xStart =	dotComLogo[ii] * 6 + 156;
		yStart =	dotComLogo[ii+1] * 6 - 300;
		xEnd = dotComLogo[ii+2] * 6 + 156;
		yEnd = dotComLogo[ii+3] * 6 - 300;

		// encode vector into command buffer

		zvgFrameVector( xStart, yStart, xEnd, yEnd);
	}

	// Draw a small logo in the upper left corner

	for (ii = 0; ii < sizeof( zektorLogo) / sizeof( *zektorLogo); ii += 4)
	{
		// draw on screen 2x size, and up 195 points and to the left 290 points

		xStart =	zektorLogo[ii] * 2 - 290;
		yStart =	zektorLogo[ii+1] * 2 + 195;
		xEnd = zektorLogo[ii+2] * 2 - 290;
		yEnd = zektorLogo[ii+3] * 2 + 195;

		// encode vector into command buffer

		zvgFrameVector( xStart, yStart, xEnd, yEnd);
	}

	// Place real big logo on top of everything.  This logo should go off the edges
	// of a properly sized screen.

	for (ii = 0; ii < sizeof( zektorLogo) / sizeof( *zektorLogo); ii += 4)
	{
		// place logo in center of screen, 32x larger than it was drawn.

		xStart =	zektorLogo[ii] * 32;
		yStart =	zektorLogo[ii+1] * 32;
		xEnd = zektorLogo[ii+2] * 32;
		yEnd = zektorLogo[ii+3] * 32;

		// encode vector into command buffer

		zvgFrameVector( xStart, yStart, xEnd, yEnd);
	}
	return (errOk);
}

/*****************************************************************************
* Syncronize the ZVG with the local variables.
*
* The 'ZvgMon' structure is used to keep track of the ZVG's internal state.
* This routine compares the 'Param[]' array values with the 'ZvgMon'
* variables, and if a difference is found, sends the new 'Param[].value' to
* the ZVG and updates the ZvgMon structure.
*****************************************************************************/
void paramSync( void)
{
	if (Param[ZSHIFT].value != ZvgMon.zShift)
	{	ZvgMon.zShift = Param[ZSHIFT].value;
		zvgDmaPutc( zcZSHIFT);
		zvgDmaPutc( ZvgMon.zShift);
	}

	if (Param[ZOSHOOT].value != ZvgMon.oShoot)
	{	ZvgMon.oShoot = Param[ZOSHOOT].value;
		zvgDmaPutc( zcOSHOOT);
		zvgDmaPutc( ZvgMon.oShoot);
	}

	if (Param[ZJUMP].value != ZvgMon.jumpFactor)
	{	ZvgMon.jumpFactor = Param[ZJUMP].value;
		zvgDmaPutc( zcJUMP);
		zvgDmaPutc( ZvgMon.jumpFactor);
	}

	if (Param[ZSETTLE].value != ZvgMon.settle)
	{	ZvgMon.settle = Param[ZSETTLE].value;
		zvgDmaPutc( zcSETTLE);
		zvgDmaPutc( ZvgMon.settle);
	}

	if (Param[ZMAXINT].value != ZvgMon.max_i)
	{	ZvgMon.max_i = Param[ZMAXINT].value;
		zvgDmaPutc( zcMAX_I);
		zvgDmaPutc( ZvgMon.max_i);
	}

	if (Param[ZMININT].value != ZvgMon.min_i)
	{	ZvgMon.min_i = Param[ZMININT].value;
		zvgDmaPutc( zcMIN_I);
		zvgDmaPutc( ZvgMon.min_i);
	}

	if (Param[ZPOINT].value != ZvgMon.point_i)
	{	ZvgMon.point_i = Param[ZPOINT].value;
		zvgDmaPutc( zcPOINT_I);
		zvgDmaPutc( ZvgMon.point_i);
	}

	if (Param[ZSIZE].value != ZvgMon.scale)
	{	ZvgMon.scale = Param[ZSIZE].value;
		zvgDmaPutc( zcSCALE);
		zvgDmaPutc( ZvgMon.scale);
	}
}

/*****************************************************************************
* Read the 'ZvgMon' into the 'Param[]' array.
*****************************************************************************/
void paramRead( void)
{
	Param[ZSHIFT].value = ZvgMon.zShift;
	Param[ZOSHOOT].value = ZvgMon.oShoot;
	Param[ZJUMP].value = ZvgMon.jumpFactor;
	Param[ZSETTLE].value = ZvgMon.settle;
	Param[ZMAXINT].value = ZvgMon.max_i;
	Param[ZMININT].value = ZvgMon.min_i;
	Param[ZPOINT].value = ZvgMon.point_i;
	Param[ZSIZE].value = ZvgMon.scale;
}

/*****************************************************************************
* Print a single 'Param[]' value.
*
* Called with:
*    *prm = Pointer to a member of a Param_s[] array.
*    attr = Attribute used to print parameter's value.
*    sc   = Character to print in front of the value.
*    ec   = Character to print after the value.
*****************************************************************************/
#if defined(WIN32)
#elif defined(linux)

void pParam( Param_s *prm, short attr, char sc, char ec)

#else // DOS

void pParam( Param_s *prm, char attr, char sc, char ec)

#endif // OS
{
	gotoxy( prm->col, prm->row);
	cputs( prm->title);

	gotoxy( prm->vcol, prm->vrow);
	textattr( attr);
	cprintf( prm->fmt, sc, prm->value, ec);
	textattr( ATTR_DEF);
}

/*****************************************************************************
* Print a all parameters to the screen.
*
* Called with:
*    sel = index of selected parameter to be highlighted, if == -1, no highlight.
*****************************************************************************/
void pParamAll( int sel)
{
	uint	ii;

	// display all settings

	for (ii = 0; ii < sizeof( Param) / sizeof( *Param); ii++)
	{
		if (sel == ii)
			pParam( Param + ii, ATTR_SEL, ' ', ' ');

		else
			pParam( Param + ii, ATTR_DEF, ' ', ' ');
	}
}

/*****************************************************************************
* Print a string in the horizontal center of the screen.
*
* Called with:
*    row = Row of string.
*    ss  = String to print.
*****************************************************************************/
void pCenter( int row, char *ss)
{
	int	col;

	col = strlen( ss);
	col = ((80 - col) / 2) + 1;

	gotoxy( col, row);
	cputs( ss);
}

/*****************************************************************************
* Print an informative message.
*
* Called with:
*    row = Row of string.
*    ss  = String to print.
*****************************************************************************/
void pMsg( char *ss)
{
	gotoxy( 1, 20);

	clreol();
	textattr( ATTR_MSG);
	pCenter( 20, ss);
	textattr( ATTR_DEF);
}

/*****************************************************************************
* Initialize ...
*
* Called with:
*****************************************************************************/
#if defined(WIN32)
#elif defined(linux)

void init_zvgtweak( void)
{
	initscr();	// clear window
	start_color();
	cbreak();
	noecho();
	nonl();
	intrflush( stdscr, FALSE);
	keypad( stdscr, TRUE);
	
	init_pair( DEF_PAIR, COLOR_CYAN, COLOR_BLUE);
	init_pair( SEL_PAIR, COLOR_YELLOW, COLOR_BLACK);
	init_pair( MSG_PAIR, COLOR_BLACK, COLOR_RED);

	nodelay( stdscr, TRUE);
}

#else // DOS
#endif // OS

/*****************************************************************************
* 
*****************************************************************************/
void displayMenu( void)
{
	pCenter( 1, "ZVGTWEAK UTILITY VERSION 0.9");

#if defined(WIN32)
#elif defined(linux)

	mvaddstr( 15, 1, "Commands:");
	mvaddstr( 16, 3, "Up/Down-Select a parameter.            "
					 "Left/Right-Update a parameter.");
	mvaddstr( 17, 3, "I-Initialize ZVG to factory settings.  "
					 "X-Exit to DOS (or use ESC).");
	mvaddstr( 18, 3, "L-Load values from EEPROM.             "
					 "S-Save values to EEPROM.");

#else // DOS

	gotoxy( 1, 15);
	cputs( "Commands:\r\n");
	cputs( " Up/Down-Select a parameter.            Left/Right-Update a parameter.\r\n");
	cputs( " I-Initialize ZVG to factory settings.  X-Exit to DOS (or use ESC).\r\n");
	cputs( " L-Load values from EEPROM.             S-Save values to EEPROM.\r\n");
//	cputs( " R-Read values from a config file.      W-Write values to a config file.\r\n");

#endif // OS

	pCenter( 21, "(c) Copyright 2003, Zektor, LLC.  All rights reserved.");
}

/*****************************************************************************
* MAIN
*****************************************************************************/
int main( int argc, char *argv[])
{
	uint	err;
	uint	paramsChanged;
	int	cc;
	int	pIdx, o_pIdx, clearCount;

	// Setup the ZVG subsytem, this routine initializes the timers and calibrates any timing
	// needed by the the zvgPort routines.

	err = zvgFrameOpen();

	if (err)
	{	zvgError( err);
		exitProg();
	}

	zvgBanner( ZvgSpeeds, &ZvgID);

	fputs( "\n\nPress any key to continue...", stdout);
	fflush( stdout);

#if defined(WIN32)
#elif defined(linux)

	fgetc( stdin);
	init_zvgtweak();

#else // DOS

	getch();

#endif // OS

	// setup the default screen colors, clear screen and turn off cursor

	textattr( ATTR_DEF);
#if defined(WIN32)
#elif defined(linux)

	initscr();
//!!!	_setcursortype( _NOCURSOR);

#else // DOS

	clrscr();
	_setcursortype( _NOCURSOR);

#endif // OS

	displayMenu();

	err = errOk;

	paramRead();							// move ZVG variables into local variables
	pParamAll( -1);

	// Dump frame over and over to the ZVG while looking for key presses

	tmrSetFrameRate( FRAMERATE);		// set frame rate

	pIdx = 0;
	o_pIdx = -1;
	err = errOk;

	while (err == errOk)
	{
		drawDisplay();

		// if parameter pointer has moved, clear old value and highlight new value

		if (o_pIdx != pIdx)
		{	pParam( Param + pIdx, ATTR_SEL, ' ', ' ');

			// if old index was valid, clear it, otherwise do nothing

			if (o_pIdx != -1)
				pParam( Param + o_pIdx, ATTR_DEF, ' ', ' ');

			o_pIdx = pIdx;
		}

		// check to see if informative message has timed out

		if (clearCount > 0)
		{	clearCount--;

			if (clearCount == 0)
			{	gotoxy( 1, 24);
				clreol();
			}
		}

		// check for a keypress

#if defined(WIN32)
#elif defined(linux)
		// Remember we're in a nodelay state; getch returns immediately.
		cc = getch();
		if (cc != ERR)	// Had a key press?
#else // DOS
		if (kbhit())
#endif // OS
		{
			// read key

#if defined(WIN32)
#elif defined(linux)
#else // DOS
			cc = getkey();
#endif // OS

			switch (cc)
			{
			case K_Right:
				if (Param[pIdx].value < 255)
				{	Param[pIdx].value++;
					paramSync();
					o_pIdx = -1;					// cause value to be updated
				}
				break;

			case K_Left:
				if (Param[pIdx].value > 0)
				{	Param[pIdx].value--;
					paramSync();
					o_pIdx = -1;					// cause value to be updated
				}
				break;

			case K_Down:
				if (pIdx < (sizeof( Param) / sizeof( *Param)) - 1)
					pIdx++;

				break;

			case K_Up:
				if (pIdx > 0)
					pIdx--;

				break;

				// Save the current value in the ZVG, to the ZVG's EEPROM.
				// This allow the parameters to survive a power failure.
				// FYI: There are separate EEPROM locations for each table
				// speed.

			case 'S':
			case 's':
				zvgDmaPutc( zcSAVE_EE);
				pMsg( " Current values saved to EEPROM. ");
				clearCount = FRAMERATE * 4;				// display for 4 seconds
				break;

				// Reload the values from the EEPROM, incase you've screwed
				// something up and want to get back to where you were.

			case 'L':
			case 'l':
				zvgDmaPutc( zcLOAD_EE);
				pMsg( " Monitor values re-loaded from EEPROM. ");
				textattr( ATTR_DEF);
				clearCount = FRAMERATE * 4;				// display for 4 seconds
				paramsChanged = zTrue;						// indicate parameters need reading
				break;

				// Reset the values to the factor default, incase you've *really*
				// screwed something up!  If the values save in EEPROM are
				// screwed up you will have to save these values back to EEPROM
				// if you don't want them reappearing.

			case 'I':
			case 'i':
				zvgDmaPutc( zcRESET_MON);
				pMsg( " Monitor values initialized to factory defaults. ");
				clearCount = FRAMERATE * 4;				// display for 4 seconds
				paramsChanged = zTrue;						// indicate parameters need reading
				break;
			}

			if (cc == 0x1B || cc == 'X' || cc == 'x')
				break;							// if ESC pressed, leave loop
		} // End of check for keyboard input

		// wait for start of next frame

		tmrWaitFrame();

		err = zvgFrameSend();				// send frame of DMA and swap buffers

		// if parameters were changed, read new parameter into parameter structure

		if (paramsChanged)
		{	zvgReadMonitorInfo( &ZvgMon);	// read info from ZVG
			paramRead();						// update our local database
			pParamAll( pIdx);					// reprint local variables
			paramsChanged = zFalse;
		}
	} // End of while loop

	// restore ZVG stuff

	zvgFrameClose();

	// set back to original video values, clear screen, turn the cursor back on

#if defined(WIN32)
#elif defined(linux)

//!!!	normvideo();
	initscr();
//!!!	_setcursortype( _NORMALCURSOR);

#else // DOS

	normvideo();
	clrscr();
	_setcursortype( _NORMALCURSOR);

#endif // OS

	// if we exited because of an error, print the error.

	if (err)
		zvgError( err);

	exitProg();
	return (0);
}

//----------------------------------------------------------------------------
// Function name: Various zvg stubs
// Description: for debugging without having to connect to the Zektor card
//		or use any of the zvg driver code.
// Parameters: 
// Creator: JBJarvis 10/2007
//----------------------------------------------------------------------------
#ifdef DEBUG_WITHOUT_ZEKTOR_DRIVER

static uint yFrameOpen( void )					{ return errOk; }
static void yFrameClose( void )					{ return; }
static uint yFrameSend( void)					{ return errOk; }
static uint yGetABufferThreaded( void)			{ return errOk; }
static void yEncSetColor( uint newcolor)		{ return; }
static void yEncSetRGB15(unsigned int, unsigned int, unsigned int)
												{ return; }
static uint yDmaPutc( uchar cc)					{ return errOk; }
static void	yError( uint err)					{ return; }
static uint yFrameVector( int xStart, int yStart, int xEnd, int yEnd)
												{ return errOk; }
static void	ytmrSetFrameRate( int fps)			{ return; }
static uint ytmrWaitFrame()						{ return 1; }
static void yBanner( ZvgSpeeds_a speeds, ZvgID_s *id)
												{ return; }

static uint yReadMonitorInfo( ZvgMon_s *mon)
{	// For the heck of it, just put in the values we want.
	mon->point_i = 80;		// point intensity
	mon->zShift = 0;		// shift value
	mon->oShoot = 47;		// overshoot value
	mon->jumpFactor = 37;	// multiplier used to calculate between vector jump timing
	mon->settle = 6;		// minimum value of a between vector jump, used for settling time
	mon->min_i = 100;		// minimum color value allowed
	mon->max_i = 215;		// maximum color value allowed
	mon->scale = 80;		// Size of screen
	mon->flags = 0;			// Status flags
	mon->cksum = 0;			// Fletcher's check digits of monitor data
	return errOk;
}

#endif // DEBUG_WITHOUT_ZEKTOR_DRIVER

