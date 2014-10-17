/* Copyritht (c) 2012-2014, Yann Collet
 * Copyright (c) 2014, Vsevolod Stakhov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *       * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *       * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef HASH_XXHASH_H_
#define HASH_XXHASH_H_

/*
 * Here we define XXHash fast hashing algorithm:
 * https://code.google.com/p/xxhash
 */
#ifdef _MSC_VER  // Visual Studio
#  pragma warning(disable : 4127)      // disable: C4127: conditional expression is constant
#endif

#ifdef _MSC_VER    // Visual Studio
#  define FORCE_INLINE static __forceinline
#else
#  ifdef __GNUC__
#    define FORCE_INLINE static inline __attribute__((always_inline))
#  else
#    define FORCE_INLINE static inline
#  endif
#endif

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   // C99
# include <stdint.h>
  typedef uint8_t  BYTE;
  typedef uint16_t U16;
  typedef uint32_t U32;
  typedef  int32_t S32;
  typedef uint64_t U64;
#else
  typedef unsigned char      BYTE;
  typedef unsigned short     U16;
  typedef unsigned int       U32;
  typedef   signed int       S32;
  typedef unsigned long long U64;
#endif
#if defined(__GNUC__)  && !defined(XXH_USE_UNALIGNED_ACCESS)
#  define _PACKED __attribute__ ((packed))
#else
#  define _PACKED
#endif

#if !defined(XXH_USE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  ifdef __IBMC__
#    pragma pack(1)
#  else
#    pragma pack(push, 1)
#  endif
#endif

typedef struct _U32_S { U32 v; } _PACKED U32_S;
typedef struct _U64_S { U64 v; } _PACKED U64_S;

#if !defined(XXH_USE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  pragma pack(pop)
#endif

#define A32(x) (((U32_S *)(x))->v)
#define A64(x) (((U64_S *)(x))->v)


//***************************************
// Compiler-specific Functions and Macros
//***************************************
#define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

// Note : although _rotl exists for minGW (GCC under windows), performance seems poor
#if defined(_MSC_VER)
#  define XXH_rotl32(x,r) _rotl(x,r)
#  define XXH_rotl64(x,r) _rotl64(x,r)
#else
#  define XXH_rotl32(x,r) ((x << r) | (x >> (32 - r)))
#  define XXH_rotl64(x,r) ((x << r) | (x >> (64 - r)))
#endif

#if defined(_MSC_VER)     // Visual Studio
#  define XXH_swap32 _byteswap_ulong
#  define XXH_swap64 _byteswap_uint64
#elif GCC_VERSION >= 403
#  define XXH_swap32 __builtin_bswap32
#  define XXH_swap64 __builtin_bswap64
#else
static inline U32 XXH_swap32 (U32 x) {
    return  ((x << 24) & 0xff000000 ) |
                        ((x <<  8) & 0x00ff0000 ) |
                        ((x >>  8) & 0x0000ff00 ) |
                        ((x >> 24) & 0x000000ff );}
static inline U64 XXH_swap64 (U64 x) {
    return  ((x << 56) & 0xff00000000000000ULL) |
            ((x << 40) & 0x00ff000000000000ULL) |
            ((x << 24) & 0x0000ff0000000000ULL) |
            ((x << 8)  & 0x000000ff00000000ULL) |
            ((x >> 8)  & 0x00000000ff000000ULL) |
            ((x >> 24) & 0x0000000000ff0000ULL) |
            ((x >> 40) & 0x000000000000ff00ULL) |
            ((x >> 56) & 0x00000000000000ffULL);}
#endif
//**************************************
// Constants
//**************************************
#define PRIME32_1   2654435761U
#define PRIME32_2   2246822519U
#define PRIME32_3   3266489917U
#define PRIME32_4    668265263U
#define PRIME32_5    374761393U

#define HASH_INIT_XXHASH(type, field)                                          \
  struct _hash_xxh_state_##type##_##field {                               \
	  U64 total_len;                                                             \
    U32 seed;                                                                  \
    U32 v1;                                                                    \
    U32 v2;                                                                    \
    U32 v3;                                                                    \
    U32 v4;                                                                    \
    int memsize;                                                               \
    char memory[16];                                                           \
  };                                                                           \
  static void* _HU_FUNCTION(_hash_xxh_##type##_##field##_init)(void *d, unsigned seed, \
    void *space, unsigned _HU(spacelen)) \
  { \
    struct _hash_xxh_state_##type##_##field *s = (struct _hash_xxh_state_##type##_##field *)space; \
    s->seed = seed; \
    s->v1 = seed + PRIME32_1 + PRIME32_2; \
    s->v2 = seed + PRIME32_2; \
    s->v3 = seed + 0; \
    s->v4 = seed - PRIME32_1; \
    s->total_len = 0; \
    s->memsize = 0; \
    return space; \
  }                                                                            \
  static void _HU_FUNCTION(_hash_xxh_##type##_##field##_update)(void *s, const unsigned char *input, size_t len, void *_HU(d)) \
  {                                                                            \
    struct _hash_xxh_state_##type##_##field * state = (struct _hash_xxh_state_##type##_##field *) s; \
    const BYTE* p = (const BYTE*)input; \
    const BYTE* const bEnd = p + len; \
    state->total_len += len; \
    if (state->memsize + len < 16)   \
    { \
      memcpy(state->memory + state->memsize, input, len); \
      state->memsize +=  len; \
      return; \
    } \
    if (state->memsize) \
    { \
      memcpy(state->memory + state->memsize, input, 16-state->memsize); \
      { \
        const U32* p32 = (const U32*)state->memory; \
        state->v1 += A32(p32) * PRIME32_2; state->v1 = XXH_rotl32(state->v1, 13); state->v1 *= PRIME32_1; p32++; \
        state->v2 += A32(p32) * PRIME32_2; state->v2 = XXH_rotl32(state->v2, 13); state->v2 *= PRIME32_1; p32++; \
        state->v3 += A32(p32) * PRIME32_2; state->v3 = XXH_rotl32(state->v3, 13); state->v3 *= PRIME32_1; p32++; \
        state->v4 += A32(p32) * PRIME32_2; state->v4 = XXH_rotl32(state->v4, 13); state->v4 *= PRIME32_1; p32++; \
      } \
      p += 16-state->memsize; \
      state->memsize = 0; \
    } \
    if (p <= bEnd-16) \
    { \
      const BYTE* const limit = bEnd - 16; \
      U32 v1 = state->v1; \
      U32 v2 = state->v2; \
      U32 v3 = state->v3; \
      U32 v4 = state->v4; \
      do \
      { \
        v1 += A32((const U32*)p) * PRIME32_2; v1 = XXH_rotl32(v1, 13); v1 *= PRIME32_1; p+=4; \
        v2 += A32((const U32*)p) * PRIME32_2; v2 = XXH_rotl32(v2, 13); v2 *= PRIME32_1; p+=4; \
        v3 += A32((const U32*)p) * PRIME32_2; v3 = XXH_rotl32(v3, 13); v3 *= PRIME32_1; p+=4; \
        v4 += A32((const U32*)p) * PRIME32_2; v4 = XXH_rotl32(v4, 13); v4 *= PRIME32_1; p+=4; \
      } while (p<=limit); \
      state->v1 = v1; \
      state->v2 = v2; \
      state->v3 = v3; \
      state->v4 = v4; \
    } \
    if (p < bEnd) \
    { \
      memcpy(state->memory, p, bEnd-p); \
      state->memsize = (int)(bEnd-p); \
    } \
  } \
  HASH_TYPE _HU_FUNCTION(_hash_xxh_##type##_##field##_final)(void *s, void * _HU(d)) \
  { \
    struct _hash_xxh_state_##type##_##field * state = (struct _hash_xxh_state_##type##_##field *) s; \
    const BYTE * p = (const BYTE*)state->memory; \
    BYTE* bEnd = (BYTE*)state->memory + state->memsize; \
    U32 h32; \
    if (state->total_len >= 16) \
    { \
      h32 = XXH_rotl32(state->v1, 1) + XXH_rotl32(state->v2, 7) + XXH_rotl32(state->v3, 12) + XXH_rotl32(state->v4, 18); \
    } \
    else \
    { \
      h32  = state->seed + PRIME32_5; \
    } \
    h32 += (U32) state->total_len; \
    while (p+4<=bEnd) \
    { \
      h32 += A32((const U32*)p) * PRIME32_3; \
      h32  = XXH_rotl32(h32, 17) * PRIME32_4; \
      p+=4; \
    } \
    while (p<bEnd) \
    { \
      h32 += (*p) * PRIME32_5; \
      h32 = XXH_rotl32(h32, 11) * PRIME32_1; \
      p++; \
    } \
    h32 ^= h32 >> 15; \
    h32 *= PRIME32_2; \
    h32 ^= h32 >> 13; \
    h32 *= PRIME32_3; \
    h32 ^= h32 >> 16; \
    return h32; \
  } \
  static _hash_generic_hash_t _hash_xxhash_##type##_##field = { \
   .hash_init = &_hash_xxh_##type##_##field##_init, \
   .hash_update = &_hash_xxh_##type##_##field##_update, \
   .hash_final = &_hash_xxh_##type##_##field##_final, \
   .d = NULL \
  }

#define _HASH_USE_XXHASH

#endif /* HASH_XXHASH_H_ */
