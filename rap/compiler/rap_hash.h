
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Rap, Inc.
 */

#ifndef _RAP_HASH_H_INCLUDED_
#define _RAP_HASH_H_INCLUDED_

#include <deed.h>
#include <rap_string.h>
#include <rap_palloc.h>
#include <rap_array.h>

typedef struct
{
    void *value;
    uint2 len;
    uchar name[1];
} rap_hash_elt_t;

typedef struct
{
    rap_hash_elt_t **buckets;
    uint4 size;
} rap_hash_t;

typedef struct
{
    rap_hash_t hash;
    void *value;
} rap_hash_wildcard_t;

typedef struct
{
    rap_str_t key;
    uint4 key_hash;
    void *value;
} rap_hash_key_t;

typedef uint4 (*rap_hash_key_pt)(uchar *data, size_t len);

typedef struct
{
    rap_hash_t hash;
    rap_hash_wildcard_t *wc_head;
    rap_hash_wildcard_t *wc_tail;
} rap_hash_combined_t;

typedef struct
{
    rap_hash_t *hash;
    rap_hash_key_pt key;

    uint4 max_size;
    uint4 bucket_size;

    char *name;
    rap_pool_t *pool;
    rap_pool_t *temp_pool;
} rap_hash_init_t;

#define RAP_HASH_SMALL 1
#define RAP_HASH_LARGE 2

#define RAP_HASH_LARGE_ASIZE 16384
#define RAP_HASH_LARGE_HSIZE 10007

#define RAP_HASH_WILDCARD_KEY 1
#define RAP_HASH_READONLY_KEY 2

typedef struct
{
    uint4 hsize;

    rap_pool_t *pool;
    rap_pool_t *temp_pool;

    rap_array_t keys;
    rap_array_t *keys_hash;

    rap_array_t dns_wc_head;
    rap_array_t *dns_wc_head_hash;

    rap_array_t dns_wc_tail;
    rap_array_t *dns_wc_tail_hash;
} rap_hash_keys_arrays_t;

typedef struct
{
    uint4 hash;
    rap_str_t key;
    rap_str_t value;
    uchar *lowcase_key;
} rap_table_elt_t;

void *rap_hash_find(rap_hash_t *hash, uint4 key, uchar *name, size_t len);
void *rap_hash_find_wc_head(rap_hash_wildcard_t *hwc, uchar *name, size_t len);
void *rap_hash_find_wc_tail(rap_hash_wildcard_t *hwc, uchar *name, size_t len);
void *rap_hash_find_combined(rap_hash_combined_t *hash, uint4 key,
                             uchar *name, size_t len);

int4 rap_hash_init(rap_hash_init_t *hinit, rap_hash_key_t *names,
                   uint4 nelts);
int4 rap_hash_wildcard_init(rap_hash_init_t *hinit, rap_hash_key_t *names,
                            uint4 nelts);

#define rap_hash(key, c) ((uint4)key * 31 + c)
uint4 rap_hash_key(uchar *data, size_t len);
uint4 rap_hash_key_lc(uchar *data, size_t len);
uint4 rap_hash_strlow(uchar *dst, uchar *src, size_t n);

int4 rap_hash_keys_array_init(rap_hash_keys_arrays_t *ha, uint4 type);
int4 rap_hash_add_key(rap_hash_keys_arrays_t *ha, rap_str_t *key,
                      void *value, uint4 flags);

#endif /* _RAP_HASH_H_INCLUDED_ */
