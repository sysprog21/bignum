#include <ctype.h>
#include <stdio.h>

#include "apm.h"

/* radix_sizes[B] = number of radix-B digits needed to represent an 8-bit
 * unsigned integer; B on [2, 36] */
static const float radix_sizes[37] = {
    /*  0 */ 0.00000000f,
    /*  1 */ 0.00000000f,
    /*  2 */ 8.00000000f,
    /*  3 */ 5.04743803f,
    /*  4 */ 4.00000000f,
    /*  5 */ 3.44541246f,
    /*  6 */ 3.09482246f,
    /*  7 */ 2.84965750f,
    /*  8 */ 2.66666667f,
    /*  9 */ 2.52371901f,
    /* 10 */ 2.40823997f,
    /* 11 */ 2.31251861f,
    /* 12 */ 2.23154357f,
    /* 13 */ 2.16190524f,
    /* 14 */ 2.10119628f,
    /* 15 */ 2.04766420f,
    /* 16 */ 2.00000000f,
    /* 17 */ 1.95720434f,
    /* 18 */ 1.91849973f,
    /* 19 */ 1.88327131f,
    /* 20 */ 1.85102571f,
    /* 21 */ 1.82136199f,
    /* 22 */ 1.79395059f,
    /* 23 */ 1.76851784f,
    /* 24 */ 1.74483434f,
    /* 25 */ 1.72270623f,
    /* 26 */ 1.70196843f,
    /* 27 */ 1.68247934f,
    /* 28 */ 1.66411678f,
    /* 29 */ 1.64677466f,
    /* 30 */ 1.63036038f,
    /* 31 */ 1.61479269f,
    /* 32 */ 1.60000000f,
    /* 33 */ 1.58591891f,
    /* 34 */ 1.57249306f,
    /* 35 */ 1.55967218f,
    /* 36 */ 1.54741123f};

static const struct {
    apm_digit max_radix;
    unsigned int max_power;
} radix_table[37] = {
#if APM_DIGIT_SIZE == 4
    /*  0 */ {0x00000000U, 0},
    /*  1 */ {0x00000000U, 0},
    /*  2 */ {0x80000000U, 31},
    /*  3 */ {0xCFD41B91U, 20},
    /*  4 */ {0x40000000U, 15},
    /*  5 */ {0x48C27395U, 13},
    /*  6 */ {0x81BF1000U, 12},
    /*  7 */ {0x75DB9C97U, 11},
    /*  8 */ {0x40000000U, 10},
    /*  9 */ {0xCFD41B91U, 10},
    /* 10 */ {0x3B9ACA00U, 9},
    /* 11 */ {0x8C8B6D2BU, 9},
    /* 12 */ {0x19A10000U, 8},
    /* 13 */ {0x309F1021U, 8},
    /* 14 */ {0x57F6C100U, 8},
    /* 15 */ {0x98C29B81U, 8},
    /* 16 */ {0x10000000U, 7},
    /* 17 */ {0x18754571U, 7},
    /* 18 */ {0x247DBC80U, 7},
    /* 19 */ {0x3547667BU, 7},
    /* 20 */ {0x4C4B4000U, 7},
    /* 21 */ {0x6B5A6E1DU, 7},
    /* 22 */ {0x94ACE180U, 7},
    /* 23 */ {0xCAF18367U, 7},
    /* 24 */ {0x0B640000U, 6},
    /* 25 */ {0x0E8D4A51U, 6},
    /* 26 */ {0x1269AE40U, 6},
    /* 27 */ {0x17179149U, 6},
    /* 28 */ {0x1CB91000U, 6},
    /* 29 */ {0x23744899U, 6},
    /* 30 */ {0x2B73A840U, 6},
    /* 31 */ {0x34E63B41U, 6},
    /* 32 */ {0x40000000U, 6},
    /* 33 */ {0x4CFA3CC1U, 6},
    /* 34 */ {0x5C13D840U, 6},
    /* 35 */ {0x6D91B519U, 6},
    /* 36 */ {0x81BF1000U, 6}
#elif APM_DIGIT_SIZE == 8
    /*  0 */ {UINT64_C(0x0000000000000000), 0},
    /*  1 */ {UINT64_C(0x0000000000000000), 0},
    /*  2 */ {UINT64_C(0x8000000000000000), 63},
    /*  3 */ {UINT64_C(0xA8B8B452291FE821), 40},
    /*  4 */ {UINT64_C(0x4000000000000000), 31},
    /*  5 */ {UINT64_C(0x6765C793FA10079D), 27},
    /*  6 */ {UINT64_C(0x41C21CB8E1000000), 24},
    /*  7 */ {UINT64_C(0x3642798750226111), 22},
    /*  8 */ {UINT64_C(0x8000000000000000), 21},
    /*  9 */ {UINT64_C(0xA8B8B452291FE821), 20},
    /* 10 */ {UINT64_C(0x8AC7230489E80000), 19},
    /* 11 */ {UINT64_C(0x4D28CB56C33FA539), 18},
    /* 12 */ {UINT64_C(0x1ECA170C00000000), 17},
    /* 13 */ {UINT64_C(0x780C7372621BD74D), 17},
    /* 14 */ {UINT64_C(0x1E39A5057D810000), 16},
    /* 15 */ {UINT64_C(0x5B27AC993DF97701), 16},
    /* 16 */ {UINT64_C(0x1000000000000000), 15},
    /* 17 */ {UINT64_C(0x27B95E997E21D9F1), 15},
    /* 18 */ {UINT64_C(0x5DA0E1E53C5C8000), 15},
    /* 19 */ {UINT64_C(0xD2AE3299C1C4AEDB), 15},
    /* 20 */ {UINT64_C(0x16BCC41E90000000), 14},
    /* 21 */ {UINT64_C(0x2D04B7FDD9C0EF49), 14},
    /* 22 */ {UINT64_C(0x5658597BCAA24000), 14},
    /* 23 */ {UINT64_C(0xA0E2073737609371), 14},
    /* 24 */ {UINT64_C(0x0C29E98000000000), 13},
    /* 25 */ {UINT64_C(0x14ADF4B7320334B9), 13},
    /* 26 */ {UINT64_C(0x226ED36478BFA000), 13},
    /* 27 */ {UINT64_C(0x383D9170B85FF80B), 13},
    /* 28 */ {UINT64_C(0x5A3C23E39C000000), 13},
    /* 29 */ {UINT64_C(0x8E65137388122BCD), 13},
    /* 30 */ {UINT64_C(0xDD41BB36D259E000), 13},
    /* 31 */ {UINT64_C(0x0AEE5720EE830681), 12},
    /* 32 */ {UINT64_C(0x1000000000000000), 12},
    /* 33 */ {UINT64_C(0x172588AD4F5F0981), 12},
    /* 34 */ {UINT64_C(0x211E44F7D02C1000), 12},
    /* 35 */ {UINT64_C(0x2EE56725F06E5C71), 12},
    /* 36 */ {UINT64_C(0x41C21CB8E1000000), 12}
#endif
};

/* Return the size, in bytes, that a character string must be in order to hold
 * the representation of a LEN-digit number in BASE. Return value does NOT
 * account for terminating '\0'.
 */
static size_t apm_string_size(apm_size size, unsigned int radix)
{
    ASSERT(radix >= 2);
    ASSERT(radix <= 36);

    if ((radix & (radix - 1)) == 0) {
        unsigned int lg = apm_digit_lsb_shift(radix);
        return ((size * APM_DIGIT_BITS + lg - 1) / lg) + 1;
    }
    /* round up to second next largest integer */
    return (size_t)(radix_sizes[radix] * (size * APM_DIGIT_SIZE)) + 2;
}

/* Set u[size] = u[usize] / v, and return the remainder. */
static apm_digit apm_ddivi(apm_digit *u, apm_size size, apm_digit v)
{
    ASSERT(u != NULL);
    ASSERT(v != 0);

    if (v == 1)
        return 0;

    APM_NORMALIZE(u, size);
    if (!size)
        return 0;

    if ((v & (v - 1)) == 0)
        return apm_rshifti(u, size, apm_digit_lsb_shift(v));

    apm_digit s1 = 0;
    u += size;
    do {
        apm_digit s0 = *--u;
        apm_digit q, r;
        if (s1 == 0) {
            q = s0 / v;
            r = s0 % v;
        } else {
            digit_div(s1, s0, v, q, r);
        }
        *u = q;
        s1 = r;
    } while (--size);
    return s1;
}

static const char radix_chars[37] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* Return u[size] as a null-terminated character string in a radix on [2,36]. */
static char *apm_get_str(const apm_digit *u,
                         apm_size size,
                         unsigned int radix,
                         char *out)
{
    ASSERT(u != NULL);
    ASSERT(radix >= 2);
    ASSERT(radix <= 36);

    APM_NORMALIZE(u, size);
    if (size == 0 || (size == 1 && u[0] < radix)) {
        if (!out)
            out = MALLOC(2);
        out[0] = size ? radix_chars[u[0]] : '0';
        out[1] = '\0';
        return out;
    }

    const apm_digit max_radix = radix_table[radix].max_radix;
    const unsigned int max_power = radix_table[radix].max_power;

    if (!out)
        out = MALLOC(apm_string_size(size, radix) + 1);
    char *outp = out;

    if ((radix & (radix - 1)) == 0) { /* Radix is a power of two. */
        /* We need to extract LG bits for each digit. */
        const unsigned int lg = apm_digit_lsb_shift(radix);
        const apm_digit mask = radix - 1; /* mask = ((apm_digit)1 << lg) - 1; */
        const unsigned int od = APM_DIGIT_BITS / lg;
        if (APM_DIGIT_BITS % lg == 0) { /* bases 2 (2^1), 4 (2^2), 16 (2^4) */
            const apm_digit *ue = u + size;

            do {
                apm_digit r = *u;
                unsigned int i = 0;
                do {
                    *outp++ = radix_chars[r & mask];
                    r >>= lg;
                } while (++i < od);
            } while (++u < ue);
        } else { /* bases 8 (2^3), 32 (2^5) */
            /* Do it the lazy way */
            const unsigned int shift = lg * od;

            ASSERT(shift < APM_DIGIT_BITS);

            apm_digit *tmp = APM_TMP_COPY(u, size);
            apm_size tsize = size;

            do {
                apm_digit r = apm_rshifti(tmp, tsize, shift);
                tsize -= (tmp[tsize - 1] == 0);
                unsigned int i = 0;
                do {
                    *outp++ = radix_chars[r & mask];
                    r >>= lg;
                } while (++i < od);
            } while (tsize != 0);

            APM_TMP_FREE(tmp);
        }
    } else {
        apm_digit *tmp = APM_TMP_COPY(u, size);
        apm_size tsize = size;

        do {
            /* Multi-precision: divide U by largest power of RADIX to fit in
             * one apm_digit and extract remainder.
             */
            apm_digit r = apm_ddivi(tmp, tsize, max_radix);
            tsize -= (tmp[tsize - 1] == 0);
            /* Single-precision: extract K remainders from that remainder,
             * where K is the largest integer such that RADIX^K < 2^BITS.
             */
            unsigned int i = 0;
            do {
                apm_digit rq = r / radix;
                apm_digit rr = r % radix;
                *outp++ = radix_chars[rr];
                r = rq;
                if (tsize == 0 && r == 0) /* Eliminate any leading zeroes */
                    break;
            } while (++i < max_power);
            ASSERT(r == 0);
            /* Loop until TMP = 0. */
        } while (tsize != 0);

        APM_TMP_FREE(tmp);
    }

    char *f = outp - 1;
    /* Eliminate leading (trailing) zeroes */
    while (*f == '0')
        --f;
    /* NULL terminate */
    f[1] = '\0';
    /* Reverse digits */
    for (char *s = out; s < f; ++s, --f)
        SWAP(*s, *f, char);
    return out;
}

void apm_fprint(const apm_digit *u, apm_size size, unsigned int radix, FILE *fp)
{
    ASSERT(u != NULL);

    ASSERT(radix >= 2);
    ASSERT(radix <= 36);

    APM_NORMALIZE(u, size);
    const size_t string_size = apm_string_size(size, radix) + 1;
    char *str = MALLOC(string_size);
    char *p = apm_get_str(u, size, radix, str);
    ASSERT(p != NULL);
    fputs(p, fp);
    FREE(str);
}
