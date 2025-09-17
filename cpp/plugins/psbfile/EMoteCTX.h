//
// Created by LiDon on 2025/9/14.
//
#pragma once

#include <cstdint>

struct EMoteCTX {
    std::uint32_t key[4];
    std::uint32_t v;
    std::uint32_t count;
};

inline void init_emote_ctx(EMoteCTX *ctx, const std::uint32_t key[4]) {
    ctx->key[0] = key[0];
    ctx->key[1] = key[1];
    ctx->key[2] = key[2];
    ctx->key[3] = key[3];
    ctx->v = 0;
    ctx->count = 0;
}


inline void emote_decrypt(EMoteCTX *ctx, std::uint8_t *data,
                          std::uint32_t size) {
    for(std::uint32_t i = 0; i < size; i++) {
        if(!ctx->v) {
            std::uint32_t b = ctx->key[3];
            std::uint32_t a = ctx->key[0] ^ (ctx->key[0] << 0xB);
            ctx->key[0] = ctx->key[1];
            ctx->key[1] = ctx->key[2];
            std::uint32_t c = a ^ b ^ ((a ^ (b >> 0xB)) >> 8);
            ctx->key[2] = b;
            ctx->key[3] = c;
            ctx->v = c;
            ctx->count = 4;
        }

        data[i] ^= static_cast<std::uint8_t>(ctx->v);
        ctx->v >>= 8;
        ctx->count--;
    }
}
