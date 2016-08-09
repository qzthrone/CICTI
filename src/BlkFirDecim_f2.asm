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
; Function:    _blkFirDecim2
; Processor:   C55xx, Rev. 3
; Description: 
;   Block decimating FIR.
;   S18Q16 input and output data, S16Q15 coefficients.
;   Decimation factor fixed at 2.
;   Computes two outputs per inner loop 
;   (assumes even number of outputs).
;
;   x cycle inner loop.
;
;   C-callable.
;   Mnemonic assembly.
;
; Usage:    void blkFirDecim2(
;               Int32   *inSamps,       -> XAR0
;               Int16   *coefs,         -> XAR1
;               Int32   *outSamps,      -> XAR2
;               Int32   *dlyBuf,        -> XAR3
;               Uint16  numInSamps,     -> T0
;               Uint16  numCoefs        -> T1
;           );
;
;****************************************************************

                .C54CM_off                      ; enable assembler for C54CM=0
                .ARMS_off                       ; enable assembler for ARMS=0
                .CPL_on                         ; enable assembler for CPL=1

                .mmregs                         ; enable mem mapped register names

                .def    _blkFirDecim2

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
                .asg    AR0, x_ptr              ; linear pointer
                .asg    CDP, h_ptr              ; circular pointer
                .asg    AR2, r_ptr              ; linear pointer
                .asg    AR4, db_ptr2            ; circular pointer (lagging delay buffer pointer)
                .asg    AR5, db_ptr1            ; circular pointer (leading delay buffer pointer)

                .asg    BSAC, h_base            ; base addr for h_ptr
                .asg    BKC, h_sz               ; circ buffer size for h_sz
                .asg    XCDP, xh_base           ; extended base addr for h_ptr
                .asg    BSA45, db_base          ; base addr for db_ptr
                .asg    BK47, db_sz             ; circ buffer size for db_ptr
                .asg    XAR4, xdb_base2         ; extended base addr for db_ptr2 (lagging delay buffer pointer)
                .asg    XAR5, xdb_base1         ; extended base addr for db_ptr1 (leading delay buffer pointer)

                .asg    BRC0, outer_cnt         ; outer loop count
                .asg    BRC1, inner_cnt         ; inner loop count

ST2mask         .set    0000000100110000b       ; circular/linear pointers

QUANT_TRUNC     .set    0                       ; truncate
QUANT_RND       .set    1                       ; round according to rounding mode
QUANT_MODE      .set    QUANT_RND


                .text
_blkFirDecim2:

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

                OR          #04540h, mmap(ST1_55)   ; set CPL, M40, SXMD, and FRCT
                ;AND         #0F9DFh, mmap(ST1_55)   ; clear M40, SATD, 54CM 

                BCLR        ARMS                    ; clear ARMS

                ;AND         #0FCDDh, mmap(ST3_55)   ; clear SATA, SMUL ; note -- must always write 1100b to bits 11-8, 0b to bit 4

;
; Setup passed parameters in their destination registers
; Setup circular/linear CDP/ARx behavior
;----------------------------------------------------------------

; x pointer - passed in its destination register, need do nothing

; h pointer
                MOV         XAR1, xh_base           ; h array address
                MOV         mmap(AR1), h_base       ; base address of coefficients
                MOV         #0, h_ptr               ; point to first coefficient
                MOV         mmap(T1), h_sz          ; coefficient array size h_sz = nh

; r pointer - already passed in its destination register

; db pointers
                ; Initialize lagging delay pointer
                MOV         XAR3, xdb_base2         ; db array address
                ; Read index of oldest db entry
                MOV         dbl(*AR3+), AC0
                MOV         AC0, db_ptr2
                MOV         mmap(AR3), db_base      ; base address for db_ptr
                ; db_sz = 2*(nh+2) (x2 for 32-bit array)
                MOV         T1, AC0
                ADD         #2, AC0
                MOV         AC0 << #1, mmap(db_sz)

; Set circular/linear ARx behavior
                OR          #ST2mask, mmap(ST2_55)  ; config circ/linear pointers

               ; Initialize leading delay pointer
                AMOV        xdb_base2, xdb_base1
                ;AMAR        *db_ptr1-
                ;AMAR        *db_ptr1-

;
; Setup loop counts
;----------------------------------------------------------------
                MOV			T0, AC0					; AC0 = nx (nx unsigned, constrained to < 32768 since SXM is enabled)
             || AMAR        *db_ptr1-
                SFTS        AC0, #-2                ; AC0 = (nx/2)/2
             || AMAR        *db_ptr1-
                SUB         #1, AC0                 ; AC0 = (nx/2)/2-1
                MOV         AC0, mmap(outer_cnt)    ; outer_cnt = (nx/D)/2-1=(nx/4)-1
    
                SUB         #3, T1
                MOV         T1, mmap(inner_cnt)     ; inner_cnt = nh-3


;
; Start of outer loop
;----------------------------------------------------------------
                RPTBLOCAL   loop0-1                 ; outer loop (nx/D)=(nx/2) iterations

                ; Place 1 sample in delay line at lagging index
                MOV         dbl(*x_ptr+), dbl(*db_ptr2)
    
                ; Place D=2 samples in delay line at leading index
                MOV         dbl(*x_ptr+), dbl(*db_ptr1-)
                MOV         dbl(*x_ptr+), dbl(*db_ptr1)

                ; Sum h*x nh-iterations for next r value
                MPY         *db_ptr2+, *h_ptr, AC3          ; High x Low
             :: MPY         *db_ptr1+, *h_ptr, AC1          ; High x Low
                MPY         uns(*db_ptr2+), *h_ptr+, AC2    ; Low x Low
             :: MPY         uns(*db_ptr1+), *h_ptr+, AC0    ; Low x Low

             || RPTBLOCAL   loop1-1
                MAC         *db_ptr2+, *h_ptr, AC3          ; High x Low
             :: MAC         *db_ptr1+, *h_ptr, AC1          ; High x Low
                MAC         uns(*db_ptr2+), *h_ptr+, AC2    ; Low x Low
             :: MAC         uns(*db_ptr1+), *h_ptr+, AC0    ; Low x Low
loop1:

                MAC         *db_ptr2+, *h_ptr, AC3          ; High x Low
             :: MAC         *db_ptr1+, *h_ptr, AC1          ; High x Low
                .if (QUANT_MODE == QUANT_TRUNC)
                MAC         uns(*db_ptr2-), *h_ptr+, AC2    ; Low x Low
             :: MAC         uns(*db_ptr1-), *h_ptr+, AC0    ; Low x Low
                .else
                MACR        uns(*db_ptr2-), *h_ptr+, AC2    ; Low x Low
             :: MACR        uns(*db_ptr1-), *h_ptr+, AC0    ; Low x Low
                .endif

                ADD         AC3 << #16, AC2
                ADD         AC1 << #16, AC0

                ; Store result to memory
                SFTS        AC2, #-16
                MOV         AC2, dbl(*r_ptr+)       ; store S18Q16 value to memory
             || SFTS        AC0, #-16
                MOV         AC0, dbl(*r_ptr+)       ; store S18Q16 value to memory
    
                ; Place D-1=2-1=1 sample in delay line at lagging index
                MOV         dbl(*x_ptr+), dbl(*db_ptr2-)
loop0: ; end of outer loop
    
;
; Update the db entry point
;----------------------------------------------------------------
                MOV         db_ptr2, *-AR3           ; update 1st element of db array

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
