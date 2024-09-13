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

#define MAX_BS_BUF          128 * 1024 * 1024 /* byte */

#define MAX_NUM_FRMS        (1)  // supports only 1-frame output
#define FRM_IDX             (0)  // supports only 1-frame output
// check generic frame or not
#define IS_NON_AUX_FRM(frm) (((frm)->pbu_type == OAPV_PBU_TYPE_PRIMARY_FRAME) || ((frm)->pbu_type == OAPV_PBU_TYPE_NON_PRIMARY_FRAME))
// check auxiliary frame or not
#define IS_AUX_FRM(frm)     (!(IS_NON_AUX_FRM(frm)))

#define OUTPUT_CSP_NATIVE   (0)
#define OUTPUT_CSP_P210     (1)

// clang-format off

/* define various command line options as a table */
static const ARGS_OPT dec_args_opts[] = {
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
        "file name of input bitstream"
    },
    {
        'o', "output", ARGS_VAL_TYPE_STRING, 0, NULL,
        "file name of decoded output"
    },
    {
        ARGS_NO_KEY,  "frames", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "maximum number of frames to be decoded"
    },
    {
        'm',  "threads", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "force to use a specific number of threads"
    },
    {
        'd',  "output-depth", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "output bit depth (8, 10) "
    },
    {
        ARGS_NO_KEY,  "hash", ARGS_VAL_TYPE_NONE, 0, NULL,
        "parse frame hash value for conformance checking in decoding"
    },
    {
        ARGS_NO_KEY,  "output-csp", ARGS_VAL_TYPE_INTEGER, 0, NULL,
        "output color space (chroma format)\n"
        "      - 0: coded CSP\n"
        "      - 1: convert to P210 in case of YCbCr422\n"
    },
    {ARGS_END_KEY, "", ARGS_VAL_TYPE_NONE, 0, NULL, ""} /* termination */
};

// clang-format on

#define NUM_ARGS_OPT        ((int)(sizeof(dec_args_opts) / sizeof(dec_args_opts[0])))

typedef struct {
    char fname_inp[256];
    char fname_out[256];
    int  frames;
    int  hash;
    int  threads;
    int  output_depth;
    int  output_csp;
} ARGS_VAR;

static ARGS_VAR* args_init_vars(ARGS_PARSER* args)
{
    ARGS_OPT* opts;
    ARGS_VAR* vars;
    opts = args->opts;
    vars = malloc(sizeof(ARGS_VAR));
    assert_rv(vars != NULL, NULL);
    memset(vars, 0, sizeof(ARGS_VAR));

    /*args_set_variable_by_key_long(opts, "config", args->fname_cfg);*/
    args_set_variable_by_key_long(opts, "input", vars->fname_inp);
    args_set_variable_by_key_long(opts, "output", vars->fname_out);
    args_set_variable_by_key_long(opts, "frames", &vars->frames);
    args_set_variable_by_key_long(opts, "hash", &vars->hash);
    args_set_variable_by_key_long(opts, "verbose", &op_verbose);
    op_verbose = VERBOSE_SIMPLE; /* default */
    args_set_variable_by_key_long(opts, "threads", &vars->threads);
    vars->threads = 1; /* default */
    args_set_variable_by_key_long(opts, "output-depth", &vars->output_depth);
    args_set_variable_by_key_long(opts, "output-csp", &vars->output_csp);
    vars->output_csp = 0; /* default: coded CSP */

    return vars;
}

static void print_usage(const char** argv)
{
    int          i;
    char         str[1024];
    ARGS_VAR*    args_var = NULL;
    ARGS_PARSER* args;

    args = args_create(dec_args_opts, NUM_ARGS_OPT);
    if(args == NULL)
        goto ERR;
    args_var = args_init_vars(args);
    if(args_var == NULL)
        goto ERR;

    logv2("Syntax: \n");
    logv2("  %s -i 'input-file' [ options ] \n\n", argv[0]);

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

static int read_au_size(FILE* fp)
{
    unsigned char buf[4];

    for(int i = 0; i < 4; i++) {
        if(1 != fread(&buf[i], 1, 1, fp))
            return -1;
    }
    return ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3]));
}

static int read_bitstream(FILE* fp, unsigned char* bs_buf, int* bs_buf_size)
{
    int           au_size, read_size = 0;
    unsigned char b = 0;
    if(!fseek(fp, 0, SEEK_CUR)) {
        /* read size first */
        au_size = read_au_size(fp);
        if(au_size > 0) {
            while(au_size > 0) {
                /* read byte */
                if(1 != fread(&b, 1, 1, fp)) {
                    logerr("Cannot read bitstream!\n");
                    return -1;
                }
                bs_buf[read_size] = b;
                read_size++;
                au_size--;
            }
            *bs_buf_size = read_size;
        }
        else {
            if(feof(fp)) {
                logv2_line("");
                logv2("End of file\n");
            }
            else {
                logerr("Cannot read bitstream size!\n")
            };

            return -1;
        }
    }
    else {
        logerr("Cannot seek bitstream!\n");
        return -1;
    }
    return read_size + 4;
}

static int set_extra_config(oapvd_t id, ARGS_VAR* args_vars)
{
    int ret, size, value;

    if(args_vars->hash) {  // enable frame hash calculation
        value = 1;
        size  = 4;
        ret   = oapvd_config(id, OAPV_CFG_SET_USE_FRM_HASH, &value, &size);
        if(OAPV_FAILED(ret)) {
            logerr("failed to set config for using frame hash\n");
            return -1;
        }
    }
    return 0;
}

static int write_dec_img(char* fname, oapv_imgb_t* img, int flag_y4m)
{
    if(flag_y4m) {
        if(write_y4m_frame_header(fname))
            return -1;
    }
    if(imgb_write(fname, img))
        return -1;
    return 0;
}

static int check_frm_hash(oapvm_t mid, oapv_imgb_t* imgb, int group_id)
{
    unsigned char uuid_frm_hash[16] = {0xf8, 0x72, 0x1b, 0x3e, 0xcd, 0xee, 0x47, 0x21, 0x98, 0x0d, 0x9b, 0x9e, 0x39, 0x20, 0x28, 0x49};
    void*         buf;
    int           size;
    if(OAPV_SUCCEEDED(oapvm_get(mid, group_id, OAPV_METADATA_USER_DEFINED, &buf, &size, uuid_frm_hash))) {
        if(size != (imgb->np * 16) /* hash */ + 16 /* uuid */) {
            return 1;  // unexpected error
        }
        for(int i = 0; i < imgb->np; i++) {
            if(memcmp((unsigned char*)buf + ((i + 1) * 16), imgb->hash[i], 16) != 0) {
                return -1;  // frame hash is mismatched
            }
        }
        return 0;  // frame hash is correct
    }
    return 1;  // frame hash data is not available
}

static void print_stat_au(oapvd_stat_t* stat, int au_cnt, ARGS_VAR* args_var)
{
    if(au_cnt == 0) {
        if(args_var->output_csp != OUTPUT_CSP_NATIVE && args_var->hash != 0) {
            logv2("[Warning] cannot check frame hash value if special output CSP is defined\n")
        }
    }
    logv2_line("");
    logv2("AU %-5d  %10d-bytes  %3d-frame(s)\n", au_cnt, stat->read, stat->aui.num_frms);
}

static void print_stat_frm(oapvd_stat_t* stat, oapv_frms_t* frms, oapvm_t mid, ARGS_VAR* args_var)
{
    int   i, ret, hash_idx;
    oapv_frm_info_t* finfo;

    assert(stat->aui.num_frms == frms->num_frms);

    finfo = stat->aui.frm_info;

    for(i = 0; i < stat->aui.num_frms; i++) {
        // clang-format off
        const char* str_frm_type = finfo[i].pbu_type == OAPV_PBU_TYPE_PRIMARY_FRAME       ? "PRIMARY"
                                 : finfo[i].pbu_type == OAPV_PBU_TYPE_NON_PRIMARY_FRAME ? "NON-PRIMARY"
                                 : finfo[i].pbu_type == OAPV_PBU_TYPE_PREVIEW_FRAME     ? "PREVIEW"
                                 : finfo[i].pbu_type == OAPV_PBU_TYPE_DEPTH_FRAME       ? "DEPTH"
                                 : finfo[i].pbu_type == OAPV_PBU_TYPE_ALPHA_FRAME       ? "ALPHA"
                                 : "UNKNOWN";

        const char * str_csp = finfo[i].cs == OAPV_CS_YCBCR400_10LE ? "4:0:0-10"
                             : finfo[i].cs == OAPV_CS_YCBCR422_10LE ? "4:2:2-10"
                             : finfo[i].cs == OAPV_CS_YCBCR444_10LE ? "4:4:4-10"
                             : finfo[i].cs == OAPV_CS_YCBCR4444_10LE ? "4:4:4:4-10"
                             : "unknown-cs";

        // clang-format on

        logv2("- FRM %-2d GID %-5d %-11s %9d-bytes %5dx%4d %-10s",
            i, finfo[i].group_id, str_frm_type, stat->frm_size[i], finfo[i].w, finfo[i].h, str_csp);

        if(args_var->hash) {
            char* str_hash[4] = {"unsupport", "mismatch", "unavail", "match"};

            if(args_var->output_csp != OUTPUT_CSP_NATIVE) {
                hash_idx = 0;
            }
            else {
                ret = check_frm_hash(mid, frms->frm[i].imgb, frms->frm[i].group_id);
                if(ret < 0)
                    hash_idx = 1;  // mismatch
                else if(ret > 0)
                    hash_idx = 2;  // unavailable
                else
                    hash_idx = 3;  // matched
            }
            logv2("hash:%s", str_hash[hash_idx]);
        }
        logv2("\n");
    }
}

int main(int argc, const char** argv)
{
    ARGS_PARSER*     args;
    ARGS_VAR*        args_var;
    unsigned char*   bs_buf = NULL;
    oapvd_t          did    = NULL;
    oapvm_t          mid    = NULL;
    oapvd_cdesc_t    cdesc;
    oapv_bitb_t      bitb;
    oapv_frms_t      ofrms;
    oapv_imgb_t*     imgb_w = NULL;
    oapv_imgb_t*     imgb_o = NULL;
    oapv_frm_t*      frm    = NULL;
    oapv_au_info_t   aui;
    oapvd_stat_t     stat;
    int              ret = 0;
    oapv_clk_t       clk_beg, clk_tot;
    int              au_cnt, frm_cnt[OAPV_MAX_NUM_FRAMES];
    int              read_size, bs_buf_size = 0;
    int              i, w, h;
    FILE*            fp_bs  = NULL;
    int              is_y4m = 0;
    char*            errstr = NULL;
    oapv_frm_info_t* finfo  = NULL;

    /* help message */
    if(argc < 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
        print_usage(argv);
        return 0;
    }
    /* parse command line */
    args = args_create(dec_args_opts, NUM_ARGS_OPT);
    if(args == NULL) {
        logerr("cannot create argument parser\n");
        ret = -1;
        goto ERR;
    }
    args_var = args_init_vars(args);
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
    logv2(" / __ \\___  ___ ___  / _ | / _ \\ | / / Decoder\n");
    logv2("/ /_/ / _ \\/ -_) _ \\/ __ |/ ___/ |/ / \n");
    logv2("\\____/ .__/\\__/_//_/_/ |_/_/   |___/  \n");
    logv2("    /_/                               \n");
    logv2("\n");


    /* open input file */
    fp_bs = fopen(args_var->fname_inp, "rb");
    if(fp_bs == NULL) {
        logerr("ERROR: cannot open bitstream file = %s\n", args_var->fname_inp);
        print_usage(argv);
        return -1;
    }
    /* open output file */
    if(strlen(args_var->fname_out) > 0) {
        char  fext[16];
        char* fname = (char*)args_var->fname_out;

        if(strlen(fname) < 5) { /* at least x.yuv or x.y4m */
            logerr("ERROR: invalide output file name\n");
            return -1;
        }
        strncpy(fext, fname + strlen(fname) - 3, sizeof(fext) - 1);
        fext[0] = toupper(fext[0]);
        fext[1] = toupper(fext[1]);
        fext[2] = toupper(fext[2]);

        if(strcmp(fext, "YUV") == 0) {
            is_y4m = 0;
        }
        else if(strcmp(fext, "Y4M") == 0) {
            is_y4m = 1;
        }
        else {
            logerr("ERROR: unknown output format\n");
            ret = -1;
            goto ERR;
        }
        clear_data(fname); /* remove decoded file contents if exists */
    }

    // create bitstream buffer
    bs_buf = malloc(MAX_BS_BUF);
    if(bs_buf == NULL) {
        logerr("ERROR: cannot allocate bitstream buffer, size=%d\n", MAX_BS_BUF);
        ret = -1;
        goto ERR;
    }
    // create decoder
    cdesc.threads = args_var->threads;
    did           = oapvd_create(&cdesc, &ret);
    if(did == NULL) {
        logerr("ERROR: cannot create OAPV decoder (err=%d)\n", ret);
        ret = -1;
        goto ERR;
    }
    if(set_extra_config(did, args_var)) {
        logerr("ERROR: cannot set extra configurations\n");
        ret = -1;
        goto ERR;
    }

    clk_tot = 0;
    au_cnt  = 0;
    memset(frm_cnt, 0, sizeof(int) * OAPV_MAX_NUM_FRAMES);
    memset(&aui, 0, sizeof(oapv_au_info_t));
    memset(&ofrms, 0, sizeof(oapv_frms_t));

    /* create metadata container */
    mid = oapvm_create(&ret);
    if(OAPV_FAILED(ret)) {
        logerr("ERROR: cannot create OAPV metadata container (err=%d)\n", ret);
        ret = -1;
        goto ERR;
    }

    /* decoding loop */
    while(1) {
        read_size = read_bitstream(fp_bs, bs_buf, &bs_buf_size);
        if(read_size <= 0) {
            logv3("--> end of bitstream or reading error\n");
            break;
        }

        if(OAPV_FAILED(oapvd_info(bs_buf, bs_buf_size, &aui))) {
            logerr("cannot get information from bitstream\n");
            ret = -1;
            goto ERR;
        }

        /* create decoding frame buffers */
        ofrms.num_frms = aui.num_frms;
        for(i = 0; i < ofrms.num_frms; i++) {
            finfo = &aui.frm_info[FRM_IDX];
            frm   = &ofrms.frm[i];

            if(frm->imgb != NULL && (frm->imgb->w[0] != finfo->w || frm->imgb->h[0] != finfo->h)) {
                frm->imgb->release(frm->imgb);
                frm->imgb = NULL;
            }

            if(frm->imgb == NULL) {
                if(args_var->output_csp == 1) {
                    frm->imgb = imgb_create(finfo->w, finfo->h, OAPV_CS_SET(OAPV_CF_PLANAR2, 10, 0));
                }
                else {
                    frm->imgb = imgb_create(finfo->w, finfo->h, finfo->cs);
                }
                if(frm->imgb == NULL) {
                    logerr("cannot allocate image buffer (w:%d, h:%d, cs:%d)\n",
                           finfo->w, finfo->h, finfo->cs);
                    ret = -1;
                    goto ERR;
                }
            }
        }

        if(args_var->output_depth == 0) {
            args_var->output_depth = OAPV_CS_GET_BIT_DEPTH(finfo->cs);
        }

        /* main decoding block */
        bitb.addr  = bs_buf;
        bitb.ssize = bs_buf_size;
        memset(&stat, 0, sizeof(oapvd_stat_t));

        clk_beg = oapv_clk_get();

        ret     = oapvd_decode(did, &bitb, &ofrms, mid, &stat);

        clk_tot += oapv_clk_from(clk_beg);

        if(OAPV_FAILED(ret)) {
            logerr("failed to decode bitstream\n");
            ret = -1;
            goto END;
        }
        if(stat.read != bs_buf_size) {
            logerr("\t=> different reading of bitstream (in:%d, read:%d)\n",
                   bs_buf_size, stat.read);
        }

        /* testing of metadata reading */
        if(mid) {
            int              num_plds = 0;
            oapvm_payload_t* pld      = NULL;

            ret = oapvm_get_all(mid, NULL, &num_plds);
            if(OAPV_FAILED(ret)) {
                logerr("failed to read metadata\n");
                goto END;
            }
            if(num_plds > 0) {
                pld = malloc(sizeof(oapvm_payload_t) * num_plds);
                ret = oapvm_get_all(mid, pld, &num_plds);
            }
            if(pld != NULL)
                free(pld);
        }

        /* print decoding results */
        print_stat_au(&stat, au_cnt, args_var);
        print_stat_frm(&stat, &ofrms, mid, args_var);

        /* write decoded frames into files */
        for(i = 0; i < ofrms.num_frms; i++) {
            frm = &ofrms.frm[i];
            if(ofrms.num_frms > 0) {
                if(OAPV_CS_GET_BIT_DEPTH(frm->imgb->cs) != args_var->output_depth) {
                    if(imgb_w == NULL) {
                        imgb_w = imgb_create(frm->imgb->w[0], frm->imgb->h[0],
                                             OAPV_CS_SET(OAPV_CS_GET_FORMAT(frm->imgb->cs), args_var->output_depth, 0));
                        if(imgb_w == NULL) {
                            logerr("cannot allocate image buffer (w:%d, h:%d, cs:%d)\n",
                                   frm->imgb->w[0], frm->imgb->h[0], frm->imgb->cs);
                            ret = -1;
                            goto ERR;
                        }
                    }
                    imgb_cpy(imgb_w, frm->imgb);
                    imgb_o = imgb_w;
                }
                else {
                    imgb_o = frm->imgb;
                }

                w = imgb_o->w[0];
                h = imgb_o->h[0];

                if(strlen(args_var->fname_out)) {
                    if(frm_cnt[i] == 0 && is_y4m) {
                        if(write_y4m_header(args_var->fname_out, imgb_o)) {
                            logerr("cannot write Y4M header\n");
                            ret = -1;
                            goto END;
                        }
                    }
                    write_dec_img(args_var->fname_out, imgb_o, is_y4m);
                }
                frm_cnt[i]++;
            }
        }
        au_cnt++;
        oapvm_rem_all(mid);  // remove all metadata for next au decoding
        fflush(stdout);
        fflush(stderr);
    }

END:
    logv2_line("Summary");
    logv2("Processed AUs                     = %d\n", au_cnt);
    int total_frame_count = 0;
    for(i = 0; i < OAPV_MAX_NUM_FRAMES; i++)
        total_frame_count += frm_cnt[i];
    logv2("Decoded frame count               = %d\n", total_frame_count);
    if(total_frame_count > 0) {
        logv2("Total decoding time               = %d msec,", (int)oapv_clk_msec(clk_tot));
        logv2(" %.3f sec\n", (float)(oapv_clk_msec(clk_tot) / 1000.0));
        logv2("Average decoding time for a frame = %d msec\n", (int)oapv_clk_msec(clk_tot) / total_frame_count);
        logv2("Average decoding speed            = %.3f frames/sec\n", ((float)total_frame_count * 1000) / ((float)oapv_clk_msec(clk_tot)));
    }
    logv2_line(NULL);

ERR:
    if(did)
        oapvd_delete(did);
    if(mid)
        oapvm_delete(mid);
    for(int i = 0; i < ofrms.num_frms; i++) {
        if(ofrms.frm[i].imgb != NULL) {
            ofrms.frm[i].imgb->release(ofrms.frm[i].imgb);
        }
    }
    if(imgb_w != NULL)
        imgb_w->release(imgb_w);
    if(fp_bs)
        fclose(fp_bs);
    if(bs_buf)
        free(bs_buf);
    if(args)
        args->release(args);
    if(args_var)
        free(args_var);
    return ret;
}
