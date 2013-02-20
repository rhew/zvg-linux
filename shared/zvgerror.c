/*****************************************************************************
* Single routine use to print a return error from the ZVG library.
*
* Placed into a seperate file to allow using a different 'zvgError()' that
* would be more platform dependent.
*
* Author:  Zonn Moore
* Created: 05/21/03
*
* History:
*    07/03/03
*       Rewrote error messages to be less generic and a bit more like help
*       messages.
*
* (c) Copyright 2003-2004, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#ifdef WIN32
	#include <windows.h>

#elif defined(linux)
#else // DOS
#endif

#include	<stdio.h>
#include	"zstddef.h"
#include	"zvgport.h"

/*****************************************************************************
* Print an error returned by the ZVG routines.
*
* Returns:
*    0 - Ok to continue.
*    1 - Fatal error, exit program.
*****************************************************************************/
void zvgError( uint err)
{
	fprintf( stdout, "\nZVG: ");

	switch (err)
	{
	case errOk:
		fputs( "No Error", stdout);
		break;

	case errNoEnv:
		fputs( "No 'ZVGPORT=' environment variable found!", stdout);
		break;

	case errEnvPort:
		fputs( "The Port address given in the 'ZVGPORT=' environment variable\n", stdout);
		fputs( "     is invalid. Fix port address parameter 'Pxxx', in 'ZVGPORT='", stdout);
		break;

	case errEnvDMA:
		fputs( "DMA channel given in 'ZVGPORT=' environment variable is invalid.\n", stdout);
		fputs( "     Fix DMA parameter 'Dx', in 'ZVGPORT='.", stdout);
		break;

	case errEnvMode:
		fputs( "DMA mode given in 'ZVGPORT=' environment variable is invalid.\n", stdout);
		fputs( "     Fix DMA Mode parameter 'Dx,x', in 'ZVGPORT='.", stdout);
		break;

	case errEnvIRQ:
		fputs( "IRQ number given in 'ZVGPORT=' environment variable is invalid.\n", stdout);
		fputs( "     Fix IRQ parameter 'Ix', in 'ZVGPORT='.", stdout);
		break;

	case errEnvMon:
		fputs( "Monitor type given in 'ZVGPORT=' environment variable is invalid.\n", stdout);
		fputs( "     Fix Monitor Type parameter 'Mxx', in 'ZVGPORT='.", stdout);
		break;

	case errNoPort:
		fputs( "No PORT address specified in the 'ZVGPORT=' environment variable!\n", stdout);
		fputs( "     'ZVGPORT=' must include a 'Pxxx', port parameter.", stdout);
		break;

	case errNotEcp:
		fputs( "No ECP compatible port was found at the address given in the\n", stdout);
		fputs( "     'ZVGPORT=' environment variable. Verify the ECP port address\n", stdout);
		fputs( "     is correct, and verify the BIOS setting of the parallel port\n", stdout);
		fputs( "     is set to 'ECP' mode.", stdout);
		break;

	case errNoDma:
		fputs( "No DMA channel given in the 'ZVGPORT=' environment variable,\n", stdout);
		fputs( "     and unable to read a DMA value from the ECP port.\n", stdout);
		fputs( "     'ZVGPORT=' must include a 'Dx', DMA parameter.", stdout);
		break;

	case errInvDma:
		fputs( "The DMA channel given is out of range (must be between 0-3).\n", stdout);
		fputs( "     Fix DMA parameter 'Dx', in 'ZVGPORT='.", stdout);
		break;

	case errNoIrq:
		fputs( "No IRQ level given in the 'ZVGPORT=' environment variable,\n", stdout);
		fputs( "     and unable to read an IRQ value from the ECP port.\n", stdout);
		fputs( "     'ZVGPORT=' must include an 'Ix', IRQ parameter.", stdout);
		break;

	case errInvIrq:
		fputs( "The IRQ level given is not valid.\n", stdout);
		fputs( "     Fix IRQ parameter 'Ix', in 'ZVGPORT='.", stdout);
		break;

	case errXmtErr:
		fputs( "Unable to communicate with the ZVG. Verify DMA settings are correct.\n", stdout);
		break;

	case errMemory:
		fputs( "Could not allocate enough memory.", stdout);
		break;

	case errBfrFull:
		fputs( "Too many vectors in a frame caused the DMA buffer to overflowed!", stdout);
		break;

	case errEcpWord:
		fputs( "The ECP port could not be set to the 8 bit mode, the 'ZVGPORT='\n", stdout);
		fputs( "     environment variable must specify an 8 bit ECP port!", stdout);
		break;

	case errEcpFailed:
		fputs( "No ZVG was found at the ECP port given in the 'ZVGPORT='\n", stdout);
		fputs( "     environment variable. Verify the ECP port address is\n", stdout);
		fputs( "     correct, and that the ZVG is powered on.", stdout);
		break;

	case errEcpNoData:
		fputs( "Unable to read data from the ZVG, check for loose cables or\n", stdout);
		fputs( "     power supply connections.", stdout);
		break;

	case errEcpBadData:
		fputs( "Bad information, or wrong number of bytes returned from the ZVG.", stdout);
		break;

	case errEcpTimeout:
		fputs( "A timeout occurred while waiting for the ZVG, check for loose\n", stdout);
		fputs( "     cables or power supply connections.", stdout);
		break;

	case errEcpBusy:
		fputs( "A timeout occurred while the ECP port was indicating busy, check\n", stdout);
		fputs( "     for loose cables or power supply connections.", stdout);
		break;

	case errEcpComm:
		fputs( "Unexpected data from the ZVG.", stdout);
		break;

	case errEcpBadMode:
		fputs( "An attempt to read/write to the ECP port in the wrong mode.", stdout);
		break;

	case errEcpToSpp:
		fputs( "The ZVG has stopped communicating with the PC, check for loose\n", stdout);
		fputs( "     cables or power supply connections.", stdout);
		break;

	case errZvgRomCS:
		fputs( "Bad checksum in data, unable to update EEPROM!", stdout);
		break;

	case errZvgRomNE:
		fputs( "Data written / compared did not match EEPROM memory!", stdout);
		break;

	case errZvgRomTI:
		fputs( "Writing to EEPROM not allowed, check SETUP jumper!", stdout);
		break;

	case errUnknownID:
		fputs( "Unrecognized version string returned from the ZVG. Verify the ECP\n", stdout);
		fputs( "     at the port address given in the 'ZVGPORT=' environment variable\n", stdout);
		fputs( "     is connected to a ZVG.", stdout);
		break;

#if defined(WIN32)
#elif defined(linux)

	case errIOPermFail:
		fputs( "The call to ioperm() failed to gain access to the port numbers requested.\n", stdout);
		fputs( "     Root privilege is required for this operation.\n", stdout);
		break;

	case errNoBuffer:
		fputs( "No DMA buffer available - NON-FATAL\n", stdout);
		break;

	case errThreadFail:
		fputs( "Failed to create IO thread.\n", stdout);
		break;

#else // DOS
#endif // OS

	default:
		{
			char msg[200];
			sprintf( msg, "Unknown ERROR code=%d!!\n", err); 
			fputs( msg, stdout);
		}
		break;
	}

	fputs( "\n", stdout);
	fflush( stdout);
}
