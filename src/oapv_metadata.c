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


#include "oapv_metadata.h"

static oapv_md_info_list_t* meta_id_to_ctx(oapvm_t id)
{
    oapv_md_info_list_t *ctx;
    oapv_assert_rv(id, NULL);
    ctx = (oapv_md_info_list_t *)id;
    oapv_assert_rv(ctx->magic == OAPVM_MAGIC_CODE, NULL);
    return ctx;
}

static oapv_mdp_t** oapv_mdp_find_last_with_check(oapv_md_t* md, int type, unsigned char* uuid)
{
    oapv_mdp_t** last_mdp = &md->metadata_payload;

    while ((*last_mdp) != NULL)
    {
        if ((*last_mdp) == NULL) break;
        if((*last_mdp)->metadata_payload_type == type)
        {
            if (type == OAPV_METADATA_USER_DEFINED)
            {
                if (oapv_mcmp(uuid, (*last_mdp)->data, 16) == 0)
                {
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


static oapv_mdp_t** oapv_mdp_find_last(oapv_md_t* md)
{
    oapv_mdp_t** last_mdp = &md->metadata_payload;

    while ((*last_mdp) != NULL)
    {
        if ((*last_mdp) == NULL) break;
        last_mdp = &((*last_mdp)->next);
    }
    return last_mdp;
}
oapv_mdp_t* oapv_md_find_mdp(oapv_md_t* md, int mdt)
{
    oapv_mdp_t* mdp = md->metadata_payload;

    while (mdp != NULL)
    {
        if (mdp->metadata_payload_type == mdt)
        {
            break;
        }
        mdp = mdp->next;
    }

    return mdp;
}

int oapv_md_free_mdp(oapv_mdp_t* mdp)
{
    oapv_mfree(mdp->data);
    return OAPV_OK;
}


int oapv_md_rm_mdp(oapv_md_t* md, int mdt)
{
    oapv_mdp_t* mdp = md->metadata_payload;
    oapv_mdp_t* mdp_prev = NULL;

    while (mdp->metadata_payload_type != mdt && mdp->next != NULL)
    {
        mdp_prev = mdp;
        mdp = mdp->next;
    }

    if (mdp->metadata_payload_type == mdt)
    {
        if (mdp_prev == NULL)
        {
            md->metadata_payload = NULL;
        }
        else
        {
            mdp_prev->next = mdp->next;
        }
        oapv_md_free_mdp(mdp);
        md->md_num--;
        return OAPV_OK;
    }

    return OAPV_ERR;
}



int oapv_md_rm_usd(oapv_md_t* md, unsigned char* uuid)
{
    oapv_mdp_t* mdp = md->metadata_payload;
    oapv_mdp_t* mdp_prev = NULL;
    while (mdp != NULL)
    {
        if (mdp->metadata_payload_type == OAPV_METADATA_USER_DEFINED)
        {
            if (oapv_mcmp(uuid, mdp->data, 16) == 0)
            {
                if (mdp_prev == NULL)
                {
                    md->metadata_payload = NULL;
                }
                else
                {
                    mdp_prev->next = mdp->next;
                }
                md->metadata_size -= mdp->metadata_payload_size;
                if (md->metadata_size < 0)
                {
                    oapv_assert_rv(0, OAPV_ERR);
                }
                oapv_md_free_mdp(mdp);
                md->md_num--;
                return OAPV_OK;
            }
        }
        mdp_prev = mdp;
        mdp = mdp->next;
    }

    return OAPV_ERR;
}

oapv_mdp_t* oapv_md_find_usd(oapv_md_t* md, unsigned char* uuid)
{
    oapv_mdp_t* mdp = md->metadata_payload;
    while (mdp != NULL)
    {
        if (mdp->metadata_payload_type == OAPV_METADATA_USER_DEFINED)
        {
            oapv_md_usd_t* usd = (oapv_md_usd_t*)mdp->data;
            if (strncmp(uuid, usd->uuid, 16) == 0)
            {
                return mdp;
            }
        }
        mdp = mdp->next;
    }

    return NULL;

}


void oapv_md_set_metadata(oapv_md_t* md, int mpt, u32 data_size, void* data)
{
    oapv_mdp_t* new_mdp = oapv_malloc_fast(sizeof(oapv_mdp_t));
    oapv_mset(new_mdp, 0, sizeof(oapv_mdp_t));

    if (mpt == OAPV_METADATA_USER_DEFINED)
    {
        u8 uuid[16];
        oapv_mcpy(uuid, data, sizeof(u8) * 16);
        if (oapv_md_find_usd(md, uuid) != NULL)
        {
            oapv_md_rm_usd(md, uuid);
        }
    }
    else
    {
        if (oapv_md_find_mdp(md, mpt) != NULL)
        {
            oapv_md_rm_mdp(md, mpt);
        }
    }

    /* calculate length of payload type */
    int tmp_mpt = mpt;
    while (tmp_mpt >= 255)
    {
        tmp_mpt -= 255;
        md->metadata_size += 8;
    }
    md->metadata_size += 8;

    /*  calculate length of payload data size */
    int tmp_mps = data_size;
    while (tmp_mps >= 255)
    {
        tmp_mps -= 255;
        md->metadata_size += 8;
    }
    md->metadata_size += 8;

    md->metadata_size += data_size;

    new_mdp->metadata_payload_type = mpt;
    new_mdp->metadata_payload_size = data_size;

    if (mpt == OAPV_METADATA_ITU_T_T35)
    {
        oapv_md_t35_t* md_t35 = oapv_malloc_fast(sizeof(oapv_md_t35_t));
        u8* data_t = (u8*)data;
        int idx = 0;
        md_t35->itu_t_t35_country_code = data_t[idx++];
        if (md_t35->itu_t_t35_country_code == 0xFF)
        {
            md_t35->itu_t_t35_country_code_extension = data_t[idx++];
        }
        u32 data_size_t = sizeof(u8) * (data_size - idx);
        md_t35->itu_t_t35_payload = (u8*)oapv_malloc_fast(data_size_t);
        oapv_mcpy(md_t35->itu_t_t35_payload, data_t + idx, data_size_t);

        new_mdp->data = (void*)md_t35;
    }
    else if (mpt == OAPV_METADATA_FILLER)
    {
        oapv_md_fm_t* md_fm = oapv_malloc_fast(sizeof(oapv_md_fm_t));
        md_fm->ff_byte = (u8*)oapv_malloc_fast(sizeof(u8) * data_size);
        oapv_mcpy(md_fm->ff_byte, data, data_size);

        new_mdp->data = (void*)md_fm;
    }
    else if (mpt == OAPV_METADATA_MDCV)
    {
        oapv_md_mdcv_t* md_mdcv = oapv_malloc_fast(sizeof(oapv_md_mdcv_t));
        oapv_mcpy(md_mdcv, data, sizeof(oapv_md_mdcv_t));

        new_mdp->data = (void*)md_mdcv;
    }
    else if (mpt == OAPV_METADATA_CLL)
    {
        oapv_md_cll_t* md_cll = oapv_malloc_fast(sizeof(oapv_md_cll_t));
        oapv_mcpy(md_cll, data, sizeof(oapv_md_cll_t));

        new_mdp->data = (void*)md_cll;
    }
    else if (mpt == OAPV_METADATA_USER_DEFINED)
    {
        oapv_md_usd_t* md_usd = oapv_malloc_fast(sizeof(oapv_md_usd_t));
        oapv_mcpy(md_usd->uuid, (u8*)data, sizeof(u8) * data_size);

        u8* data_t = (u8*)data;
        u32 data_size_t = sizeof(u8) * (data_size - 16);
        md_usd->user_defined_data_payload = oapv_malloc_fast(data_size_t);
        oapv_mcpy(md_usd->user_defined_data_payload, data_t + 16, data_size_t);

        new_mdp->data = (void*)md_usd;
    }
    else // METADATA_UNDEFINED
    {
        oapv_md_ud_t* md_ud = oapv_malloc_fast(sizeof(oapv_md_ud_t));

        u8* data_t = (u8*)data;
        md_ud->undefined_data_payload = oapv_malloc_fast(data_size);
        oapv_mcpy(md_ud->undefined_data_payload, data_t + 16, data_size);

        new_mdp->data = (void*)md_ud;
    }


    oapv_mdp_t** last_mdp = oapv_mdp_find_last(md);
    if (*last_mdp == NULL)
    {
        md->metadata_payload = new_mdp;
    }
    else
    {
        (*last_mdp)->next = new_mdp;
    }
}
static int oapv_verify_mdp_data(int type, int size, u8* data) {
    if (type == OAPV_METADATA_ITU_T_T35) {
        if (size == 0) {
            return OAPV_ERR;
        }
        if (*data == 0xFF) {
            if (size == 1) {
                return OAPV_ERR;
            }
        }
    }
    else if (type == OAPV_METADATA_MDCV) {
        if (size != 24) {
            return OAPV_ERR;
        }
    }
    else if (type == OAPV_METADATA_CLL) {
        if (size != 4) {
            return OAPV_ERR;
        }
    }
    else if (type == OAPV_METADATA_USER_DEFINED) {
        if (size < 16) {
            return OAPV_ERR;
        }
    }
    else {
        return OAPV_OK;
    }
    return OAPV_OK;
}


oapv_md_t* oapv_md_alloc(void)
{
    oapv_md_t* md_list = oapv_malloc_fast(sizeof(oapv_md_t));
    oapv_assert_rv(md_list, NULL);
    oapv_mset(md_list, 0, sizeof(oapv_md_t));
    return md_list;
}


int oapv_meta_get(oapv_md_t* md, int type, void** buf, int* size, unsigned char* uuid)
{
    oapv_mdp_t* mdp;

    if (type == OAPV_METADATA_USER_DEFINED)
    {
        if (uuid == NULL)
        {
            oapv_assert_rv(0, OAPV_ERR_INVALID_ARGUMENT);
        }
        mdp = oapv_md_find_usd(md, uuid);
    }
    else
    {
        mdp = oapv_md_find_mdp(md, type);
    }

    if (mdp == NULL)
    {
        return OAPV_ERR_INVALID_ARGUMENT;
    }

    *size = mdp->metadata_payload_size;
    *buf = mdp->data;

    return OAPV_OK;
}

int oapvm_set(oapvm_t mid, int group_id, int type, void* data, int size, unsigned char* uuid)
{
    if (OAPV_FAILED(oapv_verify_mdp_data(type, size, (u8*)data))) {
        return OAPV_ERR;
    }
    oapv_md_info_list_t* md_list = meta_id_to_ctx(mid);

    int md_list_idx = 0;
    while (md_list_idx < md_list->num)
    {
        if (md_list->group_ids[md_list_idx] == group_id) {
            break;
        }
        md_list_idx++;
    }
    if (md_list_idx == md_list->num)
    {
        md_list->num++;
    }
    md_list->group_ids[md_list_idx] = group_id;
    oapv_mdp_t **last_ptr = oapv_mdp_find_last_with_check(&md_list->md_arr[md_list_idx], type,
                                                          (type==OAPV_METADATA_USER_DEFINED) ? uuid : NULL);
    while (last_ptr == NULL)
    {
        oapvm_rem(mid, group_id, type, uuid);
        last_ptr = oapv_mdp_find_last_with_check(&md_list->md_arr[md_list_idx], type,
                                                          (type==OAPV_METADATA_USER_DEFINED) ? uuid : NULL);
    }
    oapv_mdp_t* tmp_mdp = oapv_malloc_fast(sizeof(oapv_mdp_t));
    oapv_mset(tmp_mdp, 0, sizeof(oapv_mdp_t));
    tmp_mdp->metadata_payload_size = size;
    tmp_mdp->metadata_payload_type = type;
    tmp_mdp->data = data;
    *last_ptr = tmp_mdp;

    /* calculate length of payload type */
    int tmp_mpt = type;
    while (tmp_mpt >= 255)
    {
        tmp_mpt -= 255;
        md_list->md_arr[md_list_idx].metadata_size += 8;
    }
    md_list->md_arr[md_list_idx].metadata_size += 8;

    /*  calculate length of payload data size */
    int tmp_mps = size;
    while (tmp_mps >= 255)
    {
        tmp_mps -= 255;
        md_list->md_arr[md_list_idx].metadata_size += 8;
    }
    md_list->md_arr[md_list_idx].metadata_size += 8;

    md_list->md_arr[md_list_idx].metadata_size += tmp_mdp->metadata_payload_size;
    md_list->md_arr[md_list_idx].md_num++;
    return OAPV_OK;
}

int oapvm_get(oapvm_t mid, int group_id, int type, void** data, int* size, unsigned char* uuid)
{
    oapv_md_info_list_t* md_list = meta_id_to_ctx(mid);
    for(int i=0; i<md_list->num; i++)
    {
        if (group_id != md_list->group_ids[i])
        {
            continue;
        }
        oapv_mdp_t* mdp = (type == OAPV_METADATA_USER_DEFINED) ? oapv_md_find_usd(&md_list->md_arr[i], uuid) : oapv_md_find_mdp(&md_list->md_arr[i], type);
        if (mdp == NULL)
        {
            return OAPV_ERR;
        }
        *data = mdp->data;
        *size = mdp->metadata_payload_size;
        return OAPV_OK;
    }

    return OAPV_ERR;
}
int oapvm_rem(oapvm_t mid, int group_id, int type, unsigned char* uuid)
{
    oapv_md_info_list_t* md_list = meta_id_to_ctx(mid);
    int md_list_idx = 0;
    while (md_list_idx < md_list->num)
    {
        if (md_list->group_ids[md_list_idx] == group_id) {
            break;
        }
        md_list_idx++;
    }
    if (md_list_idx == md_list->num)
    {
        return OAPV_ERR;
    }
    if (type == OAPV_METADATA_USER_DEFINED)
    {
        return oapv_md_rm_usd(&md_list->md_arr[md_list_idx], uuid);
    }
    return oapv_md_rm_mdp(&md_list->md_arr[md_list_idx], type);
}

int oapvm_set_all(oapvm_t mid, oapvm_payload_t* pld, int num_plds)
{
    oapv_md_info_list_t* md_list = meta_id_to_ctx(mid);
    for(int i=0; i < num_plds; i++) {
        if (OAPV_FAILED(oapv_verify_mdp_data(pld[i].type, pld[i].data_size, (u8*)pld[i].data))) {
            return OAPV_ERR;
        }
        int md_list_idx = 0;
        while (md_list_idx < md_list->num)
        {
            if (md_list->group_ids[md_list_idx] == pld[i].group_id) {
                break;
            }
            md_list_idx++;
        }
        if (md_list_idx == md_list->num)
        {
            md_list->num++;
        }
        if (pld[i].type != OAPV_METADATA_USER_DEFINED && oapv_md_find_mdp(&md_list->md_arr[md_list_idx], pld[i].group_id) != NULL) {
            return OAPV_ERR;
        }

        md_list->group_ids[md_list_idx] = pld[i].group_id;
        md_list->md_arr[md_list_idx].md_num++;
         oapv_mdp_t **last_ptr = oapv_mdp_find_last_with_check(&md_list->md_arr[md_list_idx], pld[i].type,
                                                              (pld[i].type==OAPV_METADATA_USER_DEFINED) ? pld[i].uuid : NULL);
        while (last_ptr == NULL)
        {

            oapvm_rem(mid, pld[i].group_id, pld[i].type, pld[i].uuid);
            last_ptr = oapv_mdp_find_last_with_check(&md_list->md_arr[md_list_idx], pld[i].type,
                                                              (pld[i].type==OAPV_METADATA_USER_DEFINED) ? pld[i].uuid : NULL);
        }
        oapv_mdp_t* tmp_mdp = oapv_malloc(sizeof(oapv_mdp_t));
        oapv_mset(tmp_mdp, 0, sizeof(oapv_mdp_t));
        tmp_mdp->metadata_payload_size = pld[i].data_size;
        tmp_mdp->metadata_payload_type = pld[i].type;
        tmp_mdp->data = pld[i].data;
        md_list->md_arr[md_list_idx].metadata_size += tmp_mdp->metadata_payload_size;

        *last_ptr = tmp_mdp;
    }
    return OAPV_OK;
}

int oapvm_get_all(oapvm_t mid, oapvm_payload_t* pld, int* num_plds)
{
    oapv_md_info_list_t* md_list = meta_id_to_ctx(mid);
    if (pld == NULL)
    {
        int num_payload = 0;
        for (int i=0; i< md_list->num; i++) {
            num_payload += md_list[i].md_arr->md_num;
        }
        *num_plds = num_payload;
        return OAPV_OK;
    }
    int p_cnt = 0;
    for(int i=0; i<md_list->num; i++)
    {
        int group_id = md_list->group_ids[i];
        oapv_mdp_t* mdp = md_list->md_arr[i].metadata_payload;
        while(mdp != NULL)
        {
            pld[p_cnt].group_id = group_id;
            pld[p_cnt].data_size = mdp->metadata_payload_size;
            pld[p_cnt].data = mdp->data; // TBD : Can't know data size at decoder app. (Couldn't malloc)
            pld[p_cnt].type = mdp->metadata_payload_type;
            if (pld[p_cnt].type == OAPV_METADATA_USER_DEFINED)
            {
                oapv_mcpy(pld[p_cnt].uuid, mdp->data, 16);
            }
            else
            {
                oapv_mset(pld[p_cnt].uuid, 0, 16);
            }
            p_cnt++;
            mdp = mdp->next;
        }
    }
    *num_plds = p_cnt;
    return OAPV_OK;
}

static void oapv_free_md(oapv_md_t* md) {
    oapv_mdp_t *mdp = md->metadata_payload;
    while(mdp!=NULL) {
        oapv_mdp_t *tmp_mdp = mdp;
        mdp = mdp->next;
        if (tmp_mdp->data != NULL) {
            free(tmp_mdp->data);
        }
        free(tmp_mdp);
    }
}

void oapvm_rem_all(oapvm_t mid) {

    oapv_md_info_list_t* md_list = meta_id_to_ctx(mid);
    for (int i=0; i<md_list->num; i++)
    {
        oapv_free_md(&md_list->md_arr[i]);
        oapv_mset(&md_list->md_arr[i], 0, sizeof(oapv_md_t));
    }
    md_list->num = 0;
}

oapvm_t oapvm_create(int* err)
{
    oapv_md_info_list_t* md_list;
    md_list = oapv_malloc(sizeof(oapv_md_info_list_t));
    if (md_list == NULL) {
        *err = OAPV_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    oapv_mset(md_list, 0, sizeof(oapv_md_info_list_t));

    md_list->magic = OAPVM_MAGIC_CODE;
    return md_list;
}

void oapvm_delete(oapvm_t mid)
{
    oapv_md_info_list_t* md_list = meta_id_to_ctx(mid);
    for (int i=0; i<md_list->num; i++)
    {
        oapv_free_md(&md_list->md_arr[i]);
    }
    oapv_mfree(md_list);
}
