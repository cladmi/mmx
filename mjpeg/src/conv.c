/*******************************************************************************
 * vim:set ts=3:
 * File   : conv.c, file for JPEG-JFIF sequential decoder
 *
 * Copyright (C) 2007-2010 TIMA Laboratory
 * Author(s) :      Patrice GERIN <patrice.gerin@imag.fr>
 *                  Frédéric Pétrot <Frederic.Petrot@imag.fr>
 * Bug Fixer(s) :   Xavier GUERIN <xavier.guerin@imag.fr>
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
 *************************************************************************************/

/*
 * VERSION
 * 1 : version originale
 * 2 : version déroulée toujours en C
 * 3 : Chargement dans mmx
 * 4 : soustraction 128 par packets
 * 5 : calcul de R
 * 6 : calcul de GB
 */
#define VERSION 8
#define DEBUG




#include "conv.h"
#include "stdlib.h"
#include "stdio.h"
#include "utils.h"
#include <assert.h>
#include <inttypes.h>

#ifdef DEBUG
uint64_t to_mmx_register(uint16_t first,
                uint16_t second,
                uint16_t third,
                uint16_t fourth)
{
        uint64_t mmx = 0;

        mmx  = ((uint64_t) first) << 48;
        mmx += ((uint64_t) second) << 32;
        mmx += ((uint64_t) third) << 16;
        mmx += ((uint64_t) fourth);
        return mmx;
}
#endif



#if VERSION == 1
/* Interger version */
void YCrCb_to_ARGB(uint8_t *YCrCb_MCU[3], uint32_t *RGB_MCU, uint32_t nb_MCU_H, uint32_t nb_MCU_V)
{
        uint8_t *MCU_Y, *MCU_Cr, *MCU_Cb;
        int32_t R, G, B;
        uint32_t ARGB;
        uint8_t index, i, j;

        MCU_Y = YCrCb_MCU[0];
        MCU_Cr = YCrCb_MCU[2];
        MCU_Cb = YCrCb_MCU[1];
        for (i = 0; i < 8 * nb_MCU_V; i++) {
                for (j = 0; j < 8 * nb_MCU_H; j++) {
                        index = i * (8 * nb_MCU_H)  + j;
                        R = ((MCU_Y[index] << 8) + (MCU_Cr[index] - 128) * 359) >> 8;
                        G = ((MCU_Y[index] << 8)
                                        - (MCU_Cr[index] - 128) * 183
                                        - (MCU_Cb[index] - 128) * 88) >> 8;
                        B = ((MCU_Y[index] << 8) + (MCU_Cb[index] - 128) * 455) >> 8;
                        /* Saturate */
                        if (R > 255) R = 255;
                        if (R < 0)   R = 0;
                        if (G > 255) G = 255;
                        if (G < 0)   G = 0;
                        if (B > 255) B = 255;
                        if (B < 0)   B = 0;
                        ARGB = ((R & 0xFF) << 16) | ((G & 0xFF) << 8) | (B & 0xFF);
                        RGB_MCU[index] = ARGB;
                }
        }
}
#endif
#if VERSION == 2
void YCrCb_to_ARGB(uint8_t *YCrCb_MCU[3], uint32_t *RGB_MCU, uint32_t nb_MCU_H, uint32_t nb_MCU_V)
{
        uint8_t *MCU_Y, *MCU_Cr, *MCU_Cb;
        uint8_t index, i, j, idx;
        int32_t R[4], G[4], B[4];
        uint32_t ARGB[4];

        MCU_Y = YCrCb_MCU[0];
        MCU_Cr = YCrCb_MCU[2];
        MCU_Cb = YCrCb_MCU[1];

        for (i = 0; i < 8 * nb_MCU_V; i++) {
                for (j = 0; j < 8 * nb_MCU_H; j += 4) {
                        index = i * (8 * nb_MCU_H) + j;
                        for (idx = 0; idx < 4; idx++) {
                                R[idx] = ((MCU_Y[index + idx] << 8) +
                                                (MCU_Cr[index + idx] - 128) * 359) >> 8;
                                G[idx] = ((MCU_Y[index + idx] << 8) -
                                                (MCU_Cr[index + idx] - 128) * 183 -
                                                (MCU_Cb[index + idx] - 128) * 88) >> 8;
                                B[idx] = ((MCU_Y[index + idx] << 8) +
                                                (MCU_Cb[index + idx] - 128) * 455) >> 8;
                                if (R[idx] > 255)
                                        R[idx] = 255;
                                if (G[idx] > 255)
                                        G[idx] = 255;
                                if (B[idx] > 255)
                                        B[idx] = 255;
                                if (R[idx] < 0)
                                        R[idx] = 0;
                                if (G[idx] < 0)
                                        G[idx] = 0;
                                if (B[idx] < 0)
                                        B[idx] = 0;
                                ARGB[idx] = ((R[idx] & 0xff) << 16) |
                                        ((G[idx] & 0xff) << 8) |
                                        (B[idx] & 0xff);
                                RGB_MCU[index + idx] = ARGB[idx];
                        }
                }
        }
}
#endif
#if VERSION == 3
void YCrCb_to_ARGB(uint8_t *YCrCb_MCU[3], uint32_t *RGB_MCU, uint32_t nb_MCU_H, uint32_t nb_MCU_V)
{
        uint8_t *MCU_Y, *MCU_Cr, *MCU_Cb;
        uint8_t index, i, j, idx;
        int32_t R[4], G[4], B[4];
        uint32_t ARGB[4];
#ifdef VERSION
        printf("Version %d", VERSION);
#endif
#ifdef DEBUG
        printf(" (debug)\n");
#else
        printf("\n");
#endif

        /* Vide la pile des instructions flottantes */
        __asm__("emms");
        __asm__("pxor %mm7, %mm7");

        MCU_Y = YCrCb_MCU[0];
        MCU_Cr = YCrCb_MCU[2];
        MCU_Cb = YCrCb_MCU[1];

        for (i = 0; i < 8 * nb_MCU_V; i++) {
                for (j = 0; j < 8 * nb_MCU_H; j += 4) {
                        index = i * (8 * nb_MCU_H) + j;

                        // 0x00 | MCU_Y[index + 3] | 0x00 | MCU_Y[index + 2] | 0x00 | MCU_Y[index + 1] | 0x00 | MCU_Y[index + 0] -> %mm0
                        __asm__("movq %0, %%mm0"::"m"(MCU_Y[index]));
                        __asm__("punpcklbw %mm7, %mm0");
#                       ifdef DEBUG
                        uint16_t Y[4];
                        __asm__("movq %%mm0, %0":"=m"(Y[0]));
                        assert(MCU_Y[index] == Y[0]);
                        assert(MCU_Y[index + 1] == Y[1]);
                        assert(MCU_Y[index + 2] == Y[2]);
                        assert(MCU_Y[index + 3] == Y[3]);
#                       endif


                        __asm__("movq %0, %%mm1"::"m"(MCU_Cr[index]));
                        __asm__("punpcklbw %mm7, %mm1");
#                       ifdef DEBUG
                        uint16_t Cr[4];
                        __asm__("movq %%mm1, %0":"=m"(Cr[0]));
                        assert(MCU_Cr[index] == Cr[0]);
                        assert(MCU_Cr[index + 1] == Cr[1]);
                        assert(MCU_Cr[index + 2] == Cr[2]);
                        assert(MCU_Cr[index + 3] == Cr[3]);
#                       endif

                        __asm__("movq %0, %%mm2"::"m"(MCU_Cb[index]));
                        __asm__("punpcklbw %mm7, %mm2");
#                       ifdef DEBUG
                        uint16_t Cb[4];
                        __asm__("movq %%mm2, %0":"=m"(Cb[0]));
                        assert(MCU_Cb[index] == Cb[0]);
                        assert(MCU_Cb[index + 1] == Cb[1]);
                        assert(MCU_Cb[index + 2] == Cb[2]);
                        assert(MCU_Cb[index + 3] == Cb[3]);
#                       endif

                        for (idx = 0; idx < 4; idx++) {
                                R[idx] = ((MCU_Y[index + idx] << 8) +
                                                (MCU_Cr[index + idx] - 128) * 359) >> 8;
                                G[idx] = ((MCU_Y[index + idx] << 8) -
                                                (MCU_Cr[index + idx] - 128) * 183 -
                                                (MCU_Cb[index + idx] - 128) * 88) >> 8;
                                B[idx] = ((MCU_Y[index + idx] << 8) +
                                                (MCU_Cb[index + idx] - 128) * 455) >> 8;
                                if (R[idx] > 255)
                                        R[idx] = 255;
                                if (G[idx] > 255)
                                        G[idx] = 255;
                                if (B[idx] > 255)
                                        B[idx] = 255;
                                if (R[idx] < 0)
                                        R[idx] = 0;
                                if (G[idx] < 0)
                                        G[idx] = 0;
                                if (B[idx] < 0)
                                        B[idx] = 0;
                                ARGB[idx] = ((R[idx] & 0xff) << 16) |
                                        ((G[idx] & 0xff) << 8) |
                                        (B[idx] & 0xff);
                                RGB_MCU[index + idx] = ARGB[idx];
                        }
                }
        }
}
#endif
#if VERSION == 4
void YCrCb_to_ARGB(uint8_t *YCrCb_MCU[3], uint32_t *RGB_MCU, uint32_t nb_MCU_H, uint32_t nb_MCU_V)
{
        uint8_t *MCU_Y, *MCU_Cr, *MCU_Cb;
        uint8_t index, i, j, idx;
        int32_t R[4], G[4], B[4];
        uint32_t ARGB[4];
        // Soustraction de 128 sur chacun des 16bits d'un registre mmx
        const uint64_t magic_substract = 0x0080008000800080;

#ifdef VERSION
        printf("Version %d", VERSION);
#endif
#ifdef DEBUG
        printf(" (debug)\n");
#else
        printf("\n");
#endif

        /* Vide la pile des instructions flottantes */
        __asm__("emms");
        __asm__("pxor %mm7, %mm7");

        MCU_Y = YCrCb_MCU[0];
        MCU_Cr = YCrCb_MCU[2];
        MCU_Cb = YCrCb_MCU[1];

        for (i = 0; i < 8 * nb_MCU_V; i++) {
                for (j = 0; j < 8 * nb_MCU_H; j += 4) {
                        index = i * (8 * nb_MCU_H) + j;

                        // 0x00 | MCU_Y[index + 3] | 0x00 | MCU_Y[index + 2] | 0x00 | MCU_Y[index + 1] | 0x00 | MCU_Y[index + 0] -> %mm0
                        __asm__("movq %0, %%mm0"::"m"(MCU_Y[index]));
                        __asm__("punpcklbw %mm7, %mm0");
#                       ifdef DEBUG
                        uint16_t Y[4];
                        __asm__("movq %%mm0, %0":"=m"(Y[0]));
                        assert(MCU_Y[index] == Y[0]);
                        assert(MCU_Y[index + 1] == Y[1]);
                        assert(MCU_Y[index + 2] == Y[2]);
                        assert(MCU_Y[index + 3] == Y[3]);
#                       endif


                        __asm__("movq %0, %%mm1"::"m"(MCU_Cr[index]));
                        __asm__("punpcklbw %mm7, %mm1");
#                       ifdef DEBUG
                        // test du chargement de Cr dans mm1
                        uint16_t Cr[4];
                        __asm__("movq %%mm1, %0":"=m"(Cr[0]));
                        assert(MCU_Cr[index] == Cr[0]);
                        assert(MCU_Cr[index + 1] == Cr[1]);
                        assert(MCU_Cr[index + 2] == Cr[2]);
                        assert(MCU_Cr[index + 3] == Cr[3]);
#                       endif
                        // soustraction
                        __asm__("psubw %0, %%mm1"::"m"(magic_substract));
#                       ifdef DEBUG
                        // test après soustraction
                        __asm__("movq %%mm1, %0":"=m"(Cr[0]));
                        assert((uint16_t) (MCU_Cr[index] - 128) == Cr[0]);
                        assert((uint16_t) (MCU_Cr[index + 1] - 128) == Cr[1]);
                        assert((uint16_t) (MCU_Cr[index + 2] - 128) == Cr[2]);
                        assert((uint16_t) (MCU_Cr[index + 3] - 128) == Cr[3]);
#                       endif

                        __asm__("movq %0, %%mm2"::"m"(MCU_Cb[index]));
                        __asm__("punpcklbw %mm7, %mm2");
#                       ifdef DEBUG
                        // test du chargement de Cb dans mm2
                        uint16_t Cb[4];
                        __asm__("movq %%mm2, %0":"=m"(Cb[0]));
                        assert(MCU_Cb[index] == Cb[0]);
                        assert(MCU_Cb[index + 1] == Cb[1]);
                        assert(MCU_Cb[index + 2] == Cb[2]);
                        assert(MCU_Cb[index + 3] == Cb[3]);
#                       endif
                        // soustraction
                        __asm__("psubw %0, %%mm2"::"m"(magic_substract));
#                       ifdef DEBUG
                        // test après soustraction
                        __asm__("movq %%mm2, %0":"=m"(Cb[0]));
                        assert((uint16_t) (MCU_Cb[index] - 128)== Cb[0]);
                        assert((uint16_t) (MCU_Cb[index + 1] - 128) == Cb[1]);
                        assert((uint16_t) (MCU_Cb[index + 2] - 128) == Cb[2]);
                        assert((uint16_t) (MCU_Cb[index + 3] - 128) == Cb[3]);
#                       endif

                        for (idx = 0; idx < 4; idx++) {
                                R[idx] = ((MCU_Y[index + idx] << 8) +
                                                (MCU_Cr[index + idx] - 128) * 359) >> 8;
                                G[idx] = ((MCU_Y[index + idx] << 8) -
                                                (MCU_Cr[index + idx] - 128) * 183 -
                                                (MCU_Cb[index + idx] - 128) * 88) >> 8;
                                B[idx] = ((MCU_Y[index + idx] << 8) +
                                                (MCU_Cb[index + idx] - 128) * 455) >> 8;
                                if (R[idx] > 255)
                                        R[idx] = 255;
                                if (G[idx] > 255)
                                        G[idx] = 255;
                                if (B[idx] > 255)
                                        B[idx] = 255;
                                if (R[idx] < 0)
                                        R[idx] = 0;
                                if (G[idx] < 0)
                                        G[idx] = 0;
                                if (B[idx] < 0)
                                        B[idx] = 0;
                                ARGB[idx] = ((R[idx] & 0xff) << 16) |
                                        ((G[idx] & 0xff) << 8) |
                                        (B[idx] & 0xff);
                                RGB_MCU[index + idx] = ARGB[idx];
                        }
                }
        }
}
#endif
#if VERSION == 5


void YCrCb_to_ARGB(uint8_t *YCrCb_MCU[3], uint32_t *RGB_MCU, uint32_t nb_MCU_H, uint32_t nb_MCU_V)
{
        uint8_t *MCU_Y, *MCU_Cr, *MCU_Cb;
        uint8_t index, i, j, idx;
        int32_t R[4], G[4], B[4];
        uint32_t ARGB[4];
        // Soustraction de 128 sur chacun des 16bits d'un registre mmx
        const uint64_t magic_substract = 0x0080008000800080;
        // Constante de multiplication
        //
        // R : 359 donc 0x0167 0167 0167 0167
        const uint64_t R_mult_constant = 0x0167016701670167;
        // G(Cr) : 183 donc 0x00b7 00b7 00b7 00b7
        const uint64_t G_cr_mult_constant = 0x00b700b700b700b7;
        // G(Cb) : 88 donc 0x0058 0058 0058 0058
        const uint64_t G_cb_mult_constant = 0x0058005800580058;
        // B(Cb) : 455 donc 0x01c7 01c7 01c7 01c7
        const uint64_t B_cb_mult_constant = 0x01c701c701c701c7;

#ifdef VERSION
        printf("Version %d", VERSION);
#endif
#ifdef DEBUG
        printf(" (debug)\n");
#else
        printf("\n");
#endif

        /* Vide la pile des instructions flottantes */
        __asm__("emms");

        MCU_Y = YCrCb_MCU[0];
        MCU_Cr = YCrCb_MCU[2];
        MCU_Cb = YCrCb_MCU[1];

        for (i = 0; i < 8 * nb_MCU_V; i++) {
                for (j = 0; j < 8 * nb_MCU_H; j += 4) {
                        index = i * (8 * nb_MCU_H) + j;
                        // calcul de référence
                        for (idx = 0; idx < 4; idx++) {
                                R[idx] = ((MCU_Y[index + idx] << 8) +
                                                (MCU_Cr[index + idx] - 128) * 359) >> 8;
                                G[idx] = ((MCU_Y[index + idx] << 8) -
                                                (MCU_Cr[index + idx] - 128) * 183 -
                                                (MCU_Cb[index + idx] - 128) * 88) >> 8;
                                B[idx] = ((MCU_Y[index + idx] << 8) +
                                                (MCU_Cb[index + idx] - 128) * 455) >> 8;
                                if (R[idx] > 255)
                                        R[idx] = 255;
                                if (G[idx] > 255)
                                        G[idx] = 255;
                                if (B[idx] > 255)
                                        B[idx] = 255;
                                if (R[idx] < 0)
                                        R[idx] = 0;
                                if (G[idx] < 0)
                                        G[idx] = 0;
                                if (B[idx] < 0)
                                        B[idx] = 0;
                                ARGB[idx] = ((R[idx] & 0xff) << 16) |
                                        ((G[idx] & 0xff) << 8) |
                                        (B[idx] & 0xff);
                                RGB_MCU[index + idx] = ARGB[idx];
                        }

                        // chargement de Y dans mm0
                        // 0x00 | MCU_Y[index + 3] | 0x00 | MCU_Y[index + 2] |
                        // 0x00 | MCU_Y[index + 1] | 0x00 | MCU_Y[index + 0] -> %mm0
                        //
                        __asm__("pxor %mm7, %mm7");
                        __asm__("movq %0, %%mm0"::"m"(MCU_Y[index]));
                        __asm__("punpcklbw %mm7, %mm0");
#ifdef DEBUG
                        uint16_t Y[4];
                        __asm__("movq %%mm0, %0":"=m"(Y[0]));
                        assert(MCU_Y[index]     == Y[0]);
                        assert(MCU_Y[index + 1] == Y[1]);
                        assert(MCU_Y[index + 2] == Y[2]);
                        assert(MCU_Y[index + 3] == Y[3]);
#endif

                        // chargement de CR -> mm1
                        __asm__("pxor %mm7, %mm7");
                        __asm__("movq %0, %%mm1"::"m"(MCU_Cr[index]));
                        __asm__("punpcklbw %mm7, %mm1");
#ifdef DEBUG
                        // test du chargement de Cr dans mm1
                        uint16_t Cr[4];
                        __asm__("movq %%mm1, %0":"=m"(Cr[0]));
                        assert(MCU_Cr[index] == Cr[0]);
                        assert(MCU_Cr[index + 1] == Cr[1]);
                        assert(MCU_Cr[index + 2] == Cr[2]);
                        assert(MCU_Cr[index + 3] == Cr[3]);
#endif

                        // soustraction CR -128
                        __asm__("psubw %0, %%mm1"::"m"(magic_substract));
#ifdef DEBUG
                        // test après soustraction
                        __asm__("movq %%mm1, %0":"=m"(Cr[0]));
                        assert((uint16_t) (MCU_Cr[index] - 128) == Cr[0]);
                        assert((uint16_t) (MCU_Cr[index + 1] - 128) == Cr[1]);
                        assert((uint16_t) (MCU_Cr[index + 2] - 128) == Cr[2]);
                        assert((uint16_t) (MCU_Cr[index + 3] - 128) == Cr[3]);
#endif

                        // chargement CB -> mm2
                        __asm__("movq %0, %%mm2"::"m"(MCU_Cb[index]));
                        __asm__("punpcklbw %mm7, %mm2");
#ifdef DEBUG
                        // test du chargement de Cb dans mm2
                        uint16_t Cb[4];
                        __asm__("movq %%mm2, %0":"=m"(Cb[0]));
                        assert(MCU_Cb[index] == Cb[0]);
                        assert(MCU_Cb[index + 1] == Cb[1]);
                        assert(MCU_Cb[index + 2] == Cb[2]);
                        assert(MCU_Cb[index + 3] == Cb[3]);
#endif

                        // soustraction CB - 128
                        __asm__("psubw %0, %%mm2"::"m"(magic_substract));
#ifdef DEBUG
                        // test après soustraction
                        __asm__("movq %%mm2, %0":"=m"(Cb[0]));
                        assert((uint16_t) (MCU_Cb[index] - 128)== Cb[0]);
                        assert((uint16_t) (MCU_Cb[index + 1] - 128) == Cb[1]);
                        assert((uint16_t) (MCU_Cb[index + 2] - 128) == Cb[2]);
                        assert((uint16_t) (MCU_Cb[index + 3] - 128) == Cb[3]);
#endif



                        /*
                         * * * * * * * *
                         * Calcul de R *
                         * * * * * * * *
                         *
                         * R = (((Y << 8) + (Cr - 128) * 359)) >> 8
                         * On dévelloppe le >> 8, car ici c'est autorisé
                         * R = (MM0)  +   ((MM1) * 359) >> 8
                         *
                         */

                        /*
                         * multiplication
                         */
                        __asm__("movq %mm1, %mm3");
#ifdef DEBUG
                        uint64_t mm1;
                        __asm__("movq %%mm3, %0":"=m"(mm1));
                        uint64_t mm1_theorique = to_mmx_register(
                                        MCU_Cr[index + 3] - 128,
                                        MCU_Cr[index + 2] - 128,
                                        MCU_Cr[index + 1] - 128,
                                        MCU_Cr[index + 0] - 128);
                        assert(mm1 == mm1_theorique);
#endif
                        __asm__("movq %mm3, %mm6");
                        __asm__("pmullw %0, %%mm3"::"m"(R_mult_constant));
                        // la partie haute vaut soit 0000 soit ffff, pour dire le signe
                        // l'information dans la partie basse n'est pas suffisante
                        // exemple tiré du traitement d'une image
                        // 	High mult vaut 0x0000000000000000
                        // 	low mult vaut  0x91d891d88c3c8c3c
                        // En regardant uniquement la partie basse,
                        // on pourrait croire que c'est un nombre négatif mais il est positif
                        __asm__("pmulhw %0, %%mm6"::"m"(R_mult_constant));
                        uint64_t temp;
                        __asm__("movq %%mm6, %0":"=m"(temp));
                        printf("High mult vaut 0x%016"PRIx64"\t", temp);
                        __asm__("movq %%mm3, %0":"=m"(temp));
                        printf("low mult vaut  0x%016"PRIx64"\n", temp);
#ifdef DEBUG
                        uint64_t mult_after;
                        __asm__("movq %%mm3, %0":"=m"(mult_after));
                        uint64_t mult_after_theorique = to_mmx_register(
                                        (MCU_Cr[index + 3] - 128) * 359,
                                        (MCU_Cr[index + 2] - 128) * 359,
                                        (MCU_Cr[index + 1] - 128) * 359,
                                        (MCU_Cr[index + 0] - 128) * 359);
                        assert(mult_after == mult_after_theorique);
#endif
                        // on va passer sur 4 entiers de 32 bits
                        __asm__("movq %mm3, %mm7");
                        __asm__("punpcklwd %mm6, %mm3");
                        __asm__("punpckhwd %mm6, %mm7");
                        // on fait le décalage arithmétique à droite de 8
                        __asm__("psrad $8, %mm3");
                        __asm__("psrad $8, %mm7");
                        // on va repasser sur 16 bits
                        __asm__("packssdw %mm7, %mm3");
                        __asm__("movq %%mm3, %0":"=m"(temp));

                        printf("mult >> 8 vaut 0x%016"PRIx64"\n", temp);


                        // addition
#ifdef DEBUG
                        uint64_t mm0;
                        __asm__("movq %%mm0, %0":"=m"(mm0));
                        uint64_t mm0_theorique = to_mmx_register(
                                        (MCU_Y[index + 3]),
                                        (MCU_Y[index + 2]),
                                        (MCU_Y[index + 1]),
                                        (MCU_Y[index + 0]));
                        assert(mm0 == mm0_theorique);
#endif
                        __asm__("paddw %mm0, %mm3");
#ifdef DEBUG
                        uint64_t add_after;
                        __asm__("movq %%mm3, %0":"=m"(add_after));
                        uint64_t result_theorique = to_mmx_register(
                                        ((((MCU_Y[index + 3]) << 8) + ((MCU_Cr[index + 3] - 128) * 359)) >> 8),
                                        ((((MCU_Y[index + 2]) << 8) + ((MCU_Cr[index + 2] - 128) * 359)) >> 8),
                                        ((((MCU_Y[index + 1]) << 8) + ((MCU_Cr[index + 1] - 128) * 359)) >> 8),
                                        ((((MCU_Y[index + 0]) << 8) + ((MCU_Cr[index + 0] - 128) * 359)) >> 8));
                        if (add_after != result_theorique) {
                                printf("addt 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after_theorique,
                                                mm0_theorique,
                                                result_theorique);
                                printf("addm 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after,
                                                mm0,
                                                add_after);
                                printf("\n");
                                assert(0);
                        }
#endif

                }
        }
}
#endif
#if VERSION == 6


void YCrCb_to_ARGB(uint8_t *YCrCb_MCU[3], uint32_t *RGB_MCU, uint32_t nb_MCU_H, uint32_t nb_MCU_V)
{
        uint8_t *MCU_Y, *MCU_Cr, *MCU_Cb;
        uint8_t index, i, j, idx;
        int32_t R[4], G[4], B[4];
        uint32_t ARGB[4];
        // Soustraction de 128 sur chacun des 16bits d'un registre mmx
        const uint64_t magic_substract = 0x0080008000800080;
        // Constante de multiplication
        //
        // R : 359 donc 0x0167 0167 0167 0167
        const uint64_t R_mult_constant = 0x0167016701670167;
        // G(Cr) : 183 donc 0x00b7 00b7 00b7 00b7
        const uint64_t G_cr_mult_constant = 0x00b700b700b700b7;
        // G(Cb) : 88 donc 0x0058 0058 0058 0058
        const uint64_t G_cb_mult_constant = 0x0058005800580058;
        // B(Cb) : 455 donc 0x01c7 01c7 01c7 01c7
        const uint64_t B_cb_mult_constant = 0x01c701c701c701c7;



        uint64_t temp;
        uint64_t mm1;
        uint64_t mm1_theorique;
        uint64_t mult_after;
        uint64_t mult_after_theorique;
        uint64_t mm0;
        uint64_t mm0_theorique;
        uint64_t add_after;
        uint64_t result_theorique;

#ifdef VERSION
        printf("Version %d", VERSION);
#endif
#ifdef DEBUG
        printf(" (debug)\n");
#else
        printf("\n");
#endif

        /* Vide la pile des instructions flottantes */
        __asm__("emms");

        MCU_Y = YCrCb_MCU[0];
        MCU_Cr = YCrCb_MCU[2];
        MCU_Cb = YCrCb_MCU[1];

        for (i = 0; i < 8 * nb_MCU_V; i++) {
                for (j = 0; j < 8 * nb_MCU_H; j += 4) {
                        index = i * (8 * nb_MCU_H) + j;
                        // calcul de référence
                        for (idx = 0; idx < 4; idx++) {
                                R[idx] = ((MCU_Y[index + idx] << 8) +
                                                (MCU_Cr[index + idx] - 128) * 359) >> 8;
                                G[idx] = ((MCU_Y[index + idx] << 8) -
                                                (MCU_Cr[index + idx] - 128) * 183 -
                                                (MCU_Cb[index + idx] - 128) * 88) >> 8;
                                B[idx] = ((MCU_Y[index + idx] << 8) +
                                                (MCU_Cb[index + idx] - 128) * 455) >> 8;
                                if (R[idx] > 255)
                                        R[idx] = 255;
                                if (G[idx] > 255)
                                        G[idx] = 255;
                                if (B[idx] > 255)
                                        B[idx] = 255;
                                if (R[idx] < 0)
                                        R[idx] = 0;
                                if (G[idx] < 0)
                                        G[idx] = 0;
                                if (B[idx] < 0)
                                        B[idx] = 0;
                                ARGB[idx] = ((R[idx] & 0xff) << 16) |
                                        ((G[idx] & 0xff) << 8) |
                                        (B[idx] & 0xff);
                                RGB_MCU[index + idx] = ARGB[idx];
                        }

                        // chargement de Y dans mm0
                        // 0x00 | MCU_Y[index + 3] | 0x00 | MCU_Y[index + 2] |
                        // 0x00 | MCU_Y[index + 1] | 0x00 | MCU_Y[index + 0] -> %mm0
                        //
                        __asm__("pxor %mm7, %mm7");
                        __asm__("movq %0, %%mm0"::"m"(MCU_Y[index]));
                        __asm__("punpcklbw %mm7, %mm0");
#ifdef DEBUG
                        uint16_t Y[4];
                        __asm__("movq %%mm0, %0":"=m"(Y[0]));
                        assert(MCU_Y[index]     == Y[0]);
                        assert(MCU_Y[index + 1] == Y[1]);
                        assert(MCU_Y[index + 2] == Y[2]);
                        assert(MCU_Y[index + 3] == Y[3]);
#endif

                        // chargement de CR -> mm1
                        __asm__("pxor %mm7, %mm7");
                        __asm__("movq %0, %%mm1"::"m"(MCU_Cr[index]));
                        __asm__("punpcklbw %mm7, %mm1");
#ifdef DEBUG
                        // test du chargement de Cr dans mm1
                        uint16_t Cr[4];
                        __asm__("movq %%mm1, %0":"=m"(Cr[0]));
                        assert(MCU_Cr[index] == Cr[0]);
                        assert(MCU_Cr[index + 1] == Cr[1]);
                        assert(MCU_Cr[index + 2] == Cr[2]);
                        assert(MCU_Cr[index + 3] == Cr[3]);
#endif

                        // soustraction CR -128
                        __asm__("psubw %0, %%mm1"::"m"(magic_substract));
#ifdef DEBUG
                        // test après soustraction
                        __asm__("movq %%mm1, %0":"=m"(Cr[0]));
                        assert((uint16_t) (MCU_Cr[index] - 128) == Cr[0]);
                        assert((uint16_t) (MCU_Cr[index + 1] - 128) == Cr[1]);
                        assert((uint16_t) (MCU_Cr[index + 2] - 128) == Cr[2]);
                        assert((uint16_t) (MCU_Cr[index + 3] - 128) == Cr[3]);
#endif

                        // chargement CB -> mm2
                        __asm__("movq %0, %%mm2"::"m"(MCU_Cb[index]));
                        __asm__("punpcklbw %mm7, %mm2");
#ifdef DEBUG
                        // test du chargement de Cb dans mm2
                        uint16_t Cb[4];
                        __asm__("movq %%mm2, %0":"=m"(Cb[0]));
                        assert(MCU_Cb[index] == Cb[0]);
                        assert(MCU_Cb[index + 1] == Cb[1]);
                        assert(MCU_Cb[index + 2] == Cb[2]);
                        assert(MCU_Cb[index + 3] == Cb[3]);
#endif

                        // soustraction CB - 128
                        __asm__("psubw %0, %%mm2"::"m"(magic_substract));
#ifdef DEBUG
                        // test après soustraction
                        __asm__("movq %%mm2, %0":"=m"(Cb[0]));
                        assert((uint16_t) (MCU_Cb[index] - 128)== Cb[0]);
                        assert((uint16_t) (MCU_Cb[index + 1] - 128) == Cb[1]);
                        assert((uint16_t) (MCU_Cb[index + 2] - 128) == Cb[2]);
                        assert((uint16_t) (MCU_Cb[index + 3] - 128) == Cb[3]);
#endif

                        /* on va faire dans l'ordre
                         * 	calcul de G
                         * 	calcul de R
                         * 	calcul de B
                         * 	On fait G en premier car on a besoin de plus de registres pour le calculer
                         */

                        printf("Calcul de G\n");
                        /*
                         * * * * * * * *
                         * Calcul de G *
                         * * * * * * * *
                         *
                         * G = ((Y << 8) - ((Cr - 128) * 183) - ((Cb - 128) * 88)) >> 8
                         *
                         * On dévelloppe le >> 8, car ici c'est autorisé
                         *              G = Y - ((MM1 * 183) + (MM2 * 88)) >> 8
                         *
                         */

                        /*
                         * multiplication Cr
                         */
                        __asm__("movq %mm1, %mm4");
#ifdef DEBUG
                        __asm__("movq %%mm4, %0":"=m"(mm1));
                        mm1_theorique = to_mmx_register(
                                        MCU_Cr[index + 3] - 128,
                                        MCU_Cr[index + 2] - 128,
                                        MCU_Cr[index + 1] - 128,
                                        MCU_Cr[index + 0] - 128);
                        assert(mm1 == mm1_theorique);
#endif
                        __asm__("movq %mm4, %mm7");

                        __asm__("pmullw %0, %%mm4"::"m"(G_cr_mult_constant));
                        // la partie haute vaut soit 0000 soit ffff, pour dire le signe
                        __asm__("pmulhw %0, %%mm7"::"m"(G_cr_mult_constant));


                        __asm__("movq %%mm7, %0":"=m"(temp));
                        printf("High mult vaut 0x%016"PRIx64"\t", temp);
                        __asm__("movq %%mm4, %0":"=m"(temp));
                        printf("low mult vaut  0x%016"PRIx64"\n", temp);
#ifdef DEBUG
                        __asm__("movq %%mm4, %0":"=m"(mult_after));
                        mult_after_theorique = to_mmx_register(
                                        (MCU_Cr[index + 3] - 128) * 183,
                                        (MCU_Cr[index + 2] - 128) * 183,
                                        (MCU_Cr[index + 1] - 128) * 183,
                                        (MCU_Cr[index + 0] - 128) * 183);
                        assert(mult_after == mult_after_theorique);
                        printf("Mutl_the vaut  0x%016"PRIx64"\n", mult_after_theorique);
#endif
                        // on va passer sur 4 entiers de 32 bits
                        __asm__("movq %mm4, %mm5");
                        __asm__("punpcklwd %mm7, %mm4");
                        __asm__("punpckhwd %mm7, %mm5");


                        /*
                         * multiplication Cb
                         */
                        __asm__("movq %mm2, %mm6");
#ifdef DEBUG
                        __asm__("movq %%mm6, %0":"=m"(mm1));
                        mm1_theorique = to_mmx_register(
                                        MCU_Cb[index + 3] - 128,
                                        MCU_Cb[index + 2] - 128,
                                        MCU_Cb[index + 1] - 128,
                                        MCU_Cb[index + 0] - 128);
                        assert(mm1 == mm1_theorique);
#endif
                        __asm__("movq %mm6, %mm7");
                        __asm__("pmullw %0, %%mm6"::"m"(G_cb_mult_constant));
                        // la partie haute vaut soit 0000 soit ffff, pour dire le signe
                        __asm__("pmulhw %0, %%mm7"::"m"(G_cb_mult_constant));


                        __asm__("movq %%mm7, %0":"=m"(temp));
                        printf("High mult vaut 0x%016"PRIx64"\t", temp);
                        __asm__("movq %%mm6, %0":"=m"(temp));
                        printf("low mult vaut  0x%016"PRIx64"\n", temp);
#ifdef DEBUG
                        __asm__("movq %%mm6, %0":"=m"(mult_after));
                        mult_after_theorique = to_mmx_register(
                                        (MCU_Cb[index + 3] - 128) * 88,
                                        (MCU_Cb[index + 2] - 128) * 88,
                                        (MCU_Cb[index + 1] - 128) * 88,
                                        (MCU_Cb[index + 0] - 128) * 88);
                        assert(mult_after == mult_after_theorique);
                        printf("Mutl_the vaut  0x%016"PRIx64"\n", mult_after_theorique);
#endif
                        // on va passer sur 4 entiers de 32 bits
                        // On va utiliser mm3 comme temporaire
                        __asm__("movq %mm6, %mm3");
                        __asm__("punpcklwd %mm7, %mm6");
                        __asm__("punpckhwd %mm7, %mm3");

                        __asm__("movq %mm3, %mm7");


                        printf("\n");
                        // on va additionner
                        __asm__("paddd %mm4, %mm6");
                        __asm__("paddd %mm5, %mm7");

#ifdef DEBUG
                        __asm__("movq %%mm6, %0":"=m"(temp));
                        printf("Sum == 0x%016"PRIx64"\n", temp);
                        __asm__("movq %%mm7, %0":"=m"(temp));
                        printf("Sum == 0x%016"PRIx64"\n", temp);
                        add_after = to_mmx_register(
                                        + ((MCU_Cr[index + 3] - 128) * 183)
                                        + ((MCU_Cb[index + 3] - 128) * 88),
                                        + ((MCU_Cr[index + 2] - 128) * 183)
                                        + ((MCU_Cb[index + 2] - 128) * 88),
                                        + ((MCU_Cr[index + 1] - 128) * 183)
                                        + ((MCU_Cb[index + 1] - 128) * 88),
                                        + ((MCU_Cr[index + 0] - 128) * 183)
                                        + ((MCU_Cb[index + 0] - 128) * 88));
                        printf("Sumt = 0x%016"PRIx64"\n", add_after);
#endif

                        // passage en négatif
                        __asm__("pxor %mm4, %mm4");
                        __asm__("pxor %mm5, %mm5");
                        __asm__("psubd %mm6, %mm4");
                        __asm__("psubd %mm7, %mm5");


#ifdef DEBUG
                        // Print de la somme des nombres négatifs
                        __asm__("movq %%mm4, %0":"=m"(temp));
                        printf("Sub == 0x%016"PRIx64"\n", temp);
                        __asm__("movq %%mm5, %0":"=m"(temp));
                        printf("Sub == 0x%016"PRIx64"\n", temp);
                        add_after = to_mmx_register(
                                        - ((MCU_Cr[index + 3] - 128) * 183)
                                        - ((MCU_Cb[index + 3] - 128) * 88),
                                        - ((MCU_Cr[index + 2] - 128) * 183)
                                        - ((MCU_Cb[index + 2] - 128) * 88),
                                        - ((MCU_Cr[index + 1] - 128) * 183)
                                        - ((MCU_Cb[index + 1] - 128) * 88),
                                        - ((MCU_Cr[index + 0] - 128) * 183)
                                        - ((MCU_Cb[index + 0] - 128) * 88));
                        printf("Subt = 0x%016"PRIx64"\n", add_after);
#endif
                        // on fait le décalage arithmétique à droite de 8
                        __asm__("psrad $8, %mm4");
                        __asm__("psrad $8, %mm5");

#ifdef DEBUG
                        // print après décalage
                        __asm__("movq %%mm4, %0":"=m"(temp));
                        printf("Dec == 0x%016"PRIx64"\n", temp);
                        __asm__("movq %%mm5, %0":"=m"(temp));
                        printf("Dec == 0x%016"PRIx64"\n", temp);
#endif

                        // on va repasser sur 16 bits
                        __asm__("packssdw %mm5, %mm4");

                        __asm__("movq %%mm4, %0":"=m"(temp));
                        printf("(- (mult + mul)) >> 8 vaut 0x%016"PRIx64"\n", temp);
                        temp = to_mmx_register(
                                        ((
                                          - ((MCU_Cr[index + 3] - 128) * 183)
                                          - ((MCU_Cb[index + 3] - 128) * 88)) >> 8),
                                        ((
                                          - ((MCU_Cr[index + 2] - 128) * 183)
                                          - ((MCU_Cb[index + 2] - 128) * 88)) >> 8),
                                        ((
                                          - ((MCU_Cr[index + 1] - 128) * 183)
                                          - ((MCU_Cb[index + 1] - 128) * 88)) >> 8),
                                        ((
                                          - ((MCU_Cr[index + 0] - 128) * 183)
                                          - ((MCU_Cb[index + 0] - 128) * 88)) >> 8));
                        printf("(-(mult + mul))>>8_t  vaut 0x%016"PRIx64"\n", temp);

                        __asm__("paddsw %mm0, %mm4");


#ifdef DEBUG
                        __asm__("movq %%mm0, %0":"=m"(mm0));
                        mm0_theorique = to_mmx_register(
                                        (MCU_Y[index + 3]),
                                        (MCU_Y[index + 2]),
                                        (MCU_Y[index + 1]),
                                        (MCU_Y[index + 0]));
                        assert(mm0 == mm0_theorique);

                        __asm__("movq %%mm4, %0":"=m"(add_after));
                        result_theorique = to_mmx_register(
                                        ((((MCU_Y[index + 3]) << 8)
                                          - ((MCU_Cr[index + 3] - 128) * 183)
                                          - ((MCU_Cb[index + 3] - 128) * 88)) >> 8),
                                        ((((MCU_Y[index + 2]) << 8)
                                          - ((MCU_Cr[index + 2] - 128) * 183)
                                          - ((MCU_Cb[index + 2] - 128) * 88)) >> 8),
                                        ((((MCU_Y[index + 1]) << 8)
                                          - ((MCU_Cr[index + 1] - 128) * 183)
                                          - ((MCU_Cb[index + 1] - 128) * 88)) >> 8),
                                        ((((MCU_Y[index + 0]) << 8)
                                          - ((MCU_Cr[index + 0] - 128) * 183)
                                          - ((MCU_Cb[index + 0] - 128) * 88)) >> 8));
                        temp = to_mmx_register(
                                        ((MCU_Y[index + 3]) + ((
                                                        - ((MCU_Cr[index + 3] - 128) * 183)
                                                        - ((MCU_Cb[index + 3] - 128) * 88)) >> 8)),
                                        ((MCU_Y[index + 2]) + ((
                                                        - ((MCU_Cr[index + 2] - 128) * 183)
                                                        - ((MCU_Cb[index + 2] - 128) * 88)) >> 8)),
                                        ((MCU_Y[index + 1]) + ((
                                                        - ((MCU_Cr[index + 1] - 128) * 183)
                                                        - ((MCU_Cb[index + 1] - 128) * 88)) >> 8)),
                                        ((MCU_Y[index + 0]) + ((
                                                        - ((MCU_Cr[index + 0] - 128) * 183)
                                                        - ((MCU_Cb[index + 0] - 128) * 88)) >> 8))
                                        );

                        if (add_after != result_theorique) {
                                printf("addt 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after_theorique,
                                                mm0_theorique,
                                                result_theorique);
                                printf("addn 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after_theorique,
                                                mm0_theorique,
                                                temp);
                                printf("addc 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after,
                                                mm0,
                                                add_after);
                                printf("\n");
                                assert(0);
                        }
#endif


                        printf("Calcul de R\n");
                        /*
                         * * * * * * * *
                         * Calcul de R *
                         * * * * * * * *
                         *
                         * R = (((Y << 8) + (Cr - 128) * 359)) >> 8
                         * On dévelloppe le >> 8, car ici c'est autorisé
                         * R = (MM0)  +   ((MM1) * 359) >> 8
                         *
                         */

                        /*
                         * multiplication
                         */
                        __asm__("movq %mm1, %mm3");
#ifdef DEBUG
                        __asm__("movq %%mm3, %0":"=m"(mm1));
                        mm1_theorique = to_mmx_register(
                                        MCU_Cr[index + 3] - 128,
                                        MCU_Cr[index + 2] - 128,
                                        MCU_Cr[index + 1] - 128,
                                        MCU_Cr[index + 0] - 128);
                        assert(mm1 == mm1_theorique);
#endif
                        __asm__("movq %mm3, %mm6");
                        __asm__("pmullw %0, %%mm3"::"m"(R_mult_constant));
                        // la partie haute vaut soit 0000 soit ffff, pour dire le signe
                        // l'information dans la partie basse n'est pas suffisante
                        // exemple tiré du traitement d'une image
                        // 	High mult vaut 0x0000000000000000
                        // 	low mult vaut  0x91d891d88c3c8c3c
                        // En regardant uniquement la partie basse,
                        // on pourrait croire que c'est un nombre négatif mais il est positif
                        __asm__("pmulhw %0, %%mm6"::"m"(R_mult_constant));

                        __asm__("movq %%mm6, %0":"=m"(temp));
                        printf("High mult vaut 0x%016"PRIx64"\t", temp);
                        __asm__("movq %%mm3, %0":"=m"(temp));
                        printf("low mult vaut  0x%016"PRIx64"\n", temp);
#ifdef DEBUG
                        __asm__("movq %%mm3, %0":"=m"(mult_after));
                        mult_after_theorique = to_mmx_register(
                                        (MCU_Cr[index + 3] - 128) * 359,
                                        (MCU_Cr[index + 2] - 128) * 359,
                                        (MCU_Cr[index + 1] - 128) * 359,
                                        (MCU_Cr[index + 0] - 128) * 359);
                        assert(mult_after == mult_after_theorique);
#endif
                        // on va passer sur 4 entiers de 32 bits
                        __asm__("movq %mm3, %mm7");
                        __asm__("punpcklwd %mm6, %mm3");
                        __asm__("punpckhwd %mm6, %mm7");
                        // on fait le décalage arithmétique à droite de 8
                        __asm__("psrad $8, %mm3");
                        __asm__("psrad $8, %mm7");
                        // on va repasser sur 16 bits
                        __asm__("packssdw %mm7, %mm3");

                        __asm__("movq %%mm3, %0":"=m"(temp));
                        printf("mult >> 8 vaut 0x%016"PRIx64"\n", temp);


                        // addition
#ifdef DEBUG
                        __asm__("movq %%mm0, %0":"=m"(mm0));
                        mm0_theorique = to_mmx_register(
                                        (MCU_Y[index + 3]),
                                        (MCU_Y[index + 2]),
                                        (MCU_Y[index + 1]),
                                        (MCU_Y[index + 0]));
                        assert(mm0 == mm0_theorique);
#endif
                        __asm__("paddw %mm0, %mm3");
#ifdef DEBUG
                        __asm__("movq %%mm3, %0":"=m"(add_after));
                        result_theorique = to_mmx_register(
                                        ((((MCU_Y[index + 3]) << 8) + ((MCU_Cr[index + 3] - 128) * 359)) >> 8),
                                        ((((MCU_Y[index + 2]) << 8) + ((MCU_Cr[index + 2] - 128) * 359)) >> 8),
                                        ((((MCU_Y[index + 1]) << 8) + ((MCU_Cr[index + 1] - 128) * 359)) >> 8),
                                        ((((MCU_Y[index + 0]) << 8) + ((MCU_Cr[index + 0] - 128) * 359)) >> 8));
                        if (add_after != result_theorique) {
                                printf("addt 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after_theorique,
                                                mm0_theorique,
                                                result_theorique);
                                printf("addm 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after,
                                                mm0,
                                                add_after);
                                printf("\n");
                                assert(0);
                        }
#endif


                        printf("Calcul de B\n");
                        /*
                         * * * * * * * *
                         * Calcul de B *
                         * * * * * * * *
                         *
                         * R = (((Y << 8) + (Cb - 128) * 455)) >> 8
                         * On dévelloppe le >> 8, car ici c'est autorisé
                         * R = (MM0)  +   ((MM1) * 455) >> 8
                         *
                         */

                        /*
                         * multiplication
                         */
                        __asm__("movq %mm1, %mm5");
#ifdef DEBUG
                        __asm__("movq %%mm5, %0":"=m"(mm1));
                        mm1_theorique = to_mmx_register(
                                        MCU_Cr[index + 3] - 128,
                                        MCU_Cr[index + 2] - 128,
                                        MCU_Cr[index + 1] - 128,
                                        MCU_Cr[index + 0] - 128);
                        assert(mm1 == mm1_theorique);
#endif
                        __asm__("movq %mm5, %mm6");
                        __asm__("pmullw %0, %%mm5"::"m"(B_cb_mult_constant));
                        __asm__("pmulhw %0, %%mm6"::"m"(B_cb_mult_constant));

                        __asm__("movq %%mm6, %0":"=m"(temp));
                        printf("High mult vaut 0x%016"PRIx64"\t", temp);
                        __asm__("movq %%mm5, %0":"=m"(temp));
                        printf("low mult vaut  0x%016"PRIx64"\n", temp);
#ifdef DEBUG
                        __asm__("movq %%mm5, %0":"=m"(mult_after));
                        mult_after_theorique = to_mmx_register(
                                        (MCU_Cr[index + 3] - 128) * 455,
                                        (MCU_Cr[index + 2] - 128) * 455,
                                        (MCU_Cr[index + 1] - 128) * 455,
                                        (MCU_Cr[index + 0] - 128) * 455);
                        assert(mult_after == mult_after_theorique);
#endif
                        // on va passer sur 4 entiers de 32 bits
                        __asm__("movq %mm5, %mm7");
                        __asm__("punpcklwd %mm6, %mm5");
                        __asm__("punpckhwd %mm6, %mm7");
                        // on fait le décalage arithmétique à droite de 8
                        __asm__("psrad $8, %mm5");
                        __asm__("psrad $8, %mm7");
                        // on va repasser sur 16 bits
                        __asm__("packssdw %mm7, %mm5");

                        __asm__("movq %%mm5, %0":"=m"(temp));
                        printf("mult >> 8 vaut 0x%016"PRIx64"\n", temp);


                        // addition
#ifdef DEBUG
                        __asm__("movq %%mm0, %0":"=m"(mm0));
                        mm0_theorique = to_mmx_register(
                                        (MCU_Y[index + 3]),
                                        (MCU_Y[index + 2]),
                                        (MCU_Y[index + 1]),
                                        (MCU_Y[index + 0]));
                        assert(mm0 == mm0_theorique);
#endif
                        __asm__("paddw %mm0, %mm5");
#ifdef DEBUG
                        __asm__("movq %%mm5, %0":"=m"(add_after));
                        result_theorique = to_mmx_register(
                                        ((((MCU_Y[index + 3]) << 8) + ((MCU_Cr[index + 3] - 128) * 455)) >> 8),
                                        ((((MCU_Y[index + 2]) << 8) + ((MCU_Cr[index + 2] - 128) * 455)) >> 8),
                                        ((((MCU_Y[index + 1]) << 8) + ((MCU_Cr[index + 1] - 128) * 455)) >> 8),
                                        ((((MCU_Y[index + 0]) << 8) + ((MCU_Cr[index + 0] - 128) * 455)) >> 8));
                        if (add_after != result_theorique) {
                                printf("addt 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after_theorique,
                                                mm0_theorique,
                                                result_theorique);
                                printf("addm 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after,
                                                mm0,
                                                add_after);
                                printf("\n");
                                assert(0);
                        }
#endif




                }
        }
}
#endif
#if VERSION == 7


void YCrCb_to_ARGB(uint8_t *YCrCb_MCU[3], uint32_t *RGB_MCU, uint32_t nb_MCU_H, uint32_t nb_MCU_V)
{
        uint8_t *MCU_Y, *MCU_Cr, *MCU_Cb;
        uint8_t index, i, j, idx;
        int32_t R[4], G[4], B[4];
        uint32_t ARGB[4];
        // Soustraction de 128 sur chacun des 16bits d'un registre mmx
        const uint64_t magic_substract = 0x0080008000800080;
        // Constante de multiplication
        //
        // R : 359 donc 0x0167 0167 0167 0167
        const uint64_t R_mult_constant = 0x0167016701670167;
        // G(Cr) : 183 donc 0x00b7 00b7 00b7 00b7
        const uint64_t G_cr_mult_constant = 0x00b700b700b700b7;
        // G(Cb) : 88 donc 0x0058 0058 0058 0058
        const uint64_t G_cb_mult_constant = 0x0058005800580058;
        // B(Cb) : 455 donc 0x01c7 01c7 01c7 01c7
        const uint64_t B_cb_mult_constant = 0x01c701c701c701c7;



        uint64_t temp;
        uint64_t mm1;
        uint64_t mm1_theorique;
        uint64_t mult_after;
        uint64_t mult_after_theorique;
        uint64_t mm0;
        uint64_t mm0_theorique;
        uint64_t add_after;
        uint64_t result_theorique;

#ifdef VERSION
        printf("Version %d", VERSION);
#endif
#ifdef DEBUG
        printf(" (debug)\n");
#else
        printf("\n");
#endif

        /* Vide la pile des instructions flottantes */
        __asm__("emms");

        MCU_Y = YCrCb_MCU[0];
        MCU_Cr = YCrCb_MCU[2];
        MCU_Cb = YCrCb_MCU[1];

        for (i = 0; i < 8 * nb_MCU_V; i++) {
                for (j = 0; j < 8 * nb_MCU_H; j += 4) {
                        index = i * (8 * nb_MCU_H) + j;
                        // calcul de référence
                        for (idx = 0; idx < 4; idx++) {
                                R[idx] = ((MCU_Y[index + idx] << 8) +
                                                (MCU_Cr[index + idx] - 128) * 359) >> 8;
                                G[idx] = ((MCU_Y[index + idx] << 8) -
                                                (MCU_Cr[index + idx] - 128) * 183 -
                                                (MCU_Cb[index + idx] - 128) * 88) >> 8;
                                B[idx] = ((MCU_Y[index + idx] << 8) +
                                                (MCU_Cb[index + idx] - 128) * 455) >> 8;
                                if (R[idx] > 255)
                                        R[idx] = 255;
                                if (G[idx] > 255)
                                        G[idx] = 255;
                                if (B[idx] > 255)
                                        B[idx] = 255;
                                if (R[idx] < 0)
                                        R[idx] = 0;
                                if (G[idx] < 0)
                                        G[idx] = 0;
                                if (B[idx] < 0)
                                        B[idx] = 0;
                                ARGB[idx] = ((R[idx] & 0xff) << 16) |
                                        ((G[idx] & 0xff) << 8) |
                                        (B[idx] & 0xff);
                                RGB_MCU[index + idx] = ARGB[idx];
                        }

                        // chargement de Y dans mm0
                        // 0x00 | MCU_Y[index + 3] | 0x00 | MCU_Y[index + 2] |
                        // 0x00 | MCU_Y[index + 1] | 0x00 | MCU_Y[index + 0] -> %mm0
                        //
                        __asm__("pxor %mm7, %mm7");
                        __asm__("movq %0, %%mm0"::"m"(MCU_Y[index]));
                        __asm__("punpcklbw %mm7, %mm0");
#ifdef DEBUG
                        uint16_t Y[4];
                        __asm__("movq %%mm0, %0":"=m"(Y[0]));
                        assert(MCU_Y[index]     == Y[0]);
                        assert(MCU_Y[index + 1] == Y[1]);
                        assert(MCU_Y[index + 2] == Y[2]);
                        assert(MCU_Y[index + 3] == Y[3]);
#endif

                        // chargement de CR -> mm1
                        __asm__("pxor %mm7, %mm7");
                        __asm__("movq %0, %%mm1"::"m"(MCU_Cr[index]));
                        __asm__("punpcklbw %mm7, %mm1");
#ifdef DEBUG
                        // test du chargement de Cr dans mm1
                        uint16_t Cr[4];
                        __asm__("movq %%mm1, %0":"=m"(Cr[0]));
                        assert(MCU_Cr[index] == Cr[0]);
                        assert(MCU_Cr[index + 1] == Cr[1]);
                        assert(MCU_Cr[index + 2] == Cr[2]);
                        assert(MCU_Cr[index + 3] == Cr[3]);
#endif

                        // soustraction CR -128
                        __asm__("psubw %0, %%mm1"::"m"(magic_substract));
#ifdef DEBUG
                        // test après soustraction
                        __asm__("movq %%mm1, %0":"=m"(Cr[0]));
                        assert((uint16_t) (MCU_Cr[index] - 128) == Cr[0]);
                        assert((uint16_t) (MCU_Cr[index + 1] - 128) == Cr[1]);
                        assert((uint16_t) (MCU_Cr[index + 2] - 128) == Cr[2]);
                        assert((uint16_t) (MCU_Cr[index + 3] - 128) == Cr[3]);
#endif

                        // chargement CB -> mm2
                        __asm__("movq %0, %%mm2"::"m"(MCU_Cb[index]));
                        __asm__("punpcklbw %mm7, %mm2");
#ifdef DEBUG
                        // test du chargement de Cb dans mm2
                        uint16_t Cb[4];
                        __asm__("movq %%mm2, %0":"=m"(Cb[0]));
                        assert(MCU_Cb[index] == Cb[0]);
                        assert(MCU_Cb[index + 1] == Cb[1]);
                        assert(MCU_Cb[index + 2] == Cb[2]);
                        assert(MCU_Cb[index + 3] == Cb[3]);
#endif

                        // soustraction CB - 128
                        __asm__("psubw %0, %%mm2"::"m"(magic_substract));
#ifdef DEBUG
                        // test après soustraction
                        __asm__("movq %%mm2, %0":"=m"(Cb[0]));
                        assert((uint16_t) (MCU_Cb[index] - 128)== Cb[0]);
                        assert((uint16_t) (MCU_Cb[index + 1] - 128) == Cb[1]);
                        assert((uint16_t) (MCU_Cb[index + 2] - 128) == Cb[2]);
                        assert((uint16_t) (MCU_Cb[index + 3] - 128) == Cb[3]);
#endif

                        /* on va faire dans l'ordre
                         * 	calcul de G
                         * 	calcul de R
                         * 	calcul de B
                         * 	On fait G en premier car on a besoin de plus de registres pour le calculer
                         */

                        printf("Calcul de G\n");
                        /*
                         * * * * * * * *
                         * Calcul de G *
                         * * * * * * * *
                         *
                         * G = ((Y << 8) - ((Cr - 128) * 183) - ((Cb - 128) * 88)) >> 8
                         *
                         * On dévelloppe le >> 8, car ici c'est autorisé
                         *              G = Y - ((MM1 * 183) + (MM2 * 88)) >> 8
                         *
                         */

                        /*
                         * multiplication Cr
                         */
                        __asm__("movq %mm1, %mm4");
#ifdef DEBUG
                        __asm__("movq %%mm4, %0":"=m"(mm1));
                        mm1_theorique = to_mmx_register(
                                        MCU_Cr[index + 3] - 128,
                                        MCU_Cr[index + 2] - 128,
                                        MCU_Cr[index + 1] - 128,
                                        MCU_Cr[index + 0] - 128);
                        assert(mm1 == mm1_theorique);
#endif
                        __asm__("movq %mm4, %mm7");

                        __asm__("pmullw %0, %%mm4"::"m"(G_cr_mult_constant));
                        // la partie haute vaut soit 0000 soit ffff, pour dire le signe
                        __asm__("pmulhw %0, %%mm7"::"m"(G_cr_mult_constant));


                        __asm__("movq %%mm7, %0":"=m"(temp));
                        printf("High mult vaut 0x%016"PRIx64"\t", temp);
                        __asm__("movq %%mm4, %0":"=m"(temp));
                        printf("low mult vaut  0x%016"PRIx64"\n", temp);
#ifdef DEBUG
                        __asm__("movq %%mm4, %0":"=m"(mult_after));
                        mult_after_theorique = to_mmx_register(
                                        (MCU_Cr[index + 3] - 128) * 183,
                                        (MCU_Cr[index + 2] - 128) * 183,
                                        (MCU_Cr[index + 1] - 128) * 183,
                                        (MCU_Cr[index + 0] - 128) * 183);
                        assert(mult_after == mult_after_theorique);
                        printf("Mutl_the vaut  0x%016"PRIx64"\n", mult_after_theorique);
#endif
                        // on va passer sur 4 entiers de 32 bits
                        __asm__("movq %mm4, %mm5");
                        __asm__("punpcklwd %mm7, %mm4");
                        __asm__("punpckhwd %mm7, %mm5");


                        /*
                         * multiplication Cb
                         */
                        __asm__("movq %mm2, %mm6");
#ifdef DEBUG
                        __asm__("movq %%mm6, %0":"=m"(mm1));
                        mm1_theorique = to_mmx_register(
                                        MCU_Cb[index + 3] - 128,
                                        MCU_Cb[index + 2] - 128,
                                        MCU_Cb[index + 1] - 128,
                                        MCU_Cb[index + 0] - 128);
                        assert(mm1 == mm1_theorique);
#endif
                        __asm__("movq %mm6, %mm7");
                        __asm__("pmullw %0, %%mm6"::"m"(G_cb_mult_constant));
                        // la partie haute vaut soit 0000 soit ffff, pour dire le signe
                        __asm__("pmulhw %0, %%mm7"::"m"(G_cb_mult_constant));


                        __asm__("movq %%mm7, %0":"=m"(temp));
                        printf("High mult vaut 0x%016"PRIx64"\t", temp);
                        __asm__("movq %%mm6, %0":"=m"(temp));
                        printf("low mult vaut  0x%016"PRIx64"\n", temp);
#ifdef DEBUG
                        __asm__("movq %%mm6, %0":"=m"(mult_after));
                        mult_after_theorique = to_mmx_register(
                                        (MCU_Cb[index + 3] - 128) * 88,
                                        (MCU_Cb[index + 2] - 128) * 88,
                                        (MCU_Cb[index + 1] - 128) * 88,
                                        (MCU_Cb[index + 0] - 128) * 88);
                        assert(mult_after == mult_after_theorique);
                        printf("Mutl_the vaut  0x%016"PRIx64"\n", mult_after_theorique);
#endif
                        // on va passer sur 4 entiers de 32 bits
                        // On va utiliser mm3 comme temporaire
                        __asm__("movq %mm6, %mm3");
                        __asm__("punpcklwd %mm7, %mm6");
                        __asm__("punpckhwd %mm7, %mm3");

                        __asm__("movq %mm3, %mm7");


                        printf("\n");
                        // on va additionner
                        __asm__("paddd %mm4, %mm6");
                        __asm__("paddd %mm5, %mm7");

#ifdef DEBUG
                        __asm__("movq %%mm6, %0":"=m"(temp));
                        printf("Sum == 0x%016"PRIx64"\n", temp);
                        __asm__("movq %%mm7, %0":"=m"(temp));
                        printf("Sum == 0x%016"PRIx64"\n", temp);
                        add_after = to_mmx_register(
                                        + ((MCU_Cr[index + 3] - 128) * 183)
                                        + ((MCU_Cb[index + 3] - 128) * 88),
                                        + ((MCU_Cr[index + 2] - 128) * 183)
                                        + ((MCU_Cb[index + 2] - 128) * 88),
                                        + ((MCU_Cr[index + 1] - 128) * 183)
                                        + ((MCU_Cb[index + 1] - 128) * 88),
                                        + ((MCU_Cr[index + 0] - 128) * 183)
                                        + ((MCU_Cb[index + 0] - 128) * 88));
                        printf("Sumt = 0x%016"PRIx64"\n", add_after);
#endif

                        // passage en négatif
                        __asm__("pxor %mm4, %mm4");
                        __asm__("pxor %mm5, %mm5");
                        __asm__("psubd %mm6, %mm4");
                        __asm__("psubd %mm7, %mm5");


#ifdef DEBUG
                        // Print de la somme des nombres négatifs
                        __asm__("movq %%mm4, %0":"=m"(temp));
                        printf("Sub == 0x%016"PRIx64"\n", temp);
                        __asm__("movq %%mm5, %0":"=m"(temp));
                        printf("Sub == 0x%016"PRIx64"\n", temp);
                        add_after = to_mmx_register(
                                        - ((MCU_Cr[index + 3] - 128) * 183)
                                        - ((MCU_Cb[index + 3] - 128) * 88),
                                        - ((MCU_Cr[index + 2] - 128) * 183)
                                        - ((MCU_Cb[index + 2] - 128) * 88),
                                        - ((MCU_Cr[index + 1] - 128) * 183)
                                        - ((MCU_Cb[index + 1] - 128) * 88),
                                        - ((MCU_Cr[index + 0] - 128) * 183)
                                        - ((MCU_Cb[index + 0] - 128) * 88));
                        printf("Subt = 0x%016"PRIx64"\n", add_after);
#endif
                        // on fait le décalage arithmétique à droite de 8
                        __asm__("psrad $8, %mm4");
                        __asm__("psrad $8, %mm5");

#ifdef DEBUG
                        // print après décalage
                        __asm__("movq %%mm4, %0":"=m"(temp));
                        printf("Dec == 0x%016"PRIx64"\n", temp);
                        __asm__("movq %%mm5, %0":"=m"(temp));
                        printf("Dec == 0x%016"PRIx64"\n", temp);
#endif

                        // on va repasser sur 16 bits
                        __asm__("packssdw %mm5, %mm4");

                        __asm__("movq %%mm4, %0":"=m"(temp));
                        printf("(- (mult + mul)) >> 8 vaut 0x%016"PRIx64"\n", temp);
                        temp = to_mmx_register(
                                        ((
                                          - ((MCU_Cr[index + 3] - 128) * 183)
                                          - ((MCU_Cb[index + 3] - 128) * 88)) >> 8),
                                        ((
                                          - ((MCU_Cr[index + 2] - 128) * 183)
                                          - ((MCU_Cb[index + 2] - 128) * 88)) >> 8),
                                        ((
                                          - ((MCU_Cr[index + 1] - 128) * 183)
                                          - ((MCU_Cb[index + 1] - 128) * 88)) >> 8),
                                        ((
                                          - ((MCU_Cr[index + 0] - 128) * 183)
                                          - ((MCU_Cb[index + 0] - 128) * 88)) >> 8));
                        printf("(-(mult + mul))>>8_t  vaut 0x%016"PRIx64"\n", temp);

                        __asm__("paddsw %mm0, %mm4");


#ifdef DEBUG
                        __asm__("movq %%mm0, %0":"=m"(mm0));
                        mm0_theorique = to_mmx_register(
                                        (MCU_Y[index + 3]),
                                        (MCU_Y[index + 2]),
                                        (MCU_Y[index + 1]),
                                        (MCU_Y[index + 0]));
                        assert(mm0 == mm0_theorique);

                        __asm__("movq %%mm4, %0":"=m"(add_after));
                        result_theorique = to_mmx_register(
                                        ((((MCU_Y[index + 3]) << 8)
                                          - ((MCU_Cr[index + 3] - 128) * 183)
                                          - ((MCU_Cb[index + 3] - 128) * 88)) >> 8),
                                        ((((MCU_Y[index + 2]) << 8)
                                          - ((MCU_Cr[index + 2] - 128) * 183)
                                          - ((MCU_Cb[index + 2] - 128) * 88)) >> 8),
                                        ((((MCU_Y[index + 1]) << 8)
                                          - ((MCU_Cr[index + 1] - 128) * 183)
                                          - ((MCU_Cb[index + 1] - 128) * 88)) >> 8),
                                        ((((MCU_Y[index + 0]) << 8)
                                          - ((MCU_Cr[index + 0] - 128) * 183)
                                          - ((MCU_Cb[index + 0] - 128) * 88)) >> 8));
                        temp = to_mmx_register(
                                        ((MCU_Y[index + 3]) + ((
                                                        - ((MCU_Cr[index + 3] - 128) * 183)
                                                        - ((MCU_Cb[index + 3] - 128) * 88)) >> 8)),
                                        ((MCU_Y[index + 2]) + ((
                                                        - ((MCU_Cr[index + 2] - 128) * 183)
                                                        - ((MCU_Cb[index + 2] - 128) * 88)) >> 8)),
                                        ((MCU_Y[index + 1]) + ((
                                                        - ((MCU_Cr[index + 1] - 128) * 183)
                                                        - ((MCU_Cb[index + 1] - 128) * 88)) >> 8)),
                                        ((MCU_Y[index + 0]) + ((
                                                        - ((MCU_Cr[index + 0] - 128) * 183)
                                                        - ((MCU_Cb[index + 0] - 128) * 88)) >> 8))
                                        );

                        if (add_after != result_theorique) {
                                printf("addt 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after_theorique,
                                                mm0_theorique,
                                                result_theorique);
                                printf("addn 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after_theorique,
                                                mm0_theorique,
                                                temp);
                                printf("addc 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after,
                                                mm0,
                                                add_after);
                                printf("\n");
                                assert(0);
                        }
#endif


                        printf("Calcul de R\n");
                        /*
                         * * * * * * * *
                         * Calcul de R *
                         * * * * * * * *
                         *
                         * R = (((Y << 8) + (Cr - 128) * 359)) >> 8
                         * On dévelloppe le >> 8, car ici c'est autorisé
                         * R = (MM0)  +   ((MM1) * 359) >> 8
                         *
                         */

                        /*
                         * multiplication
                         */
                        __asm__("movq %mm1, %mm3");
#ifdef DEBUG
                        __asm__("movq %%mm3, %0":"=m"(mm1));
                        mm1_theorique = to_mmx_register(
                                        MCU_Cr[index + 3] - 128,
                                        MCU_Cr[index + 2] - 128,
                                        MCU_Cr[index + 1] - 128,
                                        MCU_Cr[index + 0] - 128);
                        assert(mm1 == mm1_theorique);
#endif
                        __asm__("movq %mm3, %mm6");
                        __asm__("pmullw %0, %%mm3"::"m"(R_mult_constant));
                        // la partie haute vaut soit 0000 soit ffff, pour dire le signe
                        // l'information dans la partie basse n'est pas suffisante
                        // exemple tiré du traitement d'une image
                        // 	High mult vaut 0x0000000000000000
                        // 	low mult vaut  0x91d891d88c3c8c3c
                        // En regardant uniquement la partie basse,
                        // on pourrait croire que c'est un nombre négatif mais il est positif
                        __asm__("pmulhw %0, %%mm6"::"m"(R_mult_constant));

                        __asm__("movq %%mm6, %0":"=m"(temp));
                        printf("High mult vaut 0x%016"PRIx64"\t", temp);
                        __asm__("movq %%mm3, %0":"=m"(temp));
                        printf("low mult vaut  0x%016"PRIx64"\n", temp);
#ifdef DEBUG
                        __asm__("movq %%mm3, %0":"=m"(mult_after));
                        mult_after_theorique = to_mmx_register(
                                        (MCU_Cr[index + 3] - 128) * 359,
                                        (MCU_Cr[index + 2] - 128) * 359,
                                        (MCU_Cr[index + 1] - 128) * 359,
                                        (MCU_Cr[index + 0] - 128) * 359);
                        assert(mult_after == mult_after_theorique);
#endif
                        // on va passer sur 4 entiers de 32 bits
                        __asm__("movq %mm3, %mm7");
                        __asm__("punpcklwd %mm6, %mm3");
                        __asm__("punpckhwd %mm6, %mm7");
                        // on fait le décalage arithmétique à droite de 8
                        __asm__("psrad $8, %mm3");
                        __asm__("psrad $8, %mm7");
                        // on va repasser sur 16 bits
                        __asm__("packssdw %mm7, %mm3");

                        __asm__("movq %%mm3, %0":"=m"(temp));
                        printf("mult >> 8 vaut 0x%016"PRIx64"\n", temp);


                        // addition
#ifdef DEBUG
                        __asm__("movq %%mm0, %0":"=m"(mm0));
                        mm0_theorique = to_mmx_register(
                                        (MCU_Y[index + 3]),
                                        (MCU_Y[index + 2]),
                                        (MCU_Y[index + 1]),
                                        (MCU_Y[index + 0]));
                        assert(mm0 == mm0_theorique);
#endif
                        __asm__("paddw %mm0, %mm3");
#ifdef DEBUG
                        __asm__("movq %%mm3, %0":"=m"(add_after));
                        result_theorique = to_mmx_register(
                                        ((((MCU_Y[index + 3]) << 8) + ((MCU_Cr[index + 3] - 128) * 359)) >> 8),
                                        ((((MCU_Y[index + 2]) << 8) + ((MCU_Cr[index + 2] - 128) * 359)) >> 8),
                                        ((((MCU_Y[index + 1]) << 8) + ((MCU_Cr[index + 1] - 128) * 359)) >> 8),
                                        ((((MCU_Y[index + 0]) << 8) + ((MCU_Cr[index + 0] - 128) * 359)) >> 8));
                        if (add_after != result_theorique) {
                                printf("addt 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after_theorique,
                                                mm0_theorique,
                                                result_theorique);
                                printf("addm 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after,
                                                mm0,
                                                add_after);
                                printf("\n");
                                assert(0);
                        }
#endif


                        printf("Calcul de B\n");
                        /*
                         * * * * * * * *
                         * Calcul de B *
                         * * * * * * * *
                         *
                         * R = (((Y << 8) + (Cb - 128) * 455)) >> 8
                         * On dévelloppe le >> 8, car ici c'est autorisé
                         * R = (MM0)  +   ((MM1) * 455) >> 8
                         *
                         */

                        /*
                         * multiplication
                         */
                        __asm__("movq %mm2, %mm5");
#ifdef DEBUG
                        __asm__("movq %%mm5, %0":"=m"(mm1));
                        mm1_theorique = to_mmx_register(
                                        MCU_Cb[index + 3] - 128,
                                        MCU_Cb[index + 2] - 128,
                                        MCU_Cb[index + 1] - 128,
                                        MCU_Cb[index + 0] - 128);
                        assert(mm1 == mm1_theorique);
#endif
                        __asm__("movq %mm5, %mm6");
                        __asm__("pmullw %0, %%mm5"::"m"(B_cb_mult_constant));
                        __asm__("pmulhw %0, %%mm6"::"m"(B_cb_mult_constant));

                        __asm__("movq %%mm6, %0":"=m"(temp));
                        printf("High mult vaut 0x%016"PRIx64"\t", temp);
                        __asm__("movq %%mm5, %0":"=m"(temp));
                        printf("low mult vaut  0x%016"PRIx64"\n", temp);
#ifdef DEBUG
                        __asm__("movq %%mm5, %0":"=m"(mult_after));
                        mult_after_theorique = to_mmx_register(
                                        (MCU_Cb[index + 3] - 128) * 455,
                                        (MCU_Cb[index + 2] - 128) * 455,
                                        (MCU_Cb[index + 1] - 128) * 455,
                                        (MCU_Cb[index + 0] - 128) * 455);
                        assert(mult_after == mult_after_theorique);
#endif
                        // on va passer sur 4 entiers de 32 bits
                        __asm__("movq %mm5, %mm7");
                        __asm__("punpcklwd %mm6, %mm5");
                        __asm__("punpckhwd %mm6, %mm7");
                        // on fait le décalage arithmétique à droite de 8
                        __asm__("psrad $8, %mm5");
                        __asm__("psrad $8, %mm7");
                        // on va repasser sur 16 bits
                        __asm__("packssdw %mm7, %mm5");

                        __asm__("movq %%mm5, %0":"=m"(temp));
                        printf("mult >> 8 vaut 0x%016"PRIx64"\n", temp);


                        // addition
#ifdef DEBUG
                        __asm__("movq %%mm0, %0":"=m"(mm0));
                        mm0_theorique = to_mmx_register(
                                        (MCU_Y[index + 3]),
                                        (MCU_Y[index + 2]),
                                        (MCU_Y[index + 1]),
                                        (MCU_Y[index + 0]));
                        assert(mm0 == mm0_theorique);
#endif
                        __asm__("paddw %mm0, %mm5");
#ifdef DEBUG
                        __asm__("movq %%mm5, %0":"=m"(add_after));
                        result_theorique = to_mmx_register(
                                        ((((MCU_Y[index + 3]) << 8) + ((MCU_Cb[index + 3] - 128) * 455)) >> 8),
                                        ((((MCU_Y[index + 2]) << 8) + ((MCU_Cb[index + 2] - 128) * 455)) >> 8),
                                        ((((MCU_Y[index + 1]) << 8) + ((MCU_Cb[index + 1] - 128) * 455)) >> 8),
                                        ((((MCU_Y[index + 0]) << 8) + ((MCU_Cb[index + 0] - 128) * 455)) >> 8));
                        if (add_after != result_theorique) {
                                printf("addt 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after_theorique,
                                                mm0_theorique,
                                                result_theorique);
                                printf("addm 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                mult_after,
                                                mm0,
                                                add_after);
                                printf("\n");
                                assert(0);
                        }
#endif




                        // Récupération des valeurs pour les mettres dans ARGB
#ifdef DEBUG
                        uint64_t mm3_before;
                        __asm__("movq %%mm3, %0":"=m"(mm3_before));
                        uint64_t mm4_before;
                        __asm__("movq %%mm4, %0":"=m"(mm4_before));
                        uint64_t mm5_before;
                        __asm__("movq %%mm5, %0":"=m"(mm5_before));
#endif
                        __asm__("packuswb %mm7, %mm3");
                        __asm__("packuswb %mm7, %mm4");
                        __asm__("packuswb %mm7, %mm5");
#ifdef DEBUG
                        uint64_t mm3_after;
                        __asm__("movq %%mm3, %0":"=m"(mm3_after));
                        uint64_t mm4_after;
                        __asm__("movq %%mm4, %0":"=m"(mm4_after));
                        uint64_t mm5_after;
                        __asm__("movq %%mm5, %0":"=m"(mm5_after));
#endif
                        uint8_t R_mmx[4] = { 0 };
                        uint8_t G_mmx[4] = { 0 };
                        uint8_t B_mmx[4] = { 0 };

                        __asm__("movd %%mm3, %0":"=m"(R_mmx[0]));
                        __asm__("movd %%mm4, %0":"=m"(G_mmx[0]));
                        __asm__("movd %%mm5, %0":"=m"(B_mmx[0]));

#ifdef DEBUG
                        int tmp = 0;
                        for (tmp = 0; tmp < 4; tmp++) {
                                if (R[tmp] != R_mmx[tmp] ||
                                                G[tmp] != G_mmx[tmp] ||
                                                B[tmp] != B_mmx[tmp]) {
#if 0
                                        printf("t R[%d] 0x%04"PRIx8" ", tmp, R[tmp]);
                                        printf("G[%d] 0x%04"PRIx8" ", tmp, G[tmp]);
                                        printf("B[%d] 0x%04"PRIx8" \n", tmp, B[tmp]);
                                        printf("m R[%d] 0x%04"PRIx8" ", tmp, R_mmx[tmp]);
                                        printf("G[%d] 0x%04"PRIx8" ", tmp, G_mmx[tmp]);
                                        printf("B[%d] 0x%04"PRIx8" \n", tmp, B_mmx[tmp]);

                                        printf("Raddt 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                        R_mult_after_theorique,
                                                        mm0_theorique,
                                                        R_add_after_theorique);
                                        printf("Raddm 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                        R_mult_after,
                                                        mm0,
                                                        R_add_after);
                                        printf("Rt 0x%016"PRIx64" >> 8 = 0x%016"PRIx64"\n",
                                                        R_add_after_theorique,
                                                        R_shift_after_theorique);
                                        printf("Rm 0x%016"PRIx64" >> 8 = 0x%016"PRIx64"\n",
                                                        R_add_after,
                                                        R_shift_after);
                                        printf("Baddt 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                        B_mult_after_theorique,
                                                        mm0_theorique,
                                                        B_add_after_theorique);
                                        printf("Baddm 0x%016"PRIx64" + 0x%016"PRIx64" = 0x%016"PRIx64"\n",
                                                        B_mult_after,
                                                        mm0,
                                                        B_add_after);
                                        printf("Bt 0x%016"PRIx64" >> 8 = 0x%016"PRIx64"\n",
                                                        B_add_after_theorique,
                                                        B_shift_after_theorique);
                                        printf("Bm 0x%016"PRIx64" >> 8 = 0x%016"PRIx64"\n",
                                                        B_add_after,
                                                        B_shift_after);
                                        printf("mm3 : 0x%016"PRIx64" -> 0x%016"PRIx64"\n", mm3_before, mm3_after);
                                        printf("mm4 : 0x%016"PRIx64" -> 0x%016"PRIx64"\n", mm4_before, mm4_after);
                                        printf("mm5 : 0x%016"PRIx64" -> 0x%016"PRIx64"\n", mm5_before, mm5_after);

#endif
                                        printf("R_the[%d] = 0x%04"PRIx32"\n", tmp, R[tmp]);
                                        printf("R_mmx[%d] = 0x%04"PRIx32"\n", tmp, R_mmx[tmp]);
                                        printf("G_the[%d] = 0x%04"PRIx32"\n", tmp, G[tmp]);
                                        printf("G_mmx[%d] = 0x%04"PRIx32"\n", tmp, G_mmx[tmp]);
                                        printf("B_the[%d] = 0x%04"PRIx32"\n", tmp, B[tmp]);
                                        printf("B_mmx[%d] = 0x%04"PRIx32"\n", tmp, B_mmx[tmp]);
                                        printf("\n");
                                }
                                //assert(R[tmp] == R_mmx[tmp]);
                                //assert(G[tmp] == G_mmx[tmp]);
                                //assert(B[tmp] == B_mmx[tmp]);
                        }
#endif


                        for (idx = 0; idx < 4; idx++) {
                                ARGB[idx] = ((R_mmx[idx] & 0xff) << 16) |
                                        ((G_mmx[idx] & 0xff) << 8) |
                                        (B_mmx[idx] & 0xff);
                                RGB_MCU[index + idx] = ARGB[idx];
                        }
                }
        }
}
#endif


#if VERSION == 8


void YCrCb_to_ARGB(uint8_t *YCrCb_MCU[3], uint32_t *RGB_MCU, uint32_t nb_MCU_H, uint32_t nb_MCU_V)
{
        uint8_t *MCU_Y, *MCU_Cr, *MCU_Cb;
        uint8_t index, i, j, idx;
        uint32_t ARGB[4];
        uint8_t R_mmx[4] = { 0 };
        uint8_t G_mmx[4] = { 0 };
        uint8_t B_mmx[4] = { 0 };

        // Soustraction de 128 sur chacun des 16bits d'un registre mmx
        const uint64_t magic_substract = 0x0080008000800080;
        // Constante de multiplication
        //
        // R : 359 donc 0x0167 0167 0167 0167
        const uint64_t R_mult_constant = 0x0167016701670167;
        // G(Cr) : 183 donc 0x00b7 00b7 00b7 00b7
        const uint64_t G_cr_mult_constant = 0x00b700b700b700b7;
        // G(Cb) : 88 donc 0x0058 0058 0058 0058
        const uint64_t G_cb_mult_constant = 0x0058005800580058;
        // B(Cb) : 455 donc 0x01c7 01c7 01c7 01c7
        const uint64_t B_cb_mult_constant = 0x01c701c701c701c7;

        /* Vide la pile des instructions flottantes */
        __asm__("emms");

        MCU_Y = YCrCb_MCU[0];
        MCU_Cr = YCrCb_MCU[2];
        MCU_Cb = YCrCb_MCU[1];

        for (i = 0; i < 8 * nb_MCU_V; i++) {
                for (j = 0; j < 8 * nb_MCU_H; j += 4) {
                        index = i * (8 * nb_MCU_H) + j;

                        // chargement de Y dans mm0
                        // 0x00 | MCU_Y[index + 3] | 0x00 | MCU_Y[index + 2] |
                        // 0x00 | MCU_Y[index + 1] | 0x00 | MCU_Y[index + 0] -> %mm0
                        //
                        __asm__("pxor %mm7, %mm7");
                        __asm__("movq %0, %%mm0"::"m"(MCU_Y[index]));
                        __asm__("punpcklbw %mm7, %mm0");
                        // chargement de CR -> mm1
                        __asm__("pxor %mm7, %mm7");
                        __asm__("movq %0, %%mm1"::"m"(MCU_Cr[index]));
                        __asm__("punpcklbw %mm7, %mm1");
                        // soustraction CR -128
                        __asm__("psubw %0, %%mm1"::"m"(magic_substract));
                        // chargement CB -> mm2
                        __asm__("movq %0, %%mm2"::"m"(MCU_Cb[index]));
                        __asm__("punpcklbw %mm7, %mm2");
                        // soustraction CB - 128
                        __asm__("psubw %0, %%mm2"::"m"(magic_substract));
                        /* on va faire dans l'ordre
                         * 	calcul de G
                         * 	calcul de R
                         * 	calcul de B
                         * 	On fait G en premier car on a besoin de plus de registres pour le calculer
                         */
                        /*
                         * * * * * * * *
                         * Calcul de G *
                         * * * * * * * *
                         *
                         * G = ((Y << 8) - ((Cr - 128) * 183) - ((Cb - 128) * 88)) >> 8
                         *
                         * On dévelloppe le >> 8, car ici c'est autorisé
                         *              G = Y - ((MM1 * 183) + (MM2 * 88)) >> 8
                         *
                         */

                        /*
                         * multiplication Cr
                         */
                        __asm__("movq %mm1, %mm4");
                        __asm__("movq %mm4, %mm7");
                        __asm__("pmullw %0, %%mm4"::"m"(G_cr_mult_constant));
                        // la partie haute vaut soit 0000 soit ffff, pour dire le signe
                        __asm__("pmulhw %0, %%mm7"::"m"(G_cr_mult_constant));
                        // on va passer sur 4 entiers de 32 bits
                        __asm__("movq %mm4, %mm5");
                        __asm__("punpcklwd %mm7, %mm4");
                        __asm__("punpckhwd %mm7, %mm5");
                        /*
                         * multiplication Cb
                         */
                        __asm__("movq %mm2, %mm6");
                        __asm__("movq %mm6, %mm7");
                        __asm__("pmullw %0, %%mm6"::"m"(G_cb_mult_constant));
                        // la partie haute vaut soit 0000 soit ffff, pour dire le signe
                        __asm__("pmulhw %0, %%mm7"::"m"(G_cb_mult_constant));
                        // on va passer sur 4 entiers de 32 bits
                        // On va utiliser mm3 comme temporaire
                        __asm__("movq %mm6, %mm3");
                        __asm__("punpcklwd %mm7, %mm6");
                        __asm__("punpckhwd %mm7, %mm3");
                        __asm__("movq %mm3, %mm7");
                        // on va additionner
                        __asm__("paddd %mm4, %mm6");
                        __asm__("paddd %mm5, %mm7");
                        // passage en négatif
                        __asm__("pxor %mm4, %mm4");
                        __asm__("pxor %mm5, %mm5");
                        __asm__("psubd %mm6, %mm4");
                        __asm__("psubd %mm7, %mm5");
                        // on fait le décalage arithmétique à droite de 8
                        __asm__("psrad $8, %mm4");
                        __asm__("psrad $8, %mm5");
                        // on va repasser sur 16 bits
                        __asm__("packssdw %mm5, %mm4");

                        __asm__("paddsw %mm0, %mm4");

                        /*
                         * * * * * * * *
                         * Calcul de R *
                         * * * * * * * *
                         *
                         * R = (((Y << 8) + (Cr - 128) * 359)) >> 8
                         * On dévelloppe le >> 8, car ici c'est autorisé
                         * R = (MM0)  +   ((MM1) * 359) >> 8
                         *
                         */

                        /*
                         * multiplication
                         */
                        __asm__("movq %mm1, %mm3");
                        __asm__("movq %mm3, %mm6");
                        __asm__("pmullw %0, %%mm3"::"m"(R_mult_constant));
                        // la partie haute vaut soit 0000 soit ffff, pour dire le signe
                        // l'information dans la partie basse n'est pas suffisante
                        // exemple tiré du traitement d'une image
                        // 	High mult vaut 0x0000000000000000
                        // 	low mult vaut  0x91d891d88c3c8c3c
                        // En regardant uniquement la partie basse,
                        // on pourrait croire que c'est un nombre négatif mais il est positif
                        __asm__("pmulhw %0, %%mm6"::"m"(R_mult_constant));
                        // on va passer sur 4 entiers de 32 bits
                        __asm__("movq %mm3, %mm7");
                        __asm__("punpcklwd %mm6, %mm3");
                        __asm__("punpckhwd %mm6, %mm7");
                        // on fait le décalage arithmétique à droite de 8
                        __asm__("psrad $8, %mm3");
                        __asm__("psrad $8, %mm7");
                        // on va repasser sur 16 bits
                        __asm__("packssdw %mm7, %mm3");
                        // addition
                        __asm__("paddw %mm0, %mm3");

                        /*
                         * * * * * * * *
                         * Calcul de B *
                         * * * * * * * *
                         *
                         * R = (((Y << 8) + (Cb - 128) * 455)) >> 8
                         * On dévelloppe le >> 8, car ici c'est autorisé
                         * R = (MM0)  +   ((MM1) * 455) >> 8
                         *
                         */

                        /*
                         * multiplication
                         */
                        __asm__("movq %mm2, %mm5");
                        __asm__("movq %mm5, %mm6");
                        __asm__("pmullw %0, %%mm5"::"m"(B_cb_mult_constant));
                        __asm__("pmulhw %0, %%mm6"::"m"(B_cb_mult_constant));

                        // on va passer sur 4 entiers de 32 bits
                        __asm__("movq %mm5, %mm7");
                        __asm__("punpcklwd %mm6, %mm5");
                        __asm__("punpckhwd %mm6, %mm7");
                        // on fait le décalage arithmétique à droite de 8
                        __asm__("psrad $8, %mm5");
                        __asm__("psrad $8, %mm7");
                        // on va repasser sur 16 bits
                        __asm__("packssdw %mm7, %mm5");
                        // addition
                        __asm__("paddw %mm0, %mm5");

                        // Récupération des valeurs pour les mettres dans ARGB
                        __asm__("packuswb %mm7, %mm3");
                        __asm__("packuswb %mm7, %mm4");
                        __asm__("packuswb %mm7, %mm5");

                        __asm__("movd %%mm3, %0":"=m"(R_mmx[0]));
                        __asm__("movd %%mm4, %0":"=m"(G_mmx[0]));
                        __asm__("movd %%mm5, %0":"=m"(B_mmx[0]));


                        for (idx = 0; idx < 4; idx++) {
                                ARGB[idx] = ((R_mmx[idx] & 0xff) << 16) |
                                        ((G_mmx[idx] & 0xff) << 8) |
                                        (B_mmx[idx] & 0xff);
                                RGB_MCU[index + idx] = ARGB[idx];
                        }
                }
        }
}
#endif

#if VERSION == 20
void YCrCb_to_ARGB(uint8_t *YCrCb_MCU[3], uint32_t *RGB_MCU, uint32_t nb_MCU_H, uint32_t nb_MCU_V)
{
        uint8_t *MCU_Y, *MCU_Cr, *MCU_Cb;
        uint8_t index, i, j, idx;
        int32_t R[4], G[4], B[4];
        uint32_t ARGB[4];
        // Soustraction de 128 sur chacun des 16bits d'un registre mmx
        const uint64_t magic_substract = 0x0080008000800080;
        // Constante de multiplication
        //
        // R : 359 donc 0x0167 0167 0167 0167
        const uint64_t R_mult_constant = 0x0167016701670167;
        // G(Cr) : 183 donc 0x00b7 00b7 00b7 00b7
        const uint64_t G_cr_mult_constant = 0x00b700b700b700b7;
        // G(Cb) : 88 donc 0x0058 0058 0058 0058
        const uint64_t G_cb_mult_constant = 0x0058005800580058;
        // B(Cb) : 455 donc 0x01c7 01c7 01c7 01c7
        const uint64_t B_cb_mult_constant = 0x01c701c701c701c7;

#ifdef VERSION
        printf("Version %d", VERSION);
#endif
#ifdef DEBUG
        printf(" (debug)\n");
#else
        printf("\n");
#endif

        /* Vide la pile des instructions flottantes */
        __asm__("emms");

        MCU_Y = YCrCb_MCU[0];
        MCU_Cr = YCrCb_MCU[2];
        MCU_Cb = YCrCb_MCU[1];

        for (i = 0; i < 8 * nb_MCU_V; i++) {
                for (j = 0; j < 8 * nb_MCU_H; j += 4) {
                        index = i * (8 * nb_MCU_H) + j;
                        // calcul de référence


                        // 0x00 | MCU_Y[index + 3] | 0x00 | MCU_Y[index + 2] | 0x00 | MCU_Y[index + 1] | 0x00 | MCU_Y[index + 0] -> %mm0
                        // passage des valeurs des Y en poids fort
                        // (le calcul de RGB nécessite Y << 8
                        // donc comme ça, c'est déjà fait !)
                        __asm__("pxor %mm0, %mm0");
                        __asm__("movq %0, %%mm7"::"m"(MCU_Y[index]));
                        __asm__("punpcklbw %mm7, %mm0");



                        __asm__("pxor %mm7, %mm7");
                        __asm__("movq %0, %%mm1"::"m"(MCU_Cr[index]));
                        __asm__("punpcklbw %mm7, %mm1");

                        // soustraction
                        __asm__("psubw %0, %%mm1"::"m"(magic_substract));


                        __asm__("movq %0, %%mm2"::"m"(MCU_Cb[index]));
                        __asm__("punpcklbw %mm7, %mm2");

                        // soustraction
                        __asm__("psubw %0, %%mm2"::"m"(magic_substract));



                        // Calcul de R
                        // R = ((Y << 8) + (Cr - 128) * 359) >> 8
                        // R = ( (MM0) + (MM1) * 359) >> 8
                        //
                        // MM3 <- MM1 * 359
                        // MM3 <- MM3 + MM0
                        // shift MM3

                        // multiplication
                        __asm__("movq %mm1, %mm3");
                        __asm__("pmullw %0, %%mm3"::"m"(R_mult_constant));

								// addition
                        __asm__("paddw %mm0, %mm3");

                        // shift
                        __asm__("psrlw $8, %mm3");

                        // Calcul de G
                        // G = ((MCU_Y[index] << 8) - (MCU_Cr[index] - 128) * 183 - (MCU_Cb[index] - 128) * 88) >> 8;
                        // MM4 = ( MM0 ) - ( MM1 ) * 183 - ( MM2 ) * 88) >> 8;
                        //
                        // MM4 <- MM0
                        // MM5 <- MM1 * 183
                        // MM4 <- MM4 - MM5
                        // MM5 <- MM2 * 88
                        // MM4 <- MM5 - MM5
                        // shift MM4

                        // multiplication
                        __asm__("movq %mm1, %mm5");
                        __asm__("pmullw %0, %%mm5"::"m"(G_cr_mult_constant));

                        // soustraction
                        __asm__("movq %mm0, %mm4");
                        __asm__("psubw %mm5, %mm4");

                        // multiplication
                        __asm__("movq %mm2, %mm5");
                        __asm__("pmullw %0, %%mm5"::"m"(G_cb_mult_constant));

                        // soustraction
                        __asm__("psubw %mm5, %mm4");

                        // shift
                        __asm__("psrlw $8, %mm4");

                        // Calcul de B
                        // G = ((Y << 8) + (Cb - 128) * 455) >> 8
                        // R = ( (MM0) + (MM2) * 455) >> 8
                        //
                        // MM5 <- MM2 * 455
                        // MM5 <- MM5 + MM0
                        // shift MM5

                        // multiplication
                        __asm__("movq %mm2, %mm5");
                        __asm__("pmullw %0, %%mm5"::"m"(B_cb_mult_constant));

								// addition
                        __asm__("paddw %mm0, %mm5");

                        // shift
                        __asm__("psrlw $8, %mm5");


// Récupération des valeurs pour les mettres dans ARGB
                        __asm__("packuswb %mm7, %mm3");
                        __asm__("packuswb %mm7, %mm4");
                        __asm__("packuswb %mm7, %mm5");

                        uint8_t R_mmx[4] = { 0 };
                        uint8_t G_mmx[4] = { 0 };
                        uint8_t B_mmx[4] = { 0 };

                        __asm__("movd %%mm3, %0":"=m"(R_mmx[0]));
                        __asm__("movd %%mm4, %0":"=m"(G_mmx[0]));
                        __asm__("movd %%mm5, %0":"=m"(B_mmx[0]));


                       

                        for (idx = 0; idx < 4; idx++) {
                                ARGB[idx] = ((R_mmx[idx] & 0xff) << 16) |
                                        ((G_mmx[idx] & 0xff) << 8) |
                                        (B_mmx[idx] & 0xff);
                                RGB_MCU[index + idx] = ARGB[idx];
                        }
                }
        }
}
#endif
