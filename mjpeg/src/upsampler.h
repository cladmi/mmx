/*******************************************************************************
 * vim :set ts=3:
 * File   : upsampler.h, file for JPEG-JFIF sequential decoder    
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 ******************************************************************************/
#ifndef UPSAMPLER_H
#define UPSAMPLER_H

extern void upsampler(uint8_t *MCU_ds, uint8_t *MCU_us, uint8_t h_factor, uint8_t v_factor, uint16_t nb_MCU_H, uint16_t nb_MCU_V) ;

#endif
