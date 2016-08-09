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

#ifndef IDLELOOP_H_
#define IDLELOOP_H_

#include "pll_control.h"

/* DSP LDO setting */
#define DSP_LDO                     ( 105 )
//#define DSP_LDO                     ( 130 )

/* PLL output MHz setting */
#define PLL_MHZ                     ( PLL_FREQ_16P384MHZ )
//#define PLL_MHZ                     ( PLL_FREQ_32P768MHZ )

#define NUM_INSAMP_PER_MS           ( 1024 )                // 1-bit samples per msec
#define NUM_IN32BW_PER_MS           ( NUM_INSAMP_PER_MS/32 )    // 32-bit input words per msec
#define NUM_IN32BW_PER_MS_PER_CH    ( NUM_IN32BW_PER_MS/2 )     // 32-bit input words per msec/channel

#define NUM_MS_PER_FRAME            ( 20 )                  // msec per frame

#define IN_FRAME_LEN_PER_CH         ( NUM_IN32BW_PER_MS_PER_CH*NUM_MS_PER_FRAME)    // input frame length per channel

#define FIR_DF                      ( 2 )   // FIR1/FIR2 decimation factor

#define CIC_OUT_FRAME_LEN           ( NUM_INSAMP_PER_MS*NUM_MS_PER_FRAME / CIC_DF ) // output frame length
#define FIR1_OUT_FRAME_LEN          ( CIC_OUT_FRAME_LEN / FIR_DF )  // FIR1 output frame length
#define FIR2_OUT_FRAME_LEN          ( FIR1_OUT_FRAME_LEN / FIR_DF ) // FIR2 output frame length
#define DIGGAIN_OUT_FRAME_LEN       ( FIR2_OUT_FRAME_LEN )  // digital gain output frame length

#define FIR1_NUM_COEFS              ( 15 )  // FIR1 number of coefficients
#define FIR2_NUM_COEFS              ( 58 )  // FIR2 number of coefficients


#define NUM_FRAMES_PER_CIRCBUF      ( 10 )  // frames in circular buffer

#define IN_CIRCBUF_LEN              ( NUM_IN32BW_PER_MS_PER_CH*NUM_MS_PER_FRAME*NUM_FRAMES_PER_CIRCBUF ) // input circular buffer length
#define CIC_OUT_CIRCBUF_LEN         ( CIC_OUT_FRAME_LEN*NUM_FRAMES_PER_CIRCBUF)
#define FIR1_OUT_CIRCBUF_LEN        ( FIR1_OUT_FRAME_LEN*NUM_FRAMES_PER_CIRCBUF ) 
#define FIR2_OUT_CIRCBUF_LEN        ( FIR2_OUT_FRAME_LEN*NUM_FRAMES_PER_CIRCBUF )
#define DIGGAIN_OUT_CIRCBUF_LEN     ( FIR2_OUT_CIRCBUF_LEN )

// ping pong buffer size 32 bits per sample, one frame for ping, one frame for pong
// *2 for ping/pong
#define I2S_DMA_BUF_LEN             ( IN_FRAME_LEN_PER_CH*2 )

#endif /*IDLELOOP_H_*/
