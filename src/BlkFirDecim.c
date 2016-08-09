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
#include "BlkFirDecim.h"

#define DECIM_FACT          ( 2 )   /* decimation factor */

#define QUANT_TRUNC         ( 0 )   /* truncate */
#define QUANT_RND_INF       ( 1 )   /* round to infinite */
#define QUANT_MODE          ( QUANT_RND_INF )

/* Block decimating FIR, */
/* S18Q16 input and output data, */
/* S16Q15 coefficients. */
/* Decimation factor fixed at 2. */
/* Computes two outputs per inner loop (assumes even number of outputs). */
void blkFirDecim2(
    Int32   *inSamps,       /* input samples (S18Q16) */
    Int16   *coefs,         /* filter coefficients (S16Q15) */
    Int32   *outSamps,      /* output samples (S18Q16) */
    Int32   *dlyBuf,        /* delay buffer */
    Uint16  numInSamps,     /* number of input samples*/
    Uint16  numCoefs        /* number of coefficients */
)
{
    Uint16 numOutSamps;
    Uint16 numDlySamps;
    Uint16 dataL_1, dataL_2;
    Int16 dataH_1, dataH_2;
    Int32 prdLL_1, prdLL_2;
    Int32 prdLH_1, prdLH_2;
    Int64 acc0_40b, acc1_40b, acc2_40b, acc3_40b;
    Uint16 dlyBufIdx_1; // leading index
    Uint16 dlyBufIdx_2; // lagging index
    Uint16 inSampIdx, outSampIdx;
    Uint16 outSampCnt;
    Uint16 i;

    /* Compute number of output samples */
    numOutSamps = numInSamps>>1;

    /* Compute number of delay buffer samples */
    numDlySamps = numCoefs+2;

    /* Read delay index of oldest sample */
    dlyBufIdx_2 = dlyBuf[0]; // index of oldest sample stored in 0th location of delay buffer

    /* Initialize lagging delay index */
    dlyBufIdx_2++; // adjust index to 1->numDlySamps
    if (dlyBufIdx_2 > numDlySamps)
    {
        dlyBufIdx_2 = 1;
    }

    /* Initialize leading delay index */
    dlyBufIdx_1 = dlyBufIdx_2;
    dlyBufIdx_1--;
    if (dlyBufIdx_1 < 1)
    {
        dlyBufIdx_1 = numDlySamps;
    }

    inSampIdx = 0;
    outSampIdx = 0;
    for (outSampCnt = 0; outSampCnt < numOutSamps/2; outSampCnt++) 
    {
        /* Place 1 sample in delay line at lagging index */
        dlyBuf[dlyBufIdx_2] = inSamps[inSampIdx++];

        /* Place D=2 samples in delay line at leading index */
        dlyBuf[dlyBufIdx_1] = inSamps[inSampIdx++];
        dlyBufIdx_1--;
        if (dlyBufIdx_1 < 1)
        {
            dlyBufIdx_1 = numDlySamps;
        }
        dlyBuf[dlyBufIdx_1] = inSamps[inSampIdx++];

        /* Compute lagging output */
        /* S18Q16 */
        dataL_2 = (Uint16)dlyBuf[dlyBufIdx_2];
        dataH_2 = (Int16)(dlyBuf[dlyBufIdx_2++] >> 16);
        if (dlyBufIdx_2 > numDlySamps)
        {
            dlyBufIdx_2 = 1;
        }
        /* S18Q16 * S16Q15 = S34Q31 */
        prdLL_2 = (Uint32)dataL_2 * (Int32)coefs[0];
        prdLH_2 = (Int32)dataH_2 * coefs[0]; 
        /* S40Q31 + S34Q31 = S40Q31 */
        acc2_40b = (Int64)prdLL_2;
        acc3_40b = (Int64)prdLH_2;

        /* Compute leading output */
        /* S18Q16 */
        dataL_1 = (Uint16)dlyBuf[dlyBufIdx_1];
        dataH_1 = (Int16)(dlyBuf[dlyBufIdx_1++] >> 16);
        if (dlyBufIdx_1 > numDlySamps)
        {
            dlyBufIdx_1 = 1;
        }
        /* S18Q16 * S16Q15 = S34Q31 */
        prdLL_1 = (Uint32)dataL_1 * (Int32)coefs[0];
        prdLH_1 = (Int32)dataH_1 * coefs[0]; 
        /* S40Q31 + S34Q31 = S40Q31 */
        acc0_40b = (Int64)prdLL_1;
        acc1_40b = (Int64)prdLH_1;

        for (i = 1; i < numCoefs-1; i++)
        {
            /* Compute lagging output */
            /* S18Q16 */
            dataL_2 = (Uint16)dlyBuf[dlyBufIdx_2];
            dataH_2 = (Int16)(dlyBuf[dlyBufIdx_2++] >> 16);
            if (dlyBufIdx_2 > numDlySamps)
            {
                dlyBufIdx_2 = 1;
            }
            /* S18Q16 * S16Q15 = S34Q31 */
            prdLL_2 = (Uint32)dataL_2 * (Int32)coefs[i];
            prdLH_2 = (Int32)dataH_2 * coefs[i]; 
            /* S40Q31 + S34Q31 = S40Q31 */
            acc2_40b += (Int64)prdLL_2;
            acc3_40b += (Int64)prdLH_2;

            /* Compute leading output */
            /* S18Q16 */
            dataL_1 = (Uint16)dlyBuf[dlyBufIdx_1];
            dataH_1 = (Int16)(dlyBuf[dlyBufIdx_1++] >> 16);
            if (dlyBufIdx_1 > numDlySamps)
            {
                dlyBufIdx_1 = 1;
            }
            /* S18Q16 * S16Q15 = S34Q31 */
            prdLL_1 = (Uint32)dataL_1 * (Int32)coefs[i];
            prdLH_1 = (Int32)dataH_1 * coefs[i]; 
            /* S40Q31 + S34Q31 = S40Q31 */
            acc0_40b += (Int64)prdLL_1;
            acc1_40b += (Int64)prdLH_1;
        }

        /* Compute lagging output */
        /* S18Q16 */
        dataL_2 = (Uint16)dlyBuf[dlyBufIdx_2];
        dataH_2 = (Int16)(dlyBuf[dlyBufIdx_2] >> 16);
        if (dlyBufIdx_2 > numDlySamps)
        {
            dlyBufIdx_2 = 1;
        }
        /* S18Q16 * S16Q15 = S34Q31 */
        prdLL_2 = (Uint32)dataL_2 * (Int32)coefs[numCoefs-1];
        prdLH_2 = (Int32)dataH_2 * coefs[numCoefs-1]; 
        /* S40Q31 + S34Q31 = S40Q31 */
        acc2_40b += (Int64)prdLL_2;
        acc3_40b += (Int64)prdLH_2;

        /* Compute leading output */
        /* S18Q16 */
        dataL_1 = (Uint16)dlyBuf[dlyBufIdx_1];
        dataH_1 = (Int16)(dlyBuf[dlyBufIdx_1] >> 16);
        if (dlyBufIdx_1 > numDlySamps)
        {
            dlyBufIdx_1 = 1;
        }
        /* S18Q16 * S16Q15 = S34Q31 */
        prdLL_1 = (Uint32)dataL_1 * (Int32)coefs[numCoefs-1];
        prdLH_1 = (Int32)dataH_1 * coefs[numCoefs-1]; 
        /* S40Q31 + S34Q31 = S40Q31 */
        acc0_40b += (Int64)prdLL_1;
        acc1_40b += (Int64)prdLH_1;

        acc2_40b += acc3_40b << 16;
        acc0_40b += acc1_40b << 16;

#if (QUANT_MODE == QUANT_RND_INF)
        acc2_40b += (Uint16)1<<14; /* round to infinite */
        acc0_40b += (Uint16)1<<14; /* round to infinite */
#endif
        acc2_40b >>= 15; /* truncate */
        outSamps[outSampIdx++] = (Int32)acc2_40b;
        acc0_40b >>= 15; /* truncate */
        outSamps[outSampIdx++] = (Int32)acc0_40b;

        /* Place D-1=1 samples in delay line at lagging index */
        dlyBuf[dlyBufIdx_2] = inSamps[inSampIdx++];
        dlyBufIdx_2--;
        if (dlyBufIdx_2 < 1)
        {
            dlyBufIdx_2 = numDlySamps;
        }
    }

    /* Write delay index of oldest sample */
    dlyBuf[0] = dlyBufIdx_2-1; // adjust index to 0->numDlySamps-1
}
