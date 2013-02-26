/*****************************************************************************
* Author:  Brent Jarvis
* Created: Mar 2007
*
*****************************************************************************/

#if defined(WIN32) || defined(linux)
	#if defined(WIN32)
		#include <windows.h>
		#include <conio.h>
	#else
		#include <sys/io.h>
	#endif

	#include "wrappers.h"

#else // DOS
#endif


/*****************************************************************************
* 
*
*****************************************************************************/
#if defined(WIN32)
//----------------


#elif defined(linux)
//------------------

//int ioPermission (unsigned long int __from, unsigned long int __num,
//                   int __turn_on)
//{
//	return ioperm ( __from, __num, __turn_on);
//}

#else // DOS
#endif

/*****************************************************************************
* 
*
*****************************************************************************/
#if defined(WIN32)
//----------------

BYTE inportb( DWORD port)
{
	return (BYTE)_inp( (unsigned short)port );
}

#elif defined(linux)
//------------------

char inportb( unsigned long int port)
{
	return inb( port);
}

#else // DOS
#endif

/*****************************************************************************
* 
*
*****************************************************************************/
#if defined(WIN32)
//----------------

void outportb( DWORD port, int value)
{
	_outp( (unsigned short)port, (int)value);
}

#elif defined(linux) //------------------

void outportb( unsigned long int port, int value)
{
	outb( (unsigned char) value, (unsigned short int) port);
}

#else // DOS
#endif

/*****************************************************************************
* 
*
*****************************************************************************/
#if defined(WIN32) || defined(linux)
//----------------------------------

void movedata(unsigned source_selector, unsigned source_offset,
              unsigned dest_selector, unsigned dest_offset,
              size_t nBytes)
{
	void	*dest, *source;
	
	dest = (void *)((char *)dest_selector + dest_offset);
	source = (void *)((char *)source_selector + source_offset);

	memcpy( dest, (const void *)source, nBytes);
}

#else // DOS
#endif
