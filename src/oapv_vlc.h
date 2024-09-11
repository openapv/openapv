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

#ifndef _OAPV_VLC_H_
#define _OAPV_VLC_H_

#include "oapv_def.h"
#include "oapv_metadata.h"


void oapve_set_frame_header(oapve_ctx_t * ctx, oapv_fh_t * fh);
int  oapve_vlc_frame_info(oapv_bs_t* bs, oapv_fi_t* fi);
int  oapve_vlc_frame_header(oapv_bs_t* bs, oapve_ctx_t* ctx, oapv_fh_t* fh);
int  oapve_vlc_tile_size(oapv_bs_t* bs, int tile_size);
void oapve_set_tile_header(oapve_ctx_t* ctx, oapv_th_t* th, int tile_idx, int qp);
int  oapve_vlc_tile_header(oapve_ctx_t* ctx, oapv_bs_t* bs, oapv_th_t* th);
void oapve_vlc_run_length_cc(oapve_ctx_t* ctx, oapve_core_t* core, oapv_bs_t* bs, s16* coef, int log2_w, int log2_h, int num_sig, int ch_type);
int  oapve_vlc_metadata(oapv_md_t* md, oapv_bs_t* bs);
int  oapve_vlc_au_info(oapv_bs_t* bs, oapve_ctx_t* ctx, oapv_frms_t* frms, oapv_bs_t** bs_fi_pos);
int  oapve_vlc_pbu_header(oapv_bs_t* bs, int pbu_type, int group_id);
int  oapve_vlc_pbu_size(oapv_bs_t* bs, int pbu_size);
void oapve_vlc_ac_coeff(oapve_ctx_t* ctx, oapve_core_t* core, oapv_bs_t* bs, s16* coef, int log2_w, int log2_h, int num_sig, int ch_type);
int  oapve_vlc_dc_coeff(oapve_ctx_t* ctx, oapve_core_t* core, oapv_bs_t* bs, int log2_w, int dc_diff, int c);

int  oapvd_vlc_au_size(oapv_bs_t* bs, u32* size);
int  oapvd_vlc_pbu_size(oapv_bs_t* bs, u32* size);
int  oapvd_vlc_pbu_header(oapv_bs_t* bs, oapv_pbuh_t* pbu);
int  oapvd_vlc_au_info(oapv_bs_t* bs, oapv_aui_t* aui);

int  oapvd_vlc_frame_size(oapv_bs_t* bs);
int  oapvd_vlc_frame_header(oapv_bs_t* bs, oapv_fh_t* fh);
int  oapvd_vlc_frame_info(oapv_bs_t* bs, oapv_fi_t *fi);
int  oapvd_vlc_tiles(oapvd_ctx_t* ctx, oapv_bs_t* bs);
int  oapvd_store_tile_data(oapvd_ctx_t* ctx, oapv_bs_t* bs);
int  oapvd_vlc_tile_data(oapvd_ctx_t* ctx, oapvd_core_t* core, oapv_bs_t* bs, int x, int y, int tile_idx, int c);
int  oapvd_vlc_tile_dummy_data(oapv_bs_t* bs);
int  oapvd_vlc_run_length_cc(oapvd_ctx_t* ctx, oapvd_core_t* core, oapv_bs_t* bs, s16* coef, int log2_w, int log2_h, int ch_type);
int  oapvd_vlc_metadata(oapv_bs_t* bs, u32 pbu_size, oapvm_t mid, int group_id);
int  oapvd_vlc_filler(oapv_bs_t* bs, u32 filler_size);
int  oapvd_vlc_dc_coeff(oapvd_ctx_t* ctx, oapvd_core_t* core, oapv_bs_t* bs, int log2_w, s16* dc_diff, int c);
int  oapvd_vlc_ac_coeff(oapvd_ctx_t* ctx, oapvd_core_t* core, oapv_bs_t* bs, s16* coef, int log2_w, int log2_h, int ch_type);
#endif /* _OAPV_VLC_H_ */
