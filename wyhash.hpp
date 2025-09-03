#pragma once

#include <cstdint>
#include <string.h>

#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
#define _likely_(x) __builtin_expect(x, 1)
#define _unlikely_(x) __builtin_expect(x, 0)
#else
#define _likely_(x) (x)
#define _unlikely_(x) (x)
#endif

#if (WYHASH_LITTLE_ENDIAN)
static inline uint64_t _wyr8(const uint8_t *p) {
    uint64_t v;
    memcpy(&v, p, 8);
    return v;
}

static inline uint64_t _wyr4(const uint8_t *p) {
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}

#elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
static inline uint64_t _wyr8(const uint8_t *p) {
    uint64_t v;
    memcpy(&v, p, 8);
    return __builtin_bswap64(v);
}

static inline uint64_t _wyr4(const uint8_t *p) {
    uint32_t v;
    memcpy(&v, p, 4);
    return __builtin_bswap32(v);
}

#elif defined(_MSC_VER)
static inline uint64_t _wyr8(const uint8_t *p) {
    uint64_t v;
    memcpy(&v, p, 8);
    return _byteswap_uint64(v);
}

static inline uint64_t _wyr4(const uint8_t *p) {
    uint32_t v;
    memcpy(&v, p, 4);
    return _byteswap_ulong(v);
}

#else
static inline uint64_t _wyr8(const uint8_t *p) {
    uint64_t v;
    memcpy(&v, p, 8);
    return (((v >> 56) & 0xff) | ((v >> 40) & 0xff00) | ((v >> 24) & 0xff0000) |
            ((v >> 8) & 0xff000000) | ((v << 8) & 0xff00000000) |
            ((v << 24) & 0xff0000000000) | ((v << 40) & 0xff000000000000) |
            ((v << 56) & 0xff00000000000000));
}

static inline uint64_t _wyr4(const uint8_t *p) {
    uint32_t v;
    memcpy(&v, p, 4);
    return (((v >> 24) & 0xff) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) |
            ((v << 24) & 0xff000000));
}

#endif
static inline uint64_t _wyrot(uint64_t x) { return (x >> 32) | (x << 32); }
static inline void _wymum(uint64_t *A, uint64_t *B) {
#if (WYHASH_32BIT_MUM)
    uint64_t hh = (*A >> 32) * (*B >> 32), hl = (*A >> 32) * (uint32_t)*B,
             lh = (uint32_t)*A * (*B >> 32),
             ll = (uint64_t)(uint32_t)*A * (uint32_t)*B;
#if (WYHASH_CONDOM > 1)
    *A ^= _wyrot(hl) ^ hh;
    *B ^= _wyrot(lh) ^ ll;
#else
    *A = _wyrot(hl) ^ hh;
    *B = _wyrot(lh) ^ ll;
#endif
#elif defined(__SIZEOF_INT128__)
    __uint128_t r = *A;
    r *= *B;
#if (WYHASH_CONDOM > 1)
    *A ^= (uint64_t)r;
    *B ^= (uint64_t)(r >> 64);
#else
    *A = (uint64_t)r;
    *B = (uint64_t)(r >> 64);
#endif
#elif defined(_MSC_VER) && defined(_M_X64)
#if (WYHASH_CONDOM > 1)
    uint64_t a, b;
    a = _umul128(*A, *B, &b);
    *A ^= a;
    *B ^= b;
#else
    *A = _umul128(*A, *B, B);
#endif
#else
    uint64_t ha = *A >> 32, hb = *B >> 32, la = (uint32_t)*A, lb = (uint32_t)*B,
             hi, lo;
    uint64_t rh = ha * hb, rm0 = ha * lb, rm1 = hb * la, rl = la * lb,
             t = rl + (rm0 << 32), c = t < rl;
    lo = t + (rm1 << 32);
    c += lo < t;
    hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;
#if (WYHASH_CONDOM > 1)
    *A ^= lo;
    *B ^= hi;
#else
    *A = lo;
    *B = hi;
#endif
#endif
}

static inline uint64_t _wyr3(const uint8_t *p, size_t k) {
    return (((uint64_t)p[0]) << 16) | (((uint64_t)p[k >> 1]) << 8) | p[k - 1];
}

static inline uint64_t _wymix(uint64_t A, uint64_t B) {
    _wymum(&A, &B);
    return A ^ B;
}

static inline uint64_t wyhash(const void *key, size_t len, uint64_t seed,
                              const uint64_t *secret) {
    const uint8_t *p = (const uint8_t *)key;
    seed ^= _wymix(seed ^ secret[0], secret[1]);
    uint64_t a, b;
    if (_likely_(len <= 16)) {
        if (_likely_(len >= 4)) {
            a = (_wyr4(p) << 32) | _wyr4(p + ((len >> 3) << 2));
            b = (_wyr4(p + len - 4) << 32) |
                _wyr4(p + len - 4 - ((len >> 3) << 2));
        } else if (_likely_(len > 0)) {
            a = _wyr3(p, len);
            b = 0;
        } else
            a = b = 0;
    } else {
        size_t i = len;
        if (_unlikely_(i >= 48)) {
            uint64_t see1 = seed, see2 = seed;
            do {
                seed = _wymix(_wyr8(p) ^ secret[1], _wyr8(p + 8) ^ seed);
                see1 = _wymix(_wyr8(p + 16) ^ secret[2], _wyr8(p + 24) ^ see1);
                see2 = _wymix(_wyr8(p + 32) ^ secret[3], _wyr8(p + 40) ^ see2);
                p += 48;
                i -= 48;
            } while (_likely_(i >= 48));
            seed ^= see1 ^ see2;
        }
        while (_unlikely_(i > 16)) {
            seed = _wymix(_wyr8(p) ^ secret[1], _wyr8(p + 8) ^ seed);
            i -= 16;
            p += 16;
        }
        a = _wyr8(p + i - 16);
        b = _wyr8(p + i - 8);
    }
    a ^= secret[1];
    b ^= seed;
    _wymum(&a, &b);
    return _wymix(a ^ secret[0] ^ len, b ^ secret[1]);
}

static inline uint64_t wyrand(uint64_t *seed) {
    *seed += 0x2d358dccaa6c78a5ull;
    return _wymix(*seed, *seed ^ 0x8bb84b93962eacc9ull);
}

static inline unsigned long long
mul_mod(unsigned long long a, unsigned long long b, unsigned long long m) {
    unsigned long long r = 0;
    while (b) {
        if (b & 1) {
            unsigned long long r2 = r + a;
            if (r2 < r)
                r2 -= m;
            r = r2 % m;
        }
        b >>= 1;
        if (b) {
            unsigned long long a2 = a + a;
            if (a2 < a)
                a2 -= m;
            a = a2 % m;
        }
    }
    return r;
}

static inline unsigned long long
pow_mod(unsigned long long a, unsigned long long b, unsigned long long m) {
    unsigned long long r = 1;
    while (b) {
        if (b & 1)
            r = mul_mod(r, a, m);
        b >>= 1;
        if (b)
            a = mul_mod(a, a, m);
    }
    return r;
}

inline unsigned sprp(unsigned long long n, unsigned long long a) {
    unsigned long long d = n - 1;
    unsigned char s = 0;
    while (!(d & 0xff)) {
        d >>= 8;
        s += 8;
    }
    if (!(d & 0xf)) {
        d >>= 4;
        s += 4;
    }
    if (!(d & 0x3)) {
        d >>= 2;
        s += 2;
    }
    if (!(d & 0x1)) {
        d >>= 1;
        s += 1;
    }
    unsigned long long b = pow_mod(a, d, n);
    if ((b == 1) || (b == (n - 1)))
        return 1;
    unsigned char r;
    for (r = 1; r < s; r++) {
        b = mul_mod(b, b, n);
        if (b <= 1)
            return 0;
        if (b == (n - 1))
            return 1;
    }
    return 0;
}

inline unsigned is_prime(unsigned long long n) {
    if (n < 2 || !(n & 1))
        return 0;
    if (n < 4)
        return 1;
    if (!sprp(n, 2))
        return 0;
    if (n < 2047)
        return 1;
    if (!sprp(n, 3))
        return 0;
    if (!sprp(n, 5))
        return 0;
    if (!sprp(n, 7))
        return 0;
    if (!sprp(n, 11))
        return 0;
    if (!sprp(n, 13))
        return 0;
    if (!sprp(n, 17))
        return 0;
    if (!sprp(n, 19))
        return 0;
    if (!sprp(n, 23))
        return 0;
    if (!sprp(n, 29))
        return 0;
    if (!sprp(n, 31))
        return 0;
    if (!sprp(n, 37))
        return 0;
    return 1;
}

static inline void make_secret(uint64_t seed, uint64_t *secret) {
    uint8_t c[] = {15,  23,  27,  29,  30,  39,  43,  45,  46,  51,  53,  54,
                   57,  58,  60,  71,  75,  77,  78,  83,  85,  86,  89,  90,
                   92,  99,  101, 102, 105, 106, 108, 113, 114, 116, 120, 135,
                   139, 141, 142, 147, 149, 150, 153, 154, 156, 163, 165, 166,
                   169, 170, 172, 177, 178, 180, 184, 195, 197, 198, 201, 202,
                   204, 209, 210, 212, 216, 225, 226, 228, 232, 240};
    for (size_t i = 0; i < 4; i++) {
        uint8_t ok;
        do {
            ok = 1;
            secret[i] = 0;
            for (size_t j = 0; j < 64; j += 8)
                secret[i] |= ((uint64_t)c[wyrand(&seed) % sizeof(c)]) << j;
            if (secret[i] % 2 == 0) {
                ok = 0;
                continue;
            }
            for (size_t j = 0; j < i; j++) {
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
                if (__builtin_popcountll(secret[j] ^ secret[i]) != 32) {
                    ok = 0;
                    break;
                }
#elif defined(_MSC_VER) && defined(_M_X64)
                if (_mm_popcnt_u64(secret[j] ^ secret[i]) != 32) {
                    ok = 0;
                    break;
                }
#else
                // manual popcount
                uint64_t x = secret[j] ^ secret[i];
                x -= (x >> 1) & 0x5555555555555555;
                x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
                x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f;
                x = (x * 0x0101010101010101) >> 56;
                if (x != 32) {
                    ok = 0;
                    break;
                }
#endif
            }
            if (ok && !is_prime(secret[i]))
                ok = 0;
        } while (!ok);
    }
}
