;============================================================================
; Copyright (c) 2016 Texas Instruments Incorporated.
;
;  Redistribution and use in source and binary forms, with or without
;  modification, are permitted provided that the following conditions
;  are met:
;
;    Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
;
;    Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the
;    distribution.
;
;    Neither the name of Texas Instruments Incorporated nor the names of
;    its contributors may be used to endorse or promote products derived
;    from this software without specific prior written permission.
;
;  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
;  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
;  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
;  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
;  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
;  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
;  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
;  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
;  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
;  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;
;===============================================================================
; Function:    _appDiggain
; Processor:   C55xx, Rev. 3
; Description: 
;   Applies digital gain and saturates output.
;   S18Q16 input data, S16Q15 output data.
;   U16Q8 digital gain.
;
;   x cycle inner loop.
;
;   C-callable.
;   Mnemonic assembly.
;
; Usage:    void blkFirDecim2(
;               Int32   *inSamps,      -> XAR0
;               Uint16  diggain,       -> T0
;               Int16   *outSamps,     -> XAR1
;               Uint16  numInSamps     -> T1
;           );
;
;
; NOTE: additional optimizations possible, but low MHz contributor.
;****************************************************************

                .C54CM_off                      ; enable assembler for C54CM=0
                .ARMS_off                       ; enable assembler for ARMS=0
                .CPL_on                         ; enable assembler for CPL=1

                .mmregs                         ; enable mem mapped register names

                .def    _appDiggain

; Stack frame
; -----------
RET_ADDR_SZ     .set    1                       ; return address
REG_SAVE_SZ     .set    0                       ; save-on-entry registers saved
FRAME_SZ        .set    1                       ; local variables
ARG_BLK_SZ      .set    0                       ; argument block
PARAM_OFFSET    .set    ARG_BLK_SZ + FRAME_SZ + REG_SAVE_SZ + RET_ADDR_SZ   ; offset to function arguments on stack

; Local variables
; ---------------
                .asg    2, digGain

; Register usage
; --------------
                .asg    CDP, x_ptr              ; input sample pointer
                .asg    AR1, r_ptr              ; output sample pointer

                .asg    XCDP, xx_ptr            ; extended input sample pointer

                .asg    BRC0, outer_cnt         ; outer loop count

QUANT_TRUNC     .set    0                       ; truncate
QUANT_RND       .set    1                       ; round according to rounding mode
QUANT_MODE      .set    QUANT_RND


                .text
_appDiggain:

;
; Save any save-on-entry registers that are used
;----------------------------------------------------------------

;
; Allocate the local frame and argument block
;----------------------------------------------------------------
                AADD        #-(ARG_BLK_SZ + FRAME_SZ), SP 

;
; Save entry values for later
;----------------------------------------------------------------
                MOV         T0, *SP(digGain)

;
; Configure the status registers as needed
;----------------------------------------------------------------
                AND         #001FFh, mmap(ST0_55)   ; clear ACOV[0-3], TC[1-2], and C

                OR          #04540h, mmap(ST1_55)   ; set CPL, M40, SXMD, and FRCT
                ;AND         #0F9DFh, mmap(ST1_55)   ; clear M40, SATD, 54CM 

                BCLR        ARMS                    ; clear ARMS

                ;AND         #0FCDDh, mmap(ST3_55)   ; clear SATA, SMUL ; note -- must always write 1100b to bits 11-8, 0b to bit 4


;
; Setup passed parameters in their destination registers
;----------------------------------------------------------------

; x pointer - passed in its destination register
                AMOV        XAR0, xx_ptr

; r pointer - already passed in its destination register


;
; Setup loop counts
;----------------------------------------------------------------
                MOV			T1, AC0					; AC0 = nx (nx unsigned, constrained to < 32768 since SXM is enabled)
                SUB         #1, AC0                 ; AC0 = nx-1
                MOV         AC0, mmap(outer_cnt)    ; outer_cnt = nx-1

    
;
; Start of outer loop
;----------------------------------------------------------------
                RPTBLOCAL   loop0-1                 ; outer loop nx iterations

                MPY         *SP(#digGain), *x_ptr+, AC1          ; Low x High
                MPY         *SP(#digGain), uns(*x_ptr+), AC0     ; Low x Low
                ADD         AC1 << #16, AC0
                SFTS        AC0, #6
                .if (QUANT_MODE == QUANT_TRUNC)
                SAT         AC0
                .else
                SATR        AC0
                .endif

                ; Store result to memory
                MOV         HI(AC0), *r_ptr+        ; store S16Q15 value to memory
loop0: ; end of outer loop


;
; Restore status regs to expected C-convention values as needed
;----------------------------------------------------------------
                BCLR        M40                     ; clear M40
                BCLR        FRCT                    ; clear FRCT

                AND         #0FE00h, mmap(ST2_55)   ; clear CDPLC and AR[7-0]LC
                BSET        ARMS                    ; set SMUL

                ;BSET        SMUL                    ; set SMUL

;
; Deallocate the local frame and argument block
;----------------------------------------------------------------
                AADD        #(ARG_BLK_SZ + FRAME_SZ), SP 

;
; Restore any save-on-entry registers that are used
;----------------------------------------------------------------

;
; Return to calling function
;----------------------------------------------------------------
                RET                             ; return to calling function
