/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of the copyright owner, nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _OAPV_BSR_H_
#define _OAPV_BSR_H_

#include "oapv_port.h"

typedef struct oapv_bs oapv_bs_t;
typedef int (*oapv_bs_fn_flush_t)(oapv_bs_t *bs, int byte);

struct oapv_bs
{
    u32 code; // intermediate code buffer
    int leftbits; // left bits count in code
    u8* cur; // address of current bitstream position
    u8* end; // address of bitstream end
    u8* beg; // address of bitstream begin
    u32 size; // size of input bitstream in byte
    oapv_bs_fn_flush_t fn_flush; // function pointer for flush operation
    int ndata[4]; // arbitrary data, if needs
    void* pdata[4]; // arbitrary address, if needs
    char is_bin_count;
    u32  bin_count;
};


///////////////////////////////////////////////////////////////////////////////
// start of encoder code
#if ENABLE_ENCODER
///////////////////////////////////////////////////////////////////////////////

static inline bool bsw_is_align8(oapv_bs_t* bs)
{
    return (bool)(!((bs)->leftbits & 0x7));
}

static inline int bsw_get_write_byte(oapv_bs_t* bs)
{
    return (int)((u8*)(bs->cur) - (u8*)(bs->beg));
}

void oapv_bsw_init(oapv_bs_t * bs, u8 * buf, int size, oapv_bs_fn_flush_t fn_flush);
void oapv_bsw_deinit(oapv_bs_t * bs);
void* oapv_bsw_sink(oapv_bs_t* bs);
int oapv_bsw_write_direct(void * bits, u32 val, int len);

#if TRACE_HLS
#define oapv_bsw_write1(A, B) oapv_bsw_write1_trace(A, B, #B)
int oapv_bsw_write1_trace(oapv_bs_t* bs, int val, char* name);

#define oapv_bsw_write(A, B, C) oapv_bsw_write_trace(A, B, #B, C)
int oapv_bsw_write_trace(oapv_bs_t* bs, u32 val, char* name, int len);
#else
int oapv_bsw_write1(oapv_bs_t * bs, int val);
int oapv_bsw_write(oapv_bs_t * bs, u32 val, int len);
#endif
///////////////////////////////////////////////////////////////////////////////
// end of encoder code
#endif // ENABLE_ENCODER
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// start of decoder code
#if ENABLE_DECODER
///////////////////////////////////////////////////////////////////////////////
/*! is bitstream byte aligned? */
static bool inline bsr_is_align8(oapv_bs_t* bs)
{
    return ((bs->leftbits & 0x7) == 0) ? true: false;
}

/* get number of byte consumed */
static int inline bsr_get_read_byte(oapv_bs_t* bs)
{
    return ((int)((bs)->cur - (bs)->beg) - ((bs)->leftbits >> 3));
}

static int inline bsr_get_remained_byte(oapv_bs_t* bs)
{
    return (bs->size - bsr_get_read_byte(bs));
}

void oapv_bsr_init(oapv_bs_t * bs, u8 * buf, int size, oapv_bs_fn_flush_t fn_flush);
int oapv_bsr_flush(oapv_bs_t * bs, int byte);
int oapv_bsr_clz_in_code(u32 code);
void oapv_bsr_align8(oapv_bs_t* bs);
void oapv_bsr_skip(oapv_bs_t* bs, int size);
void oapv_bsr_peek(oapv_bs_t* bs, u32 * val, int size);
void* oapv_bsr_sink(oapv_bs_t* bs);
void oapv_bsr_move(oapv_bs_t* bs, u8* pos);

#if TRACE_HLS
#define oapv_bsr_read(A, B, C) oapv_bsr_read_trace(A, B, #B, C)
void oapv_bsr_read_trace(oapv_bs_t * bs, u32 * val, char * name, int size);

#define oapv_bsr_read1(A) oapv_bsr_read1_trace(A, " ")
int oapv_bsr_read1_trace(oapv_bs_t * bs, char * name);

#else
u32 oapv_bsr_read(oapv_bs_t * bs, int size);
int oapv_bsr_read1(oapv_bs_t * bs);
#endif
///////////////////////////////////////////////////////////////////////////////
// end of decoder code
#endif // ENABLE_DECODER
///////////////////////////////////////////////////////////////////////////////

#endif /* _OAPV_BSR_H_ */

