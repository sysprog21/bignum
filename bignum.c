#include <stdlib.h>

#include "bn.h"

#define BN_MIN_ALLOC(n, s)                                               \
    do {                                                                 \
        bn *const __n = (n);                                             \
        const apm_size __s = (s);                                        \
        if (__n->alloc < __s) {                                          \
            __n->digits =                                                \
                apm_resize(__n->digits, __n->alloc = ((__s + 3) & ~3U)); \
        }                                                                \
    } while (0)

#define BN_SIZE(n, s)                                                          \
    do {                                                                       \
        bn *const __n = (n);                                                   \
        __n->size = (s);                                                       \
        if (__n->alloc < __n->size) {                                          \
            __n->digits =                                                      \
                apm_resize(__n->digits, __n->alloc = ((__n->size + 3) & ~3U)); \
        }                                                                      \
    } while (0)

#define BN_INIT_BYTES 8
#define BN_INIT_DIGITS ((BN_INIT_BYTES + APM_DIGIT_SIZE - 1) / APM_DIGIT_SIZE)

void bn_init(bn *n)
{
    ASSERT(n != NULL);

    n->alloc = BN_INIT_DIGITS;
    n->digits = apm_new0(BN_INIT_DIGITS);
    n->size = 0;
    n->sign = 0;
}

void bn_init_u32(bn *n, uint32_t ui)
{
    bn_init(n);
    bn_set_u32(n, ui);
}

void bn_free(bn *n)
{
    ASSERT(n != NULL);

    apm_free(n->digits);
}

static void bn_set(bn *p, const bn *q)
{
    ASSERT(p != NULL);
    ASSERT(q != NULL);

    if (p == q)
        return;

    if (q->size == 0) {
        bn_zero(p);
    } else {
        BN_SIZE(p, q->size);
        apm_copy(q->digits, q->size, p->digits);
        p->sign = q->sign;
    }
}

void bn_zero(bn *n)
{
    ASSERT(n != NULL);

    n->sign = 0;
    n->size = 0;
}

void bn_set_u32(bn *n, uint32_t m)
{
    n->sign = 0;
    if (m == 0) {
        n->size = 0;
        return;
    }

#if APM_DIGIT_MAX >= UINT32_MAX
    BN_SIZE(n, 1);
    n->digits[0] = (apm_digit) m;
#else
    uint32_t t = m;
    apm_size j = 0;
    while (t) {
        t >>= APM_DIGIT_BITS;
        ++j;
    }
    BN_MIN_ALLOC(n, j);
    for (j = 0; m; ++j) {
        n->digits[j] = (apm_digit) m;
        m >>= APM_DIGIT_BITS;
    }
    n->size = j;
#endif
}

void bn_swap(bn *a, bn *b)
{
    bn tmp = *a;
    *a = *b;
    *b = tmp;
}

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

void bn_add(const bn *a, const bn *b, bn *c)
{
    if (a->size == 0) {
        if (b->size == 0)
            c->size = 0;
        else
            bn_set(c, b);
        return;
    } else if (b->size == 0) {
        bn_set(c, a);
        return;
    }

    if (a == b) {
        apm_digit cy;
        if (a == c) {
            cy = apm_lshifti(c->digits, c->size, 1);
        } else {
            BN_SIZE(c, a->size);
            cy = apm_lshift(a->digits, a->size, 1, c->digits);
        }
        if (cy) {
            BN_MIN_ALLOC(c, c->size + 1);
            c->digits[c->size++] = cy;
        }
        return;
    }

    /* Note: it should work for A == C or B == C */
    apm_size size;
    if (a->sign == b->sign) { /* Both positive or negative. */
        size = MAX(a->size, b->size);
        BN_MIN_ALLOC(c, size + 1);
        apm_digit cy =
            apm_add(a->digits, a->size, b->digits, b->size, c->digits);
        if (cy)
            c->digits[size++] = cy;
        else
            APM_NORMALIZE(c->digits, size);
        c->sign = a->sign;
    } else { /* Differing signs. */
        if (a->sign)
            SWAP(a, b);

        ASSERT(a->sign == 0);
        ASSERT(b->sign == 1);

        int cmp = apm_cmp(a->digits, a->size, b->digits, b->size);
        if (cmp > 0) { /* |A| > |B| */
            /* If B < 0 and |A| > |B|, then C = A - |B| */
            BN_MIN_ALLOC(c, a->size);
            ASSERT(apm_sub(a->digits, a->size, b->digits, b->size, c->digits) ==
                   0);
            c->sign = 0;
            size = apm_rsize(c->digits, a->size);
        } else if (cmp < 0) { /* |A| < |B| */
            /* If B < 0 and |A| < |B|, then C = -(|B| - |A|) */
            BN_MIN_ALLOC(c, b->size);
            ASSERT(apm_sub(b->digits, b->size, a->digits, a->size, c->digits) ==
                   0);
            c->sign = 1;
            size = apm_rsize(c->digits, b->size);
        } else { /* |A| = |B| */
            c->sign = 0;
            size = 0;
        }
    }
    c->size = size;
}

void bn_mul(const bn *a, const bn *b, bn *c)
{
    if (a->size == 0 || b->size == 0) {
        bn_zero(c);
        return;
    }

    if (a == b) {
        bn_sqr(a, c);
        return;
    }

    apm_size csize = a->size + b->size;
    if (a == c || b == c) {
        apm_digit *prod = APM_TMP_ALLOC(csize);
        apm_mul(a->digits, a->size, b->digits, b->size, prod);
        csize -= (prod[csize - 1] == 0);
        BN_SIZE(c, csize);
        apm_copy(prod, csize, c->digits);
        APM_TMP_FREE(prod);
    } else {
        ASSERT(a->digits[a->size - 1] != 0);
        ASSERT(b->digits[b->size - 1] != 0);
        BN_MIN_ALLOC(c, csize);
        apm_mul(a->digits, a->size, b->digits, b->size, c->digits);
        c->size = csize - (c->digits[csize - 1] == 0);
    }
    c->sign = a->sign ^ b->sign;
}

void bn_sqr(const bn *a, bn *b)
{
    if (a->size == 0) {
        bn_zero(b);
        return;
    }

    apm_size bsize = a->size * 2;
    if (a == b) {
        apm_digit *prod = APM_TMP_ALLOC(bsize);
        apm_sqr(a->digits, a->size, prod);
        bsize -= (prod[bsize - 1] == 0);
        BN_SIZE(b, bsize);
        apm_copy(prod, bsize, b->digits);
        APM_TMP_FREE(prod);
    } else {
        BN_MIN_ALLOC(b, bsize);
        apm_sqr(a->digits, a->size, b->digits);
        b->size = bsize - (b->digits[bsize - 1] == 0);
    }
    b->sign = 0;
}

void bn_lshift(const bn *p, unsigned int bits, bn *q)
{
    if (bits == 0 || bn_is_zero(p)) {
        if (bits == 0)
            bn_set(q, p);
        else
            bn_zero(q);
        return;
    }

    const unsigned int digits = bits / APM_DIGIT_BITS;
    bits %= APM_DIGIT_BITS;

    apm_digit cy;
    if (p == q) {
        cy = apm_lshifti(q->digits, q->size, bits);
        if (digits != 0) {
            BN_MIN_ALLOC(q, q->size + digits);
            for (int j = q->size - 1; j >= 0; j--)
                q->digits[j + digits] = q->digits[j];
            q->size += digits;
        }
    } else {
        BN_SIZE(q, p->size + digits);
        cy = apm_lshift(p->digits, p->size, bits, q->digits + digits);
    }

    apm_zero(q->digits, digits);
    if (cy) {
        BN_SIZE(q, q->size + 1);
        q->digits[q->size - 1] = cy;
    }
}

void bn_fprint(const bn *n, unsigned int base, FILE *fp)
{
    if (!fp)
        fp = stdout;
    if (n->size == 0) {
        fputc('0', fp);
        return;
    }
    if (n->sign)
        fputc('-', fp);
    apm_fprint(n->digits, n->size, base, fp);
}
