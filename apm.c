#include <stdlib.h>
#include <string.h>

#include "apm.h"

/* Set u[size] = u[size] + 1, and return the carry. */
static apm_digit apm_inc(apm_digit *u, apm_size size)
{
    if (size == 0)
        return 1;

    for (; size--; ++u) {
        if (++*u)
            return 0;
    }
    return 1;
}

/* Set u[size] = u[size] - 1, and return the borrow. */
static apm_digit apm_dec(apm_digit *u, apm_size size)
{
    if (size == 0)
        return 1;

    for (;;) {
        if (u[0]--)
            return 0;
        if (--size == 0)
            return 1;
        u++;
    }
    return 1;
}

apm_digit apm_daddi(apm_digit *u, apm_size size, apm_digit v)
{
    if (v == 0 || size == 0)
        return v;
    return ((u[0] += v) < v) ? apm_inc(&u[1], size - 1) : 0;
}

/* Set w[size] = u[size] + v[size] and return the carry. */
apm_digit apm_add_n(const apm_digit *u,
                    const apm_digit *v,
                    apm_size size,
                    apm_digit *w)
{
    ASSERT(u != NULL);
    ASSERT(v != NULL);
    ASSERT(w != NULL);

    apm_digit cy = 0;
    while (size--) {
        apm_digit ud = *u++;
        const apm_digit vd = *v++;
        cy = (ud += cy) < cy;
        cy += (*w = ud + vd) < vd;
        ++w;
    }
    return cy;
}

apm_digit apm_add(const apm_digit *u,
                  apm_size usize,
                  const apm_digit *v,
                  apm_size vsize,
                  apm_digit *w)
{
    ASSERT(u != NULL);
    ASSERT(usize > 0);
    ASSERT(u[usize - 1]);
    ASSERT(v != NULL);
    ASSERT(vsize > 0);
    ASSERT(v[vsize - 1]);

    if (usize < vsize) {
        apm_digit cy = apm_add_n(u, v, usize, w);
        if (v != w)
            apm_copy(v + usize, vsize - usize, w + usize);
        return cy ? apm_inc(w + usize, vsize - usize) : 0;
    } else if (usize > vsize) {
        apm_digit cy = apm_add_n(u, v, vsize, w);
        if (u != w)
            apm_copy(u + vsize, usize - vsize, w + vsize);
        return cy ? apm_inc(w + vsize, usize - vsize) : 0;
    }
    /* usize == vsize */
    return apm_add_n(u, v, usize, w);
}

/* Set u[usize] = u[usize] + v[vsize] and return the carry. */
apm_digit apm_addi(apm_digit *u,
                   apm_size usize,
                   const apm_digit *v,
                   apm_size vsize)
{
    ASSERT(u != NULL);
    ASSERT(v != NULL);
    ASSERT(usize >= vsize);

    apm_digit cy = apm_addi_n(u, v, vsize);
    return cy ? apm_inc(u + vsize, usize - vsize) : 0;
}

/* Set w[size] = u[size] - v[size] and return the borrow. */
apm_digit apm_sub_n(const apm_digit *u,
                    const apm_digit *v,
                    apm_size size,
                    apm_digit *w)
{
    ASSERT(u != NULL);
    ASSERT(v != NULL);
    ASSERT(w != NULL);

    apm_digit cy = 0;
    while (size--) {
        const apm_digit ud = *u++;
        apm_digit vd = *v++;
        cy = (vd += cy) < cy;
        cy += (*w = ud - vd) > ud;
        ++w;
    }
    return cy;
}

apm_digit apm_sub(const apm_digit *u,
                  apm_size usize,
                  const apm_digit *v,
                  apm_size vsize,
                  apm_digit *w)
{
    ASSERT(usize >= vsize);

    if (usize == vsize)
        return apm_sub_n(u, v, usize, w);

    apm_digit cy = apm_sub_n(u, v, vsize, w);
    usize -= vsize;
    w += vsize;
    apm_copy(u + vsize, usize, w);
    return cy ? apm_dec(w, usize) : 0;
}

/* Set u[size] = u[size] - v[size] and return the borrow. */
apm_digit apm_subi_n(apm_digit *u, const apm_digit *v, apm_size size)
{
    ASSERT(u != NULL);
    ASSERT(v != NULL);

    apm_digit cy = 0;
    while (size--) {
        apm_digit vd = *v++;
        const apm_digit ud = *u;
        cy = (vd += cy) < cy;
        cy += (*u -= vd) > ud;
        ++u;
    }
    return cy;
}

apm_digit apm_subi(apm_digit *u,
                   apm_size usize,
                   const apm_digit *v,
                   apm_size vsize)
{
    ASSERT(u != NULL);
    ASSERT(v != NULL);
    ASSERT(usize >= vsize);

    return apm_subi_n(u, v, vsize) ? apm_dec(u + vsize, usize - vsize) : 0;
}

apm_digit apm_dmul(const apm_digit *u, apm_size size, apm_digit v, apm_digit *w)
{
    if (v <= 1) {
        if (v == 0)
            apm_zero(w, size);
        else
            apm_copy(u, size, w);
        return 0;
    }

    apm_digit cy = 0;
    while (size--) {
        apm_digit p1, p0;
        digit_mul(*u, v, p1, p0);
        cy = ((p0 += cy) < cy) + p1;
        *w++ = p0;
        ++u;
    }
    return cy;
}

apm_digit apm_dmul_add(const apm_digit *u,
                       apm_size size,
                       apm_digit v,
                       apm_digit *w)
{
    ASSERT(u != NULL);
    ASSERT(w != NULL);

    if (v <= 1)
        return v ? apm_addi_n(w, u, size) : 0;

    apm_digit cy = 0;
    while (size--) {
        apm_digit p1, p0;
        digit_mul(*u, v, p1, p0);
        cy = ((p0 += cy) < cy) + p1;
        cy += ((*w += p0) < p0);
        ++u;
        ++w;
    }
    return cy;
}

/* Multiply u[size] by 2^shift and store in v[size], returning carry.
 * shift will be taken modulo APM_DIGIT_BITS. */
apm_digit apm_lshift(const apm_digit *u,
                     apm_size size,
                     unsigned int shift,
                     apm_digit *v)
{
    if (!size)
        return 0;

    shift &= APM_DIGIT_BITS - 1;
    if (!shift) {
        if (u != v)
            apm_copy(u, size, v);
        return 0;
    }

    const unsigned int subp = APM_DIGIT_BITS - shift;
    apm_digit q = 0;
    do {
        const apm_digit p = *u++;
        *v++ = (p << shift) | q;
        q = p >> subp;
    } while (--size);
    return q;
}

/* Multiply u[size] by 2^shift, shift taken modulo APM_DIGIT_BITS. */
apm_digit apm_lshifti(apm_digit *u, apm_size size, unsigned int shift)
{
    shift &= APM_DIGIT_BITS - 1;
    if (!size || !shift)
        return 0;

    const unsigned int subp = APM_DIGIT_BITS - shift;
    apm_digit q = 0;
    do {
        const apm_digit p = *u;
        *u++ = (p << shift) | q;
        q = p >> subp;
    } while (--size);
    return q;
}

/* Divide u[size] by 2^shift, shift taken modulo APM_DIGIT_BITS. */
apm_digit apm_rshifti(apm_digit *u, apm_size size, unsigned int shift)
{
    shift &= APM_DIGIT_BITS - 1;
    if (!size || !shift)
        return 0;

    unsigned int subp = APM_DIGIT_BITS - shift;
    u += size;
    apm_digit q = 0;
    do {
        const apm_digit p = *--u;
        *u = (p >> shift) | q;
        q = p << subp;
    } while (--size);
    return q >> subp;
}

int apm_cmp_n(const apm_digit *u, const apm_digit *v, apm_size size)
{
    u += size;
    v += size;
    while (size--) {
        --u;
        --v;
        if (*u < *v)
            return -1;
        if (*u > *v)
            return +1;
    }
    return 0;
}

int apm_cmp(const apm_digit *u,
            apm_size usize,
            const apm_digit *v,
            apm_size vsize)
{
    APM_NORMALIZE(u, usize);
    APM_NORMALIZE(v, vsize);
    if (usize < vsize)
        return -1;
    if (usize > vsize)
        return +1;
    return usize ? apm_cmp_n(u, v, usize) : 0;
}
