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
#include "BlkIir.h"

#define QUANT_TRUNC         ( 0 )   /* truncate */
#define QUANT_RND_INF       ( 1 )   /* round to infinite */
#define QUANT_MODE          ( QUANT_RND_INF )

/* Block IIR, Direct Form II */
/* S18Q16 input and output data */
/* Fixed-point format of coefficients determined by coefficient integer wordlength parameter */
/* Input gain applied to input signal */
void blkIirDf2(
    Int32   *inSamps,       /* input samples (S18Q16) */
    Int16   *coefs,         /* filter coefficients (S16Q15) */
    Int32   *outSamps,      /* output samples (S18Q16) */
    Int32   *dlyBuf,        /* delay buffer */
    Uint16  inGain,         /* input gain (U16Q16) */
    Uint16  numInSamps,     /* number of input samples */
    Uint16  numBiquads,     /* number of biquads */
    Uint16  coefIWL         /* coefficient integer wordlength */
)
{
    Uint16 numDlySamps;
    Uint16 dataL;
    Int16 dataH;
    Uint32 uPrdLL;
    Int32 prdLL, prdLH;
    Int64 acc_40b;
    Uint16 dlyBufIdx;
    Uint16 coefIdx;
    Uint16 i, j;


    /* Compute number of delay buffer samples */
    numDlySamps = 2*numBiquads;

    /* Read current delay index */
    dlyBufIdx = dlyBuf[0];

    /* Initialize delay index */
    dlyBufIdx++; // adjust index to 1->numDlySamps
    if (dlyBufIdx > numDlySamps)
    {
        dlyBufIdx -= numDlySamps;
    }

    for (i = 0; i < numInSamps; i++)
    {
        coefIdx = 0;

        //acc_40b = inSamps[i]<<15; // acc = x(n) (S18Q16->S33Q31)

        // acc = G*x(n)
        dataL = (Uint16)inSamps[i];
        dataH = (Int16)(inSamps[i] >> 16);
        /* S18Q16 * U16Q16 = S34Q32 */
        uPrdLL = (Uint32)dataL * inGain;
        prdLH = (Int32)dataH * (Uint32)inGain;
        acc_40b = (Uint64)uPrdLL;
        acc_40b += ((Int64)prdLH << 16);
        
        for (j = 0; j < numBiquads; j++)
        {
            /* Compute current biquad output */

            /* Compute d(n) */
            // acc = x(n) - a1*d(n-1)
            dataL = (Uint16)dlyBuf[dlyBufIdx]; // d(n-1)
            dataH = (Int16)(dlyBuf[dlyBufIdx] >> 16);
            dlyBufIdx += numBiquads;
            if (dlyBufIdx > numDlySamps)
            {
                dlyBufIdx -= numDlySamps;
            }
            /* S18Q16 * S16Q(16-1-IWL) = S34Q(32-1-IWL) */
            prdLL = (Uint32)dataL * (Int32)coefs[coefIdx]; // a1
            prdLH = (Int32)dataH * coefs[coefIdx++];
            /* S40Q31 + S34Q(32-1-IWL) = S40Q31 */
            acc_40b -= ((Int64)prdLL << (1+coefIWL));
            acc_40b -= ((Int64)prdLH << (16+1+coefIWL));

            // acc = x(n) - a1*d(n-1) - a2*d(n-2)
            dataL = (Uint16)dlyBuf[dlyBufIdx]; // d(n-2)
            dataH = (Int16)(dlyBuf[dlyBufIdx] >> 16);
            /* S18Q16 * S16Q(16-1-IWL) = S34Q(32-1-IWL) */
            prdLL = (Uint32)dataL * (Int32)coefs[coefIdx]; //a2
            prdLH = (Int32)dataH * coefs[coefIdx++];
            /* S40Q31 + S34Q(32-1-IWL) = S40Q31 */
            acc_40b -= ((Int64)prdLL << (1+coefIWL));
            acc_40b -= ((Int64)prdLH << (16+1+coefIWL));

            /* Update d(n) */
#if (QUANT_MODE == QUANT_RND_INF)
            acc_40b += (Uint16)1<<(14+1); /* round to infinite */
#endif
            dlyBuf[dlyBufIdx] = (Int32)(acc_40b>>(15+1));

            /* Compute y(n) */
            // acc = b2*d(n-2)
            /* S18Q16 * S16Q(16-1-IWL) = S34Q(32-1-IWL) */
            prdLL = (Uint32)dataL * (Int32)coefs[coefIdx]; // b2
            prdLH = (Int32)dataH * coefs[coefIdx++];
            /* S40Q31 + S34Q(32-1-IWL) = S40Q31 */
            acc_40b = ((Int64)prdLL << (1+coefIWL));
            acc_40b += ((Int64)prdLH << (16+1+coefIWL));

            // acc = b0*d(n) + b2*d(n-2)
            dataL = (Uint16)dlyBuf[dlyBufIdx]; // d(n)
            dataH = (Int16)(dlyBuf[dlyBufIdx] >> 16);
            dlyBufIdx += numBiquads;
            if (dlyBufIdx > numDlySamps)
            {
                dlyBufIdx -= numDlySamps;
            }
            /* S18Q16 * S16Q(16-1-IWL) = S34Q(32-1-IWL) */
            prdLL = (Uint32)dataL * (Int32)coefs[coefIdx]; // b0
            prdLH = (Int32)dataH * coefs[coefIdx++];
            /* S40Q31 + S34Q(32-1-IWL) = S40Q31 */
            acc_40b += ((Int64)prdLL << (1+coefIWL));
            acc_40b += ((Int64)prdLH << (16+1+coefIWL));

            // acc = b0*d(n) + b1*d(n-1) + b2*d(n-2)
            dataL = (Uint16)dlyBuf[dlyBufIdx]; // d(n-1)
            dataH = (Int16)(dlyBuf[dlyBufIdx] >> 16);
            dlyBufIdx++;
            if (dlyBufIdx > numDlySamps)
            {
                dlyBufIdx -= numDlySamps;
            }
            /* S18Q16 * S16Q(16-1-IWL) = S34Q(32-1-IWL) */
            prdLL = (Uint32)dataL * (Int32)coefs[coefIdx]; // b1
            prdLH = (Int32)dataH * coefs[coefIdx++];
            /* S40Q31 + S34Q(32-1-IWL) = S40Q31 */
            acc_40b += ((Int64)prdLL << (1+coefIWL));
            acc_40b += ((Int64)prdLH << (16+1+coefIWL));
        }

#if (QUANT_MODE == QUANT_RND_INF)
        acc_40b += 1<<14;
#endif

        outSamps[i] = (Int32)acc_40b>>(15+1);
    }

    /* Update current delay index */
    dlyBuf[0] = dlyBufIdx-1; // adjust index to 0->numDlySamps-1
}

/* Block IIR, Direct Form I */
/* S18Q16 input and output data */
/* Fixed-point format of coefficients determined by coefficient integer wordlength parameter */
/* Input gain applied to input signal */
void blkIirDf1(
    Int32   *inSamps,       /* input samples (S18Q16) */
    Int16   *coefs,         /* filter coefficients (S16Q15) */
    Int32   *outSamps,      /* output samples (S18Q16) */
    Int32   *dlyBuf,        /* delay buffer */
    Uint16  inGain,         /* input gain (U16Q16) */
    Uint16  numInSamps,     /* number of input samples */
    Uint16  numBiquads,     /* number of biquads */
    Uint16  coefIWL         /* coefficient integer wordlength */
)
{
    Uint16 numDlySamps;
    Uint16 dataL;
    Int16 dataH;
    Uint32 uPrdLL;
    Int32 prdLL, prdLH;
    Int64 acc_40b;
    Int32 tIn;
    Int32 *ffDlyBuf, *fbDlyBuf;
    Uint16 ffDlyBufIdx; // feed forward delay index
    Uint16 fbDlyBufIdx; // feed back delay index
    Uint16 coefIdx;
    Uint16 i, j;


    /* Compute number of delay buffer samples */
    numDlySamps = 2*numBiquads;

    /* Initialize feed forward delay buffer pointer */
    ffDlyBuf = &dlyBuf[1];
    ffDlyBufIdx = dlyBuf[0];
    /* Initialize feed back delay buffer pointer & index */
    fbDlyBuf = &dlyBuf[numDlySamps+1];
    fbDlyBufIdx = ffDlyBufIdx;

    for (i = 0; i < numInSamps; i++)
    {
        coefIdx = 0;

        // acc = x(n) (S18Q16->S33Q31)
        //acc_40b = inSamps[i]<<15;

        /* Compute G*x(n) */
        // acc = G*x(n)
        dataL = (Uint16)inSamps[i];
        dataH = (Int16)(inSamps[i] >> 16);
        /* S18Q16 * U16Q16 = S34Q32 */
        uPrdLL = (Uint32)dataL * inGain;
        prdLH = (Int32)dataH * (Uint32)inGain;
        acc_40b = (Uint64)uPrdLL;
        acc_40b += ((Int64)prdLH << 16);

        /* Store G*x(n) */
#if (QUANT_MODE == QUANT_RND_INF)
        acc_40b += (Uint16)1<<15; /* round to infinite */
#endif
        tIn = (Int32)(acc_40b >> 16); // S18Q16
        
        for (j = 0; j < numBiquads; j++)
        {
            /* Compute current biquad output */

            // acc = b0*x(n)
            dataL = (Uint16)tIn; // x(n)
            dataH = (Int16)(tIn >> 16);
            /* S18Q16 * S16Q(16-1-IWL) = S34Q(32-1-IWL) */
            prdLL = (Uint32)dataL * (Int32)coefs[coefIdx]; // b0
            prdLH = (Int32)dataH * coefs[coefIdx++];
            /* S40Q31 + S34Q(32-1-IWL) = S40Q31 */
            acc_40b = ((Int64)prdLL << coefIWL);
            acc_40b += ((Int64)prdLH << (16+coefIWL));

            // acc = b0*x(n) + b1*x(n-1)
            dataL = (Uint16)ffDlyBuf[ffDlyBufIdx]; // x(n-1)
            dataH = (Int16)(ffDlyBuf[ffDlyBufIdx] >> 16);
            /* S18Q16 * S16Q(16-1-IWL) = S34Q(32-1-IWL) */
            prdLL = (Uint32)dataL * (Int32)coefs[coefIdx]; // b1
            prdLH = (Int32)dataH * coefs[coefIdx++];
            /* S40Q31 + S34Q(32-1-IWL) = S40Q31 */
            acc_40b += ((Int64)prdLL << coefIWL);
            acc_40b += ((Int64)prdLH << (16+coefIWL));

            ffDlyBufIdx += numBiquads; // point to x(n-2D)
            if (ffDlyBufIdx > numDlySamps-1)
            {
                ffDlyBufIdx -= numDlySamps;
            }

            // acc = b0*x(n) + b1*x(n-1) + b2*x(n-2)
            dataL = (Uint16)ffDlyBuf[ffDlyBufIdx]; // x(n-2)
            dataH = (Int16)(ffDlyBuf[ffDlyBufIdx] >> 16);
            /* S18Q16 * S16Q(16-1-IWL) = S34Q(32-1-IWL) */
            prdLL = (Uint32)dataL * (Int32)coefs[coefIdx]; // b2
            prdLH = (Int32)dataH * coefs[coefIdx++];
            /* S40Q31 + S34Q(32-1-IWL) = S40Q31 */
            acc_40b += ((Int64)prdLL << coefIWL);
            acc_40b += ((Int64)prdLH << (16+coefIWL));

            // update x(n-2)
            ffDlyBuf[ffDlyBufIdx] = tIn;

            ffDlyBufIdx += numBiquads+1; // point to x(n-1) for next biquad
            if (ffDlyBufIdx > numDlySamps-1)
            {
                ffDlyBufIdx -= numDlySamps;
            }

            // acc = b0*x(n) + b1*x(n-1) + b2*x(n-2) - a1*y(n-1)
            dataL = (Uint16)fbDlyBuf[fbDlyBufIdx]; // y(n-1)
            dataH = (Int16)(fbDlyBuf[fbDlyBufIdx] >> 16);
            /* S18Q16 * S16Q(16-1-IWL) = S34Q(32-1-IWL) */
            prdLL = (Uint32)dataL * (Int32)coefs[coefIdx]; // a1
            prdLH = (Int32)dataH * coefs[coefIdx++];
            /* S40Q31 + S34Q(32-1-IWL) = S40Q31 */
            acc_40b -= ((Int64)prdLL << coefIWL);
            acc_40b -= ((Int64)prdLH << (16+coefIWL));

            fbDlyBufIdx += numBiquads; // point to y(n-2)
            if (fbDlyBufIdx > numDlySamps-1)
            {
                fbDlyBufIdx -= numDlySamps;
            }

            // acc = b0*x(n) + b1*x(n-1) + b2*x(n-2) - a1*y(n-1) - a2*y(n-2)
            dataL = (Uint16)fbDlyBuf[fbDlyBufIdx]; // y(n-2)
            dataH = (Int16)(fbDlyBuf[fbDlyBufIdx] >> 16);
            /* S18Q16 * S16Q(16-1-IWL) = S34Q(32-1-IWL) */
            prdLL = (Uint32)dataL * (Int32)coefs[coefIdx]; // a2
            prdLH = (Int32)dataH * coefs[coefIdx++];
            /* S40Q31 + S34Q(32-1-IWL) = S40Q31 */
            acc_40b -= ((Int64)prdLL << coefIWL);
            acc_40b -= ((Int64)prdLH << (16+coefIWL));

#if (QUANT_MODE == QUANT_RND_INF)
            acc_40b += (Uint16)1<<14; /* round to infinite */
#endif
            tIn = (Int32)(acc_40b >> 15); // S18Q16

            // update y(n-2)
            fbDlyBuf[fbDlyBufIdx] = tIn;

            fbDlyBufIdx += numBiquads+1; // point to y(n-1) for next biquad
            if (fbDlyBufIdx > numDlySamps-1)
            {
                fbDlyBufIdx -= numDlySamps;
            }
        }

        outSamps[i] = tIn;
    }

    /* Update current delay index */
    dlyBuf[0] = ffDlyBufIdx;
}
