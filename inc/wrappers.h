#ifndef _WRAPPER_H_INCLUDED_
#define _WRAPPER_H_INCLUDED_
/*****************************************************************************
* Author:  Brent Jarvis
* Created: Mar 2007
*
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32)

	BYTE inportb( DWORD port);
	void outportb( DWORD port, int value);

#elif defined(linux)

	#include <string.h>
	
//	int ioPermission (unsigned long int __from, unsigned long int __num,
//	                   int __turn_on);
	char inportb( unsigned long int port);
	void outportb( unsigned long int port, int value);

#else // DOS
#endif /* OS */

void movedata(unsigned source_selector, unsigned source_offset,
              unsigned dest_selector, unsigned dest_offset,
              size_t length);

#ifdef __cplusplus
}
#endif

#endif /* _WRAPPER_H_INCLUDED_ */
