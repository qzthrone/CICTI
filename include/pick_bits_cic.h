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

#ifndef __PICK_BITS_CIC_H__
#define __PICK_BITS_CIC_H__

//#include "data_types.h"

#define CIC_DF  ( 16 )  /* decimation factor for CIC */
#define CIC_NS  ( 4 )   /* number of stages for CIC */

/* Unpacks "left" and "right" 32-bit packed DMA buffers containing output from digital mic. */
/* Performs CIC on unpacked data, DS = 16 & NS = 4. */
void pickBitsCic(
    Uint32 *lData,          /* "left" channel 32-bit packed input data */
    Uint32 *rData,          /* "right" channel 32-bit packed input data */
    Uint16 inDataLen,       /* length of "left" or "right" input data in 32-bit words */
    Int32 *cicState,        /* CIC state. First NS values are integrator state, next NS values are differentiator delay buffer */
    Int32 *outSamps,        /* CIC output samples */
    Uint16 *pNumOutSamps    /* CIC number of output samples */
);

#endif  

/* __PICK_BITS_CIC_H__ */
