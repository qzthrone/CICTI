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
#include "pick_bits_cic.h"

#define BITS_PER_16BW  ( 16 )

/* Unpacks "left" and "right" 32-bit packed DMA buffers containing output from digital mic. */
/* Performs CIC on unpacked data, CIC_DF = 16 & CIC_NS = 4. */
void pickBitsCic(
    Uint32 *lData,          /* "left" channel 32-bit packed input data */
    Uint32 *rData,          /* "right" channel 32-bit packed input data */
    Uint16 inDataLen,       /* length of "left" or "right" input data in 32-bit words */
    Int32 *cicState,        /* CIC state. First NS values are integrator state, next NS values are differentiator delay buffer */
    Int32 *outSamps,        /* CIC output samples */
    Uint16 *pNumOutSamps    /* CIC number of output samples */
)
{
    Uint16 cur16bW;
    Int16 input;
    Int32 *acc;
    Int32 *diffDly;
    Int32 diff[CIC_NS];
    Int32 *pOutSamp;
    Uint16 i, j;


    /* Compute number of output samples */
    /* x32 for 32-bit word, x2 for 2 channels */
    //*pNumOutSamps = inDataLen*32*2/DS;
    *pNumOutSamps = inDataLen<<2;

    acc = &cicState[0];
    diffDly = &cicState[CIC_NS];
    pOutSamp = &outSamps[0];
    for (i = 0; i < inDataLen; i++)
    {
        /* Process "left" channel MS 16-bit word for current 32-bit word */
        cur16bW = lData[i]>>BITS_PER_16BW;
        for (j = 0; j < CIC_DF; j++)
        {
            /* Get current input */
            /* 0->-1, 1->+1 */
            input = ((cur16bW>>(BITS_PER_16BW-2))&0x2) - 1;
            cur16bW <<= 1;

            /* Perform integration for current input */
            acc[0] += input;
            acc[1] += acc[0];
            acc[2] += acc[1];
            acc[3] += acc[2];
        }

        /* Perform decimation & differentiator stages */
        diff[0] = acc[3] - diffDly[0];
        diffDly[0] = acc[3];
        diff[1] = diff[0] - diffDly[1];
        diffDly[1] = diff[0];
        diff[2] = diff[1] - diffDly[2];
        diffDly[2] = diff[1];
        diff[3] = diff[2] - diffDly[3];
        diffDly[3] = diff[2];

        /* Write output sample */
        *pOutSamp++ = diff[3];

        /* Process "left" channel LS 16-bit word for current 32-bit word */
        cur16bW = lData[i]&0xFFFF;
        for (j = 0; j < CIC_DF; j++)
        {
            /* Get current input */
            /* 0->-1, 1->+1 */
            input = ((cur16bW>>(BITS_PER_16BW-2))&0x2) - 1;
            cur16bW <<= 1;

            /* Perform integration for current input */
            acc[0] += input;
            acc[1] += acc[0];
            acc[2] += acc[1];
            acc[3] += acc[2];
        }

        /* Perform decimation & differentiator stages */
        diff[0] = acc[3] - diffDly[0];
        diffDly[0] = acc[3];
        diff[1] = diff[0] - diffDly[1];
        diffDly[1] = diff[0];
        diff[2] = diff[1] - diffDly[2];
        diffDly[2] = diff[1];
        diff[3] = diff[2] - diffDly[3];
        diffDly[3] = diff[2];

        /* Write output sample */
        *pOutSamp++ = diff[3];

        /* Process "right" channel MS 16-bit word for current 32-bit word */
        cur16bW = rData[i]>>16;
        for (j = 0; j < CIC_DF; j++)
        {
            /* Get current input */
            /* 0->-1, 1->+1 */
            input = ((cur16bW>>(BITS_PER_16BW-2))&0x2) - 1;
            cur16bW <<= 1;

            /* Perform integration for current input */
            acc[0] += input;
            acc[1] += acc[0];
            acc[2] += acc[1];
            acc[3] += acc[2];
        }

        /* Perform decimation & differentiator stages */
        diff[0] = acc[3] - diffDly[0];
        diffDly[0] = acc[3];
        diff[1] = diff[0] - diffDly[1];
        diffDly[1] = diff[0];
        diff[2] = diff[1] - diffDly[2];
        diffDly[2] = diff[1];
        diff[3] = diff[2] - diffDly[3];
        diffDly[3] = diff[2];

        /* Write output sample */
        *pOutSamp++ = diff[3];

        /* Process "right" channel LS 16-bit word for current 32-bit word */
        cur16bW = rData[i]&0xFFFF;
        for (j = 0; j < CIC_DF; j++)
        {
            /* Get current input */
            /* 0->-1, 1->+1 */
            input = ((cur16bW>>(BITS_PER_16BW-2))&0x2) - 1;
            cur16bW <<= 1;

            /* Perform integration for current input */
            acc[0] += input;
            acc[1] += acc[0];
            acc[2] += acc[1];
            acc[3] += acc[2];
        }

        /* Perform decimation & differentiator stages */
        diff[0] = acc[3] - diffDly[0];
        diffDly[0] = acc[3];
        diff[1] = diff[0] - diffDly[1];
        diffDly[1] = diff[0];
        diff[2] = diff[1] - diffDly[2];
        diffDly[2] = diff[1];
        diff[3] = diff[2] - diffDly[3];
        diffDly[3] = diff[2];

        /* Write output sample */
        *pOutSamp++ = diff[3];
    }
}
