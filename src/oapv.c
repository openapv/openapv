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
        sft_hor = get_chroma_sft_w(cfi);
        sft_ver = get_chroma_sft_h(cfi);
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
    int sft_hor, sft_ver, s_src;
    int bd = OAPV_CS_GET_BYTE_DEPTH(imgb->cs);
    int size_scale = 1;
    int tc = c;

    if (c == 0)
    {
        sft_hor = sft_ver = 0;
    }
    else
    {
        u8 cfi = color_format_to_chroma_format_idc(OAPV_CS_GET_FORMAT(imgb->cs));
        sft_hor = get_chroma_sft_w(cfi);
        sft_ver = get_chroma_sft_h(cfi);
        size_scale = 2;
        tc = 1;
    }

    s_src = imgb->s[tc] >> (bd > 1 ? 1 : 0);
    src = ((u16*)imgb->a[tc]) + ((y_l >> sft_ver) * s_src) + ((x_l * size_scale) >> sft_hor);
    dst = (u16*)block;

    for (int i = 0; i < (h_l); i++)
    {
        for (int j = 0; j < (w_l); j++)
        {
            dst[j] = (src[j * size_scale + (c >> 1)] >> 6);
        }
        src += s_src;
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
        sft_hor = get_chroma_sft_w(cfi);
        sft_ver = get_chroma_sft_h(cfi);
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
        sft_hor = get_chroma_sft_w(cfi);
        sft_ver = get_chroma_sft_h(cfi);
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

static void plus_mid_val(s16* coef, int b_w, int b_h, int bit_depth)
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
// start of encoder code
#if ENABLE_ENCODER
///////////////////////////////////////////////////////////////////////////////

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

static oapve_core_t * enc_core_alloc()
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

static void enc_minus_mid_val(s16* coef, int w_blk, int h_blk, int bit_depth)
{
  int mid_val = 1 << (bit_depth - 1);
  for (int i = 0; i < h_blk * w_blk; i++)
  {
    coef[i] -= mid_val;
  }
}

static int enc_set_tile_info(oapv_tile_info_t* ti, int w_pel, int h_pel, int tile_w,
    int tile_h, int* num_tile_cols, int* num_tile_rows, int* num_tiles)
{
    (*num_tile_cols) = (w_pel + (tile_w - 1)) / tile_w;
    (*num_tile_rows) = (h_pel + (tile_h - 1)) / tile_h;
    (*num_tiles) = (*num_tile_cols) * (*num_tile_rows);

    for (int i = 0; i < (*num_tiles); i++)
    {
        int tx = (i % (*num_tile_cols)) * tile_w;
        int ty = (i / (*num_tile_cols)) * tile_h;
        ti[i].x = tx;
        ti[i].y = ty;
        ti[i].w = tx + tile_w > w_pel ? w_pel - tx : tile_w;
        ti[i].h = ty + tile_h > h_pel ? h_pel - ty : tile_h;
    }
  return OAPV_OK;
}


static double enc_block(oapve_ctx_t* ctx, oapve_core_t* core, int x, int y, int log2_w, int log2_h, int c)
{
    ALIGNED_16(s16 coef_t[OAPV_BLK_D]);
    int b_w = 1 << log2_w;
    int b_h = 1 << log2_h;
    int bit_depth = OAPV_CS_GET_BIT_DEPTH(ctx->imgb->cs);
    int qp = ctx->th[core->tile_idx].tile_qp[c];
    int qscale = oapv_quant_scale[qp % 6];

    ctx->fn_imgb_to_block(ctx->imgb, c, x, y, b_w, b_h, core->coef);
    enc_minus_mid_val(core->coef, b_w, b_h, bit_depth);

    oapv_trans(ctx, core->coef, log2_w, log2_h, bit_depth);
    (ctx->fn_quantb)[0](qp, core->q_matrix_enc[c], core->coef, log2_w, log2_h, qscale, c, bit_depth, c ? 128 : 212);

    int prev_dc = core->prev_dc[c];
    core->prev_dc[c] = core->coef[0];
    core->coef[0] = core->coef[0] - prev_dc;

    if (ctx->param->is_rec)
    {
        oapv_mcpy(coef_t, core->coef, sizeof(s16) * OAPV_BLK_D);
        int dqscale = oapv_tbl_dq_scale[qp % 6];
        coef_t[0] = coef_t[0] + prev_dc;
        oapv_itdq_block(ctx->fn_itx, ctx->fn_iquant, core->q_matrix_dec[c], coef_t, log2_w, log2_h, dqscale, bit_depth, qp);

        plus_mid_val(coef_t, b_w, b_h, bit_depth);
        /*store recon*/
        ctx->fn_block_to_imgb(coef_t, c, x, y, (b_w), (b_h), ctx->rec);
    }

    return 0;
}

static double enc_block_rdo(oapve_ctx_t* ctx, oapve_core_t* core, int x, int y, int log2_w, int log2_h, int c)
{
    oapv_bs_t* bs = &ctx->bs_thread[core->thread_idx];
    int b_w = 1 << log2_w;
    int b_h = 1 << log2_h;
    int bit_depth = OAPV_CS_GET_BIT_DEPTH(ctx->imgb->cs);
    int nnz;
    double cost;
    int qp = ctx->th[core->tile_idx].tile_qp[c];

    s16 best_coeff[OAPV_BLK_D];
    s16 best_recon[OAPV_BLK_D];
    s16 recon[OAPV_BLK_D];
    s16 coeff[OAPV_BLK_D];
    double best_cost = MAX_COST;

    double lambda = 0.57 * pow(2.0, ((double)(qp - 12)) / 3.0);
    lambda = c ? lambda / pow(2.0, (-2) / 2.0) : lambda;

    ctx->fn_imgb_to_block(ctx->imgb, c, x, y, b_w, b_h, core->coef);
    enc_minus_mid_val(core->coef, b_w, b_h, bit_depth);
    oapv_trans(ctx, core->coef, log2_w, log2_h, bit_depth);

    int qscale = oapv_quant_scale[qp % 6];

    int prev_dc = core->prev_dc[c];
    int best_prev_dc = 0;

    int stored_dc_ctx = core->prev_dc_ctx[c];
    int stored_1st_ac_ctx = core->prev_1st_ac_ctx[c];

    int min_val = c ? 118 : 192;
    int max_val = c ? 138 : 232;
    int step = 10;

    for (int dz = min_val; dz <= max_val; dz += step)
    {
        core->prev_dc_ctx[c] = stored_dc_ctx;
        core->prev_1st_ac_ctx[c] = stored_1st_ac_ctx;

        oapv_mcpy(coeff, core->coef, sizeof(s16) * OAPV_BLK_D);

        nnz = (ctx->fn_quantb)[0](qp, core->q_matrix_enc[c], coeff, log2_w, log2_h, qscale, c, bit_depth, dz);

        prev_dc = core->prev_dc[c];
        int tmp_dc = prev_dc;
        prev_dc = coeff[0];
        coeff[0] = coeff[0] - tmp_dc;

        bs->is_bin_count = 1;
        bs->bin_count = 0;
        oapve_vlc_run_length_cc(ctx, core, bs, coeff, log2_w, log2_h, nnz, c);
        bs->is_bin_count = 0;

        oapv_mcpy(recon, coeff, sizeof(s16) * OAPV_BLK_D);

        int dqscale = oapv_tbl_dq_scale[qp % 6];
        recon[0] = recon[0] + tmp_dc;

        oapv_itdq_block(ctx->fn_itx, ctx->fn_iquant, core->q_matrix_dec[c], recon, log2_w, log2_h, dqscale, bit_depth, qp);

        plus_mid_val(recon, b_w, b_h, bit_depth);
        s16* org = (s16*)((u8*)ctx->imgb->a[c] + ((y >> (c ? ctx->ch_sft_h : 0)) * ctx->imgb->s[c]) + ((x >> (c ? ctx->ch_sft_w : 0)) * 2));
        int dist = (int)ctx->fn_ssd[0](b_w, b_h, org, recon, (ctx->imgb->s[c] >> 1), b_w, 10);

        cost = lambda * bs->bin_count + dist;

        if (cost < best_cost)
        {
            best_cost = cost;
            oapv_mcpy(best_coeff, coeff, sizeof(s16) * OAPV_BLK_D);
            oapv_mcpy(best_recon, recon, sizeof(s16) * OAPV_BLK_D);
            best_prev_dc = prev_dc;
        }
    }
    core->prev_dc[c] = best_prev_dc;
    core->prev_dc_ctx[c] = stored_dc_ctx;
    core->prev_1st_ac_ctx[c] = stored_1st_ac_ctx;
    oapv_mcpy(core->coef, best_coeff, sizeof(s16) * OAPV_BLK_D);

    if (ctx->param->is_rec)
    {
        /*store recon*/
        ctx->fn_block_to_imgb(best_recon, c, x, y, (b_w), (b_h), ctx->rec);
    }
    return best_cost;
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

    ctx->num_comp = get_num_comp(param->csp);

    if (param->preset == OAPV_PRESET_SLOW ||
        param->preset == OAPV_PRESET_PLACEBO) {
        ctx->fn_block = enc_block_rdo;
    }
    else {
        ctx->fn_block = enc_block;
    }

    ctx->log2_block = OAPV_LOG2_BLK;

    ctx->mb = 1 << OAPV_LOG2_MB;

    /* set various value */
    ctx->w = ((ctx->param->w + (ctx->mb - 1)) >> OAPV_LOG2_MB_W) << OAPV_LOG2_MB_W;
    ctx->h = ((ctx->param->h + (ctx->mb - 1)) >> OAPV_LOG2_MB_H) << OAPV_LOG2_MB_H;

    int tile_w = ctx->param->tile_w_mb * ctx->mb;
    int tile_h = ctx->param->tile_h_mb * ctx->mb;
    enc_set_tile_info(ctx->ti, ctx->w, ctx->h, tile_w, tile_h, &ctx->num_tile_cols, &ctx->num_tile_rows, &ctx->num_tiles);

    int size = ctx->num_tiles * sizeof(oapv_th_t);
    ctx->th = (oapv_th_t*)oapv_malloc(size);
    oapv_assert_rv(ctx->th, OAPV_ERR_UNKNOWN);

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
        core = enc_core_alloc();
        oapv_assert_gv(core != NULL, ret, OAPV_ERR_OUT_OF_MEMORY, ERR);
        ctx->core[i] = core;

        oapve_rc_core_t* rc_core = (oapve_rc_core_t*)oapv_malloc_fast(sizeof(oapve_rc_core_t));
        oapv_assert_gv(rc_core != NULL, ret, OAPV_ERR_OUT_OF_MEMORY, ERR);
        oapv_mset_x64a(rc_core, 0, sizeof(oapve_rc_core_t));
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
        ctx->tile_stat[i] = ENC_TILE_STAT_NOT_ENCODED;
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

static int enc_vlc_tile(oapve_ctx_t* ctx, oapve_core_t* core, int x, int y, int tile_idx, int c)
{
    oapv_bs_t* bs = &ctx->bs_thread[core->thread_idx];
    int h_mb, w_mb, b_h, b_w;

    u8* bs_cur = oapv_bsw_sink(bs);
    oapv_assert_rv(bsw_is_align8(bs), OAPV_ERR_MALFORMED_BITSTREAM);

    w_mb = OAPV_MB_W >> (c ? ctx->ch_sft_w : 0);
    h_mb = OAPV_MB_H >> (c ? ctx->ch_sft_h : 0);

    for (int ty = 0; ty < ctx->ti[tile_idx].h && y + ty < ctx->h; ty += ctx->mb)
    {
        for (int tx = 0; tx < ctx->ti[tile_idx].w; tx += ctx->mb)
        {
            int mb_x = x + tx;
            int mb_y = y + ty;

            b_h = 1 << OAPV_LOG2_BLK_H;
            b_w = 1 << OAPV_LOG2_BLK_W;

            for (int b_y = 0; b_y < h_mb; b_y += b_h)
            {
                for (int b_x = 0; b_x < w_mb; b_x += b_w)
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

                    ctx->fn_block(ctx, core, cx, cy, OAPV_LOG2_BLK_W, OAPV_LOG2_BLK_H, c);
                    oapve_vlc_dc_coeff(ctx, core, bs, core->coef[0], c);
                    oapve_vlc_ac_coeff(ctx, core, bs, core->coef, 0, c);
                }
            }
        }
    }

    /* byte align */
    while (!bsw_is_align8(bs))
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
    int x = ctx->ti[core->tile_idx].x;
    int y = ctx->ti[core->tile_idx].y;
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
    OAPV_TRACE_SET(0);
    oapve_vlc_tile_header(ctx, bs, &ctx->th[tile_idx]);

    for (int c = 0; c < ctx->num_comp; c++)
    {
        for (int cidx = 0; cidx < ctx->num_comp; cidx++)
        {
            int cnt = 0;
            int qp = ctx->th[tile_idx].tile_qp[cidx];
            int qscale = oapv_quant_scale[qp % 6];
            s32 scale_multiply_16 = (s32)(qscale << 4);
            for (int y = 0; y < OAPV_BLK_H; y++)
            {
                for (int x = 0; x < OAPV_BLK_W; x++)
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
                for (int y = 0; y < OAPV_BLK_H; y++)
                {
                    for (int x = 0; x < OAPV_BLK_W; x++)
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
    OAPV_TRACE_SET(TRACE_HLS);
    oapve_vlc_tile_header(ctx, &bs_th, &ctx->th[tile_idx]);
    oapv_bsw_deinit(&bs_th);

    ctx->tile_stat[core->tile_idx] = ENC_TILE_STAT_ENCODED;
    return OAPV_OK;
}

static int enc_thread_tile(void *arg)
{
    oapve_core_t *core = (oapve_core_t *)arg;
    oapve_ctx_t *ctx = core->ctx;
    int tile_idx, ret, run = 1;

    while (run) {
        ret = enc_tile(ctx, core);
        oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

        oapv_tpool_enter_cs(ctx->sync_obj);
        for (tile_idx = 0; tile_idx < ctx->num_tiles; tile_idx++)
        {
            if (ctx->tile_stat[tile_idx] == ENC_TILE_STAT_NOT_ENCODED)
            {
                ctx->tile_stat[tile_idx] = ENC_TILE_STAT_ON_ENCODING;
                core->tile_idx = tile_idx;
                break;
            }
        }
        oapv_tpool_leave_cs(ctx->sync_obj);
        if (tile_idx == ctx->num_tiles)
        {
            run = 0;
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

}

static int enc_prepare(oapve_ctx_t *ctx, oapv_imgb_t *imgb, oapv_bitb_t *bitb, oapv_imgb_t *rec)
{
    oapv_assert_rv(ctx->qp[Y_C] >= MIN_QUANT && ctx->qp[Y_C] <= MAX_QUANT, OAPV_ERR_INVALID_ARGUMENT);

    ctx->cfi= color_format_to_chroma_format_idc(OAPV_CS_GET_FORMAT(imgb->cs));
    ctx->num_comp = get_num_comp(ctx->cfi);
    ctx->ch_sft_w = get_chroma_sft_w(ctx->cfi);
    ctx->ch_sft_h = get_chroma_sft_h(ctx->cfi);

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
        ctx->tile_stat[i] = ENC_TILE_STAT_NOT_ENCODED;
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

static int enc_finish(oapve_ctx_t *ctx, oapve_stat_t *stat)
{
    /* de-init BSW */
    oapv_bsw_deinit(&ctx->bs);

    stat->write = bsw_get_write_byte(&ctx->bs);

    imgb_release(ctx->imgb);
    if (ctx->rec)
    {
        imgb_release(ctx->rec);
        ctx->rec = NULL;
    }
    return OAPV_OK;
}

static int enc_frame(oapve_ctx_t *ctx)
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

        double bits_pic = ((double)ctx->param->bitrate * 1000) / ((double)ctx->param->fps_num / ctx->param->fps_den);
        for (int i = 0; i < ctx->num_tiles; i++)
        {
            ctx->ti[i].rc.target_bits_left = bits_pic * ctx->ti[i].rc.cost / cost_sum;
            ctx->ti[i].rc.target_bits = ctx->ti[i].rc.target_bits_left;
        }

        ctx->rc_param.lambda = oapve_rc_estimate_pic_lambda(ctx, cost_sum);
        ctx->rc_param.qp = oapve_rc_estimate_pic_qp(ctx->rc_param.lambda);
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
        ctx->tile_stat[i] = ENC_TILE_STAT_ON_ENCODING;
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

static int enc_platform_init(oapve_ctx_t * ctx)
{
    // default settings
    ctx->fn_sad               = oapv_tbl_sad_16b;
    ctx->fn_ssd               = oapv_tbl_ssd_16b;
    ctx->fn_diff              = oapv_tbl_diff_16b;
    ctx->fn_itx                 = oapv_tbl_fn_itx;
    ctx->fn_txb                 = oapv_tbl_fn_tx;
    ctx->fn_quantb              = oapv_tbl_fn_quant;
    ctx->fn_had8x8              = oapv_dc_removed_had8x8;

#if X86_SSE
    int check_cpu, support_sse, support_avx2;

    check_cpu = oapv_check_cpu_info_x86();
    support_sse  = (check_cpu >> 0) & 1;
    support_avx2 = (check_cpu >> 2) & 1;

    if (support_avx2)
    {
        ctx->fn_sad               = oapv_tbl_sad_16b_avx;
        ctx->fn_ssd               = oapv_tbl_ssd_16b_sse;
        ctx->fn_diff              = oapv_tbl_diff_16b_sse;
        ctx->fn_itx                 = oapv_tbl_fn_itx_avx;
        ctx->fn_txb                 = oapv_tbl_txb_avx;
        ctx->fn_quantb              = oapv_tbl_quantb_avx;
        ctx->fn_iquant              = oapv_tbl_fn_iquant_avx;
        ctx->fn_had8x8              = oapv_dc_removed_had8x8_sse;
    }
    else if (support_sse)
    {
        ctx->fn_sad               = oapv_tbl_sad_16b_sse;
        ctx->fn_ssd               = oapv_tbl_ssd_16b_sse;
        ctx->fn_diff              = oapv_tbl_diff_16b_sse;
        ctx->fn_itx                 = oapv_tbl_fn_itx;
        ctx->fn_txb                 = oapv_tbl_fn_tx;
        ctx->fn_quantb              = oapv_tbl_fn_quant;
        ctx->fn_iquant              = oapv_tbl_fn_iquant;
        ctx->fn_had8x8              = oapv_dc_removed_had8x8_sse;
    }
#elif ARM_NEON
    ctx->fn_sad               = oapv_tbl_sad_16b_neon;
    ctx->fn_ssd               = oapv_tbl_ssd_16b_neon;
    ctx->fn_diff              = oapv_tbl_diff_16b_neon;
    ctx->fn_itx                 = oapv_tbl_fn_itx;
    ctx->fn_txb                 = oapv_tbl_fn_txb_neon;
    ctx->fn_quantb              = oapv_tbl_quantb_neon;
    ctx->fn_iquant              = oapv_tbl_fn_iquant;
    ctx->fn_had8x8              = oapv_dc_removed_had8x8;
#endif
    return OAPV_OK;
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
        ret = enc_platform_init(ctx);
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
        ret = enc_frame(ctx);
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

    oapvm_ctx_t* md_list = mid;
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
    ret = enc_finish(ctx, stat);
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
    case OAPV_CFG_SET_FPS_NUM:
        oapv_assert_rv(*size == sizeof(int), OAPV_ERR_INVALID_ARGUMENT);
        t0 = *((int*)buf);
        oapv_assert_rv(t0 > 0, OAPV_ERR_INVALID_ARGUMENT);
        ctx->param->fps_num = t0;
        break;
    case OAPV_CFG_SET_FPS_DEN:
        oapv_assert_rv(*size == sizeof(int), OAPV_ERR_INVALID_ARGUMENT);
        t0 = *((int*)buf);
        oapv_assert_rv(t0 > 0, OAPV_ERR_INVALID_ARGUMENT);
        ctx->param->fps_den = t0;
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
    case OAPV_CFG_GET_FPS_NUM:
        oapv_assert_rv(*size == sizeof(int), OAPV_ERR_INVALID_ARGUMENT);
        *((int*)buf) = ctx->param->fps_num;
        break;
    case OAPV_CFG_GET_FPS_DEN:
        oapv_assert_rv(*size == sizeof(int), OAPV_ERR_INVALID_ARGUMENT);
        *((int*)buf) = ctx->param->fps_den;
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
    param->level_idc = (int)(4.1 * 30);
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

static int dec_block(oapvd_ctx_t* ctx, oapvd_core_t* core, int x, int y, int log2_w, int log2_h, int c)
{
    int bit_depth = OAPV_CS_GET_BIT_DEPTH(ctx->imgb->cs);

    int b_w = 1 << log2_w;
    int b_h = 1 << log2_h;

    core->coef[0] += core->prev_dc[c];
    core->prev_dc[c] = core->coef[0];

    int dqscale = oapv_tbl_dq_scale[core->qp[c] % 6];

    int shift = bit_depth - 2 - (core->qp[c] / 6);

    ctx->fn_iquant[0](core->coef, core->q_matrix_dec[c], 3, 3, dqscale, shift);

    oapv_itrans(ctx->fn_itx, core->coef, 3, 3, bit_depth);

    plus_mid_val(core->coef, b_w, b_h, bit_depth);
    // store to output
    ctx->fn_block_to_imgb(core->coef, c, x, y, b_w, b_h, ctx->imgb);

    return OAPV_OK;
}

static int dec_set_tile_info(oapvd_tile_t* tile, int w_pel, int h_pel, int tile_w,
    int tile_h, int* num_tile_cols, int* num_tile_rows, int* num_tiles)
{
    (*num_tile_cols) = (w_pel + (tile_w - 1)) / tile_w;
    (*num_tile_rows) = (h_pel + (tile_h - 1)) / tile_h;
    (*num_tiles) = (*num_tile_cols) * (*num_tile_rows);

    for (int i = 0; i < (*num_tiles); i++)
    {
        int tx = (i % (*num_tile_cols)) * tile_w;
        int ty = (i / (*num_tile_cols)) * tile_h;
        tile[i].x = tx;
        tile[i].y = ty;
        tile[i].w = tx + tile_w > w_pel ? w_pel - tx : tile_w;
        tile[i].h = ty + tile_h > h_pel ? h_pel - ty : tile_h;
    }
  return OAPV_OK;
}

static int dec_frame_prepare(oapvd_ctx_t* ctx, oapv_imgb_t* imgb)
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

    ctx->cfi = ctx->fh.fi.chroma_format_idc;
    ctx->num_comp = get_num_comp(ctx->cfi);
    ctx->ch_sft_w = get_chroma_sft_w(ctx->cfi);
    ctx->ch_sft_h = get_chroma_sft_h(ctx->cfi);

    ctx->w = oapv_align_value(ctx->fh.fi.frame_width, OAPV_MB_W);
    ctx->h = oapv_align_value(ctx->fh.fi.frame_height, OAPV_MB_H);

    int tile_w = ctx->fh.tile_width_in_mbs * OAPV_MB_W;
    int tile_h = ctx->fh.tile_height_in_mbs * OAPV_MB_H;

    dec_set_tile_info(ctx->tile, ctx->w, ctx->h, tile_w, tile_h, &(ctx->num_tile_cols), &(ctx->num_tile_rows), &(ctx->num_tiles));

    for (int i = 0; i < ctx->num_tiles; i++)
    {
        ctx->tile[i].bs_beg = NULL;
    }
    ctx->tile[0].bs_beg = oapv_bsr_sink(&ctx->bs);

    for (int i = 0; i < ctx->num_tiles; i++)
    {
        ctx->tile[i].stat = DEC_TILE_STAT_NOT_DECODED;
    }

    return OAPV_OK;
}

static int dec_frame_finish(oapvd_ctx_t *ctx)
{
    oapv_mset(&ctx->bs, 0, sizeof(oapv_bs_t)); // clean data
    imgb_release(ctx->imgb);                     // decrease reference cnout
    ctx->imgb = NULL;
    return OAPV_OK;
}

static int dec_tile_comp(oapvd_tile_t * tile, oapvd_ctx_t* ctx, oapvd_core_t* core, oapv_bs_t* bs, int c)
{
    int h_mb, w_mb, h_blk, w_blk, ty, tx, by, bx, cx, cy;
    int ret;
    OAPV_TRACE_SET(1);

    h_mb = OAPV_MB_H >> (c ? ctx->ch_sft_h : 0);
    w_mb = OAPV_MB_W >> (c ? ctx->ch_sft_w : 0);

    for (ty = 0; ty < tile->h && tile->y + ty < ctx->h; ty += OAPV_MB_H)
    {
        for (tx = 0; tx < tile->w; tx += OAPV_MB_W)
        {
            h_blk = 1 << OAPV_LOG2_BLK_H;
            w_blk = 1 << OAPV_LOG2_BLK_W;

            for (by = 0; by < h_mb; by += h_blk)
            {
                for (bx = 0; bx < w_mb; bx += w_blk)
                {
                    cx = (tile->x + tx) + (bx << (c ? ctx->ch_sft_w : 0));
                    cy = (tile->y + ty) + (by << (c ? ctx->ch_sft_h : 0));

#if ENC_DEC_TRACE
                    OAPV_TRACE_SET(TRACE_COEF_BIN);
                    OAPV_TRACE_COUNTER;
                    OAPV_TRACE_STR("x pos ");
                    OAPV_TRACE_INT(cx);
                    OAPV_TRACE_STR("y pos ");
                    OAPV_TRACE_INT(cy);
                    OAPV_TRACE_STR("width ");
                    OAPV_TRACE_INT(h_blk);
                    OAPV_TRACE_STR("height ");
                    OAPV_TRACE_INT(w_blk);
                    OAPV_TRACE_STR("CH ");
                    OAPV_TRACE_STR((c == 0 ? "Y" : (c == 1 ? "U" : "V")));
                    OAPV_TRACE_STR("\n");
#endif

                    ret = oapvd_vlc_dc_coeff(ctx, core, bs, &core->coef[0], c);
                    oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);
                    ret = oapvd_vlc_ac_coeff(ctx, core, bs, core->coef, c);
                    oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);
                    ret = dec_block(ctx, core, cx, cy, OAPV_LOG2_BLK_W, OAPV_LOG2_BLK_H, c);
                    oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);
                }
            }
        }
    }

    /* byte align */
    oapv_bsr_align8(bs);
    return OAPV_OK;
}

static int dec_tile(oapvd_core_t* core, oapvd_tile_t * tile)
{
    int ret;
    oapvd_ctx_t *ctx = core->ctx;
    oapv_bs_t bs;

    oapv_bsr_init(&bs, tile->bs_beg + OAPV_TILE_SIZE_LEN, tile->data_size, NULL);
    oapvd_vlc_tile_header(&bs, ctx, &tile->th);

    int cnt_dec = 0;
    for (int cidx = 0; cidx < ctx->num_comp; cidx++)
    {
        cnt_dec = 0;
        int qp_dec = tile->th.tile_qp[cidx];
        int dqscale = oapv_tbl_dq_scale[qp_dec % 6];
        for (int y = 0; y < OAPV_BLK_H; y++)
        {
            for (int x = 0; x < OAPV_BLK_W; x++)
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
        core->qp[c] = tile->th.tile_qp[c];
        ret = dec_tile_comp(tile, ctx, core, &bs, c);
        oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);
    }

    oapvd_vlc_tile_dummy_data(&bs);
    tile->stat = DEC_TILE_STAT_DECODED;
    return OAPV_OK;
}


static int dec_thread_tile(void* arg)
{
    oapv_bs_t bs;
    oapvd_core_t* core = (oapvd_core_t*)arg;
    oapvd_ctx_t* ctx = core->ctx;
    oapvd_tile_t * tile = ctx->tile;

    int i, ret, run, tile_idx = 0;
    while (1)
    {
        // find not decoded tile
        oapv_tpool_enter_cs(ctx->sync_obj);
        for (i = 0; i < ctx->num_tiles; i++)
        {
            if (tile[i].stat == DEC_TILE_STAT_NOT_DECODED)
            {
                tile[i].stat = DEC_TILE_STAT_ON_DECODING;
                tile_idx = i;
                break;
            }
        }
        oapv_tpool_leave_cs(ctx->sync_obj);
        if (i == ctx->num_tiles)
        {
            break;
        }

        // wait until to know bistream start position
        run = 1;
        while (run)
        {
            oapv_tpool_enter_cs(ctx->sync_obj);
            if (tile[tile_idx].bs_beg != NULL)
            {
                run = 0;
            }
            oapv_tpool_leave_cs(ctx->sync_obj);
        }
        /* read tile size */
        oapv_bsr_init(&bs, tile[tile_idx].bs_beg, OAPV_TILE_SIZE_LEN, NULL);
        tile[tile_idx].data_size = oapvd_vlc_tile_size(&bs);

        oapv_tpool_enter_cs(ctx->sync_obj);
        if (tile_idx + 1 < ctx->num_tiles)
        {
            tile[tile_idx + 1].bs_beg = tile[tile_idx].bs_beg + OAPV_TILE_SIZE_LEN + tile[tile_idx].data_size;
        }
        else
        {
            ctx->tile_end = tile[tile_idx].bs_beg + OAPV_TILE_SIZE_LEN + tile[tile_idx].data_size;
        }
        oapv_tpool_leave_cs(ctx->sync_obj);

        ret = dec_tile(core, &tile[tile_idx]);
        oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

    }
    return OAPV_OK;

ERR:
    return ret;
}

static void dec_flush(oapvd_ctx_t* ctx)
{
    if (ctx->cdesc.threads >= 2)
    {
        if (ctx->tpool)
        {
            //thread controller instance is present
            //terminate the created thread
            for (int i = 0; i < ctx->cdesc.threads - 1; i++)
            {
                if (ctx->thread_id[i])
                {
                    //valid thread instance
                    ctx->tpool->release(&ctx->thread_id[i]);
                }
            }
            //dinitialize the tpool
            oapv_tpool_deinit(ctx->tpool);
            oapv_mfree(ctx->tpool);
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
    int i, ret = OAPV_OK;

    if (ctx->core[0] == NULL)
    {
        // create cores
        for (i = 0; i < ctx->cdesc.threads; i++)
        {
            ctx->core[i] = dec_core_alloc();
            oapv_assert_gv(ctx->core[i], ret, OAPV_ERR_OUT_OF_MEMORY, ERR);
            ctx->core[i]->ctx = ctx;
        }
    }

    //initialize the threads to NULL
    for (i = 0; i < OAPV_MAX_THREADS; i++)
    {
        ctx->thread_id[i] = 0;
    }

    //get the context synchronization handle
    ctx->sync_obj = oapv_tpool_sync_obj_create();
    oapv_assert_gv(ctx->sync_obj != NULL, ret, OAPV_ERR_UNKNOWN, ERR);

    if (ctx->cdesc.threads >= 2)
    {
        ctx->tpool = oapv_malloc(sizeof(oapv_tpool_t));
        oapv_tpool_init(ctx->tpool, ctx->cdesc.threads - 1);
        for (i = 0; i < ctx->cdesc.threads - 1; i++)
        {
            ctx->thread_id[i] = ctx->tpool->create(ctx->tpool, i);
            oapv_assert_gv(ctx->thread_id[i] != NULL, ret, OAPV_ERR_UNKNOWN, ERR);
        }
    }
    return OAPV_OK;

ERR:
    dec_flush(ctx);

    return ret;
}

static int dec_platform_init(oapvd_ctx_t * ctx)
{
    // default settings
    ctx->fn_itx = oapv_tbl_fn_itx;
    ctx->fn_iquant = oapv_tbl_fn_iquant;

#if X86_SSE
    int check_cpu, support_sse, support_avx2;

    check_cpu = oapv_check_cpu_info_x86();
    support_sse  = (check_cpu >> 0) & 1;
    support_avx2 = (check_cpu >> 2) & 1;

    if (support_avx2)
    {
        ctx->fn_itx = oapv_tbl_fn_itx_avx;
        ctx->fn_iquant = oapv_tbl_fn_iquant_avx;
    }
    else if (support_sse)
    {
        ctx->fn_itx = oapv_tbl_fn_itx;
        ctx->fn_iquant = oapv_tbl_fn_iquant;
    }
#elif ARM_NEON
    ctx->fn_itx = oapv_tbl_fn_itx;
    ctx->fn_iquant = oapv_tbl_fn_iquant;
#endif
    return OAPV_OK;
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

    /* initialize platform-specific variables */
    ret = dec_platform_init(ctx);
    oapv_assert_g(ret == OAPV_OK, ERR);

    /* ready for decoding */
    ret = dec_ready(ctx);
    oapv_assert_g(ret == OAPV_OK, ERR);

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

        pbu_size = oapvd_vlc_pbu_size(bs);
        oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);
        oapv_assert_g((pbu_size + 4) <= bs->size, ERR);

        ret = oapvd_vlc_pbu_header(bs, &pbuh);
        oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

        if (pbuh.pbu_type == OAPV_PBU_TYPE_PRIMARY_FRAME ||
        pbuh.pbu_type == OAPV_PBU_TYPE_NON_PRIMARY_FRAME ||
        pbuh.pbu_type == OAPV_PBU_TYPE_PREVIEW_FRAME ||
        pbuh.pbu_type == OAPV_PBU_TYPE_DEPTH_FRAME ||
        pbuh.pbu_type == OAPV_PBU_TYPE_ALPHA_FRAME)
        {
            ret = oapvd_vlc_frame_header(bs, &ctx->fh);
            oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

            ret = dec_frame_prepare(ctx, ofrms->frm[frame_cnt].imgb);
            oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

            int res;
            oapv_tpool_t* tpool = ctx->tpool;
            int parallel_task = 1;
            int tidx = 0;

            parallel_task = (ctx->cdesc.threads > ctx->num_tiles) ? ctx->num_tiles : ctx->cdesc.threads;

            /* decode tiles ************************************/
            for (tidx = 0; tidx < (parallel_task - 1); tidx++) {
                tpool->run(ctx->thread_id[tidx], dec_thread_tile,
                    (void*)ctx->core[tidx]);
            }
            ret = dec_thread_tile((void*)ctx->core[tidx]);
            for (tidx = 0; tidx < parallel_task - 1; tidx++)
            {
                tpool->join(ctx->thread_id[tidx], &res);
                if (OAPV_FAILED(res))
                {
                    ret = res;
                }
            }
            /****************************************************/

            /* READ FILLER HERE !!! */

            oapv_bsr_move(&ctx->bs, ctx->tile_end);
            stat->read += bsr_get_read_byte(&ctx->bs);

            copy_fi_to_finfo(&ctx->fh.fi, pbuh.pbu_type, pbuh.group_id, &stat->aui.frm_info[frame_cnt]);
            if (ret == OAPV_OK && ctx->use_frm_hash)
            {
                oapv_imgb_set_md5(ctx->imgb);
            }
            ret = dec_frame_finish(ctx); // FIX-ME
            oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

            ofrms->frm[frame_cnt].pbu_type = pbuh.pbu_type;
            ofrms->frm[frame_cnt].group_id = pbuh.group_id;
            stat->frm_size[frame_cnt] = pbu_size + 4/* PUB size length*/;
            frame_cnt++;
        }
        else if (pbuh.pbu_type == OAPV_PBU_TYPE_METADATA)
        {
            ret = oapvd_vlc_metadata(bs, pbu_size, mid, pbuh.group_id);
            oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

            stat->read += bsr_get_read_byte(&ctx->bs);
        }
        else if (pbuh.pbu_type == OAPV_PBU_TYPE_FILLER)
        {
            ret = oapvd_vlc_filler(bs, (pbu_size - 4));
            oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);
        }
        curpos += (pbu_size + 4);
        remain -= (pbu_size + 4);
    }
    stat->aui.num_frms = frame_cnt;
    oapv_assert_rv(ofrms->num_frms == frame_cnt, OAPV_ERR_MALFORMED_BITSTREAM);
    return ret;

ERR:
    return ret;
}

int oapvd_config(oapvd_t did, int cfg, void *buf, int *size)
{
    oapvd_ctx_t *ctx;

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

        pbu_size = oapvd_vlc_pbu_size(&bs); // 4 byte
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