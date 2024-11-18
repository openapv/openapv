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

static oapvm_ctx_t *meta_id_to_ctx(oapvm_t id)
{
    oapvm_ctx_t *ctx;
    oapv_assert_rv(id, NULL);
    ctx = (oapvm_ctx_t *)id;
    oapv_assert_rv(ctx->magic == OAPVM_MAGIC_CODE, NULL);
    return ctx;
}

static oapv_mdp_t **meta_mdp_find_last_with_check(oapv_md_t *md, int type, unsigned char *uuid)
{
    oapv_mdp_t **last_mdp = &md->md_payload;

    while((*last_mdp) != NULL) {
        if((*last_mdp) == NULL)
            break;
        if((*last_mdp)->pld_type == type) {
            if(type == OAPV_METADATA_USER_DEFINED) {
                if(oapv_mcmp(uuid, (*last_mdp)->pld_data, 16) == 0) {
                    return NULL;
                }
            }
            else {
                return NULL;
            }
        }
        last_mdp = &((*last_mdp)->next);
    }
    return last_mdp;
}

static oapv_mdp_t *meta_md_find_mdp(oapv_md_t *md, int mdt)
{
    oapv_mdp_t *mdp = md->md_payload;

    while(mdp != NULL) {
        if(mdp->pld_type == mdt) {
            break;
        }
        mdp = mdp->next;
    }

    return mdp;
}

static int meta_md_free_mdp(oapv_mdp_t *mdp)
{
    oapv_mfree(mdp->pld_data);
    return OAPV_OK;
}

static int meta_md_rm_mdp(oapv_md_t *md, int mdt)
{
    oapv_mdp_t *mdp = md->md_payload;
    oapv_mdp_t *mdp_prev = NULL;

    while(mdp->pld_type != mdt && mdp->next != NULL) {
        mdp_prev = mdp;
        mdp = mdp->next;
    }

    if(mdp->pld_type == mdt) {
        if(mdp_prev == NULL) {
            md->md_payload = NULL;
        }
        else {
            mdp_prev->next = mdp->next;
        }
        meta_md_free_mdp(mdp);
        md->md_num--;
        return OAPV_OK;
    }

    return OAPV_ERR_NOT_FOUND;
}

static int meta_md_rm_usd(oapv_md_t *md, unsigned char *uuid)
{
    oapv_mdp_t *mdp = md->md_payload;
    oapv_mdp_t *mdp_prev = NULL;
    while(mdp != NULL) {
        if(mdp->pld_type == OAPV_METADATA_USER_DEFINED) {
            if(oapv_mcmp(uuid, mdp->pld_data, 16) == 0) {
                if(mdp_prev == NULL) {
                    md->md_payload = NULL;
                }
                else {
                    mdp_prev->next = mdp->next;
                }
                oapv_assert_rv(md->md_size >= mdp->pld_size, OAPV_ERR_UNEXPECTED);
                md->md_size -= mdp->pld_size;
                meta_md_free_mdp(mdp);
                md->md_num--;
                return OAPV_OK;
            }
        }
        mdp_prev = mdp;
        mdp = mdp->next;
    }

    return OAPV_ERR_NOT_FOUND;
}

static oapv_mdp_t *meta_md_find_usd(oapv_md_t *md, unsigned char *uuid)
{
    oapv_mdp_t *mdp = md->md_payload;
    while(mdp != NULL) {
        if(mdp->pld_type == OAPV_METADATA_USER_DEFINED) {
            oapv_md_usd_t *usd = (oapv_md_usd_t *)mdp->pld_data;
            if(oapv_mcmp(uuid, usd->uuid, 16) == 0) {
                return mdp;
            }
        }
        mdp = mdp->next;
    }

    return NULL;
}

static int meta_verify_mdp_data(int type, int size, u8 *data)
{
    if(type == OAPV_METADATA_ITU_T_T35) {
        if(size == 0) {
            return OAPV_ERR_INVALID_ARGUMENT;
        }
        if(*data == 0xFF) {
            if(size == 1) {
                return OAPV_ERR_INVALID_ARGUMENT;
            }
        }
    }
    else if(type == OAPV_METADATA_MDCV) {
        if(size != 24) {
            return OAPV_ERR_INVALID_ARGUMENT;
        }
    }
    else if(type == OAPV_METADATA_CLL) {
        if(size != 4) {
            return OAPV_ERR_INVALID_ARGUMENT;
        }
    }
    else if(type == OAPV_METADATA_USER_DEFINED) {
        if(size < 16) {
            return OAPV_ERR_INVALID_ARGUMENT;
        }
    }
    else {
        return OAPV_OK;
    }
    return OAPV_OK;
}

static void meta_free_md(oapv_md_t *md)
{
    oapv_mdp_t *mdp = md->md_payload;
    while(mdp != NULL) {
        oapv_mdp_t *tmp_mdp = mdp;
        mdp = mdp->next;
        if(tmp_mdp->pld_data != NULL) {
            oapv_mfree(tmp_mdp->pld_data);
        }
        oapv_mfree(tmp_mdp);
    }
}

int oapvm_set(oapvm_t mid, int group_id, int type, void *data, int size, unsigned char *uuid)
{
    oapvm_ctx_t *md_list = meta_id_to_ctx(mid);

    int          ret = meta_verify_mdp_data(type, size, (u8 *)data);
    oapv_assert_rv(OAPV_SUCCEEDED(ret), ret);

    int md_list_idx = 0;
    while(md_list_idx < md_list->num) {
        if(md_list->group_ids[md_list_idx] == group_id) {
            break;
        }
        md_list_idx++;
    }
    if(md_list_idx == md_list->num) {
        md_list->num++;
    }
    md_list->group_ids[md_list_idx] = group_id;
    oapv_mdp_t **last_ptr = meta_mdp_find_last_with_check(&md_list->md_arr[md_list_idx], type,
                                                          (type == OAPV_METADATA_USER_DEFINED) ? uuid : NULL);
    while(last_ptr == NULL) {
        oapvm_rem(mid, group_id, type, uuid);
        last_ptr = meta_mdp_find_last_with_check(&md_list->md_arr[md_list_idx], type,
                                                 (type == OAPV_METADATA_USER_DEFINED) ? uuid : NULL);
    }
    oapv_mdp_t *tmp_mdp = oapv_malloc(sizeof(oapv_mdp_t));
    oapv_assert_rv(tmp_mdp != NULL, OAPV_ERR_OUT_OF_MEMORY);

    oapv_mset(tmp_mdp, 0, sizeof(oapv_mdp_t));
    tmp_mdp->pld_size = size;
    tmp_mdp->pld_type = type;
    tmp_mdp->pld_data = data;
    *last_ptr = tmp_mdp;

    /* calculate length of payload type */
    int tmp_mpt = type;
    while(tmp_mpt >= 255) {
        tmp_mpt -= 255;
        md_list->md_arr[md_list_idx].md_size += 1;
    }
    md_list->md_arr[md_list_idx].md_size += 1;

    /*  calculate length of payload data size */
    int tmp_mps = size;
    while(tmp_mps >= 255) {
        tmp_mps -= 255;
        md_list->md_arr[md_list_idx].md_size += 1;
    }
    md_list->md_arr[md_list_idx].md_size += 1;

    md_list->md_arr[md_list_idx].md_size += tmp_mdp->pld_size;
    md_list->md_arr[md_list_idx].md_num++;
    return OAPV_OK;
}

int oapvm_get(oapvm_t mid, int group_id, int type, void **data, int *size, unsigned char *uuid)
{
    oapvm_ctx_t *md_list = meta_id_to_ctx(mid);
    for(int i = 0; i < md_list->num; i++) {
        if(group_id != md_list->group_ids[i]) {
            continue;
        }
        oapv_mdp_t *mdp = (type == OAPV_METADATA_USER_DEFINED) ? meta_md_find_usd(&md_list->md_arr[i], uuid) : meta_md_find_mdp(&md_list->md_arr[i], type);
        if(mdp != NULL) {
            *data = mdp->pld_data;
            *size = mdp->pld_size;
            return OAPV_OK;
        }
    }

    return OAPV_ERR_NOT_FOUND;
}
int oapvm_rem(oapvm_t mid, int group_id, int type, unsigned char *uuid)
{
    oapvm_ctx_t *md_list = meta_id_to_ctx(mid);
    int          md_list_idx = 0;
    while(md_list_idx < md_list->num) {
        if(md_list->group_ids[md_list_idx] == group_id) {
            break;
        }
        md_list_idx++;
    }
    oapv_assert_rv(md_list_idx < md_list->num, OAPV_ERR_NOT_FOUND);
    if(type == OAPV_METADATA_USER_DEFINED) {
        return meta_md_rm_usd(&md_list->md_arr[md_list_idx], uuid);
    }
    return meta_md_rm_mdp(&md_list->md_arr[md_list_idx], type);
}

int oapvm_set_all(oapvm_t mid, oapvm_payload_t *pld, int num_plds)
{
    int          ret;
    oapvm_ctx_t *md_list = meta_id_to_ctx(mid);
    for(int i = 0; i < num_plds; i++) {
        ret = meta_verify_mdp_data(pld[i].type, pld[i].data_size, (u8 *)pld[i].data);
        oapv_assert_g(OAPV_SUCCEEDED(ret), ERR);

        int md_list_idx = 0;
        while(md_list_idx < md_list->num) {
            if(md_list->group_ids[md_list_idx] == pld[i].group_id) {
                break;
            }
            md_list_idx++;
        }
        if(md_list_idx == md_list->num) {
            md_list->num++;
        }

        md_list->group_ids[md_list_idx] = pld[i].group_id;
        md_list->md_arr[md_list_idx].md_num++;
        oapv_mdp_t **last_ptr = meta_mdp_find_last_with_check(&md_list->md_arr[md_list_idx], pld[i].type,
                                                              (pld[i].type == OAPV_METADATA_USER_DEFINED) ? pld[i].uuid : NULL);
        while(last_ptr == NULL) {
            oapvm_rem(mid, pld[i].group_id, pld[i].type, pld[i].uuid);
            last_ptr = meta_mdp_find_last_with_check(&md_list->md_arr[md_list_idx], pld[i].type,
                                                     (pld[i].type == OAPV_METADATA_USER_DEFINED) ? pld[i].uuid : NULL);
        }
        oapv_mdp_t *tmp_mdp = oapv_malloc(sizeof(oapv_mdp_t));
        oapv_assert_rv(tmp_mdp != NULL, OAPV_ERR_OUT_OF_MEMORY);
        oapv_mset(tmp_mdp, 0, sizeof(oapv_mdp_t));
        tmp_mdp->pld_size = pld[i].data_size;
        tmp_mdp->pld_type = pld[i].type;
        tmp_mdp->pld_data = pld[i].data;
        md_list->md_arr[md_list_idx].md_size += tmp_mdp->pld_size;

        *last_ptr = tmp_mdp;
    }
    return OAPV_OK;

ERR:
    return ret;
}

int oapvm_get_all(oapvm_t mid, oapvm_payload_t *pld, int *num_plds)
{
    oapvm_ctx_t *md_list = meta_id_to_ctx(mid);
    if(pld == NULL) {
        int num_payload = 0;
        for(int i = 0; i < md_list->num; i++) {
            num_payload += md_list->md_arr[i].md_num;
        }
        *num_plds = num_payload;
        return OAPV_OK;
    }
    int p_cnt = 0;
    for(int i = 0; i < md_list->num; i++) {
        int         group_id = md_list->group_ids[i];
        oapv_mdp_t *mdp = md_list->md_arr[i].md_payload;
        while(mdp != NULL) {
            oapv_assert_rv(p_cnt < *num_plds, OAPV_ERR_REACHED_MAX);
            pld[p_cnt].group_id = group_id;
            pld[p_cnt].data_size = mdp->pld_size;
            pld[p_cnt].data = mdp->pld_data;
            pld[p_cnt].type = mdp->pld_type;
            if(pld[p_cnt].type == OAPV_METADATA_USER_DEFINED) {
                oapv_mcpy(pld[p_cnt].uuid, mdp->pld_data, 16);
            }
            else {
                oapv_mset(pld[p_cnt].uuid, 0, 16);
            }
            p_cnt++;
            mdp = mdp->next;
        }
    }
    *num_plds = p_cnt;
    return OAPV_OK;
}

void oapvm_rem_all(oapvm_t mid)
{

    oapvm_ctx_t *md_list = meta_id_to_ctx(mid);
    for(int i = 0; i < md_list->num; i++) {
        meta_free_md(&md_list->md_arr[i]);
        oapv_mset(&md_list->md_arr[i], 0, sizeof(oapv_md_t));
    }
    md_list->num = 0;
}

oapvm_t oapvm_create(int *err)
{
    oapvm_ctx_t *md_list;
    md_list = oapv_malloc(sizeof(oapvm_ctx_t));
    if(md_list == NULL) {
        *err = OAPV_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    oapv_mset(md_list, 0, sizeof(oapvm_ctx_t));

    md_list->magic = OAPVM_MAGIC_CODE;
    return md_list;
}

void oapvm_delete(oapvm_t mid)
{
    oapvm_ctx_t *md_list = meta_id_to_ctx(mid);
    for(int i = 0; i < md_list->num; i++) {
        meta_free_md(&md_list->md_arr[i]);
    }
    oapv_mfree(md_list);
}
