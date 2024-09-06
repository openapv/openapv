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

#include "oapv_def.h"
#include "oapv_tq_neon.h"
#include "sse2neon.h"

#define OAPV_ITX_CLIP_SSE(X, min, max) \
    X = _mm_max_epi32(X, min_val);    \
    X = _mm_min_epi32(X, max_val);

#define vmadd_s16(a, coef) \
    vpaddq_s32(vmull_s16(a.val[0], vget_low_s16(coef)), vmull_s16(a.val[1], vget_high_s16(coef)));

typedef union
{
    int8x16_t vect_s8[2];
    int16x8_t vect_s16[2];
    int32x4_t vect_s32[2];
    int64x2_t vect_s64[2];
    uint8x16_t vect_u8[2];
    uint16x8_t vect_u16[2];
    uint32x4_t vect_u32[2];
    uint64x2_t vect_u64[2];
    __m128i vect_i128[2];
} __m256i __attribute__((aligned(32)));

typedef struct
{
    float32x4_t vect_f32[2];
} __m256;

typedef struct
{
    float64x2_t vect_f64[2];
} __m256d;

__m256i _mm256_setzero_si256()
{
    __m256i ret;
    ret.vect_s32[0] = ret.vect_s32[1] = vdupq_n_s32(0);
    return ret;
}

__m256i _mm256_loadu_si256(__m256i const *mem_addr)
{
    __m256i ret;
    ret.vect_s32[0] = vld1q_s32((int32_t const *)mem_addr);
    ret.vect_s32[1] = vld1q_s32(((int32_t const *)mem_addr) + 4);
    return ret;
}

__m256i _mm256_set_m128i(__m128i hi, __m128i lo)
{
    __m256i res_m256i;
    res_m256i.vect_s32[0] = vreinterpretq_s32_m128i(lo);
    res_m256i.vect_s32[1] = vreinterpretq_s32_m128i(hi);
    return res_m256i;
}

__m256i _mm256_set1_epi16(int8_t a)
{
    __m256i ret;
    ret.vect_s16[0] = ret.vect_s16[1] = vdupq_n_s16(a);
    return ret;
}

__m256i _mm256_set1_epi32(int16_t a)
{
    __m256i ret;
    ret.vect_s32[0] = ret.vect_s32[1] = vdupq_n_s32(a);
    return ret;
}
__m256i _mm256_set_epi32(int16_t e7, int16_t e6, int16_t e5, int16_t e4, int16_t e3, int16_t e2, int16_t e1, int16_t e0)
{
    __m256i res_m256i;
    int32_t v0[] = {e0, e1, e2, e3};
    int32_t v1[] = {e4, e5, e6, e7};
    res_m256i.vect_s32[0] = vld1q_s32(v0);
    res_m256i.vect_s32[1] = vld1q_s32(v1);
    return res_m256i;
}

__m256i _mm256_set_epi16(int16_t e15, int16_t e14, int16_t e13, int16_t e12, int16_t e11, int16_t e10, int16_t e9, int16_t e8, int16_t e7, int16_t e6, int16_t e5, int16_t e4, int16_t e3, int16_t e2, int16_t e1, int16_t e0)
{
    __m256i res_m256i;
    int16_t v0[] = {e0, e1, e2, e3, e4, e5, e6, e7};
    int16_t v1[] = {e8, e9, e10, e11, e12, e13, e14, e15};
    res_m256i.vect_s16[0] = vld1q_s16(v0);
    res_m256i.vect_s16[1] = vld1q_s16(v1);
    return res_m256i;
}

__m256i _mm256_packs_epi32(__m256i a, __m256i b)
{
    __m256i res;
    res.vect_s16[0] = vcombine_s16(vqmovn_s32(a.vect_s32[0]), vqmovn_s32(b.vect_s32[0]));
    res.vect_s16[1] = vcombine_s16(vqmovn_s32(a.vect_s32[1]), vqmovn_s32(b.vect_s32[1]));
    return res;
}
__m256i _mm256_add_epi32(__m256i a, __m256i b)
{
    __m256i res_m256i;
    res_m256i.vect_s32[0] = vaddq_s32(a.vect_s32[0], b.vect_s32[0]);
    res_m256i.vect_s32[1] = vaddq_s32(a.vect_s32[1], b.vect_s32[1]);
    return res_m256i;
}

__m256i _mm256_srai_epi32(__m256i a, int imm8)
{
    __m256i result_m256i;

    if (imm8 >= 0 && imm8 < 64)
    {
        int32x4_t vect_imm = vdupq_n_s32(-imm8);
        result_m256i.vect_s32[0] = vshlq_s32(a.vect_s32[0], vect_imm);
        result_m256i.vect_s32[1] = vshlq_s32(a.vect_s32[1], vect_imm);
    }
    else
    {
        result_m256i.vect_s32[0] = vdupq_n_s32(0);
        result_m256i.vect_s32[1] = vdupq_n_s32(0);
    }
    return result_m256i;
}

void _mm256_storeu_si256(__m256i *mem_addr, __m256i a)
{
    vst1q_s8((int8_t *)mem_addr, a.vect_s8[0]);
    vst1q_s8((int8_t *)mem_addr + 16, a.vect_s8[1]);
}

__m256i _mm256_madd_epi16(__m256i a, __m256i b)
{
    int32x4_t low_1 = vmull_s16(vget_low_s16(a.vect_s16[0]),
                                vget_low_s16(b.vect_s16[0]));
    int32x4_t low_2 = vmull_s16(vget_high_s16(a.vect_s16[0]),
                                vget_high_s16(b.vect_s16[0]));
    int32x4_t high_1 = vmull_s16(vget_low_s16(a.vect_s16[1]),
                                 vget_low_s16(b.vect_s16[1]));
    int32x4_t high_2 = vmull_s16(vget_high_s16(a.vect_s16[1]),
                                 vget_high_s16(b.vect_s16[1]));

    int32x2_t low_1_sum = vpadd_s32(vget_low_s32(low_1), vget_high_s32(low_1));
    int32x2_t low_2_sum = vpadd_s32(vget_low_s32(low_2), vget_high_s32(low_2));
    int32x2_t high_1_sum = vpadd_s32(vget_low_s32(high_1), vget_high_s32(high_1));
    int32x2_t high_2_sum = vpadd_s32(vget_low_s32(high_2), vget_high_s32(high_2));
    __m256i result;
    result.vect_s32[0] = vcombine_s32(low_1_sum, low_2_sum);
    result.vect_s32[1] = vcombine_s32(high_1_sum, high_2_sum);
    return result;
}

__m256i _mm256_hadd_epi32(__m256i _a, __m256i _b)
{
    int32x4_t a1 = _a.vect_s32[0];
    int32x4_t b1 = _b.vect_s32[0];
    int32x4_t a2 = _a.vect_s32[1];
    int32x4_t b2 = _b.vect_s32[1];
    __m256i result;
    result.vect_s32[0] = vcombine_s32(vpadd_s32(vget_low_s32(a1), vget_high_s32(a1)), vpadd_s32(vget_low_s32(b1), vget_high_s32(b1)));
    result.vect_s32[1] = vcombine_s32(vpadd_s32(vget_low_s32(a2), vget_high_s32(a2)), vpadd_s32(vget_low_s32(b2), vget_high_s32(b2)));
    return result;
}

__m256i _mm256_permute4x64_epi64(__m256i a, const int imm8)
{
    __m256i res = _mm256_setzero_si256();
    int64_t ptr_a[4];
    vst1q_s64(ptr_a, a.vect_s64[0]);
    vst1q_s64(ptr_a + 2, a.vect_s64[1]);
    const int id0 = imm8 & 0x03;
    const int id1 = (imm8 >> 2) & 0x03;
    const int id2 = (imm8 >> 4) & 0x03;
    const int id3 = (imm8 >> 6) & 0x03;
    res.vect_s64[0] = vsetq_lane_s64(ptr_a[id0], res.vect_s64[0], 0);
    res.vect_s64[0] = vsetq_lane_s64(ptr_a[id1], res.vect_s64[0], 1);
    res.vect_s64[1] = vsetq_lane_s64(ptr_a[id2], res.vect_s64[1], 0);
    res.vect_s64[1] = vsetq_lane_s64(ptr_a[id3], res.vect_s64[1], 1);
    return res;
}

__m256i _mm256_permute2x128_si256(__m256i a, __m256i b, int imm8)
{
    __m256i res;
    int bit_0 = imm8 & 0x1, bit_1 = imm8 & 0x2, bit_3 = imm8 & 0x8;
    if (bit_1 == 0)
    {
        res.vect_s32[0] = a.vect_s32[bit_0];
    }
    else
    {
        res.vect_s32[0] = b.vect_s32[bit_0];
    }
    if (bit_3)
    {
        res.vect_s32[0] = vdupq_n_s32(0);
    }
    bit_0 = (imm8 & 0x10) >> 4, bit_1 = imm8 & 0x20, bit_3 = imm8 & 0x80;
    if (bit_1 == 0)
    {
        res.vect_s32[1] = a.vect_s32[bit_0];
    }
    else
    {
        res.vect_s32[1] = b.vect_s32[bit_0];
    }
    if (bit_3)
    {
        res.vect_s32[1] = vdupq_n_s32(0);
    }
    return res;
}
__m256i _mm256_shuffle_epi8(__m256i a, __m256i b)
{
    __m256i res_m256i;
    uint8x16_t mask_and = vdupq_n_u8(0x8f);
    res_m256i.vect_u8[0] = vqtbl1q_u8(a.vect_u8[0], vandq_u8(b.vect_u8[0], mask_and));
    res_m256i.vect_u8[1] = vqtbl1q_u8(a.vect_u8[1], vandq_u8(b.vect_u8[1], mask_and));
    return res_m256i;
}

__m128i _mm256_extracti128_si256(__m256i a, const int imm8)
{
    assert(imm8 >= 0 && imm8 <= 1);
    __m128i res_m128i;
    res_m128i = a.vect_s64[imm8];
    return res_m128i;
}
__m128i _mm256_castsi256_si128(__m256i a)
{
    __m128i ret;
    ret = a.vect_s64[0];
    return ret;
}

__m256i _mm256_sub_epi32(__m256i a, __m256i b)
{
    __m256i res_m256i;
    res_m256i.vect_s32[0] = vsubq_s32(a.vect_s32[0], b.vect_s32[0]);
    res_m256i.vect_s32[1] = vsubq_s32(a.vect_s32[1], b.vect_s32[1]);
    return res_m256i;
}

__m256i _mm256_mullo_epi32(__m256i a, __m256i b)
{
    __m256i res_m256i;
    res_m256i.vect_s32[0] = vmulq_s32(a.vect_s32[0], b.vect_s32[0]);
    res_m256i.vect_s32[1] = vmulq_s32(a.vect_s32[1], b.vect_s32[1]);
    return res_m256i;
}

__m256i _mm256_hsub_epi32(__m256i a, __m256i b)
{
    __m256i res_m256i;
    res_m256i.vect_i128[0] = _mm_hsub_epi32(a.vect_s64[0], b.vect_s64[0]);
    res_m256i.vect_i128[1] = _mm_hsub_epi32(a.vect_s64[1], b.vect_s64[1]);
    return res_m256i;
}

__m256i _mm256_cvtepi16_epi32(__m128i a)
{
    __m256i ret;
    ret.vect_s32[0] = vmovl_s16(vget_low_s16(vreinterpretq_s16_m128i(a)));
    ret.vect_s32[1] = vmovl_s16(vget_high_s16(vreinterpretq_s16_m128i(a)));
    return ret;
} // warn

#if OPTIMIZATION_QUANT_1
__m256i _mm256_xor_si256(__m256i x, __m256i y) {

    __m256i result_m256i;
    result_m256i.vect_s32[0] = veorq_s32(x.vect_s32[0], y.vect_s32[0]);
    result_m256i.vect_s32[1] = veorq_s32(x.vect_s32[1], y.vect_s32[1]);
    return result_m256i;
}

__m256i _mm256_abs_epi32(__m256i a) {

    __m256i result_m256i;
    const int32x4_t mask = vdupq_n_s32(0x7FFFFFFF);
    int32x4_t lo_part = vandq_s32(a.vect_s32[0], mask);
    int32x4_t hi_part = vandq_s32(a.vect_s32[1], mask);

    result_m256i.vect_s32[0] = lo_part;
    result_m256i.vect_s32[1] = hi_part;

    return result_m256i;
}


__m128i _mm256_cvtepi32_epi16(__m256i a) {

    int32x4_t lo_part = vget_low_s32(a);
    int32x4_t hi_part = vget_high_s32(a);

    int16x4_t lo_lo = vmovn_s32(lo_part);
    int16x4_t lo_hi = vmovn_s32(hi_part);
    int16x8_t result = vcombine_s16(lo_lo, lo_hi);

    return vreinterpretq_s16_s8(result);
}

__m256i _mm256_slli_epi32(__m256i a, int imm8) {

    __m256i res_m256i;

    res_m256i.vect_s32[0] = vshlq_n_s32(a.vect_s32[0], imm8);

    res_m256i.vect_s32[1] = vshlq_n_s32(a.vect_s32[1], imm8);

    return res_m256i;

}

#endif

__m256i _mm256_setr_epi32(int16_t e7, int16_t e6, int16_t e5, int16_t e4, int16_t e3, int16_t e2, int16_t e1, int16_t e0)
{
    __m256i res_m256i;
    int32_t v0[] = {e7, e6, e5, e4};
    int32_t v1[] = {e3, e2, e1, e0};
    res_m256i.vect_s32[0] = vld1q_s32(v0);
    res_m256i.vect_s32[1] = vld1q_s32(v1);
    return res_m256i;
} // warn

#ifndef _mm256_loadu2_m128i
#define _mm256_loadu2_m128i(/* __m128i const* */ hiaddr, \
                            /* __m128i const* */ loaddr) \
    _mm256_set_m128i(_mm_loadu_si128(hiaddr), _mm_loadu_si128(loaddr))
#endif

const s32 oapv_coeff[8][4] =
{
    {64, 64, 64, 64}, // 0th row coeff
    {89, 75, 50, 18}, // 2nd row coeff
    {84, 35, 84, 35}, // 3rd row coeff
    {75,-18,-89,-50}, // 4th row coeff
    {64,-64, 64,-64}, // 5th row coeff
    {50,-89, 18, 75}, // 6th row coeff
    {35,-84, 35,-84}, // 7th row coeff
    {18,-50, 75,-89}  // 8th row coeff
};

#define multiply_s32(part1, part2, coeff, res) \
    low = vmulq_s32(part1, coeff); \
    high = vmulq_s32(part2, coeff); \
    res = vcombine_s32(vpadd_s32(vget_low_s32(low), vget_high_s32(low)), vpadd_s32(vget_low_s32(high), vget_high_s32(high))); \

static void oapv_tx_pb8b_neon(s16 *src, s16 *dst, const int shift, int line)
{
    s16 i;
    s16 *tempSrc = src;
    int src_index;
    int16x4_t src_part1, src_part2;
    int32x4_t coeff0, coeff1, coeff2, coeff3, coeff4, coeff5, coeff6, coeff7;
    int32x4_t add = vdupq_n_s32(1 << (shift - 1));
    int32x4_t sh = vdupq_n_s32(-shift);

    int32x4_t EE_part1, EE_part2, EO_part1, EO_part2, low, high, result0, result1, result2, result3, result4, result5, result6, result7, E1, O1, E2, O2, res1, res2, res3, res4;

    for(i = 0; i < 8; i += 4)
    {
        // Loading src[0 - 3] and src[4 - 7]
        src_part1 = vld1_s16(tempSrc);
        tempSrc += 4;
        src_part2 = vld1_s16(tempSrc);
        tempSrc += 4;

        //reverse src_part2
        src_part2 = vrev64_s16(src_part2);

        E1 = vaddl_s16(src_part1, src_part2);
        O1 = vsubl_s16(src_part1, src_part2);

        // Loading src[8 - 11] and src[12 - 15]
        src_part1 = vld1_s16(tempSrc);
        tempSrc += 4;
        src_part2 = vld1_s16(tempSrc);
        tempSrc += 4;

        //reverse src_part2
        src_part2 = vrev64_s16(src_part2);

        E2 = vaddl_s16(src_part1, src_part2);
        O2 = vsubl_s16(src_part1, src_part2);

        int32x4_t tmp1 = vcombine_s32(vget_low_s32(E1), vget_low_s32(E2));
        int32x4_t tmp2 = vcombine_s32(vget_high_s32(E1), vget_high_s32(E2));
        tmp2 = vrev64q_s32(tmp2);

        EE_part1 = vaddq_s32(tmp1, tmp2);
        EO_part1 = vsubq_s32(tmp1, tmp2);

        coeff1 = vld1q_s32(oapv_coeff[1]);
        coeff3 = vld1q_s32(oapv_coeff[3]);
        coeff5 = vld1q_s32(oapv_coeff[5]);
        coeff7 = vld1q_s32(oapv_coeff[7]);

        multiply_s32(O1, O2, coeff1, result1);
        multiply_s32(O1, O2, coeff3, result3);
        multiply_s32(O1, O2, coeff5, result5);
        multiply_s32(O1, O2, coeff7, result7);

        res1 = vpaddq_s32(result1, result3);
        res2 = vpaddq_s32(result5, result7);

        // add and shift
        res1 = vshlq_s32(vaddq_s32(res1, add), sh);
        res2 = vshlq_s32(vaddq_s32(res2, add), sh);

        // Loading src[16 - 19] and src[20 - 23]
        src_part1 = vld1_s16(tempSrc);
        tempSrc += 4;
        src_part2 = vld1_s16(tempSrc);
        tempSrc += 4;

        //reverse src_part2
        src_part2 = vrev64_s16(src_part2);

        E1 = vaddl_s16(src_part1, src_part2);
        O1 = vsubl_s16(src_part1, src_part2);

        // Loading src[24 - 27] and src[28 - 31]
        src_part1 = vld1_s16(tempSrc);
        tempSrc += 4;
        src_part2 = vld1_s16(tempSrc);
        tempSrc += 4;

        //reverse src_part2
        src_part2 = vrev64_s16(src_part2);

        E2 = vaddl_s16(src_part1, src_part2);
        O2 = vsubl_s16(src_part1, src_part2);

        multiply_s32(O1, O2, coeff1, result1);
        multiply_s32(O1, O2, coeff3, result3);
        multiply_s32(O1, O2, coeff5, result5);
        multiply_s32(O1, O2, coeff7, result7);

        res3 = vpaddq_s32(result1, result3);
        res4 = vpaddq_s32(result5, result7);

        // add and shift
        res3 = vshlq_s32(vaddq_s32(res3, add), sh);
        res4 = vshlq_s32(vaddq_s32(res4, add), sh);

        // store result in destination
        vst1_s16(dst + 1 * line + i, vmovn_s32(vcombine_s32(vget_low_s32(res1), vget_low_s32(res3))));
        vst1_s16(dst + 3 * line + i, vmovn_s32(vcombine_s32(vget_high_s32(res1), vget_high_s32(res3))));
        vst1_s16(dst + 5 * line + i, vmovn_s32(vcombine_s32(vget_low_s32(res2), vget_low_s32(res4))));
        vst1_s16(dst + 7 * line + i, vmovn_s32(vcombine_s32(vget_high_s32(res2), vget_high_s32(res4))));

        coeff0 = vld1q_s32(oapv_coeff[0]);
        coeff2 = vld1q_s32(oapv_coeff[2]);
        coeff4 = vld1q_s32(oapv_coeff[4]);
        coeff6 = vld1q_s32(oapv_coeff[6]);

        tmp1 = vcombine_s32(vget_low_s32(E1), vget_low_s32(E2));
        tmp2 = vcombine_s32(vget_high_s32(E1), vget_high_s32(E2));
        tmp2 = vrev64q_s32(tmp2);

        EE_part2 = vaddq_s32(tmp1, tmp2);
        EO_part2 = vsubq_s32(tmp1, tmp2);

        multiply_s32(EE_part1, EE_part2, coeff0, result0);
        multiply_s32(EE_part1, EE_part2, coeff4, result4);
        multiply_s32(EO_part1, EO_part2, coeff2, result2);
        multiply_s32(EO_part1, EO_part2, coeff6, result6);

        // add and shift
        result0 = vshlq_s32(vaddq_s32(result0, add), sh);
        result2 = vshlq_s32(vaddq_s32(result2, add), sh);
        result4 = vshlq_s32(vaddq_s32(result4, add), sh);
        result6 = vshlq_s32(vaddq_s32(result6, add), sh);

        // store result in destination
        vst1_s16(dst + 0 * line + i, vmovn_s32(result0));
        vst1_s16(dst + 2 * line + i, vmovn_s32(result2));
        vst1_s16(dst + 4 * line + i, vmovn_s32(result4));
        vst1_s16(dst + 6 * line + i, vmovn_s32(result6));
    }
}

const oapv_fn_tx_t oapv_tbl_fn_txb_neon[1] =
    {
        oapv_tx_pb8b_neon,
};

///////////////////////////////////////////////////////////////////////////////
// end of encoder code
// ENABLE_ENCODER
///////////////////////////////////////////////////////////////////////////////

static void oapv_itx_pb8b_neon(s16 *src, s16 *dst, int shift, int line)
{
    s16 *pel_src = (s16 *)src;
    s32 *pel_dst = (s32 *)dst;
    __m128i r0, r1, r2, r3, r4, r5, r6, r7;
    __m128i a0, a1, a2, a3;
    __m128i e0, e1, e2, e3, o0, o1, o2, o3, eo0, eo1, ee0, ee1;
    __m128i v0, v1, v2, v3, v4, v5, v6, v7;
    __m128i t0, t1, t2, t3;
    __m128i max_val = _mm_set1_epi32(MAX_TX_VAL_32);
    __m128i min_val = _mm_set1_epi32(MIN_TX_VAL_32);
    __m128i coef[4][4];

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            coef[i][j] = _mm_set1_epi32(((s32)(oapv_tbl_tm8[j + 4][i]) << 16) | (oapv_tbl_tm8[j][i] & 0xFFFF));
        }
    }

    int i_src1 = line;
    int i_src2 = i_src1 + i_src1;
    int i_src3 = i_src2 + i_src1;
    int i_src4 = i_src3 + i_src1;
    int i_src5 = i_src4 + i_src1;
    int i_src6 = i_src5 + i_src1;
    int i_src7 = i_src6 + i_src1;

    for (int j = 0; j < line; j += 4)
    {
        r0 = _mm_loadl_epi64((__m128i *)(pel_src + j));
        r1 = _mm_loadl_epi64((__m128i *)(pel_src + i_src1 + j));
        r2 = _mm_loadl_epi64((__m128i *)(pel_src + i_src2 + j));
        r3 = _mm_loadl_epi64((__m128i *)(pel_src + i_src3 + j));
        r4 = _mm_loadl_epi64((__m128i *)(pel_src + i_src4 + j));
        r5 = _mm_loadl_epi64((__m128i *)(pel_src + i_src5 + j));
        r6 = _mm_loadl_epi64((__m128i *)(pel_src + i_src6 + j));
        r7 = _mm_loadl_epi64((__m128i *)(pel_src + i_src7 + j));

        a1 = _mm_unpacklo_epi16(r1, r5);
        a3 = _mm_unpacklo_epi16(r3, r7);

        t0 = _mm_madd_epi16(a1, coef[0][1]);
        t1 = _mm_madd_epi16(a3, coef[0][3]);
        o0 = _mm_add_epi32(t0, t1);

        t0 = _mm_madd_epi16(a1, coef[1][1]);
        t1 = _mm_madd_epi16(a3, coef[1][3]);
        o1 = _mm_add_epi32(t0, t1);

        t0 = _mm_madd_epi16(a1, coef[2][1]);
        t1 = _mm_madd_epi16(a3, coef[2][3]);
        o2 = _mm_add_epi32(t0, t1);

        t0 = _mm_madd_epi16(a1, coef[3][1]);
        t1 = _mm_madd_epi16(a3, coef[3][3]);
        o3 = _mm_add_epi32(t0, t1);

        a0 = _mm_unpacklo_epi16(r0, r4);
        a2 = _mm_unpacklo_epi16(r2, r6);

        eo0 = _mm_madd_epi16(a2, coef[0][2]);
        eo1 = _mm_madd_epi16(a2, coef[1][2]);
        ee0 = _mm_madd_epi16(a0, coef[0][0]);
        ee1 = _mm_madd_epi16(a0, coef[1][0]);

        e0 = _mm_add_epi32(ee0, eo0);
        e3 = _mm_sub_epi32(ee0, eo0);
        e1 = _mm_add_epi32(ee1, eo1);
        e2 = _mm_sub_epi32(ee1, eo1);

        v0 = _mm_add_epi32(e0, o0);
        v7 = _mm_sub_epi32(e0, o0);
        v1 = _mm_add_epi32(e1, o1);
        v6 = _mm_sub_epi32(e1, o1);
        v2 = _mm_add_epi32(e2, o2);
        v5 = _mm_sub_epi32(e2, o2);
        v3 = _mm_add_epi32(e3, o3);
        v4 = _mm_sub_epi32(e3, o3);

        OAPV_ITX_CLIP_SSE(v0, min_val, max_val);
        OAPV_ITX_CLIP_SSE(v1, min_val, max_val);
        OAPV_ITX_CLIP_SSE(v2, min_val, max_val);
        OAPV_ITX_CLIP_SSE(v3, min_val, max_val);
        OAPV_ITX_CLIP_SSE(v4, min_val, max_val);
        OAPV_ITX_CLIP_SSE(v5, min_val, max_val);
        OAPV_ITX_CLIP_SSE(v6, min_val, max_val);
        OAPV_ITX_CLIP_SSE(v7, min_val, max_val);

        t0 = _mm_unpacklo_epi32(v0, v1);
        t2 = _mm_unpackhi_epi32(v0, v1);
        t1 = _mm_unpacklo_epi32(v2, v3);
        t3 = _mm_unpackhi_epi32(v2, v3);

        v0 = _mm_unpacklo_epi64(t0, t1);
        v1 = _mm_unpackhi_epi64(t0, t1);
        v2 = _mm_unpacklo_epi64(t2, t3);
        v3 = _mm_unpackhi_epi64(t2, t3);

        t0 = _mm_unpacklo_epi32(v4, v5);
        t2 = _mm_unpackhi_epi32(v4, v5);
        t1 = _mm_unpacklo_epi32(v6, v7);
        t3 = _mm_unpackhi_epi32(v6, v7);

        v4 = _mm_unpacklo_epi64(t0, t1);
        v5 = _mm_unpackhi_epi64(t0, t1);
        v6 = _mm_unpacklo_epi64(t2, t3);
        v7 = _mm_unpackhi_epi64(t2, t3);

        _mm_storeu_si128((__m128i *)(pel_dst), v0);
        _mm_storeu_si128((__m128i *)(pel_dst + 4), v4);
        _mm_storeu_si128((__m128i *)(pel_dst + 8), v1);
        _mm_storeu_si128((__m128i *)(pel_dst + 12), v5);
        _mm_storeu_si128((__m128i *)(pel_dst + 16), v2);
        _mm_storeu_si128((__m128i *)(pel_dst + 20), v6);
        _mm_storeu_si128((__m128i *)(pel_dst + 24), v3);
        _mm_storeu_si128((__m128i *)(pel_dst + 28), v7);

        pel_dst += 32;
    }
}

const oapv_fn_itx_t oapv_tbl_fn_itx_neon[1] =
    {
        oapv_itx_pb8b_neon,
};

static int oapv_quant_nnz_simple_neon(u8 qp, int q_matrix[OAPV_BLOCK_H * OAPV_BLOCK_W], s16* coef, int log2_block_w, int log2_block_h,
    u16 scale, int ch_type, int bit_depth, int deadzone_offset)
{
    int nnz = 0;
    s32 offset;
    int shift;
    int tr_shift;

    int log2_size = (log2_block_w + log2_block_h) >> 1;
    tr_shift = MAX_TX_DYNAMIC_RANGE - bit_depth - log2_size;
    shift = QUANT_SHIFT + tr_shift + (qp / 6);
    offset = deadzone_offset << (shift - 9);
    int pixels=(1 << (log2_block_w + log2_block_h));

    int i;
    int32x4_t matrix_low ;
    int16x8_t coef_8_Val;
    int32x4_t offset_1 	= vdupq_n_s32(offset);
    int32x4_t offset_2 	= vdupq_n_s32(offset);
    int32x4_t shift_vector = vdupq_n_s32(-shift);
    uint16x8_t zero_vec            = vdupq_n_s16(0);


    for (i = 0; i < pixels; i+=8)
    {
        coef_8_Val 								= vld1q_s16(coef+i);
        uint16x8_t sign_mask            		= vcltq_s16(coef_8_Val, zero_vec);
        int16x8_t coef_8_Val_abs 				= vabsq_s16(coef_8_Val);

        int32x4_t coef_8_Val_act_low 			= vmovl_s16(vget_low_s16(coef_8_Val_abs));
        int32x4_t coef_8_Val_act_high 			= vmovl_s16(vget_high_s16(coef_8_Val_abs));

        int32x4_t matrix_low         			= vld1q_s32(q_matrix + i);
        int32x4_t matrix_high         		= vld1q_s32(q_matrix + i + 4);

                  coef_8_Val_act_low			= vmlaq_s32(offset_1, coef_8_Val_act_low, matrix_low);
                  coef_8_Val_act_high			= vmlaq_s32(offset_2, coef_8_Val_act_high, matrix_high);
                  coef_8_Val_act_low            = vshlq_s32(coef_8_Val_act_low, shift_vector);
                  coef_8_Val_act_high           = vshlq_s32(coef_8_Val_act_high, shift_vector);



        int16x8_t output_vector         		= vcombine_u16(vmovn_s32(coef_8_Val_act_low), vmovn_s32(coef_8_Val_act_high));
                  output_vector           		= vbslq_s16(sign_mask,  vnegq_s16(output_vector), output_vector);

        vst1q_s16(coef + i, output_vector);
    }
    return nnz;
}


const oapv_fn_quant_t oapv_tbl_quantb_neon[1] =
{
    oapv_quant_nnz_simple_neon,
};

static void oapv_dquant_simple_neon(s16 *coef, int q_matrix[OAPV_BLOCK_H * OAPV_BLOCK_W], int log2_w, int log2_h, int scale, s8 shift)
{
    int i;
    int pixels = (1 << (log2_w + log2_h));
    if (shift > 0)
    {
        int32x4_t shift_vector = vdupq_n_s32(-shift);
        s32 offset = (1 << (shift - 1));
        int16x8_t coef_8_Val;

        int32x4_t offset_1 = vdupq_n_s32(offset);
        int32x4_t offset_2 = vdupq_n_s32(offset);

        int32x4_t matrix_low = vdupq_n_s32(q_matrix[0]);

        for (i = 0; i < pixels; i += 8)
        {
            coef_8_Val = vld1q_s16(coef + i);
            int32x4_t matrix_low = vld1q_s32(q_matrix + i);
            int32x4_t matrix_high = vld1q_s32(q_matrix + i + 4);

            int32x4_t coef_8_Val_act_low = vmovl_s16(vget_low_s16(coef_8_Val));
            int32x4_t coef_8_Val_act_high = vmovl_s16(vget_high_s16(coef_8_Val));

            coef_8_Val_act_low = vmlaq_s32(offset_1, coef_8_Val_act_low, matrix_low);
            coef_8_Val_act_high = vmlaq_s32(offset_2, coef_8_Val_act_high, coef_8_Val_act_high);
            coef_8_Val_act_low = vshlq_s32(coef_8_Val_act_low, shift_vector);
            coef_8_Val_act_high = vshlq_s32(coef_8_Val_act_high, shift_vector);
            int16x8_t output_vector = vcombine_u16(vmovn_s32(coef_8_Val_act_low), vmovn_s32(coef_8_Val_act_high));

            vst1q_s16(coef + i, output_vector);
        }
    }
    else
    {
        int32x4_t shift_vector = vdupq_n_s32(shift);
        int16x8_t coef_8_Val;

        int32x4_t offset_1 = vdupq_n_s32(0);
        int32x4_t offset_2 = vdupq_n_s32(0);

        for (i = 0; i < pixels; i += 8)
        {
            coef_8_Val = vld1q_s16(coef + i);
            int32x4_t matrix_low = vld1q_s32(q_matrix + i);
            int32x4_t matrix_high = vld1q_s32(q_matrix + i + 4);

            int32x4_t coef_8_Val_act_low = vmovl_s16(vget_low_s16(coef_8_Val));
            int32x4_t coef_8_Val_act_high = vmovl_s16(vget_high_s16(coef_8_Val));

            coef_8_Val_act_low = vmlaq_s32(offset_1, coef_8_Val_act_low, matrix_low);
            coef_8_Val_act_high = vmlaq_s32(offset_2, coef_8_Val_act_high, matrix_high);
            coef_8_Val_act_low = vshlq_s32(coef_8_Val_act_low, shift_vector);
            coef_8_Val_act_high = vshlq_s32(coef_8_Val_act_high, shift_vector);
            int16x8_t output_vector = vcombine_u16(vmovn_s32(coef_8_Val_act_low), vmovn_s32(coef_8_Val_act_high));

            vst1q_s16(coef + i, output_vector);
        }
    }
}
const oapv_fn_iquant_t oapv_tbl_fn_iquant_neon[1] =
{
    oapv_dquant_simple_neon,
};
