#ifndef _ZVGCMDS_H_
#define _ZVGCMDS_H_
/*****************************************************************************
* Define ZVG command set data.
*
* Author:       Zonn Moore
* Created:      11/06/02
* Last Updated: 05/21/03
*
* History:
*
* (c) Copyright 2002-2004, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#define		zINIT_COLOR	0x7BEF	// The default color used by the ZVG firmware

// Bitmaps that make up the ZVG vector commands

#define		zbHZVT		0x04		// Bit set if a horizontal or vertical line being drawn
#define		zbVERT		0x02		// Bit set if above HZVT line is Vertical

#define		zbYLEN		0x04		// Bit set if Y is given as length
#define		zbRATIO		0x08		// Bit set if RATIO value given
#define		zbVECTOR		0x10		// Bit set if VECTOR command (as opposed to POINT)
#define		zbSHORT		0x20		// Bit set if 8 bit SHORT offsets given
#define		zbCOLOR		0x40		// Bit set if color information given
#define		zbABS			0x80		// Bit set if starting X/Y given.

// List of extended commands

#define		zcEXTENDED	0xE0		// extended commands

#define		zcNOP			0xE0		// Do nothing
#define		zcBLINK		0xE1		// Blink VTG LED
#define		zcZSHIFT		0xE2		// Set Z-Shift
#define		zcOSHOOT		0xE3		// Set Overshoot
#define		zcJUMP		0xE4		// Set JUMP factor
#define		zcSETTLE		0xE5		// Set min Settling time
#define		zcPOINT_I	0xE6		// Set POINT Intensity
#define		zcMIN_I		0xE7		// Set MIN DAC Intensity
#define		zcMAX_I		0xE8		// Set MAX DAC Intensity
#define		zcSCALE		0xE9		// Set Scale size
#define		zcSAVE_EE	0xEA		// Save current setting into EEPROM
#define		zcLOAD_EE	0xEB		// Restore current setting from EEPROM
#define		zcREAD_MON	0xEC		// Send current monitor settings to Host
#define		zcRESET_MON	0xED		// Reset monitor to factory default settings
#define		zcREAD_SPD	0xEE		// Read speed table values

#define		zcCENTER		0xEF		// Center beam

#ifdef __cplusplus
}
#endif

#endif
