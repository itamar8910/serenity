/*
 * Copyright (c) 2020, Ali Mohammad Pur <ali.mpfard@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <AK/ByteBuffer.h>
#include <AK/String.h>
#include <AK/StringBuilder.h>
#include <AK/StringView.h>
#include <AK/Types.h>
#include <AK/Vector.h>

constexpr static auto IPAD = 0x36;
constexpr static auto OPAD = 0x5c;

namespace Crypto {
namespace Authentication {
    template <typename HashT>
    class HMAC {
    public:
        using HashType = HashT;
        using TagType = typename HashType::DigestType;
        static constexpr size_t BlockSize = HashType::BlockSize;
        static constexpr size_t DigestSize = HashType::DigestSize;

        template <typename KeyBufferType, typename... Args>
        HMAC(KeyBufferType key, Args... args)
            : m_hasher(args...)
        {
            derive_key(key);
        }

        TagType process(const u8* message, size_t length)
        {
            m_hasher.update(m_key_data, BlockSize);
            m_hasher.update(message, length);
            auto inner_hash = m_hasher.digest();
            m_hasher.update(m_key_data + BlockSize, BlockSize);
            m_hasher.update(inner_hash.data, sizeof(TagType));

            return m_hasher.digest();
        }

        TagType process(const ByteBuffer& buffer) { return process(buffer.data(), buffer.size()); }
        TagType process(const StringView& string) { return process((const u8*)string.characters_without_null_termination(), string.length()); }

        String class_name() const
        {
            StringBuilder builder;
            builder.append("HMAC-");
            builder.append(m_hasher.class_name());
            return builder.build();
        }

    private:
        void derive_key(const u8* key, size_t length)
        {
            u8 v_key[BlockSize];
            __builtin_memset(v_key, 0, BlockSize);
            ByteBuffer key_buffer = ByteBuffer::wrap(v_key, BlockSize);
            // m_key_data is zero'd, so copying the data in
            // the first few bytes leaves the rest zero, which
            // is exactly what we want (zero padding)
            if (length > BlockSize) {
                m_hasher.update(key, length);
                auto digest = m_hasher.digest();
                // FIXME: should we check if the hash function creates more data than its block size?
                key_buffer.overwrite(0, digest.data, sizeof(TagType));
            } else {
                key_buffer.overwrite(0, key, length);
            }

            // fill out the inner and outer padded keys
            auto* i_key = m_key_data;
            auto* o_key = m_key_data + BlockSize;
            for (size_t i = 0; i < BlockSize; ++i) {
                auto key_byte = key_buffer[i];
                i_key[i] = key_byte ^ IPAD;
                o_key[i] = key_byte ^ OPAD;
            }
        }

        void derive_key(const ByteBuffer& key) { derive_key(key.data(), key.size()); }
        void derive_key(const StringView& key) { derive_key((const u8*)key.characters_without_null_termination(), key.length()); }

        HashType m_hasher;
        u8 m_key_data[BlockSize * 2];
    };
}
}
