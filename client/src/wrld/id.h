/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <stdint.h>

typedef uint64_t    EntityID;

typedef uint32_t    GlobalID; // is 24 bits when inside EntityID
typedef uint16_t    LocalID;
typedef uint8_t     HandlerID;
typedef uint16_t    Generation;

#define GLOBAL_SHIFT 40
#define LOCAL_SHIFT  24
#define HANDLER_SHIFT 16

#define GLOBAL_MASK  0xFFFFFFULL
#define LOCAL_MASK   0xFFFFULL
#define HANDLER_MASK 0xFFULL
#define GENERATION_MASK 0xFFFFULL

#define ENTITY_ID_INVALID UINT64_MAX

static inline EntityID entity_id(
    GlobalID global,
    LocalID local,
    HandlerID handler,
    Generation generation)
{
    return ((uint64_t)(global & GLOBAL_MASK) << GLOBAL_SHIFT) |
           ((uint64_t)local << LOCAL_SHIFT) |
           ((uint64_t)handler << HANDLER_SHIFT) |
           generation;
}

static inline GlobalID entity_global(EntityID id) { return (id >> GLOBAL_SHIFT)   & GLOBAL_MASK; }
static inline LocalID entity_local(EntityID id)  { return (id >> LOCAL_SHIFT)    & LOCAL_MASK; }
static inline HandlerID entity_handler(EntityID id) { return (id >> HANDLER_SHIFT)  & HANDLER_MASK; }

static inline Generation entity_generation(EntityID id) { return id & GENERATION_MASK; }
static inline EntityID entity_set_generation(EntityID id, Generation generation) {
    return (id & ~GENERATION_MASK) |
           (uint64_t)generation;
}

#define MAX_HANDLER_ID UINT8_MAX
