/*
 * File   : utils.h, file for JPEG-JFIF Multi-thread decoder    
 *
 * Copyright (C) 2007 TIMA Laboratory
 * Author(s) :      Pascal GOMEZ
 * Bug Fixer(s) :   
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

static inline int32_t intceil (int32_t N, int32_t D)
{
   int32_t i = N / D;

   if (N > D * i) i++;
   return i;
}

static inline int32_t intfloor (int32_t N, int32_t D)
{
   int32_t i = N / D;

   if (N < D * i) i--;
   return i;
}

static inline int32_t reformat (uint32_t S, int32_t good)
{
   int32_t St = 0;

   if (good == 0) return 0;
   St = 1 << (good - 1);   /* 2^(good-1) */

   if (S < St) return (S + 1 + ((-1) << good));
   else return S;
}
#endif
