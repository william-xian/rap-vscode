
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Rap, Inc.
 */


#ifndef _RAP_ARRAY_H_INCLUDED_
#define _RAP_ARRAY_H_INCLUDED_

#include <deed.h>
#include <rap_palloc.h>


typedef struct {
    void        *elts;
    uint4   nelts;
    size_t       size;
    uint4   nalloc;
    rap_pool_t  *pool;
} rap_array_t;


rap_array_t *rap_array_create(rap_pool_t *p, uint4 n, size_t size);
void rap_array_destroy(rap_array_t *a);
void *rap_array_push(rap_array_t *a);
void *rap_array_push_n(rap_array_t *a, uint4 n);


static inline int4
rap_array_init(rap_array_t *array, rap_pool_t *pool, uint4 n, size_t size)
{
    /*
     * set "array->nelts" before "array->elts", otherwise MSVC thinks
     * that "array->nelts" may be used without having been initialized
     */

    array->nelts = 0;
    array->size = size;
    array->nalloc = n;
    array->pool = pool;

    array->elts = rap_palloc(pool, n * size);
    if (array->elts == NULL) {
        return RAP_ERROR;
    }

    return RAP_OK;
}

#endif /* _RAP_ARRAY_H_INCLUDED_ */
