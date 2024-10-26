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

#include "oapv_sad_avx.h"

#if X86_SSE
const oapv_fn_sad_t oapv_tbl_fn_sad_16b_avx[2] =
{
    oapv_sad_16b_sse_8x2n,
        NULL
};

static s64 ssd_16b_sse_8x8_avx(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    s16* s1 = (s16*)src1;
    s16* s2 = (s16*)src2;
    int t[8] = { 0 };
    __m256i sum = _mm256_setzero_si256();
    __m256i v1, v2;

    for (int i = 0; i < 64; i += 8)
    {
        v1 = _mm256_loadu_si256((const __m256i*)(s1 + i));
        v2 = _mm256_loadu_si256((const __m256i*)(s2 + i));
        v2 = _mm256_sub_epi16(v1, v2);
        v2 = _mm256_madd_epi16(v2, v2);
        sum = _mm256_add_epi32(sum, v2);
        _mm256_storeu_si256((__m256i*)(t), sum);
    }
    return t[0] + t[1] + t[2] + t[3];
}

const oapv_fn_ssd_t oapv_tbl_fn_ssd_16b_avx[2] =
{
    ssd_16b_sse_8x8_avx,
        NULL
};

#endif