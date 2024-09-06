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

#ifndef _OAPV_SAD_H_
#define _OAPV_SAD_H_

#include "oapv_port.h"

int  oapv_sad_16b(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth);
void oapv_diff_16b(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth);
s64  oapv_ssd_16b(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth);
int oapv_dc_removed_had8x8(pel* org, int s_org);

typedef int  (*OAPV_FN_SAD)  (int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth);
typedef s64  (*OAPV_FN_SSD)  (int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth);
typedef void (*OAPV_FN_DIFF) (int w, int h, void *src1, void *src2, int s_src1, int s_src2, int s_diff, s16 *diff, int bit_depth);

extern const OAPV_FN_SAD  oapv_tbl_sad_16b[1];
extern const OAPV_FN_SSD  oapv_tbl_ssd_16b[1];
extern const OAPV_FN_DIFF oapv_tbl_diff_16b[1];

extern const OAPV_FN_SAD  (* oapv_func_sad);
extern const OAPV_FN_SSD  (* oapv_func_ssd);
extern const OAPV_FN_DIFF (* oapv_func_diff);


#define oapv_sad(log2w, log2h, src1, src2, s_src1, s_src2, bit_depth)\
        oapv_func_sad[0](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, bit_depth)
#define oapv_ssd(log2w, log2h, src1, src2, s_src1, s_src2, bit_depth)\
        oapv_func_ssd[0](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, bit_depth)
#define oapv_diff(log2w, log2h, src1, src2, s_src1, s_src2, s_diff, diff, bit_depth) \
        oapv_func_diff[0](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, s_diff, diff, bit_depth)

#endif /* _OAPV_SAD_H_ */