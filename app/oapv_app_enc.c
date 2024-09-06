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

#include "oapv.h"
#include "oapv_app_util.h"
#include "oapv_app_args.h"
#include "oapv_app_y4m.h"

#define MAX_BS_BUF   (128 * 1024 * 1024)
#define MAX_NUM_FRMS (1)  // supports only 1-frame input
#define FRM_IDX      (0)  // supports only 1-frame input

typedef enum _STATES {
    STATE_ENCODING,
    STATE_SKIPPING,
    STATE_STOP
} STATES;

// clang-format off

/* define various command line options as a table */
static const ARGS_OPT enc_args_opts[] = {
    {
        'v',  "verbose", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "verbose (log) level\n"
        "      - 0: no message\n"
        "      - 1: only error message\n"
        "      - 2: simple messages\n"
        "      - 3: frame-level messages"
    },
    {
        'i', "input", ARGS_VAL_TYPE_STRING | ARGS_VAL_TYPE_MANDATORY, 0, NULL,
        "file name of input video"
    },
    {
        'o', "output", ARGS_VAL_TYPE_STRING, 0, NULL,
        "file name of output bitstream"
    },
    {
        'r', "recon", ARGS_VAL_TYPE_STRING, 0, NULL,
        "file name of reconstructed video"
    },
    {
        'w',  "width", ARGS_VAL_TYPE_INTEGER | ARGS_VAL_TYPE_MANDATORY, 0, NULL,
        "pixel width of input video"
    },
    {
        'h',  "height", ARGS_VAL_TYPE_INTEGER | ARGS_VAL_TYPE_MANDATORY, 0, NULL,
        "pixel height of input video"
    },
    {
        'q',  "qp", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "QP value (0~51)"
    },
    {
        'z',  "fps", ARGS_VAL_TYPE_INTEGER | ARGS_VAL_TYPE_MANDATORY, 0, NULL,
        "frame rate (frame per second)"
    },
    {
        'm',  "threads", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "force to use a specific number of threads"
    },
    {
        ARGS_NO_KEY,  "preset", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "encoder preset\n"
        "      - 0: fastest\n"
        "      - 1: fast\n"
        "      - 2: medium\n"
        "      - 3: slow\n"
        "      - 4: placebo\n"
    },
    {
        'd',  "input-depth", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "input bit depth (8, 10) "
    },
    {
        ARGS_NO_KEY,  "input-csp", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "input color space (chroma format)\n"
        "      - 0: YUV400\n"
        "      - 1: YUV420\n"
        "      - 2: YUV422\n"
        "      - 3: YUV444\n"
        "      - 4: YUV4444\n"
        "      - 5: P2(Planar Y, Combined UV, 422)\n"
    },
    {
        ARGS_NO_KEY,  "profile", ARGS_VAL_TYPE_STRING, 0, NULL,
        "profile setting flag  (422-10)"
    },
    {
        ARGS_NO_KEY,  "level-idc", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "level setting "
    },
    {
        ARGS_NO_KEY,  "frames", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "maximum number of frames to be encoded"
    },
    {
        ARGS_NO_KEY,  "seek", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "number of skipped frames before encoding"
    },
    {
        ARGS_NO_KEY,  "qp-cb-offset", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "cb qp offset"
    },
    {
        ARGS_NO_KEY,  "qp-cr-offset", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "cr qp offset"
    },
    {
        ARGS_NO_KEY,  "tile-w-mb", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "width of tile in units of MBs"
    },
    {
        ARGS_NO_KEY,  "tile-h-mb", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "height of tile in units of MBs"
    },
    {
        ARGS_NO_KEY,  "rc-type", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "Rate control type, (0: OFF, 1: ABR, 2: CRF)"
    },
    {
        ARGS_NO_KEY,  "bitrate", ARGS_VAL_TYPE_STRING, 0, NULL,
        "Bitrate in terms of kilo-bits per second: Kbps(none,K,k), Mbps(M,m)\n"
        "      ex) 100 = 100K = 0.1M"
    },
    {
        ARGS_NO_KEY,  "use-filler", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "user filler flag"
    },
    {
        ARGS_NO_KEY,  "q-matrix-y", ARGS_VAL_TYPE_STRING, 0, NULL,
        "q_matrix_y \"q1 q2 ... q63 q64\""
    },
    {
        ARGS_NO_KEY,  "q-matrix-u", ARGS_VAL_TYPE_STRING, 0, NULL,
        "q_matrix_u \"q1 q2 ... q63 q64\""
    },
    {
        ARGS_NO_KEY,  "q-matrix-v", ARGS_VAL_TYPE_STRING, 0, NULL,
        "q_matrix_v \"q1 q2 ... q63 q64\""
    },
    {
        ARGS_NO_KEY,  "udm", ARGS_VAL_TYPE_STRING, 0, NULL,
        "input file of user defined metadata\""
    },
    {
        ARGS_NO_KEY,  "hash", ARGS_VAL_TYPE_NONE, 0, NULL,
        "embed frame hash value for conformance checking in decoding"
    },
    {ARGS_END_KEY, "", ARGS_VAL_TYPE_NONE, 0, NULL, ""} /* termination */
};

// clang-format on

#define NUM_ARGS_OPT ((int)(sizeof(enc_args_opts) / sizeof(enc_args_opts[0])))

typedef struct {
    /* variables for options */
    char           fname_inp[256];
    char           fname_out[256];
    char           fname_rec[256];
    int            frames;
    int            hash;
    int            input_depth;
    int            input_csp;
    int            seek;
    int            threads;
    char           profile[32];
    char           bitrate[64];
    char           q_matrix_y[512];
    char           q_matrix_u[512];
    char           q_matrix_v[512];
    char           user_defined_metadata_file[256];
    oapve_param_t* param;
} ARGS_VAR;

static ARGS_VAR* args_init_vars(ARGS_PARSER* args, oapve_param_t* param)
{
    ARGS_OPT* opts;
    ARGS_VAR* vars;

    opts = args->opts;
    opts = args->opts;
    vars = malloc(sizeof(ARGS_VAR));
    assert_rv(vars != NULL, NULL);
    memset(vars, 0, sizeof(ARGS_VAR));

    vars->param = param;

    /*args_set_variable_by_key_long(opts, "config", args->fname_cfg);*/
    args_set_variable_by_key_long(opts, "input", vars->fname_inp);
    args_set_variable_by_key_long(opts, "output", vars->fname_out);
    args_set_variable_by_key_long(opts, "recon", vars->fname_rec);
    args_set_variable_by_key_long(opts, "frames", &vars->frames);
    args_set_variable_by_key_long(opts, "hash", &vars->hash);
    args_set_variable_by_key_long(opts, "verbose", &op_verbose);
    op_verbose = VERBOSE_SIMPLE; /* default */
    args_set_variable_by_key_long(opts, "input-depth", &vars->input_depth);
    vars->input_depth = 10; /* default */
    args_set_variable_by_key_long(opts, "input-csp", &vars->input_csp);
    vars->input_csp = 2; /* default: YUV422 */
    args_set_variable_by_key_long(opts, "seek", &vars->seek);
    args_set_variable_by_key_long(opts, "profile", vars->profile);
    strncpy(vars->profile, "422-10", sizeof(vars->profile) - 1);
    args_set_variable_by_key_long(opts, "bitrate", vars->bitrate);
    args_set_variable_by_key_long(opts, "q-matrix-y", vars->q_matrix_y);
    strncpy(vars->q_matrix_y, "", sizeof(vars->q_matrix_y) - 1);
    args_set_variable_by_key_long(opts, "q-matrix-u", vars->q_matrix_u);
    strncpy(vars->q_matrix_u, "", sizeof(vars->q_matrix_y) - 1);
    args_set_variable_by_key_long(opts, "q-matrix-v", vars->q_matrix_v);
    strncpy(vars->q_matrix_v, "", sizeof(vars->q_matrix_y) - 1);
    args_set_variable_by_key_long(opts, "udm", vars->user_defined_metadata_file);
    strncpy(vars->user_defined_metadata_file, "", sizeof(vars->user_defined_metadata_file) - 1);
    args_set_variable_by_key_long(opts, "threads", &vars->threads);
    vars->threads = 1; /* default */

    ARGS_SET_PARAM_VAR_KEY(opts, param, w);
    ARGS_SET_PARAM_VAR_KEY(opts, param, h);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, qp);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, fps);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, preset);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, level_idc);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, rc_type);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, use_filler);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, tile_w_mb);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, tile_h_mb);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, qp_cb_offset);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, qp_cr_offset);

    return vars;
}

static void print_usage(const char** argv)
{
    int           i;
    char          str[1024];
    ARGS_PARSER*  args;
    ARGS_VAR*     args_var = NULL;
    oapve_param_t default_param;

    oapve_param_default(&default_param);
    args = args_create(enc_args_opts, NUM_ARGS_OPT);
    if(args == NULL)
        goto ERR;
    args_var = args_init_vars(args, &default_param);
    if(args_var == NULL)
        goto ERR;

    logv2("Syntax: \n");
    logv2("  %s -i 'input-file' [ options ] \n\n", "oapv_app_enc");

    logv2("Options:\n");
    logv2("  --help\n    : list options\n");
    for(i = 0; i < args->num_option; i++) {
        if(args->get_help(args, i, str) < 0)
            return;
        logv2("%s\n", str);
    }

ERR:
    if(args)
        args->release(args);
    if(args_var)
        free(args_var);
}

static int check_conf(oapve_cdesc_t* cdesc, ARGS_VAR* vars)
{
    int            i, ret = 0;
    oapve_param_t* param;

    param = cdesc->param;

    for(i = 0; i < cdesc->max_num_frms; i++) {
        if(vars->hash && param[i].is_rec == 0) {
            logerr("cannot use frame hash without reconstructed picture option!\n");
            ret = -1;
        }
    }
    return ret;
}

static int set_extra_config(oapve_t id, ARGS_VAR* vars, oapve_param_t* param)
{
    int ret = OAPV_OK, size, value;

    if(vars->hash) {
        value = 1;
        size  = 4;
        ret   = oapve_config(id, OAPV_CFG_SET_USE_FRM_HASH, &value, &size);
        if(OAPV_FAILED(ret)) {
            logerr("failed to set config for using frame hash\n");
            return -1;
        }
    }
    return ret;
}

static void print_config(ARGS_VAR* vars, oapve_param_t* param)
{
    if(op_verbose < VERBOSE_FRAME)
        return;

    logv3_line("Configurations");
    logv2("Input image   : %s \n", vars->fname_inp);
    if(strlen(vars->fname_out) > 0) {
        logv2("Output stream : %s \n", vars->fname_out);
    }
    if(strlen(vars->fname_rec) > 0) {
        logv2("Output image  : %s \n", vars->fname_rec);
    }
    logv2("\tprofile                  = %s\n", vars->profile);
    logv2("\twidth                    = %d\n", param->w);
    logv2("\theight                   = %d\n", param->h);
    logv2("\tFPS                      = %d\n", param->fps);
    logv2("\tQP                       = %d\n", param->qp);
    logv2("\tframes                   = %d\n", vars->frames);
    logv2("\trate-control type        = %s\n", (param->rc_type == OAPV_RC_ABR) ? "ABR" : "CQP");
    if(param->rc_type == OAPV_RC_ABR) {
        logv2("\tBit_Rate                 = %dkbps\n", param->bitrate);
    }
    logv2("\tnumber of tile cols      = %d\n", param->tile_cols);
    logv2("\tnumber of tile rows      = %d\n", param->tile_rows);
}

void print_stat_frame(int fnum, int gid, double* psnr, int bitrate, oapv_clk_t clk_end, int is_first)
{
    if(op_verbose < VERBOSE_FRAME)
        return;

    if(is_first) {
        logv3_line("Stat");
        logv3("Num   Gid   PSNR-Y    PSNR-U    PSNR-V    Bits      EncT(ms)\n");
        logv3_line("");
    }

    logv3("%-7d%-5d%-10.4f%-10.4f%-10.4f%-10d%-10d\n",
          fnum, gid, psnr[0], psnr[1], psnr[2],
          bitrate, oapv_clk_msec(clk_end));

    fflush(stdout);
    fflush(stderr);
}

static int kbps_str_to_int(char* str)
{
    int kbps = 0;
    if(strchr(str, 'K') || strchr(str, 'k')) {
        char* tmp = strtok(str, "Kk ");
        kbps      = (int)(atof(tmp));
    }
    else if(strchr(str, 'M') || strchr(str, 'm')) {
        char* tmp = strtok(str, "Mm ");
        kbps      = (int)(atof(tmp) * 1000);
    }
    else {
        kbps = atoi(str);
    }
    return kbps;
}

static int update_param(ARGS_VAR* vars, oapve_param_t* param)
{
    /* update reate controller  parameters */
    if(strlen(vars->bitrate) > 0) {
        param->bitrate = kbps_str_to_int(vars->bitrate);
    }

    if(vars->fname_rec[0] == '\0') {
        op_verbose    = op_verbose > 2 ? 2 : op_verbose;
        param->is_rec = 0;
    }
    else {
        param->is_rec = 1;
    }

    /* update q_matrix */
    int len_y = (int)strlen(vars->q_matrix_y);
    if(len_y > 0) {
        param->use_q_matrix = 1;
        char* tmp           = vars->q_matrix_y;
        int   cnt           = 0;
        int   len_cnt       = 0;
        while(len_cnt < len_y && cnt < OAPV_BLOCK_D) {
            sscanf(tmp, "%d", &param->q_matrix_y[cnt]);
            if(param->q_matrix_y[cnt] < 1 || param->q_matrix_y[cnt] > 256) {
                logerr("input value of q_matrix_y is valid\n");
                return OAPV_ERR;
            }
            len_cnt += (int)log10(param->q_matrix_y[cnt]) + 2;
            tmp = vars->q_matrix_y + len_cnt;
            cnt++;
        }
        if(cnt < OAPV_BLOCK_D) {
            logerr("input number of q_matrix_y is not enough\n");
            return OAPV_ERR;
        }
    }

    int len_u = (int)strlen(vars->q_matrix_u);
    if(len_u > 0) {
        param->use_q_matrix = 1;
        char* tmp           = vars->q_matrix_u;
        int   cnt           = 0;
        int   len_cnt       = 0;
        while(len_cnt < len_u && cnt < OAPV_BLOCK_D) {
            sscanf(tmp, "%d", &param->q_matrix_u[cnt]);
            if(param->q_matrix_u[cnt] < 1 || param->q_matrix_u[cnt] > 256) {
                logerr("input value of q_matrix_u is valid\n");
                return OAPV_ERR;
            }
            len_cnt += (int)log10(param->q_matrix_u[cnt]) + 2;
            tmp = vars->q_matrix_u + len_cnt;
            cnt++;
        }
        if(cnt < OAPV_BLOCK_D) {
            logerr("input number of q_matrix_u is not enough\n");
            return OAPV_ERR;
        }
    }

    int len_v = (int)strlen(vars->q_matrix_v);
    if(len_v > 0) {
        param->use_q_matrix = 1;
        char* tmp           = vars->q_matrix_v;
        int   cnt           = 0;
        int   len_cnt       = 0;
        while(len_cnt < len_v && cnt < OAPV_BLOCK_D) {
            sscanf(tmp, "%d", &param->q_matrix_v[cnt]);
            if(param->q_matrix_v[cnt] < 1 || param->q_matrix_v[cnt] > 256) {
                logerr("input value of q_matrix_v is valid\n");
                return OAPV_ERR;
            }
            len_cnt += (int)log10(param->q_matrix_v[cnt]) + 2;
            tmp = vars->q_matrix_v + len_cnt;
            cnt++;
        }
        if(cnt < OAPV_BLOCK_D) {
            logerr("input number of q_matrix_v is not enough\n");
            return OAPV_ERR;
        }
    }

    if(param->use_q_matrix) {
        if(len_y == 0) {
            for(int i = 0; i < OAPV_BLOCK_D; i++) {
                param->q_matrix_y[i] = 16;
            }
        }

        if(len_u == 0) {
            for(int i = 0; i < OAPV_BLOCK_D; i++) {
                param->q_matrix_u[i] = 16;
            }
        }

        if(len_v == 0) {
            for(int i = 0; i < OAPV_BLOCK_D; i++) {
                param->q_matrix_v[i] = 16;
            }
        }
    }

    param->csp = vars->input_csp;

    return 0;
}

int main(int argc, const char** argv)
{
    ARGS_PARSER*   args     = NULL;
    ARGS_VAR*      args_var = NULL;
    STATES         state    = STATE_ENCODING;
    unsigned char* bs_buf   = NULL;
    FILE*          fp_inp   = NULL;
    oapve_t        id       = NULL;
    oapvm_t        mid      = NULL;
    oapve_cdesc_t  cdesc;
    oapve_param_t* param = NULL;
    oapv_bitb_t    bitb;
    oapve_stat_t   stat;
    oapv_imgb_t*   imgb_r = NULL;  // image buffer for read
    oapv_imgb_t*   imgb_w = NULL;  // image buffer for write
    oapv_imgb_t*   imgb_i = NULL;  // image buffer for input
    oapv_imgb_t*   imgb_o = NULL;  // image buffer for output
    oapv_frms_t    ifrms;          // frames for input
    oapv_frms_t    rfrms;          // frames for reconstruction
    int            ret;
    oapv_clk_t     clk_beg, clk_end, clk_tot;
    oapv_mtime_t   pic_cnt, pic_skip;
    double         bitrate;
    double         psnr[4] = {
        0,
    };
    double psnr_avg[4] = {
        0,
    };
    int       encod_frames = 0;
    int       is_y4m       = 0;
    Y4M_INFO  y4m;
    char      fname_inp[256], fname_out[256], fname_rec[256];
    int       is_out = 0, is_rec = 0;
    int       max_frames    = 0;
    int       skip_frames   = 0;
    int       is_max_frames = 0, is_skip_frames = 0;
    char*     errstr = NULL;
    int       cfmt;  // color format
    int       width, height;
    const int num_frames = MAX_NUM_FRMS;

    /* help message */
    if(argc < 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
        print_usage(argv);
        return 0;
    }

    memset(ifrms.frm, 0, sizeof(oapv_frm_t*) * OAPV_MAX_NUM_FRAMES);
    memset(rfrms.frm, 0, sizeof(oapv_frm_t*) * OAPV_MAX_NUM_FRAMES);

    /* set default parameters */
    memset(&cdesc, 0, sizeof(oapve_cdesc_t));
    param = &cdesc.param[FRM_IDX];
    ret   = oapve_param_default(param);
    if(OAPV_FAILED(ret)) {
        logerr("cannot set default parameter\n");
        ret = -1;
        goto ERR;
    }
    /* parse command line */
    args = args_create(enc_args_opts, NUM_ARGS_OPT);
    if(args == NULL) {
        logerr("cannot create argument parser\n");
        ret = -1;
        goto ERR;
    }
    args_var = args_init_vars(args, param);
    if(args_var == NULL) {
        logerr("cannot initialize argument parser\n");
        ret = -1;
        goto ERR;
    }
    if(args->parse(args, argc, argv, &errstr)) {
        logerr("command parsing error (%s)\n", errstr);
        ret = -1;
        goto ERR;
    }

    logv2("  ____                ___   ___ _   __\n");
    logv2(" / __ \\___  ___ ___  / _ | / _ \\ | / / Encoder\n");
    logv2("/ /_/ / _ \\/ -_) _ \\/ __ |/ ___/ |/ / \n");
    logv2("\\____/ .__/\\__/_//_/_/ |_/_/   |___/  \n");
    logv2("    /_/                               \n");
    logv2("\n");

    /* try to open input file */
    if(args->get_str(args, "input", fname_inp, NULL)) {
        logerr("input file should be set\n");
        ret = -1;
        goto ERR;
    }
    fp_inp = fopen(fname_inp, "rb");
    if(fp_inp == NULL) {
        logerr("cannot open input file (%s)\n", fname_inp);
        ret = -1;
        goto ERR;
    }

    /* y4m header parsing  */
    is_y4m = y4m_test(fp_inp);
    if(is_y4m) {
        if(y4m_header_parser(fp_inp, &y4m)) {
            logerr("This y4m is not supported (%s)\n", fname_inp);
            ret = -1;
            goto ERR;
        }
        y4m_update_param(args, &y4m, param);
        cfmt = y4m.color_format;
    }
    else {
        int csp;
        if(args->get_int(args, "input-csp", &csp, NULL)) {
            logerr("cannot get input-csp value");
            ret = -1;
            goto ERR;
        }
        // clang-format off
        cfmt = (csp == 0 ? OAPV_CF_YCBCR400 : \
            (csp == 1 ? OAPV_CF_YCBCR420 : \
            (csp == 2 ? OAPV_CF_YCBCR422 : \
            (csp == 3 ? OAPV_CF_YCBCR444  : \
            (csp == 4 ? OAPV_CF_YCBCR4444 : \
            (csp == 5 ? OAPV_CF_PLANAR2   : OAPV_CF_UNKNOWN))))));
        // clang-format on

        if(cfmt == OAPV_CF_UNKNOWN) {
            logerr("Unknow color format\n");
            ret = -1;
            goto ERR;
        }
    }

    /* update parameters */
    if(update_param(args_var, param)) {
        logerr("parameters is not proper\n");
        ret = -1;
        goto ERR;
    }
    /* check mandatory parameters */
    if(args->check_mandatory(args, &errstr)) {
        logerr("[%s] argument should be set\n", errstr);
        ret = -1;
        goto ERR;
    }

    cdesc.max_bs_buf_size = MAX_BS_BUF; /* maximum bitstream buffer size */
    cdesc.max_num_frms    = MAX_NUM_FRMS;
    cdesc.threads         = args_var->threads;

    if(check_conf(&cdesc, args_var)) {
        logerr("invalid configuration\n");
        ret = -1;
        goto ERR;
    }

    if(args->get_str(args, "output", fname_out, &is_out)) {
        logerr("cannot get 'output' option\n");
        ret = -1;
        goto ERR;
    }
    if(is_out) {
        clear_data(fname_out);
    }
    if(args->get_str(args, "recon", fname_rec, &is_rec)) {
        logerr("cannot get 'recon' option\n");
        ret = -1;
        goto ERR;
    }
    if(is_rec) {
        clear_data(fname_rec);
    }
    if(args->get_int(args, "frames", &max_frames, &is_max_frames)) {
        logerr("cannot get 'frames' option\n");
        ret = -1;
        goto ERR;
    }
    if(args->get_int(args, "seek", &skip_frames, &is_skip_frames)) {
        logerr("cannot get 'seek' option\n");
        ret = -1;
        goto ERR;
    }

    /* allocate bitstream buffer */
    bs_buf = (unsigned char*)malloc(MAX_BS_BUF);
    if(bs_buf == NULL) {
        logerr("cannot allocate bitstream buffer, size=%d", MAX_BS_BUF);
        ret = -1;
        goto ERR;
    }

    /* create encoder */
    id = oapve_create(&cdesc, NULL);
    if(id == NULL) {
        logerr("cannot create OAPV encoder\n");
        ret = -1;
        goto ERR;
    }

    /* create metadata handler */
    mid = oapvm_create(&ret);
    if(mid == NULL || OAPV_FAILED(ret)) {
        logerr("cannot create OAPV metadata handler\n");
        ret = -1;
        goto ERR;
    }

    if(set_extra_config(id, args_var, param)) {
        logerr("cannot set extra configurations\n");
        ret = -1;
        goto ERR;
    }

    width  = param->w;
    height = param->h;

    print_config(args_var, param);

    bitrate    = 0;
    bitb.addr  = bs_buf;
    bitb.bsize = MAX_BS_BUF;

    if(is_skip_frames && skip_frames > 0) {
        state = STATE_SKIPPING;
    }

    clk_tot  = 0;
    pic_cnt  = 0;
    pic_skip = 0;

    // create image buffers
    for(int i = 0; i < num_frames; i++) {
        if(args_var->input_depth == 10) {
            ifrms.frm[i].imgb = imgb_create(width, height, OAPV_CS_SET(cfmt, args_var->input_depth, 0));
        }
        else {
            imgb_r            = imgb_create(width, height, OAPV_CS_SET(cfmt, args_var->input_depth, 0));
            ifrms.frm[i].imgb = imgb_create(width, height, OAPV_CS_SET(cfmt, 10, 0));
        }

        if(is_rec) {
            if(args_var->input_depth == 10) {
                rfrms.frm[i].imgb = imgb_create(width, height, OAPV_CS_SET(cfmt, args_var->input_depth, 0));
            }
            else {
                imgb_w            = imgb_create(width, height, OAPV_CS_SET(cfmt, args_var->input_depth, 0));
                rfrms.frm[i].imgb = imgb_create(width, height, OAPV_CS_SET(cfmt, 10, 0));
            }
        }
    }

    ifrms.num_frms = num_frames;

    /* encode pictures *******************************************************/
    while((is_max_frames && (pic_cnt < max_frames)) || !is_max_frames) {
        for(int i = 0; i < num_frames; i++) {
            if(args_var->input_depth == 10) {
                imgb_i = ifrms.frm[i].imgb;
            }
            else {
                imgb_i = imgb_r;
            }
            ret = imgb_read(fp_inp, imgb_i, param->w, param->h, is_y4m);
            if(ret < 0) {
                logv3("reached out the end of input file\n");
                ret   = OAPV_OK;
                state = STATE_STOP;
                break;
            }
            if(args_var->input_depth != 10) {
                imgb_cpy(ifrms.frm[i].imgb, imgb_i);
            }
            ifrms.frm[i].group_id = 1;  // FIX-ME : need to set properly in case of multi-frame
            ifrms.frm[i].pbu_type = OAPV_PBU_TYPE_PRIMARY_FRAME;
        }

        /* encoding */
        clk_beg = oapv_clk_get();

        if(state == STATE_ENCODING) {
            ret = oapve_encode(id, &ifrms, mid, &bitb, &stat, &rfrms);

            for(int i = 0; i < num_frames; i++) {
                if(args_var->input_depth != 10) {
                    imgb_cpy(imgb_w, rfrms.frm[i].imgb);
                    imgb_o = imgb_w;
                }
                else {
                    imgb_o = rfrms.frm[i].imgb;
                }

                clk_end = oapv_clk_from(clk_beg);
                clk_tot += clk_end;

                /* store bitstream */
                if(OAPV_SUCCEEDED(ret)) {
                    if(is_out && stat.write > 0) {
                        if(write_data(fname_out, bs_buf, stat.write)) {
                            logerr("cannot write bitstream\n");
                            ret = -1;
                            goto ERR;
                        }
                    }
                }
                else {
                    logerr("failed to encode\n");
                    ret = -1;
                    goto ERR;
                }

                // store recon image
                if(is_rec) {
                    if(imgb_write(args_var->fname_rec, imgb_o)) {
                        logerr("cannot write reconstruction image\n");
                        ret = -1;
                        goto ERR;
                    }
                }

                bitrate += stat.frm_size[FRM_IDX];

                /* calculate PSNR */
                if(op_verbose == VERBOSE_FRAME) {
                    measure_psnr(ifrms.frm[i].imgb, rfrms.frm[i].imgb, psnr, args_var->input_depth);
                    print_stat_frame(pic_cnt, stat.aui.frm_info[FRM_IDX].group_id, psnr, (stat.frm_size[FRM_IDX]) << 3, clk_end, (pic_cnt == 0));
                    for(i = 0; i < (cfmt == OAPV_CF_YCBCR4444 ? 4 : 3); i++)
                        psnr_avg[i] += psnr[i];
                }
                else {
                    int total_time      = ((int)oapv_clk_msec(clk_tot) / 1000);
                    int h               = total_time / 3600;
                    total_time          = total_time % 3600;
                    int m               = total_time / 60;
                    total_time          = total_time % 60;
                    int    s            = total_time;
                    double curr_bitrate = bitrate;
                    curr_bitrate *= (param->fps * 8);
                    curr_bitrate /= (encod_frames + 1);
                    curr_bitrate /= 1000;
                    logv2("[ %d / %d frames ] [ %.2f frame/sec ] [ %.4f kbps ] [ %2dh %2dm %2ds ] \r", encod_frames, max_frames, ((float)(encod_frames + 1) * 1000) / ((float)oapv_clk_msec(clk_tot)), curr_bitrate, h, m, s);
                    fflush(stdout);
                    encod_frames++;
                }
                pic_cnt++;
            }
        }
        else if(state == STATE_SKIPPING) {
            if(pic_skip < skip_frames) {
                pic_skip++;
                continue;
            }
            else {
                state = STATE_ENCODING;
            }
        }
        else if(state == STATE_STOP) {
            break;
        }
        oapvm_rem_all(mid);
    }

    logv2_line("Summary");
    psnr_avg[0] /= pic_cnt;
    psnr_avg[1] /= pic_cnt;
    psnr_avg[2] /= pic_cnt;
    if(cfmt == OAPV_CF_YCBCR4444) {
        psnr_avg[3] /= pic_cnt;
    }

    logv3("  PSNR Y(dB)       : %-5.4f\n", psnr_avg[0]);
    logv3("  PSNR U(dB)       : %-5.4f\n", psnr_avg[1]);
    logv3("  PSNR V(dB)       : %-5.4f\n", psnr_avg[2]);
    if(cfmt == OAPV_CF_YCBCR4444) {
        logv3("  PSNR T(dB)       : %-5.4f\n", psnr_avg[3]);
    }
    logv3("  Total bits(bits) : %.0f\n", bitrate * 8);
    bitrate *= (param->fps * 8);
    bitrate /= pic_cnt;
    bitrate /= 1000;

    logv3("  Labeles          : br,kbps\tPSNR,Y\tPSNR,U\tPSNR,V\t\n");
    if(cfmt == OAPV_CF_YCBCR4444) {
        logv3("  Summary          : %-5.4f\t%-5.4f\t%-5.4f\t%-5.4f\t%-5.4f\n",
              bitrate, psnr_avg[0], psnr_avg[1], psnr_avg[2], psnr_avg[3]);
    }
    else {
        logv3("  Summary          : %-5.4f\t%-5.4f\t%-5.4f\t%-5.4f\n",
              bitrate, psnr_avg[0], psnr_avg[1], psnr_avg[2]);
    }

    logv2("Bitrate                           = %.4f kbps\n", bitrate);
    logv2("Encoded frame count               = %d\n", (int)pic_cnt);
    logv2("Total encoding time               = %.3f msec,",
          (float)oapv_clk_msec(clk_tot));
    logv2(" %.3f sec\n", (float)(oapv_clk_msec(clk_tot) / 1000.0));

    logv2("Average encoding time for a frame = %.3f msec\n",
          (float)oapv_clk_msec(clk_tot) / pic_cnt);
    logv2("Average encoding speed            = %.3f frames/sec\n",
          ((float)pic_cnt * 1000) / ((float)oapv_clk_msec(clk_tot)));
    logv2_line(NULL);

    if(is_max_frames && pic_cnt != max_frames) {
        logv3("Wrong frames count: should be %d was %d\n",
              max_frames, (int)pic_cnt);
    }
ERR:

    if(imgb_r != NULL)
        imgb_r->release(imgb_r);
    if(imgb_w != NULL)
        imgb_w->release(imgb_w);

    for(int i = 0; i < num_frames; i++) {
        if(ifrms.frm[i].imgb != NULL) {
            ifrms.frm[i].imgb->release(ifrms.frm[i].imgb);
        }
    }
    for(int i = 0; i < num_frames; i++) {
        if(rfrms.frm[i].imgb != NULL) {
            rfrms.frm[i].imgb->release(rfrms.frm[i].imgb);
        }
    }

    if(id)
        oapve_delete(id);
    if(mid)
        oapvm_delete(mid);
    if(fp_inp)
        fclose(fp_inp);
    if(bs_buf)
        free(bs_buf); /* release bitstream buffer */
    if(args)
        args->release(args);
    if(args_var)
        free(args_var);

    return ret;
}
