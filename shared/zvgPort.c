/*****************************************************************************
* Routines to talk to the ZVG board from DOS through a 1284 compliant
* parallel port.
*
* Author:  Zonn Moore
* Created: 11/06/02
*
* History:
*    07/01/03
*       Added a bit to monitor type in 'ZVGPORT=' to indicate a B&W monitor
*       is connected to the ZVG, to allow Color to B&W mix down.
*
*    06/27/03
*       Changes to 'ZVGPORT=' to add monitor type, and changed the way
*       DMA mode is given. DMA Mode 0 used to disables DMA.
*       DMA channel = 0 is now valid, though not part of the ECP standard.
*
*    06/26/03
*       Re-separated the Encoder and Port driver routines, they were
*       becoming a bit intertwined.
*
*    06/09/03
*       Allow for a non-DMA, polled mode, by specifying DMA=0.
*
*    05/21/03
*       Did a full overhaul that added DMA to the driver. Simplified the
*       setup of the ZVG, it now looks for an environment variable to
*       determine port address, DMA and IRQ.
*
* (c) Copyright 2002-2004, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#include	<sys/io.h>

#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<stdio.h>

#include	"zstddef.h"
#include	"zvgCmds.h"
#include	"timer.h"
#include	"zvgPort.h"
//#include	"zvgError.h"

ZvgIO_s	ZvgIO;							// Structure used to communicate with ZVG

static const uchar IrqLookup[] = { 0, 7, 9, 10, 11, 14, 15, 5};

/*****************************************************************************
* Wait for the DSR to equal the given value.
*
* Called with:
*    mask   = Bitmask used to mask DSR before comparison.
*    bitVal = Bit values, that when matched, causes routine to return.
*    ms     = Timeout in milliseconds.
*
* Returns all (unmasked bits) of DSR when a match is found, or returns
* ZVG_TIMEOUT (-1) if routine timed out while waiting for match.
*****************************************************************************/
static uint waitForDsrEQ( uchar mask, uchar bitVal, ulong ms)
{
	uchar		testVal;
	uchar		readVal;
	long long int	timer;
	bool		foundF;

	testVal = (bitVal ^ DSR_InvMask) & mask;
	foundF = zFalse;
	//tmrReadTimer(&timer);					// read timer value
	timer = tmrReadTimer();
	// loop until match found, or timeout

	do
	{
		if (((readVal = inportb( ZvgIO.ecpDsr)) & mask) == testVal)
		{	foundF = zTrue;				// indicate match was found
			break;
		}
	} while (!tmrTestMillis( timer, ms));

	if (!foundF)				
		return (ZVG_TIMEOUT);			// match not found, return error

	return (readVal ^ DSR_InvMask);	// fix logic and return value
}

/*****************************************************************************
* Wait for the DSR to NOT equal the given value.
*
* Called with:
*    mask   = Bitmask used to mask DSR before comparison.
*    bitVal = Bit values, that when not matched, causes routine to return.
*    ms     = Timeout in milliseconds.
*
* Returns all (unmasked bits) of DSR when the DSR does NOT match the given
* 'bits' value, or returns ZVG_TIMEOUT (-1) if routine timed out while waiting
* for a mismatch.
*****************************************************************************/
static uint	waitForDsrNE( uchar mask, uchar bitVal, ulong ms)
{
	uchar		testVal;
	uchar		readVal;
	long long int		timer;
	bool		foundF;

	testVal = ( bitVal ^ DSR_InvMask) & mask;
	foundF = zFalse;
	//tmrReadTimer(&timer);
	timer = tmrReadTimer();
	do
	{
		if (((readVal = inportb( ZvgIO.ecpDsr)) & mask) != testVal)
		{	foundF = zTrue;
			break;
		}
	} while (!tmrTestMillis( timer, ms));

	if (!foundF)
		return (ZVG_TIMEOUT);

	return (readVal ^ DSR_InvMask);
}

/*****************************************************************************
* Wait for the ECR to equal the given value.
*
* Called with:
*    mask   = Bitmask used to mask DSR before comparison.
*    bitVal = Bit values, that when matched, causes routine to return.
*    ms     = Timeout in milliseconds.
*
* Returns all (unmasked bits) of ECR when a match is found, or returns
* ZVG_TIMEOUT (-1) if routine timed out while waiting for match.
*****************************************************************************/
static uint waitForEcrEQ( uchar mask, uchar bitVal, ulong ms)
{
	uchar			testVal;
	uchar			readVal;
	long long int	timer;
	bool			foundF;

	testVal = bitVal & mask;
	foundF = zFalse;
	//tmrReadTimer(&timer);
	timer = tmrReadTimer();
	do
	{
		if (((readVal = inportb( ZvgIO.ecpEcr)) & mask) == testVal)
		{	foundF = zTrue;
			break;
		}
	} while (!tmrTestMillis( timer, ms));

	if (!foundF)
		return (ZVG_TIMEOUT);

	return (readVal);
}

/*****************************************************************************
* Set bits in the DCR register
*****************************************************************************/
static void sdcr( uchar bits)
{
	ZvgIO.ecpDcrState |= bits;								// set control bits
	ZvgIO.ecpDcrState ^= DCR_InvMask & bits;			// invert them if needed
	outportb( ZvgIO.ecpDcr, ZvgIO.ecpDcrState);		// set new control line values
}

/*****************************************************************************
* Clear bits in the DCR register
*****************************************************************************/
static void cdcr( uchar bits)
{
	ZvgIO.ecpDcrState &= ~bits;							// clear control bits
	ZvgIO.ecpDcrState ^= DCR_InvMask & bits;			// invert them if needed
	outportb( ZvgIO.ecpDcr, ZvgIO.ecpDcrState); 			// set new control line values
}

/*****************************************************************************
* Set & Clear bits in the DCR in one update
* 
* First arg sets bits, 2nd clears
*****************************************************************************/
static void scdcr( uchar setBits, uchar clearBits)
{
	ZvgIO.ecpDcrState |= setBits;						// set control bits
	ZvgIO.ecpDcrState &= ~clearBits;					// clear control bits

	// invert them if needed

	ZvgIO.ecpDcrState ^= DCR_InvMask & (setBits | clearBits);
	outportb( ZvgIO.ecpDcr, ZvgIO.ecpDcrState);	// set new control line values
}

/*****************************************************************************
* Write to the DCR register
*****************************************************************************/
static void wdcr( uchar bits)
{
	ZvgIO.ecpDcrState = bits ^ DCR_InvMask;
	outportb( ZvgIO.ecpDcr, ZvgIO.ecpDcrState);
}

/*****************************************************************************
* Read from the DSR register
*****************************************************************************/
static uchar rdsr( void)
{
	return (inportb( ZvgIO.ecpDsr) ^ DSR_InvMask);
} 

/*****************************************************************************
* Return immediately to the compatibility mode.  Something has gone wrong,
* there has been an event out of sequence.  This routine is called to reset
* everything.
*****************************************************************************/
static uint compatibility( void)
{
	ZvgIO.ecpFlags &= ~(ECPF_ECP | ECPF_NIBBLE);		// turn off MODE flags

	// Set the status lines to compatibility mode:
	//
	//    nSelectIn - Low
	//    nAutoFeed - High
	//    nStrobe   - High
	//    nInit     - High
	//

	wdcr( DCR_nAutoFeed | DCR_nStrobe | DCR_nInit);    
	return (errOk);
}

/*****************************************************************************
* Do negotiation sequence, does not check to see if mode given was accepted
* or not, just returns when negotiations successful, or with an error code
* if an error occured.
*****************************************************************************/
static uint negotiate_1284( uchar mode)
{
	// Setup a negotiation request, setup for at least 1us

	outportb( ZvgIO.ecpData, mode);					// setup data lines
	outportb( ZvgIO.ecpData, mode);
	outportb( ZvgIO.ecpData, mode);
	outportb( ZvgIO.ecpData, mode);

	// Set 1284_Active high and HostBusy low

	scdcr( DCR_1284_Active, DCR_HostBusy);

	// look for awhile and see if the peripheral will respond

	if (waitForDsrEQ( DSR_AckDataReq|DSR_PtrClk|DSR_nDataAvail|DSR_XFlag,
			DSR_AckDataReq|DSR_nDataAvail|DSR_XFlag, PERIPH_TIMEOUT) == ZVG_TIMEOUT)
		goto WaitTimeout;

	// set HostClk low for 1us

	cdcr( DCR_HostClk);
	outportb( ZvgIO.ecpDcr, ZvgIO.ecpDcrState);
	outportb( ZvgIO.ecpDcr, ZvgIO.ecpDcrState);
	outportb( ZvgIO.ecpDcr, ZvgIO.ecpDcrState);

	// finish pulse and set HostBusy high

	sdcr( DCR_HostClk | DCR_HostBusy);

	// wait for peripheral to respond

	if (waitForDsrNE( DSR_PtrClk, 0, PERIPH_TIMEOUT) == ZVG_TIMEOUT)
		goto WaitTimeout;

	// negotiations successful, return

	return (errOk);

WaitTimeout:
	compatibility();
	return (errEcpFailed);		// timeout is not an error, just a non-1284 device
}

/*****************************************************************************
* Do a normal negotiated terminatation to return to the Compatibility mode.
*
* Follows the standard 1284 termination sequence.
*****************************************************************************/
static uint terminate_1284( void)
{
	ZvgIO.ecpFlags &= ~(ECPF_ECP | ECPF_NIBBLE);		// turn off MODE flags

	// set port back to SPP mode

	outportb( ZvgIO.ecpEcr, ECR_SPP_mode | ECR_nErrIntrEn | ECR_serviceIntr);

	// release 1284 active lines

	scdcr( DCR_HostBusy | DCR_HostClk, DCR_1284_Active);

	// wait for P. to respond. XFlag is also inverted but we're not checking
	// for that.

	if (waitForDsrEQ( DSR_PtrClk, 0, PERIPH_TIMEOUT) == ZVG_TIMEOUT)
		goto WaitTimeout;

	// Ack P.

	cdcr( DCR_HostBusy);

	// Wait for P. to set itself back to compatibility mode

	if (waitForDsrEQ( DSR_PtrClk, DSR_PtrClk, PERIPH_TIMEOUT) == ZVG_TIMEOUT)
		goto WaitTimeout;

	// indicate we're also ready to move on

	sdcr( DCR_HostBusy | DCR_nInit);
	return (errOk);

WaitTimeout:
	compatibility();
	return (errOk);			// terminate always works, even if a timeout occured
}

/*****************************************************************************
* Get a byte of data using the NIBBLE mode.  Checks to see if data is
* available.  If not, it will return a 'errEcpNoData' error.
*
* Reverse NIBBLE mode must have already been negotiated, or an error will
* result.
*****************************************************************************/
static uint getByteNb( uchar *byte)
{
	uint	loNib, hiNib;

	// check for proper mode

	if (!(ZvgIO.ecpFlags & ECPF_NIBBLE))
		return (errEcpBadMode);		// not in Nibble mode

	// has data just become available?

	if (rdsr() & DSR_AckDataReq)
	{	cdcr( DCR_HostBusy);			// indicate host is not busy

		// check to see if the peripheral is indicating that data is available
		// wait for awhile.  Check for a full second to see if data is available.

		if (waitForDsrNE( DSR_nDataAvail, DSR_nDataAvail, PERIPH_WAIT) == ZVG_TIMEOUT)
		{	sdcr( DCR_HostBusy);		// host is once again busy
			terminate_1284();			// if no data, leave NIBBLE mode
			return (errEcpNoData);
		}

		sdcr( DCR_HostBusy);			// indicate host has received the IRQ

		// wait for P. to ack

		if (waitForDsrNE( DSR_AckDataReq, DSR_AckDataReq, PERIPH_TIMEOUT) == ZVG_TIMEOUT)
			goto WaitTimeout;	
	}
	cdcr( DCR_HostBusy);				// indicate host is not busy

	// wait for P. response

	if (waitForDsrNE( DSR_PtrClk, DSR_PtrClk, PERIPH_TIMEOUT) == ZVG_TIMEOUT)
		goto WaitTimeout;

	// when data is ready, read it.

	loNib = rdsr();

	// indicate we've read the data

	sdcr( DCR_HostBusy);

	// shift the bits around to build a real nibble

	loNib = ((loNib >> 3) & 0x07) | ((loNib & DSR_Busy) >> 4);

	// wait for the P. to know we've read the data

	if (waitForDsrNE( DSR_PtrClk, 0, PERIPH_TIMEOUT) == ZVG_TIMEOUT)
		goto WaitTimeout;

	// indicate were ready for more data
 
	cdcr( DCR_HostBusy);

	// wait for data

	if (waitForDsrNE( DSR_PtrClk, DSR_PtrClk, PERIPH_TIMEOUT) == ZVG_TIMEOUT)
		goto WaitTimeout;

	// read next nibble

	hiNib = rdsr();

	// tell the P. we've got it.

	sdcr( DCR_HostBusy);

	// shift into real nibble

	hiNib = ((hiNib << 1) & 0x70) | (hiNib & DSR_Busy);

	// wait for P. to catch up

	if (waitForDsrNE( DSR_PtrClk, 0, PERIPH_TIMEOUT) == ZVG_TIMEOUT)
		goto WaitTimeout;

	// return the full byte

	*byte = hiNib | loNib;
	return (errOk);

WaitTimeout:
	compatibility();
	return (errEcpTimeout);
}

/*****************************************************************************
* Check if data is available, assumes we are in the reverse nibble mode.
*
* This routine should only be called in reverse NIBBLE mode.
*****************************************************************************/
static uint isDataAvail( void)
{
	// check for Nibble mode data available

	return (!(rdsr() & DSR_nDataAvail));
}

/*****************************************************************************
* Look for the 'ZVGPORT=' environment variable and parse it.
*
* Returns:
*    errCode
*****************************************************************************/
uint zvgEnv( uint *portAdr, uint *monitor)
{
	char	*env, *envP, cmd;

	env = getenv("ZVGPORT");				// look for environment variable

	if (env == NULL)
		return (errNoEnv);					// no environment variable found

	// Look for 'P', 'D' or 'I' attributes
	envP = env;									// point to environment string

	while (*envP != '\0')
	{
		while (*envP == ' ')					// skip leading blanks
			envP++;

		cmd = toupper( *envP);
		envP++;									// skip command character

		while (*envP == ' ')
			envP++;								// skip blanks

		// decode attribute

		switch (cmd)
		{
		case 'P':								// check for 'P'ort address
			if (!isdigit( *envP))
				return (errEnvPort);			// bad environment port value

			*portAdr = strtoul( envP, &envP, 16);	// read port address
			break;

		case 'M':								// or check for 'M'onitor type
			if (!isdigit( *envP))
				return (errEnvMon);			// bad environment monitor value

			*monitor = strtoul( envP, &envP, 10);		// read monitor type
			break;
		}
	}
	return (errOk);
}

/*****************************************************************************
* Look for an ECP port at the given address.
*
* If ECP found, EcpDMA and EcpIRQ to the values read from the ECP registers.
*
* Port is set to the Compatibility mode upon exit.
*
* Returns:
*    errCode
*****************************************************************************/
uint zvgDetectECP( uint portAdr)
{
	uchar		pv, cnfgA, cnfgB;
	uint		err;

	err = errOk;

	// setup port addresses

	ZvgIO.ecpPort = portAdr;
	ZvgIO.ecpData = portAdr + ECP_data;
	ZvgIO.ecpDcr = portAdr + ECP_dcr;
	ZvgIO.ecpDsr = portAdr + ECP_dsr;
	ZvgIO.ecpEcr = portAdr + ECP_ecr;
	ZvgIO.ecpEcpDFifo = portAdr + ECP_ecpDFifo;

	// Reset all ECP flags

	ZvgIO.ecpFlags = 0;

	// Claim port access
	err = iopl(3);
	if (err)
	{
		return(1);
	}
	// look for ECP port

	pv = inportb( ZvgIO.ecpEcr);

	// Check that the full bit is off, and the empty bit is set

	if (!((pv & (ECR_full | ECR_empty)) == ECR_empty))
		return (errNotEcp);

	// verify that we cannot change the empty bit to a zero 

	outportb( ZvgIO.ecpEcr, 0x34);				// write 0x34

	if (inportb( ZvgIO.ecpEcr) != 0x35)			// verify 0x35 is read back
		return (errNotEcp);

	// gain access to the configuration registers

	outportb( ZvgIO.ecpEcr, ECR_Cnfg_mode | ECR_nErrIntrEn | ECR_serviceIntr);

	// read the configuration registers

	cnfgA = inportb( portAdr + ECP_cnfgA);
	cnfgB = inportb( portAdr + ECP_cnfgB);

	// Check the size of the data word being transferred

	if ((cnfgA & 0x60) != 0x10)
	{
		// if not set to 8 bit mode, try to set it

		cnfgA &= 0x9F;												// remove word size bits
		cnfgA |= 0x10;												// set to 8 bit words

		outportb( portAdr + ECP_cnfgA, cnfgA);

		// if not writeable, error

		if (inportb( portAdr + ECP_cnfgA) != cnfgA)
			err = errEcpWord;
	}

	// set port to SPP mode

	outportb( ZvgIO.ecpEcr, ECR_SPP_mode | ECR_nErrIntrEn | ECR_serviceIntr);

	// Set the flags to compatibility mode:
	//
	//    nSelectIn - Low
	//    nAutoFeed - High
	//    nStrobe   - High
	//    nInit     - High
	//
	// Also set are:
	//
	//    Direction - Low (buffers are enabled for writing to peripheral)
	//    ackIntEn  - Low (nAck interrupts are disabled)

	wdcr( DCR_nAutoFeed | DCR_nStrobe | DCR_nInit);

	return (err);
}

/*****************************************************************************
* Return information about the port
*
* Called with:
*    aPORT = Pointer to uint to receive port address.
*    aMON  = Pointer to uint to receive monitor type.
*****************************************************************************/
void zvgGetPortInfo( uint *aPORT, uint *aMON)
{
	*aPORT = ZvgIO.ecpPort;			// return PORT address
	*aMON = ZvgIO.envMonitor;		// return the monitor type
}

/*****************************************************************************
* Initialize the ZVG.
*
* Returns:
*    errCode
*****************************************************************************/
uint zvgInit( void)
{
	uint				err, ii;
	uint				envPort, envMode;

	envPort = (uint)-1;				// mark as non-existant
	envMode = (uint)-1;				// mark as non-existant

	ZvgIO.envMonitor = (uint)-1;			// mark as non-existant

	tmrInit();					// initialize timers
	tmrSetFrameRate(60);				// set the frame rate

	// read the 'ZVGPORT=' environment variable

	err = zvgEnv( &envPort, &ZvgIO.envMonitor);

	if (err)
		return (err);

	if (envPort == (uint)-1)
		return (errNoPort);						// no port address given, can't continue

	// check for a monitor type, if not, set a default value

	if (ZvgIO.envMonitor == (uint)-1)
		ZvgIO.envMonitor = MONF_SPOTKILL;	// handle spotkiller by default

	err = zvgDetectECP( envPort);				// validate ECP port, get chipset DMA and IRQ

	if (err)
		return (err);

	// Allocate two buffers

	ZvgIO.dmaBf1P = (uchar*)(malloc( MEM_BFR_SZ ));
	ZvgIO.dmaBf2P = (uchar*)(malloc( MEM_BFR_SZ ));

	if (ZvgIO.dmaBf1P == 0 || ZvgIO.dmaBf2P == 0)
		return (errMemory);

	// reset buffer index / count

	ZvgIO.dmaCurP = ZvgIO.dmaBf1P;
	ZvgIO.dmaCurCount = 0;

	// attempt to transmit a block of NOPs to the ZVG.
	// The number of NOPs sent should overflow the ECP buffer, to verify that the ZVG
	// is indeed receiving the NOPs.

	for (ii = 0; ii < 1024; ii++)
		zvgDmaPutc( zcNOP);

	// send the block of NOPs to the ZVG

	err = zvgDmaSendSwap();

	return (err);
}

/*****************************************************************************
* Close down the ZVG subsystem.
*
* Does a controlled shutdown of the ZVG, DMA, etc.  Releases all allocated
* memory.
*
* Called with:
*    NONE
*
* Returns:
*    NONE
*****************************************************************************/
void zvgClose( void)
{
	// release memory

	if (ZvgIO.dmaBf1P != 0)
	{	free( ZvgIO.dmaBf1P);
		ZvgIO.dmaBf1P = 0;
	}

	if (ZvgIO.dmaBf2P != 0)
	{	free( ZvgIO.dmaBf2P);
		ZvgIO.dmaBf2P = 0;
	}

	// if we were in the ECP mode, send a center command

	if (ZvgIO.ecpFlags & ECPF_ECP)
	{
		zvgEcpPutc( zcNOP);											// flush any possible half sent commands
		zvgEcpPutc( zcNOP);
		zvgEcpPutc( zcNOP);
		zvgEcpPutc( zcNOP);
		zvgEcpPutc( zcNOP);
		zvgEcpPutc( zcNOP);
		zvgEcpPutc( zcNOP);
		zvgEcpPutc( zcNOP);

		// Center the beam on the CRT

		zvgEcpPutc( zcCENTER);										// send CENTER command

		zvgEcpPutc( zcNOP);											// fill buffer to flush out CENTER command
		zvgEcpPutc( zcNOP);
		zvgEcpPutc( zcNOP);
		zvgEcpPutc( zcNOP);
		zvgEcpPutc( zcNOP);
		zvgEcpPutc( zcNOP);
		zvgEcpPutc( zcNOP);
		zvgEcpPutc( zcNOP);

		// Send one more NOP to allow us to wait for the CENTER command to finish

		zvgEcpPutc( zcNOP);

		zvgSetSppMode();											// go back to the compatibility mode
	}
}

/*****************************************************************************
* Write a character to the SPP port.
*
* Toggles the lines manually, so this should work with any port.
* This routine is BUSY flag driven only and uses no IRQs.
*
* This routine must not be used to send ZVG vector data. The firmware will
* not except vector data using the SPP mode.  To send vector data call
* 'zvgSetEcpMode()' and use the 'zvgEcpPutc()' routine.
*
* The SPP mode is used for bi-directional commands and status data only.
*
* Called with:
*    cc = Character to send to the ZVG.
*
* Returns:
*    'portErrCode'
*****************************************************************************/
uint zvgSppPutc( uchar cc)
{
	// check for proper mode

	if (ZvgIO.ecpFlags & (ECPF_ECP | ECPF_NIBBLE))
		return (errEcpBadMode);	// this routine only works for SPP modes

	// wait for busy to go low

	if (waitForDsrEQ( DSR_Busy, 0, PERIPH_WAIT) == ZVG_TIMEOUT)
		return (errEcpBusy);		// Peripheral is apparently too busy for us

	// place data on bus, assume at least one cycle per access, wait for at least 1us

	outportb( ZvgIO.ecpData, cc);
	outportb( ZvgIO.ecpData, cc);
	outportb( ZvgIO.ecpData, cc);
	outportb( ZvgIO.ecpData, cc);

	cdcr( DCR_nStrobe);			// set strobe low

	// this wait is slightly outside the SPP compatibility standard.  But this assures
	// us that the ZVG (which is firmware based), has read the data.

	if (waitForDsrEQ( DSR_Busy, DSR_Busy, PERIPH_TIMEOUT) == ZVG_TIMEOUT)
	{	sdcr( DCR_nStrobe);		// set strobe back high
		return (errEcpTimeout);	// indicate error
	}
	sdcr( DCR_nStrobe);			// release strobe
	return (errOk);
}

/*****************************************************************************
* Put a block of memory to the ZVG using the SPP mode.
*
* This routine will read 'bfrLen' number of bytes, or until no more data
* is available from the ZVG.  Number of bytes actually read is returned
* in the integer pointed to by 'aReadLen'.
*
* Called with:
*    ss  = Pointer to block of memory to send to ZVG.
*    len = Number of bytes to send.
*
* Returns:
*    'portErrCode'
*****************************************************************************/
uint zvgSppPutMem( uchar *ss, uint len)
{
	uint	err;

	err = errOk;

	while (len-- > 0)
	{	err = zvgSppPutc( *ss++);

		if (err)
			break;
	}
	return (err);
}

/*****************************************************************************
* Read block of memory from ZVG.
*
* This routine will read 'bfrLen' number of bytes, or until no more data
* is available from the ZVG.  Number of bytes actually read is returned
* in the integer pointed to by 'aReadLen'.
*
* This routine will negotiate into the reverse nibble mode, if not already
* there.  This routine can be called from the ECP mode, and will
* terminate the ECP mode before entering the NIBBLE mode.
*
* Upon exit this routine will leave the port in the SPP mode, regardless
* of what the mode was when called. 
*
* Called with:
*    ss       = Pointer to buffer to accept string from ZVG.
*    bfrLen   = Length of buffer.
*    aReadLen = Pointer to 'uint' to be set to number of bytes read.
*
* Returns:
*    'portErrCode'
*****************************************************************************/
uint zvgGetMem( uchar *ss, uint bfrLen, uint *aReadLen)
{
	uint	err, readLen;

	// if in ECP mode, terminate first

	if (ZvgIO.ecpFlags & ECPF_ECP)
		terminate_1284();

	// if not already in nibble mode, do a REQ for NIBBLE mode

	if (!(ZvgIO.ecpFlags & ECPF_NIBBLE))
		if ((err = (negotiate_1284( EMODE_NIBBLE))) != errOk)
			return (err);

	// up to us to set the flag

	ZvgIO.ecpFlags |= ECPF_NIBBLE;		// indicate nibble mode

	readLen = 0;

	// read first byte without checking if data is available.  This allows
	// up to PERIPH_WAIT time for data to show up.

	err = getByteNb( ss);

	// if error, return a timeout (or whatever error was returned).

	if (err)
		return (err);

	ss++;
	readLen++;

	// Read string of length 'len' from peripheral using the NIBBLE mode.
	// Check each time through loop to see if more data is available.
	// If no more data available, exit loop without an error.

	while (readLen < bfrLen && isDataAvail())
	{
		err = getByteNb( ss);

		if (err)
			break;

		ss++;
		readLen++;
	}
	*aReadLen = readLen;

	if (!err)
		terminate_1284();

	return (err);
}

/*****************************************************************************
* Get Device ID from ZVG.
*
* This routine will read 'bfrLen' number of bytes, or until no more data
* is available from the ZVG.  Number of bytes actually read is returned
* in the integer pointed to by 'aReadLen'.
*
* Data is read using the 1284 REQ ID in the Nibble mode.  Any 1284 compliant
* device is required to support this option, so this may or may not be
* returned data from a ZVG, it could be any 1284 compliant device.
*
* This routine will negotiate into the reverse nibble mode, if not already
* there.  This routine can be called from the ECP mode, and will
* terminate the ECP mode before entering the NIBBLE mode.
*
* Upon exit, this routine will leave the port in the SPP mode, regardless
* of what the mode was when called. 
*
* Called with:
*    ss       = Pointer to buffer to accept string from ZVG.
*    idLen    = Length of buffer.
*    aReadLen = Pointer to 'uint' to be set to number of bytes read.
*
* Returns:
*    'portErrCode'
*****************************************************************************/
uint zvgGetDeviceID( uchar *ss, uint idLen, uint *aReadLen)
{
	uint	err, count, readLen;
	uchar	countMSB, countLSB=0;

	// if in ECP mode, terminate first

	if (ZvgIO.ecpFlags & ECPF_ECP)
		terminate_1284();

	if (idLen < 2)
		return (errEcpNoData);				// no room in buffer

	// do a REQ for ID using NIBBLE mode

	if (!(ZvgIO.ecpFlags & ECPF_NIBBLE))
		if ((err = (negotiate_1284( EMODE_REQID_NIBBLE))) != errOk)
			return (err);

	// up to us to set the flag

	ZvgIO.ecpFlags |= ECPF_NIBBLE;		// indicate nibble mode

	// get count word

	err = getByteNb( &countMSB);			// get a byte using nibble mode

	if (!err)
		err = getByteNb( &countLSB);

	// check for error reading first two bytes

	if (err)
		return (err);

	count = (countMSB << 8) + countLSB;

	if (count <= 2)
	{	terminate_1284();
		*ss = '\0';
		*aReadLen = 0;
		return (errEcpNoData);				// no proper ID is given
	}

	count -= 2;									// get number of bytes to read

	// check if it will fit in buffer, leave room for trailing '\0'

	if (count > idLen - 1)
		count = idLen - 1;

	readLen = 0;

	// read string of length 'count' from peripheral using the NIBBLE mode

	while (readLen < count && isDataAvail())
	{
		err = getByteNb( ss);

		if (err)
			break;

		ss++;
		readLen++;
	}
	*ss = '\0';
	*aReadLen = readLen;

	// switch to SPP mode (if not already there because of error)

	zvgSetSppMode();
	return (err);
}

/*****************************************************************************
* Set to ECP mode from compatibility mode.
*
* If no errors occur, sets up port to ECP forward mode.
*****************************************************************************/
uint zvgSetEcpMode( void)
{
	uint	err;

	if (ZvgIO.ecpFlags & ECPF_ECP)
		return (errOk);					// already in ECP mode

	// negotiate to ECP mode

	if ((err = (negotiate_1284( EMODE_ECP))) != errOk)
		return (err);

	// is ECP mode supported?

	if (!(rdsr() & DSR_XFlag))
	{	terminate_1284();					// exit negotiations
		return (errEcpFailed);			// if ECP mode not supported, return error
	}

	// Set the ECP mode flag

	ZvgIO.ecpFlags |= ECPF_ECP;

	// ECP setup phase

	cdcr( DCR_HostAck);					// indicate that XFlag read

	// wait for busy to go low

	if (waitForDsrEQ( DSR_nAckReverse|DSR_PeriphClk|DSR_PerphAck|DSR_XFlag,
			DSR_nAckReverse|DSR_PeriphClk|DSR_XFlag, PERIPH_WAIT) == ZVG_TIMEOUT)
	{	compatibility();					// if timeout, return to compatability mode
		return (errEcpFailed);			// could not get into ECP mode
	}

	// setup the hardware to do automatic ECP mode transfers

	outportb( ZvgIO.ecpEcr, ECR_ECP_mode | ECR_nErrIntrEn | ECR_serviceIntr);
	sdcr( DCR_HostAck | DCR_HostClk);
	return (errOk);
}

/*****************************************************************************
* Return to SPP (compatibility) mode.
*
* Checks first to see if we are *not* already in SPP mode.
*****************************************************************************/
void zvgSetSppMode( void)
{
	// if not in SPP mode, switch to it

	if (ZvgIO.ecpFlags & (ECPF_ECP | ECPF_NIBBLE))
		terminate_1284();
}

/*****************************************************************************
* Check to see if any data is available in the ECP mode.
*
* Called with:
*    time = Time to wait for data, in milliseconds.
*
* Returns:
*    errOk         - if data is available.
*    errEcpTimeout - if data was not available.
*
*    Else, returns an ECP error code.
*****************************************************************************/
uint zvgIsDataAvail( uint aTime)
{
	uint	dsr;

	// must be in ECP mode

	if (!(ZvgIO.ecpFlags & ECPF_ECP))
		return (errEcpBadMode);

	// wait for nPeriphRequest to go low

	dsr = waitForDsrNE( DSR_PeriphClk|DSR_nPeriphRequest|DSR_XFlag,
			DSR_PeriphClk|DSR_nPeriphRequest|DSR_XFlag, aTime);

	if (dsr == ZVG_TIMEOUT)
		return (errEcpTimeout);		// nothing wrong, just no data available

	// check for a breach in the ECP protocol

	else if ((dsr & (DSR_XFlag|DSR_PeriphClk)) != (DSR_XFlag | DSR_PeriphClk))
	{	compatibility();				// if status incorrect, return to compatability mode
		return (errEcpToSpp);		// indicate no longer in ECP mode
	}
	return (errOk);
}

/*****************************************************************************
* Write a byte to the port using ECP mode with hardware assist.
*
* ECP mode must have already been negotiated.
*
* If too much time passes, this routine will timeout, but it will not
* terminate the ECP mode.  So it may be called repeatably.
*
* Returns:
*    errEcpTimeout - If no response.
*    errEcpToSpp   - If DSR_XFlag line was dropped, also resets ECP to SPP mode.
*****************************************************************************/
uint zvgEcpPutc( uchar cc)
{
	uint	ii;

	// for speed, check first if room in ECP buffer

	if (!(inportb( ZvgIO.ecpEcr) & ECR_full))
		outportb( ZvgIO.ecpEcpDFifo, cc);		// send data, let hardware handshake

	// If not, do the longer TIMED version of the code

	else
	{
		// wait for 1 second

		for (ii = 0; ii < 10; ii++)
		{
			// check every 100ms for a breach in protocol

			if (waitForEcrEQ( ECR_full, 0, 100) == ZVG_TIMEOUT)
			{
				// if no response after 100ms, do a quick check of the status lines to
				// see if XFlag or PeriphClk has dropped.

				if ((rdsr() & (DSR_XFlag | DSR_PeriphClk)) != (DSR_XFlag | DSR_PeriphClk))
				{
					// The 1284 peripheral is not allowed to drop out of ECP
					// mode without being requested to do so, and the
					// the PeriphClk must remain high while in ECP forward transfer
					// mode, so something unusual has happened, like a cable disconnect.

					compatibility();				// go immediatly into SPP mode
					return (errEcpToSpp);
				}
			}
			else
				break;
		}

		if (ii == 10)
			return (errEcpTimeout);				// it's taken too long, something wrong

		// if no timeout, send data, hardware takes care of handshaking

		outportb( ZvgIO.ecpEcpDFifo, cc);
	}
	return (errOk);
}

/*****************************************************************************
* Write a block of memory to the port using ECP mode with hardware assist.
*
* ECP mode must have already been negotiated.
*
* Called with:
*    mem     = Pointer that points to memory block.
*    memSize = Size of block of data to be sent.
*****************************************************************************/
uint zvgEcpPutMem( uchar *mem, uint memSize)
{
	uint	err;

	err = errOk;

	while (memSize-- > 0)
	{
		err = zvgEcpPutc( *mem);

		if (err)
			break;

		else
			mem++;
	}
	return (err);
}

/*****************************************************************************
* Start a DMA transfer to the ZVG.
*
* Called with:
*    poffset = Protected mode offset.
*    page    = 64k Page of buffer to transfer.
*    offset	 = 16 bit offset of buffer to transfer.
*    count   = Count of bytes to transfer.
*
* Returns:
*    errOk         - if DMA started.
*    errEcpTimeout - if timeout while waiting for ZVG.
*
*    Else, returns a ZVG error code.
*****************************************************************************/
static uint zvgDmaStart( uchar *mem, uint count)
{
	uint	err;

	// make sure we're in the ECP mode

	if (!(ZvgIO.ecpFlags & ECPF_ECP))
	{	err = zvgSetEcpMode();										// set to ECP mode

		if (err)
			return (err);												// if error, return
	}

	err = errOk;

	// loop through buffer sending bytes to the ZVG

	while (!err && count > 0)
	{	err = zvgEcpPutc( *mem);
		mem++;
		count--;
	}
	if (err == errEcpTimeout)
		compatibility();												// if timeout, force compatibility mode

	return (err);
}

/*****************************************************************************
* Write a byte to the port using a buffered DMA mode.
*
* Byte is added to a DMA buffer to be sent later.
*
* Returns:
*    errOk       - No error.
*    errBfrFull  - If buffer has overflowed.
*****************************************************************************/
uint zvgDmaPutc( uchar cc)
{
	if (ZvgIO.dmaCurCount < MEM_BFR_SZ )
		ZvgIO.dmaCurP[ZvgIO.dmaCurCount++] = cc;

	else
		return (errBfrFull);

	return (errOk);
}

/*****************************************************************************
* Write a block of memory to the port using a buffered DMA mode.
*
* Memory block is added to a DMA buffer to be sent later.
*
* Returns:
*    errOk       - No error.
*    errBfrFull  - If buffer has overflowed.
*****************************************************************************/
uint zvgDmaPutMem( uchar *mem, uint len)
{
	if (ZvgIO.dmaCurCount < MEM_BFR_SZ)
	{
		if ((ZvgIO.dmaCurCount + len) > MEM_BFR_SZ)
			len = MEM_BFR_SZ - ZvgIO.dmaCurCount;

		memcpy( &ZvgIO.dmaCurP[ZvgIO.dmaCurCount], mem, len);
		ZvgIO.dmaCurCount += len;
	}
	else
		return (errBfrFull);

	return (errOk);
}

/*****************************************************************************
* Clear the DMA buffer by resetting the DMA count, which is also used as
* the buffer index pointer.
*****************************************************************************/
void zvgDmaClearBfr( void)
{
	ZvgIO.dmaCurCount = 0;				// point to start of protected mode buffer
}

/*****************************************************************************
* Send the current DMA buffer to the ZVG using a DMA.
*
* This routine does not swap or clear the DMA buffers and can be called
* repeatedly to send the same block of information to the ZVG.
*****************************************************************************/
uint zvgDmaSend( void)
{
	return (zvgDmaStart( ZvgIO.dmaCurP, ZvgIO.dmaCurCount));
}

/*****************************************************************************
* Send the current DMA buffer to the ZVG using a DMA. Swap DMA buffers.
*
* This routine starts a DMA request on the current buffer.  It then swaps in
* the 2nd DMA buffer allowing 'zvgDmaPutc()' and 'zvgDmaPutMem()' calls to
* be made while the current DMA buffer is being sent.
*
* This allows the next frame to be built while the current one is being sent.
*****************************************************************************/
uint zvgDmaSendSwap( void)
{
	uint	err;

	err = zvgDmaStart( ZvgIO.dmaCurP, ZvgIO.dmaCurCount);

	if (!err)
	{
		if (ZvgIO.dmaCurP == ZvgIO.dmaBf1P)
		{	ZvgIO.dmaBf1Count = ZvgIO.dmaCurCount;			// keep track of count for possible resend
			ZvgIO.dmaCurP = ZvgIO.dmaBf2P;					// point to 2nd buffer
		}
		else
		{	ZvgIO.dmaBf2Count = ZvgIO.dmaCurCount;			// keep track of count for possible resend
			ZvgIO.dmaCurP = ZvgIO.dmaBf1P;					// point to 1st buffer
		}
		ZvgIO.dmaCurCount = 0;								// clear buffer
	}

	ZvgIO.dmaCurCount = 0;
	return (err);
}

/*****************************************************************************
* Send the previous DMA buffer to the ZVG using a DMA.
*
* This routine is only valid after a call to 'zvgDmaSendSwap()' is made. This
* routine allows the previous DMA frame buffer to be resent to the ZVG.
*
* This routine can be used to continuously send the last frame while a new
* frame is worked on, or to remove flicker by double sending the each DMA
* frame.
*****************************************************************************/
uint zvgDmaSendPrev(void)
{
	if (ZvgIO.dmaCurP == ZvgIO.dmaBf1P)
		return (zvgDmaStart( ZvgIO.dmaBf2P, ZvgIO.dmaBf2Count));

	else
		return (zvgDmaStart( ZvgIO.dmaBf1P, ZvgIO.dmaBf1Count));
}

/*****************************************************************************
* Read and parse the ID string returned from the ZVG
*
* Upon exit this routine will leave the port in the ECP mode, regardless
* of what the mode was when called.
*****************************************************************************/
uint zvgReadDeviceID( ZvgID_s *devID)
{
	uint	idLen, err, ii, jj;

	err = zvgGetDeviceID( ZvgIO.mBfr, ZVG_MAX_BFRSZ, &idLen);

	if (err)
		return (err);

	// return to ECP mode

	err = zvgSetEcpMode();

	if (err)
		return (err);

	// parse buffer

	ii = 0;

	// Get "MFG:"

	if (strncmp( (char*)ZvgIO.mBfr + ii, "MFG:", 4) != 0)
		return (errUnknownID);

	ii += 4;

	for (jj = 0; jj < ZVG_MAX_IDSZ-1 && ZvgIO.mBfr[ii] != ';' && ii < idLen; )
		devID->mfg[jj++] = ZvgIO.mBfr[ii++];

	devID->mfg[jj] = '\0';

	ii += 3;						// skip ';',CR,LF

	// Get "CMD:"

	if (strncmp( (char*)ZvgIO.mBfr + ii, "CMD:", 4) != 0 || ii >= idLen)
		return (errUnknownID);

	ii += 4;

	for (jj = 0; jj < ZVG_MAX_IDSZ-1 && ZvgIO.mBfr[ii] != ';' && ii < idLen; )
		devID->cmd[jj++] = ZvgIO.mBfr[ii++];

	devID->cmd[jj] = '\0';

	ii += 3;						// skip ';',CR,LF

	// Get "MDL:"

	if (strncmp( (char*)ZvgIO.mBfr + ii, "MDL:", 4) != 0 || ii >= idLen)
		return (errUnknownID);

	ii += 4;

	for (jj = 0; jj < ZVG_MAX_IDSZ-1 && ZvgIO.mBfr[ii] != ';' && ii < idLen; )
		devID->mdl[jj++] = ZvgIO.mBfr[ii++];

	devID->mdl[jj] = '\0';

	ii += 3;						// skip ';',CR,LF

	// Get "VER:"

	if (strncmp( (char*)ZvgIO.mBfr + ii, "VER:", 4) != 0 || ii >= idLen)
		return (errUnknownID);

	ii += 4;

	devID->fVer = strtoul( (char*)ZvgIO.mBfr + ii, 0, 16);
	ii += 4;

	if (ZvgIO.mBfr[ii] != ',' || ii >= idLen)
		return (errUnknownID);

	ii++;
	devID->bVer = strtoul( (char*)ZvgIO.mBfr + ii, 0, 16);
	ii += 4;

	if (ZvgIO.mBfr[ii] != ',' || ii >= idLen)
		return (errUnknownID);

	ii++;
	devID->vVer = strtoul( (char*)ZvgIO.mBfr + ii, 0, 16);
	ii += 4;

	if (ZvgIO.mBfr[ii] != ';' || ii >= idLen)
		return (errUnknownID);

	ii += 3;						// skip ';',CR,LF

	// Get "SWS:"

	if (strncmp( (char*)ZvgIO.mBfr + ii, "SWS:", 4) != 0 || ii >= idLen)
		return (errUnknownID);

	ii += 4;
	devID->sws = strtoul( (char*)ZvgIO.mBfr + ii, 0, 16);
	ii += 2;

	if (ZvgIO.mBfr[ii] != ';' || ii >= idLen)
		return (errUnknownID);

	ii += 3;						// skip ';',CR,LF

	// Get "ESB:" (Error Status Bits)

	if (strncmp( (char*)ZvgIO.mBfr + ii, "ESB:", 4) != 0 || ii >= idLen)
		return (errUnknownID);

	ii += 4;
	devID->fESB = strtoul( (char*)ZvgIO.mBfr + ii, 0, 16);
	ii += 4;

	if (ZvgIO.mBfr[ii] != ',' || ii >= idLen)
		return (errUnknownID);

	ii++;
	devID->vESB = strtoul( (char*)ZvgIO.mBfr + ii, 0, 16);
	ii += 4;

	if (ZvgIO.mBfr[ii] != ';' || ii >= idLen)
		return (errUnknownID);

	return (err);
}

/*****************************************************************************
* Read current monitor information from the ZVG.
*
* This routine will send a zcREAD_MON command to the ZVG, and must wait
* for the command to be executed, since there can be vector commands
* ahead of this command in the ZVG command buffer. The data is sent in the
* ECP mode, and the port will be set to the ECP mode if not already there.
*
* When the READ_MON command is executed, the ZVG will indicates data is ready
* by setting the 'nPeriphRequest' line low, indicating data is available.
*
* Since the ZVG cannot return data in the ECP mode, the ECP mode must
* be terminated, and the reverse nibble mode used to read the data.
*
* Upon exit this routine will leave the port in the ECP mode, regardless
* of what the mode was when called.
*
* Called with:
*    mon = Pointer to a 'ZvgMon_s' structure used to hold ZVG data.
******************************************************************************/
uint zvgReadMonitorInfo( ZvgMon_s *mon)
{
	uint	readLen, ecpErr, err=0;

	if (!(ZvgIO.ecpFlags & ECPF_ECP))
		err = zvgSetEcpMode();				// if not ECP mode, set to ECP mode

	if (err)
		return (err);							// return on any errors

	// request monitor information
	//
	// The ZVG has a 9 byte look ahead buffer.  Until at least 9 bytes are
	// in it's command buffer, nothing will get executed.  This requires
	// the zcREAD_MON command to be followed by 8 NOPs in order to guarantee
	// execution.

	zvgEcpPutc( zcREAD_MON);				// setup to read monitor information
	zvgEcpPutc( zcNOP);						// must fill enough of buffer to allow
	zvgEcpPutc( zcNOP);						// READ_MON to execute
	zvgEcpPutc( zcNOP);
	zvgEcpPutc( zcNOP);
	zvgEcpPutc( zcNOP);
	zvgEcpPutc( zcNOP);
	zvgEcpPutc( zcNOP);
	zvgEcpPutc( zcNOP);

	// wait for command to execute, this could be behind a bunch of vector commands

	err = zvgIsDataAvail( 1000);			// allow a second for data to show up

	if (err)
		return (err);

	// read monitor information into a simple buffer

	err = zvgGetMem( ZvgIO.mBfr, ZVG_MAX_BFRSZ, &readLen);

	if (!err && (readLen != ZVG_MON_SIZE))
		err = errEcpBadData;

	if (!err)
	{
		// GCC does not pack its structure by default, so we need to move each value
		// one byte at a time

		mon->point_i = ZvgIO.mBfr[0];
		mon->zShift = ZvgIO.mBfr[1];
		mon->oShoot = ZvgIO.mBfr[2];
		mon->jumpFactor = ZvgIO.mBfr[3];
		mon->settle = ZvgIO.mBfr[4];
		mon->min_i = ZvgIO.mBfr[5];
		mon->max_i = ZvgIO.mBfr[6];
		mon->scale = ZvgIO.mBfr[7];
		mon->flags = ZvgIO.mBfr[8];

		// for word data, LSB byte is first.

		mon->cksum = ZvgIO.mBfr[9] + ((ushort)ZvgIO.mBfr[10] << 8);
	}

	// return to ECP mode

	ecpErr = zvgSetEcpMode();

	if (ecpErr)
		err = ecpErr;							// an ECP error has higher priority than a bad data error

	return (err);
}

/*****************************************************************************
* Read speed table information from the ZVG.
*
* This routine will send a zcREAD_SPD command to the ZVG, and must wait
* for the command to be executed, since there can be vector commands
* ahead of this command in the ZVG command buffer. The data is sent in the
* ECP mode, and the port will be set to the ECP mode if not already there.
*
* When the READ_SPD command is executed, the ZVG indicates data is ready
* by setting the 'nPeriphRequest' line low, indicating data is available.
*
* Since the ZVG cannot return data in the ECP mode, the ECP mode must
* be terminated, and the reverse nibble mode used to read the data.
*
* Upon exit this routine will leave the port in the ECP mode, regardless
* of what the mode was when called.
*
* Called with:
*    speeds = A 4 byte buffer used to read the four different available
*             ZVG speeds.
*****************************************************************************/
uint zvgReadSpeedInfo( ZvgSpeeds_a speeds)
{
	uint	readLen, ecpErr, err=0;

	if (!(ZvgIO.ecpFlags & ECPF_ECP))
		err = zvgSetEcpMode();				// if not ECP mode, set to ECP mode

	if (err)
		return (err);							// return on any errors

	// request speed table information

	// The ZVG has a 9 byte look ahead buffer.  Until at least 9 bytes are
	// in it's command buffer, nothing will get executed.  This requires
	// the zcREAD_SPD command to be followed by 8 NOPs in order to guarantee
	// execution.

	zvgEcpPutc( zcREAD_SPD);				// setup to read monitor information
	zvgEcpPutc( zcNOP);						// must fill enough of buffer to allow
	zvgEcpPutc( zcNOP);						// READ_MON to execute
	zvgEcpPutc( zcNOP);
	zvgEcpPutc( zcNOP);
	zvgEcpPutc( zcNOP);
	zvgEcpPutc( zcNOP);
	zvgEcpPutc( zcNOP);
	zvgEcpPutc( zcNOP);

	// wait for command to execute

	err = zvgIsDataAvail( 1000);			// wait for a second for data to show up

	if (err)
		return (err);

	// read 4 speed table bytes into buffer

	err = zvgGetMem( ZvgIO.mBfr, 4, &readLen);

	if (err)
		return( err);

	if (!err && (readLen != 4))
		err = errEcpBadData;

	// move the buffered speeds to the speed array

	speeds[0] = (uint)ZvgIO.mBfr[0];
	speeds[1] = (uint)ZvgIO.mBfr[1];
	speeds[2] = (uint)ZvgIO.mBfr[2];
	speeds[3] = (uint)ZvgIO.mBfr[3];

	// return to ECP mode

	ecpErr = zvgSetEcpMode();

	if (ecpErr)
		err = ecpErr;							// an ECP error has higher priority than a bad data error

	return (err);
}
