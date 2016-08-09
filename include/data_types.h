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
#ifndef __DATA_TYPES_H__
#define __DATA_TYPES_H__

//#define FALSE ( 0 )
//#define TRUE  ( 1 )

#ifdef WIN32
#define restrict
#define onchip
#define Int40               Int64
#define Uint40              Uint64

typedef short               Int16;
typedef long                Int32;
typedef unsigned short      Uint16;
typedef unsigned long       Uint32;
typedef float               Float32;
typedef double              Float64;
//typedef float              Float64;
#if (_MSC_VER == 1200)
typedef __int64             Int64;
typedef unsigned __int64    Uint64;
#else
typedef long long           Int64;
typedef unsigned long long  Uint64;
#endif

#elif defined(__TMS320C55X__)
#define Int64               Int40   /* no 64-bit integer on C55x */
#define Uint64              Uint40
#define Float32             Float64 /* no 64-bit float on C55x */

typedef short               Int16;
typedef long                Int32;
typedef long long           Int40;
typedef unsigned short      Uint16;
typedef unsigned long       Uint32;
typedef unsigned long long  Uint40;
typedef float               Float32;

#else
#define restrict
#define Int40               Int64

typedef short               Int16;
typedef int                 Int32;
typedef long long           Int64;
typedef unsigned short      Uint16;
typedef unsigned int        Uint32;
typedef unsigned long long  Uint64;
typedef float               Float32;
typedef double              Float64;

#endif

#endif /* __DATA_TYPES_H__ */
