
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Rap, Inc.
 */


#include <rap_hash.h>

void *
rap_hash_find(rap_hash_t *hash, uint4 key, uchar *name, size_t len)
{
    uint4       i;
    rap_hash_elt_t  *elt;

#if 0
    rap_log_error(RAP_LOG_ALERT, rap_cycle->log, 0, "hf:\"%*s\"", len, name);
#endif

    elt = hash->buckets[key % hash->size];

    if (elt == NULL) {
        return NULL;
    }

    while (elt->value) {
        if (len != (size_t) elt->len) {
            goto next;
        }

        for (i = 0; i < len; i++) {
            if (name[i] != elt->name[i]) {
                goto next;
            }
        }

        return elt->value;

    next:

        elt = (rap_hash_elt_t *) rap_align_ptr(&elt->name[0] + elt->len,
                                               sizeof(void *));
        continue;
    }

    return NULL;
}


void *
rap_hash_find_wc_head(rap_hash_wildcard_t *hwc, uchar *name, size_t len)
{
    void        *value;
    uint4   i, n, key;

#if 0
    rap_log_error(RAP_LOG_ALERT, rap_cycle->log, 0, "wch:\"%*s\"", len, name);
#endif

    n = len;

    while (n) {
        if (name[n - 1] == '.') {
            break;
        }

        n--;
    }

    key = 0;

    for (i = n; i < len; i++) {
        key = rap_hash(key, name[i]);
    }

#if 0
    rap_log_error(RAP_LOG_ALERT, rap_cycle->log, 0, "key:\"%ui\"", key);
#endif

    value = rap_hash_find(&hwc->hash, key, &name[n], len - n);

#if 0
    rap_log_error(RAP_LOG_ALERT, rap_cycle->log, 0, "value:\"%p\"", value);
#endif

    if (value) {

        /*
         * the 2 low bits of value have the special meaning:
         *     00 - value is data pointer for both "example.com"
         *          and "*.example.com";
         *     01 - value is data pointer for "*.example.com" only;
         *     10 - value is pointer to wildcard hash allowing
         *          both "example.com" and "*.example.com";
         *     11 - value is pointer to wildcard hash allowing
         *          "*.example.com" only.
         */

        if ((uintptr_t) value & 2) {

            if (n == 0) {

                /* "example.com" */

                if ((uintptr_t) value & 1) {
                    return NULL;
                }

                hwc = (rap_hash_wildcard_t *)
                                          ((uintptr_t) value & (uintptr_t) ~3);
                return hwc->value;
            }

            hwc = (rap_hash_wildcard_t *) ((uintptr_t) value & (uintptr_t) ~3);

            value = rap_hash_find_wc_head(hwc, name, n - 1);

            if (value) {
                return value;
            }

            return hwc->value;
        }

        if ((uintptr_t) value & 1) {

            if (n == 0) {

                /* "example.com" */

                return NULL;
            }

            return (void *) ((uintptr_t) value & (uintptr_t) ~3);
        }

        return value;
    }

    return hwc->value;
}


void *
rap_hash_find_wc_tail(rap_hash_wildcard_t *hwc, uchar *name, size_t len)
{
    void        *value;
    uint4   i, key;

#if 0
    rap_log_error(RAP_LOG_ALERT, rap_cycle->log, 0, "wct:\"%*s\"", len, name);
#endif

    key = 0;

    for (i = 0; i < len; i++) {
        if (name[i] == '.') {
            break;
        }

        key = rap_hash(key, name[i]);
    }

    if (i == len) {
        return NULL;
    }

#if 0
    rap_log_error(RAP_LOG_ALERT, rap_cycle->log, 0, "key:\"%ui\"", key);
#endif

    value = rap_hash_find(&hwc->hash, key, name, i);

#if 0
    rap_log_error(RAP_LOG_ALERT, rap_cycle->log, 0, "value:\"%p\"", value);
#endif

    if (value) {

        /*
         * the 2 low bits of value have the special meaning:
         *     00 - value is data pointer;
         *     11 - value is pointer to wildcard hash allowing "example.*".
         */

        if ((uintptr_t) value & 2) {

            i++;

            hwc = (rap_hash_wildcard_t *) ((uintptr_t) value & (uintptr_t) ~3);

            value = rap_hash_find_wc_tail(hwc, &name[i], len - i);

            if (value) {
                return value;
            }

            return hwc->value;
        }

        return value;
    }

    return hwc->value;
}


void *
rap_hash_find_combined(rap_hash_combined_t *hash, uint4 key, uchar *name,
    size_t len)
{
    void  *value;

    if (hash->hash.buckets) {
        value = rap_hash_find(&hash->hash, key, name, len);

        if (value) {
            return value;
        }
    }

    if (len == 0) {
        return NULL;
    }

    if (hash->wc_head && hash->wc_head->hash.buckets) {
        value = rap_hash_find_wc_head(hash->wc_head, name, len);

        if (value) {
            return value;
        }
    }

    if (hash->wc_tail && hash->wc_tail->hash.buckets) {
        value = rap_hash_find_wc_tail(hash->wc_tail, name, len);

        if (value) {
            return value;
        }
    }

    return NULL;
}


#define RAP_HASH_ELT_SIZE(name)                                               \
    (sizeof(void *) + rap_align((name)->key.len + 2, sizeof(void *)))

int4
rap_hash_init(rap_hash_init_t *hinit, rap_hash_key_t *names, uint4 nelts)
{
    uchar          *elts;
    size_t           len;
    uint2         *test;
    uint4       i, n, key, size, start, bucket_size;
    rap_hash_elt_t  *elt, **buckets;

    if (hinit->max_size == 0) {
          log_e("could not build %s, you should increase %s_max_size: %i", hinit->name, hinit->name, hinit->max_size);
        return RAP_ERROR;
    }

    if (hinit->bucket_size > 65536 - rap_cacheline_size) {
        log_e("could not build %s, too large %s_bucket_size: %i",hinit->name, hinit->name, hinit->bucket_size);
        return RAP_ERROR;
    }

    for (n = 0; n < nelts; n++) {
        if (hinit->bucket_size < RAP_HASH_ELT_SIZE(&names[n]) + sizeof(void *))
        {
            log_e("could not build %s, you should increase %s_bucket_size: %i", hinit->name, hinit->name, hinit->bucket_size);
            return RAP_ERROR;
        }
    }

    test = rap_alloc(hinit->max_size * sizeof(uint2));
    if (test == NULL) {
        return RAP_ERROR;
    }

    bucket_size = hinit->bucket_size - sizeof(void *);

    start = nelts / (bucket_size / (2 * sizeof(void *)));
    start = start ? start : 1;

    if (hinit->max_size > 10000 && nelts && hinit->max_size / nelts < 100) {
        start = hinit->max_size - 1000;
    }

    for (size = start; size <= hinit->max_size; size++) {

        rap_memzero(test, size * sizeof(uint2));

        for (n = 0; n < nelts; n++) {
            if (names[n].key.data == NULL) {
                continue;
            }

            key = names[n].key_hash % size;
            len = test[key] + RAP_HASH_ELT_SIZE(&names[n]);

#if 0
            rap_log_error(RAP_LOG_ALERT, hinit->pool->log, 0,
                          "%ui: %ui %uz \"%V\"",
                          size, key, len, &names[n].key);
#endif

            if (len > bucket_size) {
                goto next;
            }

            test[key] = (uint2) len;
        }

        goto found;

    next:

        continue;
    }

    size = hinit->max_size;

    log_e("could not build optimal %s, you should increase "
                  "either %s_max_size: %i or %s_bucket_size: %i; "
                  "ignoring %s_bucket_size",
                  hinit->name, hinit->name, hinit->max_size,
                  hinit->name, hinit->bucket_size, hinit->name);

found:

    for (i = 0; i < size; i++) {
        test[i] = sizeof(void *);
    }

    for (n = 0; n < nelts; n++) {
        if (names[n].key.data == NULL) {
            continue;
        }

        key = names[n].key_hash % size;
        len = test[key] + RAP_HASH_ELT_SIZE(&names[n]);

        if (len > 65536 - rap_cacheline_size) {
            log_e("could not build %s, you should "
                          "increase %s_max_size: %i",
                          hinit->name, hinit->name, hinit->max_size);
            rap_free(test);
            return RAP_ERROR;
        }

        test[key] = (uint2) len;
    }

    len = 0;

    for (i = 0; i < size; i++) {
        if (test[i] == sizeof(void *)) {
            continue;
        }

        test[i] = (uint2) (rap_align(test[i], rap_cacheline_size));

        len += test[i];
    }

    if (hinit->hash == NULL) {
        hinit->hash = rap_pcalloc(hinit->pool, sizeof(rap_hash_wildcard_t)
                                             + size * sizeof(rap_hash_elt_t *));
        if (hinit->hash == NULL) {
            rap_free(test);
            return RAP_ERROR;
        }

        buckets = (rap_hash_elt_t **)
                      ((uchar *) hinit->hash + sizeof(rap_hash_wildcard_t));

    } else {
        buckets = rap_pcalloc(hinit->pool, size * sizeof(rap_hash_elt_t *));
        if (buckets == NULL) {
            rap_free(test);
            return RAP_ERROR;
        }
    }

    elts = rap_palloc(hinit->pool, len + rap_cacheline_size);
    if (elts == NULL) {
        rap_free(test);
        return RAP_ERROR;
    }

    elts = rap_align_ptr(elts, rap_cacheline_size);

    for (i = 0; i < size; i++) {
        if (test[i] == sizeof(void *)) {
            continue;
        }

        buckets[i] = (rap_hash_elt_t *) elts;
        elts += test[i];
    }

    for (i = 0; i < size; i++) {
        test[i] = 0;
    }

    for (n = 0; n < nelts; n++) {
        if (names[n].key.data == NULL) {
            continue;
        }

        key = names[n].key_hash % size;
        elt = (rap_hash_elt_t *) ((uchar *) buckets[key] + test[key]);

        elt->value = names[n].value;
        elt->len = (uint2) names[n].key.len;

        rap_strlow(elt->name, names[n].key.data, names[n].key.len);

        test[key] = (uint2) (test[key] + RAP_HASH_ELT_SIZE(&names[n]));
    }

    for (i = 0; i < size; i++) {
        if (buckets[i] == NULL) {
            continue;
        }

        elt = (rap_hash_elt_t *) ((uchar *) buckets[i] + test[i]);

        elt->value = NULL;
    }

    rap_free(test);

    hinit->hash->buckets = buckets;
    hinit->hash->size = size;

#if 0

    for (i = 0; i < size; i++) {
        rap_str_t   val;
        uint4  key;

        elt = buckets[i];

        if (elt == NULL) {
            rap_log_error(RAP_LOG_ALERT, hinit->pool->log, 0,
                          "%ui: NULL", i);
            continue;
        }

        while (elt->value) {
            val.len = elt->len;
            val.data = &elt->name[0];

            key = hinit->key(val.data, val.len);

            rap_log_error(RAP_LOG_ALERT, hinit->pool->log, 0,
                          "%ui: %p \"%V\" %ui", i, elt, &val, key);

            elt = (rap_hash_elt_t *) rap_align_ptr(&elt->name[0] + elt->len,
                                                   sizeof(void *));
        }
    }

#endif

    return RAP_OK;
}


int4
rap_hash_wildcard_init(rap_hash_init_t *hinit, rap_hash_key_t *names,
    uint4 nelts)
{
    size_t                len, dot_len;
    uint4            i, n, dot;
    rap_array_t           curr_names, next_names;
    rap_hash_key_t       *name, *next_name;
    rap_hash_init_t       h;
    rap_hash_wildcard_t  *wdc;

    if (rap_array_init(&curr_names, hinit->temp_pool, nelts,
                       sizeof(rap_hash_key_t))
        != RAP_OK)
    {
        return RAP_ERROR;
    }

    if (rap_array_init(&next_names, hinit->temp_pool, nelts,
                       sizeof(rap_hash_key_t))
        != RAP_OK)
    {
        return RAP_ERROR;
    }

    for (n = 0; n < nelts; n = i) {

#if 0
        rap_log_error(RAP_LOG_ALERT, hinit->pool->log, 0,
                      "wc0: \"%V\"", &names[n].key);
#endif

        dot = 0;

        for (len = 0; len < names[n].key.len; len++) {
            if (names[n].key.data[len] == '.') {
                dot = 1;
                break;
            }
        }

        name = rap_array_push(&curr_names);
        if (name == NULL) {
            return RAP_ERROR;
        }

        name->key.len = len;
        name->key.data = names[n].key.data;
        name->key_hash = hinit->key(name->key.data, name->key.len);
        name->value = names[n].value;

#if 0
        rap_log_error(RAP_LOG_ALERT, hinit->pool->log, 0,
                      "wc1: \"%V\" %ui", &name->key, dot);
#endif

        dot_len = len + 1;

        if (dot) {
            len++;
        }

        next_names.nelts = 0;

        if (names[n].key.len != len) {
            next_name = rap_array_push(&next_names);
            if (next_name == NULL) {
                return RAP_ERROR;
            }

            next_name->key.len = names[n].key.len - len;
            next_name->key.data = names[n].key.data + len;
            next_name->key_hash = 0;
            next_name->value = names[n].value;

#if 0
            rap_log_error(RAP_LOG_ALERT, hinit->pool->log, 0,
                          "wc2: \"%V\"", &next_name->key);
#endif
        }

        for (i = n + 1; i < nelts; i++) {
            if (rap_strncmp(names[n].key.data, names[i].key.data, len) != 0) {
                break;
            }

            if (!dot
                && names[i].key.len > len
                && names[i].key.data[len] != '.')
            {
                break;
            }

            next_name = rap_array_push(&next_names);
            if (next_name == NULL) {
                return RAP_ERROR;
            }

            next_name->key.len = names[i].key.len - dot_len;
            next_name->key.data = names[i].key.data + dot_len;
            next_name->key_hash = 0;
            next_name->value = names[i].value;

#if 0
            rap_log_error(RAP_LOG_ALERT, hinit->pool->log, 0,
                          "wc3: \"%V\"", &next_name->key);
#endif
        }

        if (next_names.nelts) {

            h = *hinit;
            h.hash = NULL;

            if (rap_hash_wildcard_init(&h, (rap_hash_key_t *) next_names.elts,
                                       next_names.nelts)
                != RAP_OK)
            {
                return RAP_ERROR;
            }

            wdc = (rap_hash_wildcard_t *) h.hash;

            if (names[n].key.len == len) {
                wdc->value = names[n].value;
            }

            name->value = (void *) ((uintptr_t) wdc | (dot ? 3 : 2));

        } else if (dot) {
            name->value = (void *) ((uintptr_t) name->value | 1);
        }
    }

    if (rap_hash_init(hinit, (rap_hash_key_t *) curr_names.elts,
                      curr_names.nelts)
        != RAP_OK)
    {
        return RAP_ERROR;
    }

    return RAP_OK;
}


uint4
rap_hash_key(uchar *data, size_t len)
{
    uint4  i, key;

    key = 0;

    for (i = 0; i < len; i++) {
        key = rap_hash(key, data[i]);
    }

    return key;
}


uint4
rap_hash_key_lc(uchar *data, size_t len)
{
    uint4  i, key;

    key = 0;

    for (i = 0; i < len; i++) {
        key = rap_hash(key, rap_tolower(data[i]));
    }

    return key;
}


uint4
rap_hash_strlow(uchar *dst, uchar *src, size_t n)
{
    uint4  key;

    key = 0;

    while (n--) {
        *dst = rap_tolower(*src);
        key = rap_hash(key, *dst);
        dst++;
        src++;
    }

    return key;
}


int4
rap_hash_keys_array_init(rap_hash_keys_arrays_t *ha, uint4 type)
{
    uint4  asize;

    if (type == RAP_HASH_SMALL) {
        asize = 4;
        ha->hsize = 107;

    } else {
        asize = RAP_HASH_LARGE_ASIZE;
        ha->hsize = RAP_HASH_LARGE_HSIZE;
    }

    if (rap_array_init(&ha->keys, ha->temp_pool, asize, sizeof(rap_hash_key_t))
        != RAP_OK)
    {
        return RAP_ERROR;
    }

    if (rap_array_init(&ha->dns_wc_head, ha->temp_pool, asize,
                       sizeof(rap_hash_key_t))
        != RAP_OK)
    {
        return RAP_ERROR;
    }

    if (rap_array_init(&ha->dns_wc_tail, ha->temp_pool, asize,
                       sizeof(rap_hash_key_t))
        != RAP_OK)
    {
        return RAP_ERROR;
    }

    ha->keys_hash = rap_pcalloc(ha->temp_pool, sizeof(rap_array_t) * ha->hsize);
    if (ha->keys_hash == NULL) {
        return RAP_ERROR;
    }

    ha->dns_wc_head_hash = rap_pcalloc(ha->temp_pool,
                                       sizeof(rap_array_t) * ha->hsize);
    if (ha->dns_wc_head_hash == NULL) {
        return RAP_ERROR;
    }

    ha->dns_wc_tail_hash = rap_pcalloc(ha->temp_pool,
                                       sizeof(rap_array_t) * ha->hsize);
    if (ha->dns_wc_tail_hash == NULL) {
        return RAP_ERROR;
    }

    return RAP_OK;
}


int4
rap_hash_add_key(rap_hash_keys_arrays_t *ha, rap_str_t *key, void *value,
    uint4 flags)
{
    size_t           len;
    uchar          *p;
    rap_str_t       *name;
    uint4       i, k, n, skip, last;
    rap_array_t     *keys, *hwc;
    rap_hash_key_t  *hk;

    last = key->len;

    if (flags & RAP_HASH_WILDCARD_KEY) {

        /*
         * supported wildcards:
         *     "*.example.com", ".example.com", and "www.example.*"
         */

        n = 0;

        for (i = 0; i < key->len; i++) {

            if (key->data[i] == '*') {
                if (++n > 1) {
                    return RAP_DECLINED;
                }
            }

            if (key->data[i] == '.' && key->data[i + 1] == '.') {
                return RAP_DECLINED;
            }

            if (key->data[i] == '\0') {
                return RAP_DECLINED;
            }
        }

        if (key->len > 1 && key->data[0] == '.') {
            skip = 1;
            goto wildcard;
        }

        if (key->len > 2) {

            if (key->data[0] == '*' && key->data[1] == '.') {
                skip = 2;
                goto wildcard;
            }

            if (key->data[i - 2] == '.' && key->data[i - 1] == '*') {
                skip = 0;
                last -= 2;
                goto wildcard;
            }
        }

        if (n) {
            return RAP_DECLINED;
        }
    }

    /* exact hash */

    k = 0;

    for (i = 0; i < last; i++) {
        if (!(flags & RAP_HASH_READONLY_KEY)) {
            key->data[i] = rap_tolower(key->data[i]);
        }
        k = rap_hash(k, key->data[i]);
    }

    k %= ha->hsize;

    /* check conflicts in exact hash */

    name = ha->keys_hash[k].elts;

    if (name) {
        for (i = 0; i < ha->keys_hash[k].nelts; i++) {
            if (last != name[i].len) {
                continue;
            }

            if (rap_strncmp(key->data, name[i].data, last) == 0) {
                return RAP_BUSY;
            }
        }

    } else {
        if (rap_array_init(&ha->keys_hash[k], ha->temp_pool, 4,
                           sizeof(rap_str_t))
            != RAP_OK)
        {
            return RAP_ERROR;
        }
    }

    name = rap_array_push(&ha->keys_hash[k]);
    if (name == NULL) {
        return RAP_ERROR;
    }

    *name = *key;

    hk = rap_array_push(&ha->keys);
    if (hk == NULL) {
        return RAP_ERROR;
    }

    hk->key = *key;
    hk->key_hash = rap_hash_key(key->data, last);
    hk->value = value;

    return RAP_OK;


wildcard:

    /* wildcard hash */

    k = rap_hash_strlow(&key->data[skip], &key->data[skip], last - skip);

    k %= ha->hsize;

    if (skip == 1) {

        /* check conflicts in exact hash for ".example.com" */

        name = ha->keys_hash[k].elts;

        if (name) {
            len = last - skip;

            for (i = 0; i < ha->keys_hash[k].nelts; i++) {
                if (len != name[i].len) {
                    continue;
                }

                if (rap_strncmp(&key->data[1], name[i].data, len) == 0) {
                    return RAP_BUSY;
                }
            }

        } else {
            if (rap_array_init(&ha->keys_hash[k], ha->temp_pool, 4,
                               sizeof(rap_str_t))
                != RAP_OK)
            {
                return RAP_ERROR;
            }
        }

        name = rap_array_push(&ha->keys_hash[k]);
        if (name == NULL) {
            return RAP_ERROR;
        }

        name->len = last - 1;
        name->data = rap_pnalloc(ha->temp_pool, name->len);
        if (name->data == NULL) {
            return RAP_ERROR;
        }

        rap_memcpy(name->data, &key->data[1], name->len);
    }


    if (skip) {

        /*
         * convert "*.example.com" to "com.example.\0"
         *      and ".example.com" to "com.example\0"
         */

        p = rap_pnalloc(ha->temp_pool, last);
        if (p == NULL) {
            return RAP_ERROR;
        }

        len = 0;
        n = 0;

        for (i = last - 1; i; i--) {
            if (key->data[i] == '.') {
                rap_memcpy(&p[n], &key->data[i + 1], len);
                n += len;
                p[n++] = '.';
                len = 0;
                continue;
            }

            len++;
        }

        if (len) {
            rap_memcpy(&p[n], &key->data[1], len);
            n += len;
        }

        p[n] = '\0';

        hwc = &ha->dns_wc_head;
        keys = &ha->dns_wc_head_hash[k];

    } else {

        /* convert "www.example.*" to "www.example\0" */

        last++;

        p = rap_pnalloc(ha->temp_pool, last);
        if (p == NULL) {
            return RAP_ERROR;
        }

        rap_cpystrn(p, key->data, last);

        hwc = &ha->dns_wc_tail;
        keys = &ha->dns_wc_tail_hash[k];
    }


    /* check conflicts in wildcard hash */

    name = keys->elts;

    if (name) {
        len = last - skip;

        for (i = 0; i < keys->nelts; i++) {
            if (len != name[i].len) {
                continue;
            }

            if (rap_strncmp(key->data + skip, name[i].data, len) == 0) {
                return RAP_BUSY;
            }
        }

    } else {
        if (rap_array_init(keys, ha->temp_pool, 4, sizeof(rap_str_t)) != RAP_OK)
        {
            return RAP_ERROR;
        }
    }

    name = rap_array_push(keys);
    if (name == NULL) {
        return RAP_ERROR;
    }

    name->len = last - skip;
    name->data = rap_pnalloc(ha->temp_pool, name->len);
    if (name->data == NULL) {
        return RAP_ERROR;
    }

    rap_memcpy(name->data, key->data + skip, name->len);


    /* add to wildcard hash */

    hk = rap_array_push(hwc);
    if (hk == NULL) {
        return RAP_ERROR;
    }

    hk->key.len = last - 1;
    hk->key.data = p;
    hk->key_hash = 0;
    hk->value = value;

    return RAP_OK;
}
