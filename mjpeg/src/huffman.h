/*******************************************************************************
 * vim:set ts=3:
 * File   : huffman.h, file for JPEG-JFIF sequential decoder    
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
#ifndef __HUFFMAN_H
#define __HUFFMAN_H

#include "jpeg.h"

extern void free_huffman_tables(huff_table_t *root) ;

extern int load_huffman_table(FILE * movie,
												  uint16_t DHT_section_length,
												  uint8_t DHT_section_info,
                                      huff_table_t * ht) ;

#endif
