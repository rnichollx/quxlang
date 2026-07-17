// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_BLAKE2B_HEADER_GUARD
#define QUXLANG_BLAKE2B_HEADER_GUARD

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace quxlang::blake2b
{
    inline constexpr std::uint32_t digest_size = 64;
    inline constexpr std::uint32_t block_size = 128;

    namespace detail
    {
        inline constexpr std::array< std::uint64_t, 8 > initialization_vector = {
            0x6a09e667f3bcc908ULL,
            0xbb67ae8584caa73bULL,
            0x3c6ef372fe94f82bULL,
            0xa54ff53a5f1d36f1ULL,
            0x510e527fade682d1ULL,
            0x9b05688c2b3e6c1fULL,
            0x1f83d9abfb41bd6bULL,
            0x5be0cd19137e2179ULL,
        };

        inline constexpr std::array< std::array< std::uint8_t, 16 >, 12 > permutations = {{
            {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}},
            {{14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3}},
            {{11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4}},
            {{7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8}},
            {{9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13}},
            {{2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9}},
            {{12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11}},
            {{13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10}},
            {{6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5}},
            {{10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0}},
            {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}},
            {{14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3}},
        }};

        inline auto load64(std::byte const* input) noexcept -> std::uint64_t
        {
            std::uint64_t result = 0;
            for (std::uint32_t i = 0; i < 8; ++i)
            {
                result |= static_cast< std::uint64_t >(std::to_integer< std::uint8_t >(input[i])) << (i * 8);
            }
            return result;
        }

        inline void store64(std::byte* output, std::uint64_t value) noexcept
        {
            for (std::uint32_t i = 0; i < 8; ++i)
            {
                output[i] = static_cast< std::byte >((value >> (i * 8)) & 0xffU);
            }
        }

        inline void mix(
            std::array< std::uint64_t, 16 >& state,
            std::uint8_t a,
            std::uint8_t b,
            std::uint8_t c,
            std::uint8_t d,
            std::uint64_t x,
            std::uint64_t y) noexcept
        {
            state[a] = state[a] + state[b] + x;
            state[d] = std::rotr(state[d] ^ state[a], 32);
            state[c] += state[d];
            state[b] = std::rotr(state[b] ^ state[c], 24);
            state[a] = state[a] + state[b] + y;
            state[d] = std::rotr(state[d] ^ state[a], 16);
            state[c] += state[d];
            state[b] = std::rotr(state[b] ^ state[c], 63);
        }
    } // namespace detail

    class hasher
    {
    public:
        hasher() noexcept : m_state(detail::initialization_vector)
        {
            // Parameter block: digest length 64, key length 0, fanout 1, depth 1.
            m_state[0] ^= 0x01010000U ^ digest_size;
        }

        void update(std::span< std::byte const > input) noexcept
        {
            if (input.empty())
            {
                return;
            }

            if (m_buffer_size != 0)
            {
                std::uint64_t const fill = block_size - m_buffer_size;
                if (static_cast< std::uint64_t >(input.size()) > fill)
                {
                    for (std::uint64_t i = 0; i < fill; ++i)
                    {
                        m_buffer[m_buffer_size + i] = input[i];
                    }
                    increment_counter(block_size);
                    compress(m_buffer, false);
                    m_buffer_size = 0;
                    input = input.subspan(fill);
                }
                else
                {
                    for (std::uint64_t i = 0; i < static_cast< std::uint64_t >(input.size()); ++i)
                    {
                        m_buffer[m_buffer_size + i] = input[i];
                    }
                    m_buffer_size += input.size();
                    return;
                }
            }

            while (input.size() > block_size)
            {
                increment_counter(block_size);
                compress(input.first< block_size >(), false);
                input = input.subspan(block_size);
            }

            for (std::uint64_t i = 0; i < static_cast< std::uint64_t >(input.size()); ++i)
            {
                m_buffer[i] = input[i];
            }
            m_buffer_size = input.size();
        }

        auto finalize() noexcept -> std::array< std::byte, digest_size >
        {
            increment_counter(m_buffer_size);
            for (std::uint64_t i = m_buffer_size; i < block_size; ++i)
            {
                m_buffer[i] = std::byte{0};
            }
            compress(m_buffer, true);

            std::array< std::byte, digest_size > digest{};
            for (std::uint32_t i = 0; i < m_state.size(); ++i)
            {
                detail::store64(digest.data() + (i * 8), m_state[i]);
            }
            return digest;
        }

    private:
        std::array< std::uint64_t, 8 > m_state;
        std::array< std::byte, block_size > m_buffer{};
        std::uint64_t m_buffer_size = 0;
        std::uint64_t m_counter_low = 0;
        std::uint64_t m_counter_high = 0;

        void increment_counter(std::uint64_t amount) noexcept
        {
            std::uint64_t const previous = m_counter_low;
            m_counter_low += amount;
            if (m_counter_low < previous)
            {
                ++m_counter_high;
            }
        }

        void compress(std::span< std::byte const, block_size > block, bool final_block) noexcept
        {
            std::array< std::uint64_t, 16 > message{};
            for (std::uint32_t i = 0; i < message.size(); ++i)
            {
                message[i] = detail::load64(block.data() + (i * 8));
            }

            std::array< std::uint64_t, 16 > working{};
            for (std::uint32_t i = 0; i < 8; ++i)
            {
                working[i] = m_state[i];
                working[i + 8] = detail::initialization_vector[i];
            }
            working[12] ^= m_counter_low;
            working[13] ^= m_counter_high;
            if (final_block)
            {
                working[14] = ~working[14];
            }

            for (std::array< std::uint8_t, 16 > const& permutation : detail::permutations)
            {
                detail::mix(working, 0, 4, 8, 12, message[permutation[0]], message[permutation[1]]);
                detail::mix(working, 1, 5, 9, 13, message[permutation[2]], message[permutation[3]]);
                detail::mix(working, 2, 6, 10, 14, message[permutation[4]], message[permutation[5]]);
                detail::mix(working, 3, 7, 11, 15, message[permutation[6]], message[permutation[7]]);
                detail::mix(working, 0, 5, 10, 15, message[permutation[8]], message[permutation[9]]);
                detail::mix(working, 1, 6, 11, 12, message[permutation[10]], message[permutation[11]]);
                detail::mix(working, 2, 7, 8, 13, message[permutation[12]], message[permutation[13]]);
                detail::mix(working, 3, 4, 9, 14, message[permutation[14]], message[permutation[15]]);
            }

            for (std::uint32_t i = 0; i < m_state.size(); ++i)
            {
                m_state[i] ^= working[i] ^ working[i + 8];
            }
        }
    };

    inline auto hash(std::span< std::byte const > input) noexcept -> std::array< std::byte, digest_size >
    {
        hasher state;
        state.update(input);
        return state.finalize();
    }

    inline auto hex(std::span< std::byte const > input) -> std::string
    {
        static constexpr char digits[] = "0123456789abcdef";
        std::array< std::byte, digest_size > const digest = hash(input);
        std::string result;
        result.resize(digest.size() * 2);
        for (std::uint32_t i = 0; i < digest.size(); ++i)
        {
            std::uint8_t const value = std::to_integer< std::uint8_t >(digest[i]);
            result[i * 2] = digits[value >> 4];
            result[(i * 2) + 1] = digits[value & 0x0fU];
        }
        return result;
    }
} // namespace quxlang::blake2b

#endif // QUXLANG_BLAKE2B_HEADER_GUARD
