// Cross-platform Implementation of SHA256

#include <stdint.h>
#include <cstddef>
#include <vector>
#include <ifc/file.hxx>

#include <stdlib.h>
#include <memory.h>

namespace {
    // Defined values of K for SHA-256
    constexpr uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

    // Initial hash values for SHA-256
    constexpr std::array<uint32_t, 8> initial_hash_values{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                                          0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    // Various functions performing operations as defined by SHA-256.
    constexpr uint32_t ch(uint32_t x, uint32_t y, uint32_t z)
    {
        return (x & y) ^ ((~x) & z);
    }

    constexpr uint32_t maj(uint32_t x, uint32_t y, uint32_t z)
    {
        return (x & y) ^ (x & z) ^ (y & z);
    }

    uint32_t constexpr SIGMA0(uint32_t w)
    {
        return std::rotr(w, 2) xor std::rotr(w, 13) xor std::rotr(w, 22);
    }

    uint32_t constexpr SIGMA1(uint32_t w)
    {
        return std::rotr(w, 6) xor std::rotr(w, 11) xor std::rotr(w, 25);
    }

    uint32_t constexpr sigma0(uint32_t w)
    {
        return std::rotr(w, 7) xor std::rotr(w, 18) xor (w >> 3);
    }

    uint32_t constexpr sigma1(uint32_t w)
    {
        return std::rotr(w, 17) xor std::rotr(w, 19) xor (w >> 10);
    }

    constexpr void process_chunk(std::array<uint32_t, 8>& hash, const std::byte* p)
    {
        // w is the message schedule array.
        uint32_t w[64] = {0};
        for (int i = 0, j = 0; i < 16; ++i, j += 4)
        {
            // Can't do a memcpy and then change_endianness as memcpy is not constexpr.
            uint32_t b0, b1, b2, b3;
            if constexpr (std::endian::native == std::endian::little)
            {
                b0 = static_cast<uint32_t>(p[j]) << 24;
                b1 = static_cast<uint32_t>(p[j + 1]) << 16;
                b2 = static_cast<uint32_t>(p[j + 2]) << 8;
                b3 = static_cast<uint32_t>(p[j + 3]);
            }
            else
            {
                b0 = static_cast<uint32_t>(p[j]);
                b1 = static_cast<uint32_t>(p[j + 1]) << 8;
                b2 = static_cast<uint32_t>(p[j + 2]) << 16;
                b3 = static_cast<uint32_t>(p[j + 3]) << 24;
            }
            w[i] = b0 | b1 | b2 | b3;
        }

        for (int i = 16; i < 64; ++i)
            w[i] = sigma1(w[i - 2]) + w[i - 7] + sigma0(w[i - 15]) + w[i - 16];

        auto a = hash[0];
        auto b = hash[1];
        auto c = hash[2];
        auto d = hash[3];
        auto e = hash[4];
        auto f = hash[5];
        auto g = hash[6];
        auto h = hash[7];

        for (size_t i = 0; i < 64; ++i)
        {
            uint32_t T1 = h + SIGMA1(e) + ch(e, f, g) + K[i] + w[i];
            uint32_t T2 = SIGMA0(a) + maj(a, b, c);

            h = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }

        hash[0] += a;
        hash[1] += b;
        hash[2] += c;
        hash[3] += d;
        hash[4] += e;
        hash[5] += f;
        hash[6] += g;
        hash[7] += h;
    }

    // convert between little-endian/big-endian
    constexpr void change_endianness(uint32_t& v)
    {
        // TODO use std::byteswap when C++23 targeted.
        v = ((v & 0xff000000) >> 24) | ((v & 0xff0000) >> 8) | ((v & 0xff00) << 8) | ((v & 0xff) << 24);
    }

    constexpr ifc::SHA256Hash sha256(gsl::span<const std::byte> span)
    {
        ifc::SHA256Hash hash = {initial_hash_values};

        auto length = span.size();
        auto base   = span.data();
        // Process whole chunks first
        auto iterations = length / 64;
        for (size_t iter = 0; iter < iterations; ++iter)
            process_chunk(hash.value, base + iter * 64);

        /*
            append a single '1' bit
            append K '0' bits, where K is the minimum number >= 0 such that (L + 1 + K + 64) is a multiple of 512
            append L as a 64-bit big-endian integer, making the total post-processed length a multiple of 512 bits
            This requires at least 9 bytes. If the remainder is greater than 55, a second extra chunk is needed.
        */

        // remainder is number of bytes in message past 64byte boundary (could be zero)
        auto remainder = length % 64;

        // Room for one or two chunks (as needed)
        std::byte v[128] = {std::byte{0}}; // Fill with zeros
        std::copy(base + iterations * 64, base + iterations * 64 + remainder, v);
        v[remainder] = std::byte{0x80}; // Put 10000000 after data
        // The data, the "1" bit, and zero bits are now in place.

        // Put total number of bits in message at end.
        uint64_t total_bits = length * 8;
        if (remainder > 55) // Need the second block
        {
            // Put number of bits at end of second block.
            std::byte* pbits = v + 128 - sizeof(uint64_t);
            for (int i = 0; i < 8; ++i)
            {
                uint8_t byte = static_cast<uint8_t>(total_bits >> (7 - i) * 8);
                *pbits++     = std::byte{byte};
            }
            process_chunk(hash.value, v);
            process_chunk(hash.value, v + 64);
        }
        else
        {
            // Put number of bits at end of first block.
            std::byte* pbits = v + 64 - sizeof(uint64_t);
            for (int i = 0; i < 8; ++i)
            {
                uint8_t byte = static_cast<uint8_t>(total_bits >> (7 - i) * 8);
                *pbits++     = std::byte{byte};
            }
            process_chunk(hash.value, v);
        }

        if constexpr (std::endian::native == std::endian::little)
        {
            // Convert hash bytes to proper endianness
            for (auto& v : hash.value)
                change_endianness(v);
        }

        return hash;
    }

#ifndef NDEBUG
    // Statically verify the hash is correct with two simple tests.

    // "", "E3B0C442 98FC1C14 9AFBF4C8 996FB924 27AE41E4 649B934C A495991B 7852B855");
    constexpr const std::byte test1[1] = {};
    constexpr ifc::SHA256Hash hash1    = {0x42C4B0E3, 0x141CFC98, 0xC8F4FB9A, 0x24B96F99,
                                          0xE441AE27, 0x4C939B64, 0x1B9995A4, 0x55B85278};
    static_assert(sha256(gsl::span<const std::byte>(test1, test1)).value == hash1.value);

    // "a" "CA978112 CA1BBDCA FAC231B3 9A23DC4D A786EFF8 147C4E72 B9807785 AFEE48BB");
    constexpr const std::byte test2[1] = {std::byte{'a'}};
    constexpr ifc::SHA256Hash hash2    = {0x128197CA, 0xCABD1BCA, 0xB331C2FA, 0x4DDC239A,
                                          0xF8EF86A7, 0x724E7C14, 0x857780B9, 0xBB48EEAF};
    static_assert(sha256(gsl::span<const std::byte>(test2, 1)).value == hash2.value);
#endif
} // namespace

namespace ifc {
    SHA256Hash hash_bytes(const std::byte* first, const std::byte* last)
    {
        gsl::span<const std::byte> span(first, last);
        return sha256(span);
    }
} // namespace ifc
