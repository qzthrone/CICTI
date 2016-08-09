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
#include "csl_pll.h"
#include "csl_general.h"
#include "csl_pllAux.h"
#include "pll_control.h"

PLL_Obj pllObj;
PLL_Config pllCfg1;

PLL_Handle hPll;

const PLL_Config pllCfg_16p384MHz   = {0x83E4, 0x8000, 0x0806, 0x0201};
const PLL_Config pllCfg_32p768MHz   = {0x87CC, 0x8000, 0x0806, 0x0201};
const PLL_Config pllCfg_40MHz       = {0x8988, 0x8000, 0x0806, 0x0201};
const PLL_Config pllCfg_50MHz       = {0x8BE8, 0x8000, 0x0806, 0x0201};
const PLL_Config pllCfg_60MHz       = {0x8724, 0x8000, 0x0806, 0x0000};
const PLL_Config pllCfg_75MHz       = {0x88ED, 0x8000, 0x0806, 0x0000};
const PLL_Config pllCfg_100MHz      = {0x8BE8, 0x8000, 0x0806, 0x0000};
const PLL_Config pllCfg_120MHz      = {0x8E4A, 0x8000, 0x0806, 0x0000};

PLL_Config *pConfigInfo;

/* Sets PLL to desired output frequency = 
{32.768, 40, 50, 60, 75, 100, 120} MHZ.
Default is 60 MHz.
Assumes CPU core voltage is set appropriately for requested output frequency. */
CSL_Status pll_sample(
    EPllFreq pllFreq
)
{
    CSL_Status status;
    volatile int i;

    status = PLL_init(&pllObj, CSL_PLL_INST_0);
    if (status != CSL_SOK)
    {
        return status;
    }

    hPll = (PLL_Handle)(&pllObj);

    PLL_reset(hPll);

    status = PLL_bypass(hPll);
    if (status != CSL_SOK)
    {
        return status;
    }

    /* Configure the PLL */
    switch (pllFreq)
    {
    case PLL_FREQ_16P384MHZ:
        pConfigInfo = (PLL_Config *)&pllCfg_16p384MHz;
        break;

    case PLL_FREQ_32P768MHZ:
        pConfigInfo = (PLL_Config *)&pllCfg_32p768MHz;
        break;

    case PLL_FREQ_40MHZ:
        pConfigInfo = (PLL_Config *)&pllCfg_40MHz;
        break;
        
    case PLL_FREQ_50MHZ:
        pConfigInfo = (PLL_Config *)&pllCfg_50MHz;
        break;

    case PLL_FREQ_60MHZ:
        pConfigInfo = (PLL_Config *)&pllCfg_60MHz;
        break;

    case PLL_FREQ_75MHZ:
        pConfigInfo = (PLL_Config *)&pllCfg_75MHz;
        break;

    case PLL_FREQ_100MHZ:
        pConfigInfo = (PLL_Config *)&pllCfg_100MHz;
        break;

    case PLL_FREQ_120MHZ:
        pConfigInfo = (PLL_Config *)&pllCfg_120MHz;
        break;
    
    default:
        pConfigInfo = (PLL_Config *)&pllCfg_60MHz;
    }
    
    // set the PLL using CSL function
    status = PLL_config (hPll, pConfigInfo);
    if (status != CSL_SOK)
    {
        return(status);
    }

    // read the PLL configure back
    status = PLL_getConfig(hPll, &pllCfg1);
    if (status != CSL_SOK)
    {
        return status;
    }

    /* Wait for PLL to stabilize */
    for (i=0; i<100; i++);

    // enable the PLL
    status = PLL_enable(hPll);
    if (status != CSL_SOK)
    {
        return status;
    }

    return CSL_SOK;
}

