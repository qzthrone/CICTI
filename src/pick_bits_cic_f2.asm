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
; Function:    pickBitsCic
; Processor:   C55xx, Rev. 3
; Description: 
;   Unpacks "left" and "right" 32-bit packed DMA buffers 
;   containing output from digital mic.
;   Performs CIC on unpacked data, DS = 16 & NS = 4.
;
;   x cycle inner loop.
;
;   C-callable.
;   Mnemonic assembly.
;
; Usage:    void pick_bits_cic(
;               Uint32 *lData,          -> XAR0
;               Uint32 *rData,          -> XAR1
;               Uint16 inDataLen,       -> T0
;               Int32 *cicState,        -> XAR2
;               Int32 *outSamps,        -> XAR3
;               Uint16 *pNumOutSamps    -> XAR4
;           );
;
;****************************************************************

                .C54CM_off                      ; enable assembler for C54CM=0
                .ARMS_off                       ; enable assembler for ARMS=0
                .CPL_on                         ; enable assembler for CPL=1

                .mmregs                         ; enable mem mapped register names

                .def    _pickBitsCic

                .include "pick_bits_cic.inc"

; Stack frame
; -----------
RET_ADDR_SZ     .set    1                       ; return address
REG_SAVE_SZ     .set    0                       ; save-on-entry registers saved
FRAME_SZ        .set    0                       ; local variables
ARG_BLK_SZ      .set    0                       ; argument block
PARAM_OFFSET    .set    ARG_BLK_SZ + FRAME_SZ + REG_SAVE_SZ + RET_ADDR_SZ   ; offset to function arguments on stack

; Local variables
; ---------------

; Register usage
; --------------
                .asg    XAR2, cicState_xptr     ; extended pointer to CIC state
                .asg    AR0, lData_ptr          ; pointer to "left" input data
                .asg    AR1, rData_ptr          ; pointer to "right" input data
                .asg    AR3, outSamps_ptr       ; pointer to output samples
                .asg    AR4, numOutSamps_ptr    ; pointer to number of output samples
                .asg    BRC0, loop0_cnt         ; loop0 count
                .asg    BRC1, loop1_cnt         ; loop1 count


                .text
_pickBitsCic:


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

;
; Configure the status registers as needed
;----------------------------------------------------------------
                AND         #001FFh, mmap(ST0_55)   ; clear ACOV[0-3], TC[1-2], and C

                ;OR          #04540h, mmap(ST1_55)   ; set CPL, M40, SXMD, FRCT
                ;AND         #0FDDFh, mmap(ST1_55)   ; clear SATD, C54CM

                ;AND         #07A00h, mmap(ST2_55)   ; clear ARMS, RDM, CDPLC, AR[0-7]LC
                BCLR        ARMS                    ; clear ARMS

                ;AND         #0FCDDh, mmap(ST3_55)   ; clear SATA, SMUL ; note -- must always write 1100b to bits 11-8, 0b to bit 4

;
; Compute number of output samples
;----------------------------------------------------------------
                MOV         T0, AC0                     ; AC0 = inDataLen
                MOV         AC0 << #2, *numOutSamps_ptr ; *pNumOutSamps = inDataLen<<2

;
; Loop setup
;----------------------------------------------------------------
                ; Initialize loop0 (outer) count
                ; Initialize loop1 (inner) count
                SUB         #1, T0                  ; T0 = inBufLen-1
             || MOV         #(DS-1), loop1_cnt
                MOV         T0, mmap(@loop0_cnt)    ; loop0_cnt = inBufLen-1

;
; Output sample loop
;----------------------------------------------------------------
                RPTB        loop0-1

;
; Unpack current "left" word
;----------------------------------------------------------------
                ;
                ; MSW of 32-bit word
                ;

                ; Load integrator state
                AMOV        cicState_xptr, XAR4
             || MOV         *lData_ptr+, T1  ; get MSW
                MOV         dbl(*AR4+), AC0
                MOV         dbl(*AR4+), AC1
                MOV         dbl(*AR4+), AC2
                MOV         dbl(*AR4+), AC3

             || RPTBLOCAL   loop1_left1-1
                MOV         #0, T0
             || ROL         TC2, T1, TC2, T1
                ROL         TC2, T0, TC2, T0    ; LSB of T0 = bit
                XCCPART     T0==#0
             || SUB         #1, T0              ; T0 = input = 2*bit-1: 0->-1, 1->+1

                ; Perform integration for current input
                ADD         T0, AC0             ; compute integrator 1 output ; AC0 = acc[0]
                ADD         AC0, AC1            ; compute integrator 2 output ; AC1 = acc[1]
                ADD         AC1, AC2            ; compute integrator 3 output ; AC2 = acc[2]
                ADD         AC2, AC3            ; compute integrator 4 output ; AC3 = acc[3]
loop1_left1:

                ; Store integrator state
                AMOV        cicState_xptr, XAR4
                MOV         AC0, dbl(*AR4+)
                MOV         AC1, dbl(*AR4+)
                MOV         AC2, dbl(*AR4+)
                MOV         AC3, dbl(*AR4+)

                ; Compute differentiator output
                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[0]
                MOV         AC3, dbl(*AR4+)     ; diffDly[0] = AC3 = acc[3]
             || SUB         AC0, AC3            ; compute differentiator 1 output
                                                ; AC3 = diff[0] = acc[3] - diffDly[0]

                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[1]
                MOV         AC3, dbl(*AR4+)     ; diffDly[1] = AC3 = diff[0]
             || SUB         AC0, AC3            ; compute differentiator 1 output
                                                ; AC3 = diff[1] = diff[0] - diffDly[1]

                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[2]
                MOV         AC3, dbl(*AR4+)     ; diffDly[2] = AC3 = diff[1]
             || SUB         AC0, AC3            ; compute differentiator 2 output
                                                ; AC3 = diff[2] = diff[1] - diffDly[2]

                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[3]
                MOV         AC3, dbl(*AR4+)     ; diffDly[3] = AC3 = diff[2]
             || SUB         AC0, AC3            ; compute differentiator 3 output
                                                ; AC3 = diff[3] = diff[2] - diffDly[3]

                ; Store output
                MOV         AC3, dbl(*outSamps_ptr+)

                ;
                ; LSW of 32-bit word
                ;

                ; Load integrator state
                AMOV        cicState_xptr, XAR4
             || MOV         *lData_ptr+, T1  ; get LSW
                MOV         dbl(*AR4+), AC0
             ;|| MOV         AC3, dbl(*outSamps_ptr+)
                MOV         dbl(*AR4+), AC1
                MOV         dbl(*AR4+), AC2
                MOV         dbl(*AR4+), AC3

             || RPTBLOCAL   loop1_left2-1
                MOV         #0, T0
             || ROL         TC2, T1, TC2, T1
                ROL         TC2, T0, TC2, T0    ; LSB of T0 = bit
                XCCPART     T0==#0
             || SUB         #1, T0              ; T0 = input = 2*bit-1: 0->-1, 1->+1

                ; Perform integration for current input
                ADD         T0, AC0             ; compute integrator 1 output ; AC0 = acc[0]
                ADD         AC0, AC1            ; compute integrator 2 output ; AC1 = acc[1]
                ADD         AC1, AC2            ; compute integrator 3 output ; AC2 = acc[2]
                ADD         AC2, AC3            ; compute integrator 4 output ; AC3 = acc[3]
loop1_left2:

                ; Store integrator state
                AMOV        cicState_xptr, XAR4
                MOV         AC0, dbl(*AR4+)
                MOV         AC1, dbl(*AR4+)
                MOV         AC2, dbl(*AR4+)
                MOV         AC3, dbl(*AR4+)

                ; Compute differentiator output
                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[0]
                MOV         AC3, dbl(*AR4+)     ; diffDly[0] = AC3 = acc[3]
             || SUB         AC0, AC3            ; compute differentiator 1 output
                                                ; AC3 = diff[0] = acc[3] - diffDly[0]

                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[1]
                MOV         AC3, dbl(*AR4+)     ; diffDly[1] = AC3 = diff[0]
             || SUB         AC0, AC3            ; compute differentiator 1 output
                                                ; AC3 = diff[1] = diff[0] - diffDly[1]

                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[2]
                MOV         AC3, dbl(*AR4+)     ; diffDly[2] = AC3 = diff[1]
             || SUB         AC0, AC3            ; compute differentiator 2 output
                                                ; AC3 = diff[2] = diff[1] - diffDly[2]

                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[3]
                MOV         AC3, dbl(*AR4+)     ; diffDly[3] = AC3 = diff[2]
             || SUB         AC0, AC3            ; compute differentiator 3 output
                                                ; AC3 = diff[3] = diff[2] - diffDly[3]

                ; Store output
                MOV         AC3, dbl(*outSamps_ptr+)

;
; Unpack current "right" word
;----------------------------------------------------------------
                ;
                ; MSW of 32-bit word
                ;

                ; Load integrator state
                AMOV        cicState_xptr, XAR4
             || MOV         *rData_ptr+, T1  ; get MSW
                MOV         dbl(*AR4+), AC0
             ;|| MOV         AC3, dbl(*outSamps_ptr+)
                MOV         dbl(*AR4+), AC1
                MOV         dbl(*AR4+), AC2
                MOV         dbl(*AR4+), AC3

             || RPTBLOCAL   loop1_right1-1
                MOV         #0, T0
             || ROL         TC2, T1, TC2, T1
                ROL         TC2, T0, TC2, T0    ; LSB of T0 = bit
                XCCPART     T0==#0
             || SUB         #1, T0              ; T0 = input = 2*bit-1: 0->-1, 1->+1

                ; Perform integration for current input
                ADD         T0, AC0             ; compute integrator 1 output ; AC0 = acc[0]
                ADD         AC0, AC1            ; compute integrator 2 output ; AC1 = acc[1]
                ADD         AC1, AC2            ; compute integrator 3 output ; AC2 = acc[2]
                ADD         AC2, AC3            ; compute integrator 4 output ; AC3 = acc[3]
loop1_right1:

                ; Store integrator state
                AMOV        cicState_xptr, XAR4
                MOV         AC0, dbl(*AR4+)
                MOV         AC1, dbl(*AR4+)
                MOV         AC2, dbl(*AR4+)
                MOV         AC3, dbl(*AR4+)

                ; Compute differentiator output
                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[0]
                MOV         AC3, dbl(*AR4+)     ; diffDly[0] = AC3 = acc[3]
             || SUB         AC0, AC3            ; compute differentiator 1 output
                                                ; AC3 = diff[0] = acc[3] - diffDly[0]

                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[1]
                MOV         AC3, dbl(*AR4+)     ; diffDly[1] = AC3 = diff[0]
             || SUB         AC0, AC3            ; compute differentiator 1 output
                                                ; AC3 = diff[1] = diff[0] - diffDly[1]

                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[2]
                MOV         AC3, dbl(*AR4+)     ; diffDly[2] = AC3 = diff[1]
             || SUB         AC0, AC3            ; compute differentiator 2 output
                                                ; AC3 = diff[2] = diff[1] - diffDly[2]

                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[3]
                MOV         AC3, dbl(*AR4+)     ; diffDly[3] = AC3 = diff[2]
             || SUB         AC0, AC3            ; compute differentiator 3 output
                                                ; AC3 = diff[3] = diff[2] - diffDly[3]

                ; Store output
                MOV         AC3, dbl(*outSamps_ptr+)

                ;
                ; LSW of 32-bit word
                ;

                ; Load integrator state
                AMOV        cicState_xptr, XAR4
             || MOV         *rData_ptr+, T1  ; get LSW
                MOV         dbl(*AR4+), AC0
             ;|| MOV         AC3, dbl(*outSamps_ptr+)
                MOV         dbl(*AR4+), AC1
                MOV         dbl(*AR4+), AC2
                MOV         dbl(*AR4+), AC3

             || RPTBLOCAL   loop1_right2-1
                MOV         #0, T0
             || ROL         TC2, T1, TC2, T1
                ROL         TC2, T0, TC2, T0    ; LSB of T0 = bit
                XCCPART     T0==#0
             || SUB         #1, T0              ; T0 = input = 2*bit-1: 0->-1, 1->+1

                ; Perform integration for current input
                ADD         T0, AC0             ; compute integrator 1 output ; AC0 = acc[0]
                ADD         AC0, AC1            ; compute integrator 2 output ; AC1 = acc[1]
                ADD         AC1, AC2            ; compute integrator 3 output ; AC2 = acc[2]
                ADD         AC2, AC3            ; compute integrator 4 output ; AC3 = acc[3]
loop1_right2:

                ; Store integrator state
                AMOV        cicState_xptr, XAR4
                MOV         AC0, dbl(*AR4+)
                MOV         AC1, dbl(*AR4+)
                MOV         AC2, dbl(*AR4+)
                MOV         AC3, dbl(*AR4+)

                ; Compute differentiator output
                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[0]
                MOV         AC3, dbl(*AR4+)     ; diffDly[0] = AC3 = acc[3]
             || SUB         AC0, AC3            ; compute differentiator 1 output
                                                ; AC3 = diff[0] = acc[3] - diffDly[0]

                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[1]
                MOV         AC3, dbl(*AR4+)     ; diffDly[1] = AC3 = diff[0]
             || SUB         AC0, AC3            ; compute differentiator 1 output
                                                ; AC3 = diff[1] = diff[0] - diffDly[1]

                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[2]
                MOV         AC3, dbl(*AR4+)     ; diffDly[2] = AC3 = diff[1]
             || SUB         AC0, AC3            ; compute differentiator 2 output
                                                ; AC3 = diff[2] = diff[1] - diffDly[2]

                MOV         dbl(*AR4), AC0      ; AC0 = diffDly[3]
                MOV         AC3, dbl(*AR4+)     ; diffDly[3] = AC3 = diff[2]
             || SUB         AC0, AC3            ; compute differentiator 3 output
                                                ; AC3 = diff[3] = diff[2] - diffDly[3]

                ; Store output
                MOV         AC3, dbl(*outSamps_ptr+)
loop0:

;
; Restore status regs to expected C-convention values as needed
;----------------------------------------------------------------
                ;BCLR        M40                 ; clear M40
                ;BCLR        FRCT                ; clear FRCT

                BSET        ARMS                ; set ARMS

                ;BSET        SMUL                ; set SMUL

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
