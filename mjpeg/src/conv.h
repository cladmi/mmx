/*******************************************************************************
 * vim:set ts=3:
 * File   : conv.h, file for JPEG-JFIF sequential decoder    
 *
 * Copyright (C) 2007 TIMA Laboratory
 * Author(s) :      Pierre-Henri HORREIN pierre-henri.horrein@imag.fr
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA02110-1301, USA.
 *
 ******************************************************************************/
#ifndef __CONV_H__
#define __CONV_H__

#include <stdint.h>

/*
 * Basic functions
 */

extern void YCrCb_to_ARGB(uint8_t  *YCrCb_MCU[3], uint32_t *RGB_MCU, uint32_t nb_MCU_H, uint32_t nb_MCU_V);
#endif
