# ZVG Software Development Kit for Linux

## About

The ZVG (which stands for Zektor Vector Generator) is a vector generator which
attaches to a PC through a ECP compatible parallel port, and allows a PC to control a vector monitor.

The ZVG Software Development Kit for Linux provides a C interface to the ZVG.
- sdk.txt
- zvgCmds.txt: 
<dl>
  <dt><strong>sdk.txt</strong></dt>
  <dd>Documents the SDK usage.</dd>
  <dt><strong>zvgCmds.txt</strong></dt>
  <dd>Documents the low level command set used to talk directly to the ZVG.</dd>
</dl>

More information is available on the [Zektor ZVG website](http://www.zektor.com/zvg/downloads.htm).

## Linux build instructions

Make sure you have the development libraries for Curses and Threads installed.  Then, from the repository root:

    cmake .
    make

## Copyright

The following appears throughout the source code:

> (c) Copyright 2002-2003, Zektor, LLC.  All Rights Reserved.

The ZVG Software Development Kit is hosted on GitHub by permission of Zonn Moore - CTO and Founder of Zektor, LLC. 

## Credits

- Zonn Moore wrote the [ZVG Software Development Kit 1.1a](http://www.zektor.com/zvg/downloads/zvg_sdk11a.zip) for DOS.
- Brent Jarvis ported the source to Linux, excluding DMA support.
- James Rhew packaged and currently maintains the source on GitHub.
