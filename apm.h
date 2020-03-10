/* Low-level layer for Arbitrary-Precision arithmetic (APM) */

#ifndef _APM_H_
#define _APM_H_

#include <stdint.h>
#include <stdio.h>  /* for FILE*, stdout */
#include <string.h> /* for memmove */

#include "apm_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

#if APM_DIGIT_SIZE == 4
typedef uint32_t apm_digit;
#define APM_DIGIT_HMASK UINT32_C(0xffff0000)
#define APM_DIGIT_LMASK UINT32_C(0x0000ffff)
#define APM_DIGIT_BITS 32U
#define APM_DIGIT_HSHIFT 16U
#define APM_DIGIT_MAX UINT32_MAX

#elif APM_DIGIT_SIZE == 8
typedef uint64_t apm_digit;
#define APM_DIGIT_HMASK UINT64_C(0xffffffff00000000)
#define APM_DIGIT_LMASK UINT64_C(0x00000000ffffffff)
#define APM_DIGIT_BITS 64U
#define APM_DIGIT_HSHIFT 32U
#define APM_DIGIT_MAX UINT64_MAX

#else
#error "APM_DIGIT_SIZE must be 4 or 8"
#endif

typedef uint32_t apm_size;

/* Set u[size] to zero. */
#define apm_zero(u, size) memset((u), 0, APM_DIGIT_SIZE *(size))

/* Copy u[size] onto v[size]. */
#define apm_copy(u, size, v) memmove((v), (u), APM_DIGIT_SIZE *(size))

/* Allocate an uninitialized size-digit number. */
static inline apm_digit *apm_new(apm_size size)
{
    ASSERT(size != 0);
    return MALLOC(size * APM_DIGIT_SIZE);
}

/* Allocate a zeroed out size-digit number. */
static inline apm_digit *apm_new0(apm_size size)
{
    ASSERT(size != 0);
    return apm_zero(apm_new(size), size);
}

/* Resize the number U to size digits. */
static inline apm_digit *apm_resize(apm_digit *u, apm_size size)
{
    if (u)
        return REALLOC(u, size * APM_DIGIT_SIZE);
    return apm_new(size);
}

/* Release the number U. */
static inline void apm_free(apm_digit *u)
{
    FREE(u);
}

/* Return real size of u[size] with leading zeros removed. */
static inline apm_size apm_rsize(const apm_digit *u, apm_size size)
{
    u += size;
    while (size && !*--u)
        --size;
    return size;
}

/* Compare equally sized U and V. */
int apm_cmp_n(const apm_digit *u, const apm_digit *v, apm_size size);
/* Compare u[usize] to v[vsize]; -1 if u < v, 0 if u == v, and +1 if u > v; */
int apm_cmp(const apm_digit *u,
            apm_size usize,
            const apm_digit *v,
            apm_size vsize);

/* Return the number of shift positions that U must be shifted right until its
 * least significant bit is set. Argument MUST be non-zero. */
#if APM_DIGIT_SIZE == 4
#define apm_digit_lsb_shift(u) __builtin_ctz(u)
#elif APM_DIGIT_SIZE == 8
#define apm_digit_lsb_shift(u) __builtin_ctzll(u)
#endif

/* Set u[size] = u[size] + v and return the carry. */
apm_digit apm_daddi(apm_digit *u, apm_size size, apm_digit v);

/* Set w[size] = u[size] + v[size] and return the carry. */
apm_digit apm_add_n(const apm_digit *u,
                    const apm_digit *v,
                    apm_size size,
                    apm_digit *w);
/* Set w[max(usize, vsize)] = u[usize] + v[vsize] and return the carry. */
apm_digit apm_add(const apm_digit *u,
                  apm_size usize,
                  const apm_digit *v,
                  apm_size vsize,
                  apm_digit *w);

/* Set u[size] = u[size] + v[size] and return the carry. */
/* apm_digit	apm_addi_n(apm_digit *u, const apm_digit *v, apm_size size); */
#define apm_addi_n(u, v, size) apm_add_n(u, v, size, u)
/* Set u[usize] = u[usize] + v[vsize], usize >= vsize, and return the carry. */
apm_digit apm_addi(apm_digit *u,
                   apm_size usize,
                   const apm_digit *v,
                   apm_size vsize);

/* Set u[size] = u[size] - v[size] and return the borrow. */
apm_digit apm_subi_n(apm_digit *u, const apm_digit *v, apm_size size);
/* Set w[size] = u[size] - v[size], and return the borrow. */
apm_digit apm_sub_n(const apm_digit *u,
                    const apm_digit *v,
                    apm_size size,
                    apm_digit *w);

/* Set u[usize] = u[usize] - v[vsize], usize >= vsize, and return the borrow. */
apm_digit apm_subi(apm_digit *u,
                   apm_size usize,
                   const apm_digit *v,
                   apm_size vsize);
/* Set w[usize] = u[usize] - v[vsize], and return the borrow. */
apm_digit apm_sub(const apm_digit *u,
                  apm_size usize,
                  const apm_digit *v,
                  apm_size vsize,
                  apm_digit *w);

/* Set w[size] = u[size] * v, and return the carry. */
apm_digit apm_dmul(const apm_digit *u,
                   apm_size size,
                   apm_digit v,
                   apm_digit *w);
/* Set w[size] = w[size] + u[size] * v, and return the carry. */
apm_digit apm_dmul_add(const apm_digit *u,
                       apm_size size,
                       apm_digit v,
                       apm_digit *w);

/* Set w[usize + vsize] = u[usize] * v[vsize]. */
void apm_mul(const apm_digit *u,
             apm_size usize,
             const apm_digit *v,
             apm_size vsize,
             apm_digit *w);

/* Set v[usize*2] = u[usize]^2. */
void apm_sqr(const apm_digit *u, apm_size usize, apm_digit *v);

/* Multiply or divide by a power of two, with power taken modulo APM_DIGIT_BITS,
 * and return the carry (left shift) or remainder (right shift). */
apm_digit apm_lshift(const apm_digit *u,
                     apm_size size,
                     unsigned int shift,
                     apm_digit *v);
apm_digit apm_lshifti(apm_digit *u, apm_size size, unsigned int shift);
apm_digit apm_rshifti(apm_digit *u, apm_size size, unsigned int shift);

/* Print u[size] in a radix on [2,36] to the stream fp. No newline is output. */
void apm_fprint(const apm_digit *u, apm_size size, unsigned int radix, FILE *fp);
/* Convenience macros for bases 2, 10, and 16, with fp = stdout. */
#define apm_print(u, size, b) apm_fprint((u), (size), (b), stdout)
#define apm_print_dec(u, size) apm_fprint_dec((u), (size), stdout)
#define apm_print_hex(u, size) apm_fprint_hex((u), (size), stdout)

#define APM_NORMALIZE(u, usize)         \
    while ((usize) && !(u)[(usize) -1]) \
        --(usize);

#ifdef __cplusplus
}
#endif

#endif /* !_APM_H_ */
