
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Rap, Inc.
 */



#include <rap_array.h>


#define rap_memcpy(dst, src, n)   (void) memcpy(dst, src, n)

rap_array_t *
rap_array_create(rap_pool_t *p, uint4 n, size_t size)
{
    rap_array_t *a;

    a = rap_palloc(p, sizeof(rap_array_t));
    if (a == NULL) {
        return NULL;
    }

    if (rap_array_init(a, p, n, size) != RAP_OK) {
        return NULL;
    }

    return a;
}


void
rap_array_destroy(rap_array_t *a)
{
    rap_pool_t  *p;

    p = a->pool;

    if ((uchar *) a->elts + a->size * a->nalloc == p->d.last) {
        p->d.last -= a->size * a->nalloc;
    }

    if ((uchar *) a + sizeof(rap_array_t) == p->d.last) {
        p->d.last = (uchar *) a;
    }
}


void *
rap_array_push(rap_array_t *a)
{
    void        *elt, *new;
    size_t       size;
    rap_pool_t  *p;

    if (a->nelts == a->nalloc) {

        /* the array is full */

        size = a->size * a->nalloc;

        p = a->pool;

        if ((uchar *) a->elts + size == p->d.last
            && p->d.last + a->size <= p->d.end)
        {
            /*
             * the array allocation is the last in the pool
             * and there is space for new allocation
             */

            p->d.last += a->size;
            a->nalloc++;

        } else {
            /* allocate a new array */

            new = rap_palloc(p, 2 * size);
            if (new == NULL) {
                return NULL;
            }

            rap_memcpy(new, a->elts, size);
            a->elts = new;
            a->nalloc *= 2;
        }
    }

    elt = (uchar *) a->elts + a->size * a->nelts;
    a->nelts++;

    return elt;
}


void *
rap_array_push_n(rap_array_t *a, uint4 n)
{
    void        *elt, *new;
    size_t       size;
    uint4   nalloc;
    rap_pool_t  *p;

    size = n * a->size;

    if (a->nelts + n > a->nalloc) {

        /* the array is full */

        p = a->pool;

        if ((uchar *) a->elts + a->size * a->nalloc == p->d.last
            && p->d.last + size <= p->d.end)
        {
            /*
             * the array allocation is the last in the pool
             * and there is space for new allocation
             */

            p->d.last += size;
            a->nalloc += n;

        } else {
            /* allocate a new array */

            nalloc = 2 * ((n >= a->nalloc) ? n : a->nalloc);

            new = rap_palloc(p, nalloc * a->size);
            if (new == NULL) {
                return NULL;
            }

            rap_memcpy(new, a->elts, a->nelts * a->size);
            a->elts = new;
            a->nalloc = nalloc;
        }
    }

    elt = (uchar *) a->elts + a->size * a->nelts;
    a->nelts += n;

    return elt;
}
