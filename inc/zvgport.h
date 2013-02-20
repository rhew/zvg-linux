#ifndef _ZVGPORT_H_
#define _ZVGPORT_H_
/*****************************************************************************
* Header file for ZVGPORT.C.
*
* Author:       Zonn Moore
* Created:      11/06/02
* Last Updated: 05/21/03
*
* History:
*
* (c) Copyright 2002-2004, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#ifndef _ZSTDDEF_H_
#include	"zstddef.h"
#endif

#if defined(WIN32) || defined(linux)
	#include <string.h>

#else // DOS
#endif // OS

#define ZVG_IO_THREAD_ENABLED

#define	ZVG_MAX_BFRSZ			256		// maximum number of bytes the ZVG can return

#define	ZVG_MAX_IDSZ			17			// size of ID strings in 'zvgReadDeviceID()' strings

#define	PERIPH_TIMEOUT			100		// the standard says 35ms, we've loosened it too 100ms
#define	PERIPH_WAIT	  			1000 		// this is a non-error timeout used to wait for Peripheral responses

// Offset from base address of parallel port registers

#define	ECP_data				0x000
#define	ECP_ecpAFifo			0x000
#define	ECP_dsr					0x001	
#define	ECP_dcr					0x002
#define	ECP_cFifo				0x400
#define	ECP_ecpDFifo			0x400
#define	ECP_tFifo				0x400
#define	ECP_cnfgA				0x400
#define	ECP_cnfgB				0x401
#define	ECP_ecr					0x402

// ECR bitmaps

#define	ECR_SPP_mode			0x00		// SPP mode
#define	ECR_EPP_mode			0x20		// Bi-Di mode
#define	ECR_FSPP_mode			0x40		// Fast SPP mode
#define	ECR_ECP_mode			0x60		// ECP mode
#define	ECR_Cnfg_mode			0xE0		// Confige mode, makes 'cnfgA' and 'cnfgB' available

#define	ECR_nErrIntrEn			0x10
#define	ECR_dmaEn				0x08
#define	ECR_serviceIntr			0x04
#define	ECR_full				0x02
#define	ECR_empty				0x01

#define	CFGB_compress			0x80		// if set, use ECP compression

// DSR Bitmaps

#define	DSR_InvMask				(DSR_Busy)

//    SPP Mode

#define	DSR_Busy				0x80
#define	DSR_nAck				0x40
#define	DSR_PError				0x20
#define	DSR_Select				0x10
#define	DSR_nFault				0x08

//    NIBBLE Mode

#define	DSR_PtrBusy				0x80
#define	DSR_PtrClk				0x40
#define	DSR_AckDataReq			0x20
#define	DSR_XFlag				0x10
#define	DSR_nDataAvail			0x08

//    ECP Mode

#define	DSR_PerphAck			0x80
#define	DSR_PeriphClk			0x40
#define	DSR_nAckReverse			0x20
#define	DSR_nPeriphRequest		0x08

// DCR bitmaps

#define	DCR_InvMask				(DCR_nSelectIn | DCR_nAutoFeed | DCR_nStrobe)

#define	DCR_Direction			0x20
#define	DCR_ackIntEn			0x10

//    SPP Mode

#define	DCR_nSelectIn			0x08
#define	DCR_nInit				0x04
#define	DCR_nAutoFeed			0x02
#define	DCR_nStrobe				0x01

//    NIBBLE Mode

#define	DCR_1284_Active			0x08
#define	DCR_HostBusy			0x02
#define	DCR_HostClk				0x01

//    ECP Mode

#define	DCR_nReverseRequest		0x04
#define	DCR_HostAck				0x02

// Negotiation modes

#define	EMODE_NIBBLE			0x00
#define	EMODE_REQID_NIBBLE		0x04
#define	EMODE_ECP				0x10
#define	EMODE_REQID_ECP			0x14

// Value returned in case of a timeout

#define	ZVG_TIMEOUT				((uint)-1)

// Define some error codes

enum portErrCode
{	errOk = 0,					// no error (must be set to 0)

	errNoEnv,					// no 'ZVGPORT=' environment variable found
	errEnvPort,					// port value given in environment is invalid
	errEnvDMA,					// DMA value given in environment is invalid
	errEnvMode,					// DMA Mode value given in environment is invalid
	errEnvIRQ,					// IRQ value given in environment is invalid
	errEnvMon,					// MONITOR value given in environment is invalid
	errNoPort,					// no PORT address given
	errNotEcp,					// ECP port not found at given address
	errNoDma,					// No DMA given and unable to obtain from chipset
	errInvDma,					// Invalid DMA channel
	errInvDmaMode,				// Invalid DMA mode
	errNoIrq,					// No DMA given and unable to obtain from chipset
	errInvIrq,					// Invalid IRQ level
	errXmtErr,					// Could not send data to the ZVG during initialize test
	errMemory,					// Out of memory!
	errBfrFull,					// DMA buffer is full
	errEcpWord,					// Not an 8 bit ECP
	errEcpFailed,				// Could not negotiate for ECP mode
	errEcpNoData,				// no data available from the peripheral
	errEcpBadData,				// bad data (or data count) read from ZVG
	errEcpTimeout,				// something took too long (cable disconnected? ZVG powered off?)
	errEcpBusy,					// peripheral is too busy for us
	errEcpComm,					// communication error
	errEcpBadMode,				// trying to do something in the wrong mode
	errEcpToSpp,				// Break in ECP Protocol (cable disconnected? ZVG powered off?)
	errZvgRomCS,				// checksum error in ROM packet
	errZvgRomNE,				// flash data did not match ROM packet
	errZvgRomTI,				// flash timeout during write
	errUnknownID,				// unknow ID string returned from request ID

#if defined(WIN32)
#elif defined(linux)

	errIOPermFail,
	errNoBuffer,				// No buffer available.
	errThreadFail,				// Thread creation failed.

#else // DOS
#endif // OS

};

// This structure reflects the structure inside the ZVG firmware. Note that DJGPP does not
// pack structures by default, but the data inside the ZVG is packed.

#define	ZVG_MON_SIZE		11	// size of packed 'ZvgMon_s' structure used by ZVG firmware

typedef struct ZVGMON_S
{	uchar		point_i;		// point intensity
	uchar		zShift;			// shift value
	uchar		oShoot;			// overshoot value
	uchar		jumpFactor;		// multiplier used to calculate between vector jump timing
	uchar		settle;			// minimum value of a between vector jump, used for settling time
	uchar		min_i;			// minimum color value allowed
	uchar		max_i;			// maximum color value allowed
	uchar		scale;			// Size of screen
	uchar		flags;			// Status flags
	ushort		cksum;			// Fletcher's check digits of monitor data
} ZvgMon_s;

// Structure to return the value of the four different monitor speed tables available
// in the ZVG.

typedef uint ZvgSpeeds_a[4];

#if 0
//typedef struct ZVGSPEEDS_S
//{	uint	speed1;				// J3: SPD1=OFF, SPD2=OFF (fastest speed available)
//	uint	speed2;				// J3: SPD1=ON,  SPD2=OFF
//	uint	speed3;				// J3: SPD1=OFF, SPD2=ON
//	uint	speed4;				// J3: SPD1=ON,  SPD2=ON  (slowest speed available)
//} ZvgSpeeds_s;
#endif

// Structure to return the ZVG ID information

typedef struct ZVGID_S
{	char	mfg[ZVG_MAX_IDSZ];	// manufactorer
	char	cmd[ZVG_MAX_IDSZ];	// command set
	char	mdl[ZVG_MAX_IDSZ];	// model string
	uint	model;					// model number
	uint	fVer;						// firmware version
	uint	bVer;						// boot loader version
	uint	vVer;						// vtg version
	uint	sws;						// switch settings
	uint	fESB;						// firmware error status bits
	uint	vESB;						// vtg error status bits
} ZvgID_s;

typedef struct ZVGIO_S
{
	// These are initialized by caller's arguments

	uint		ecpPort;				// address of ZVG ECP port
	uint		ecpDMA;					// DMA channel of ZVG's ECP port
	uint		ecpIRQ;					// IRQ number of ZVG's ECP port

	uint		envMonitor;				// type of monitor indicated in ENVIRONMENT variable

	// These are used internally by ZVG routines

	// ECP variables

	uint		ecpData;				// Data address of port
	uint		ecpDsr;					// DSR address of port
	uint		ecpDcr;					// DCR address of port
	uint		ecpEcr;					// ECR address of port
	uint		ecpEcpDFifo;			// ecpDFifo address of port

	uchar		ecpDcrState;			// Current state of the DCR register

	uint		ecpFlags;				// Flags to keep track of various ECP states

	// DMA variables

	uint		dmaMemSeg;				// Real mode segment of DOS DMA memory block
	uint		dmaMemSel;				// Protected mode selector of DOS DMA memory block

#if defined(WIN32)
	HLOCAL		physicalMemAddr;
#elif defined(linux)
	void		*physicalMemAddr;
	unsigned long memSize;
#else // DOS
#endif

	uint		dmaBf1Page;				// 64k page of 1st DOS DMA memory block
	uint		dmaBf1Offs;				// 64k offset of 1st DOS DMA memory block
	uint		dmaBf1PMode;			// Protected mode offset from selector of 1st DOS DMA memory block
	uint		dmaBf1Count;			// Count of data in 2nd DMA buffer

	uint		dmaBf2Page;				// 64k page of 2nd DOS DMA memory block
	uint		dmaBf2Offs;				// 64k offset of 2nd DOS DMA memory block
	uint		dmaBf2PMode;			// Protected mode offset from selector of 2nd DOS DMA memory block
	uint		dmaBf2Count;			// Count of data in 2nd DMA buffer

	uint		dmaCurPage;				// 64k page of current DOS DMA memory block
	uint		dmaCurOffs;				// 64k offset of current DOS DMA memory block
	uint		dmaCurPMode;			// Protected mode offset from selector of current DOS DMA memory block
	uint		dmaCurCount;			// Count of characters in current DMA buffer

	uint		dmaPage;				// Page port of DMA controller
	uint		dmaAddr;				// Address port of DMA controller
	uint		dmaLen;					// Length port of DMA controller
	uint		dmaMode;				// DMA mode used for transfers

	// Miscellaneous buffer used to communicate with the ZVG

#if defined(WIN32) || defined(linux)
	char		mBfr[ZVG_MAX_BFRSZ];
#else // DOS
	uchar		mBfr[ZVG_MAX_BFRSZ];
#endif /* OS */

} ZvgIO_s;


// Define flags for 'ZvgIO_s.ecpFlags'

#define	ECPF_ECP			0x01		// if set, indicates we're in ECP mode
#define	ECPF_NIBBLE			0x02		// if set, indicates we're in reverse NIBBLE mode
#define	ECPF_DMA			0x04		// if set, indicates DMA available and enabled

// Define flags for 'ZVgIO.envMonitor'

#define	MONF_FLIPX		0x01			// set if monitor needs X axis flipped
#define	MONF_FLIPY		0x02			// set if monitor needs Y axis flipped
#define	MONF_SPOTKILL	0x04			// set if monitor needs spotkill killer activated
#define	MONF_BW			0x08			// set if monitor is a B&W monitor
#define	MONF_NOOVS		0x10			// set if monitor cannot be overscanned

// Defines for setting the DMA mode

typedef enum DMA_MODE_E
{	DmaModeDisabled = 0,				// used with 'zvgDmaMode()' call to DISABLE DMA
	DmaModeDemand,						// used with 'zvgDmaMode()' call to set DEMAND mode DMA
	DmaModeSingle						// used with 'zvgDmaMode()' call to set SINGLE mode DMA
} DmaMode_e;

extern ZvgIO_s	ZvgIO;					// Structure used to communicate with ZVG

extern void zvgBanner( ZvgSpeeds_a speeds, ZvgID_s *id);
extern void	zvgError( uint err);
extern uint zvgInit( void);
extern void zvgClose( void);
extern uint zvgDetectECP( uint portAdr);
extern uint zvgSppPutc( uchar cc);
extern uint zvgSppPutMem( uchar *ss, uint len);
extern uint zvgGetMem( uchar *ss, uint bfrLen, uint *aReadLen);
extern uint zvgGetDeviceID( uchar *ss, uint idLen, uint *aReadLen);
extern uint zvgSetEcpMode( void);
extern uint zvgEcpPutc( uchar cc);
extern uint zvgEcpPutMem( uchar *mem, uint memSize);
extern void zvgSetSppMode( void);
extern uint	zvgIsDataAvail( uint aTime);
extern uint zvgGetDeviceID( uchar *ss, uint idLen, uint *aReadLen);
extern uint zvgReadDeviceID( ZvgID_s *devID);
extern uint zvgReadMonitorInfo( ZvgMon_s *mon);
extern uint zvgReadSpeedInfo( ZvgSpeeds_a speeds);

extern void	zvgSetDma( uint aDMA);
extern void zvgSetDmaMode( DmaMode_e mode);
extern void	zvgSetIrq( uint aIRQ);
extern void zvgGetPortInfo( uint *aPORT, uint *aDMA, uint *aMODE, uint *aIRQ, uint *aMON);
extern uint zvgDmaStatus( void);
extern uint zvgDmaWait( void);
extern uint zvgDmaSend( void);
extern uint zvgDmaSendSwap( void);
extern uint zvgDmaSendPrev( void);
extern void	zvgDmaAbort( void);
extern uint zvgDmaPutc( uchar cc);
extern uint zvgDmaPutMem( uchar *mem, uint len);
extern void zvgDmaClearBfr( void);

#if defined(WIN32) || defined(linux)
void movedata(unsigned source_selector, unsigned source_offset,
              unsigned dest_selector, unsigned dest_offset,
              size_t length);
#else /* DOS */
#endif /* OS */

#ifdef ZVG_IO_THREAD_ENABLED
// Funtions supporting the separate IO thread.  These functions do not deal
// with thread calls (create or mutex lock/unlock).
	#if defined(WIN32)
	#elif defined(linux)

		uint zvgDmaSwap( void);
		uint zvgCurrentBufferID( void);
		uint zvgDmaSendBuff( uint bufID);

	#else /* DOS - not supported */
	#endif /* OS */
#endif /* ZVG_IO_THREAD_ENABLED */

#endif /* _ZVGPORT_H_ */
