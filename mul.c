#include <stdbool.h>

#include "apm.h"

/* Multiply u[usize] by v[vsize] and store the result in w[usize + vsize],
 * using the simple quadratic-time algorithm.
 */
void _apm_mul_base(const apm_digit *u,
                   apm_size usize,
                   const apm_digit *v,
                   apm_size vsize,
                   apm_digit *w)
{
    ASSERT(usize >= vsize);

    /* Find real sizes and zero any part of answer which will not be set. */
    apm_size ul = apm_rsize(u, usize);
    apm_size vl = apm_rsize(v, vsize);
    /* Zero digits which will not be set in multiply-and-add loop. */
    if (ul + vl != usize + vsize)
        apm_zero(w + (ul + vl), usize + vsize - (ul + vl));
    /* One or both are zero. */
    if (!ul || !vl)
        return;

    /* Now multiply by forming partial products and adding them to the result
     * so far. Rather than zero the low ul digits of w before starting, we
     * store, rather than add, the first partial product.
     */
    apm_digit *wp = w + ul;
    *wp = apm_dmul(u, ul, *v, w);
    while (--vl) {
        apm_digit vd = *++v;
        *++wp = apm_dmul_add(u, ul, vd, ++w);
    }
}

/* TODO: switch to Schönhage–Strassen algorithm
 * https://en.wikipedia.org/wiki/Sch%C3%B6nhage%E2%80%93Strassen_algorithm
 */

/* Karatsuba multiplication [cf. Knuth 4.3.3, vol.2, 3rd ed, pp.294-295]
 * Given U = U1*2^N + U0 and V = V1*2^N + V0,
 * we can recursively compute U*V with
 * (2^2N + 2^N)U1*V1 + (2^N)(U1-U0)(V0-V1) + (2^N + 1)U0*V0
 *
 * We might otherwise use
 * (2^2N - 2^N)U1*V1 + (2^N)(U1+U0)(V1+V0) + (1 - 2^N)U0*V0
 * except that (U1+U0) or (V1+V0) may become N+1 bit numbers if there is carry
 * in the additions, and this will slow down the routine.  However, if we use
 * the first formula the middle terms will not grow larger than N bits.
 */
static void apm_mul_n(const apm_digit *u,
                      const apm_digit *v,
                      apm_size size,
                      apm_digit *w)
{
    /* TODO: Only allocate a temporary buffer which is large enough for all
     * following recursive calls, rather than allocating at each call.
     */
    if (u == v) {
        apm_sqr(u, size, w);
        return;
    }

    if (size < KARATSUBA_MUL_THRESHOLD) {
        _apm_mul_base(u, size, v, size, w);
        return;
    }

    const bool odd = size & 1;
    const apm_size even_size = size - odd;
    const apm_size half_size = even_size / 2;

    const apm_digit *u0 = u, *u1 = u + half_size;
    const apm_digit *v0 = v, *v1 = v + half_size;
    apm_digit *w0 = w, *w1 = w + even_size;

    /* U0 * V0 => w[0..even_size-1]; */
    /* U1 * V1 => w[even_size..2*even_size-1]. */
    if (half_size >= KARATSUBA_MUL_THRESHOLD) {
        apm_mul_n(u0, v0, half_size, w0);
        apm_mul_n(u1, v1, half_size, w1);
    } else {
        _apm_mul_base(u0, half_size, v0, half_size, w0);
        _apm_mul_base(u1, half_size, v1, half_size, w1);
    }

    /* Since we cannot add w[0..even_size-1] to w[half_size ...
     * half_size+even_size-1] in place, we have to make a copy of it now.
     * This later gets used to store U1-U0 and V0-V1.
     */
    apm_digit *tmp = APM_TMP_COPY(w0, even_size);

    apm_digit cy;
    /* w[half_size..half_size+even_size-1] += U1*V1. */
    cy = apm_addi_n(w + half_size, w1, even_size);
    /* w[half_size..half_size+even_size-1] += U0*V0. */
    cy += apm_addi_n(w + half_size, tmp, even_size);

    /* Get absolute value of U1-U0. */
    apm_digit *u_tmp = tmp;
    bool prod_neg = apm_cmp_n(u1, u0, half_size) < 0;
    if (prod_neg)
        apm_sub_n(u0, u1, half_size, u_tmp);
    else
        apm_sub_n(u1, u0, half_size, u_tmp);

    /* Get absolute value of V0-V1. */
    apm_digit *v_tmp = tmp + half_size;
    if (apm_cmp_n(v0, v1, half_size) < 0)
        apm_sub_n(v1, v0, half_size, v_tmp), prod_neg ^= 1;
    else
        apm_sub_n(v0, v1, half_size, v_tmp);

    /* tmp = (U1-U0)*(V0-V1). */
    tmp = APM_TMP_ALLOC(even_size);
    if (half_size >= KARATSUBA_MUL_THRESHOLD)
        apm_mul_n(u_tmp, v_tmp, half_size, tmp);
    else
        _apm_mul_base(u_tmp, half_size, v_tmp, half_size, tmp);
    APM_TMP_FREE(u_tmp);

    /* Now add / subtract (U1-U0)*(V0-V1) from
     * w[half_size..half_size+even_size-1] based on whether it is negative or
     * positive.
     */
    if (prod_neg)
        cy -= apm_subi_n(w + half_size, tmp, even_size);
    else
        cy += apm_addi_n(w + half_size, tmp, even_size);
    APM_TMP_FREE(tmp);

    /* Now if there was any carry from the middle digits (which is at most 2),
     * add that to w[even_size+half_size..2*even_size-1]. */
    if (cy) {
        ASSERT(apm_daddi(w + even_size + half_size, half_size, cy) == 0);
    }

    if (odd) {
        /* We have the product U[0..even_size-1] * V[0..even_size-1] in
         * W[0..2*even_size-1].  We need to add the following to it:
         * V[size-1] * U[0..size-2]
         * U[size-1] * V[0..size-1] */
        w[even_size * 2] =
            apm_dmul_add(u, even_size, v[even_size], &w[even_size]);
        w[even_size * 2 + 1] =
            apm_dmul_add(v, size, u[even_size], &w[even_size]);
    }
}

void apm_mul(const apm_digit *u,
             apm_size usize,
             const apm_digit *v,
             apm_size vsize,
             apm_digit *w)
{
    {
        const apm_size ul = apm_rsize(u, usize);
        const apm_size vl = apm_rsize(v, vsize);
        if (!ul || !vl) {
            apm_zero(w, usize + vsize);
            return;
        }
        /* Zero digits which won't be set. */
        if (ul + vl != usize + vsize)
            apm_zero(w + (ul + vl), (usize + vsize) - (ul + vl));

        /* Wanted: USIZE >= VSIZE. */
        if (ul < vl) {
            SWAP(u, v);
            usize = vl;
            vsize = ul;
        } else {
            usize = ul;
            vsize = vl;
        }
    }

    ASSERT(usize >= vsize);

    if (vsize < KARATSUBA_MUL_THRESHOLD) {
        _apm_mul_base(u, usize, v, vsize, w);
        return;
    }

    apm_mul_n(u, v, vsize, w);
    if (usize == vsize)
        return;

    apm_size wsize = usize + vsize;
    apm_zero(w + (vsize * 2), wsize - (vsize * 2));
    w += vsize;
    wsize -= vsize;
    u += vsize;
    usize -= vsize;

    apm_digit *tmp = NULL;
    if (usize >= vsize) {
        tmp = APM_TMP_ALLOC(vsize * 2);
        do {
            apm_mul_n(u, v, vsize, tmp);
            ASSERT(apm_addi(w, wsize, tmp, vsize * 2) == 0);
            w += vsize;
            wsize -= vsize;
            u += vsize;
            usize -= vsize;
        } while (usize >= vsize);
    }

    if (usize) { /* Size of U isn't a multiple of size of V. */
        if (!tmp)
            tmp = APM_TMP_ALLOC(usize + vsize);
        /* Now usize < vsize. Rearrange operands. */
        if (usize < KARATSUBA_MUL_THRESHOLD)
            _apm_mul_base(v, vsize, u, usize, tmp);
        else
            apm_mul(v, vsize, u, usize, tmp);
        ASSERT(apm_addi(w, wsize, tmp, usize + vsize) == 0);
    }
    APM_TMP_FREE(tmp);
}
