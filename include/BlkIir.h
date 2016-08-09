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
#ifndef __BLK_IIR_H__
#define __BLK_IIR_H__

#include "data_types.h"

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
);

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
);

#endif /* __BLK_IIR_H__ */
