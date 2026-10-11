/*
lonesha256.h - Portable, endian-proof, single-file, single-function sha256 implementation, originally based on LibTomCrypt

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring
rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software.
If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

/*
lonesha256 supports the following configurations:
#define LONESHA256_EXTERN
    Default, should be used when using lonesha256 in multiple compilation units within the same project.
#define LONESHA256_IMPLEMENTATION
    Must be defined in exactly one source file within a project for lonesha256 to be found by the linker.
#define LONESHA256_STATIC
    Defines functions as static, useful if lonesha256 is only used in a single compilation unit.
#define LONESHA256_STDIO <n>
    Enables file and stream hashing functions (lonesha256_file, lonesha256_fh) which depend on <stdio.h>.
    Define to the chunk size in bytes to be used when reading from file.
    If defined without a value, buffer size defaults to BUFSIZ.

Functions:
(static|extern) int lonesha256 (unsigned char out[32], const void* in, size_t len)
    writes the sha256 hash of the first "len" bytes in buffer "in" to buffer "out"
    returns 0 on success, non-0 to indicate error

(static|extern) int lonesha256_init (lonesha256_ctx* ctx)
    initializes the sha256 context structure
    returns 0 on success, non-0 to indicate error

(static|extern) int lonesha256_update (lonesha256_ctx* ctx, const void* in, size_t len)
    processes "len" bytes of input data
    returns 0 on success, non-0 to indicate error

(static|extern) int lonesha256_final (lonesha256_ctx* ctx, unsigned char out[32])
    finalizes the hash and writes the 32-byte digest to buffer "out"
    returns 0 on success, non-0 to indicate error

(static|extern) int lonesha256_fh (unsigned char out[32], FILE* fp)
    [LONESHA256_STDIO] writes the sha256 hash of open stream "fp" to buffer "out"
    returns 0 on success, non-0 to indicate error

(static|extern) int lonesha256_file (unsigned char out[32], const char* filename)
    [LONESHA256_STDIO] writes the sha256 hash of file "filename" to buffer "out"
    returns 0 on success, non-0 to indicate error

Error Codes:
    1 - Illegal Arguments
    2 - File could not be opened (lonesha256_file)
    3 - Read error (lonesha256_file, lonesha256_fh)
*/

/* header section */
#ifndef LONESHA256_H
#define LONESHA256_H

/* process configuration */
#ifdef LONESHA256_STATIC
    #define LONESHA256_IMPLEMENTATION
    #define LSHA256DEF static
#else /* LONESHA256_EXTERN */
    #define LSHA256DEF extern
#endif

/* includes */
#include <stddef.h> /* size_t */
#include <stdint.h> /* uint32_t, uint64_t */

#ifdef LONESHA256_STDIO
#include <stdio.h> /* FILE*, fopen, fread, fclose, ferror, BUFSIZ */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* context structure */
typedef struct {
    uint64_t length;
    uint32_t state[8];
    size_t curlen;
    unsigned char buf[64];
} lonesha256_ctx;

/* core functions */
LSHA256DEF int lonesha256(unsigned char[32], const void*, size_t);
LSHA256DEF int lonesha256_init(lonesha256_ctx*);
LSHA256DEF int lonesha256_update(lonesha256_ctx*, const void*, size_t);
LSHA256DEF int lonesha256_final(lonesha256_ctx*, unsigned char[32]);

#ifdef LONESHA256_STDIO
/* file functions */
LSHA256DEF int lonesha256_fh(unsigned char[32], FILE*);
LSHA256DEF int lonesha256_file(unsigned char[32], const char*);
#endif

#ifdef __cplusplus
}
#endif

#endif

/* implementation section */
#ifdef LONESHA256_IMPLEMENTATION
#undef LONESHA256_IMPLEMENTATION

/* macros */
#define S(x, n) (((((uint32_t)(x)&0xFFFFFFFFU)>>(uint32_t)((n)&31))|((uint32_t)(x)<<(uint32_t)((32-((n)&31))&31)))&0xFFFFFFFFU)
#define R(x, n) (((x)&0xFFFFFFFFU)>>(n))
#define Gamma0(x) (S(x, 7) ^ S(x, 18) ^ R(x, 3))
#define Gamma1(x) (S(x, 17) ^ S(x, 19) ^ R(x, 10))
#define RND(a,b,c,d,e,f,g,h,i) do { \
    t0 = h + (S(e, 6) ^ S(e, 11) ^ S(e, 25)) + (g ^ (e & (f ^ g))) + K[i] + W[i]; \
    t1 = (S(a, 2) ^ S(a, 13) ^ S(a, 22)) + (((a | b) & c) | (a & b)); \
    d += t0; \
    h  = t0 + t1; \
} while ( 0 )
#define STORE32H(x, y) do { \
    (y)[0] = (unsigned char)(((x)>>24)&255); (y)[1] = (unsigned char)(((x)>>16)&255); \
    (y)[2] = (unsigned char)(((x)>>8)&255); (y)[3] = (unsigned char)((x)&255); \
} while ( 0 )
#define LOAD32H(x, y) do { \
    x = ((uint32_t)((y)[0]&255)<<24)|((uint32_t)((y)[1]&255)<<16)|((uint32_t)((y)[2]&255)<<8)|((uint32_t)((y)[3]&255)); \
} while ( 0 )
#define STORE64H(x, y) do { \
    (y)[0] = (unsigned char)(((x)>>56)&255); (y)[1] = (unsigned char)(((x)>>48)&255); \
    (y)[2] = (unsigned char)(((x)>>40)&255); (y)[3] = (unsigned char)(((x)>>32)&255); \
    (y)[4] = (unsigned char)(((x)>>24)&255); (y)[5] = (unsigned char)(((x)>>16)&255); \
    (y)[6] = (unsigned char)(((x)>>8)&255); (y)[7] = (unsigned char)((x)&255); \
} while ( 0 )
/* includes */
#include <string.h> /* memcpy, memset */

/* internal compression function */
static void lonesha256_compress (uint32_t state[8], const unsigned char buff[64]) {
    static const uint32_t K[64] = {
        0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
        0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
        0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
        0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
        0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
        0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
        0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
        0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
        0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
        0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
        0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
        0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
        0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
        0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
        0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
        0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
    };
    uint32_t S[8], W[64], t0, t1, t;
    int i;

    for (i = 0; i < 8; i++) S[i] = state[i];
    for (i = 0; i < 16; i++) LOAD32H(W[i], buff + (4*i));
    for (i = 16; i < 64; i++) W[i] = Gamma1(W[i-2]) + W[i-7] + Gamma0(W[i-15]) + W[i-16];
    for (i = 0; i < 64; i++) {
        RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],i);
        t = S[7]; S[7] = S[6]; S[6] = S[5]; S[5] = S[4];
        S[4] = S[3]; S[3] = S[2]; S[2] = S[1]; S[1] = S[0]; S[0] = t;
    }
    for (i = 0; i < 8; i++) state[i] = state[i] + S[i];
}

/* lonesha256_init function */
LSHA256DEF int lonesha256_init (lonesha256_ctx* ctx) {
    if (ctx == NULL) {
        return 1;
    }
    ctx->length = 0;
    ctx->curlen = 0;
    ctx->state[0] = 0x6A09E667U;
    ctx->state[1] = 0xBB67AE85U;
    ctx->state[2] = 0x3C6EF372U;
    ctx->state[3] = 0xA54FF53AU;
    ctx->state[4] = 0x510E527FU;
    ctx->state[5] = 0x9B05688CU;
    ctx->state[6] = 0x1F83D9ABU;
    ctx->state[7] = 0x5BE0CD19U;
    return 0;
}

/* lonesha256_update function */
LSHA256DEF int lonesha256_update (lonesha256_ctx* ctx, const void* in, size_t len) {
    const unsigned char* src = (const unsigned char*)in;

    if (ctx == NULL || in == NULL) {
        return 1;
    }
    if (len == 0) {
        return 0;
    }
    if (ctx->length + len < ctx->length) {
        return 1;
    }

    ctx->length += len;

    if (ctx->curlen > 0) {
        size_t n = 64 - ctx->curlen;
        if (len < n) {
            n = len;
        }
        memcpy(ctx->buf + ctx->curlen, src, n);
        ctx->curlen += n;
        src += n;
        len -= n;
        if (ctx->curlen == 64) {
            lonesha256_compress(ctx->state, ctx->buf);
            ctx->curlen = 0;
        }
    }

    while (len >= 64) {
        lonesha256_compress(ctx->state, src);
        src += 64;
        len -= 64;
    }

    if (len > 0) {
        memcpy(ctx->buf, src, len);
        ctx->curlen = len;
    }

    return 0;
}

/* lonesha256_final function */
LSHA256DEF int lonesha256_final (lonesha256_ctx* ctx, unsigned char out[32]) {
    int i;

    if (ctx == NULL || out == NULL || (ctx->length >> 61) != 0) {
        return 1;
    }

    ctx->buf[ctx->curlen++] = 0x80;
    if (ctx->curlen > 56) {
        while (ctx->curlen < 64) ctx->buf[ctx->curlen++] = 0;
        lonesha256_compress(ctx->state, ctx->buf);
        ctx->curlen = 0;
    }
    while (ctx->curlen < 56) ctx->buf[ctx->curlen++] = 0;
    STORE64H(ctx->length << 3, ctx->buf + 56);
    lonesha256_compress(ctx->state, ctx->buf);

    for (i = 0; i < 8; i++) {
        STORE32H(ctx->state[i], out + 4*i);
    }
    memset(ctx, 0, sizeof(*ctx));
    return 0;
}

/* lonesha256 function */
LSHA256DEF int lonesha256 (unsigned char out[32], const void* in, size_t len) {
    lonesha256_ctx ctx;

    if (in == NULL) {
        return 1;
    }
    lonesha256_init(&ctx);
    lonesha256_update(&ctx, in, len);
    return lonesha256_final(&ctx, out);
}

#ifdef LONESHA256_STDIO
/* lonesha256_fh function */
LSHA256DEF int lonesha256_fh (unsigned char out[32], FILE* fp) {
    lonesha256_ctx ctx;
#if LONESHA256_STDIO + 0 == 0
    unsigned char buf[BUFSIZ];
#else
    unsigned char buf[LONESHA256_STDIO];
#endif
    size_t n;

    if (fp == NULL || out == NULL) {
        return 1;
    }

    lonesha256_init(&ctx);
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        lonesha256_update(&ctx, buf, n);
    }
    if (ferror(fp)) {
        memset(&ctx, 0, sizeof(ctx));
        return 3;
    }
    return lonesha256_final(&ctx, out);
}

/* lonesha256_file function */
LSHA256DEF int lonesha256_file (unsigned char out[32], const char* filename) {
    FILE* fp;
    int res;

    if (filename == NULL || out == NULL) {
        return 1;
    }

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        return 2;
    }

    res = lonesha256_fh(out, fp);
    fclose(fp);
    return res;
}
#endif

/* undef macros */
#undef S
#undef R
#undef Gamma0
#undef Gamma1
#undef RND
#undef STORE32H
#undef LOAD32H
#undef STORE64H

#endif
