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

#if ARM_NEON
#include "sse2neon.h"

/* SAD for 16bit **************************************************************/
int sad_16b_neon_8x2n(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    __m128i src_8x16b;
    __m128i src_8x16b_1;

    __m128i pred_8x16b;
    __m128i pred_8x16b_1;

    __m128i temp;
    __m128i temp_1;
    __m128i temp_2;

    __m128i temp_dummy;
    __m128i result;

    short *pu2_inp, *pu2_inp2;
    short *pu2_ref, *pu2_ref2;

    int i, j;
    int sad = 0;
    int s_src1_t2 = s_src1 * 2;
    int s_src2_t2 = s_src2 * 2;

    assert(bit_depth <= 14);
    assert(!(w & 7)); /* width has to be multiple of 4  */
    assert(!(h & 3)); /* height has to be multiple of 4 */

    pu2_inp = src1;
    pu2_ref = src2;
    pu2_inp2 = (short *)src1 + s_src1;
    pu2_ref2 = (short *)src2 + s_src2;

    temp_dummy = _mm_setzero_si128();
    result = _mm_setzero_si128();

    for (i = 0; i < h >> 1; i++)
    {
        for (j = 0; j < w; j += 8)
        {
            src_8x16b = _mm_loadu_si128((__m128i *)(&pu2_inp[j]));
            src_8x16b_1 = _mm_loadu_si128((__m128i *)(&pu2_inp2[j]));

            pred_8x16b = _mm_loadu_si128((__m128i *)(&pu2_ref[j]));
            pred_8x16b_1 = _mm_loadu_si128((__m128i *)(&pu2_ref2[j]));

            temp = _mm_sub_epi16(src_8x16b, pred_8x16b);
            temp_1 = _mm_sub_epi16(src_8x16b_1, pred_8x16b_1);

            temp = _mm_abs_epi16(temp);
            temp_1 = _mm_abs_epi16(temp_1);

            temp = _mm_add_epi16(temp, temp_1);

            temp_1 = _mm_unpackhi_epi16(temp, temp_dummy);
            temp_2 = _mm_unpacklo_epi16(temp, temp_dummy);

            temp = _mm_add_epi32(temp_1, temp_2);
            result = _mm_add_epi32(result, temp);
        }

        pu2_inp += s_src1_t2;
        pu2_ref += s_src2_t2;
        pu2_inp2 += s_src1_t2;
        pu2_ref2 += s_src2_t2;
    }

    result = _mm_hadd_epi32(result, result);
    result = _mm_hadd_epi32(result, result);
    sad = _mm_extract_epi32(result, 0);

    return (sad);
}

const oapv_fn_sad_t oapv_tbl_sad_16b_neon[2] =
    {
        sad_16b_neon_8x2n,
            NULL
};

/* DIFF **********************************************************************/
#define SSE_DIFF_16B_8PEL(src1, src2, diff, m00, m01, m02) \
    m00 = _mm_loadu_si128((__m128i *)(src1));              \
    m01 = _mm_loadu_si128((__m128i *)(src2));              \
    m02 = _mm_sub_epi16(m00, m01);                         \
    _mm_storeu_si128((__m128i *)(diff), m02);

static void diff_16b_neon_8x8(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int s_diff, s16 *diff, int bit_depth)
{
    s16 *s1;
    s16 *s2;
    __m128i m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
    SSE_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
    SSE_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
    SSE_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
    SSE_DIFF_16B_8PEL(s1 + s_src1 * 4, s2 + s_src2 * 4, diff + s_diff * 4, m01, m02, m03);
    SSE_DIFF_16B_8PEL(s1 + s_src1 * 5, s2 + s_src2 * 5, diff + s_diff * 5, m04, m05, m06);
    SSE_DIFF_16B_8PEL(s1 + s_src1 * 6, s2 + s_src2 * 6, diff + s_diff * 6, m07, m08, m09);
    SSE_DIFF_16B_8PEL(s1 + s_src1 * 7, s2 + s_src2 * 7, diff + s_diff * 7, m10, m11, m12);
}
const oapv_fn_diff_t oapv_tbl_diff_16b_neon[2] =
    {
        diff_16b_neon_8x8,
            NULL};

/* SSD ***********************************************************************/
#define SSE_SSD_16B_8PEL(src1, src2, shift, s00, s01, s02, s00a) \
    s00 = _mm_loadu_si128((__m128i *)(src1));                    \
    s01 = _mm_loadu_si128((__m128i *)(src2));                    \
    s02 = _mm_sub_epi16(s00, s01);                               \
                                                                 \
    s00 = _mm_cvtepi16_epi32(s02);                               \
    s00 = _mm_mullo_epi32(s00, s00);                             \
                                                                 \
    s01 = _mm_srli_si128(s02, 8);                                \
    s01 = _mm_cvtepi16_epi32(s01);                               \
    s01 = _mm_mullo_epi32(s01, s01);                             \
                                                                 \
    s00 = _mm_srli_epi32(s00, shift);                            \
    s01 = _mm_srli_epi32(s01, shift);                            \
    s00a = _mm_add_epi32(s00a, s00);                             \
    s00a = _mm_add_epi32(s00a, s01);

static s64 ssd_16b_neon_8x8(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s64 ssd;
    s16 *s1;
    s16 *s2;
    const int shift = 0;
    __m128i s00, s01, s02, s00a;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    s00a = _mm_setzero_si128();

    SSE_SSD_16B_8PEL(s1, s2, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 4, s2 + s_src2 * 4, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 5, s2 + s_src2 * 5, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 6, s2 + s_src2 * 6, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 7, s2 + s_src2 * 7, shift, s00, s01, s02, s00a);

    ssd = _mm_extract_epi32(s00a, 0);
    ssd += _mm_extract_epi32(s00a, 1);
    ssd += _mm_extract_epi32(s00a, 2);
    ssd += _mm_extract_epi32(s00a, 3);

    return ssd;
}

const oapv_fn_ssd_t oapv_tbl_ssd_16b_neon[2] =
    {
        ssd_16b_neon_8x8,
            NULL};
int oapv_dc_removed_had8x8_neon(pel* org, int s_org)
{
    int satd = 0;
    /* all 128 bit registers are named with a suffix mxnb, where m is the */
    /* number of n bits packed in the register                            */

    int16x8_t src0_8x16b, src1_8x16b, src2_8x16b, src3_8x16b;
    int16x8_t src4_8x16b, src5_8x16b, src6_8x16b, src7_8x16b;
    int16x8_t pred0_8x16b, pred1_8x16b, pred2_8x16b, pred3_8x16b;
    int16x8_t pred4_8x16b, pred5_8x16b, pred6_8x16b, pred7_8x16b;
    int16x8_t out0_8x16b, out1_8x16b, out2_8x16b, out3_8x16b;
    int16x8_t out4_8x16b, out5_8x16b, out6_8x16b, out7_8x16b;
    int16x8x2_t out0_8x16bx2, out1_8x16bx2, out2_8x16bx2, out3_8x16bx2;

    src0_8x16b = (vld1q_s16(&org[0]));
    org = org + s_org;
    src1_8x16b = (vld1q_s16(&org[0]));
    org = org + s_org;
    src2_8x16b = (vld1q_s16(&org[0]));
    org = org + s_org;
    src3_8x16b = (vld1q_s16(&org[0]));
    org = org + s_org;
    src4_8x16b = (vld1q_s16(&org[0]));
    org = org + s_org;
    src5_8x16b = (vld1q_s16(&org[0]));
    org = org + s_org;
    src6_8x16b = (vld1q_s16(&org[0]));
    org = org + s_org;
    src7_8x16b = (vld1q_s16(&org[0]));
    org = org + s_org;

    /**************** 8x8 horizontal transform *******************************/
    /***********************    8x8 16 bit Transpose  ************************/

    out3_8x16b = vcombine_s16(vget_low_s16(src0_8x16b), vget_low_s16(src1_8x16b));
    out7_8x16b = vcombine_s16(vget_high_s16(src0_8x16b), vget_high_s16(src1_8x16b));

    pred0_8x16b = vcombine_s16(vget_low_s16(src2_8x16b), vget_low_s16(src3_8x16b));
    src2_8x16b = vcombine_s16(vget_high_s16(src2_8x16b), vget_high_s16(src3_8x16b));

    out2_8x16b = vcombine_s16(vget_low_s16(src4_8x16b), vget_low_s16(src5_8x16b));
    pred7_8x16b = vcombine_s16(vget_high_s16(src4_8x16b), vget_high_s16(src5_8x16b));

    pred3_8x16b = vcombine_s16(vget_low_s16(src6_8x16b), vget_low_s16(src7_8x16b));
    src6_8x16b = vcombine_s16(vget_high_s16(src6_8x16b), vget_high_s16(src7_8x16b));


    out1_8x16b = vzip1q_s32(out3_8x16b, pred0_8x16b);
    out3_8x16b = vzip2q_s32(out3_8x16b, pred0_8x16b);

    pred1_8x16b = vzip1q_s32(out2_8x16b, pred3_8x16b);
    pred3_8x16b = vzip2q_s32(out2_8x16b, pred3_8x16b);

    out5_8x16b = vzip1q_s32(out7_8x16b, src2_8x16b);
    out7_8x16b = vzip2q_s32(out7_8x16b, src2_8x16b);

    pred5_8x16b = vzip1q_s32(pred7_8x16b, src6_8x16b);
    pred7_8x16b = vzip2q_s32(pred7_8x16b, src6_8x16b);

    out0_8x16b = vzip1q_s64(out1_8x16b,pred1_8x16b);
    out1_8x16b = vzip2q_s64(out1_8x16b,pred1_8x16b);
    out2_8x16b = vzip1q_s64(out3_8x16b,pred3_8x16b);
    out3_8x16b = vzip2q_s64(out3_8x16b,pred3_8x16b);
    out4_8x16b = vzip1q_s64(out5_8x16b,pred5_8x16b);
    out5_8x16b = vzip2q_s64(out5_8x16b,pred5_8x16b);
    out6_8x16b = vzip1q_s64(out7_8x16b,pred7_8x16b);
    out7_8x16b = vzip2q_s64(out7_8x16b,pred7_8x16b);

    /**********************   8x8 16 bit Transpose End   *********************/

    /* r0 + r1 */
    pred0_8x16b = vaddq_s16(out0_8x16b, out1_8x16b);
    /* r2 + r3 */
    pred2_8x16b = vaddq_s16(out2_8x16b, out3_8x16b);
    /* r4 + r5 */
    pred4_8x16b = vaddq_s16(out4_8x16b, out5_8x16b);
    /* r6 + r7 */
    pred6_8x16b = vaddq_s16(out6_8x16b, out7_8x16b);


    /* r0 + r1 + r2 + r3 */
    pred1_8x16b = vaddq_s16(pred0_8x16b, pred2_8x16b);
    /* r4 + r5 + r6 + r7 */
    pred5_8x16b = vaddq_s16(pred4_8x16b, pred6_8x16b);
    /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
    src0_8x16b = vaddq_s16(pred1_8x16b, pred5_8x16b);
    /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
    src4_8x16b = vsubq_s16(pred1_8x16b, pred5_8x16b);

    /* r0 + r1 - r2 - r3 */
    pred1_8x16b = vsubq_s16(pred0_8x16b, pred2_8x16b);
    /* r4 + r5 - r6 - r7 */
    pred5_8x16b = vsubq_s16(pred4_8x16b, pred6_8x16b);
    /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
    src2_8x16b = vaddq_s16(pred1_8x16b, pred5_8x16b);
    /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
    src6_8x16b = vsubq_s16(pred1_8x16b, pred5_8x16b);

    /* r0 - r1 */
    pred0_8x16b = vsubq_s16(out0_8x16b, out1_8x16b);
    /* r2 - r3 */
    pred2_8x16b = vsubq_s16(out2_8x16b, out3_8x16b);
    /* r4 - r5 */
    pred4_8x16b = vsubq_s16(out4_8x16b, out5_8x16b);
    /* r6 - r7 */
    pred6_8x16b = vsubq_s16(out6_8x16b, out7_8x16b);

    /* r0 - r1 + r2 - r3 */
    pred1_8x16b = vaddq_s16(pred0_8x16b, pred2_8x16b);
    /* r4 - r5 + r6 - r7 */
    pred5_8x16b = vaddq_s16(pred4_8x16b, pred6_8x16b);
    /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
    src1_8x16b = vaddq_s16(pred1_8x16b, pred5_8x16b);
    /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
    src5_8x16b = vsubq_s16(pred1_8x16b, pred5_8x16b);

    /* r0 - r1 - r2 + r3 */
    pred1_8x16b = vsubq_s16(pred0_8x16b, pred2_8x16b);
    /* r4 - r5 - r6 + r7 */
    pred5_8x16b = vsubq_s16(pred4_8x16b, pred6_8x16b);
    /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
    src3_8x16b = vaddq_s16(pred1_8x16b, pred5_8x16b);
    /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
    src7_8x16b = vsubq_s16(pred1_8x16b, pred5_8x16b);


    /***********************    8x8 16 bit Transpose  ************************/
    out3_8x16b = vzip1q_s16(src0_8x16b, src1_8x16b);
    pred0_8x16b = vzip1q_s16(src2_8x16b, src3_8x16b);
    out2_8x16b = vzip1q_s16(src4_8x16b, src5_8x16b);
    pred3_8x16b = vzip1q_s16(src6_8x16b, src7_8x16b);
    out7_8x16b = vzip2q_s16(src0_8x16b, src1_8x16b);
    src2_8x16b = vzip2q_s16(src2_8x16b, src3_8x16b);
    pred7_8x16b = vzip2q_s16(src4_8x16b, src5_8x16b);
    src6_8x16b = vzip2q_s16(src6_8x16b, src7_8x16b);

    out1_8x16b = vzip1q_s32(out3_8x16b, pred0_8x16b);
    out3_8x16b = vzip2q_s32(out3_8x16b, pred0_8x16b);

    pred1_8x16b = vzip1q_s32(out2_8x16b, pred3_8x16b);
    pred3_8x16b = vzip2q_s32(out2_8x16b, pred3_8x16b);

    out5_8x16b = vzip1q_s32(out7_8x16b, src2_8x16b);
    out7_8x16b = vzip2q_s32(out7_8x16b, src2_8x16b);

    pred5_8x16b = vzip1q_s32(pred7_8x16b, src6_8x16b);
    pred7_8x16b = vzip2q_s32(pred7_8x16b, src6_8x16b);

    src0_8x16b = vzip1q_s64(out1_8x16b,pred1_8x16b);
    src1_8x16b = vzip2q_s64(out1_8x16b,pred1_8x16b);
    src2_8x16b = vzip1q_s64(out3_8x16b,pred3_8x16b);
    src3_8x16b = vzip2q_s64(out3_8x16b,pred3_8x16b);
    src4_8x16b = vzip1q_s64(out5_8x16b,pred5_8x16b);
    src5_8x16b = vzip2q_s64(out5_8x16b,pred5_8x16b);
    src6_8x16b = vzip1q_s64(out7_8x16b,pred7_8x16b);
    src7_8x16b = vzip2q_s64(out7_8x16b,pred7_8x16b);

    /**********************   8x8 16 bit Transpose End   *********************/
    /**************** 8x8 horizontal transform *******************************/
    {
        int16x8_t out0a_8x16b, out1a_8x16b, out2a_8x16b, out3a_8x16b;
        int16x8_t out4a_8x16b, out5a_8x16b, out6a_8x16b, out7a_8x16b;
        int16x8_t tmp0_8x16b, tmp1_8x16b, tmp2_8x16b, tmp3_8x16b;
        int16x8_t tmp4_8x16b, tmp5_8x16b, tmp6_8x16b, tmp7_8x16b;

        /************************* 8x8 Vertical Transform*************************/
        tmp0_8x16b = vcombine_s16(vget_high_s16(src0_8x16b), vcreate_s32(0));
        tmp1_8x16b = vcombine_s16(vget_high_s16(src1_8x16b), vcreate_s32(0));
        tmp2_8x16b = vcombine_s16(vget_high_s16(src2_8x16b), vcreate_s32(0));
        tmp3_8x16b = vcombine_s16(vget_high_s16(src3_8x16b), vcreate_s32(0));
        tmp4_8x16b = vcombine_s16(vget_high_s16(src4_8x16b), vcreate_s32(0));
        tmp5_8x16b = vcombine_s16(vget_high_s16(src5_8x16b), vcreate_s32(0));
        tmp6_8x16b = vcombine_s16(vget_high_s16(src6_8x16b), vcreate_s32(0));
        tmp7_8x16b = vcombine_s16(vget_high_s16(src7_8x16b), vcreate_s32(0));

        /*************************First 4 pixels ********************************/

        src0_8x16b = vmovl_s16(vget_low_s16(src0_8x16b));
        src1_8x16b = vmovl_s16(vget_low_s16(src1_8x16b));
        src2_8x16b = vmovl_s16(vget_low_s16(src2_8x16b));
        src3_8x16b = vmovl_s16(vget_low_s16(src3_8x16b));
        src4_8x16b = vmovl_s16(vget_low_s16(src4_8x16b));
        src5_8x16b = vmovl_s16(vget_low_s16(src5_8x16b));
        src6_8x16b = vmovl_s16(vget_low_s16(src6_8x16b));
        src7_8x16b = vmovl_s16(vget_low_s16(src7_8x16b));

        /* r0 + r1 */
        pred0_8x16b = vaddq_s32(src0_8x16b, src1_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s32(src2_8x16b, src3_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s32(src4_8x16b, src5_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s32(src6_8x16b, src7_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out0_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out4_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out2_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out6_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s32(src0_8x16b, src1_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s32(src2_8x16b, src3_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s32(src4_8x16b, src5_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s32(src6_8x16b, src7_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out1_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out5_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out3_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out7_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /*************************First 4 pixels ********************************/

        /**************************Next 4 pixels *******************************/
        src0_8x16b = vmovl_s16(vget_low_s16(tmp0_8x16b));
        src1_8x16b = vmovl_s16(vget_low_s16(tmp1_8x16b));
        src2_8x16b = vmovl_s16(vget_low_s16(tmp2_8x16b));
        src3_8x16b = vmovl_s16(vget_low_s16(tmp3_8x16b));
        src4_8x16b = vmovl_s16(vget_low_s16(tmp4_8x16b));
        src5_8x16b = vmovl_s16(vget_low_s16(tmp5_8x16b));
        src6_8x16b = vmovl_s16(vget_low_s16(tmp6_8x16b));
        src7_8x16b = vmovl_s16(vget_low_s16(tmp7_8x16b));

        /* r0 + r1 */
        pred0_8x16b = vaddq_s32(src0_8x16b, src1_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s32(src2_8x16b, src3_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s32(src4_8x16b, src5_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s32(src6_8x16b, src7_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out0a_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out4a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out2a_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out6a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s32(src0_8x16b, src1_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s32(src2_8x16b, src3_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s32(src4_8x16b, src5_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s32(src6_8x16b, src7_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out1a_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out5a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out3a_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out7a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /**************************Next 4 pixels *******************************/
        /************************* 8x8 Vertical Transform*************************/

        /****************************SATD calculation ****************************/
        src0_8x16b = vabsq_s32(out0_8x16b);
        src1_8x16b = vabsq_s32(out1_8x16b);
        src2_8x16b = vabsq_s32(out2_8x16b);
        src3_8x16b = vabsq_s32(out3_8x16b);
        src4_8x16b = vabsq_s32(out4_8x16b);
        src5_8x16b = vabsq_s32(out5_8x16b);
        src6_8x16b = vabsq_s32(out6_8x16b);
        src7_8x16b = vabsq_s32(out7_8x16b);
        s32* p = (s32*)&src0_8x16b;
        p[0] = 0;

        satd = vaddvq_s32(src0_8x16b);
        satd += vaddvq_s32(src1_8x16b);
        satd += vaddvq_s32(src2_8x16b);
        satd += vaddvq_s32(src3_8x16b);
        satd += vaddvq_s32(src4_8x16b);
        satd += vaddvq_s32(src5_8x16b);
        satd += vaddvq_s32(src6_8x16b);
        satd += vaddvq_s32(src7_8x16b);

        src0_8x16b = vabsq_s32(out0a_8x16b);
        src1_8x16b = vabsq_s32(out1a_8x16b);
        src2_8x16b = vabsq_s32(out2a_8x16b);
        src3_8x16b = vabsq_s32(out3a_8x16b);
        src4_8x16b = vabsq_s32(out4a_8x16b);
        src5_8x16b = vabsq_s32(out5a_8x16b);
        src6_8x16b = vabsq_s32(out6a_8x16b);
        src7_8x16b = vabsq_s32(out7a_8x16b);

        satd += vaddvq_s32(src0_8x16b);
        satd += vaddvq_s32(src1_8x16b);
        satd += vaddvq_s32(src2_8x16b);
        satd += vaddvq_s32(src3_8x16b);
        satd += vaddvq_s32(src4_8x16b);
        satd += vaddvq_s32(src5_8x16b);
        satd += vaddvq_s32(src6_8x16b);
        satd += vaddvq_s32(src7_8x16b);

        satd = (satd + 2) >> 2;
        return satd;
    }
}
#endif /* X86_SSE */
