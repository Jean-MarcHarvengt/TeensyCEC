/* Header file for the instruction set simulator.
   Copyright (C) 1995  Frank D. Cringle.
   Modifications for MMU and CP/M 3.1 Copyright (C) 2000/2003 by Andreas Gerlich
   

This file is part of yaze-ag - yet another Z80 emulator by ag.

Yaze-ag is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#ifndef _SIMZ80_H

#include <limits.h>

#if UCHAR_MAX == 255
typedef unsigned char	BYTE;
#else
#error Need to find an 8-bit type for BYTE
#endif

#if USHRT_MAX == 65535
typedef unsigned short	WORD;
#else
#error Need to find an 16-bit type for WORD
#endif

/* FASTREG needs to be at least 16 bits wide and efficient for the
   host architecture */
#if UINT_MAX >= 65535
typedef unsigned int	FASTREG;
#else
typedef unsigned long	FASTREG;
#endif

/* FASTWORK needs to be wider than 16 bits and efficient for the host
   architecture */
#if UINT_MAX > 65535
typedef unsigned int	FASTWORK;
#else
typedef unsigned long	FASTWORK;
#endif

struct ddregs
{
	WORD bc;
	WORD de;
	WORD hl;
};

struct z80
{
  WORD af[ 2 ];
  int  af_sel;

  struct ddregs regs[ 2 ];
  int    regs_sel;

  WORD ir;
  WORD ix;
  WORD iy;
  WORD sp;
  WORD pc;
  WORD iff;
};

extern void z80_step( struct z80* z80 );
extern BYTE z80_fetch( struct z80* z80, WORD a );
extern BYTE z80_read( struct z80* z80, WORD a );
extern void z80_write( struct z80* z80, WORD a, BYTE b );
extern BYTE z80_in( struct z80* z80, WORD a );
extern void z80_out( struct z80* z80, WORD a, BYTE b );

#endif /* _SIMZ80_H */
