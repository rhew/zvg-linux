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
#include	<dpmi.h>

#define	PIC_PIC1IOBASE		0x20			// base address of primary PIC
#define	PIC_PIC1IRQOFF		0x07			// vector offset of PIC1 IRQ vectors

#define	PIC_PIC2IOBASE		0xA0			// base address of PIC 2
#define	PIC_PIC2IRQOFF		0x70			// vector offset of PIC2 IRQ vectors

#define	PIC_REG_OCW1		1				// offset from base of the OCW1 register
#define	PIC_CMD_EOI			0x20			// non-specific end of interrupt

typedef struct	IRQ_S
{	__dpmi_paddr	*oPModeHandler;		// original protected mode handler
	__dpmi_paddr	*nPModeHandler;		// new protected mode handler

	__dpmi_raddr	*oRModeHandler;		// original real mode handler
	__dpmi_raddr	*nRModeHandler;		// new real mode handler

	int				irqNumber;				// IRQ number
	int				irqVector;				// IRQ number + proper offset

	int				irqCount;		  		// Incremented when IRQ routine executed.
} Irq_s;

#endif
