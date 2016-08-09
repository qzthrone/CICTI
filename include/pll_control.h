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
#ifndef _PLL_CONTROL_H_
#define _PLL_CONTROL_H_

#include "csl_types.h"

/* PLL frequency setting options -- all options use RTC clock as input */
typedef enum
{
    PLL_FREQ_16P384MHZ = 0, 
    PLL_FREQ_32P768MHZ, 
    PLL_FREQ_40MHZ, 
    PLL_FREQ_50MHZ,
    PLL_FREQ_60MHZ, 
    PLL_FREQ_75MHZ, 
    PLL_FREQ_100MHZ, 
    PLL_FREQ_120MHZ
} EPllFreq;

/* Sets PLL to desired output frequency = 
{32.768, 40, 50, 60, 75, 100, 120} MHZ.
Default is 60 MHz.
Assumes CPU core voltage is set appropriately for requested output frequency. */
CSL_Status pll_sample(
    EPllFreq pllFreq
);

#endif /* _PLL_CONTROL_H_ */
