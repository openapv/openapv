/* Copyright (c) 2022-2023, Samsung Electronics Co., Ltd.
   All Rights Reserved. */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

   - Neither the name of the copyright owner, nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#include "oapv_rc.h"

int oapve_rc_get_tile_cost(oapve_ctx_t* ctx, int tile_idx)
{
    int step = 8;
    ALIGNED_16(pel temp[64]);
    int sum = 0;

    for (int c = Y_C; c < ctx->num_comp; c++)
    {
        int step_w = c ? 8 << ctx->ch_sft_w : 8;
        int step_h = c ? 8 << ctx->ch_sft_h : 8;
        for (int y = 0; y < ctx->ti[tile_idx].height; y += step_h)
        {
            for (int x = 0; x < ctx->ti[tile_idx].width; x += step_w)
            {
                int tx = ctx->ti[tile_idx].col + x;
                int ty = ctx->ti[tile_idx].row + y;
                ctx->fn_imgb_to_block(ctx->imgb, c, tx, ty, 8, 8, temp);
                sum += ctx->fn_had8x8(temp, 8);
                ctx->ti[tile_idx].rc.number_pixel += 64;
            }
        }
    }

    ctx->ti[tile_idx].rc.cost = sum;

    return OAPV_OK;
}

int get_tile_cost_thread(void* arg)
{
    rc_core_t* rc_core = (rc_core_t*)arg;
    oapve_ctx_t* ctx = rc_core->ctx;
    int tile_idx, ret;

    while (1) {
        ret = oapve_rc_get_tile_cost(ctx, rc_core->tile_idx);
        oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

        oapv_tpool_enter_cs(ctx->sync_obj);
        for (tile_idx = 0; tile_idx < ctx->num_tiles; tile_idx++)
        {
            if (ctx->sync_flag[tile_idx] == 0)
            {
                ctx->sync_flag[tile_idx] = 1;
                rc_core->tile_idx = tile_idx;
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

int oapve_rc_get_tile_cost_thread(oapve_ctx_t* ctx, u64* sum)
{
    for (int i = 0; i < ctx->num_tiles; i++) {
        ctx->sync_flag[i] = 0;
    }

    int parallel_task = 1;
    parallel_task = (ctx->cdesc.threads > ctx->num_tiles) ? ctx->num_tiles : ctx->cdesc.threads;

    oapv_tpool_t* tpool;
    tpool = ctx->tpool;

    for (int i = 0; i < parallel_task; i++) {
        ctx->sync_flag[i] = 1;
    }
    // run new threads
    int tidx = 0;
    for (tidx = 0; tidx < (parallel_task - 1); tidx++) {
        ctx->rc_core[tidx]->ctx = ctx;
        ctx->rc_core[tidx]->tile_idx = tidx;
        tpool->run(ctx->thread_id[tidx], get_tile_cost_thread, (void*)ctx->rc_core[tidx]);
    }
    // use main thread
    ctx->rc_core[tidx]->ctx = ctx;
    ctx->rc_core[tidx]->tile_idx = tidx;
    int ret = get_tile_cost_thread((void*)ctx->rc_core[tidx]);
    oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);

    for (int thread_num1 = 0; thread_num1 < parallel_task - 1; thread_num1++) {
        int res = tpool->join(ctx->thread_id[thread_num1], &ret);
        oapv_assert_rv(res == TPOOL_SUCCESS, ret);
        oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);
    }

    *sum = 0;
    for (int i = 0; i < ctx->num_tiles; i++)
    {
        *sum += ctx->ti[i].rc.cost;
        ctx->sync_flag[i] = 0;
    }

    return ret;
}

static double rc_calculate_lambda(double alpha, double beta, double cost_pixel, double bits_pixel)
{
    return ((alpha / 256.0) * pow(cost_pixel / bits_pixel, beta));
}

double oapve_rc_estimate_pic_lambda(oapve_ctx_t* ctx, double cost)
{
    int num_pixel = ctx->w * ctx->h;
    for (int c = 1; c < ctx->num_comp; c++)
    {
        int step_w = c ? 8 << ctx->ch_sft_w : 8;
        int step_h = c ? 8 << ctx->ch_sft_h : 8;
        num_pixel += (ctx->w * ctx->h) >> (ctx->ch_sft_h + ctx->ch_sft_w);
    }

    double alpha = ctx->rc_param.alpha;
    double beta = ctx->rc_param.beta;
    double bpp = (double)ctx->param->bitrate * 1000 / (double)(num_pixel * ctx->param->fps);

    double est_lambda = rc_calculate_lambda(alpha, beta, pow(cost / (double)num_pixel, OAPV_RC_BETA), bpp);
    est_lambda = oapv_clip3(0.1, 10000.0, est_lambda);
    const int lambda_prec = 1000000;

    est_lambda = (double)((s64)(est_lambda * (double)lambda_prec + 0.5)) / (double)lambda_prec;

    return est_lambda;
}

int oapve_rc_estimate_pic_qp(oapve_ctx_t* ctx, double lambda)
{
    int qp = (int)(4.2005 * log(lambda) + 13.7122 + 0.5) + OAPV_RC_QP_OFFSET;
    qp = oapv_clip3(MIN_QUANT, MAX_QUANT, qp);
    return qp;
}

void oapve_rc_get_qp(oapve_ctx_t* ctx, int tile_idx, int pic_qp, int* qp)
{
    int   tile_col_idx = tile_idx % ctx->num_tile_cols;
    int   tile_row_idx = tile_idx / ctx->num_tile_cols;

    double   alpha = ctx->rc_param.alpha;
    double   beta = ctx->rc_param.beta;

    double cost_pixel = ctx->ti[tile_idx].rc.cost / (double)ctx->ti[tile_idx].rc.number_pixel;
    cost_pixel = pow(cost_pixel, OAPV_RC_BETA);

    double bit_pixel =  (double)ctx->ti[tile_idx].rc.target_bits / (double)ctx->ti[tile_idx].rc.number_pixel;
    double est_lambda = rc_calculate_lambda(alpha, beta, cost_pixel, bit_pixel);

    int min_qp = pic_qp - 2 - OAPV_RC_QP_OFFSET;
    int max_qp = pic_qp + 2 - OAPV_RC_QP_OFFSET;

    double max_lambda = exp(((double)(max_qp + 0.49) - 13.7122) / 4.2005);
    double min_lambda = exp(((double)(min_qp - 0.49) - 13.7122) / 4.2005);

    const int LAMBDA_PREC = 1000000;
    est_lambda = oapv_clip3(min_lambda, max_lambda, est_lambda);
    est_lambda = (double)((s64)(est_lambda * (double)LAMBDA_PREC + 0.5)) / (double)LAMBDA_PREC;
    *qp = (int)(4.2005 * log(est_lambda) + 13.7122 + 0.5);
    *qp = oapv_clip3(min_qp, max_qp, *qp);
    *qp += OAPV_RC_QP_OFFSET;

    return;
}

void oapve_rc_update_after_pic(oapve_ctx_t* ctx, double cost)
{
    int num_pixel = ctx->w * ctx->h;
    for (int c = 1; c < ctx->num_comp; c++)
    {
        num_pixel += (ctx->w * ctx->h) >> (ctx->ch_sft_h + ctx->ch_sft_w);
    }

    int total_bits = 0;
    for (int i = 0; i < ctx->num_tiles; i++)
    {
        total_bits += ctx->fh.tile_size[i] * 8;
    }

    double ln_bpp = log(pow(cost / (double)num_pixel, OAPV_RC_BETA));
    double diff_lambda = (ctx->rc_param.beta) * (log((double)total_bits) - log(((double)ctx->param->bitrate * 1000 / ctx->param->fps)));

    diff_lambda = oapv_clip3(-0.125, 0.125, 0.25 * diff_lambda);
    ctx->rc_param.alpha = (ctx->rc_param.alpha) * exp(diff_lambda);
    ctx->rc_param.beta = (ctx->rc_param.beta) + diff_lambda / ln_bpp;
}
