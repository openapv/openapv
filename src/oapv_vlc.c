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
#include "oapv_metadata.h"
#include <limits.h>
#include <math.h>
#include <inttypes.h>

///////////////////////////////////////////////////////////////////////////////
// start of encoder code
#if ENABLE_ENCODER
///////////////////////////////////////////////////////////////////////////////

static inline void enc_vlc_write(oapv_bs_t* bs, int coef, int k)
{
    const s32 simple_vlc_table[3][2] = { {1,  },
                                         {0, 0},
                                         {0, 1} };
    u32 symbol = coef;
    u32 simple_vlc_val = oapv_clip3(0, 2, symbol >> k);
    int bit_cnt = 0;
    if (bs->is_bin_count)
    {
        bs->bin_count += coef;
        return;
     }
    if (symbol >= (u32)(1 << k))
    {
        symbol -= (1 << k);
        int val = simple_vlc_table[simple_vlc_val][bit_cnt];
        bs->leftbits--;
        bs->code |= ((val & 0x1) << bs->leftbits);
        if (bs->leftbits == 0)
        {
            OAPV_FLUSH(bs);
        }
        bit_cnt++;
    }
    if (symbol >= (u32)(1 << k) && simple_vlc_val>0)
    {
        symbol -= (1 << k);
        int val = simple_vlc_table[simple_vlc_val][bit_cnt];
        bs->leftbits--;
        bs->code |= ((val & 0x1) << bs->leftbits);
        if (bs->leftbits == 0)
        {
            OAPV_FLUSH(bs);
        }
        bit_cnt++;
    }
    while (symbol >= (u32)(1 << k))
    {
        symbol -= (1 << k);
        bs->leftbits--;
        if (bs->leftbits == 0)
        {
           OAPV_FLUSH(bs);
        }
        if( bit_cnt >= 2)
        {
            k++;
        }
        bit_cnt++;
    }
    if (bit_cnt < 2)
    {
        int val = simple_vlc_table[simple_vlc_val][bit_cnt];
        bs->leftbits--;
        bs->code |= ((val & 0x1) << bs->leftbits);
        if (bs->leftbits == 0)
        {
            OAPV_FLUSH(bs);
        }
    }
    else
    {
        bs->leftbits--;
        bs->code |= ((1 & 0x1) << bs->leftbits);
        if (bs->leftbits == 0)
        {
           OAPV_FLUSH(bs);
        }
    }
    if (k > 0)
    {
        int leftbits;
        leftbits = bs->leftbits;
        symbol <<= (32 - k);
        bs->code |= (symbol >> (32 - leftbits));
        if(k < leftbits)
        {
            bs->leftbits -= k;
        }
        else
        {
            bs->leftbits = 0;
            OAPV_FLUSH(bs);
            bs->code = (leftbits < 32 ? symbol << leftbits : 0);
            bs->leftbits = 32 - (k - leftbits);
        }
    }
}

static void inline bsr_skip_code_opt(oapv_bs_t* bs, int size)
{

    if (size == 32) {
        bs->code = 0;
        bs->leftbits = 0;
    }
    else {
        bs->code <<= size;
        bs->leftbits -= size;
    }
}
static int dec_vlc_read_1bit_read(oapv_bs_t* bs, int k)
{
    u32 symbol = 0;
    int t0 = -1;
    int parse_exp_golomb = 1;
    if (bs->leftbits == 0)
    {
        OAPV_READ_FLUSH(bs, 4);
    }
    t0 = (u32)(bs->code >> 31);
    bs->code <<= 1;
    bs->leftbits -= 1;
    if (t0 == 0)
    {
        symbol += (1 << k);
        parse_exp_golomb = 0;
    }
    else
    {
        symbol += (2 << k);
        parse_exp_golomb = 1;
    }
    if (parse_exp_golomb)
    {
        while (1)
        {
            if (bs->leftbits == 0)
            {
                OAPV_READ_FLUSH(bs, 4);
            }
            t0 = (u32)(bs->code >> 31);
            bs->code <<= 1;
            bs->leftbits -= 1;
            if (t0 == 1)
            {
                break;
            }
            else
            {
                symbol += (1 << k);
                k++;
            }
        }
    }
    if (k > 0)
    {
        u32 code = 0;
        if (bs->leftbits < k)
        {
            code = bs->code >> (32 - k);
            k -= bs->leftbits;
            OAPV_READ_FLUSH(bs, 4);
        }
        code |= bs->code >> (32 - k);
        bsr_skip_code_opt(bs, k);
        symbol += code;
    }
    return symbol;
}
static int dec_vlc_read(oapv_bs_t* bs, int k)
{
    u32 symbol = 0;
    int t0 = -1;
    int parse_exp_golomb = 1;
    if (bs->leftbits == 0)
    {
        OAPV_READ_FLUSH(bs, 4);
    }
    t0 = (u32)(bs->code >> 31);
    bs->code <<= 1;
    bs->leftbits -= 1;
    if (t0 == 1)
    {
        parse_exp_golomb = 0;
    }
    else
    {
        if (bs->leftbits == 0)
        {
            OAPV_READ_FLUSH(bs, 4);
        }
        t0 = (u32)(bs->code >> 31);
        bs->code <<= 1;
        bs->leftbits -= 1;
        if (t0 == 0)
        {
            symbol += (1 << k);
            parse_exp_golomb = 0;
        }
        else
        {
            symbol += (2 << k);
            parse_exp_golomb = 1;
        }
    }
    if (parse_exp_golomb)
    {
        while (1)
        {
            if (bs->leftbits == 0)
            {
                OAPV_READ_FLUSH(bs, 4);
            }
            t0 = (u32)(bs->code >> 31);
            bs->code <<= 1;
            bs->leftbits -= 1;
            if (t0 == 1)
            {
                break;
            }
            else
            {
                symbol += (1 << k);
                k++;
            }
        }
    }
    if (k > 0)
    {
        u32 code = 0;
        if (bs->leftbits < k)
        {
            code = bs->code >> (32 - k);
            k -= bs->leftbits;
            OAPV_READ_FLUSH(bs, 4);
        }
        code |= bs->code >> (32 - k);
        bsr_skip_code_opt(bs, k);
        symbol += code;
    }
    return symbol;
}

void oapve_set_frame_header(oapve_ctx_t * ctx, oapv_fh_t * fh)
{
    oapv_mset(fh, 0, sizeof(oapv_fh_t));
    fh->fi.profile_idc = ctx->param->profile_idc;
    fh->fi.level_idc = ctx->param->level_idc;
    fh->fi.band_idc = ctx->param->band_idc;
    fh->fi.frame_width  = ctx->param->w;
    fh->fi.frame_height = ctx->param->h;
    fh->fi.chroma_format_idc = ctx->cfi;
    fh->fi.bit_depth = OAPV_CS_GET_BIT_DEPTH(ctx->imgb->cs);
    fh->tile_width_in_mbs  = ctx->param->tile_w_mb;
    fh->tile_height_in_mbs = ctx->param->tile_h_mb;
    if(fh->color_description_present_flag == 0)
    {
        fh->color_primaries = 2;
        fh->transfer_characteristics = 2;
        fh->matrix_coefficients = 2;
    }
    fh->use_q_matrix = ctx->param->use_q_matrix;
    if (fh->use_q_matrix == 0)
    {
         for (int cidx = 0; cidx < ctx->num_comp; cidx++)
        {
            for (int y = 0; y < OAPV_BLOCK_H; y++)
            {
                for (int x = 0; x < OAPV_BLOCK_W; x++)
                {
                    fh->q_matrix[cidx][y][x] = 16;
                }
            }
        }
    }
    else
    {
        int mod = (1 << OAPV_LOG2_BLOCK) - 1;
        for (int i = 0; i < OAPV_BLOCK_D; i++)
        {
            fh->q_matrix[Y_C][i >> OAPV_LOG2_BLOCK][i & mod] = ctx->param->q_matrix_y[i];
            fh->q_matrix[U_C][i >> OAPV_LOG2_BLOCK][i & mod] = ctx->param->q_matrix_u[i];
            fh->q_matrix[V_C][i >> OAPV_LOG2_BLOCK][i & mod] = ctx->param->q_matrix_v[i];
        }
    }
    int cnt = 0;
    fh->tile_size_present_in_fh_flag = 0;
}

static int enc_vlc_quantization_matrix(oapv_bs_t* bs, oapve_ctx_t* ctx, oapv_fh_t* fh)
{
    for (int cidx = 0; cidx < ctx->num_comp; cidx++)
    {
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x++)
            {
                oapv_bsw_write(bs, fh->q_matrix[cidx][y][x] - 1, 8);
            }
        }
    }
    return 0;
}

static int enc_vlc_tile_info(oapv_bs_t* bs, oapve_ctx_t* ctx, oapv_fh_t* fh)
{
    oapv_bsw_write(bs, fh->tile_width_in_mbs - 1,       28);
    oapv_bsw_write(bs, fh->tile_height_in_mbs - 1,      28);
    oapv_bsw_write(bs, fh->tile_size_present_in_fh_flag, 1);
    if (fh->tile_size_present_in_fh_flag)
    {
        for (int i = 0; i < ctx->num_tiles; i++)
        {
            oapv_bsw_write(bs, fh->tile_size[i] - 1,    32);
        }
    }

    return 0;
}

int oapve_vlc_frame_info(oapv_bs_t* bs, oapv_fi_t* fi)
{
    oapv_bsw_write(bs, fi->profile_idc, 8);
    oapv_bsw_write(bs, fi->level_idc, 8);
    oapv_bsw_write(bs, fi->band_idc, 3);
    oapv_bsw_write(bs, 0, 5); // reserved_zero_5bits
    oapv_bsw_write(bs, fi->frame_width - 1, 32);
    oapv_bsw_write(bs, fi->frame_height - 1, 32);
    oapv_bsw_write(bs, fi->chroma_format_idc, 4);
    oapv_bsw_write(bs, fi->bit_depth - 8, 4);
    oapv_bsw_write(bs, fi->capture_time_distance, 8);
    oapv_bsw_write(bs, 0, 8); // reserved_zero_8bits
    return OAPV_OK;
}

int oapve_vlc_frame_header(oapv_bs_t* bs, oapve_ctx_t* ctx, oapv_fh_t* fh)
{
    oapv_assert_rv(oapv_bsw_is_align8(bs), OAPV_ERR_MALFORMED_BITSTREAM);

    oapve_vlc_frame_info(bs, &fh->fi);
    oapv_bsw_write(bs, 0, 8); // reserved_zero_8bits
    oapv_bsw_write(bs, fh->color_description_present_flag,   1);
    if(fh->color_description_present_flag)
    {
        oapv_bsw_write(bs, fh->color_primaries,              8);
        oapv_bsw_write(bs, fh->transfer_characteristics,     8);
        oapv_bsw_write(bs, fh->matrix_coefficients,          8);
    }
    oapv_bsw_write(bs, fh->use_q_matrix,                     1);
    if (fh->use_q_matrix)
    {
      enc_vlc_quantization_matrix(bs, ctx, fh);
    }
    enc_vlc_tile_info(bs, ctx, fh);

    oapv_bsw_write(bs, 0, 8); // reserved_zero_8bits
    return OAPV_OK;
}


int oapve_vlc_tile_size(oapv_bs_t* bs, int tile_size)
{
    u8* bs_cur;
    OAPV_TRACE_SET(1);
    oapv_assert_rv(oapv_bsw_is_align8(bs), OAPV_ERR_MALFORMED_BITSTREAM);
    bs_cur = oapv_bsw_sink(bs); // write bits to bitstream buffer
    oapv_bsw_write(bs, tile_size - 1, 32);
    return OAPV_OK;
}

void oapve_set_tile_header(oapve_ctx_t* ctx, oapv_th_t* th, int tile_idx, int qp)
{
    oapv_mset(th, 0, sizeof(oapv_th_t));
    for (int c = 0; c < ctx->num_comp; c++)
    {
      th->tile_qp[c] = qp;
      if (c == 1)
      {
          th->tile_qp[c] += ctx->param->qp_cb_offset;
      }
      else if (c == 2)
      {
          th->tile_qp[c] += ctx->param->qp_cr_offset;
      }
    }
    th->tile_index = tile_idx;
}

int oapve_vlc_tile_header(oapve_ctx_t* ctx, oapv_bs_t* bs, oapv_th_t* th)
{
    u8* bs_cur;
    OAPV_TRACE_SET(1);
    oapv_assert_rv(oapv_bsw_is_align8(bs), OAPV_ERR_MALFORMED_BITSTREAM);
    bs_cur = oapv_bsw_sink(bs); // write bits to bitstream buffer
    oapv_bsw_write(bs, 0, 16); // header_size will be written properly, later
    oapv_bsw_write(bs, th->tile_index, 16);
    for (int c = 0; c < ctx->num_comp; c++)
    {
        oapv_bsw_write(bs, th->tile_data_size[c] - 1, 32);
    }
    for (int c = 0; c < ctx->num_comp; c++)
    {
        oapv_bsw_write(bs, th->tile_qp[c], 8);
    }
    oapv_bsw_write(bs, th->reserved_zero_8bits, 8);
    // with byte align
    th->tile_header_size = (int)((u8*)oapv_bsw_sink(bs) - bs_cur);

    oapv_bsw_write_direct(bs_cur, th->tile_header_size, 16);
    return OAPV_OK;
}

void oapve_vlc_run_length_cc(oapve_ctx_t* ctx, oapve_core_t * core, oapv_bs_t *bs, s16 *coef, int log2_w, int log2_h, int num_sig, int ch_type)
{
    u32             num_coeff, scan_pos;
    u32             sign, level, prev_level, run;
    const u16     * scanp;
    s16             coef_cur;
    int             ctx_last = 0;

    scanp = oapv_tbl_scan;
    num_coeff = 1 << (log2_w + log2_h);
    run = 0;
    int first_ac = 1;
    prev_level = core->prev_1st_ac_ctx[ch_type];

    int prev_run = 0;
    OAPV_TRACE_SET(0);

    int rice_level = 0;
    scan_pos = 0;

    // for DC
    {
        coef_cur = coef[scanp[scan_pos]];
        level = oapv_abs16(coef_cur);
        sign = (coef_cur > 0) ? 0 : 1;

        rice_level = oapv_clip3(OAPV_MIN_DC_LEVEL_CTX, OAPV_MAX_DC_LEVEL_CTX, core->prev_dc_ctx[ch_type] >> 1);

        enc_vlc_write(bs, level, rice_level);

        if(level)
        oapv_bsw_write1(bs, sign);

        core->prev_dc_ctx[ch_type] = level;
    }

    for(scan_pos = 1; scan_pos < num_coeff; scan_pos++)
    {
        coef_cur = coef[scanp[scan_pos]];

        if(coef_cur)
        {
            level = oapv_abs16(coef_cur);
            sign = (coef_cur > 0) ? 0 : 1;

            /* Run coding */
            int rice_run = 0;
            rice_run = prev_run / 4;
            if (rice_run > 2) rice_run = 2;
            enc_vlc_write(bs, run, rice_run);


            /* Level coding */
            rice_level = oapv_clip3(OAPV_MIN_AC_LEVEL_CTX, OAPV_MAX_AC_LEVEL_CTX, prev_level >> 2);
            enc_vlc_write(bs, level - 1, rice_level);


            OAPV_TRACE_STR("level ");
            OAPV_TRACE_INT(level);
            OAPV_TRACE_STR("\n");

            /* Sign coding */
            oapv_bsw_write1(bs, sign);

            if (first_ac)
            {
                first_ac = 0;
                core->prev_1st_ac_ctx[ch_type] = level;
            }

            if(scan_pos == num_coeff - 1)
            {
                break;
            }
            prev_run = run;
            run = 0;

            {
                prev_level = level;
            }

            num_sig--;
        }
        else
        {
            run++;
        }
    }

    if (coef[scanp[num_coeff - 1]] == 0)
    {
        int run_ctx = prev_run > 15 ? 15 : prev_run;
        OAPV_TRACE_STR("run_ctx ");
        OAPV_TRACE_INT(run_ctx);
        OAPV_TRACE_STR("\n");

        int rice_run = 0;
        rice_run = prev_run / 4;
        if (rice_run > 2) rice_run = 2;
        enc_vlc_write(bs, run, rice_run);

    }

#if ENC_DEC_TRACE
    if(fp_trace_print == 1)
    OAPV_TRACE_SET(TRACE_TILE_DATA);
    OAPV_TRACE_STR("coef ");
    OAPV_TRACE_INT(ch_type);
    OAPV_TRACE_STR(": ");
    for (scan_pos = 0; scan_pos < num_coeff; scan_pos++)
    {
        OAPV_TRACE_INT(coef[scanp[scan_pos]]);
    }
    OAPV_TRACE_STR("\n");
#endif
}

int oapve_vlc_au_info(oapv_bs_t* bs, oapve_ctx_t* ctx, oapv_frms_t* frms, oapv_bs_t** bs_fi_pos)
{
    oapv_bsw_write(bs, frms->num_frms, 16);
    for(int fidx = 0; fidx < frms->num_frms; fidx++)
    {
        oapv_bsw_write(bs, frms->frm[fidx].pbu_type, 8);
        oapv_bsw_write(bs, frms->frm[fidx].group_id, 16);
        oapv_bsw_write(bs, 0, 8);
        memcpy(*(bs_fi_pos + sizeof(oapv_bs_t) * fidx), bs, sizeof(oapv_bs_t));/* store fi pos in au to re-write */
        oapve_vlc_frame_info(bs, &ctx->fh.fi);
    }

    oapv_bsw_write(bs, 0, 8);
    while (!oapv_bsw_is_align8(bs))
    {
        oapv_bsw_write1(bs, 0);
    }
    return OAPV_OK;
}

int oapve_vlc_pbu_size(oapv_bs_t* bs, int pbu_size)
{
    OAPV_TRACE_SET(1);
    oapv_assert_rv(oapv_bsw_is_align8(bs), OAPV_ERR_MALFORMED_BITSTREAM);
    oapv_bsw_write(bs, pbu_size, 32);
    return OAPV_OK;
}

int oapve_vlc_pbu_header(oapv_bs_t* bs, int pbu_type, int group_id)
{
    oapv_bsw_write(bs, pbu_type, 8);
    oapv_bsw_write(bs, group_id, 16);
    oapv_bsw_write(bs, 0, 8); // reserved_zero_8bit

    return OAPV_OK;
}

/****** ENABLE_DECODER ******/
int oapvd_vlc_run_length_cc(oapvd_ctx_t* ctx, oapvd_core_t* core, oapv_bs_t* bs, s16* coef, int log2_w, int log2_h, int ch_type)
{
    int            sign, level, prev_level, run;
    int            scan_pos_offset, num_coeff, i, coef_cnt = 0;
    const u16* scanp;
    int            ctx_last = 0;

    scanp = oapv_tbl_scan;
    num_coeff = 1 << (log2_w + log2_h);
    scan_pos_offset = 0;
    run = 0;
    int first_ac = 1;
    prev_level = core->prev_1st_ac_ctx[ch_type];

    int prev_run = 0;

    do
    {
        int rice_run = 0;
        rice_run = prev_run / 4;
        if (rice_run > 2) rice_run = 2;
        run = dec_vlc_read(bs, rice_run);

        for (i = scan_pos_offset; i < scan_pos_offset + run; i++)
        {
            coef[scanp[i]] = 0;
        }

        if (scan_pos_offset + run == num_coeff)
        {
            break;
        }

        scan_pos_offset += run;

        /* Level parsing */
        int rice_level = 0;
        if (scan_pos_offset == 0)
        {
            rice_level = oapv_clip3(OAPV_MIN_DC_LEVEL_CTX, OAPV_MAX_DC_LEVEL_CTX, core->prev_dc_ctx[ch_type] >> 1);
        }
        else
        {
            rice_level = oapv_clip3(OAPV_MIN_AC_LEVEL_CTX, OAPV_MAX_AC_LEVEL_CTX, prev_level >> 2);
        }
        level = dec_vlc_read(bs, rice_level);
        level++;

        OAPV_TRACE_STR("level ");
        OAPV_TRACE_INT(level);
        OAPV_TRACE_STR("\n");

        if (scan_pos_offset != 0)
        {
            prev_level = level;
        }
        prev_run = run;

        if (scan_pos_offset == 0)
        {
            core->prev_dc_ctx[ch_type] = level;
        }
        else if (first_ac)
        {
            first_ac = 0;
            core->prev_1st_ac_ctx[ch_type] = level;
        }

        /* Sign parsing */
        oapv_bsr_read1(bs, &sign);
        coef[scanp[scan_pos_offset]] = sign ? -(s16)level : (s16)level;

        coef_cnt++;

        if (scan_pos_offset >= num_coeff - 1)
        {
            break;
        }
        scan_pos_offset++;

    } while (1);

#if ENC_DEC_TRACE
    OAPV_TRACE_STR("coef ");
    OAPV_TRACE_INT(ch_type);
    OAPV_TRACE_STR(": ");
    for (scan_pos_offset = 0; scan_pos_offset < num_coeff; scan_pos_offset++)
    {
        OAPV_TRACE_INT(coef[scanp[scan_pos_offset]]);
    }
    OAPV_TRACE_STR("\n");
#endif
    return OAPV_OK;
}

int oapve_vlc_metadata(oapv_md_t* md, oapv_bs_t* bs)
{
    oapv_bsw_write(bs, md->md_size, 32);
    oapv_mdp_t* mdp = md->md_payload;

    while (mdp != NULL)
    {
        u32 mdp_pltype = mdp->pld_type;
        while (mdp_pltype >= 255)
        {
            oapv_bsw_write(bs, 0xFF, 8);
            mdp_pltype -= 255;
        }
        oapv_bsw_write(bs, mdp_pltype, 8);

        u32 mdp_size = mdp->pld_size;
        while (mdp_size >= 255)
        {
            oapv_bsw_write(bs, 0xFF, 8);
            mdp_size -= 255;
        }
        oapv_bsw_write(bs, mdp_size, 8);

        for (u32 i=0; i < mdp->pld_size; i++) {
            u8* payload_data = (u8*)mdp->pld_data;
            oapv_bsw_write(bs, payload_data[i], 8);
        }

        mdp = mdp->next;
    }
    return OAPV_OK;
}


///////////////////////////////////////////////////////////////////////////////
// end of encoder code
#endif // ENABLE_ENCODER
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// start of decoder code
#if ENABLE_DECODER
///////////////////////////////////////////////////////////////////////////////
int oapvd_vlc_au_size(oapv_bs_t* bs, u32* size)
{
    oapv_bsr_read(bs, size, 32);
    oapv_assert_rv(*size > 0, OAPV_ERR_MALFORMED_BITSTREAM);
    return OAPV_OK;
}


int oapvd_vlc_pbu_size(oapv_bs_t* bs, u32* size)
{
    oapv_bsr_read(bs, size, 32);
    return OAPV_OK;
}

int  oapvd_vlc_pbu_header(oapv_bs_t* bs, oapv_pbuh_t* pbuh)
{
    int t0;
    oapv_bsr_read(bs, &pbuh->pbu_type, 8);
    oapv_bsr_read(bs, &pbuh->group_id, 16);
    oapv_bsr_read(bs, &t0, 8);
    return OAPV_OK;
}

int oapvd_vlc_frame_info(oapv_bs_t* bs, oapv_fi_t *fi)
{
    int reserved_zero;
    oapv_bsr_read(bs, &fi->profile_idc, 8);
    oapv_bsr_read(bs, &fi->level_idc, 8);
    oapv_bsr_read(bs, &fi->band_idc, 3);
    oapv_bsr_read(bs, &reserved_zero, 5);
    oapv_assert_rv(reserved_zero==0, OAPV_ERR_MALFORMED_BITSTREAM);
    oapv_bsr_read(bs, &fi->frame_width, 32);
    oapv_bsr_read(bs, &fi->frame_height, 32);
    oapv_bsr_read(bs, &fi->chroma_format_idc, 4);
    oapv_bsr_read(bs, &fi->bit_depth, 4);
    oapv_bsr_read(bs, &fi->capture_time_distance, 8);
    oapv_bsr_read(bs, &reserved_zero, 8);
    oapv_assert_rv(reserved_zero==0, OAPV_ERR_MALFORMED_BITSTREAM);

    fi->frame_width += 1;
    fi->frame_height += 1;
    fi->bit_depth += 8;
    return OAPV_OK;
}

int oapvd_vlc_au_info(oapv_bs_t* bs, oapv_aui_t* aui)
{
    int ret;
    int reserved_zero_8bits;

    oapv_bsr_read(bs, &aui->num_frames, 16);
    for (int fidx = 0; fidx < aui->num_frames; fidx++)
    {
        oapv_bsr_read(bs, &aui->pbu_type[fidx], 8);
        oapv_bsr_read(bs, &aui->group_id[fidx], 16);
        oapv_bsr_read(bs, &reserved_zero_8bits, 8);
        oapv_assert_rv(reserved_zero_8bits==0, OAPV_ERR_MALFORMED_BITSTREAM);
        ret = oapvd_vlc_frame_info(bs, &aui->frame_info[fidx]);
        oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);
    }
    oapv_bsr_read(bs, &reserved_zero_8bits, 8);
    oapv_assert_rv(reserved_zero_8bits==0, OAPV_ERR_MALFORMED_BITSTREAM);
    /* byte align */
    oapv_bsr_align8(bs);
    return OAPV_OK;
}

int oapvd_vlc_frame_size(oapv_bs_t* bs)
{
    int size;
    oapv_assert_rv(oapv_bsr_get_remained_byte(bs) >= 4, -1);
    oapv_bsr_read(bs, &size, 32);
    return size;
}

static int dec_vlc_quantization_matrix(oapv_bs_t* bs, oapv_fh_t* fh)
{
    u32 t0;
    int num_comp = oapv_get_num_comp(fh->fi.chroma_format_idc);
    for (int cidx = 0; cidx < num_comp; cidx++)
    {
        for (int y = 0; y < OAPV_BLOCK_H; y++)
        {
            for (int x = 0; x < OAPV_BLOCK_W; x++)
            {
                oapv_bsr_read(bs, &t0, 8);
                fh->q_matrix[cidx][y][x] = t0 + 1;
            }
        }
    }
    return OAPV_OK;
}

static int dec_vlc_tile_info(oapv_bs_t* bs, oapv_fh_t* fh)
{
    oapv_bsr_read(bs, &fh->tile_width_in_mbs,  28);
    oapv_bsr_read(bs, &fh->tile_height_in_mbs, 28);
    fh->tile_width_in_mbs  += 1;
    fh->tile_height_in_mbs += 1;

    /* set various value */
    int pic_w = ((fh->fi.frame_width >> OAPV_LOG2_MB) + ((fh->fi.frame_width & (OAPV_MB - 1)) ? 1 : 0));
    pic_w = pic_w << OAPV_LOG2_MB;
    int pic_h = ((fh->fi.frame_height >> OAPV_LOG2_MB) + ((fh->fi.frame_height & (OAPV_MB - 1)) ? 1 : 0));
    pic_h = pic_h << OAPV_LOG2_MB;
    int tile_w = fh->tile_width_in_mbs * OAPV_MB;
    int tile_h = fh->tile_height_in_mbs * OAPV_MB;
    int tile_cols = (pic_w / tile_w) + ((pic_w % tile_w) ? 1 : 0);
    int tile_rows = (pic_h / tile_h) + ((pic_h % tile_h) ? 1 : 0);

    oapv_bsr_read(bs, &fh->tile_size_present_in_fh_flag, 1);
    if (fh->tile_size_present_in_fh_flag)
    {
        for (int i = 0; i < tile_cols * tile_rows; i++)
        {
            oapv_bsr_read(bs, &fh->tile_size[i], 32);
            fh->tile_size[i] += 1;
        }
    }

    return tile_cols * tile_rows;
}


int oapvd_vlc_frame_header(oapv_bs_t* bs, oapv_fh_t* fh)
{
    int reserved_zero;
    oapvd_vlc_frame_info(bs, &fh->fi);
    oapv_bsr_read(bs, &reserved_zero,            8);
    oapv_bsr_read(bs, &fh->color_description_present_flag,   1);
    if(fh->color_description_present_flag)
    {
        oapv_bsr_read(bs, &fh->color_primaries,              8);
        oapv_bsr_read(bs, &fh->transfer_characteristics,     8);
        oapv_bsr_read(bs, &fh->matrix_coefficients,          8);
    }
    else
    {
      fh->color_primaries          = 2;
      fh->transfer_characteristics = 2;
      fh->matrix_coefficients      = 2;
    }
    oapv_bsr_read(bs, &fh->use_q_matrix,                     1);
    if (fh->use_q_matrix)
    {
      dec_vlc_quantization_matrix(bs, fh);
    }
    int num_tiles = dec_vlc_tile_info(bs, fh);

    oapv_bsr_read(bs, &reserved_zero,            8);

    /* byte align */
    oapv_bsr_align8(bs);

    if (fh->use_q_matrix == 0)
    {
      int num_comp = oapv_get_num_comp(fh->fi.chroma_format_idc);
        for (int cidx = 0; cidx < num_comp; cidx++)
        {
            for (int y = 0; y < OAPV_BLOCK_H; y++)
            {
                for (int x = 0; x < OAPV_BLOCK_W; x++)
                {
                    fh->q_matrix[cidx][y][x] = 16;
                }
            }
        }
    }

    return OAPV_OK;
}

static int oapvd_vlc_tile_size(oapv_bs_t* bs, oapvd_ctx_t* ctx, int tile_idx)
{
    oapv_bsr_read(bs, &ctx->tile_size[tile_idx], 32);
    ctx->tile_size[tile_idx]++;
    return OAPV_OK;
}

static int oapvd_vlc_tile_header(oapv_bs_t* bs, oapvd_ctx_t* ctx, oapv_th_t* th)
{
    oapv_bsr_read(bs, &th->tile_header_size,  16);
    oapv_bsr_read(bs, &th->tile_index,        16);
    for (int c = 0; c < ctx->num_comp; c++)
    {
        oapv_bsr_read(bs, &th->tile_data_size[c], 32);
        th->tile_data_size[c]++;
    }
    for (int c = 0; c < ctx->num_comp; c++)
    {
        oapv_bsr_read(bs, &th->tile_qp[c], 8);
    }
    oapv_bsr_read(bs, &th->reserved_zero_8bits, 8);
    /* byte align */
    oapv_bsr_align8(bs);

    return OAPV_OK;
}

int oapvd_vlc_tiles(oapvd_ctx_t* ctx, oapv_bs_t* bs)
{
    oapv_bs_t bs_temp;
    u8* cur = bs->cur - (bs->leftbits >> 3);
    int size = bs->size;
    int read_size = 0;

    for (int i = 0; i < ctx->num_tiles; i++)
    {
        oapv_bsr_init(&bs_temp, cur, size, NULL);
        oapvd_vlc_tile_size(&bs_temp, ctx, i);
        oapvd_vlc_tile_header(&bs_temp, ctx, &ctx->th[i]);
        read_size = ctx->tile_size[i] + OAPV_TILE_SIZE_LEN;
        cur = cur + read_size;
        size = size - read_size;
    }

    return OAPV_OK;
}

int oapvd_store_tile_data(oapvd_ctx_t* ctx, oapv_bs_t* bs)
{
    oapv_bs_t bs_temp;
    u8* cur = bs->cur - (bs->leftbits >> 3);
    bs->leftbits = 0;
    bs->code = 0;
    int size = bs->size;
    int read_size = 0;

    int tile_idx = 0;

    for (int i = 0; i < ctx->num_tiles; i++)
    {
        oapv_bsr_init(&bs_temp, cur, size, NULL);
        ctx->ti[i].data_size = ctx->tile_size[i];
        ctx->tile_data[i] = cur + ctx->th[i].tile_header_size + OAPV_TILE_SIZE_LEN;
        read_size = ctx->ti[i].data_size + OAPV_TILE_SIZE_LEN;
        cur = cur + read_size;
        size = size - read_size;
    }
    bs->cur = cur;

    return OAPV_OK;
}

int oapve_vlc_dc_coeff(oapve_ctx_t* ctx, oapve_core_t* core, oapv_bs_t* bs, int log2_w, int dc_diff, int c)
{
    int rice_level = 0;
    int abs_dc_diff = oapv_abs32(dc_diff);
    int sign_dc_diff = (dc_diff > 0) ? 0 : 1;

    OAPV_TRACE_SET(TRACE_COEF_BIN);
    rice_level = oapv_clip3(OAPV_MIN_DC_LEVEL_CTX, OAPV_MAX_DC_LEVEL_CTX, core->prev_dc_ctx[c] >> 1);

    enc_vlc_write(bs, abs_dc_diff, rice_level);


    if (abs_dc_diff)
        oapv_bsw_write1(bs, sign_dc_diff);

    core->prev_dc_ctx[c] = abs_dc_diff;

    OAPV_TRACE_SET(TRACE_COEF_BIN);
    OAPV_TRACE_COUNTER
    OAPV_TRACE_STR("dc");
    OAPV_TRACE_INT(dc_diff);
    OAPV_TRACE_STR("\n");

    return OAPV_OK;
}
void oapve_vlc_ac_coeff(oapve_ctx_t* ctx, oapve_core_t* core, oapv_bs_t* bs, s16* coef, int log2_w, int log2_h, int num_sig, int ch_type)
{
    u32             num_coeff, scan_pos;
    u32             sign, level, prev_level, run;
    const u16* scanp;
    s16             coef_cur;
    int             ctx_last = 0;
    scanp = oapv_tbl_scan;
    num_coeff = (1 << (log2_w + log2_h));
    run = 0;
    int first_ac = 1;
    prev_level = core->prev_1st_ac_ctx[ch_type];
    int prev_run = 0;
    int rice_run = 0;
    int rice_level = 0;
    int lb = bs->leftbits;
    u32 code = bs->code;
    u8* cur = bs->cur;
    ALIGNED_16(s16 coef_temp[64]);
    OAPV_TRACE_SET(TRACE_COEF_BIN);
    OAPV_TRACE_COUNTER
    OAPV_TRACE_STR("ac ");
    for (scan_pos = 1; scan_pos < num_coeff; scan_pos++)
    {
        coef_temp[scan_pos] = coef[scanp[scan_pos]];
        OAPV_TRACE_INT(coef[scanp[scan_pos]]);
    }
    OAPV_TRACE_STR("\n");
    const s32 simple_vlc_table[3][2] = { {1,  },
                                        {0, 0},
                                        {0, 1} };
    for (scan_pos = 1; scan_pos < num_coeff-1; scan_pos++)
    {
        coef_cur = coef_temp[scan_pos];
        if (coef_cur)
        {
            level = oapv_abs16(coef_cur);
            sign = (coef_cur > 0) ? 0 : 1;
            rice_run = prev_run >> 2;
            if (rice_run > 2) rice_run = 2;
            if(run == 0 && rice_run == 0)
            {
                lb--; // bs->leftbits--;
                code |= (1 << lb);
                if (lb == 0)
                {
                    OAPV_FLUSH_SWAP(cur,code,lb);
                }
            }
            else
            {
                int leftbits;
                leftbits = lb;
                u32 code_from_lut = CODE_LUT_100[run][rice_run][0];
                int len_from_lut = CODE_LUT_100[run][rice_run][1];
                code |= (code_from_lut >> (32 - leftbits));
                if (len_from_lut < leftbits)
                {
                    lb -= len_from_lut;
                }
                else
                {
                    lb = 0;
                    OAPV_FLUSH_SWAP(cur,code,lb);
                    code = (leftbits < 32 ? code_from_lut << leftbits : 0);
                    lb = 32 - (len_from_lut - leftbits);
                }
            }
            rice_level = prev_level >> 2;
            if(rice_level>4)
                rice_level = OAPV_MAX_AC_LEVEL_CTX;
            if (level - 1 == 0 && rice_level == 0)
            {
                lb--;
                code |= (1 << lb);
                if (lb == 0)
                {
                    OAPV_FLUSH_SWAP(cur,code,lb);
                }
            }
            else
            {
                if (level - 1 > 98)
                {
                    {
                        int k =  rice_level;
                        u32 symbol = level - 1;
                        u32 simple_vlc_val = oapv_clip3(0, 2, symbol >> k);
                        int bit_cnt = 0;
                        if (symbol >= (u32)(1 << k))
                        {
                            symbol -= (1 << k);
                            int val = simple_vlc_table[simple_vlc_val][bit_cnt];
                            lb--;
                            code |= ((val & 0x1) << lb);
                            if (lb == 0)
                            {
                                OAPV_FLUSH_SWAP(cur,code,lb);
                            }
                            bit_cnt++;
                        }
                        if (symbol >= (u32)(1 << k) && simple_vlc_val>0)
                        {
                            symbol -= (1 << k);
                            int val = simple_vlc_table[simple_vlc_val][bit_cnt];
                            lb--;
                            code |= ((val & 0x1) << lb);
                            if (lb == 0)
                            {
                                OAPV_FLUSH_SWAP(cur,code,lb);
                            }
                            bit_cnt++;
                        }
                        while (symbol >= (u32)(1 << k))
                        {
                            symbol -= (1 << k);
                            lb--;
                            if (lb == 0)
                            {
                                OAPV_FLUSH_SWAP(cur,code,lb);
                            }
                            if( bit_cnt >= 2)
                            {
                                k++;
                            }
                            bit_cnt++;
                        }
                        if (bit_cnt < 2)
                        {
                            int val = simple_vlc_table[simple_vlc_val][bit_cnt];
                            lb--;
                            code |= ((val & 0x1) << lb);
                            if (lb == 0)
                            {
                                OAPV_FLUSH_SWAP(cur,code,lb);
                            }
                        }
                        else
                        {
                            lb--;
                            code |= ((1 & 0x1) << lb);
                            if (lb == 0)
                            {
                                OAPV_FLUSH_SWAP(cur,code,lb);
                            }
                        }
                        if (k > 0)
                        {
                            int leftbits;
                            leftbits = lb;
                            symbol <<= (32 - k);
                            code |= (symbol >> (32 - leftbits));
                            if(k < leftbits)
                            {
                                lb -= k;
                            }
                            else
                            {
                                lb = 0;
                                OAPV_FLUSH_SWAP(cur,code,lb);
                                code = (leftbits < 32 ? symbol << leftbits : 0);
                                lb = 32 - (k - leftbits);
                            }
                        }
                    }
                }
                else
                {
                    int leftbits;
                    leftbits = lb;
                    u32 code_from_lut    = CODE_LUT_100[level - 1][rice_level][0];
                    int len_from_lut    = CODE_LUT_100[level - 1][rice_level][1] ;
                    code |= (code_from_lut >> (32 - leftbits));
                    if (len_from_lut < leftbits)
                    {
                        lb -= len_from_lut;
                    }
                    else
                    {
                        lb = 0;
                        OAPV_FLUSH_SWAP(cur,code,lb);
                        code = (leftbits < 32 ? code_from_lut << leftbits : 0);
                        lb = 32 - (len_from_lut - leftbits);
                    }
                }
            }
            {
                lb--;
                code |= ((sign & 0x1) << lb);
                if (lb == 0)
                {
                    OAPV_FLUSH_SWAP(cur,code,lb);
                }
            }
            if (first_ac)
            {
                first_ac = 0;
                core->prev_1st_ac_ctx[ch_type] = level;
            }
            prev_run = run;
            run = 0;
            prev_level = level;
        }
        else
        {
            run++;
        }
    }
    bs->cur = cur;
    bs->code = code;
    bs->leftbits = lb ;
    coef_cur = coef_temp[scan_pos];
    if (coef_cur)
    {
        level = oapv_abs16(coef_cur);
        sign = (coef_cur > 0) ? 0 : 1;
        /* Run coding */
        rice_run = prev_run >> 2;
        if (rice_run > 2) rice_run = 2;
        if (run == 0 && rice_run == 0)
        {
            bs->leftbits--;
            bs->code |= (1 << bs->leftbits);
            if (bs->leftbits == 0)
            {
                OAPV_FLUSH(bs);
            }
        }
        else
        {
            int leftbits;
            leftbits = bs->leftbits;
            u32 code_from_lut = CODE_LUT_100[run][rice_run][0];
            int len_from_lut = CODE_LUT_100[run][rice_run][1];
            bs->code |= (code_from_lut >> (32 - leftbits));
            if (len_from_lut < leftbits)
            {
                bs->leftbits -= len_from_lut;
            }
            else
            {
                bs->leftbits = 0;
                OAPV_FLUSH(bs);
                bs->code = (leftbits < 32 ? code_from_lut << leftbits : 0);
                bs->leftbits = 32 - (len_from_lut - leftbits);
            }
        }
        /* Level coding */
        rice_level = prev_level >> 2;
        if(rice_level>4)  rice_level = OAPV_MAX_AC_LEVEL_CTX;
        if (level - 1 == 0 && rice_level == 0)
        {
            bs->leftbits--;
            bs->code |= (1 << bs->leftbits);
            if (bs->leftbits == 0)
            {
                OAPV_FLUSH(bs);
            }
        }
        else
        {
            if (level - 1 > 98)
            {
                enc_vlc_write(bs, level - 1, rice_level);
            }
            else
            {
                int leftbits;
                leftbits = bs->leftbits;
                u32 code_from_lut = CODE_LUT_100[level - 1][rice_level][0];
                int len_from_lut = CODE_LUT_100[level - 1][rice_level][1];
                bs->code |= (code_from_lut >> (32 - leftbits));
                if (len_from_lut < leftbits)
                {
                    bs->leftbits -= len_from_lut;
                }
                else
                {
                    bs->leftbits = 0;
                    OAPV_FLUSH(bs);
                    bs->code = (leftbits < 32 ? code_from_lut << leftbits : 0);
                    bs->leftbits = 32 - (len_from_lut - leftbits);
                }
            }
        }
        /* Sign coding */
        {
            bs->leftbits--;
            bs->code |= ((sign & 0x1) << bs->leftbits);
            if (bs->leftbits == 0)
            {
                OAPV_FLUSH(bs);
            }
        }
        if (first_ac)
        {
            first_ac = 0;
            core->prev_1st_ac_ctx[ch_type] = level;
        }
    }
    else
    {
        run++;
    }
    if (coef_temp[num_coeff - 1] == 0)
    {
        int run_ctx = prev_run > 15 ? 15 : prev_run;
        int rice_run = 0;
        rice_run = prev_run >> 2;
        if (rice_run > 2) rice_run = 2;

        if (run == 0 && rice_run == 0)
        {
            bs->leftbits--;
            bs->code |= (1 << bs->leftbits);
            if (bs->leftbits == 0)
            {
                OAPV_FLUSH(bs);
            }
        }
        else
        {
            int leftbits;
            leftbits = bs->leftbits;
            u32 code_from_lut = CODE_LUT_100[run][rice_run][0];
            int len_from_lut = CODE_LUT_100[run][rice_run][1];
            bs->code |= (code_from_lut >> (32 - leftbits));
            if (len_from_lut < leftbits)
            {
                bs->leftbits -= len_from_lut;
            }
            else
            {
                bs->leftbits = 0;
                OAPV_FLUSH(bs);
                bs->code = (leftbits < 32 ? code_from_lut << leftbits : 0);
                bs->leftbits = 32 - (len_from_lut - leftbits);
            }
        }
    }
}

int oapvd_vlc_dc_coeff(oapvd_ctx_t* ctx, oapvd_core_t* core, oapv_bs_t* bs, int log2_w, s16* dc_diff, int c)
{
    int rice_level = 0;
    int abs_dc_diff;
    int sign_dc_diff = 0;

    OAPV_TRACE_SET(TRACE_COEF_BIN);
    rice_level = oapv_clip3(OAPV_MIN_DC_LEVEL_CTX, OAPV_MAX_DC_LEVEL_CTX, core->prev_dc_ctx[c] >> 1);
    abs_dc_diff = dec_vlc_read(bs, rice_level);
    if (abs_dc_diff)
        oapv_bsr_read1(bs, &sign_dc_diff);

    *dc_diff = sign_dc_diff ? -abs_dc_diff : abs_dc_diff;

    core->prev_dc_ctx[c] = abs_dc_diff;

    OAPV_TRACE_SET(TRACE_COEF_BIN);
    OAPV_TRACE_COUNTER;
    OAPV_TRACE_STR("dc");
    OAPV_TRACE_INT(*dc_diff);
    OAPV_TRACE_STR("\n");

    return OAPV_OK;
}

int oapvd_vlc_ac_coeff(oapvd_ctx_t* ctx, oapvd_core_t* core, oapv_bs_t* bs, s16* coef, int log2_w, int log2_h, int ch_type)
{
    int            sign, level, prev_level, run;
    int            scan_pos_offset, num_coeff, i, coef_cnt = 0;
    const u16* scanp;
    int            ctx_last = 0;

    scanp = oapv_tbl_scan;
    num_coeff = (1 << (log2_w + log2_h));
    scan_pos_offset = 1;
    run = 0;

    int first_ac = 1;
    prev_level = core->prev_1st_ac_ctx[ch_type];
    int prev_run = 0;

    OAPV_TRACE_SET(TRACE_COEF_BIN);
    do
    {
        int rice_run = 0;
        rice_run = prev_run / 4;
        if (rice_run > 2) rice_run = 2;
        if (rice_run == 0)
        {
            if (bs->leftbits == 0)
            {
                OAPV_READ_FLUSH(bs, 4);
            }
            u32 t0 = (u32)(bs->code >> 31);
            bs->code <<= 1;
            bs->leftbits -= 1;
            if (t0)
                run = 0;
            else
                run = dec_vlc_read_1bit_read(bs, rice_run);
        }
        else
        {
            run = dec_vlc_read(bs, rice_run);
        }

        for (i = scan_pos_offset; i < scan_pos_offset + run; i++)
        {
            coef[scanp[i]] = 0;
        }

        if (scan_pos_offset + run == num_coeff)
        {
            break;
        }

        scan_pos_offset += run;

        /* Level parsing */
        int rice_level = 0;
        if (scan_pos_offset == 0)
        {
            rice_level = oapv_clip3(OAPV_MIN_DC_LEVEL_CTX, OAPV_MAX_DC_LEVEL_CTX, core->prev_dc_ctx[ch_type] >> 1);
        }
        else
        {
            rice_level = oapv_clip3(OAPV_MIN_AC_LEVEL_CTX, OAPV_MAX_AC_LEVEL_CTX, prev_level >> 2);
        }
        if (rice_level == 0)
        {
            if (bs->leftbits == 0)
            {
                OAPV_READ_FLUSH(bs, 4);
            }
            u32 t0 = (u32)(bs->code >> 31);
            bs->code <<= 1;
            bs->leftbits -= 1;
            if (t0)
                level = 0;
            else
                level = dec_vlc_read_1bit_read(bs, rice_level);
        }
        else
        {
        level = dec_vlc_read(bs, rice_level);
        }
        level++;

        if (scan_pos_offset != 0)
        {
            prev_level = level;
        }
        prev_run = run;

        if (scan_pos_offset == 0)
        {
            core->prev_dc_ctx[ch_type] = level;
        }
        else if (first_ac)
        {
            first_ac = 0;
            core->prev_1st_ac_ctx[ch_type] = level;
        }

        /* Sign parsing */
        if (bs->leftbits == 0)
        {
            OAPV_READ_FLUSH(bs, 4);
        }
        sign = (u32)(bs->code >> 31);
        bs->code <<= 1;
        bs->leftbits -= 1;
        coef[scanp[scan_pos_offset]] = sign ? -(s16)level : (s16)level;

        coef_cnt++;

        if (scan_pos_offset >= num_coeff - 1)
        {
            break;
        }
        scan_pos_offset++;
    } while (1);

#if ENC_DEC_TRACE
    OAPV_TRACE_SET(TRACE_COEF_BIN);
    OAPV_TRACE_COUNTER;
    OAPV_TRACE_STR("ac ");
    for (scan_pos_offset = 1; scan_pos_offset < num_coeff; scan_pos_offset++)
    {
        OAPV_TRACE_INT(coef[scanp[scan_pos_offset]]);
    }
    OAPV_TRACE_STR("\n");
#endif
    return OAPV_OK;
}

int oapvd_vlc_tile_dummy_data(oapv_bs_t* bs)
{
    u32 t0;
    while (bs->cur <= bs->end)
    {
        oapv_bsr_read(bs, &t0, 8);
    }

    return OAPV_OK;
}

#define INIT_MDP(TYPE, VAL) \
    TYPE* VAL = (TYPE*)oapv_malloc(sizeof(TYPE));\
    oapv_mset(VAL, 0, sizeof(TYPE));

int oapvd_vlc_metadata(oapv_bs_t* bs, u32 pbu_size, oapvm_t mid, int group_id)
{
    u32 t0;

    u32 metadata_size;
    oapv_bsr_read(bs, &metadata_size,32);
    u8* bs_start_pos = bs->cur;
    oapv_mdp_t* curr_mdp = NULL;
    u8* payload_data = NULL;
    while (metadata_size > 0)
    {
        u32 payload_type = 0, payload_size = 0;

        t0 = 0;
        do
        {
            oapv_bsr_read(bs, &t0, 8);
            metadata_size -= 8;
            if (t0 == 0xFF)
            {
                payload_type += 255;
            }
        } while (t0 == 0xFF);
        payload_type += t0;

        t0 = 0;
        do
        {
            oapv_bsr_read(bs, &t0, 8);
            metadata_size -= 8;
            if (t0 == 0xFF)
            {
                payload_size += 255;
            }
        } while (t0 == 0xFF);
        payload_size += t0;


        if (payload_size > 0) {

            payload_data = oapv_malloc(payload_size);
            if (payload_type == OAPV_METADATA_FILLER) {
                for(u32 i=0; i<payload_size; i++) {
                    oapv_bsr_read(bs, &t0, 8);
                    if (t0 != 0xFF) {
                        oapv_mfree(payload_data);
                        return OAPV_ERR_MALFORMED_BITSTREAM;
                    }
                    payload_data[i] = 0xFF;
                }

            }
            else {
                for(u32 i=0; i < payload_size; i++) {
                    oapv_bsr_read(bs, &t0, 8);
                    payload_data[i] = t0;
                }
            }

        }

        oapvm_set(mid, group_id, payload_type, payload_data, payload_size,
                  payload_type == OAPV_METADATA_USER_DEFINED ? payload_data : NULL);

        metadata_size -= payload_size;
        if (metadata_size < 0)
        {
            oapv_assert_rv(0, OAPV_ERR);
        }
    }
    const u32 target_read_size = (pbu_size - 8);
    if (OAPV_FAILED(oapvd_vlc_filler(bs, target_read_size - (bs->cur - bs_start_pos)))) {
        return OAPV_ERR_MALFORMED_BITSTREAM;
    }
    return OAPV_OK;
}

int oapvd_vlc_filler(oapv_bs_t* bs, u32 filler_size) {
    u32 val;
    while (filler_size > 0) {
        oapv_bsr_read(bs, &val, 8);
        if (val != 0xFF) {
            return OAPV_ERR_MALFORMED_BITSTREAM;
        }
        filler_size--;
    }
    return OAPV_OK;
}

///////////////////////////////////////////////////////////////////////////////
// end of decoder code
#endif // ENABLE_DECODER
///////////////////////////////////////////////////////////////////////////////

