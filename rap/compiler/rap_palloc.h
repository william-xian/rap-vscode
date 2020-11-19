
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Rap, Inc.
 */


#ifndef _RAP_PALLOC_H_INCLUDED_
#define _RAP_PALLOC_H_INCLUDED_


#include <deed.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>


#define rap_pagesize  8192
#define rap_cacheline_size  32
#ifndef RAP_ALIGNMENT
#define RAP_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#endif

#define RAP_INT32_LEN   (sizeof("-2147483648") - 1)
#define RAP_INT64_LEN   (sizeof("-9223372036854775808") - 1)


#define RAP_INT32_LEN   (sizeof("-2147483648") - 1)
#define RAP_INT64_LEN   (sizeof("-9223372036854775808") - 1)

#if (RAP_PTR_SIZE == 4)
#define RAP_INT_T_LEN   RAP_INT32_LEN
#define RAP_MAX_INT_T_VALUE  2147483647

#else
#define RAP_INT_T_LEN   RAP_INT64_LEN
#define RAP_MAX_INT_T_VALUE  0x7fffffff
#endif


#ifndef RAP_ALIGNMENT
#define RAP_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#endif

#define rap_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define rap_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))


#define RAP_MAX_UINT32_VALUE  (uint32_t) 0xffffffff
#define RAP_MAX_INT32_VALUE   (uint32_t) 0x7fffffff

#define rap_alloc(size)       malloc(size)
#define rap_free(p)       free(p)
#define rap_memory_barrier()    __asm__ volatile ("" ::: "memory")
#define rap_memzero(buf, n)       (void) memset(buf, 0, n)
#define rap_memset(buf, c, n)     (void) memset(buf, c, n)
#define rap_memalign(alignment, size)  rap_alloc(size)


#define rap_close_file(fd)           close(fd)
#define rap_close_file_n         "close()"

#define rap_delete_file(name)    unlink((const char *) name)
#define rap_delete_file_n        "unlink()"


#define LF     (u_char) '\n'
#define CR     (u_char) '\r'
#define CRLF   "\r\n"


#define rap_abs(value)       (((value) >= 0) ? (value) : - (value))
#define rap_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define rap_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))

#define  RAP_OK          0
#define  RAP_ERROR      -1
#define  RAP_AGAIN      -2
#define  RAP_BUSY       -3
#define  RAP_DONE       -4
#define  RAP_DECLINED   -5
#define  RAP_ABORT      -6
#define RAP_INVALID_FILE         -1
#define RAP_FILE_ERROR           -1

/*
 * RAP_MAX_ALLOC_FROM_POOL should be (rap_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define RAP_MAX_ALLOC_FROM_POOL  (rap_pagesize - 1)

#define RAP_DEFAULT_POOL_SIZE    (16 * 1024)


#define RAP_POOL_ALIGNMENT       16
#define RAP_MIN_POOL_SIZE                                                     \
    rap_align((sizeof(rap_pool_t) + 2 * sizeof(rap_pool_large_t)),            \
              RAP_POOL_ALIGNMENT)


typedef void (*rap_pool_cleanup_pt)(void *data);

typedef struct rap_pool_cleanup_s  rap_pool_cleanup_t;

struct rap_pool_cleanup_s {
    rap_pool_cleanup_pt   handler;
    void                 *data;
    rap_pool_cleanup_t   *next;
};


typedef struct rap_pool_large_s  rap_pool_large_t;

struct rap_pool_large_s {
    rap_pool_large_t     *next;
    void                 *alloc;
};

typedef struct rap_pool_s rap_pool_t;

typedef struct {
    uchar               *last;
    uchar               *end;
    rap_pool_t           *next;
    uint4            failed;
} rap_pool_data_t;


typedef struct rap_pool_s {
    rap_pool_data_t       d;
    size_t                max;
    rap_pool_t           *current;
    rap_pool_large_t     *large;
    rap_pool_cleanup_t   *cleanup;
} rap_pool_t;


typedef struct {
    int4            fd;
    uchar            *name;
} rap_pool_cleanup_file_t;


rap_pool_t *rap_create_pool(size_t size);
void rap_destroy_pool(rap_pool_t *pool);
void rap_reset_pool(rap_pool_t *pool);

void *rap_palloc(rap_pool_t *pool, size_t size);
void *rap_pnalloc(rap_pool_t *pool, size_t size);
void *rap_pcalloc(rap_pool_t *pool, size_t size);
void *rap_pmemalign(rap_pool_t *pool, size_t size, size_t alignment);
int4 rap_pfree(rap_pool_t *pool, void *p);


rap_pool_cleanup_t *rap_pool_cleanup_add(rap_pool_t *p, size_t size);
void rap_pool_run_cleanup_file(rap_pool_t *p, int4 fd);
void rap_pool_cleanup_file(void *data);
void rap_pool_delete_file(void *data);


#endif /* _RAP_PALLOC_H_INCLUDED_ */
