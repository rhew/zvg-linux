#ifndef _PIC_H_
#define _PIC_H_
/*****************************************************************************
* Header file for adding / removing IRQ routines using the Programmable
* Interrupt Controller (PIC).
*
* Author:       Zonn Moore
* Last Updated: 05/21/03
*
* History:
*
* (c) Copyright 2002-2003, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) || defined(linux)
	typedef struct {
	  unsigned short offset16;
	  unsigned short segment;
	} __dpmi_raddr;

	typedef struct {
	  unsigned long  offset32;
	  unsigned short selector;
	} __dpmi_paddr;

	typedef struct {
	  DWORD  handle;			/* 0, 2 */
	  DWORD  size; 	/* or count */	/* 4, 6 */
#if defined(WIN32)
	  HLOCAL address;		/* 8, 10 */
#else /* defined(linux) */
	  void	*address;		/* 8, 10 */
#endif
	} __dpmi_meminfo;

	//!!! Need to provide a REAL solution for these!!!
	#define _farpeekb(a,b) ((unsigned char)*((char *)(a) + (b)))
	#define _farpokeb(a,b,c) ( *((char*)(a) + (b)) = (c) )
	#define _my_ds() (0UL)

#else /* DOS */
	#include	<dpmi.h>

#endif /* OS */


#define	PIC_PIC1IOBASE		0x20			// base address of primary PIC
#define	PIC_PIC1IRQOFF		0x07			// vector offset of PIC1 IRQ vectors

#define	PIC_PIC2IOBASE		0xA0			// base address of PIC 2
#define	PIC_PIC2IRQOFF		0x70			// vector offset of PIC2 IRQ vectors

#define	PIC_REG_OCW1		1				// offset from base of the OCW1 register
#define	PIC_CMD_EOI			0x20			// non-specific end of interrupt

typedef struct	IRQ_S
{
	__dpmi_paddr	*oPModeHandler;			// original protected mode handler
	__dpmi_paddr	*nPModeHandler;			// new protected mode handler

	__dpmi_raddr	*oRModeHandler;			// original real mode handler
	__dpmi_raddr	*nRModeHandler;			// new real mode handler

	int				irqNumber;				// IRQ number
	int				irqVector;				// IRQ number + proper offset

	int				irqCount;		  		// Incremented when IRQ routine executed.
} Irq_s;

#ifdef __cplusplus
}
#endif

#endif /* _PIC_H_ */
