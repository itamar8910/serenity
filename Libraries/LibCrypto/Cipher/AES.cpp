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

#include <AK/StringBuilder.h>
#include <LibCrypto/Cipher/AES.h>

namespace Crypto {

template <typename T>
constexpr u32 get_key(T pt)
{
    return ((u32)(pt)[0] << 24) ^ ((u32)(pt)[1] << 16) ^ ((u32)(pt)[2] << 8) ^ ((u32)(pt)[3]);
}

constexpr void swap_keys(u32* keys, size_t i, size_t j)
{
    u32 temp = keys[i];
    keys[i] = keys[j];
    keys[j] = temp;
}

String AESCipherBlock::to_string() const
{
    StringBuilder builder;
    for (size_t i = 0; i < BLOCK_SIZE / 8; ++i)
        builder.appendf("%02x", m_data[i]);
    return builder.build();
}

String AESCipherKey::to_string() const
{
    StringBuilder builder;
    for (size_t i = 0; i < rounds() * 4; ++i)
        builder.appendf("%x", m_rd_keys[i]);
    return builder.build();
}

void AESCipherKey::expand_encrypt_key(const StringView& user_key, size_t bits)
{
    u32* round_key;
    u32 temp;
    size_t i { 0 };

    ASSERT(!user_key.is_null());
    ASSERT(is_valid_key_size(bits));

    round_key = round_keys();

    if (bits == 128) {
        m_rounds = 10;
    } else if (bits == 192) {
        m_rounds = 12;
    } else {
        m_rounds = 14;
    }

    round_key[0] = get_key(user_key.substring_view(0, 4).characters_without_null_termination());
    round_key[1] = get_key(user_key.substring_view(4, 4).characters_without_null_termination());
    round_key[2] = get_key(user_key.substring_view(8, 4).characters_without_null_termination());
    round_key[3] = get_key(user_key.substring_view(12, 4).characters_without_null_termination());
    if (bits == 128) {
        for (;;) {
            temp = round_key[3];
            // clang-format off
            round_key[4] = round_key[0] ^
                    (Tables::Encode2[(temp >> 16) & 0xff] & 0xff000000) ^
                    (Tables::Encode3[(temp >>  8) & 0xff] & 0x00ff0000) ^
                    (Tables::Encode0[(temp      ) & 0xff] & 0x0000ff00) ^
                    (Tables::Encode1[(temp >> 24)       ] & 0x000000ff) ^ Tables::RCON[i];
            // clang-format on
            round_key[5] = round_key[1] ^ round_key[4];
            round_key[6] = round_key[2] ^ round_key[5];
            round_key[7] = round_key[3] ^ round_key[6];
            ++i;
            if (i == 10)
                break;
            round_key += 4;
        }
        return;
    }

    round_key[4] = get_key(user_key.substring_view(16, 4).characters_without_null_termination());
    round_key[5] = get_key(user_key.substring_view(20, 4).characters_without_null_termination());
    if (bits == 192) {
        for (;;) {
            temp = round_key[5];
            // clang-format off
            round_key[6] = round_key[0] ^
                    (Tables::Encode2[(temp >> 16) & 0xff] & 0xff000000) ^
                    (Tables::Encode3[(temp >>  8) & 0xff] & 0x00ff0000) ^
                    (Tables::Encode0[(temp      ) & 0xff] & 0x0000ff00) ^
                    (Tables::Encode1[(temp >> 24)       ] & 0x000000ff) ^ Tables::RCON[i];
            // clang-format on
            round_key[7] = round_key[1] ^ round_key[6];
            round_key[8] = round_key[2] ^ round_key[7];
            round_key[9] = round_key[3] ^ round_key[8];

            ++i;
            if (i == 8)
                break;

            round_key[10] = round_key[4] ^ round_key[9];
            round_key[11] = round_key[5] ^ round_key[10];

            round_key += 6;
        }
        return;
    }

    round_key[6] = get_key(user_key.substring_view(24, 4).characters_without_null_termination());
    round_key[7] = get_key(user_key.substring_view(28, 4).characters_without_null_termination());
    if (true) { // bits == 256
        for (;;) {
            temp = round_key[7];
            // clang-format off
            round_key[8] = round_key[0] ^
                    (Tables::Encode2[(temp >> 16) & 0xff] & 0xff000000) ^
                    (Tables::Encode3[(temp >>  8) & 0xff] & 0x00ff0000) ^
                    (Tables::Encode0[(temp      ) & 0xff] & 0x0000ff00) ^
                    (Tables::Encode1[(temp >> 24)       ] & 0x000000ff) ^ Tables::RCON[i];
            // clang-format on
            round_key[9] = round_key[1] ^ round_key[8];
            round_key[10] = round_key[2] ^ round_key[9];
            round_key[11] = round_key[3] ^ round_key[10];

            ++i;
            if (i == 7)
                break;

            temp = round_key[11];
            // clang-format off
            round_key[12] = round_key[4] ^
                    (Tables::Encode2[(temp >> 16) & 0xff] & 0xff000000) ^
                    (Tables::Encode3[(temp >>  8) & 0xff] & 0x00ff0000) ^
                    (Tables::Encode0[(temp      ) & 0xff] & 0x0000ff00) ^
                    (Tables::Encode1[(temp >> 24)       ] & 0x000000ff) ;
            // clang-format on
            round_key[13] = round_key[5] ^ round_key[12];
            round_key[14] = round_key[6] ^ round_key[13];
            round_key[15] = round_key[7] ^ round_key[14];

            round_key += 8;
        }
        return;
    }
}

void AESCipherKey::expand_decrypt_key(const StringView& user_key, size_t bits)
{
    u32* round_key;

    expand_encrypt_key(user_key, bits);

    round_key = round_keys();

    // reorder round keys
    for (size_t i = 0, j = 4 * rounds(); i < j; i += 4, j -= 4) {
        swap_keys(round_key, i, j);
        swap_keys(round_key, i + 1, j + 1);
        swap_keys(round_key, i + 2, j + 2);
        swap_keys(round_key, i + 3, j + 3);
    }

    // apply inverse mix-column to middle rounds
    for (size_t i = 1; i < rounds(); ++i) {
        round_key += 4;
        // clang-format off
        round_key[0] =
                Tables::Decode0[Tables::Encode1[(round_key[0] >> 24)       ] & 0xff] ^
                Tables::Decode1[Tables::Encode1[(round_key[0] >> 16) & 0xff] & 0xff] ^
                Tables::Decode2[Tables::Encode1[(round_key[0] >>  8) & 0xff] & 0xff] ^
                Tables::Decode3[Tables::Encode1[(round_key[0]      ) & 0xff] & 0xff] ;
        round_key[1] =
                Tables::Decode0[Tables::Encode1[(round_key[1] >> 24)       ] & 0xff] ^
                Tables::Decode1[Tables::Encode1[(round_key[1] >> 16) & 0xff] & 0xff] ^
                Tables::Decode2[Tables::Encode1[(round_key[1] >>  8) & 0xff] & 0xff] ^
                Tables::Decode3[Tables::Encode1[(round_key[1]      ) & 0xff] & 0xff] ;
        round_key[2] =
                Tables::Decode0[Tables::Encode1[(round_key[2] >> 24)       ] & 0xff] ^
                Tables::Decode1[Tables::Encode1[(round_key[2] >> 16) & 0xff] & 0xff] ^
                Tables::Decode2[Tables::Encode1[(round_key[2] >>  8) & 0xff] & 0xff] ^
                Tables::Decode3[Tables::Encode1[(round_key[2]      ) & 0xff] & 0xff] ;
        round_key[3] =
                Tables::Decode0[Tables::Encode1[(round_key[3] >> 24)       ] & 0xff] ^
                Tables::Decode1[Tables::Encode1[(round_key[3] >> 16) & 0xff] & 0xff] ^
                Tables::Decode2[Tables::Encode1[(round_key[3] >>  8) & 0xff] & 0xff] ^
                Tables::Decode3[Tables::Encode1[(round_key[3]      ) & 0xff] & 0xff] ;
        // clang-format on
    }
}

void AESCipher::encrypt_block(const AESCipherBlock& in, AESCipherBlock& out)
{
    u32 s0, s1, s2, s3, t0, t1, t2, t3;
    size_t r { 0 };

    const auto& dec_key = key();
    const auto* round_keys = dec_key.round_keys();

    s0 = get_key(in.data().offset_pointer(0)) ^ round_keys[0];
    s1 = get_key(in.data().offset_pointer(4)) ^ round_keys[1];
    s2 = get_key(in.data().offset_pointer(8)) ^ round_keys[2];
    s3 = get_key(in.data().offset_pointer(12)) ^ round_keys[3];

    r = dec_key.rounds() >> 1;

    // apply the first |r - 1| rounds
    auto i { 0 };
    for (;;) {
        ++i;
        // clang-format off
        t0 = Tables::Encode0[(s0 >> 24)       ] ^
             Tables::Encode1[(s1 >> 16) & 0xff] ^
             Tables::Encode2[(s2 >>  8) & 0xff] ^
             Tables::Encode3[(s3      ) & 0xff] ^ round_keys[4];
        t1 = Tables::Encode0[(s1 >> 24)       ] ^
             Tables::Encode1[(s2 >> 16) & 0xff] ^
             Tables::Encode2[(s3 >>  8) & 0xff] ^
             Tables::Encode3[(s0      ) & 0xff] ^ round_keys[5];
        t2 = Tables::Encode0[(s2 >> 24)       ] ^
             Tables::Encode1[(s3 >> 16) & 0xff] ^
             Tables::Encode2[(s0 >>  8) & 0xff] ^
             Tables::Encode3[(s1      ) & 0xff] ^ round_keys[6];
        t3 = Tables::Encode0[(s3 >> 24)       ] ^
             Tables::Encode1[(s0 >> 16) & 0xff] ^
             Tables::Encode2[(s1 >>  8) & 0xff] ^
             Tables::Encode3[(s2      ) & 0xff] ^ round_keys[7];
        // clang-format on

        round_keys += 8;
        --r;
        ++i;
        if (r == 0)
            break;

        // clang-format off
        s0 = Tables::Encode0[(t0 >> 24)       ] ^
             Tables::Encode1[(t1 >> 16) & 0xff] ^
             Tables::Encode2[(t2 >>  8) & 0xff] ^
             Tables::Encode3[(t3      ) & 0xff] ^ round_keys[0];
        s1 = Tables::Encode0[(t1 >> 24)       ] ^
             Tables::Encode1[(t2 >> 16) & 0xff] ^
             Tables::Encode2[(t3 >>  8) & 0xff] ^
             Tables::Encode3[(t0      ) & 0xff] ^ round_keys[1];
        s2 = Tables::Encode0[(t2 >> 24)       ] ^
             Tables::Encode1[(t3 >> 16) & 0xff] ^
             Tables::Encode2[(t0 >>  8) & 0xff] ^
             Tables::Encode3[(t1      ) & 0xff] ^ round_keys[2];
        s3 = Tables::Encode0[(t3 >> 24)       ] ^
             Tables::Encode1[(t0 >> 16) & 0xff] ^
             Tables::Encode2[(t1 >>  8) & 0xff] ^
             Tables::Encode3[(t2      ) & 0xff] ^ round_keys[3];
        // clang-format on
    }

    // apply the last round and put the encrypted data into out
    // clang-format off
    s0 = (Tables::Encode2[(t0 >> 24)       ] & 0xff000000) ^
         (Tables::Encode3[(t1 >> 16) & 0xff] & 0x00ff0000) ^
         (Tables::Encode0[(t2 >>  8) & 0xff] & 0x0000ff00) ^
         (Tables::Encode1[(t3      ) & 0xff] & 0x000000ff) ^ round_keys[0];
    out.put(0, s0);

    s1 = (Tables::Encode2[(t1 >> 24)       ] & 0xff000000) ^
         (Tables::Encode3[(t2 >> 16) & 0xff] & 0x00ff0000) ^
         (Tables::Encode0[(t3 >>  8) & 0xff] & 0x0000ff00) ^
         (Tables::Encode1[(t0      ) & 0xff] & 0x000000ff) ^ round_keys[1];
    out.put(4, s1);

    s2 = (Tables::Encode2[(t2 >> 24)       ] & 0xff000000) ^
         (Tables::Encode3[(t3 >> 16) & 0xff] & 0x00ff0000) ^
         (Tables::Encode0[(t0 >>  8) & 0xff] & 0x0000ff00) ^
         (Tables::Encode1[(t1      ) & 0xff] & 0x000000ff) ^ round_keys[2];
    out.put(8, s2);

    s3 = (Tables::Encode2[(t3 >> 24)       ] & 0xff000000) ^
         (Tables::Encode3[(t0 >> 16) & 0xff] & 0x00ff0000) ^
         (Tables::Encode0[(t1 >>  8) & 0xff] & 0x0000ff00) ^
         (Tables::Encode1[(t2      ) & 0xff] & 0x000000ff) ^ round_keys[3];
    out.put(12, s3);
    // clang-format on
}

void AESCipher::decrypt_block(const AESCipherBlock& in, AESCipherBlock& out)
{

    u32 s0, s1, s2, s3, t0, t1, t2, t3;
    size_t r { 0 };

    const auto& dec_key = key();
    const auto* round_keys = dec_key.round_keys();

    s0 = get_key(in.data().offset_pointer(0)) ^ round_keys[0];
    s1 = get_key(in.data().offset_pointer(4)) ^ round_keys[1];
    s2 = get_key(in.data().offset_pointer(8)) ^ round_keys[2];
    s3 = get_key(in.data().offset_pointer(12)) ^ round_keys[3];

    r = dec_key.rounds() >> 1;

    // apply the first |r - 1| rounds
    for (;;) {
        // clang-format off
        t0 = Tables::Decode0[(s0 >> 24)       ] ^
             Tables::Decode1[(s3 >> 16) & 0xff] ^
             Tables::Decode2[(s2 >>  8) & 0xff] ^
             Tables::Decode3[(s1      ) & 0xff] ^ round_keys[4];
        t1 = Tables::Decode0[(s1 >> 24)       ] ^
             Tables::Decode1[(s0 >> 16) & 0xff] ^
             Tables::Decode2[(s3 >>  8) & 0xff] ^
             Tables::Decode3[(s2      ) & 0xff] ^ round_keys[5];
        t2 = Tables::Decode0[(s2 >> 24)       ] ^
             Tables::Decode1[(s1 >> 16) & 0xff] ^
             Tables::Decode2[(s0 >>  8) & 0xff] ^
             Tables::Decode3[(s3      ) & 0xff] ^ round_keys[6];
        t3 = Tables::Decode0[(s3 >> 24)       ] ^
             Tables::Decode1[(s2 >> 16) & 0xff] ^
             Tables::Decode2[(s1 >>  8) & 0xff] ^
             Tables::Decode3[(s0      ) & 0xff] ^ round_keys[7];
        // clang-format on

        round_keys += 8;
        --r;
        if (r == 0)
            break;

        // clang-format off
        s0 = Tables::Decode0[(t0 >> 24)       ] ^
             Tables::Decode1[(t3 >> 16) & 0xff] ^
             Tables::Decode2[(t2 >>  8) & 0xff] ^
             Tables::Decode3[(t1      ) & 0xff] ^ round_keys[0];
        s1 = Tables::Decode0[(t1 >> 24)       ] ^
             Tables::Decode1[(t0 >> 16) & 0xff] ^
             Tables::Decode2[(t3 >>  8) & 0xff] ^
             Tables::Decode3[(t2      ) & 0xff] ^ round_keys[1];
        s2 = Tables::Decode0[(t2 >> 24)       ] ^
             Tables::Decode1[(t1 >> 16) & 0xff] ^
             Tables::Decode2[(t0 >>  8) & 0xff] ^
             Tables::Decode3[(t3      ) & 0xff] ^ round_keys[2];
        s3 = Tables::Decode0[(t3 >> 24)       ] ^
             Tables::Decode1[(t2 >> 16) & 0xff] ^
             Tables::Decode2[(t1 >>  8) & 0xff] ^
             Tables::Decode3[(t0      ) & 0xff] ^ round_keys[3];
        // clang-format on
    }

    // apply the last round and put the decrypted data into out
    // clang-format off
    s0 = ((u32)Tables::Decode4[(t0 >> 24)       ] << 24) ^
         ((u32)Tables::Decode4[(t3 >> 16) & 0xff] << 16) ^
         ((u32)Tables::Decode4[(t2 >>  8) & 0xff] <<  8) ^
         ((u32)Tables::Decode4[(t1      ) & 0xff]      ) ^ round_keys[0];
    out.put(0, s0);

    s1 = ((u32)Tables::Decode4[(t1 >> 24)       ] << 24) ^
         ((u32)Tables::Decode4[(t0 >> 16) & 0xff] << 16) ^
         ((u32)Tables::Decode4[(t3 >>  8) & 0xff] <<  8) ^
         ((u32)Tables::Decode4[(t2      ) & 0xff]      ) ^ round_keys[1];
    out.put(4, s1);

    s2 = ((u32)Tables::Decode4[(t2 >> 24)       ] << 24) ^
         ((u32)Tables::Decode4[(t1 >> 16) & 0xff] << 16) ^
         ((u32)Tables::Decode4[(t0 >>  8) & 0xff] <<  8) ^
         ((u32)Tables::Decode4[(t3      ) & 0xff]      ) ^ round_keys[2];
    out.put(8, s2);

    s3 = ((u32)Tables::Decode4[(t3 >> 24)       ] << 24) ^
         ((u32)Tables::Decode4[(t2 >> 16) & 0xff] << 16) ^
         ((u32)Tables::Decode4[(t1 >>  8) & 0xff] <<  8) ^
         ((u32)Tables::Decode4[(t0      ) & 0xff]      ) ^ round_keys[3];
    out.put(12, s3);
    // clang-format on
}

void AESCipherBlock::overwrite(const ByteBuffer& buffer)
{
    overwrite(buffer.data(), buffer.size());
}

void AESCipherBlock::overwrite(const u8* data, size_t length)
{
    ASSERT(length <= m_data.size());
    m_data.overwrite(0, data, length);
    if (length < m_data.size()) {
        switch (padding_mode()) {
        default:
            // FIXME: We should handle the rest of the common padding modes
        case PaddingMode::Null:
            // fill with zeros
            __builtin_memset(m_data.data() + length, 0, m_data.size() - length);
            break;
        case PaddingMode::CMS:
            // fill with the length of the padding bytes
            __builtin_memset(m_data.data() + length, m_data.size() - length, m_data.size() - length);
            break;
        }
    }
}
}
