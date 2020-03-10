#ifndef _APM_INTERNAL_H_
#define _APM_INTERNAL_H_

#ifndef APM_DIGIT_SIZE
#if defined(__LP64__) || defined(__x86_64__) || defined(__amd64__) || \
    defined(__aarch64__)
#define APM_DIGIT_SIZE 8
#else
#define APM_DIGIT_SIZE 4
#endif
#endif

/* Tunable parameters: Karatsuba multiplication and squaring cutoff. */
#define KARATSUBA_MUL_THRESHOLD 32
#define KARATSUBA_SQR_THRESHOLD 64

#if APM_DIGIT_SIZE == 4
#if defined(i386) || defined(__i386__)
#define digit_mul(u, v, hi, lo) \
    __asm__("mull %3" : "=a"(lo), "=d"(hi) : "%0"(u), "g"(v) : "cc")
#define digit_div(n1, n0, d, q, r) \
    __asm__("divl %4" : "=a"(q), "=d"(r) : "0"(n0), "1"(n1), "g"(d))
#endif
#elif APM_DIGIT_SIZE == 8
#if defined(__amd64__) || defined(__x86_64__)
#define digit_mul(u, v, hi, lo) \
    __asm__("mulq %3" : "=a"(lo), "=d"(hi) : "%0"(u), "rm"(v))
#define digit_div(n1, n0, d, q, r) \
    __asm__("divq %4" : "=a"(q), "=d"(r) : "0"(n0), "1"(n1), "rm"(d))
#endif
#endif

#include "memory.h"

#define APM_TMP_ALLOC(size) apm_new(size)
#define APM_TMP_FREE(num) apm_free(num)
#define APM_TMP_COPY(num, size) \
    memcpy(APM_TMP_ALLOC(size), (num), (size) *APM_DIGIT_SIZE)

/* If we have no inline assembly versions of these primitive operations,
 * fall back onto the generic one.
 */
#ifdef digit_mul
#define digit_sqr(u, hi, lo) digit_mul((u), (u), (hi), (lo))
#else
#define digit_sqr(u, hi, lo)                    \
    do {                                        \
        apm_digit __u0, __u1, __hi, __lo;       \
        __u0 = (u);                             \
        __u1 = __u0 >> APM_DIGIT_HSHIFT;        \
        __u0 &= APM_DIGIT_LMASK;                \
        __lo = __u0 * __u0;                     \
        __hi = __u1 * __u1;                     \
        __u1 *= __u0;                           \
        __u0 = __u1 << (APM_DIGIT_HSHIFT + 1);  \
        __u1 >>= (APM_DIGIT_HSHIFT - 1);        \
        __hi += __u1 + ((__lo += __u0) < __u0); \
        (lo) = __lo;                            \
        (hi) = __hi;                            \
    } while (0)
#endif

#ifndef digit_mul
#define digit_mul(u, v, hi, lo)                    \
    do {                                           \
        apm_digit __u0 = (u);                      \
        apm_digit __u1 = __u0 >> APM_DIGIT_HSHIFT; \
        __u0 &= APM_DIGIT_LMASK;                   \
        apm_digit __v0 = (v);                      \
        apm_digit __v1 = __v0 >> APM_DIGIT_HSHIFT; \
        __v0 &= APM_DIGIT_LMASK;                   \
        apm_digit __lo = __u0 * __v0;              \
        apm_digit __hi = __u1 * __v1;              \
        __u1 *= __v0;                              \
        __v0 = __u1 << APM_DIGIT_HSHIFT;           \
        __u1 >>= APM_DIGIT_HSHIFT;                 \
        __hi += __u1 + ((__lo += __v0) < __v0);    \
        __v1 *= __u0;                              \
        __u0 = __v1 << APM_DIGIT_HSHIFT;           \
        __v1 >>= APM_DIGIT_HSHIFT;                 \
        __hi += __v1 + ((__lo += __u0) < __u0);    \
        (lo) = __lo;                               \
        (hi) = __hi;                               \
    } while (0)
#endif

#ifndef digit_div
#if APM_DIGIT_SIZE == 4
#define digit_div(n1, n0, d, q, r)                              \
    do {                                                        \
        uint64_t __d = (d);                                     \
        uint64_t __n = ((uint64_t)(n1) << 32) | (uint64_t)(n0); \
        (q) = (apm_digit)(__n / __d);                           \
        (r) = (apm_digit)(__n % __d);                           \
    } while (0)
#elif APM_DIGIT_SIZE == 8
/* GCC has builtin support for the types __int128. */
typedef unsigned __int128 uint128_t;
#define digit_div(n1, n0, d, q, r)                                 \
    do {                                                           \
        uint128_t __d = (d);                                       \
        uint128_t __n = ((uint128_t)(n1) << 64) | (uint128_t)(n0); \
        (q) = (apm_digit)(__n / __d);                              \
        (r) = (apm_digit)(__n % __d);                              \
    } while (0)
#else
#error "Unsupported platform"
#endif
#endif

#ifndef SWAP
#define SWAP(a, b, type)  \
    do {                  \
        type __tmp = (a); \
        (a) = (b);        \
        (b) = __tmp;      \
    } while (0)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect((x), 0)
#endif

/* The expression always executes, regardless of whether NDEBUG is defined
 * or not. This is intentional and sometimes useful.
 */
#ifndef NDEBUG
#include <stdio.h>
#include <stdlib.h>
#define ASSERT(expr)                                                           \
    do {                                                                       \
        if (!unlikely(expr)) {                                                 \
            fprintf(stderr, "%s:%d (%s) assertion failed: \"%s\"\n", __FILE__, \
                    __LINE__, __PRETTY_FUNCTION__, #expr);                     \
            abort();                                                           \
        }                                                                      \
    } while (0)
#else
#define ASSERT(expr) (void) (expr)
#endif

#endif /* !_APM_INTERNAL_H_ */
