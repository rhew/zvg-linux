#ifndef ZSTDDEF_H
#define ZSTDDEF_H
/*****************************************************************************
* MAME to ZVG driver routines.
*
* Author:       Zonn Moore
* Last Updated: 05/21/03
*
* History:
*
* (c) Copyright 2003-2004, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

typedef enum TLOGIC_E
{
	zFalse = 0,				// Logical False
	zTrue = 1,				// Logical True

	zNo = zFalse,			// Synounym for False
	zYes = zTrue,			// Synounym for True

	zOff = zFalse,			// Synounym for False
	zOn = zTrue,			// Synounym for True

	zReset = zFalse,
	zSet = zTrue,

	zOk = zFalse,
	zError = zTrue,

	zDisable = zFalse,
	zEnable = zTrue,
	zClear,					// Used to clear something
	zKeep,					// Used to keep something
} t_logic;

#ifndef NULL
#define	NULL	0
#endif

typedef	unsigned char	uchar;
typedef	unsigned short	ushort;
typedef	unsigned int	uint;
typedef	unsigned long	ulong;
typedef	signed char		schar;
typedef	signed short	sshort;
typedef	signed int		sint;
typedef	signed long		slong;

#ifndef bool
typedef	unsigned char	bool;
#endif

#ifdef __cplusplus
}
#endif

#endif
