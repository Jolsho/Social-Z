#pragma once 
#include "sz/api/vec.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t BufferSize; 
#define BUFF_XXS    ((BufferSize)512)
#define BUFF_XS     ((BufferSize)1024)
#define BUFF_S      ((BufferSize)2048)
#define BUFF_M      ((BufferSize)4096)
#define BUFF_L      ((BufferSize)8192)
#define BUFF_XL     ((BufferSize)16384)
#define BUFF_XXL    ((BufferSize)32768)
#define BUFF_SU     ((BufferSize)65386)

const size_t BUFFER_SIZE_CNT = 8;

typedef struct BufferCaps {
    size_t xxs;
    size_t xs;
    size_t s;
    size_t m;
    size_t l;
    size_t xl;
    size_t xxl;
    size_t su;
} BufferCaps;

typedef struct BufferStore BufferStore;
BufferStore* new_buffer_store(BufferCaps*);
void put_buff(BufferStore* bs, Vec* v);
Vec* grab_buff(BufferStore* bs, uint16_t size);

#ifdef __cplusplus
}
#endif
