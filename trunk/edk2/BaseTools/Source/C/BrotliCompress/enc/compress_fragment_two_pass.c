/* Copyright 2015 Google Inc. All Rights Reserved.

   Distributed under MIT license.
   See file LICENSE for detail or copy at https://opensource.org/licenses/MIT
*/

/* Function for fast encoding of an input fragment, independently from the input
   history. This function uses two-pass processing: in the first pass we save
   the found backward matches and literal bytes into a buffer, and in the
   second pass we emit them into the bit stream using prefix codes built based
   on the actual command and literal byte histograms. */

#include "./compress_fragment_two_pass.h"

#include <string.h>  /* memcmp, memcpy, memset */

#include "../common/types.h"
#include "./bit_cost.h"
#include "./brotli_bit_stream.h"
#include "./entropy_encode.h"
#include "./fast_log.h"
#include "./find_match_length.h"
#include "./memory.h"
#include "./port.h"
#include "./write_bits.h"


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* kHashMul32 multiplier has these properties:
   * The multiplier must be odd. Otherwise we may lose the highest bit.
   * No long streaks of 1s or 0s.
   * There is no effort to ensure that it is a prime, the oddity is enough
     for this use.
   * The number has been tuned heuristically against compression benchmarks. */
static const uint32_t kHashMul32 = 0x1e35a7bd;

static BROTLI_INLINE uint32_t Hash(const uint8_t* p, size_t shift) {
  const uint64_t h = (BROTLI_UNALIGNED_LOAD64(p) << 16) * kHashMul32;
  return (uint32_t)(h >> shift);
}

static BROTLI_INLINE uint32_t HashBytesAtOffset(
    uint64_t v, int offset, size_t shift) {
  assert(offset >= 0);
  assert(offset <= 2);
  {
    const uint64_t h = ((v >> (8 * offset)) << 16) * kHashMul32;
    return (uint32_t)(h >> shift);
  }
}

static BROTLI_INLINE BROTLI_BOOL IsMatch(const uint8_t* p1, const uint8_t* p2) {
  return TO_BROTLI_BOOL(
      BROTLI_UNALIGNED_LOAD32(p1) == BROTLI_UNALIGNED_LOAD32(p2) &&
      p1[4] == p2[4] &&
      p1[5] == p2[5]);
}

/* Builds a command and distance prefix code (each 64 symbols) into "depth" and
   "bits" based on "histogram" and stores it into the bit stream. */
static void BuildAndStoreCommandPrefixCode(
    const uint32_t histogram[128],
    uint8_t depth[128], uint16_t bits[128],
    size_t* storage_ix, uint8_t* storage) {
  /* Tree size for building a tree over 64 symbols is 2 * 64 + 1. */
  HuffmanTree tree[129];
  uint8_t cmd_depth[BROTLI_NUM_COMMAND_SYMBOLS] = { 0 };
  uint16_t cmd_bits[64];
  BrotliCreateHuffmanTree(histogram, 64, 15, tree, depth);
  BrotliCreateHuffmanTree(&histogram[64], 64, 14, tree, &depth[64]);
  /* We have to jump through a few hoopes here in order to compute
     the command bits because the symbols are in a different order than in
     the full alphabet. This looks complicated, but having the symbols
     in this order in the command bits saves a few branches in the Emit*
     functions. */
  memcpy(cmd_depth, depth + 24, 24);
  memcpy(cmd_depth + 24, depth, 8);
  memcpy(cmd_depth + 32, depth + 48, 8);
  memcpy(cmd_depth + 40, depth + 8, 8);
  memcpy(cmd_depth + 48, depth + 56, 8);
  memcpy(cmd_depth + 56, depth + 16, 8);
  BrotliConvertBitDepthsToSymbols(cmd_depth, 64, cmd_bits);
  memcpy(bits, cmd_bits + 24, 16);
  memcpy(bits + 8, cmd_bits + 40, 16);
  memcpy(bits + 16, cmd_bits + 56, 16);
  memcpy(bits + 24, cmd_bits, 48);
  memcpy(bits + 48, cmd_bits + 32, 16);
  memcpy(bits + 56, cmd_bits + 48, 16);
  BrotliConvertBitDepthsToSymbols(&depth[64], 64, &bits[64]);
  {
    /* Create the bit length array for the full command alphabet. */
    size_t i;
    memset(cmd_depth, 0, 64);  /* only 64 first values were used */
    memcpy(cmd_depth, depth + 24, 8);
    memcpy(cmd_depth + 64, depth + 32, 8);
    memcpy(cmd_depth + 128, depth + 40, 8);
    memcpy(cmd_depth + 192, depth + 48, 8);
    memcpy(cmd_depth + 384, depth + 56, 8);
    for (i = 0; i < 8; ++i) {
      cmd_depth[128 + 8 * i] = depth[i];
      cmd_depth[256 + 8 * i] = depth[8 + i];
      cmd_depth[448 + 8 * i] = depth[16 + i];
    }
    BrotliStoreHuffmanTree(
        cmd_depth, BROTLI_NUM_COMMAND_SYMBOLS, tree, storage_ix, storage);
  }
  BrotliStoreHuffmanTree(&depth[64], 64, tree, storage_ix, storage);
}

static BROTLI_INLINE void EmitInsertLen(
    uint32_t insertlen, uint32_t** commands) {
  if (insertlen < 6) {
    **commands = insertlen;
  } else if (insertlen < 130) {
    const uint32_t tail = insertlen - 2;
    const uint32_t nbits = Log2FloorNonZero(tail) - 1u;
    const uint32_t prefix = tail >> nbits;
    const uint32_t inscode = (nbits << 1) + prefix + 2;
    const uint32_t extra = tail - (prefix << nbits);
    **commands = inscode | (extra << 8);
  } else if (insertlen < 2114) {
    const uint32_t tail = insertlen - 66;
    const uint32_t nbits = Log2FloorNonZero(tail);
    const uint32_t code = nbits + 10;
    const uint32_t extra = tail - (1u << nbits);
    **commands = code | (extra << 8);
  } else if (insertlen < 6210) {
    const uint32_t extra = insertlen - 2114;
    **commands = 21 | (extra << 8);
  } else if (insertlen < 22594) {
    const uint32_t extra = insertlen - 6210;
    **commands = 22 | (extra << 8);
  } else {
    const uint32_t extra = insertlen - 22594;
    **commands = 23 | (extra << 8);
  }
  ++(*commands);
}

static BROTLI_INLINE void EmitCopyLen(size_t copylen, uint32_t** commands) {
  if (copylen < 10) {
    **commands = (uint32_t)(copylen + 38);
  } else if (copylen < 134) {
    const size_t tail = copylen - 6;
    const size_t nbits = Log2FloorNonZero(tail) - 1;
    const size_t prefix = tail >> nbits;
    const size_t code = (nbits << 1) + prefix + 44;
    const size_t extra = tail - (prefix << nbits);
    **commands = (uint32_t)(code | (extra << 8));
  } else if (copylen < 2118) {
    const size_t tail = copylen - 70;
    const size_t nbits = Log2FloorNonZero(tail);
    const size_t code = nbits + 52;
    const size_t extra = tail - ((size_t)1 << nbits);
    **commands = (uint32_t)(code | (extra << 8));
  } else {
    const size_t extra = copylen - 2118;
    **commands = (uint32_t)(63 | (extra << 8));
  }
  ++(*commands);
}

static BROTLI_INLINE void EmitCopyLenLastDistance(
    size_t copylen, uint32_t** commands) {
  if (copylen < 12) {
    **commands = (uint32_t)(copylen + 20);
    ++(*commands);
  } else if (copylen < 72) {
    const size_t tail = copylen - 8;
    const size_t nbits = Log2FloorNonZero(tail) - 1;
    const size_t prefix = tail >> nbits;
    const size_t code = (nbits << 1) + prefix + 28;
    const size_t extra = tail - (prefix << nbits);
    **commands = (uint32_t)(code | (extra << 8));
    ++(*commands);
  } else if (copylen < 136) {
    const size_t tail = copylen - 8;
    const size_t code = (tail >> 5) + 54;
    const size_t extra = tail & 31;
    **commands = (uint32_t)(code | (extra << 8));
    ++(*commands);
    **commands = 64;
    ++(*commands);
  } else if (copylen < 2120) {
    const size_t tail = copylen - 72;
    const size_t nbits = Log2FloorNonZero(tail);
    const size_t code = nbits + 52;
    const size_t extra = tail - ((size_t)1 << nbits);
    **commands = (uint32_t)(code | (extra << 8));
    ++(*commands);
    **commands = 64;
    ++(*commands);
  } else {
    const size_t extra = copylen - 2120;
    **commands = (uint32_t)(63 | (extra << 8));
    ++(*commands);
    **commands = 64;
    ++(*commands);
  }
}

static BROTLI_INLINE void EmitDistance(uint32_t distance, uint32_t** commands) {
  uint32_t d = distance + 3;
  uint32_t nbits = Log2FloorNonZero(d) - 1;
  const uint32_t prefix = (d >> nbits) & 1;
  const uint32_t offset = (2 + prefix) << nbits;
  const uint32_t distcode = 2 * (nbits - 1) + prefix + 80;
  uint32_t extra = d - offset;
  **commands = distcode | (extra << 8);
  ++(*commands);
}

/* REQUIRES: len <= 1 << 20. */
static void BrotliStoreMetaBlockHeader(
    size_t len, BROTLI_BOOL is_uncompressed, size_t* storage_ix,
    uint8_t* storage) {
  /* ISLAST */
  BrotliWriteBits(1, 0, storage_ix, storage);
  if (len <= (1U << 16)) {
    /* MNIBBLES is 4 */
    BrotliWriteBits(2, 0, storage_ix, storage);
    BrotliWriteBits(16, len - 1, storage_ix, storage);
  } else {
    /* MNIBBLES is 5 */
    BrotliWriteBits(2, 1, storage_ix, storage);
    BrotliWriteBits(20, len - 1, storage_ix, storage);
  }
  /* ISUNCOMPRESSED */
  BrotliWriteBits(1, (uint64_t)is_uncompressed, storage_ix, storage);
}

static void CreateCommands(const uint8_t* input, size_t block_size,
    size_t input_size, const uint8_t* base_ip, int* table, size_t table_size,
    uint8_t** literals, uint32_t** commands) {
  /* "ip" is the input pointer. */
  const uint8_t* ip = input;
  const size_t shift = 64u - Log2FloorNonZero(table_size);
  const uint8_t* ip_end = input + block_size;
  /* "next_emit" is a pointer to the first byte that is not covered by a
     previous copy. Bytes between "next_emit" and the start of the next copy or
     the end of the input will be emitted as literal bytes. */
  const uint8_t* next_emit = input;

  int last_distance = -1;
  const size_t kInputMarginBytes = 16;
  const size_t kMinMatchLen = 6;

  assert(table_size);
  assert(table_size <= (1u << 31));
  /* table must be power of two */
  assert((table_size & (table_size - 1)) == 0);
  assert(table_size - 1 ==
      (size_t)(MAKE_UINT64_T(0xFFFFFFFF, 0xFFFFFF) >> shift));

  if (PREDICT_TRUE(block_size >= kInputMarginBytes)) {
    /* For the last block, we need to keep a 16 bytes margin so that we can be
       sure that all distances are at most window size - 16.
       For all other blocks, we only need to keep a margin of 5 bytes so that
       we don't go over the block size with a copy. */
    const size_t len_limit = BROTLI_MIN(size_t, block_size - kMinMatchLen,
                                        input_size - kInputMarginBytes);
    const uint8_t* ip_limit = input + len_limit;

    uint32_t next_hash;
    for (next_hash = Hash(++ip, shift); ; ) {
      /* Step 1: Scan forward in the input looking for a 6-byte-long match.
         If we get close to exhausting the input then goto emit_remainder.

         Heuristic match skipping: If 32 bytes are scanned with no matches
         found, start looking only at every other byte. If 32 more bytes are
         scanned, look at every third byte, etc.. When a match is found,
         immediately go back to looking at every byte. This is a small loss
         (~5% performance, ~0.1% density) for compressible data due to more
         bookkeeping, but for non-compressible data (such as JPEG) it's a huge
         win since the compressor quickly "realizes" the data is incompressible
         and doesn't bother looking for matches everywhere.

         The "skip" variable keeps track of how many bytes there are since the
         last match; dividing it by 32 (ie. right-shifting by five) gives the
         number of bytes to move ahead for each iteration. */
      uint32_t skip = 32;

      const uint8_t* next_ip = ip;
      const uint8_t* candidate;

      assert(next_emit < ip);

      do {
        uint32_t hash = next_hash;
        uint32_t bytes_between_hash_lookups = skip++ >> 5;
        ip = next_ip;
        assert(hash == Hash(ip, shift));
        next_ip = ip + bytes_between_hash_lookups;
        if (PREDICT_FALSE(next_ip > ip_limit)) {
          goto emit_remainder;
        }
        next_hash = Hash(next_ip, shift);
        candidate = ip - last_distance;
        if (IsMatch(ip, candidate)) {
          if (PREDICT_TRUE(candidate < ip)) {
            table[hash] = (int)(ip - base_ip);
            break;
          }
        }
        candidate = base_ip + table[hash];
        assert(candidate >= base_ip);
        assert(candidate < ip);

        table[hash] = (int)(ip - base_ip);
      } while (PREDICT_TRUE(!IsMatch(ip, candidate)));

      /* Step 2: Emit the found match together with the literal bytes from
         "next_emit", and then see if we can find a next macth immediately
         afterwards. Repeat until we find no match for the input
         without emitting some literal bytes. */

      {
        /* We have a 6-byte match at ip, and we need to emit bytes in
           [next_emit, ip). */
        const uint8_t* base = ip;
        size_t matched = 6 + FindMatchLengthWithLimit(
            candidate + 6, ip + 6, (size_t)(ip_end - ip) - 6);
        int distance = (int)(base - candidate);  /* > 0 */
        int insert = (int)(base - next_emit);
        ip += matched;
        assert(0 == memcmp(base, candidate, matched));
        EmitInsertLen((uint32_t)insert, commands);
        memcpy(*literals, next_emit, (size_t)insert);
        *literals += insert;
        if (distance == last_distance) {
          **commands = 64;
          ++(*commands);
        } else {
          EmitDistance((uint32_t)distance, commands);
          last_distance = distance;
        }
        EmitCopyLenLastDistance(matched, commands);

        next_emit = ip;
        if (PREDICT_FALSE(ip >= ip_limit)) {
          goto emit_remainder;
        }
        {
          /* We could immediately start working at ip now, but to improve
             compression we first update "table" with the hashes of some
             positions within the last copy. */
          uint64_t input_bytes = BROTLI_UNALIGNED_LOAD64(ip - 5);
          uint32_t prev_hash = HashBytesAtOffset(input_bytes, 0, shift);
          uint32_t cur_hash;
          table[prev_hash] = (int)(ip - base_ip - 5);
          prev_hash = HashBytesAtOffset(input_bytes, 1, shift);
          table[prev_hash] = (int)(ip - base_ip - 4);
          prev_hash = HashBytesAtOffset(input_bytes, 2, shift);
          table[prev_hash] = (int)(ip - base_ip - 3);
          input_bytes = BROTLI_UNALIGNED_LOAD64(ip - 2);
          cur_hash = HashBytesAtOffset(input_bytes, 2, shift);
          prev_hash = HashBytesAtOffset(input_bytes, 0, shift);
          table[prev_hash] = (int)(ip - base_ip - 2);
          prev_hash = HashBytesAtOffset(input_bytes, 1, shift);
          table[prev_hash] = (int)(ip - base_ip - 1);

          candidate = base_ip + table[cur_hash];
          table[cur_hash] = (int)(ip - base_ip);
        }
      }

      while (IsMatch(ip, candidate)) {
        /* We have a 6-byte match at ip, and no need to emit any
           literal bytes prior to ip. */
        const uint8_t* base = ip;
        size_t matched = 6 + FindMatchLengthWithLimit(
            candidate + 6, ip + 6, (size_t)(ip_end - ip) - 6);
        ip += matched;
        last_distance = (int)(base - candidate);  /* > 0 */
        assert(0 == memcmp(base, candidate, matched));
        EmitCopyLen(matched, commands);
        EmitDistance((uint32_t)last_distance, commands);

        next_emit = ip;
        if (PREDICT_FALSE(ip >= ip_limit)) {
          goto emit_remainder;
        }
        {
          /* We could immediately start working at ip now, but to improve
             compression we first update "table" with the hashes of some
             positions within the last copy. */
          uint64_t input_bytes = BROTLI_UNALIGNED_LOAD64(ip - 5);
          uint32_t prev_hash = HashBytesAtOffset(input_bytes, 0, shift);
          uint32_t cur_hash;
          table[prev_hash] = (int)(ip - base_ip - 5);
          prev_hash = HashBytesAtOffset(input_bytes, 1, shift);
          table[prev_hash] = (int)(ip - base_ip - 4);
          prev_hash = HashBytesAtOffset(input_bytes, 2, shift);
          table[prev_hash] = (int)(ip - base_ip - 3);
          input_bytes = BROTLI_UNALIGNED_LOAD64(ip - 2);
          cur_hash = HashBytesAtOffset(input_bytes, 2, shift);
          prev_hash = HashBytesAtOffset(input_bytes, 0, shift);
          table[prev_hash] = (int)(ip - base_ip - 2);
          prev_hash = HashBytesAtOffset(input_bytes, 1, shift);
          table[prev_hash] = (int)(ip - base_ip - 1);

          candidate = base_ip + table[cur_hash];
          table[cur_hash] = (int)(ip - base_ip);
        }
      }

      next_hash = Hash(++ip, shift);
    }
  }

emit_remainder:
  assert(next_emit <= ip_end);
  /* Emit the remaining bytes as literals. */
  if (next_emit < ip_end) {
    const uint32_t insert = (uint32_t)(ip_end - next_emit);
    EmitInsertLen(insert, commands);
    memcpy(*literals, next_emit, insert);
    *literals += insert;
  }
}

static void StoreCommands(MemoryManager* m,
                          const uint8_t* literals, const size_t num_literals,
                          const uint32_t* commands, const size_t num_commands,
                          size_t* storage_ix, uint8_t* storage) {
  static const uint32_t kNumExtraBits[128] = {
    0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 10, 12, 14, 24,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 10, 24,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8,
    9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24,
  };
  static const uint32_t kInsertOffset[24] = {
    0, 1, 2, 3, 4, 5, 6, 8, 10, 14, 18, 26, 34, 50, 66, 98, 130, 194, 322, 578,
    1090, 2114, 6210, 22594,
  };

  uint8_t lit_depths[256];
  uint16_t lit_bits[256];
  uint32_t lit_histo[256] = { 0 };
  uint8_t cmd_depths[128] = { 0 };
  uint16_t cmd_bits[128] = { 0 };
  uint32_t cmd_histo[128] = { 0 };
  size_t i;
  for (i = 0; i < num_literals; ++i) {
    ++lit_histo[literals[i]];
  }
  BrotliBuildAndStoreHuffmanTreeFast(m, lit_histo, num_literals,
                                     /* max_bits = */ 8,
                                     lit_depths, lit_bits,
                                     storage_ix, storage);
  if (BROTLI_IS_OOM(m)) return;

  for (i = 0; i < num_commands; ++i) {
    ++cmd_histo[commands[i] & 0xff];
  }
  cmd_histo[1] += 1;
  cmd_histo[2] += 1;
  cmd_histo[64] += 1;
  cmd_histo[84] += 1;
  BuildAndStoreCommandPrefixCode(cmd_histo, cmd_depths, cmd_bits,
                                 storage_ix, storage);

  for (i = 0; i < num_commands; ++i) {
    const uint32_t cmd = commands[i];
    const uint32_t code = cmd & 0xff;
    const uint32_t extra = cmd >> 8;
    BrotliWriteBits(cmd_depths[code], cmd_bits[code], storage_ix, storage);
    BrotliWriteBits(kNumExtraBits[code], extra, storage_ix, storage);
    if (code < 24) {
      const uint32_t insert = kInsertOffset[code] + extra;
      uint32_t j;
      for (j = 0; j < insert; ++j) {
        const uint8_t lit = *literals;
        BrotliWriteBits(lit_depths[lit], lit_bits[lit], storage_ix, storage);
        ++literals;
      }
    }
  }
}

/* Acceptable loss for uncompressible speedup is 2% */
#define MIN_RATIO 0.98
#define SAMPLE_RATE 43

static BROTLI_BOOL ShouldCompress(
    const uint8_t* input, size_t input_size, size_t num_literals) {
  double corpus_size = (double)input_size;
  if (num_literals < MIN_RATIO * corpus_size) {
    return BROTLI_TRUE;
  } else {
    uint32_t literal_histo[256] = { 0 };
    const double max_total_bit_cost = corpus_size * 8 * MIN_RATIO / SAMPLE_RATE;
    size_t i;
    for (i = 0; i < input_size; i += SAMPLE_RATE) {
      ++literal_histo[input[i]];
    }
    return TO_BROTLI_BOOL(BitsEntropy(literal_histo, 256) < max_total_bit_cost);
  }
}

void BrotliCompressFragmentTwoPass(MemoryManager* m,
                                   const uint8_t* input, size_t input_size,
                                   BROTLI_BOOL is_last,
                                   uint32_t* command_buf, uint8_t* literal_buf,
                                   int* table, size_t table_size,
                                   size_t* storage_ix, uint8_t* storage) {
  /* Save the start of the first block for position and distance computations.
  */
  const uint8_t* base_ip = input;

  while (input_size > 0) {
    size_t block_size =
        BROTLI_MIN(size_t, input_size, kCompressFragmentTwoPassBlockSize);
    uint32_t* commands = command_buf;
    uint8_t* literals = literal_buf;
    size_t num_literals;
    CreateCommands(input, block_size, input_size, base_ip, table, table_size,
                   &literals, &commands);
    num_literals = (size_t)(literals - literal_buf);
    if (ShouldCompress(input, block_size, num_literals)) {
      const size_t num_commands = (size_t)(commands - command_buf);
      BrotliStoreMetaBlockHeader(block_size, 0, storage_ix, storage);
      /* No block splits, no contexts. */
      BrotliWriteBits(13, 0, storage_ix, storage);
      StoreCommands(m, literal_buf, num_literals, command_buf, num_commands,
                    storage_ix, storage);
      if (BROTLI_IS_OOM(m)) return;
    } else {
      /* Since we did not find many backward references and the entropy of
         the data is close to 8 bits, we can simply emit an uncompressed block.
         This makes compression speed of uncompressible data about 3x faster. */
      BrotliStoreMetaBlockHeader(block_size, 1, storage_ix, storage);
      *storage_ix = (*storage_ix + 7u) & ~7u;
      memcpy(&storage[*storage_ix >> 3], input, block_size);
      *storage_ix += block_size << 3;
      storage[*storage_ix >> 3] = 0;
    }
    input += block_size;
    input_size -= block_size;
  }

  if (is_last) {
    BrotliWriteBits(1, 1, storage_ix, storage);  /* islast */
    BrotliWriteBits(1, 1, storage_ix, storage);  /* isempty */
    *storage_ix = (*storage_ix + 7u) & ~7u;
  }
}

#if defined(__cplusplus) || defined(c_plusplus)
}  /* extern "C" */
#endif
