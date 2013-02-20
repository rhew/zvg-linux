/*****************************************************************************
* Print ZVG banner.
*
* This routine will print all the vital ZVG parameters to STDOUT when called.
*
* Author:  Zonn Moore
* Created: 06/30/03
*
* History:
*
* (c) Copyright 2002-2004, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#ifdef WIN32
	#include <windows.h>

#elif defined(linux)
#else // DOS
#endif

#include	<stdio.h>
#include	"zvgport.h"

// Decode the 16 bit version number returned from the ZVG.

/*****************************************************************************
* 
*
*****************************************************************************/
static char *sVer( uint ver)
{
	static char verStr[8];

	if (ver & 0x000F)
		sprintf( verStr, "%u.%u%c", (ver >> 11) & 0x1F, (ver >> 4) & 0x7F, (ver & 0x0F) + ('a' - 1));

	else
		sprintf( verStr, "%u.%u", (ver >> 11) & 0x1F, (ver >> 4) & 0x7F);

	return (verStr);
}

/*****************************************************************************
* 
*
*****************************************************************************/
void zvgBanner( ZvgSpeeds_a speeds, ZvgID_s *id)
{
	uint	port, dma, dmaMode, irq, monFlags;
	uint	monSpeed;

	// get port stuff

	zvgGetPortInfo( &port, &dma, &dmaMode, &irq, &monFlags);

	// lookup monitor speed in speed table based on switch settings

	monSpeed = speeds[(id->sws >> 4) & 0x03];

	// print banner

	fprintf( stdout, "\nZVG found on PORT=%03X, ", port);

	if (dmaMode != 0)
		fprintf( stdout, "DMA=%u, DMA Mode=%u, IRQ=%u.", dma, dmaMode, irq);

	else
		fputs( "Using polled mode (DMA disabled).", stdout);

	fputs( "\n\nFirmware Version:", stdout);

	fprintf( stdout, "\n   Controller:   %s", sVer( id->fVer));
	fprintf( stdout, "\n   Bootloader:   %s", sVer( id->bVer));
	fprintf( stdout, "\n   Vector Timer: %s", sVer( id->vVer));

	fprintf( stdout, "\n\nMonitor Settings (%u): ", monFlags);
	fputs( "\n   Type:           ", stdout);

	if (monFlags & MONF_BW)
		fputs( "B&W", stdout);

	else
		fputs( "Color", stdout);

	fputs( "\n   Spotkill Logic: ", stdout);

	if (monFlags & MONF_SPOTKILL)
		fputs( "Enabled", stdout);

	else
		fputs( "Disabled", stdout);

	fputs( "\n   Orientation:    ", stdout);

	if ((monFlags & MONF_FLIPX) && (monFlags & MONF_FLIPY))
		fputs( "Flip X and Y axes", stdout);

	else if (monFlags & MONF_FLIPX)
		fputs( "Flip X axis", stdout);

	else if (monFlags & MONF_FLIPY)
		fputs( "Flip Y axis", stdout);

	else
		fputs( "Do not flip X or Y axes", stdout);

	fputs( "\n   Overscan:       ", stdout);

	if (monFlags & MONF_NOOVS)
		fputs( "Do not overscan monitor", stdout);

	else
		fputs( "Overscanning allowed", stdout);

	fprintf( stdout, "\n   Speed:          %uus per inch", monSpeed);

	fflush( stdout);
}
