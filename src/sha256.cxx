// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Cross-platform Implementation of SHA256

#include <stdint.h>
#include <bit>
#include <cstddef>
#include <array>
#include <ifc/file.hxx>

namespace {
    // -- Standard SHA-256 round constants; keeping the specified values visible aids independent auditing.
    constexpr uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

    // -- SHA-256 choice primitive used in each compression round.
    constexpr uint32_t ch(uint32_t x, uint32_t y, uint32_t z)
    {
        return (x & y) ^ ((~x) & z);
    }

    // -- SHA-256 majority primitive used in each compression round.
    constexpr uint32_t maj(uint32_t x, uint32_t y, uint32_t z)
    {
        return (x & y) ^ (x & z) ^ (y & z);
    }

    // -- SHA-256 upper-case sigma 0 transform for the working state.
    uint32_t constexpr SIGMA0(uint32_t w)
    {
        return std::rotr(w, 2) xor std::rotr(w, 13) xor std::rotr(w, 22);
    }

    // -- SHA-256 upper-case sigma 1 transform for the working state.
    uint32_t constexpr SIGMA1(uint32_t w)
    {
        return std::rotr(w, 6) xor std::rotr(w, 11) xor std::rotr(w, 25);
    }

    // -- SHA-256 lower-case sigma 0 transform for message expansion.
    uint32_t constexpr sigma0(uint32_t w)
    {
        return std::rotr(w, 7) xor std::rotr(w, 18) xor (w >> 3);
    }

    // -- SHA-256 lower-case sigma 1 transform for message expansion.
    uint32_t constexpr sigma1(uint32_t w)
    {
        return std::rotr(w, 17) xor std::rotr(w, 19) xor (w >> 10);
    }

    // -- Shared compression boundary used by every incremental and one-shot portable hash operation.
    constexpr void process_chunk(std::array<uint32_t, 8>& hash, const std::byte* p)
    {
        // w is the message schedule array.
        uint32_t w[64] = {0};
        for (int i = 0, j = 0; i < 16; ++i, j += 4)
        {
            const uint32_t b0 = static_cast<uint32_t>(p[j]) << 24;
            const uint32_t b1 = static_cast<uint32_t>(p[j + 1]) << 16;
            const uint32_t b2 = static_cast<uint32_t>(p[j + 2]) << 8;
            const uint32_t b3 = static_cast<uint32_t>(p[j + 3]);
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

    // -- IFC stores digest bytes in SHA-256's big-endian representation on every host architecture.
    constexpr void change_endianness(uint32_t& v)
    {
        // TODO use std::byteswap when C++23 targeted.
        v = ((v & 0xff000000) >> 24) | ((v & 0xff0000) >> 8) | ((v & 0xff00) << 8) | ((v & 0xff) << 24);
    }

} // namespace

namespace ifc {
    Sha256::Sha256()
    {
    }

    Sha256::~Sha256()
    {
    }

    void Sha256::update(gsl::span<const std::byte> bytes)
    {
        message_length += bytes.size();
        const std::byte* p = bytes.data();
        std::size_t remaining = bytes.size();

        // Finish any block left partly filled by a previous update.
        if (block_length != 0)
        {
            const std::size_t room = block.size() - block_length;
            const std::size_t take = remaining < room ? remaining : room;
            for (std::size_t i = 0; i < take; ++i)
                block[block_length + i] = p[i];
            block_length += take;
            p += take;
            remaining -= take;
            if (block_length < block.size())
                return;
            process_chunk(state, block.data());
            block_length = 0;
        }
        // Fold whole blocks straight from the input.
        while (remaining >= block.size())
        {
            process_chunk(state, p);
            p += block.size();
            remaining -= block.size();
        }
        // Stash the remainder for the next update or finish.
        for (std::size_t i = 0; i < remaining; ++i)
            block[i] = p[i];
        block_length = remaining;
    }

    SHA256Hash Sha256::finish()
    {
        const std::uint64_t bit_length = message_length * 8;

        // Append the '1' bit, pad with zeros, and finish with the 64-bit big-endian bit length,
        // spilling into a second block when the length would not fit in the current one.
        block[block_length++] = std::byte{0x80};
        if (block_length > block.size() - sizeof bit_length)
        {
            while (block_length < block.size())
                block[block_length++] = std::byte{0};
            process_chunk(state, block.data());
            block_length = 0;
        }
        while (block_length < block.size() - sizeof bit_length)
            block[block_length++] = std::byte{0};
        for (std::size_t i = 0; i < sizeof bit_length; ++i)
            block[block.size() - sizeof bit_length + i] =
                static_cast<std::byte>(bit_length >> (8 * (sizeof bit_length - 1 - i)));
        process_chunk(state, block.data());

        SHA256Hash hash;
        hash.value = state;
        if constexpr (std::endian::native == std::endian::little)
            for (std::uint32_t& word : hash.value)
                change_endianness(word);
        return hash;
    }

} // namespace ifc
