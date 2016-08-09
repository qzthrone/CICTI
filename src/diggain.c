/* ============================================================================
 * Copyright (c) 2016 Texas Instruments Incorporated.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
  ===============================================================================*/

#include "data_types.h"
#include "diggain.h"

#define QUANT_TRUNC         ( 0 )   /* truncate */
#define QUANT_RND_INF       ( 1 )   /* round to infinite */
#define QUANT_MODE          ( QUANT_RND_INF )

/* Applies digital gain and saturates output. */
/* S18Q16 input data, S16Q15 output data. */
/* U16Q8 digital gain. */
void appDiggain(
    Int32   *inSamps,   /* input samples (S18Q16) */
    Uint16  diggain,    /* digital gain (U16Q8) */
    Int16   *outSamps,  /* output samples (S16Q15) */
    Uint16  numInSamps  /* number of input samples */
)
{
    Uint16 dataL;
    Int16 dataH;
    Uint32 prdLL;
    Int32 prdLH;
    Int64 acc0_40b;
    Uint16 i;
    
    for (i = 0; i < numInSamps; i++)
    {
        /* S18Q16 */
        dataL = (Uint16)inSamps[i];
        dataH = (Int16)(inSamps[i] >> 16);

        /* S18Q16 * U16Q8 = S34Q24 */
        prdLL = (Uint32)dataL * diggain;
        prdLH = (Int32)dataH * (Uint32)diggain;

        /* S40Q24 + S34Q24 = S40Q24 */
        acc0_40b = (Uint64)prdLL;
        acc0_40b += (Int64)(prdLH << 16);

#if (QUANT_MODE == QUANT_RND_INF)
        acc0_40b += (Uint16)1<<8; /* round to infinite */
#endif
        acc0_40b >>= 9; /* truncate */

#ifdef _WIN32
        /* Saturate output */
        if (acc0_40b > (Int64)0x7FFF)
        {
            acc0_40b = (Int64)0x7FFF;
        }
        else if (acc0_40b < (Int64)0xFFFFFFFFFFFF8000)
        {
            acc0_40b = (Int64)0xFFFFFFFFFFFF8000;
        }
#else
        /* Saturate output */
        if (acc0_40b > (Int64)0x7FFF)
        {
            acc0_40b = (Int64)0x7FFF;
        }
        else if (acc0_40b < (Int64)0xFFFFFF8000)
        {
            acc0_40b = (Int64)0xFFFFFF8000;
        }

#endif        

        outSamps[i] = (Int16)acc0_40b; /* S16Q15 */
    }
}

