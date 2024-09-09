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

#include <math.h>
#include "oapv_def.h"
#include "oapv_metadata.h"

///////////////////////////////////////////////////////////////////////////////
// start of common code
#if ENABLE_ENCODER || ENABLE_DECODER
///////////////////////////////////////////////////////////////////////////////

static void imgb_to_block(oapv_imgb_t *imgb, int c, int x_l, int y_l, int w_l, int h_l, s16 *block)
{
    u8 *src, *dst;
    int i, sft_hor, sft_ver;
    int bd = OAPV_CS_GET_BYTE_DEPTH(imgb->cs);

    if (c == 0)
    {
        sft_hor = sft_ver = 0;
    }
    else
    {
        u8 cfi = color_format_to_chroma_format_idc(OAPV_CS_GET_FORMAT(imgb->cs));
        sft_hor = oapv_get_chroma_sft_w(cfi);
        sft_ver = oapv_get_chroma_sft_h(cfi);
    }

    src = ((u8 *)imgb->a[c]) + ((y_l >> sft_ver) * imgb->s[c]) + ((x_l * bd) >> sft_hor);
    dst = (u8 *)block;

    for (i = 0; i < (h_l); i++)
    {
        oapv_mcpy(dst, src, (w_l)*bd);

        src += imgb->s[c];
        dst += (w_l)*bd;
    }
}

static void imgb_to_block_p210(oapv_imgb_t* imgb, int c, int x_l, int y_l, int w_l, int h_l, s16* block)
{
    u16* src, * dst;
    int sft_hor, sft_ver;
    int bd = OAPV_CS_GET_BYTE_DEPTH(imgb->cs);
    int format = OAPV_CS_GET_FORMAT(imgb->cs);
    int size_scale = 1;
    int tc = c;

    if (c == 0)
    {
        sft_hor = sft_ver = 0;
    }
    else
    {
        u8 cfi = color_format_to_chroma_format_idc(OAPV_CS_GET_FORMAT(imgb->cs));
        sft_hor = oapv_get_chroma_sft_w(cfi);
        sft_ver = oapv_get_chroma_sft_h(cfi);
        size_scale = 2;
        tc = 1;
    }

    u32 src_s = imgb->s[tc] >> (bd > 1 ? 1 : 0);
    src = ((u16*)imgb->a[tc]) + ((y_l >> sft_ver) * src_s) + ((x_l * size_scale) >> sft_hor);
    dst = (u16*)block;

    for (int i = 0; i < (h_l); i++)
    {
        for (int j = 0; j < (w_l); j++)
        {
            dst[j] = (src[j * size_scale + (c >> 1)] >> 6);
        }
        src += src_s;
        dst += w_l;
    }
}
static void block_to_imgb(s16 *block, int c, int x_l, int y_l, int w_l, int h_l, oapv_imgb_t *imgb)
{
    u8 *src, *dst;
    int i, sft_hor, sft_ver;
    int bd = OAPV_CS_GET_BYTE_DEPTH(imgb->cs);

    if (c == 0)
    {
        sft_hor = sft_ver = 0;
    }
    else
    {
        u8 cfi = color_format_to_chroma_format_idc(OAPV_CS_GET_FORMAT(imgb->cs));
        sft_hor = oapv_get_chroma_sft_w(cfi);
        sft_ver = oapv_get_chroma_sft_h(cfi);
    }

    src = (u8 *)block;
    dst = ((u8 *)imgb->a[c]) + ((y_l >> sft_ver) * imgb->s[c]) + ((x_l * bd) >> sft_hor);

    for (i = 0; i < (h_l); i++)
    {
        oapv_mcpy(dst, src, (w_l)*bd);

        src += (w_l)*bd;
        dst += imgb->s[c];
    }
}

static void block_to_imgb_p210(s16 *block, int c, int x_l, int y_l, int w_l, int h_l, oapv_imgb_t *imgb)
{
    s16 *src, *dst;
    int sft_hor, sft_ver;
    int size_scale = 1;
    int tc = c;

    if (c == 0)
    {
        sft_hor = sft_ver = 0;
    }
    else
    {
        u8 cfi = color_format_to_chroma_format_idc(OAPV_CS_GET_FORMAT(imgb->cs));
        sft_hor = oapv_get_chroma_sft_w(cfi);
        sft_ver = oapv_get_chroma_sft_h(cfi);
        size_scale = 2;
        tc = 1;
    }

    src = block;
    dst = ((s16*)imgb->a[tc] + ((y_l >> sft_ver) * imgb->aw[tc]) + ((x_l * size_scale) >> sft_hor));


    for (int i = 0; i < (h_l); i++)
    {
        for (int j = 0; j < (w_l); j++)
        {
          dst[j * size_scale + (c >> 1)] = (src[j] << 6);
        }
        src += (w_l);
        dst += imgb->aw[tc];
    }
}

static void oapv_plus_mid_val(s16* coef, int b_w, int b_h, int bit_depth)
{
  int mid_val = 1 << (bit_depth - 1);
  for (int i = 0; i < b_h * b_w; i++)
  {
    coef[i] = oapv_clip3(0, (1 << bit_depth) - 1, coef[i] + mid_val);
  }
}


static void copy_fi_to_finfo(oapv_fi_t* fi, int pbu_type, int group_id, oapv_frm_info_t* finfo)
{
    finfo->w = fi->frame_width;
    finfo->h = fi->frame_height;
    finfo->cs = OAPV_CS_SET(chroma_format_idc_to_color_format(fi->chroma_format_idc), fi->bit_depth, 0);
    finfo->pbu_type = pbu_type;
    finfo->group_id = group_id;
    finfo->profile_idc = fi->profile_idc;
    finfo->level_idc = fi->level_idc;
    finfo->band_idc = fi->band_idc;
    finfo->chroma_format_idc = fi->chroma_format_idc;
    finfo->bit_depth = fi->bit_depth;
    finfo->capture_time_distance = fi->capture_time_distance;
}

///////////////////////////////////////////////////////////////////////////////
// enc of common code
#endif // ENABLE_ENCODER || ENABLE_DECODER
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// start of encoder code
#if ENABLE_ENCODER
///////////////////////////////////////////////////////////////////////////////

double enc_block(oapve_ctx_t* ctx, oapve_core_t* core, int x, int y, int log2_block,int c);
double enc_block_rdo(oapve_ctx_t* ctx, oapve_core_t* core, int x, int y, int log2_block, int c);

static oapve_ctx_t * enc_id_to_ctx(oapve_t id)
{
    oapve_ctx_t *ctx;
    oapv_assert_rv(id, NULL);
    ctx = (oapve_ctx_t *)id;
    oapv_assert_rv((ctx)->magic == OAPVE_MAGIC_CODE, NULL);
    return ctx;
}

static oapve_ctx_t * enc_ctx_alloc(void)
{
    oapve_ctx_t *ctx;
    ctx = (oapve_ctx_t *)oapv_malloc_fast(sizeof(oapve_ctx_t));
    oapv_assert_rv(ctx, NULL);
    oapv_mset_x64a(ctx, 0, sizeof(oapve_ctx_t));
    return ctx;
}

static void enc_ctx_free(oapve_ctx_t *ctx)
{
    oapv_mfree_fast(ctx);
}

static oapve_core_t * enc_core_alloc(int chroma_format_idc)
{
    oapve_core_t * core;
    core = (oapve_core_t *)oapv_malloc_fast(sizeof(oapve_core_t));

    oapv_assert_rv(core, NULL);
    oapv_mset_x64a(core, 0, sizeof(oapve_core_t));

    return core;
}

static void enc_core_free(oapve_core_t * core)
{
    oapv_mfree_fast(core);
}

static int enc_core_init(oapve_core_t * core, oapve_ctx_t * ctx, int tile_idx, int thread_idx)
{
    core->tile_idx = tile_idx;
    core->thread_idx = thread_idx;
    core->ctx = ctx;
    return OAPV_OK;
}

static void enc_minus_mid_val(s16* coef, int b_w, int b_h, int bit_depth)
{
  int mid_val = 1 << (bit_depth - 1);
  for (int i = 0; i < b_h * b_w; i++)
  {
    coef[i] -= mid_val;
  }
}

static int set_tile_info(oapv_tile_info_t* ti, int pic_w, int pic_h, int tile_w, int tile_h
                       , u16* tile_cols, u16* tile_rows, u16* tile_num)
{
    (*tile_cols) = (pic_w / tile_w) + ((pic_w % tile_w) ? 1 : 0);
    (*tile_rows) = (pic_h / tile_h) + ((pic_h % tile_h) ? 1 : 0);
    (*tile_num) = (*tile_cols) * (*tile_rows);

    for (int i = 0; i < (*tile_num); i++)
    {
        int tx = (i % (*tile_cols)) * tile_w;
        int ty = (i / (*tile_cols)) * tile_h;
        ti[i].col = tx;
        ti[i].row = ty;
        ti[i].width = tx + tile_w > pic_w ? pic_w - tx : tile_w;
        ti[i].height = ty + tile_h > pic_h ? pic_h - ty : tile_h;
    }

  return 0;
}

static int enc_init_param(oapve_ctx_t *ctx, oapve_param_t *param)
{
    /* check input parameters */
    oapv_assert_rv(param->w > 0 && param->h > 0, OAPV_ERR_INVALID_ARGUMENT);
    oapv_assert_rv(param->qp >= MIN_QUANT && param->qp <= MAX_QUANT, OAPV_ERR_INVALID_ARGUMENT);

    ctx->qp[Y_C] = param->qp;
    ctx->qp[U_C] = param->qp + param->qp_cb_offset;
    ctx->qp[V_C] = param->qp + param->qp_cr_offset;
    ctx->qp[T_C] = param->qp;

    ctx->num_comp = oapv_get_num_comp(param->csp);

    if (param->preset == OAPV_PRESET_SLOW ||
        param->preset == OAPV_PRESET_PLACEBO) {
        ctx->fn_block = enc_block_rdo;
    }
    else {
        ctx->fn_block = enc_block;
    }

    ctx->log2_mb = OAPV_LOG2_MB;
    ctx->log2_block = OAPV_LOG2_BLOCK;

    ctx->mb = 1 << ctx->log2_mb;
    ctx->block = 1 << ctx->log2_block;

    /* set various value */
    int tmp_w = ((ctx->param->w >> ctx->log2_mb) + ((ctx->param->w & (ctx->mb - 1)) ? 1 : 0));
    ctx->w = tmp_w << ctx->log2_mb;
    int tmp_h = ((ctx->param->h >> ctx->log2_mb) + ((ctx->param->h & (ctx->mb - 1)) ? 1 : 0));
    ctx->h = tmp_h << ctx->log2_mb;

    int tile_w = ctx->param->tile_w_mb * ctx->mb;
    int tile_h = ctx->param->tile_h_mb * ctx->mb;
    set_tile_info(ctx->ti, ctx->w, ctx->h, tile_w, tile_h, &ctx->num_tile_cols, &ctx->num_tile_rows, &ctx->num_tiles);

    int size = ctx->num_tiles * sizeof(oapv_th_t);
    ctx->th = (oapv_th_t*)oapv_malloc(size);
    oapv_assert_rv(ctx->th, OAPV_ERR_UNKNOWN);

    if (param->rc_type != 0)
    {
        int num_mb = tmp_w * tmp_h;
        ctx->rc_param.target_bits_per_mb = (int64_t)param->bitrate * 1000 / (num_mb * param->fps);
    }

    return OAPV_OK;
}


static void enc_flush(oapve_ctx_t* ctx)
{
    //Release thread pool controller and created threads
    if (ctx->cdesc.threads >= 1)
    {
        if (ctx->tpool)
        {
            //thread controller instance is present
            //terminate the created thread
            for (int i = 0; i < ctx->cdesc.threads; i++)
            {
                if (ctx->thread_id[i])
                {
                    //valid thread instance
                    ctx->tpool->release(&ctx->thread_id[i]);
                }
            }
            //dinitialize the tc
            oapv_tpool_deinit(ctx->tpool);
            oapv_mfree_fast(ctx->tpool);
            ctx->tpool = NULL;
        }
    }

    oapv_tpool_sync_obj_delete(&ctx->sync_obj);
    for (int i = 0; i < ctx->cdesc.threads; i++)
    {
        enc_core_free(ctx->core[i]);
        ctx->core[i] = NULL;
    }


    oapv_mfree_fast(ctx->bs_temp_buf[0]);
    oapv_mfree_fast(ctx->th);
}

static int enc_ready(oapve_ctx_t *ctx)
{
    oapve_core_t *core = NULL;
    int ret = OAPV_OK;
    oapv_assert(ctx->core[0] == NULL);

    for (int i = 0; i < ctx->cdesc.threads; i++)
    {
        core = enc_core_alloc(ctx->cfi);
        oapv_assert_gv(core != NULL, ret, OAPV_ERR_OUT_OF_MEMORY, ERR);
        ctx->core[i] = core;

        rc_core_t* rc_core = (rc_core_t*)oapv_malloc_fast(sizeof(rc_core_t));
        oapv_assert_gv(rc_core != NULL, ret, OAPV_ERR_OUT_OF_MEMORY, ERR);
        oapv_mset_x64a(rc_core, 0, sizeof(rc_core_t));
        ctx->rc_core[i] = rc_core;
    }

    //initialize the threads to NULL
    for (int i = 0; i < OAPV_MAX_THREADS; i++)
    {
        ctx->thread_id[i] = 0;
    }

    //get the context synchronization handle
    ctx->sync_obj = oapv_tpool_sync_obj_create();
    oapv_assert_gv(ctx->sync_obj != NULL, ret, OAPV_ERR_UNKNOWN, ERR);

    if (ctx->cdesc.threads >= 1)
    {
        ctx->tpool = oapv_malloc(sizeof(oapv_tpool_t));
        oapv_tpool_init(ctx->tpool, ctx->cdesc.threads);
        for (int i = 0; i < ctx->cdesc.threads; i++)
        {
            ctx->thread_id[i] = ctx->tpool->create(ctx->tpool, i);
            oapv_assert_gv(ctx->thread_id[i] != NULL, ret, OAPV_ERR_UNKNOWN, ERR);
        }
    }

    for (int i = 0; i < OAPV_MAX_TILES; i++)
    {
        ctx->sync_flag[i] = 0;
    }
    ctx->bs_temp_buf[0] = (u8*)oapv_malloc(ctx->cdesc.max_bs_buf_size);
    oapv_assert_gv(ctx->bs_temp_buf[0], ret, OAPV_ERR_UNKNOWN, ERR);

    ctx->rc_param.alpha = OAPV_RC_ALPHA;
    ctx->rc_param.beta = OAPV_RC_BETA;

    return OAPV_OK;
ERR:

    enc_flush(ctx);

    return ret;
}

double enc_block(oapve_ctx_t* ctx, oapve_core_t* core, int x, int y, int log2_block, int c)
{
    int b_w = 1 << log2_block;
    int b_h = 1 << log2_block;
    int bit_depth = OAPV_CS_GET_BIT_DEPTH(ctx->imgb->cs);
    int nnz = 0;
    int qp = ctx->th[core->tile_idx].tile_qp[c];
    int qscale = oapv_quant_scale[qp % 6];

    ctx->fn_imgb_to_block(ctx->imgb, c, x, y, b_w, b_h, core->coef);
    enc_minus_mid_val(core->coef, b_w, b_h, bit_depth);

    oapv_trans(ctx, core->coef, log2_block, log2_block, bit_depth);
    (ctx->fn_quantb)[0](qp, core->q_matrix_enc[c], core->coef, log2_block, log2_block, qscale, c, bit_depth, c ? 128 : 212);

    int tmp_dc = core->prev_dc[c];
    core->prev_dc[c] = core->coef[0];
    core->coef[0] = core->coef[0] - tmp_dc;

    if (ctx->param->is_rec)
    {
        ALIGNED_16(s16 coef_temp[OAPV_BLOCK_D]);
        oapv_mcpy(coef_temp, core->coef, sizeof(s16) * OAPV_BLOCK_D);
        int dqscale = oapv_tbl_dq_scale[qp % 6];
        coef_temp[0] = coef_temp[0] + tmp_dc;
        oapv_itdq_block(ctx->fn_itx, ctx->fn_iquant, core->q_matrix_dec[c], coef_temp, log2_block, log2_block, dqscale, bit_depth, qp);

        oapv_plus_mid_val(coef_temp, b_w, b_h, bit_depth);
        /*store recon*/
        ctx->fn_block_to_imgb(coef_temp, c, x, y, (b_w), (b_h), ctx->rec);
    }

    return 0;
}

double enc_block_rdo(oapve_ctx_t* ctx, oapve_core_t* core, int x, int y, int log2_block, int c)
{
    oapv_bs_t* bs = &ctx->bs_thread[core->thread_idx];
    int b_w = 1 << log2_block;
    int b_h = 1 << log2_block;
    int bit_depth = OAPV_CS_GET_BIT_DEPTH(ctx->imgb->cs);
    int nnz;
    double cost;
    int qp = ctx->th[core->tile_idx].tile_qp[c];

    s16 best_coeff[OAPV_BLOCK_D];
    s16 best_recon[OAPV_BLOCK_D];
    s16 recon[OAPV_BLOCK_D];
    s16 coeff[OAPV_BLOCK_D];
    double best_cost = MAX_COST;

    double lambda = 0.57 * pow(2.0, ((double)(ctx->param->qp - 12)) / 3.0);
    lambda = c ? lambda / pow(2.0, (-2) / 2.0) : lambda;

    ctx->fn_imgb_to_block(ctx->imgb, c, x, y, b_w, b_h, core->coef);
    enc_minus_mid_val(core->coef, b_w, b_h, bit_depth);
    oapv_trans(ctx, core->coef, log2_block, log2_block, bit_depth);

    int qscale = oapv_quant_scale[qp % 6];

    int prev_dc = core->prev_dc[c];
    int best_prev_dc = 0;

    int stored_dc_ctx = core->prev_dc_ctx[c];
    int stored_1st_ac_ctx = core->prev_1st_ac_ctx[c];

    int min_val = c ? 154 : 100;
    int max_val = c ? 154 : 220;
    int step = 15;

    for (int dz = min_val; dz <= max_val; dz += step)
    {
        core->prev_dc_ctx[c] = stored_dc_ctx;
        core->prev_1st_ac_ctx[c] = stored_1st_ac_ctx;

        oapv_mcpy(coeff, core->coef, sizeof(s16) * ((u64)1 << (log2_block + log2_block)));

        nnz = (ctx->fn_quantb)[0](qp, core->q_matrix_enc[c], coeff, log2_block, log2_block, qscale, c, bit_depth, dz);

        prev_dc = core->prev_dc[c];
        int tmp_dc = prev_dc;
        prev_dc = coeff[0];
        coeff[0] = coeff[0] - tmp_dc;

        bs->is_bin_count = 1;
        bs->bin_count = 0;
        oapve_vlc_run_length_cc(ctx, core, bs, coeff, log2_block, log2_block, nnz, c);
        bs->is_bin_count = 0;

        oapv_mcpy(recon, coeff, sizeof(s16) * ((u64)1 << (log2_block + log2_block)));

        int dqscale = oapv_tbl_dq_scale[qp % 6];
        recon[0] = recon[0] + tmp_dc;

        oapv_itdq_block(ctx->fn_itx, ctx->fn_iquant, core->q_matrix_dec[c], recon, log2_block, log2_block, dqscale, bit_depth, qp);

        oapv_plus_mid_val(recon, b_w, b_h, bit_depth);
        s16* org = (s16*)((u8*)ctx->imgb->a[c] + ((y >> (c ? ctx->ch_sft_h : 0)) * ctx->imgb->s[c]) + ((x >> (c ? ctx->ch_sft_w : 0)) * 2));
        int dist = (int)oapv_ssd(log2_block, log2_block, org, recon, (ctx->imgb->s[c] >> 1), b_w, 10);
        cost = lambda * bs->bin_count + dist;

        if (cost < best_cost)
        {
            best_cost = cost;
            oapv_mcpy(best_coeff, coeff, sizeof(s16) * ((u64)1 << (log2_block + log2_block)));
            oapv_mcpy(best_recon, recon, sizeof(s16) * ((u64)1 << (log2_block + log2_block)));
            best_prev_dc = prev_dc;
        }
    }
    core->prev_dc[c] = best_prev_dc;
    core->prev_dc_ctx[c] = stored_dc_ctx;
    core->prev_1st_ac_ctx[c] = stored_1st_ac_ctx;
    oapv_mcpy(core->coef, best_coeff, sizeof(s16) * ((u64)1 << (log2_block + log2_block)));

    if (ctx->param->is_rec)
    {
        /*store recon*/
        ctx->fn_block_to_imgb(best_recon, c, x, y, (b_w), (b_h), ctx->rec);
    }
    return best_cost;
}

int enc_vlc_tile(oapve_ctx_t* ctx, oapve_core_t* core, int x, int y, int tile_idx, int c)
{
    oapv_bs_t* bs = &ctx->bs_thread[core->thread_idx];
    int mb_h, mb_w, b_h, b_w;

    u8* bs_cur = oapv_bsw_sink(bs);
    oapv_assert_rv(oapv_bsw_is_align8(bs), OAPV_ERR_MALFORMED_BITSTREAM);

    mb_h = ctx->mb >> (c ? ctx->ch_sft_h : 0);
    mb_w = ctx->mb >> (c ? ctx->ch_sft_w : 0);

    for (int ty = 0; ty < ctx->ti[tile_idx].height && y + ty < ctx->h; ty += ctx->mb)
    {
        for (int tx = 0; tx < ctx->ti[tile_idx].width; tx += ctx->mb)
        {
            int mb_x = x + tx;
            int mb_y = y + ty;
            int log2_block = ctx->log2_block;

            b_h = 1 << log2_block;
            b_w = 1 << log2_block;

            for (int b_y = 0; b_y < mb_h; b_y += b_h)
            {
                for (int b_x = 0; b_x < mb_w; b_x += b_w)
                {
                    int cx = mb_x + (b_x << (c ? ctx->ch_sft_w : 0));
                    int cy = mb_y + (b_y << (c ? ctx->ch_sft_h : 0));

#if ENC_DEC_TRACE
                    OAPV_TRACE_SET(TRACE_COEF_BIN);
                    OAPV_TRACE_COUNTER;
                    OAPV_TRACE_STR("x pos ");
                    OAPV_TRACE_INT(cx);
                    OAPV_TRACE_STR("y pos ");
                    OAPV_TRACE_INT(cy);
                    OAPV_TRACE_STR("width ");
                    OAPV_TRACE_INT(b_h);
                    OAPV_TRACE_STR("height ");
                    OAPV_TRACE_INT(b_w);
                    OAPV_TRACE_STR("CH ");
                    OAPV_TRACE_STR((c == 0 ? "Y" : (c == 1 ? "U" : "V")));
                    OAPV_TRACE_STR("\n");
#endif

                    ctx->fn_block(ctx, core, cx, cy, log2_block, c);
                    oapve_vlc_dc_coeff(ctx, core, bs, log2_block, core->coef[0], c);
                    oapve_vlc_ac_coeff(ctx, core, bs, core->coef, log2_block, log2_block, 0, c);
                }
            }
        }
    }

    /* byte align */
    while (!oapv_bsw_is_align8(bs))
    {
        oapv_bsw_write1(bs, 0);
    }

    /* de-init BSW */
    oapv_bsw_deinit(bs);

    return (int)(bs->cur - bs_cur);
}

static int enc_tile(oapve_ctx_t *ctx, oapve_core_t *core)
{
    oapv_bs_t *bs = &ctx->bs_thread[core->thread_idx];
    oapv_bsw_init(bs, bs->beg, bs->size, NULL);
    void* bs_cur = oapv_bsw_sink(bs);
    int x = ctx->ti[core->tile_idx].col;
    int y = ctx->ti[core->tile_idx].row;
    int tile_idx = core->tile_idx;

    int qp = 0;
    if (ctx->param->rc_type != 0)
    {
        oapve_rc_get_qp(ctx, core->tile_idx, ctx->qp[Y_C], &qp);
        oapv_assert(qp != 0);
    }
    else
    {
        qp = ctx->qp[Y_C];
    }

    ctx->tile_size[tile_idx] = 0;
    oapve_vlc_tile_size(bs, ctx->tile_size[tile_idx]);
    oapve_set_tile_header(ctx, &ctx->th[tile_idx], tile_idx, qp);
    oapve_vlc_tile_header(ctx, bs, &ctx->th[tile_idx]);

    for (int c = 0; c < ctx->num_comp; c++)
    {
        for (int cidx = 0; cidx < ctx->num_comp; cidx++)
        {
            int cnt = 0;
            int qp = ctx->th[tile_idx].tile_qp[cidx];
            int qscale = oapv_quant_scale[qp % 6];
            s32 scale_multiply_16 = (s32)(qscale << 4);
            for (int y = 0; y < OAPV_BLOCK_H; y++)
            {
                for (int x = 0; x < OAPV_BLOCK_W; x++)
                {
                    core->q_matrix_enc[cidx][cnt++] = scale_multiply_16 / ctx->fh.q_matrix[cidx][y][x];
                }
            }
        }
        if (ctx->param->is_rec)
        {
            for (int cidx = 0; cidx < ctx->num_comp; cidx++)
            {
                int cnt = 0;
                int qp = ctx->th[tile_idx].tile_qp[cidx];
                int dqscale = oapv_tbl_dq_scale[qp % 6];
                for (int y = 0; y < OAPV_BLOCK_H; y++)
                {
                    for (int x = 0; x < OAPV_BLOCK_W; x++)
                    {
                        core->q_matrix_dec[cidx][cnt++] = dqscale * ctx->fh.q_matrix[cidx][y][x];
                    }
                }
            }
        }
        core->prev_dc_ctx[c] = 20;
        core->prev_1st_ac_ctx[c] = 0;
        core->prev_dc[c] = 0;
    }

    for (int c = 0; c < ctx->num_comp; c++)
    {
        core->prev_dc_ctx[c] = 20;
        core->prev_1st_ac_ctx[c] = 0;
        core->prev_dc[c] = 0;
        ctx->th[tile_idx].tile_data_size[c] = enc_vlc_tile(ctx, core, x, y, tile_idx, c);
    }

    u32 bs_size = (int)(bs->cur - bs->beg);
    if (bs_size > ctx->ti[tile_idx].max_data_size)
    {
        return OAPV_ERR_OUT_OF_BS_BUF;
    }
    ctx->bs_temp_size[core->tile_idx] = bs_size;
    oapv_mcpy(ctx->bs_temp_buf[core->tile_idx], bs->beg, bs_size);

    oapv_bs_t  bs_th;
    bs_th.is_bin_count = 0;
    oapv_bsw_init(&bs_th, ctx->bs_temp_buf[tile_idx], ctx->bs_temp_size[tile_idx], NULL);
    ctx->tile_size[tile_idx] = bs_size - OAPV_TILE_SIZE_LEN;
    oapve_vlc_tile_size(&bs_th, ctx->tile_size[tile_idx]);
    oapve_vlc_tile_header(ctx, &bs_th, &ctx->th[tile_idx]);
    oapv_bsw_deinit(&bs_th);

    threadsafe_assign(&ctx->sync_flag[core->tile_idx], TPOOL_TERMINATED);

    return OAPV_OK;
}

static int enc_thread_tile(void *arg)
{
    oapve_core_t *core = (oapve_core_t *)arg;
    oapve_ctx_t *ctx = core->ctx;
    int tile_idx, ret;

    while (1) {
        ret = enc_tile(ctx, core);
        oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

        oapv_tpool_enter_cs(ctx->sync_obj);
        for (tile_idx = 0; tile_idx < ctx->num_tiles; tile_idx++)
        {
            if (ctx->sync_flag[tile_idx] == 0)
            {
                ctx->sync_flag[tile_idx] = 1;
                core->tile_idx = tile_idx;
                break;
            }
        }
        oapv_tpool_leave_cs(ctx->sync_obj);
        if (tile_idx == ctx->num_tiles)
        {
            break;
        }
    }
ERR:
    return ret;
}

static void enc_img_pad_p210(oapve_ctx_t* ctx, oapv_imgb_t* imgb)
{
    if (ctx->w == ctx->param->w && ctx->h == ctx->param->h)
    {
        return;
    }

    if (ctx->w != ctx->param->w)
    {
        for (int c = 0; c < imgb->np; c++)
        {
            int shift_w = 0;
            int shift_h = 0;

            int sw = ctx->param->w >> shift_w;
            int ew = ctx->w >> shift_w;
            int th = ctx->h >> shift_h;
            pel* dst = (pel*)imgb->a[c];
            pel src;

            for (int h = 0; h < th; h++)
            {
                src = dst[sw - 1];
                for (int w = sw; w < ew; w++)
                {
                    dst[w] = src;
                }
                dst += (imgb->s[c] >> 1);
            }
        }
    }

    if (ctx->h != ctx->param->h)
    {
        for (int c = 0; c < imgb->np; c++)
        {
            int shift_w = 0;
            int shift_h = 0;

            int sh = ctx->param->h >> shift_h;
            int eh = ctx->h >> shift_h;
            int tw = ctx->w >> shift_w;
            pel* dst = ((pel*)imgb->a[c]) + sh * (imgb->s[c] >> 1);
            pel* src = dst - (imgb->s[c] >> 1);

            for (int h = sh; h < eh; h++)
            {
                oapv_mcpy(dst, src, sizeof(pel) * tw);
                dst += (imgb->s[c] >> 1);
            }
        }
    }

    return;
}
static void enc_img_pad(oapve_ctx_t* ctx, oapv_imgb_t* imgb)
{
  if (ctx->w == ctx->param->w && ctx->h == ctx->param->h)
  {
       return;
  }

  if (ctx->w != ctx->param->w)
  {
        for (int c = 0; c < imgb->np; c++)
    {
      int shift_w = c ? ctx->ch_sft_w : 0;
      int shift_h = c ? ctx->ch_sft_h : 0;

      int sw = ctx->param->w >> shift_w;
      int ew = ctx->w >> shift_w;
      int th = ctx->h >> shift_h;
      pel * dst = (pel*)imgb->a[c];
      pel src;

      for (int h = 0; h < th; h++)
      {
        src = dst[sw - 1];
        for (int w = sw; w < ew; w++)
        {
          dst[w] = src;
        }
        dst += (imgb->s[c] >> 1);
      }
    }
  }

  if (ctx->h != ctx->param->h)
  {
        for (int c = 0; c < imgb->np; c++)
    {
      int shift_w = c ? ctx->ch_sft_w : 0;
      int shift_h = c ? ctx->ch_sft_h : 0;

      int sh = ctx->param->h >> shift_h;
      int eh = ctx->h >> shift_h;
      int tw = ctx->w >> shift_w;
      pel * dst = ((pel*)imgb->a[c]) + sh * (imgb->s[c] >> 1);
      pel * src = dst - (imgb->s[c] >> 1);

      for (int h = sh; h < eh; h++)
      {
        oapv_mcpy(dst, src, sizeof(pel) * tw);
        dst += (imgb->s[c] >> 1);
      }
    }
  }

    return;
}

static int enc_prepare(oapve_ctx_t *ctx, oapv_imgb_t *imgb, oapv_bitb_t *bitb, oapv_imgb_t *rec)
{
    oapv_assert_rv(ctx->qp[Y_C] >= MIN_QUANT && ctx->qp[Y_C] <= MAX_QUANT, OAPV_ERR_INVALID_ARGUMENT);

    ctx->cfi= color_format_to_chroma_format_idc(OAPV_CS_GET_FORMAT(imgb->cs));
    ctx->num_comp = oapv_get_num_comp(ctx->cfi);
    ctx->ch_sft_w = oapv_get_chroma_sft_w(ctx->cfi);
    ctx->ch_sft_h = oapv_get_chroma_sft_h(ctx->cfi);

    if (OAPV_CS_GET_FORMAT(imgb->cs) == OAPV_CF_PLANAR2)
    {
        ctx->fn_imgb_to_block = imgb_to_block_p210;
        ctx->fn_block_to_imgb = block_to_imgb_p210;
        ctx->fn_img_pad = enc_img_pad_p210;
    }
    else
    {
        ctx->fn_imgb_to_block = imgb_to_block;
        ctx->fn_block_to_imgb = block_to_imgb;
        ctx->fn_img_pad = enc_img_pad;
    }
    /* initialize bitstream container */
    //oapv_bsw_init(&ctx->bs, bitb->addr, bitb->bsize, NULL); // TODO : remove
    for (int i = 0; i < ctx->cdesc.threads; i++)
    {
        oapv_bsw_init(&ctx->bs_thread[i], ctx->bs_thread[i].beg, bitb->bsize, NULL);
    }

    ctx->w = (imgb->aw[Y_C] > 0)? imgb->aw[Y_C]: imgb->w[Y_C];
    ctx->h = (imgb->ah[Y_C] > 0)? imgb->ah[Y_C]: imgb->h[Y_C];

    ctx->fn_img_pad(ctx, imgb);

    for (int i = 0; i < ctx->num_tiles; i++) {
        ctx->sync_flag[i] = 0;
    }

    ctx->imgb = imgb;
    imgb_addref(ctx->imgb);
    if (rec)
    {
        for (int c = 0; c < ctx->num_comp; c++)
        {
            rec->w[c] = imgb->w[c];
            rec->h[c] = imgb->h[c];
            rec->x[c] = imgb->x[c];
            rec->y[c] = imgb->y[c];
        }
        ctx->rec = rec;
        imgb_addref(ctx->rec);
    }

    int buf_size = ctx->cdesc.max_bs_buf_size / ctx->num_tiles;
    ctx->ti[0].max_data_size = buf_size;
    for (int i = 1; i < ctx->num_tiles; i++)
    {
        ctx->bs_temp_buf[i] = ctx->bs_temp_buf[i - 1] + buf_size;
        ctx->ti[i].max_data_size = buf_size;
    }

    return OAPV_OK;
}

static int enc_finish(oapve_ctx_t *ctx, oapv_bitb_t *bitb, oapve_stat_t *stat)
{
    /* de-init BSW */
    oapv_bsw_deinit(&ctx->bs);

    stat->write = oapv_bsw_get_write_byte(&ctx->bs);

    imgb_release(ctx->imgb);
    if (ctx->rec)
    {
        imgb_release(ctx->rec);
        ctx->rec = NULL;
    }
    return OAPV_OK;
}

static int enc_frame(oapve_ctx_t *ctx, oapv_imgb_t *imgb, oapv_bitb_t *bitb, oapve_stat_t *stat, oapv_imgb_t *rec)
{
    oapv_bs_t *bs = &ctx->bs;
    //u8 *bs_cur_frame_size; // TODO : remove
    int ret = OAPV_OK;

    oapv_bs_t  bs_fh;
    oapv_mcpy(&bs_fh, bs, sizeof(oapv_bs_t));

    /* write frame header */
    oapve_set_frame_header(ctx, &ctx->fh);
    oapve_vlc_frame_header(bs, ctx, &ctx->fh);

    /* de-init BSW */
    oapv_bsw_deinit(bs);

    /* Tile wise encoding */
    u32 k = 0;
    int i = 0;
    oapv_tpool_t *tpool;
    int res;
    tpool = ctx->tpool;
    int parallel_task = 1;
    int tidx = 0, thread_num1 = 0;


    // RC init
    u64 cost_sum = 0;
    if (ctx->param->rc_type != 0)
    {
        oapve_rc_get_tile_cost_thread(ctx, &cost_sum);

        double bits_pic = (double)(ctx->param->bitrate * 1000 / ctx->param->fps);
        for (int i = 0; i < ctx->num_tiles; i++)
        {
            ctx->ti[i].rc.target_bits_left = bits_pic * ctx->ti[i].rc.cost / cost_sum;
            ctx->ti[i].rc.target_bits = ctx->ti[i].rc.target_bits_left;
        }

        ctx->rc_param.lambda = oapve_rc_estimate_pic_lambda(ctx, cost_sum);
        ctx->rc_param.qp = oapve_rc_estimate_pic_qp(ctx, ctx->rc_param.lambda);
        for (int c = 0; c < ctx->num_comp; c++)
        {
            ctx->qp[c] = ctx->rc_param.qp;
            if (c == 1)
            {
                ctx->qp[c] += ctx->param->qp_cb_offset;
            }
            else if (c == 2)
            {
                ctx->qp[c] += ctx->param->qp_cr_offset;
            }
        }
    }

    // Code for CTU parallel encoding
    parallel_task = (ctx->cdesc.threads > ctx->num_tiles) ? ctx->num_tiles : ctx->cdesc.threads;

    for (int i = 0; i < ctx->cdesc.threads; i++) {
        ctx->sync_flag[i] = 1;
    }
    // run new threads
    for (tidx = 0; tidx < (parallel_task - 1); tidx++) {
        enc_core_init(ctx->core[tidx], ctx, tidx, tidx);
        tpool->run(ctx->thread_id[tidx], enc_thread_tile,
                (void *) ctx->core[tidx]);
    }
    // use main thread
    enc_core_init(ctx->core[tidx], ctx, tidx, tidx);
    ret = enc_thread_tile((void *) ctx->core[tidx]);
    oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

    for (thread_num1 = 0; thread_num1 < parallel_task - 1; thread_num1++) {
        res = tpool->join(ctx->thread_id[thread_num1], &ret);
        oapv_assert_gv (res==TPOOL_SUCCESS, ret, OAPV_ERR_FAILED_SYSCALL, ERR);
        oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);
    }

    for (int i = 0; i < ctx->num_tiles; i++)
    {
        oapv_mcpy(ctx->bs.cur, ctx->bs_temp_buf[i], ctx->bs_temp_size[i]);
        ctx->bs.cur = ctx->bs.cur + ctx->bs_temp_size[i];
        ctx->fh.tile_size[i] = ctx->bs_temp_size[i] - OAPV_TILE_SIZE_LEN;
    }

    /* rewrite frame header */
    if (ctx->fh.tile_size_present_in_fh_flag)
    {
        oapve_vlc_frame_header(&bs_fh, ctx, &ctx->fh);
    }
    if (ctx->param->rc_type != 0)
    {
        oapve_rc_update_after_pic(ctx, cost_sum);
    }
    return ret;

ERR:
    return ret;
}

oapve_t oapve_create(oapve_cdesc_t *cdesc, int *err)
{
    oapve_ctx_t *ctx;
    int ret;

#if ENC_DEC_TRACE
    fp_trace = fopen("enc_trace.txt", "w+");
#if TRACE_HLS
    OAPV_TRACE_SET(1);
#endif
#endif

    /* memory allocation for ctx and core structure */
    ctx = (oapve_ctx_t *)enc_ctx_alloc();
    if (ctx != NULL)
    {
        oapv_mcpy(&ctx->cdesc, cdesc, sizeof(oapve_cdesc_t));
        ret = oapve_platform_init_extention(ctx);
        oapv_assert_g(ret == OAPV_OK, ERR);

        ret = oapve_create_bs_buf(ctx, cdesc->max_bs_buf_size); // create bitstream buffer
        oapv_assert_g(ret == OAPV_OK, ERR);

        ret = enc_ready(ctx);
        oapv_assert_g(ret == OAPV_OK, ERR);

        /* set default value for ctx */
        ctx->magic = OAPVE_MAGIC_CODE;
        ctx->id = (oapve_t)ctx;
        if (err)
        {
          *err = OAPV_OK;
        }
        return (ctx->id);
    }
    else
    {
        ret = OAPV_ERR;
    }
ERR:
    if (ctx)
    {
        oapve_delete_bs_buf(ctx);
        enc_ctx_free(ctx);
    }
    if (err)
    {
        *err = ret;
    }
    return NULL;
}

void oapve_delete(oapve_t eid)
{
    oapve_ctx_t *ctx;

    ctx = enc_id_to_ctx(eid);
    oapv_assert_r(ctx);

#if ENC_DEC_TRACE
    fclose(fp_trace);
#endif
    enc_flush(ctx);
    oapve_delete_bs_buf(ctx);
    enc_ctx_free(ctx);
}

int oapve_encode(oapve_t eid, oapv_frms_t* ifrms, oapvm_t mid, oapv_bitb_t* bitb, oapve_stat_t* stat, oapv_frms_t* rfrms)
{
    oapve_ctx_t* ctx;
    oapv_frm_t* frm;
    oapv_bs_t* bs;
    int i, ret;

    ctx = enc_id_to_ctx(eid);
    oapv_assert_rv(ctx != NULL && bitb->addr && bitb->bsize > 0, OAPV_ERR_INVALID_ARGUMENT);

    bs = &ctx->bs;

    oapv_bsw_init(bs, bitb->addr, bitb->bsize, NULL);
    oapv_mset(stat, 0, sizeof(oapve_stat_t));

    u8* bs_pos_au_size = oapv_bsw_sink(bs); // address syntax of au size
    u8* bs_pos_pbu_beg;
    oapv_bsw_write(bs, 0, 32);

    oapv_bs_t* bs_pos_fi = NULL;

    for (i = 0; i < ifrms->num_frms; i++)
    {
        frm = &ifrms->frm[i];

            /* set default value for encoding parameter */
        ctx->param = &ctx->cdesc.param[i];
        ret = enc_init_param(ctx, ctx->param);
        oapv_assert_rv(ret == OAPV_OK, OAPV_ERR);

        oapv_assert_rv(ctx->param->profile_idc == OAPV_PROFILE_422_10, OAPV_ERR_UNSUPPORTED);

        // prepare for encoding a frame
        ret = enc_prepare(ctx, frm->imgb, bitb, rfrms->frm[i].imgb);
        oapv_assert_rv(ret == OAPV_OK, ret);

        bs_pos_pbu_beg = oapv_bsw_sink(bs); /* store pbu pos of ai to re-write */
        oapve_vlc_pbu_size(bs, 0);
        oapve_vlc_pbu_header(bs, frm->pbu_type, frm->group_id);
        // encode a frame
        ret = enc_frame(ctx, frm->imgb, bitb, stat, rfrms->frm[i].imgb);
        oapv_assert_rv(ret == OAPV_OK, ret);

        int pbu_size = ((u8*)oapv_bsw_sink(bs)) - bs_pos_pbu_beg - 4;
        oapv_bsw_write_direct(bs_pos_pbu_beg, pbu_size, 32);

        stat->frm_size[i] = pbu_size + 4/* PUB size length*/;
        copy_fi_to_finfo(&ctx->fh.fi, frm->pbu_type, frm->group_id, &stat->aui.frm_info[i]);

        // add frame hash value of reconstructed frame into metadata list
        if (ctx->use_frm_hash &&
            (frm->pbu_type == OAPV_PBU_TYPE_PRIMARY_FRAME ||
            frm->pbu_type == OAPV_PBU_TYPE_NON_PRIMARY_FRAME)) {
            oapv_assert_rv(mid != NULL, OAPV_ERR_INVALID_ARGUMENT);
            ret = oapv_set_md5_pld(mid, frm->group_id, ctx->rec);
            oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);
        }
    }

    oapv_md_info_list_t* md_list = mid;
    if (md_list!=NULL)
    {
        int num_md = md_list->num;
        for (i = 0; i < num_md; i++)
        {
            int group_id = md_list->group_ids[i];
            bs_pos_pbu_beg = oapv_bsw_sink(bs); /* store pbu pos of ai to re-write */
            oapve_vlc_pbu_size(bs, 0);
            oapve_vlc_pbu_header(bs, OAPV_PBU_TYPE_METADATA, group_id);
            oapve_vlc_metadata(&md_list->md_arr[i], bs);
            int pbu_size = ((u8*)oapv_bsw_sink(bs)) - bs_pos_pbu_beg - 4;
            oapv_bsw_write_direct(bs_pos_pbu_beg, pbu_size, 32);
        }
    }

    int au_size = (int)((u8*)oapv_bsw_sink(bs) - bs_pos_au_size) - 4;
    oapv_bsw_write_direct(bs_pos_au_size, au_size, 32); /* u(32) */

    // finishing of encoding a frame
    ret = enc_finish(ctx, bitb, stat);
    oapv_assert_rv(ret == OAPV_OK, ret);

    return OAPV_OK;
}

int oapve_config(oapve_t eid, int cfg, void *buf, int *size)
{
    oapve_ctx_t *ctx;
    int t0;

    ctx = enc_id_to_ctx(eid);
    oapv_assert_rv(ctx, OAPV_ERR_INVALID_ARGUMENT);

    switch (cfg)
    {
    /* set config **********************************************************/
    case OAPV_CFG_SET_QP:
        oapv_assert_rv(*size == sizeof(int), OAPV_ERR_INVALID_ARGUMENT);
        t0 = *((int *)buf);
        oapv_assert_rv(t0 >= MIN_QUANT && t0 <= MAX_QUANT,
                       OAPV_ERR_INVALID_ARGUMENT);
        ctx->param->qp = t0;
        break;
    case OAPV_CFG_SET_FPS:
        oapv_assert_rv(*size == sizeof(int), OAPV_ERR_INVALID_ARGUMENT);
        t0 = *((int *)buf);
        oapv_assert_rv(t0 > 0, OAPV_ERR_INVALID_ARGUMENT);
        ctx->param->fps = t0;
        break;
    case OAPV_CFG_SET_BPS:
        oapv_assert_rv(*size == sizeof(int), OAPV_ERR_INVALID_ARGUMENT);
        t0 = *((int *)buf);
        oapv_assert_rv(t0 > 0, OAPV_ERR_INVALID_ARGUMENT);
        ctx->param->bitrate = t0;
        break;
    case OAPV_CFG_SET_USE_FRM_HASH:
        oapv_assert_rv(*size == sizeof(int), OAPV_ERR_INVALID_ARGUMENT);
        ctx->use_frm_hash = (*((int*)buf)) ? 1 : 0;
        break;
    /* get config *******************************************************/
    case OAPV_CFG_GET_QP:
        oapv_assert_rv(*size == sizeof(int), OAPV_ERR_INVALID_ARGUMENT);
        *((int *)buf) = ctx->param->qp;
        break;
    case OAPV_CFG_GET_WIDTH:
        oapv_assert_rv(*size == sizeof(int), OAPV_ERR_INVALID_ARGUMENT);
        *((int *)buf) = ctx->param->w;
        break;
    case OAPV_CFG_GET_HEIGHT:
        oapv_assert_rv(*size == sizeof(int), OAPV_ERR_INVALID_ARGUMENT);
        *((int *)buf) = ctx->param->h;
        break;
    case OAPV_CFG_GET_FPS:
        oapv_assert_rv(*size == sizeof(int), OAPV_ERR_INVALID_ARGUMENT);
        *((int *)buf) = ctx->param->fps;
        break;
    case OAPV_CFG_GET_BPS:
        oapv_assert_rv(*size == sizeof(int), OAPV_ERR_INVALID_ARGUMENT);
        *((int *)buf) = ctx->param->bitrate;
        break;
    default:
        oapv_trace("unknown config value (%d)\n", cfg);
        oapv_assert_rv(0, OAPV_ERR_UNSUPPORTED);
    }

    return OAPV_OK;
}

int oapve_param_default(oapve_param_t *param)
{
    oapv_mset(param, 0, sizeof(oapve_param_t));
    param->preset = OAPV_PRESET_DEFAULT;

    param->qp_cb_offset = 0;
    param->qp_cr_offset = 0;

    param->tile_w_mb = 16;
    param->tile_h_mb = 16;

    param->profile_idc = OAPV_PROFILE_422_10;
    param->level_idc = (int)(2.1 * 30);
    param->band_idc = 2;

    return OAPV_OK;
}

///////////////////////////////////////////////////////////////////////////////
// enc of encoder code
#endif // ENABLE_ENCODER
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// start of decoder code
#if ENABLE_DECODER
///////////////////////////////////////////////////////////////////////////////
static oapvd_ctx_t* dec_id_to_ctx(oapvd_t id)
{
    oapvd_ctx_t *ctx;
    oapv_assert_rv(id, NULL);
    ctx = (oapvd_ctx_t *)id;
    oapv_assert_rv(ctx->magic == OAPVD_MAGIC_CODE, NULL);
    return ctx;
}

static oapvd_ctx_t* dec_ctx_alloc(void)
{
    oapvd_ctx_t *ctx;

    ctx = (oapvd_ctx_t *)oapv_malloc_fast(sizeof(oapvd_ctx_t));

    oapv_assert_rv(ctx != NULL, NULL);
    oapv_mset_x64a(ctx, 0, sizeof(oapvd_ctx_t));

    return ctx;
}

static void dec_ctx_free(oapvd_ctx_t *ctx)
{
    oapv_mfree_fast(ctx);
}

static oapvd_core_t* dec_core_alloc(void)
{
    oapvd_core_t *core;

    core = (oapvd_core_t *)oapv_malloc_fast(sizeof(oapvd_core_t));

    oapv_assert_rv(core, NULL);
    oapv_mset_x64a(core, 0, sizeof(oapvd_core_t));

    return core;
}

static void dec_core_free(oapvd_core_t *core)
{
    oapv_mfree_fast(core);
}

static int dec_block(oapvd_ctx_t* ctx, oapvd_core_t* core, int x, int y, int log2_block_w, int log2_block_h, int c)
{
    oapv_bs_t* bs = &core->bs;
    int bit_depth = OAPV_CS_GET_BIT_DEPTH(ctx->imgb->cs);
    int byte_depth = OAPV_CS_GET_BYTE_DEPTH(ctx->imgb->cs);

    int b_w = 1 << log2_block_w;
    int b_h = 1 << log2_block_h;

    core->coef[0] += core->prev_dc[c];
    core->prev_dc[c] = core->coef[0];

    int dqscale = oapv_tbl_dq_scale[core->qp[c] % 6];

    int shift = bit_depth - 2 - (core->qp[c] / 6);

    ctx->fn_iquant[0](core->coef, core->q_matrix_dec[c], 3, 3, dqscale, shift);

    oapv_itrans(ctx->fn_itx, core->coef, 3, 3, bit_depth);

    oapv_plus_mid_val(core->coef, b_w, b_h, bit_depth);
    // store to output
    ctx->fn_block_to_imgb(core->coef, c, x, y, b_w, b_h, ctx->imgb);

    return OAPV_OK;
}

static int dec_prepare(oapvd_ctx_t* ctx, oapv_imgb_t* imgb)
{
    ctx->imgb = imgb;
    imgb_addref(ctx->imgb); // increase reference count

    if (OAPV_CS_GET_FORMAT(imgb->cs) == OAPV_CF_PLANAR2)
    {
        ctx->fn_block_to_imgb = block_to_imgb_p210;
    }
    else
    {
        ctx->fn_block_to_imgb = block_to_imgb;
    }
    return OAPV_OK;
}

static int dec_finish(oapvd_ctx_t *ctx)
{
    oapv_mset(&ctx->bs, 0, sizeof(oapv_bs_t)); // clean data
    imgb_release(ctx->imgb);                     // decrease reference cnout
    ctx->imgb = NULL;
    return OAPV_OK;
}

int dec_tile_comp(oapvd_ctx_t* ctx, oapvd_core_t* core, oapv_bs_t* bs, int x, int y, int tile_idx, int c)
{
    int mb_h, mb_w, b_h, b_w;
    int ret;
    OAPV_TRACE_SET(1);

    mb_h = ctx->mb >> (c ? ctx->ch_sft_h : 0);
    mb_w = ctx->mb >> (c ? ctx->ch_sft_w : 0);

    for (int ty = 0; ty < ctx->ti[tile_idx].height && y + ty < ctx->h; ty += ctx->mb)
    {
        for (int tx = 0; tx < ctx->ti[tile_idx].width; tx += ctx->mb)
        {
            int mb_x = x + tx;
            int mb_y = y + ty;
            int log2_block = ctx->log2_block;

            b_h = 1 << log2_block;
            b_w = 1 << log2_block;

            for (int b_y = 0; b_y < mb_h; b_y += b_h)
            {
                for (int b_x = 0; b_x < mb_w; b_x += b_w)
                {
                    int cx = mb_x + (b_x << (c ? ctx->ch_sft_w : 0));
                    int cy = mb_y + (b_y << (c ? ctx->ch_sft_h : 0));

#if ENC_DEC_TRACE
                    OAPV_TRACE_SET(TRACE_COEF_BIN);
                    OAPV_TRACE_COUNTER;
                    OAPV_TRACE_STR("x pos ");
                    OAPV_TRACE_INT(cx);
                    OAPV_TRACE_STR("y pos ");
                    OAPV_TRACE_INT(cy);
                    OAPV_TRACE_STR("width ");
                    OAPV_TRACE_INT(b_h);
                    OAPV_TRACE_STR("height ");
                    OAPV_TRACE_INT(b_w);
                    OAPV_TRACE_STR("CH ");
                    OAPV_TRACE_STR((c == 0 ? "Y" : (c == 1 ? "U" : "V")));
                    OAPV_TRACE_STR("\n");
#endif

                    ret = oapvd_vlc_dc_coeff(ctx, core, bs, log2_block, &core->coef[0], c);
                    oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);
                    ret = oapvd_vlc_ac_coeff(ctx, core, bs, core->coef, log2_block, log2_block, c);
                    oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);
                    ret = dec_block(ctx, core, cx, cy, log2_block, log2_block, c);
                    oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);
                }
            }
        }
    }

    /* byte align */
    oapv_bsr_align8(bs);
    return OAPV_OK;
}

static int dec_tile(void *arg)
{
    int ret;
    oapvd_core_t *core = (oapvd_core_t *)arg;
    oapvd_ctx_t *ctx = core->ctx;
    oapv_bs_t *bs = &core->bs;

    int tile_idx = core->tile_idx;
    int x = ctx->ti[tile_idx].col;
    int y = ctx->ti[tile_idx].row;

    oapv_bsr_init(bs, ctx->tile_data[tile_idx], ctx->ti[tile_idx].data_size - ctx->th[tile_idx].tile_header_size, NULL);

    int cnt_dec = 0;
    for (int cidx = 0; cidx < ctx->num_comp; cidx++)
    {
        cnt_dec = 0;
        int qp_dec = ctx->th[core->tile_idx].tile_qp[cidx];
        int dqscale = oapv_tbl_dq_scale[qp_dec % 6];
        for (int y = 0; y < OAPV_BLOCK_H; y++)
        {
            for (int x = 0; x < OAPV_BLOCK_W; x++)
            {
                core->q_matrix_dec[cidx][cnt_dec++] = dqscale * ctx->fh.q_matrix[cidx][y][x];
            }
        }
    }

    for (int c = 0; c < ctx->num_comp; c++)
    {
        core->prev_dc_ctx[c] = 20;
        core->prev_1st_ac_ctx[c] = 0;
        core->prev_dc[c] = 0;
        core->qp[c] = ctx->th[core->tile_idx].tile_qp[c];
        ret = dec_tile_comp(ctx, core, bs, x, y, tile_idx, c);
        oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);
    }

    oapvd_vlc_tile_dummy_data(bs);
    return OAPV_OK;
}


static int dec_thread_tile(void* arg)
{
    oapvd_core_t* core = (oapvd_core_t*)arg;
    oapvd_ctx_t* ctx = core->ctx;

    int i;
    while (1)
    {
        dec_tile(arg);

        oapv_tpool_enter_cs(ctx->sync_obj);
        for (i = 0; i < ctx->num_tiles; i++)
        {
            if (ctx->sync_flag[i] == 0)
            {
                ctx->sync_flag[i] = 1;
                core->tile_idx = i;
                break;
            }
        }
        oapv_tpool_leave_cs(ctx->sync_obj);
        if (i == ctx->num_tiles)
        {
            break;
        }
    }
    return OAPV_OK;
}

static int dec_init_tile(oapvd_ctx_t* ctx)
{
    /* set various value */
    int tile_w = ctx->fh.tile_width_in_mbs * OAPV_MB;
    int tile_h = ctx->fh.tile_height_in_mbs * OAPV_MB;
    int tile_cols = (ctx->w / tile_w) + ((ctx->w % tile_w) ? 1 : 0);
    int tile_rows = (ctx->h / tile_h) + ((ctx->h % tile_h) ? 1 : 0);

    ctx->num_tiles = tile_cols * tile_rows;
    set_tile_info(ctx->ti, ctx->w, ctx->h, tile_w, tile_h, &(ctx->num_tile_cols), &(ctx->num_tile_rows), &(ctx->num_tiles));

    return OAPV_OK;
}

static int dec_init_tile_data(oapvd_ctx_t* ctx)
{
    for (int i = 0; i < ctx->num_tiles; i++)
    {
        ctx->tile_data[i] = NULL;
    }

    for (int i = 0; i < ctx->num_tiles; i++)
    {
        ctx->sync_flag[i] = 0;
    }
    return OAPV_OK;
}

static int dec_core_init(oapvd_core_t* core, oapvd_ctx_t* ctx, int tile_idx, int thread_idx)
{
    core->tile_idx = tile_idx;
    core->thread_idx = thread_idx;
    core->ctx = ctx;
    return OAPV_OK;
}


static void dec_flush(oapvd_ctx_t* ctx)
{
    if (ctx->cdesc.threads >= 1)
    {
        if (ctx->tpool)
        {
            //thread controller instance is present
            //terminate the created thread
            for (int i = 0; i < ctx->cdesc.threads; i++)
            {
                if (ctx->thread_id[i])
                {
                    //valid thread instance
                    ctx->tpool->release(&ctx->thread_id[i]);
                }
            }
            //dinitialize the tpool
            oapv_tpool_deinit(ctx->tpool);
            oapv_mfree_fast(ctx->tpool);
            ctx->tpool = NULL;
        }
    }

    oapv_tpool_sync_obj_delete(&(ctx->sync_obj));

    for (int i = 0; i < ctx->cdesc.threads; i++)
    {
        dec_core_free(ctx->core[i]);
    }
}


static int dec_ready(oapvd_ctx_t* ctx)
{
    int ret = OAPV_OK;

    if (ctx->core[0] == NULL)
    {
        for (int i = 0; i < ctx->cdesc.threads; i++)
        {
            ctx->core[i] = dec_core_alloc();
            oapv_assert_gv(ctx->core[i], ret, OAPV_ERR_OUT_OF_MEMORY, ERR);
            ctx->core[i]->ctx = ctx;
        }
    }

    //initialize the threads to NULL
    for (int i = 0; i < OAPV_MAX_THREADS; i++)
    {
        ctx->thread_id[i] = 0;
    }

    //get the context synchronization handle
    ctx->sync_obj = oapv_tpool_sync_obj_create();
    oapv_assert_gv(ctx->sync_obj != NULL, ret, OAPV_ERR_UNKNOWN, ERR);

    if (ctx->cdesc.threads >= 1)
    {
        ctx->tpool = oapv_malloc(sizeof(oapv_tpool_t));
        oapv_tpool_init(ctx->tpool, ctx->cdesc.threads);
        for (int i = 0; i < ctx->cdesc.threads; i++)
        {
            ctx->thread_id[i] = ctx->tpool->create(ctx->tpool, i);
            oapv_assert_gv(ctx->thread_id[i] != NULL, ret, OAPV_ERR_UNKNOWN, ERR);
        }
    }
    oapv_mset(ctx->tile_data, 0, sizeof(u8*) * OAPV_MAX_TILES);
    return OAPV_OK;

ERR:
    dec_flush(ctx);

    return ret;
}

oapvd_t oapvd_create(oapvd_cdesc_t *cdesc, int *err)
{
    oapvd_ctx_t *ctx;
    int ret;

#if ENC_DEC_TRACE
    fp_trace = fopen("dec_trace.txt", "w+");
#if TRACE_HLS
    OAPV_TRACE_SET(1);
#endif
#endif
    ctx = NULL;

    /* memory allocation for ctx and core structure */
    ctx = (oapvd_ctx_t *)dec_ctx_alloc();
    oapv_assert_gv(ctx != NULL, ret, OAPV_ERR_OUT_OF_MEMORY, ERR);
    oapv_mcpy(&ctx->cdesc, cdesc, sizeof(oapvd_cdesc_t));

    ret = dec_ready(ctx);
    oapv_assert_g(ret == OAPV_OK, ERR);
    oapvd_platform_init_extention(ctx);

    ctx->magic = OAPVD_MAGIC_CODE;
    ctx->id = (oapvd_t)ctx;
    if (err)
    {
        *err = OAPV_OK;
    }
    return (ctx->id);

ERR:
    if (ctx)
    {
        dec_ctx_free(ctx);
    }
    if (err)
    {
        *err = ret;
    }
    return NULL;
}

void oapvd_delete(oapvd_t did)
{
    oapvd_ctx_t *ctx;
    ctx = dec_id_to_ctx(did);
    oapv_assert_r(ctx);

#if ENC_DEC_TRACE
    fclose(fp_trace);
#endif
    dec_flush(ctx);
    dec_ctx_free(ctx);
}

static int dec_init_frame(oapvd_ctx_t* ctx)
{
    ctx->log2_mb = OAPV_LOG2_MB;
    ctx->log2_block = OAPV_LOG2_BLOCK;

    ctx->mb = 1 << ctx->log2_mb;
    ctx->block = 1 << ctx->log2_block;

    /* set various value */
    int tmp_w = ((ctx->fh.fi.frame_width >> ctx->log2_mb) + ((ctx->fh.fi.frame_width & (ctx->mb - 1)) ? 1 : 0));
    ctx->w = tmp_w << ctx->log2_mb;

    int tmp_h = ((ctx->fh.fi.frame_height >> ctx->log2_mb) + ((ctx->fh.fi.frame_height & (ctx->mb - 1)) ? 1 : 0));
    ctx->h = tmp_h << ctx->log2_mb;

    return OAPV_OK;
}

int oapvd_decode(oapvd_t did, oapv_bitb_t *bitb, oapv_frms_t* ofrms, oapvm_t mid, oapvd_stat_t *stat)
{
    oapvd_ctx_t *ctx;
    oapv_bs_t *bs;
    oapv_pbuh_t pbuh;
    int ret = OAPV_OK;
    u32 pbu_size;
    int remain;
    u8* curpos;
    int frame_cnt = 0;

    ctx = dec_id_to_ctx(did);
    oapv_assert_rv(ctx, OAPV_ERR_INVALID_ARGUMENT);


    curpos = (u8*)bitb->addr;
    remain = bitb->ssize;

    while (remain > 8)
    {
        OAPV_TRACE_SET(TRACE_HLS);
        oapv_bsr_init(&ctx->bs, curpos, remain, NULL);
        bs = &ctx->bs;
        ret = oapvd_vlc_pbu_size(bs, &pbu_size);
        oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);
        oapv_assert_g((pbu_size + 4) <= bs->size, ERR);
        ret = oapvd_vlc_pbu_header(bs, &pbuh);
        if (pbuh.pbu_type == OAPV_PBU_TYPE_PRIMARY_FRAME ||
        pbuh.pbu_type == OAPV_PBU_TYPE_NON_PRIMARY_FRAME ||
        pbuh.pbu_type == OAPV_PBU_TYPE_PREVIEW_FRAME ||
        pbuh.pbu_type == OAPV_PBU_TYPE_DEPTH_FRAME ||
        pbuh.pbu_type == OAPV_PBU_TYPE_ALPHA_FRAME)
        {
            dec_prepare(ctx, ofrms->frm[frame_cnt].imgb);
            ret = oapvd_vlc_frame_header(bs, &ctx->fh);
            oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);
            ctx->cfi = ctx->fh.fi.chroma_format_idc;
            ctx->num_comp = oapv_get_num_comp(ctx->cfi);
            ctx->ch_sft_w = oapv_get_chroma_sft_w(ctx->cfi);
            ctx->ch_sft_h = oapv_get_chroma_sft_h(ctx->cfi);

            ret = dec_init_frame(ctx);
            oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

            ret = dec_init_tile(ctx);
            oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

            ret = oapvd_vlc_tiles(ctx, bs);
            oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

            ret = dec_init_tile_data(ctx);
            oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

            ret = oapvd_store_tile_data(ctx, bs);
            oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

            int i = 0;
            int res;
            oapv_tpool_t* tpool = ctx->tpool;
            int parallel_task = 1;
            int tidx = 0;

            parallel_task = (ctx->cdesc.threads > ctx->num_tiles) ? ctx->num_tiles : ctx->cdesc.threads;

            for (i = 0; i < ctx->cdesc.threads; i++)
            {
                ctx->sync_flag[i] = 1;
            }

            for (tidx = 0; tidx < (parallel_task - 1); tidx++) {
                dec_core_init(ctx->core[tidx], ctx, tidx, tidx);
                tpool->run(ctx->thread_id[tidx], dec_thread_tile,
                    (void*)ctx->core[tidx]);
            }
            dec_core_init(ctx->core[tidx], ctx, tidx, tidx);
            ret = dec_thread_tile((void*)ctx->core[tidx]);

            for (tidx = 0; tidx < parallel_task - 1; tidx++)
            {
                tpool->join(ctx->thread_id[tidx], &res);
                if (OAPV_FAILED(res))
                {
                    ret = res;
                }
            }
            stat->read += oapv_bsr_get_read_byte(&ctx->bs);

            copy_fi_to_finfo(&ctx->fh.fi, pbuh.pbu_type, pbuh.group_id, &stat->aui.frm_info[frame_cnt]);
            if (ret == OAPV_OK && ctx->use_frm_hash)
            {
                ret = oapv_imgb_set_md5(ctx->imgb);
            }
            dec_finish(ctx); // FIX-ME
            ofrms->frm[frame_cnt].pbu_type = pbuh.pbu_type;
            ofrms->frm[frame_cnt].group_id = pbuh.group_id;
            stat->frm_size[frame_cnt] = pbu_size + 4/* PUB size length*/;
            frame_cnt++;
        }
        else if (pbuh.pbu_type == OAPV_PBU_TYPE_METADATA)
        {
            ret = oapvd_vlc_metadata(bs, pbu_size, mid, pbuh.group_id);
            stat->read += oapv_bsr_get_read_byte(&ctx->bs);
        }
        else if (pbuh.pbu_type == OAPV_PBU_TYPE_FILLER)
        {
            ret = oapvd_vlc_filler(bs, (pbu_size - 4));
        }
        curpos += (pbu_size + 4);
        remain -= (pbu_size + 4);
    }
    stat->aui.num_frms = frame_cnt;
    oapv_assert_rv(ofrms->num_frms == frame_cnt, OAPV_ERR);
    return ret;

ERR:
    return ret;
}

int oapvd_config(oapvd_t did, int cfg, void *buf, int *size)
{
    oapvd_ctx_t *ctx;
    int t0 = 0;

    ctx = dec_id_to_ctx(did);
    oapv_assert_rv(ctx, OAPV_ERR_INVALID_ARGUMENT);

    switch (cfg)
    {
    /* set config ************************************************************/
    case OAPV_CFG_SET_USE_FRM_HASH:
        ctx->use_frm_hash = (*((int *)buf)) ? 1 : 0;
        break;

    default:
        oapv_assert_rv(0, OAPV_ERR_UNSUPPORTED);
    }
    return OAPV_OK;
}

int oapvd_info(void* au, int au_size, oapv_au_info_t* aui)
{
    int ret, remain, frm_count = 0;
    int pbu_cnt = 0;
    u8 * curpos;

    curpos = (u8*)au;
    remain = au_size;

    while(remain > 8) // FIX-ME (8byte?)
    {
        u32 pbu_size = 0;

        oapv_bs_t bs;
        oapv_bsr_init(&bs, curpos, remain, NULL);

        oapvd_vlc_pbu_size(&bs, &pbu_size); // 4 byte
        oapv_assert_rv(pbu_size > 0, OAPV_ERR_MALFORMED_BITSTREAM);

        /* pbu header */
        oapv_pbuh_t pbuh;
        oapvd_vlc_pbu_header(&bs, &pbuh); // 4 byte

        if (pbuh.pbu_type == OAPV_PBU_TYPE_AU_INFO) {
            // parse access_unit_info in PBU
            oapv_aui_t ai;

            ret = oapvd_vlc_au_info(&bs, &ai);
            oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);

            aui->num_frms = ai.num_frames;
            for(int i = 0; i < ai.num_frames; i++){
                copy_fi_to_finfo(&ai.frame_info[i], ai.pbu_type[i], ai.group_id[i], &aui->frm_info[i]);
            }
            return OAPV_OK;
        }
        if (pbuh.pbu_type == OAPV_PBU_TYPE_PRIMARY_FRAME ||
            pbuh.pbu_type == OAPV_PBU_TYPE_NON_PRIMARY_FRAME ||
            pbuh.pbu_type == OAPV_PBU_TYPE_PREVIEW_FRAME ||
            pbuh.pbu_type == OAPV_PBU_TYPE_DEPTH_FRAME ||
            pbuh.pbu_type == OAPV_PBU_TYPE_ALPHA_FRAME){
            // parse frame_info in PBU
            oapv_fi_t fi;

            ret = oapvd_vlc_frame_info(&bs, &fi);
            oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);

            copy_fi_to_finfo(&fi, pbuh.pbu_type, pbuh.group_id, &aui->frm_info[frm_count]);
            frm_count++;
        }
        aui->num_frms = frm_count;

        curpos += (pbu_size + 4); // includes pbu_size syntax
        remain -= (pbu_size + 4);

        ++pbu_cnt;
    }
    return OAPV_OK;
}

///////////////////////////////////////////////////////////////////////////////
// end of decoder code
#endif // ENABLE_DECODER
///////////////////////////////////////////////////////////////////////////////