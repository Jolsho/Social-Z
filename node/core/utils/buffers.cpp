#include "sz/api/vec.h"
#include "sz/utils/buffers.h"
#include <cstring>
#include <stdlib.h>
#include <vector>

struct BufferStore {
    BufferSize          sizes_[BUFFER_SIZE_CNT];
    std::vector<Vec*>   buffers_[BUFFER_SIZE_CNT];
};

BufferCaps* default_caps() {
}

BufferStore* new_buffer_store(BufferCaps* caps) {
    BufferStore* bs = new BufferStore{};
    if (!caps) {
        caps = default_caps();
    }
    bs->sizes_ [0] = BUFF_XXS;
    bs->sizes_ [1] = BUFF_XS;
    bs->sizes_ [2] = BUFF_S;
    bs->sizes_ [3] = BUFF_M;
    bs->sizes_ [4] = BUFF_L;
    bs->sizes_ [5] = BUFF_XL;
    bs->sizes_ [6] = BUFF_XXL;
    bs->sizes_ [7] = BUFF_SU;

    bs->buffers_[0].reserve(caps->xxs);
    bs->buffers_[1].reserve(caps->xs);
    bs->buffers_[2].reserve(caps->s);
    bs->buffers_[3].reserve(caps->m);
    bs->buffers_[4].reserve(caps->l);
    bs->buffers_[5].reserve(caps->xl);
    bs->buffers_[6].reserve(caps->xxl);
    bs->buffers_[7].reserve(caps->su);

    free(caps);

    return bs;
}

Vec* grab_buff(BufferStore* bs, uint16_t size) {
    if (size == 0) return NULL;

    for (auto i { 0 }; i < BUFFER_SIZE_CNT; i++) {
        if (size <= bs->sizes_[i]) {
            auto &buffs = bs->buffers_[i];
            if (buffs.size() < 1) {
                Vec* v = new Vec{};
                v->b = (uint8_t*)malloc(size);
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

void put_buff(BufferStore* bs, Vec* v) {
    if (!v) return;

    for (auto i { 0 }; i < BUFFER_SIZE_CNT; i++) {
        if (v->cap == bs->sizes_[i]) {
            auto &buffs = bs->buffers_[i];
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
