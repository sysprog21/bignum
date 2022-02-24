#include <stdbool.h>

#include "apm.h"

extern void _apm_mul_base(const apm_digit *u,
                          apm_size usize,
                          const apm_digit *v,
                          apm_size vsize,
                          apm_digit *w);

/* Square diagonal. */
static void apm_sqr_diag(const apm_digit *u, apm_size size, apm_digit *v)
{
    if (!size)
        return;
    /* No compiler seems to recognize that if ((A+B) mod 2^N) < A (or B) iff
     * (A+B) >= 2^N and it can use the carry flag after the adds rather than
     * doing comparisons to see if overflow has ocurred. Instead they generate
     * code to perform comparisons, retaining values in already scarce
     * registers after they should be "dead." At any rate this isn't the
     * time-critical part of squaring so it's nothing to lose sleep over. */
    apm_digit p0, p1;
    digit_sqr(*u, p1, p0);
    p1 += (v[0] += p0) < p0;
    apm_digit cy = (v[1] += p1) < p1;
    while (--size) {
        u += 1;
        v += 2;
        digit_sqr(*u, p1, p0);
        p1 += (p0 += cy) < cy;
        p1 += (v[0] += p0) < p0;
        cy = (v[1] += p1) < p1;
    }
    ASSERT(cy == 0);
}

#ifndef BASE_SQR_THRESHOLD
#define BASE_SQR_THRESHOLD 10
#endif /* !BASE_SQR_THRESHOLD */

static void apm_sqr_base(const apm_digit *u, apm_size usize, apm_digit *v)
{
    if (!usize)
        return;

    /* Find size, and zero any digits which will not be set. */
    apm_size ul = apm_rsize(u, usize);
    if (ul != usize) {
        apm_zero(v + (ul * 2), (usize - ul) * 2);
        if (ul == 0)
            return;
        usize = ul;
    }

    /* Single-precision case. */
    if (usize == 1) {
        /* FIXME can't do this: digit_sqr(u[0], v[1], v[0]) */
        apm_digit v0, v1;
        digit_sqr(*u, v1, v0);
        v[1] = v1;
        v[0] = v0;
        return;
    }

    /* It is better to use the multiply routine if the number is small. */
    if (usize <= BASE_SQR_THRESHOLD) {
        _apm_mul_base(u, usize, u, usize, v);
        return;
    }

    /* Calculate products u[i] * u[j] for i != j.
     * Most of the savings vs long multiplication come here, since we only
     * perform (N-1) + (N-2) + ... + 1 = (N^2-N)/2 multiplications, vs a full
     * N^2 in long multiplication. */
    v[0] = 0;
    const apm_digit *ui = u;
    apm_digit *vp = &v[1];
    ul = usize - 1;
    vp[ul] = apm_dmul(&ui[1], ul, ui[0], vp);
    for (vp += 2; ++ui, --ul; vp += 2)
        vp[ul] = apm_dmul_add(&ui[1], ul, ui[0], vp);

    /* Double cross-products. */
    ul = usize * 2 - 1;
    v[ul] = apm_lshifti(v + 1, ul - 1, 1);

    /* Add "main diagonal:"
     * for i=0 .. n-1
     *     v += u[i]^2 * B^2i */
    apm_sqr_diag(u, usize, v);
}

/* Karatsuba squaring recursively applies the formula:
 *		U = U1*2^N + U0
 *		U^2 = (2^2N + 2^N)U1^2 - (U1-U0)^2 + (2^N + 1)U0^2
 * From my own testing this uses ~20% less time compared with slightly easier to
 * code formula:
 *		U^2 = (2^2N)U1^2 + (2^(N+1))(U1*U0) + U0^2
 */
void apm_sqr(const apm_digit *u, apm_size size, apm_digit *v)
{
    apm_size rsize = apm_rsize(u, size);
    if (rsize != size) {
        apm_zero(v + rsize * 2, (size - rsize) * 2);
        size = rsize;
    }

    if (size < KARATSUBA_SQR_THRESHOLD) {
        if (!size)
            return;
        if (size <= BASE_SQR_THRESHOLD)
            _apm_mul_base(u, size, u, size, v);
        else
            apm_sqr_base(u, size, v);
        return;
    }

    const bool odd_size = size & 1;
    const apm_size even_size = size & ~1;
    const apm_size half_size = even_size / 2;
    const apm_digit *u0 = u, *u1 = u + half_size;
    apm_digit *v0 = v, *v1 = v + even_size;

    /* Choose the appropriate squaring function. */
    void (*sqr_fn)(const apm_digit *, apm_size, apm_digit *) =
        (half_size >= KARATSUBA_SQR_THRESHOLD) ? apm_sqr : apm_sqr_base;
    /* Compute the low and high squares, potentially recursively. */
    sqr_fn(u0, half_size, v0); /* U0^2 => V0 */
    sqr_fn(u1, half_size, v1); /* U1^2 => V1 */

    apm_digit *tmp = APM_TMP_ALLOC(even_size * 2);
    apm_digit *tmp2 = tmp + even_size;
    /* tmp = w[0..even_size-1] */
    apm_copy(v0, even_size, tmp);
    /* v += U1^2 * 2^N */
    apm_digit cy = apm_addi_n(v + half_size, v1, even_size);
    /* v += U0^2 * 2^N */
    cy += apm_addi_n(v + half_size, tmp, even_size);

    int cmp = apm_cmp_n(u1, u0, half_size);
    if (cmp) {
        if (cmp < 0)
            apm_sub_n(u0, u1, half_size, tmp);
        else
            apm_sub_n(u1, u0, half_size, tmp);
        sqr_fn(tmp, half_size, tmp2);
        cy -= apm_subi_n(v + half_size, tmp2, even_size);
    }
    APM_TMP_FREE(tmp);

    if (cy) {
        ASSERT(apm_daddi(v + even_size + half_size, half_size, cy) == 0);
    }

    if (odd_size) {
        v[even_size * 2] =
            apm_dmul_add(u, even_size, u[even_size], &v[even_size]);
        v[even_size * 2 + 1] =
            apm_dmul_add(u, size, u[even_size], &v[even_size]);
    }
}
