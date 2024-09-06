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

typedef struct _Y4M_PARAMS
{
    int w;
    int h;
    int fps;
    int color_format;
    int bit_depth;
}Y4M_INFO;

static int y4m_test(FILE * fp)
{

    char buffer[9] = { 0 };

    /*Peek to check if y4m header is present*/
    if (!fread(buffer, 1, 8, fp)) return -1;
    fseek( fp, 0, SEEK_SET );
    buffer[8] = '\0';
    if (memcmp(buffer, "YUV4MPEG", 8))
    {
        return 0;
    }
    return 1;
}

static int y4m_parse_tags(Y4M_INFO * y4m, char * tags)
{

    char *p;
    char *q;
    char t_buff[20];
    int found_w = 0, found_h = 0, found_cf = 0;
    int fps_n, fps_d, pix_ratio_n, pix_ratio_d, interlace;

    for (p = tags;; p = q)
    {

        /*Skip any leading spaces.*/
        while (*p == ' ') p++;

        /*If that's all we have, stop.*/
        if (p[0] == '\0') break;

        /*Find the end of this tag.*/
        for (q = p + 1; *q != '\0' && *q != ' '; q++) {
        }


        /*Process the tag.*/
        switch (p[0])
        {
        case 'W':
        {
            if (sscanf(p + 1, "%d", &y4m->w) != 1) return OAPV_ERR;
            found_w = 1;
            break;
        }
        case 'H':
        {
            if (sscanf(p + 1, "%d", &y4m->h) != 1) return OAPV_ERR;
            found_h = 1;
            break;
        }
        case 'F':
        {
            if (sscanf(p + 1, "%d:%d", &fps_n, &fps_d) != 2) return OAPV_ERR;
             y4m->fps = (int)((fps_n /(double)fps_d) + 0.5);
            break;
        }
        case 'I':
        {
           interlace = p[1];
           break;
        }
        case 'A':
        {
            if (sscanf(p + 1, "%d:%d", &pix_ratio_n, & pix_ratio_d) != 2) return OAPV_ERR;
            break;
        }
        case 'C':
        {
            if (q - p > 16) return OAPV_ERR;
            memcpy(t_buff, p + 1, q - p - 1);
            t_buff[q - p - 1] = '\0';
            found_cf = 1;
            break;
        }
        /*Ignore unknown tags.*/
        }
    }

    if (!(found_w == 1 && found_h == 1))
    {
        logerr("Mandatory arugments are not found in y4m header");
        return OAPV_ERR;
    }
    /* Setting default colorspace to yuv420 and input_bd to 8 if header info. is NA */
    if (!found_cf)
    {
        y4m->color_format = OAPV_CF_YCBCR420;
        y4m->bit_depth = 8;
    }

    if (strcmp(t_buff, "420jpeg") == 0 || strcmp(t_buff, "420") == 0 || \
        strcmp(t_buff, "420mpeg2") == 0 || strcmp(t_buff, "420paidv") == 0)
    {
         y4m->color_format = OAPV_CF_YCBCR420;
         y4m->bit_depth = 8;
    }
    else if (strcmp(t_buff, "422") == 0)
    {
         y4m->color_format = OAPV_CF_YCBCR422;
         y4m->bit_depth  = 8;
    }
    else if (strcmp(t_buff, "444") == 0)
    {
         y4m->color_format= OAPV_CF_YCBCR444;
         y4m->bit_depth  = 8;
    }
    else if (strcmp(t_buff, "420p10") == 0)
    {
        y4m->color_format = OAPV_CF_YCBCR420;
        y4m->bit_depth  = 10;
    }
    else if (strcmp(t_buff, "422p10") == 0)
    {
        y4m->color_format = OAPV_CF_YCBCR422;
        y4m->bit_depth  = 10;
    }
    else if (strcmp(t_buff, "444p10") == 0)
    {
        y4m->color_format = OAPV_CF_YCBCR444;
        y4m->bit_depth  = 10;
    }
    else if (strcmp(t_buff, "mono") == 0)
    {
        y4m->color_format = OAPV_CF_YCBCR400;
        y4m->bit_depth  = 8;
    }
    return OAPV_OK;
}


int y4m_header_parser(FILE * ip_y4m, Y4M_INFO * y4m)
{
    const int head_size = 128;
    char buffer[128];
    int ret;
    int i;

    memset(buffer, 0, sizeof(char)*head_size);

    /*Read until newline, or 128 cols, whichever happens first.*/
    for (i = 0; i < (head_size-1); i++) {

        if (!fread(buffer + i, 1, 1, ip_y4m)) return -1;
        if (buffer[i] == '\n') break;
    }
    /*We skipped too much header data.*/
   if (i == (head_size-1)) {
        logerr("Error parsing header; not a YUV2MPEG2 file?\n");
        return -1;
    }
    buffer[i] = '\0';
    if (memcmp(buffer, "YUV4MPEG", 8))
    {
        logerr("Incomplete magic for YUV4MPEG file.\n");
        return -1;
    }
    if (buffer[8] != '2')
    {
        logerr("Incorrect YUV input file version; YUV4MPEG2 required.\n");
    }
    ret = y4m_parse_tags(y4m, buffer + 5);
    if (ret < 0)
    {
        logerr("Error parsing YUV4MPEG2 header.\n");
        return ret;
    }
    return 0;
}

static void y4m_update_param(ARGS_PARSER * args, Y4M_INFO * y4m, oapve_param_t * param)
{
    args->set_int(args, "width", y4m->w);
    args->set_int(args, "height", y4m->h);
    args->set_int(args, "fps", y4m->fps);
    args->set_int(args, "input-depth", y4m->bit_depth);
}

static int write_y4m_header(char * fname, oapv_imgb_t * imgb)
{
    int color_format = OAPV_CS_GET_FORMAT(imgb->cs);
    int bit_depth  =   OAPV_CS_GET_BIT_DEPTH(imgb->cs);
    int len = 80;
    int buff_len = 0;
    char buf[80] = {'\0', };
    char c_buf[16] = { '\0', };
    FILE          * fp;

    if (color_format == OAPV_CF_YCBCR420)
    {
        if (bit_depth == 8) strcpy(c_buf, "420mpeg2");
        else if (bit_depth == 10) strcpy(c_buf, "420p10");
    }
    else if (color_format == OAPV_CF_YCBCR422)
    {
        if (bit_depth == 8) strcpy(c_buf, "422");
        else if (bit_depth == 10) strcpy(c_buf, "422p10");
    }
    else if (color_format == OAPV_CF_YCBCR444)
    {
        if (bit_depth == 8)  strcpy(c_buf, "444");
        else if (bit_depth == 10) strcpy(c_buf, "444p10");
    }
    else if (color_format == OAPV_CF_YCBCR400)
    {
        if (bit_depth == 8)  strcpy(c_buf, "mono");
    }

    if(strlen(c_buf) == 0)
    {
        logerr("Color format is not suuported by y4m");
        return -1;
    }

    /*setting fps to 30 by default as there is no fps related parameter */
    buff_len = snprintf(buf, len, "YUV4MPEG2 W%d H%d F%d:%d Ip C%s\n", \
        imgb->w[0], imgb->h[0], 30, 1, c_buf);


    fp = fopen(fname, "ab");
    if (fp == NULL)
    {
        logerr("cannot open file = %s\n", fname);
        return -1;
    }
    if (buff_len != fwrite(buf, 1, buff_len, fp))
    {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;

}
/* Frame level header or separator */
static int write_y4m_frame_header(char * fname)
{
    FILE * fp;
    fp = fopen(fname, "ab");
    if (fp == NULL)
    {
        logerr("cannot open file = %s\n", fname);
        return -1;
    }
    if (6 != fwrite("FRAME\n", 1, 6, fp))
    {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;

}
