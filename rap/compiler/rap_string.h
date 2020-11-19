#ifndef _RAP_STRING_H_INCLUDED_
#define _RAP_STRING_H_INCLUDED_


#include <deed.h>
#include <rap_palloc.h>
#include <rap_rbtree.h>

typedef rap_rbtree_key_t      rap_msec_t;
typedef rap_rbtree_key_int_t  rap_msec_int_t;


typedef struct {
    size_t      len;
    uchar     *data;
} rap_str_t;


typedef struct {
    rap_str_t   key;
    rap_str_t   value;
} rap_keyval_t;


typedef struct {
    unsigned    len:28;

    unsigned    valid:1;
    unsigned    no_cacheable:1;
    unsigned    not_found:1;
    unsigned    escape:1;

    uchar     *data;
} rap_variable_value_t;


#define rap_string(str)     { sizeof(str) - 1, (uchar *) str }
#define rap_null_string     { 0, NULL }
#define rap_str_set(str, text)                                               \
    (str)->len = sizeof(text) - 1; (str)->data = (uchar *) text
#define rap_str_null(str)   (str)->len = 0; (str)->data = NULL


#define rap_tolower(c)      (uchar) ((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)
#define rap_toupper(c)      (uchar) ((c >= 'a' && c <= 'z') ? (c & ~0x20) : c)

void rap_strlow(uchar *dst, uchar *src, size_t n);


#define rap_strncmp(s1, s2, n)  strncmp((const char *) s1, (const char *) s2, n)


/* msvc and icc7 compile strcmp() to inline loop */
#define rap_strcmp(s1, s2)  strcmp((const char *) s1, (const char *) s2)


#define rap_strstr(s1, s2)  strstr((const char *) s1, (const char *) s2)
#define rap_strlen(s)       strlen((const char *) s)

size_t rap_strnlen(uchar *p, size_t n);

#define rap_strchr(s1, c)   strchr((const char *) s1, (int) c)

static inline uchar *
rap_strlchr(uchar *p, uchar *last, uchar c)
{
    while (p < last) {

        if (*p == c) {
            return p;
        }

        p++;
    }

    return NULL;
}


/*
 * msvc and icc7 compile memset() to the inline "rep stos"
 * while ZeroMemory() and bzero() are the calls.
 * icc7 may also inline several mov's of a zeroed register for small blocks.
 */
#define rap_memzero(buf, n)       (void) memset(buf, 0, n)
#define rap_memset(buf, c, n)     (void) memset(buf, c, n)

void rap_explicit_memzero(void *buf, size_t n);


#if (RAP_MEMCPY_LIMIT)

void *rap_memcpy(void *dst, const void *src, size_t n);
#define rap_cpymem(dst, src, n)   (((uchar *) rap_memcpy(dst, src, n)) + (n))

#else

/*
 * gcc3, msvc, and icc7 compile memcpy() to the inline "rep movs".
 * gcc3 compiles memcpy(d, s, 4) to the inline "mov"es.
 * icc8 compile memcpy(d, s, 4) to the inline "mov"es or XMM moves.
 */
#define rap_memcpy(dst, src, n)   (void) memcpy(dst, src, n)
#define rap_cpymem(dst, src, n)   (((uchar *) memcpy(dst, src, n)) + (n))

#endif


#if ( __INTEL_COMPILER >= 800 )

/*
 * the simple inline cycle copies the variable length strings up to 16
 * bytes faster than icc8 autodetecting _intel_fast_memcpy()
 */

static inline uchar *
rap_copy(uchar *dst, uchar *src, size_t len)
{
    if (len < 17) {

        while (len) {
            *dst++ = *src++;
            len--;
        }

        return dst;

    } else {
        return rap_cpymem(dst, src, len);
    }
}

#else

#define rap_copy                  rap_cpymem

#endif


#define rap_memmove(dst, src, n)   (void) memmove(dst, src, n)
#define rap_movemem(dst, src, n)   (((uchar *) memmove(dst, src, n)) + (n))


/* msvc and icc7 compile memcmp() to the inline loop */
#define rap_memcmp(s1, s2, n)  memcmp((const char *) s1, (const char *) s2, n)

uchar *rap_cpystrn(uchar *dst, uchar *src, size_t n);
uchar *rap_pstrdup(rap_pool_t *pool, rap_str_t *src);
uchar *rap_sprintf(uchar *buf, const char *fmt, ...);
uchar *rap_snprintf(uchar *buf, size_t max, const char *fmt, ...);
uchar *rap_slprintf(uchar *buf, uchar *last, const char *fmt,...);
uchar *rap_vslprintf(uchar *buf, uchar *last, const char *fmt, va_list args);
#define rap_vsnprintf(buf, max, fmt, args)                                   \
rap_vslprintf(buf, buf + (max), fmt, args)

int4 rap_strcasecmp(uchar *s1, uchar *s2);
int4 rap_strncasecmp(uchar *s1, uchar *s2, size_t n);

uchar *rap_strnstr(uchar *s1, char *s2, size_t n);

uchar *rap_strstrn(uchar *s1, char *s2, size_t n);
uchar *rap_strcasestrn(uchar *s1, char *s2, size_t n);
uchar *rap_strlcasestrn(uchar *s1, uchar *last, uchar *s2, size_t n);

int4 rap_rstrncmp(uchar *s1, uchar *s2, size_t n);
int4 rap_rstrncasecmp(uchar *s1, uchar *s2, size_t n);
int4 rap_memn2cmp(uchar *s1, uchar *s2, size_t n1, size_t n2);
int4 rap_dns_strcmp(uchar *s1, uchar *s2);
int4 rap_filename_cmp(uchar *s1, uchar *s2, size_t n);

int4 rap_atoi(uchar *line, size_t n);
int4 rap_atofp(uchar *line, size_t n, size_t point);
int4 rap_hextoi(uchar *line, size_t n);

uchar *rap_hex_dump(uchar *dst, uchar *src, size_t len);


#define rap_base64_encoded_length(len)  (((len + 2) / 3) * 4)
#define rap_base64_decoded_length(len)  (((len + 3) / 4) * 3)

void rap_encode_base64(rap_str_t *dst, rap_str_t *src);
void rap_encode_base64url(rap_str_t *dst, rap_str_t *src);
int4 rap_decode_base64(rap_str_t *dst, rap_str_t *src);
int4 rap_decode_base64url(rap_str_t *dst, rap_str_t *src);

uint32_t rap_utf8_decode(uchar **p, size_t n);
size_t rap_utf8_length(uchar *p, size_t n);
uchar *rap_utf8_cpystrn(uchar *dst, uchar *src, size_t n, size_t len);


#define RAP_ESCAPE_URI            0
#define RAP_ESCAPE_ARGS           1
#define RAP_ESCAPE_URI_COMPONENT  2
#define RAP_ESCAPE_HTML           3
#define RAP_ESCAPE_REFRESH        4
#define RAP_ESCAPE_MEMCACHED      5
#define RAP_ESCAPE_MAIL_AUTH      6

#define RAP_UNESCAPE_URI       1
#define RAP_UNESCAPE_REDIRECT  2

uintptr_t rap_escape_uri(uchar *dst, uchar *src, size_t size,
    uint4 type);
void rap_unescape_uri(uchar **dst, uchar **src, size_t size, uint4 type);
uintptr_t rap_escape_html(uchar *dst, uchar *src, size_t size);
uintptr_t rap_escape_json(uchar *dst, uchar *src, size_t size);


typedef struct {
    rap_rbtree_node_t         node;
    rap_str_t                 str;
} rap_str_node_t;



void rap_str_rbtree_insert_value(rap_rbtree_node_t *temp,
    rap_rbtree_node_t *node, rap_rbtree_node_t *sentinel);

rap_str_node_t *rap_str_rbtree_lookup(rap_rbtree_t *rbtree, rap_str_t *name,
    uint32_t hash);


void rap_sort(void *base, size_t n, size_t size,
    int4 (*cmp)(const void *, const void *));
#define rap_qsort             qsort


#define rap_value_helper(n)   #n
#define rap_value(n)          rap_value_helper(n)


#endif /* _RAP_STRING_H_INCLUDED_ */
