
#include <stdio.h>

#include "crypto_core_curve25519.h"
#include "crypto_core_ed25519.h"
#include "crypto_scalarmult_curve25519.h"
#include "export.h"
#include "private/common.h"
#include "private/ed25519_ref10.h"

#ifdef HAVE_TI_MODE
# include "../ed25519/ref10/fe_51/constants.h"
#else
# include "../ed25519/ref10/fe_25_5/constants.h"
#endif

static int
is_canonical(const unsigned char *s)
{
    unsigned char c;
    unsigned char d;
    unsigned int  i;

    c = s[31] ^ 0x7f;
    for (i = 30; i > 0; i--) {
        c |= s[i] ^ 0xff;
    }
    c = (((unsigned int) c) - 1U) >> 8;
    d = (0xed - 1U - (unsigned int) s[0]) >> 8;

    return 1 - (c & d & 1);
}

static int
has_small_order(const unsigned char s[32])
{
    CRYPTO_ALIGN(16)
    static const unsigned char blacklist[][32] = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0xe0, 0xeb, 0x7a, 0x7c, 0x3b, 0x41, 0xb8, 0xae, 0x16, 0x56, 0xe3, 0xfa, 0xf1, 0x9f, 0xc4, 0x6a, 0xda, 0x09, 0x8d, 0xeb, 0x9c, 0x32, 0xb1, 0xfd, 0x86, 0x62, 0x05, 0x16, 0x5f, 0x49, 0xb8, 0x00 },
        { 0x5f, 0x9c, 0x95, 0xbc, 0xa3, 0x50, 0x8c, 0x24, 0xb1, 0xd0, 0xb1, 0x55, 0x9c, 0x83, 0xef, 0x5b, 0x04, 0x44, 0x5c, 0xc4, 0x58, 0x1c, 0x8e, 0x86, 0xd8, 0x22, 0x4e, 0xdd, 0xd0, 0x9f, 0x11, 0x57 },
        { 0xec, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f },
        { 0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f },
        { 0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f },
        { 0xcd, 0xeb, 0x7a, 0x7c, 0x3b, 0x41, 0xb8, 0xae, 0x16, 0x56, 0xe3, 0xfa, 0xf1, 0x9f, 0xc4, 0x6a, 0xda, 0x09, 0x8d, 0xeb, 0x9c, 0x32, 0xb1, 0xfd, 0x86, 0x62, 0x05, 0x16, 0x5f, 0x49, 0xb8, 0x80 },
        { 0x4c, 0x9c, 0x95, 0xbc, 0xa3, 0x50, 0x8c, 0x24, 0xb1, 0xd0, 0xb1, 0x55, 0x9c, 0x83, 0xef, 0x5b, 0x04, 0x44, 0x5c, 0xc4, 0x58, 0x1c, 0x8e, 0x86, 0xd8, 0x22, 0x4e, 0xdd, 0xd0, 0x9f, 0x11, 0xd7 },
        { 0xd9, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
        { 0xda, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
        { 0xdb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
    };
    unsigned char c[12] = { 0 };
    unsigned int  k;
    size_t        i, j;

    COMPILER_ASSERT(12 == sizeof blacklist / sizeof blacklist[0]);
    for (j = 0; j < 32; j++) {
        for (i = 0; i < sizeof blacklist / sizeof blacklist[0]; i++) {
            c[i] |= s[j] ^ blacklist[i][j];
        }
    }
    k = 0;
    for (i = 0; i < sizeof blacklist / sizeof blacklist[0]; i++) {
        k |= (c[i] - 1);
    }
    return (int) ((k >> 8) & 1);
}

static int
is_on_main_subgroup(const fe25519 x1)
{
    static const unsigned char L[32] = {
        0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7,
        0xa2, 0xde, 0xf9, 0xde, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10
    };
    fe25519      x2;
    fe25519      z2;
    fe25519      x3;
    fe25519      z3;
    fe25519      tmp0;
    fe25519      tmp1;
    int          pos;
    unsigned int swap;
    unsigned int b;

    fe25519_1(x2);
    fe25519_0(z2);
    fe25519_copy(x3, x1);
    fe25519_1(z3);
    swap = 0;
    for (pos = 252; pos >= 0; --pos) {
        b = L[pos / 8] >> (pos & 7);
        b &= 1;
        swap ^= b;
        fe25519_cswap(x2, x3, swap);
        fe25519_cswap(z2, z3, swap);
        swap = b;
        fe25519_sub(tmp0, x3, z3);
        fe25519_sub(tmp1, x2, z2);
        fe25519_add(x2, x2, z2);
        fe25519_add(z2, x3, z3);
        fe25519_mul(z3, tmp0, x2);
        fe25519_mul(z2, z2, tmp1);
        fe25519_sq(tmp0, tmp1);
        fe25519_sq(tmp1, x2);
        fe25519_add(x3, z3, z2);
        fe25519_sub(z2, z3, z2);
        fe25519_mul(x2, tmp1, tmp0);
        fe25519_sub(tmp1, tmp1, tmp0);
        fe25519_sq(z2, z2);
        fe25519_scalar_product(z3, tmp1, 121666);
        fe25519_sq(x3, x3);
        fe25519_add(tmp0, tmp0, z3);
        fe25519_mul(z3, x1, z2);
        fe25519_mul(z2, tmp1, tmp0);
    }
    fe25519_cswap(x2, x3, swap);
    fe25519_cswap(z2, z3, swap);

    fe25519_invert(z2, z2);
    fe25519_mul(x2, x2, z2);

    return fe25519_iszero(x2);
}

int
crypto_core_curve25519_is_valid_point(const unsigned char *p)
{
    fe25519 t;
    fe25519 x;
    fe25519 x2;
    fe25519 Ax;
    fe25519 one;
    fe25519 x2_Ax_1;
    fe25519 y2;

    if (is_canonical(p) == 0 || has_small_order(p) != 0) {
        return 0;
    }
    /* y2 = x3 + Ax2 + x = x(x2 + Ax + 1) */
    fe25519_frombytes(x, p);
    fe25519_sq(x2, x);
    fe25519_mul(Ax, curve25519_A, x);
    fe25519_1(one);
    fe25519_add(x2_Ax_1, x2, Ax);
    fe25519_add(x2_Ax_1, x2_Ax_1, one);
    fe25519_mul(y2, x, x2_Ax_1);

    /* Legendre symbol */
    fe25519_pow22523(t, y2);
    fe25519_sq(t, t);
    fe25519_sq(t, t);
    fe25519_mul(t, t, y2);
    fe25519_mul(t, t, y2);

    /* No roots if the legendre symbol is not 1 */
    if (fe25519_isnegative(t) == 0) {
        return 0;
    }

    /* Multiply by the group order, check that the result is the point at infinity */
    if (is_on_main_subgroup(x) == 0) {
        return 0;
    }
    return 1;
}
