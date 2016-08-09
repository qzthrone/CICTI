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

#include "stdio.h"
#include "stdlib.h"

// CSLR include files
#include "soc.h"
#include "cslr.h"
#include "cslr_sysctrl.h"

// CSL include files
#include "csl_i2s.h"
#include "csl_dma.h"
#include "csl_intc.h"

// other include files
#include "pll_control.h"
#include "IdleLoop.h"
#include "pick_bits_cic.h"
#include "BlkFirDecim.h"
#include "diggain.h"


#define MAX_LINE_LEN                ( 80 )  /* maximum line length */
char buffer[MAX_LINE_LEN];


/* Circular buffers for capturing output data (debug) */
#if 1
/* Input circular data buffer left */
Uint32 inCircBufLeft[IN_CIRCBUF_LEN];
/* Input circular data buffer right */
Uint32 inCircBufRight[IN_CIRCBUF_LEN];
#endif
#if 0 
/* CIC output circular buffer */
Int32 cicOutCircBuf[CIC_OUT_CIRCBUF_LEN];
#endif
#if 0
/* FIR1 output circular buffer */
Int32 fir1OutCircBuf[FIR1_OUT_CIRCBUF_LEN];
#endif 
#if 0
/* FIR2 output circular buffer */
Int32 fir2OutCircBuf[FIR2_OUT_CIRCBUF_LEN];
#endif

/* Digital gain output circular buffer */
Int16 digGainOutCircBuf[DIGGAIN_OUT_CIRCBUF_LEN];


CSL_I2sHandle    hI2s;
I2S_Config        hwConfig;

CSL_DMA_Handle dmaLeftRxHandle;
CSL_DMA_Handle dmaRightRxHandle;
CSL_DMA_Config dmaConfig;
CSL_DMA_ChannelObj dmaObj0, dmaObj1;
Uint16 dmaFrameCount = 0;

#pragma DATA_ALIGN(i2sDmaReadBufLeft, 2);
#pragma DATA_SECTION(i2sDmaReadBufLeft, ".i2sDmaReadBufLeft")
Uint32 i2sDmaReadBufLeft[I2S_DMA_BUF_LEN];
#pragma DATA_ALIGN(i2sDmaReadBufRight, 2);
#pragma DATA_SECTION(i2sDmaReadBufRight, ".i2sDmaReadBufRight")
Uint32 i2sDmaReadBufRight[I2S_DMA_BUF_LEN];
Uint16 pingPongFlag = 0;

/* CIC output frame */
#pragma DATA_SECTION(cicOutFrame, ".cicOutFrame")
Int32 cicOutFrame[CIC_OUT_FRAME_LEN];

/* CIC state */
Int32 cicState[2*CIC_NS];

/* FIR1 Coefficients (S16Q15) */
#pragma DATA_SECTION(fir1Coefs, ".fir1Coefs")
const Int16 fir1Coefs[FIR1_NUM_COEFS] = 
{     -98,        0,      609,        0,    -2288,        0,     9968,    16386,     
     9968,        0,    -2288,        0,      609,        0,      -98
};

/* Delay line */
#pragma DATA_SECTION(fir1DlyBuf, ".fir1DlyBuf")
Int32 fir1DlyBuf[FIR1_NUM_COEFS+2+1]; // +2 for input samples req'd for 2 output samples computed per outer loop, +1 for index of oldest sample

/* FIR1 Output frame */
#pragma DATA_SECTION(fir1OutFrame, ".fir1OutFrame")
Int32 fir1OutFrame[FIR1_OUT_FRAME_LEN];

/* FIR2 Coefficients (S16Q15) */
#pragma DATA_SECTION(fir2Coefs, ".fir2Coefs")
const Int16 fir2Coefs[FIR2_NUM_COEFS] = 
{      -4,        2,       10,       -3,      -27,       -2,       51,       15,
      -89,      -50,      135,      114,     -185,     -222,      226,      386,    
     -241,     -623,      198,      947,      -56,    -1396,     -266,     2043,
      959,    -3164,    -2809,     6281,    16481,    16481,     6281,    -2809,    
    -3164,      959,     2043,     -266,    -1396,      -56,      947,      198,    
     -623,     -241,      386,      226,     -222,     -185,      114,      135,    
      -50,      -89,       15,       51,       -2,      -27,       -3,       10,        
        2,       -4
};

/* Delay line */
#pragma DATA_SECTION(fir2DlyBuf, ".fir2DlyBuf")
Int32 fir2DlyBuf[FIR2_NUM_COEFS+2+1]; // +2 for input samples req'd for 2 output samples computed per outer loop, +1 for index of oldest sample

/* FIR2 Output frame */
#pragma DATA_SECTION(fir2OutFrame, ".fir2OutFrame")
Int32 fir2OutFrame[FIR2_OUT_FRAME_LEN];

/* Digital gain output frame */
#pragma DATA_SECTION(digGainOutFrame, ".digGainOutFrame")
Int16 digGainOutFrame[DIGGAIN_OUT_FRAME_LEN];

//#define DIGGAIN ( (Uint16)5<<8 ) /* 1.0 (0 dB) in U16Q8 */
#define DIGGAIN ( (Uint16)10<<8 ) /* 10.0 (20 dB) in U16Q8 */
//#define DIGGAIN ( (Uint16)0x1f9f ) /* 31.6228 (30 dB) in U16Q8 */
//#define DIGGAIN ( (Uint16)100<<8 ) /* 100.0 (40 dB) in U16Q8 */

// Clock gating for all peripherals
void ClockGatingAll(void);

// DSP LDO Switch
// mode:    105 - set DSP LDO to 1.05V  
//            130 - set DSP LDO to 1.3V
//            other - no change
void DspLdoSwitch(int mode);

// USB LDO Switch
// mode:    0 - disable USB LDO  
//          1 - enable USB LDO
//          other - no change
void UsbLdoSwitch(int mode);

// GI2S and DMA initialization
CSL_Status I2sDmaInit(void);

// user defined algorithm
void UserAlgorithm(void);

// Put CPU in idle
void UserIdle(void);

// DMA ISR
interrupt void DmaIsr(void);

void main(void)
{

    CSL_Status status;
    
    printf("Start the IdleLoop\n");
    
    /* Clock gate all peripherals */
    ClockGatingAll();
    
    /* Set DSP LDO to desired output voltage */
    DspLdoSwitch(DSP_LDO);

    /* Set the PLL to pll_mhz */
    status = pll_sample(PLL_MHZ);
    if (status != CSL_SOK)
    {
        printf("ERROR: Unable to set PLL\n");
        exit(1);
    }
    
    /* Turn off the USB LDO */
    UsbLdoSwitch(0);

    /* Initialize I2S and DMA engine */
    status = I2sDmaInit();
    if (status != CSL_SOK)
    {
        printf("ERROR: Unable to initialize I2S and DMA\n");
        exit(1);
    }
    
    /* SP0 Mode 1 (I2S0 and GP[5:4]) */
    CSL_FINST(CSL_SYSCTRL_REGS->EBSR, SYS_EBSR_SP0MODE, MODE1);
    /* SP1 Mode 1 (I2S1 and GP[11:10]) */
    CSL_FINST(CSL_SYSCTRL_REGS->EBSR, SYS_EBSR_SP1MODE, MODE1);
    /* PP Mode 1 (SPI, GPIO[17:12], UART, and I2S2) */
    CSL_FINST(CSL_SYSCTRL_REGS->EBSR, SYS_EBSR_PPMODE, MODE1);

    /* Configure PCGCR1 & PCGCR2 */
    CSL_FINS(CSL_SYSCTRL_REGS->PCGCR2, SYS_PCGCR2_DMA1CG, CSL_SYS_PCGCR2_DMA1CG_DISABLED);
    CSL_FINS(CSL_SYSCTRL_REGS->PCGCR2, SYS_PCGCR2_DMA2CG, CSL_SYS_PCGCR2_DMA2CG_DISABLED);
    CSL_FINS(CSL_SYSCTRL_REGS->PCGCR2, SYS_PCGCR2_DMA3CG, CSL_SYS_PCGCR2_DMA3CG_DISABLED);

    /* Set CPUI, DPORTI, IPORTI, XPORT and HWAI in ICR */
    *(volatile ioport Uint16 *)(0x0001) = (0x000E | 1<<0 | 1<<5 | 1<<6 | 1<<8 | 1<<9);

    while (1)
    {
        /* Perform your algorithm here */
        UserAlgorithm();

#if 1
        /* Set CPU to Idle */
        UserIdle();
#endif
    }
}

// Clock gating for all peripherals
void ClockGatingAll(void)
{
    Uint16 pcgcr_value;
    //Uint16 clkstop_value;
    //Uint16 status;

    // enable the MPORT and disable HWA
    *(volatile ioport unsigned int *)0x0001 = 0x020E;
    asm("   idle");
    
    // set PCGCR1
    pcgcr_value = 0; 
    // clock gating SPI
    pcgcr_value |= CSL_FMKT(SYS_PCGCR1_SPICG, DISABLED);
    // clock gating SD/MMC
    pcgcr_value |= CSL_FMKT(SYS_PCGCR1_MMCSD0CG, DISABLED);
    pcgcr_value |= CSL_FMKT(SYS_PCGCR1_MMCSD1CG, DISABLED);

    // clock gating UART
    pcgcr_value |= CSL_FMKT(SYS_PCGCR1_UARTCG, DISABLED);


    // clock gating EMIF
    pcgcr_value |= CSL_FMKT(SYS_PCGCR1_EMIFCG, DISABLED);

    // clock gating I2S I2S 0
    pcgcr_value |= CSL_FMKT(SYS_PCGCR1_I2S0CG, DISABLED);
    // clock gating I2S I2S 1
   // pcgcr_value |= CSL_FMKT(SYS_PCGCR1_I2S1CG, DISABLED);
    // clock gating I2S I2S 2
    pcgcr_value |= CSL_FMKT(SYS_PCGCR1_I2S2CG, DISABLED);
    // clock gating I2S I2S 3
    pcgcr_value |= CSL_FMKT(SYS_PCGCR1_I2S3CG, DISABLED);

    // clock gating DMA0
    pcgcr_value |= CSL_FMKT(SYS_PCGCR1_DMA0CG, DISABLED);

        // clock gating Timer 0
    pcgcr_value |= CSL_FMKT(SYS_PCGCR1_TMR0CG, DISABLED);
    // clock gating Timer 1
    pcgcr_value |= CSL_FMKT(SYS_PCGCR1_TMR1CG, DISABLED);
    // clock gating Timer 2
    pcgcr_value |= CSL_FMKT(SYS_PCGCR1_TMR2CG, DISABLED);

    // clock gating I2C
    pcgcr_value |= CSL_FMKT(SYS_PCGCR1_I2CCG, DISABLED);

        // write to PCGCR1
    CSL_FSET(CSL_SYSCTRL_REGS->PCGCR1, 15, 0, pcgcr_value);
    
    // set PCGCR2
    pcgcr_value = 0; 
    // clock gating LCD
 //  pcgcr_value |= CSL_FMKT(SYS_PCGCR2_LCDCG, DISABLED);

    // clock gating SAR
    pcgcr_value |= CSL_FMKT(SYS_PCGCR2_SARCG, DISABLED);

    // clock gating DMA1
    pcgcr_value |= CSL_FMKT(SYS_PCGCR2_DMA1CG, DISABLED);
    // clock gating DMA2
    pcgcr_value |= CSL_FMKT(SYS_PCGCR2_DMA2CG, DISABLED);
    // clock gating DMA3
    pcgcr_value |= CSL_FMKT(SYS_PCGCR2_DMA3CG, DISABLED);

    // clock gating analog registers
    pcgcr_value |= CSL_FMKT(SYS_PCGCR2_ANAREGCG, DISABLED);

        // clock gating USB
    pcgcr_value |= CSL_FMKT(SYS_PCGCR2_USBCG, DISABLED);

    // write to PCGCR2
    CSL_FSET(CSL_SYSCTRL_REGS->PCGCR2, 15, 0, pcgcr_value);
    
    // turn off the XF
    // set bit 13 of ST1_55 to 0
    asm("    bit(ST1, #ST1_XF) = #0");

#if 0
    // set all GPIO pins to be output and low to save power
    // set the GPIO pin 0 - 15 to output, set SYS_GPIO_DIR0 (0x1C06) bit 0 and 15 to 1 
    *(volatile ioport unsigned int *)(0x1C06) = 0xFFFF;
    // set the GPIO pin 16 - 31 to output, set SYS_GPIO_DIR1 (0x1C07) bit 0 and 15 to 1 
    *(volatile ioport unsigned int *)(0x1C07) = 0xFFFF;
    
    // set the GPIO 0 - 15 to 0, set SYS_GPIO_DATAOUT0 (0x1C0A) bit 0 and 15 to 0
    *(volatile ioport unsigned int *)(0x1C0A) = 0x0000;
    // set the GPIO 16 - 31 to 0, set SYS_GPIO_DATAOUT1 (0x1C0B) bit 0 and 15 to 0
    *(volatile ioport unsigned int *)(0x1C0B) = 0x0000;
#endif
    

    return;
}

// DSP LDO Switch
// mode:    105 - set DSP LDO to 1.05V  
//            130 - set DSP LDO to 1.3V
//            other - no change
void DspLdoSwitch(int mode)
{
    if (mode==130)
    {
        /* enable the Analog Register only */
        *((ioport volatile unsigned int *)0x1C03) &= ~(0x0040);
        // set DSP LDO to 1.05V (clear bit 1)
        *(volatile ioport unsigned int *)(0x7004) &= 0xFFFD;
        /* disable the Analog Register only */
        *((ioport volatile unsigned int *)0x1C03) |= 0x0040;
    }
    
    if (mode==105)
    {
        /* enable the Analog Register only */
        *((ioport volatile unsigned int *)0x1C03) &= ~(0x0040);
        // set DSP LDO to 1.05V (set bit 1)
        *(volatile ioport unsigned int *)(0x7004) |= 0x0002;
        /* disable the Analog Register only */
        *((ioport volatile unsigned int *)0x1C03) |= 0x0040;
    }
}

// USB LDO Switch
// mode:    0 - disable USB LDO  
//          1 - enable USB LDO
//          other - no change
void UsbLdoSwitch(int mode)
{
    if (mode==0)
    {
        /* enable the Analog Register only */
        *((ioport volatile unsigned int *)0x1C03) &= ~(0x0040);
        // disable USB LDO (clear bit 0)
        *(volatile ioport unsigned int *)(0x7004) &= 0xFFFE;
        /* disable the Analog Register only */
        *((ioport volatile unsigned int *)0x1C03) |= 0x0040;
    }
    
    if (mode==1)
    {
        /* enable the Analog Register only */
        *((ioport volatile unsigned int *)0x1C03) &= ~(0x0040);
        // enable USB LDO (set bit 0)
        *(volatile ioport unsigned int *)(0x7004) |= 0x0001;
        /* disable the Analog Register only */
        *((ioport volatile unsigned int *)0x1C03) |= 0x0040;
    }
}

// Address for interrupt vector table
extern void VECSTART(void); // defined in vector table
CSL_IRQ_Dispatch     dispatchTable;
// GI2S and DMA initialization
CSL_Status I2sDmaInit(void)
{
    CSL_Status status;
    Uint16 ifrValue;

    // set up DMA INT and ISR
    // set up dispatch table
    status = IRQ_init(&dispatchTable, 0);
    if(status != CSL_SOK)
    {
        return (status);
    } else
    {
        printf ("INT Module Configured successfully\n");
    }

    /* Configure and Enable the DMA interrupt */
    IRQ_globalDisable();

    /* Clear any pending interrupts */
    IRQ_clearAll();

    /* Disable all the interrupts */
    IRQ_disableAll();
    
    // to set the interrupt vector table address
    status = IRQ_setVecs((Uint32)&VECSTART);
    if(status != CSL_SOK)
    {
        return (status);
    } else
    {
        printf ("Get interrupt vector table successfully\n");
    }
    
    // clear pending DMA interrupt
    IRQ_clear(DMA_EVENT);
    
    // clear the DMA interrupt
    ifrValue = CSL_SYSCTRL_REGS->DMAIFR;
    CSL_SYSCTRL_REGS->DMAIFR |= ifrValue;

    // plug in DMA ISR
    IRQ_plug(DMA_EVENT, &DmaIsr);
    
    // enable DMA interrupt
    IRQ_enable(DMA_EVENT);

    // enable global interrupt
    IRQ_globalEnable();


    // DMA engine initialization
    // Open the device with instance 0 (digital mic connected to I2S0 on C5515 EVM)
    hI2s = I2S_open(I2S_INSTANCE0, DMA_POLLED, I2S_CHAN_STEREO);
    if(NULL == hI2s)
    {
        status = CSL_ESYS_FAIL;
        return (status);
    }
    else
    {
        printf ("I2S Module Instance opened successfully\n");
    }

    /** Set the value for the configure structure                */
    hwConfig.dataFormat     = I2S_DATAFORMAT_LJUST;
    hwConfig.i2sMode        = I2S_MASTER;
    hwConfig.wordLen        = I2S_WORDLEN_32;
    hwConfig.signext        = I2S_SIGNEXT_ENABLE;   // don't care since 32-bit wordlength
    hwConfig.datapack       = I2S_DATAPACK_DISABLE; // don't care since 32-bit wordlength
    hwConfig.datadelay      = I2S_DATADELAY_ONEBIT;
    hwConfig.clkPol         = I2S_RISING_EDGE;      // receive data is sampled on the rising edge
    hwConfig.fsPol          = I2S_FSPOL_LOW;
    hwConfig.loopBackMode   = I2S_LOOPBACK_DISABLE;
    hwConfig.dataType       = I2S_STEREO_ENABLE;
    //hwConfig.clkDiv         = I2S_CLKDIV32;         // 32.768e6/32 = 1.024e6 (clock to digital mic)
    hwConfig.clkDiv         = I2S_CLKDIV16;         // 16.384e6/16 = 1.024e6 (clock to digital mic)
    hwConfig.fsDiv          = I2S_FSDIV64;          // 64 bit clocks per frame, or 32 bit clocks per channel */
    hwConfig.FError         = I2S_FSERROR_DISABLE;
    hwConfig.OuError        = I2S_OUERROR_DISABLE;

    /** Configure hardware registers                            */
    status = I2S_setup(hI2s, &hwConfig);
    if(status != CSL_SOK)
    {
        return (status);
    }
    else
    {
        printf ("I2S Module Configured successfully\n");
    }

    // Configure DMA0 channel 0 for I2S0 channel read
    dmaConfig.pingPongMode = CSL_DMA_PING_PONG_ENABLE;
    dmaConfig.autoMode     = CSL_DMA_AUTORELOAD_ENABLE;
    dmaConfig.burstLen     = CSL_DMA_TXBURST_1WORD;
    dmaConfig.trigger      = CSL_DMA_EVENT_TRIGGER;
    dmaConfig.dmaEvt       = CSL_DMA_EVT_I2S0_RX;
    dmaConfig.dmaInt       = CSL_DMA_INTERRUPT_ENABLE;
    dmaConfig.chanDir      = CSL_DMA_READ;
    dmaConfig.trfType      = CSL_DMA_TRANSFER_IO_MEMORY;
    dmaConfig.dataLen      = I2S_DMA_BUF_LEN*4; // bytes in 32-bit word
    dmaConfig.srcAddr      = (Uint32)0x2828;
    dmaConfig.destAddr     = (Uint32)i2sDmaReadBufLeft;

    // DMA initialization
    status = DMA_init();
    if (status != CSL_SOK)
    {
        printf("DMA_init() Failed \n");
        dmaLeftRxHandle = NULL;
        dmaRightRxHandle = NULL;
        return status;
    }

    // Open DMA0 channel 0 for I2S0 channel read
    dmaLeftRxHandle = DMA_open(CSL_DMA_CHAN0, &dmaObj0, &status);
    if (dmaLeftRxHandle == NULL)
    {
        printf("DMA_open CH0 Failed \n");
        dmaLeftRxHandle = NULL;
    }

    status = DMA_config(dmaLeftRxHandle, &dmaConfig);
    if (status != CSL_SOK)
    {
        printf("DMA_config CH0 Failed \n");
        dmaLeftRxHandle = NULL;
    }

   /* Configure DMA0 channel 1 for I2S0 channel read */
    dmaConfig.pingPongMode = CSL_DMA_PING_PONG_ENABLE;
    dmaConfig.autoMode     = CSL_DMA_AUTORELOAD_ENABLE;
    dmaConfig.burstLen     = CSL_DMA_TXBURST_1WORD;
    dmaConfig.trigger      = CSL_DMA_EVENT_TRIGGER;
    dmaConfig.dmaEvt       = CSL_DMA_EVT_I2S0_RX;
    dmaConfig.dmaInt       = CSL_DMA_INTERRUPT_ENABLE;
    dmaConfig.chanDir      = CSL_DMA_READ;
    dmaConfig.trfType      = CSL_DMA_TRANSFER_IO_MEMORY;
    dmaConfig.dataLen      = I2S_DMA_BUF_LEN*4; // bytes in 32-bit word
    dmaConfig.srcAddr      = (Uint32)0x282C;
    dmaConfig.destAddr     = (Uint32)i2sDmaReadBufRight;

    // Open DMA0 channel 1 for I2S0 channel read
    dmaRightRxHandle = DMA_open(CSL_DMA_CHAN1, &dmaObj1, &status);
    if (dmaRightRxHandle == NULL)
    {
        printf("DMA_open CH1 Failed \n");
        dmaRightRxHandle = NULL;
    }

    status = DMA_config(dmaRightRxHandle, &dmaConfig);
    if (status != CSL_SOK)
    {
        printf("DMA_config CH1 Failed \n");
        dmaRightRxHandle = NULL;
    }

    // enable I2S
    I2S_transEnable(hI2s, TRUE);
    
    // Start left Rx DMA
    status = DMA_start(dmaLeftRxHandle);
    if (status != CSL_SOK)
    {
        printf("I2S Dma Left Failed!!\n");
        return (status);
    }
    
    // Start right Rx DMA
    status = DMA_start(dmaRightRxHandle);
    if (status != CSL_SOK)
    {
        printf("I2S Dma Right Failed!!\n");
        return (status);
    }

    return CSL_SOK;
}

volatile long LoopCount = 0;
// user defined algorithm
void UserAlgorithm(void)
{
    volatile int i, numFrame, offset;
    Uint16 numOutSamps;

    if (dmaFrameCount >= 2)
    {
        /* Determine which frame to use ping or pong */
        offset = pingPongFlag*IN_FRAME_LEN_PER_CH;

        /* Perform CIC */
        pickBitsCic(&i2sDmaReadBufLeft[offset], &i2sDmaReadBufRight[offset], IN_FRAME_LEN_PER_CH, cicState, cicOutFrame, &numOutSamps);

        /* Compute FIR1 output */
        blkFirDecim2(cicOutFrame, (Int16 *)fir1Coefs, fir1OutFrame, fir1DlyBuf, CIC_OUT_FRAME_LEN, FIR1_NUM_COEFS);

        /* Compute FIR2 output */
        blkFirDecim2(fir1OutFrame, (Int16 *)fir2Coefs, fir2OutFrame, fir2DlyBuf, FIR1_OUT_FRAME_LEN, FIR2_NUM_COEFS);

        /* Apply digital gain */
       appDiggain(fir2OutFrame, DIGGAIN, digGainOutFrame, FIR2_OUT_FRAME_LEN);

        /* Get current frame number */
        numFrame = LoopCount%NUM_FRAMES_PER_CIRCBUF;

        /* Write digital gain output to circular buffer */
        for (i=0; i<DIGGAIN_OUT_FRAME_LEN; i++)
        {
            digGainOutCircBuf[numFrame*DIGGAIN_OUT_FRAME_LEN+i] = digGainOutFrame[i];
        }
       
#if 0 // write input data into circular buffers
        /* Copy the current frame from ping or pong frame into input circular buffers */
        for (i=0; i<IN_FRAME_LEN_PER_CH; i++)
        {
            /* Write left channel */
            inCircBufLeft[numFrame*IN_FRAME_LEN_PER_CH+i] = i2sDmaReadBufLeft[offset+i];
            /* Write right channel */
            inCircBufRight[numFrame*IN_FRAME_LEN_PER_CH+i] = i2sDmaReadBufRight[offset+i];
        }
#endif

#if 0 // write CIC output into circular buffers
        for (i=0; i<CIC_OUT_FRAME_LEN; i++)
        {
            cicOutCircBuf[numFrame*CIC_OUT_FRAME_LEN+i] = cicOutFrame[i];
        }
#endif        

#if 0 // write FIR1 output into circular buffers
        for (i=0; i<FIR1_OUT_FRAME_LEN; i++)
        {
            fir1OutCircBuf[numFrame*FIR1_OUT_FRAME_LEN+i] = fir1OutFrame[i];
        }
#endif        

#if 0 // write FIR2 output into circular buffers
        /* Copy the current frame from ping or pong frame into input circular buffers */
        for (i=0; i<FIR2_OUT_FRAME_LEN; i++)
        {
            fir2OutCircBuf[numFrame*FIR2_OUT_FRAME_LEN+i] = fir2OutFrame[i];
        }
#endif        

#if 0 // debug -- stop DMAs on frame boundary
        /* Stop DMAs to get consistent DMA transfers from digital mic */
        if ((pingPongFlag == 0) && (LoopCount > 500)) // 500*20e-3 = 10 sec. 
        {
            *((ioport volatile unsigned int *)0x0C05) &= ~0x8000; // disable DMA
            *((ioport volatile unsigned int *)0x0C25) &= ~0x8000; // disable DMA
            while(1);
        }
#endif

        /* Toggle ping/pong flag */
        pingPongFlag ^= 1;
        
        /* Clear DMA frame count */
        dmaFrameCount = 0;

        LoopCount++;
    }
}

// Put CPU in idle
void UserIdle(void)
{
    /* Execute idle instruction */
    asm("    idle");
}

Uint32 dmaIntCount = 0;
// DMA ISR
interrupt void DmaIsr(void)
{
    Uint16 ifrValue;

    // clear the DMA interrupt
    ifrValue = CSL_SYSCTRL_REGS->DMAIFR;
    CSL_SYSCTRL_REGS->DMAIFR = ifrValue;
    
    // wait for the DMA0 CH0 transfer to complete
    // if left frame is in
    if (ifrValue & 0x1)    
    {
        dmaFrameCount++;
    }
    
    // wait for the DMA0 CH1 transfer to complete
    // if the right frame is in
    if (ifrValue & 0x2)
    {
        dmaFrameCount++;
    }    

    dmaIntCount++;
}
