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
 * 5 : calcul de RGB
 */
#define VERSION 6
#define DEBUG




#include "conv.h"
#include "stdlib.h"
#include "stdio.h"
#include "utils.h"
#include <assert.h>
#include <inttypes.h>

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
                        G = ((MCU_Y[index] << 8) - (MCU_Cr[index] - 128) * 183 - (MCU_Cb[index] - 128) * 88) >> 8;
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
                        
                        // 0x00 | MCU_Y[index + 3] | 0x00 | MCU_Y[index + 2] | 0x00 | MCU_Y[index + 1] | 0x00 | MCU_Y[index + 0] -> %mm0
                        // passage des valeurs des Y en poids fort 
                        // (le calcul de RGB nécessite Y << 8 
                        //  donc comme ça, c'est déjà fait !)
                        __asm__("pxor %mm0, %mm0");
                        __asm__("movq %0, %%mm7"::"m"(MCU_Y[index]));
                        __asm__("punpcklbw %mm7, %mm0");
#                       ifdef DEBUG
                        uint16_t Y[4];
                        __asm__("movq %%mm0, %0":"=m"(Y[0]));
                        assert(MCU_Y[index] == Y[0] >> 8);
                        assert(MCU_Y[index + 1] == Y[1] >> 8);
                        assert(MCU_Y[index + 2] == Y[2] >> 8);
                        assert(MCU_Y[index + 3] == Y[3] >> 8);
#                       endif


                        __asm__("pxor %mm7, %mm7");
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


                        // Calcul de R
                        // R = ((Y << 8) + (Cr - 128) * 359) >> 8
                        // R = (  (MM0)  +    (MM1)   * 359) >> 8
                        //
                        // MM3 <- MM1 * 359
                        // MM3 <- MM3 + MM0
                        // shift MM3
                        uint64_t R_avant;
                        uint64_t R_apres;
                        
                        // multiplication
                        __asm__("movq %mm1, %mm3");

                        __asm__("movq %%mm3, %0":"=m"(R_avant));
                        
                        __asm__("pmullw %0, %%mm3"::"m"(R_mult_constant));
                        

                        __asm__("movq %%mm3, %0":"=m"(R_apres));
                        printf("Rmull 0x%016"PRIx64" x 0x%016"PRIx64" | 0x%016"PRIx64"\n", R_avant, R_mult_constant, R_apres);
                        printf("Rmull theorique 0x%x 0x%x 0x%x 0x%x\n", 
                                        (((MCU_Cr[index] - 128) * 359)),
                                        (((MCU_Cr[index + 1] - 128) * 359)),
                                        (((MCU_Cr[index + 2] - 128) * 359)),
                                        (((MCU_Cr[index + 3] - 128) * 359)));
                        // addition

                        __asm__("movq %%mm3, %0":"=m"(R_avant));
                        uint64_t mm0;
                        __asm__("movq %%mm0, %0":"=m"(mm0));

                        __asm__("paddsw %mm0, %mm3");
                        //__asm__("packuswb %mm7, %mm3");


                        __asm__("movq %%mm3, %0":"=m"(R_apres));
                        printf("Radd 0x%016"PRIx64" + 0x%016"PRIx64" | 0x%016"PRIx64"\n", R_avant, mm0, R_apres);
                        printf("Radd theorique 0x%x 0x%x 0x%x 0x%x\n", 
                                        ((MCU_Y[index] << 8) + ((MCU_Cr[index] - 128) * 359)),
                                        ((MCU_Y[index + 1] << 8) + ((MCU_Cr[index + 1] - 128) * 359)),
                                        ((MCU_Y[index + 2] << 8) + ((MCU_Cr[index + 2] - 128) * 359)),
                                        ((MCU_Y[index + 3] << 8) + ((MCU_Cr[index + 3] - 128) * 359)));

                        // shift

                        __asm__("movq %%mm3, %0":"=m"(R_avant));

                        __asm__("psrlw $8, %mm3");

                        __asm__("movq %%mm3, %0":"=m"(R_apres));
                        printf("Rshift 0x%016"PRIx64" | 0x%016"PRIx64"\n", R_avant, R_apres);
                        printf("Rshift theorique 0x%x 0x%x 0x%x 0x%x\n", 
                                        ((MCU_Y[index] << 8) + ((MCU_Cr[index] - 128) * 359)) >> 8,
                                        ((MCU_Y[index + 1] << 8) + ((MCU_Cr[index + 1] - 128) * 359)) >> 8,
                                        ((MCU_Y[index + 2] << 8) + ((MCU_Cr[index + 2] - 128) * 359)) >> 8,
                                        ((MCU_Y[index + 3] << 8) + ((MCU_Cr[index + 3] - 128) * 359)) >> 8);


#                       ifdef DEBUG
                        int16_t R_mmx[4];
                        __asm__("movq %%mm3, %0":"=m"(R_mmx[0]));
                        if ((int16_t) R_mmx[0] < 0) R_mmx[0] = 0;
                        if ((int16_t) R_mmx[1] < 0) R_mmx[1] = 0;
                        if ((int16_t) R_mmx[2] < 0) R_mmx[2] = 0;
                        if ((int16_t) R_mmx[3] < 0) R_mmx[3] = 0;
                        if ((int16_t) R_mmx[0] > 255) R_mmx[0] = 255;
                        if ((int16_t) R_mmx[1] > 255) R_mmx[1] = 255;
                        if ((int16_t) R_mmx[2] > 255) R_mmx[2] = 255;
                        if ((int16_t) R_mmx[3] > 255) R_mmx[3] = 255;

                        printf("R_mmx[3] = %d | R[3] %x\n", (int16_t) R_mmx[3], R[3]);
                        assert(R_mmx[3] == R[3]);
                        assert(R_mmx[2] == R[2]);
                        assert(R_mmx[1] == R[1]);
                        assert(R_mmx[0] == R[0]);
#                       endif





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

                        // 0x00 | MCU_Y[index + 3] | 0x00 | MCU_Y[index + 2] | 0x00 | MCU_Y[index + 1] | 0x00 | MCU_Y[index + 0] -> %mm0
                        // passage des valeurs des Y en poids fort 
                        // (le calcul de RGB nécessite Y << 8 
                        //  donc comme ça, c'est déjà fait !)
                        __asm__("pxor %mm0, %mm0");
                        __asm__("movq %0, %%mm7"::"m"(MCU_Y[index]));
                        __asm__("punpcklbw %mm7, %mm0");
#                       ifdef DEBUG
                        uint16_t Y[4];
                        __asm__("movq %%mm0, %0":"=m"(Y[0]));
                        assert(MCU_Y[index] == Y[0] >> 8);
                        assert(MCU_Y[index + 1] == Y[1] >> 8);
                        assert(MCU_Y[index + 2] == Y[2] >> 8);
                        assert(MCU_Y[index + 3] == Y[3] >> 8);
#                       endif


                        __asm__("pxor %mm7, %mm7");
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


                        // Calcul de R
                        // R = ((Y << 8) + (Cr - 128) * 359) >> 8
                        // R = (  (MM0)  +    (MM1)   * 359) >> 8
                        //
                        // MM3 <- MM1 * 359
                        // MM3 <- MM3 + MM0
                        // shift MM3

                        // multiplication
                        __asm__("movq %mm1, %mm3");
                        __asm__("pmullw %0, %%mm3"::"m"(R_mult_constant));
                        // addition
                        __asm__("paddsw %mm0, %mm3");
                        // shift
                        __asm__("psrlw $8, %mm3");

                        // Calcul de G
                        __asm__("movq %mm1, %mm6");
                        __asm__("pmullw %0, %%mm6"::"m"(G_cr_mult_constant));
                        __asm__("movq %mm2, %mm5");
                        __asm__("pmullw %0, %%mm5"::"m"(G_cb_mult_constant));
                        __asm__("movq %mm0, %mm4");
                        __asm__("psubw %mm6, %mm4");
                        __asm__("psubw %mm5, %mm4");
                        __asm__("psrlw $8, %mm4");

                        // Calcul de B
                        __asm__("movq %mm2, %mm5");
                        __asm__("pmullw %0, %%mm5"::"m"(B_cb_mult_constant));
                        __asm__("paddsw %mm0, %mm5");
                        __asm__("psrlw $8, %mm5");


                        int16_t R[4];
                        int16_t G[4];
                        int16_t B[4];
                        __asm__("movq %%mm3, %0":"=m"(R[0]));
                        __asm__("movq %%mm4, %0":"=m"(G[0]));
                        __asm__("movq %%mm5, %0":"=m"(B[0]));

                       ARGB[0] =        ((R[0] & 0xff) << 16) |
                                        ((G[0] & 0xff) <<  8) |
                                        ((B[0] & 0xff));
                       RGB_MCU[index] = ARGB[0];
                       ARGB[1] =        ((R[1] & 0xff) << 16) |
                                        ((G[1] & 0xff) <<  8) |
                                        ((B[1] & 0xff));
                       RGB_MCU[index + 1] = ARGB[1];
                       ARGB[2] =        ((R[2] & 0xff) << 16) |
                                        ((G[2] & 0xff) <<  8) |
                                        ((B[2] & 0xff));
                       RGB_MCU[index + 2] = ARGB[2];
                       ARGB[3] =        ((R[3] & 0xff) << 16) |
                                        ((G[3] & 0xff) <<  8) |
                                        ((B[3] & 0xff));
                       RGB_MCU[index + 3] = ARGB[3];
                       
                }
        }
}
#endif
