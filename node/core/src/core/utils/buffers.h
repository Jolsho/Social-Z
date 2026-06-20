#pragma once 
#include "bindings.h"
#include <cstdlib>
#include <cstring>
#include <vector>
#include <array>

enum BufferSize : uint16_t{
    XXS = 512,
    XS  = 1024,

    S   = 2048,
    M   = 4096,
    L   = 8192,

    XL  = 16384,
    XXL = 32768,

    SU  = 65386

    // 8 sizes
};
static constexpr size_t BUFFER_SIZE_CNT = 8;

struct BufferCaps {
    size_t xxs  = 10;
    size_t xs   = 10;
    size_t s    = 10;
    size_t m    = 10;
    size_t l    = 10;
    size_t xl   = 10;
    size_t xxl  = 10;
    size_t su   = 10;
};

class BufferStore {
    std::array<BufferSize, BUFFER_SIZE_CNT> sizes_;
    std::array<std::vector<Vec*>, BUFFER_SIZE_CNT> buffers_;

public:
    BufferStore(BufferCaps caps) {
        sizes_ = {
            BufferSize::XXS, BufferSize::XS, BufferSize::S,
            BufferSize::M,
            BufferSize::L, BufferSize::XL, BufferSize::XXL,
            BufferSize::SU
        };

        buffers_[0].reserve(caps.xxs);
        buffers_[1].reserve(caps.xs);
        buffers_[2].reserve(caps.s);
        buffers_[3].reserve(caps.m);
        buffers_[4].reserve(caps.l);
        buffers_[5].reserve(caps.xl);
        buffers_[6].reserve(caps.xxl);
        buffers_[7].reserve(caps.su);
    }

    Vec* grab(uint16_t size) {
        if (size == 0) return NULL;

        for (auto i { 0 }; i < sizes_.size(); i++) {
            if (size <= sizes_[i]) {
                auto &buffs = buffers_[i];
                if (buffs.size() < 1) {
                    Vec* v = new Vec{};
                    v->b = (unsigned char*)malloc(size);
                    v->cap = size;
                    v->len = 0;
                    v->c = v->b;
                    return v;
                }
                Vec* v = buffs.back();
                buffs.pop_back();
                return v;
            }
        }
        return nullptr;
    }

    void put(Vec* v) {
        if (!v) return;

        for (auto i { 0 }; i < sizes_.size(); i++) {
            if (v->cap == sizes_[i]) {
                auto &buffs = buffers_[i];
                if (buffs.size() < buffs.capacity() - 1) {
                    v->len = 0; 
                    v->c = v->b;
                    memset(v->b, 0, v->cap);
                    buffs.push_back(v);
                    return;
                }
            }
        }
        free(v->b);
        delete v;
    }
};

