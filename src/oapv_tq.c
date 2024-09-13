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


#include "oapv_tq.h"
#include <math.h>

///////////////////////////////////////////////////////////////////////////////
// start of encoder code
#if ENABLE_ENCODER
///////////////////////////////////////////////////////////////////////////////


const int oapv_quant_scale[6] = { 26214, 23302, 20560, 18396, 16384, 14769 };

void oapv_tx_pb8b(s16* src, s16* dst, int shift, int line)
{
    int j, k;
    int E[4], O[4];
    int EE[2], EO[2];
    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* E and O*/
        for (k = 0; k < 4; k++)
        {
            E[k] = src[j * 8 + k] + src[j * 8 + 7 - k];
            O[k] = src[j * 8 + k] - src[j * 8 + 7 - k];
        }
        /* EE and EO */
        EE[0] = E[0] + E[3];
        EO[0] = E[0] - E[3];
        EE[1] = E[1] + E[2];
        EO[1] = E[1] - E[2];

        dst[0 * line + j] = (oapv_tbl_tm8[0][0] * EE[0] + oapv_tbl_tm8[0][1] * EE[1] + add) >> shift;
        dst[4 * line + j] = (oapv_tbl_tm8[4][0] * EE[0] + oapv_tbl_tm8[4][1] * EE[1] + add) >> shift;
        dst[2 * line + j] = (oapv_tbl_tm8[2][0] * EO[0] + oapv_tbl_tm8[2][1] * EO[1] + add) >> shift;
        dst[6 * line + j] = (oapv_tbl_tm8[6][0] * EO[0] + oapv_tbl_tm8[6][1] * EO[1] + add) >> shift;

        dst[1 * line + j] = (oapv_tbl_tm8[1][0] * O[0] + oapv_tbl_tm8[1][1] * O[1] + oapv_tbl_tm8[1][2] * O[2] + oapv_tbl_tm8[1][3] * O[3] + add) >> shift;
        dst[3 * line + j] = (oapv_tbl_tm8[3][0] * O[0] + oapv_tbl_tm8[3][1] * O[1] + oapv_tbl_tm8[3][2] * O[2] + oapv_tbl_tm8[3][3] * O[3] + add) >> shift;
        dst[5 * line + j] = (oapv_tbl_tm8[5][0] * O[0] + oapv_tbl_tm8[5][1] * O[1] + oapv_tbl_tm8[5][2] * O[2] + oapv_tbl_tm8[5][3] * O[3] + add) >> shift;
        dst[7 * line + j] = (oapv_tbl_tm8[7][0] * O[0] + oapv_tbl_tm8[7][1] * O[1] + oapv_tbl_tm8[7][2] * O[2] + oapv_tbl_tm8[7][3] * O[3] + add) >> shift;
    }
}

const oapv_fn_tx_t oapv_tbl_fn_tx[1] =
{
    oapv_tx_pb8b,
};

static __inline int get_transform_shift(int log2_size, int type, int bit_depth)
{
    if (type == 0) {
        return ((log2_size) - 1 + bit_depth - 8);
    }
    else {
        return ((log2_size) + 6);
    }
}

void oapv_trans(oapve_ctx_t * ctx, s16 * coef, int log2_block_w, int log2_block_h, int bit_depth)
{
    int shift1 = get_transform_shift(log2_block_w, 0, bit_depth);
    int shift2 = get_transform_shift(log2_block_h, 1, bit_depth);

    ALIGNED_16(s16 tb[OAPV_BLOCK_D]);
    (ctx->fn_txb)[0](coef, tb, shift1, 1 << log2_block_h);
    (ctx->fn_txb)[0](tb, coef, shift2, 1 << log2_block_w);

}
int oapv_quant_nnz(u8 qp, int q_matrix[OAPV_BLOCK_H * OAPV_BLOCK_W], s16* coef, int log2_block_w, int log2_block_h,
    u16 scale, int ch_type, int bit_depth, int deadzone_offset)
{
    int nnz = 0;
    s32 lev;
    s32 offset;
    int sign;
    int i;
    int shift;
    int tr_shift;
    int log2_size = (log2_block_w + log2_block_h) >> 1;
    tr_shift = MAX_TX_DYNAMIC_RANGE - bit_depth - log2_size;
    shift = QUANT_SHIFT + tr_shift + (qp / 6);
    offset = deadzone_offset << (shift - 9);
    int pixels = (1 << (log2_block_w + log2_block_h));
    for (i = 0; i < pixels; i++)
    {
        sign = oapv_get_sign(coef[i]);
        lev = (s32)oapv_abs(coef[i]) * (q_matrix[i]);
        lev = (lev + offset) >> shift;
        lev = oapv_set_sign(lev, sign);
        coef[i] = (s16)(oapv_clip3(-32768, 32767, lev));
        nnz += !!(coef[i]);
    }
    return nnz;
}
const oapv_fn_quant_t oapv_tbl_fn_quant[1] =
{
    oapv_quant_nnz,
};

///////////////////////////////////////////////////////////////////////////////
// end of encoder code
#endif // ENABLE_ENCODER
///////////////////////////////////////////////////////////////////////////////

void oapv_itx_pb8b(s16* src, s16* dst, int shift, int line)
{
    int j, k;
    int E[4], O[4];
    int EE[2], EO[2];
    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
        for (k = 0; k < 4; k++)
        {
            O[k] = oapv_tbl_tm8[1][k] * src[1 * line + j] + oapv_tbl_tm8[3][k] * src[3 * line + j] + oapv_tbl_tm8[5][k] * src[5 * line + j] + oapv_tbl_tm8[7][k] * src[7 * line + j];
        }

        EO[0] = oapv_tbl_tm8[2][0] * src[2 * line + j] + oapv_tbl_tm8[6][0] * src[6 * line + j];
        EO[1] = oapv_tbl_tm8[2][1] * src[2 * line + j] + oapv_tbl_tm8[6][1] * src[6 * line + j];
        EE[0] = oapv_tbl_tm8[0][0] * src[0 * line + j] + oapv_tbl_tm8[4][0] * src[4 * line + j];
        EE[1] = oapv_tbl_tm8[0][1] * src[0 * line + j] + oapv_tbl_tm8[4][1] * src[4 * line + j];

        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
        E[0] = EE[0] + EO[0];
        E[3] = EE[0] - EO[0];
        E[1] = EE[1] + EO[1];
        E[2] = EE[1] - EO[1];

        for (k = 0; k < 4; k++)
        {
            dst[j * 8 + k]     = ((E[k] + O[k] + add) >> shift);
            dst[j * 8 + k + 4] = ((E[3 - k] - O[3 - k] + add) >> shift);
        }
    }
}

const oapv_fn_itx_t oapv_tbl_fn_itx[1] =
{
    oapv_itx_pb8b,
};

static void oapv_dquant(s16 *coef, int q_matrix[OAPV_BLOCK_H * OAPV_BLOCK_W], int log2_w, int log2_h, int scale, s8 shift)
{
    int i;
    int lev;
    int pixels = (1 << (log2_w + log2_h));

    if (shift > 0)
    {
        s32 offset = (1 << (shift - 1));
        for (i = 0; i < pixels; i++)
        {
            lev = (coef[i] * q_matrix[i] + offset) >> shift;
            coef[i] = (s16)oapv_clip3(-32768, 32767, lev);
        }
    }
    else
    {
        int left_shift = -shift;
        for (i = 0; i < pixels; i++)
        {
            lev = (coef[i] * q_matrix[i]) << left_shift;
            coef[i] = (s16)oapv_clip3(-32768, 32767, lev);
        }
    }
}
const oapv_fn_iquant_t oapv_tbl_fn_iquant[1] =
{
    oapv_dquant,
};
static void oapv_iquant_nnz_no_qp_matrix(s16 *coef, int q_matrix, s8 shift)
{
    int i;
    int lev;
    int pixels = 64;

    if (shift > 0)
    {
        s32 offset = (1 << (shift - 1));
        for (i = 0; i < pixels; i++)
        {
            lev = (coef[i] * q_matrix + offset) >> shift;
            coef[i] = (s16)oapv_clip3(-32768, 32767, lev);
        }
    }
    else
    {
        int left_shift = -shift;
        for (i = 0; i < pixels; i++)
        {
            lev = (coef[i] * q_matrix) << left_shift;
            coef[i] = (s16)oapv_clip3(-32768, 32767, lev);
        }
    }
}
const oapv_fn_iquant_t_no_qp_matrix oapv_tbl_fn_iquant_no_qp_matrix[1] =
{
    oapv_iquant_nnz_no_qp_matrix,
};

void oapv_itrans(const oapv_fn_itx_t* fn_itx, s16* coef, int log2_w, int log2_h, int bit_depth)
{
    ALIGNED_16(s16 tb[OAPV_BLOCK_D]); /* temp buffer */

    fn_itx[0](coef, tb, ITX_SHIFT1, 1 << log2_w);
    fn_itx[0](tb, coef, ITX_SHIFT2(bit_depth), 1 << log2_h);


}
void oapv_itdq_block(const oapv_fn_itx_t *fn_itx, const oapv_fn_iquant_t *fn_iquant, int q_matrix[OAPV_BLOCK_H  * OAPV_BLOCK_W], s16 *coef, int log2_w, int log2_h, int scale, int bit_depth, int qp)
{
    s8 shift;
    int log2_size = (log2_w + log2_h) >> 1;

    shift = bit_depth + log2_size - 5 - (qp / 6);
    fn_iquant[0](coef, q_matrix, log2_w, log2_h, scale, shift);
    oapv_itrans(fn_itx, coef, log2_w, log2_h, bit_depth);
}
