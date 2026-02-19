/************************************************************************************\

  typedefs.h - generic sdcc typedefs

  (c) 2008-2020 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  History:

  See makefile.
  
\************************************************************************************/

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

typedef unsigned char   byte;           // 8-bit
typedef unsigned short   word;           // 16-bit
typedef unsigned long   dword;          // 32-bit

#define LITTLE_ENDIAN

#ifdef LITTLE_ENDIAN     // => 16bit: (LSB,MSB), 32bit: (LSW,MSW) or (LSB0,LSB1,LSB2,LSB3) or (MSB3,MSB2,MSB1,MSB0)
#  define MSB(word)        (((byte* )&word)[1])
#  define LSB(word)        (((byte* )&word)[0])
#  define MSW(dword)        (((word*)&dword)[1])
#  define LSW(dword)        (((word*)&dword)[0])
#  define MSB0(dword)       (((byte* )&dword)[3])
#  define MSB1(dword)       (((byte* )&dword)[2])
#  define MSB2(dword)       (((byte* )&dword)[1])
#  define MSB3(dword)       (((byte* )&dword)[0])
#  define LSB0(dword)       MSB3(dword)
#  define LSB1(dword)       MSB2(dword)
#  define LSB2(dword)       MSB1(dword)
#  define LSB3(dword)       MSB0(dword)
#else // BIG_ENDIAN         => 16bit: (MSB,LSB), 32bit: (MSW,LSW) or (LSB3,LSB2,LSB1,LSB0) or (MSB0,MSB1,MSB2,MSB3)
#  define MSB(word)        (((byte* )&word)[0])
#  define LSB(word)        (((byte* )&word)[1])
#  define MSW(dword)        (((word*)&dword)[0])
#  define LSW(dword)        (((word*)&dword)[1])
#  define MSB0(dword)       (((byte* )&dword)[0])
#  define MSB1(dword)       (((byte* )&dword)[1])
#  define MSB2(dword)       (((byte* )&dword)[2])
#  define MSB3(dword)       (((byte* )&dword)[3])
#  define LSB0(dword)       MSB3(dword)
#  define LSB1(dword)       MSB2(dword)
#  define LSB2(dword)       MSB1(dword)
#  define LSB3(dword)       MSB0(dword)
#endif

//typedef void(*pFunc)(void);

typedef enum _BOOL { FALSE = 0, TRUE } BOOL;

#define OK      TRUE
#define FAIL    FALSE

#endif //TYPEDEFS_H
