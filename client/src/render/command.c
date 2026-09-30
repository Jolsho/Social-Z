/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "command.h"
#include <string.h>

static size_t align8(size_t value) {
    return (value + 7) & ~(size_t)7;
}

void cmd_init(
    CommandBuffer *cmd,
    void *memory,
    size_t capacity
) {
    cmd->data = memory;
    cmd->size = 0;
    cmd->capacity = capacity;
}

void cmd_reset(
    CommandBuffer *cmd
) {
    cmd->size = 0;
}

void *cmd_push(
    CommandBuffer *cmd,
    uint8_t opcode,
    size_t payload_size
) {
    size_t total_size = align8(sizeof(CmdHeader) + payload_size);

    if (total_size > cmd->capacity - cmd->size)
        return NULL;

    CmdHeader *header = (CmdHeader *)(cmd->data + cmd->size);

    header->opcode = opcode;
    header->reserved = 0;
    header->size = (uint32_t)total_size;

    void *payload = header + 1;

    cmd->size += total_size;

    return payload;
}

/* ------------------------------------------------------------------------- */
/* Render pass                                                               */
/* ------------------------------------------------------------------------- */

void cmd_begin_renderpass(
    CommandBuffer *cmd,
    const RenderPassDesc *desc
) {
    uint32_t color_count = desc->color_count;
    uint32_t has_depth = desc->depth != NULL;

    size_t payload_size =
        sizeof(CmdBeginRenderPass) +
        sizeof(ColorAttachmentDesc) * color_count +
        sizeof(DepthAttachmentDesc) * has_depth;

    CmdBeginRenderPass *c =
        cmd_push(cmd, CMD_BEGIN_RENDERPASS, payload_size);

    if (!c) return;

    c->color_count = color_count;
    c->has_depth = has_depth;

    uint8_t *data = (uint8_t *)(c + 1);

    if (color_count > 0) {
        memcpy(
            data,
            desc->colors,
            sizeof(ColorAttachmentDesc) * color_count
        );

        data +=
            sizeof(ColorAttachmentDesc) * color_count;
    }

    if (has_depth) {
        memcpy(
            data,
            desc->depth,
            sizeof(DepthAttachmentDesc)
        );
    }
}

void cmd_end_renderpass(
    CommandBuffer *cmd
) {
    cmd_push(cmd, CMD_END_RENDERPASS, 0);
}

/* ------------------------------------------------------------------------- */
/* State                                                                     */
/* ------------------------------------------------------------------------- */

void cmd_set_pipeline(
    CommandBuffer *cmd,
    Pipeline pipeline
) {
    CmdSetPipeline *c =
        cmd_push(cmd, CMD_SET_PIPELINE, sizeof(*c));

    if (!c) return;

    c->pipeline = pipeline;
}

void cmd_set_descriptor_set(
    CommandBuffer *cmd,
    uint32_t slot,
    DescriptorSet set
) {
    CmdSetDescriptorSet *c =
        cmd_push(
            cmd,
            CMD_SET_DESCRIPTOR_SET,
            sizeof(*c)
        );

    if (!c)
        return;

    c->slot = slot;
    c->set = set;
}

void cmd_set_vertex_buffer(
    CommandBuffer *cmd,
    uint32_t binding,
    Buffer buffer,
    uint64_t offset
) {
    CmdSetVertexBuffer *c =
        cmd_push(
            cmd,
            CMD_SET_VERTEX_BUFFER,
            sizeof(*c)
        );

    if (!c)
        return;

    c->binding = binding;
    c->buffer = buffer;
    c->offset = offset;
}

void cmd_set_index_buffer(
    CommandBuffer *cmd,
    Buffer buffer,
    uint64_t offset
) {
    CmdSetIndexBuffer *c =
        cmd_push(
            cmd,
            CMD_SET_INDEX_BUFFER,
            sizeof(*c)
        );

    if (!c)
        return;

    c->buffer = buffer;
    c->offset = offset;
}

/* ------------------------------------------------------------------------- */
/* Draw                                                                      */
/* ------------------------------------------------------------------------- */

void cmd_draw(
    CommandBuffer *cmd,
    uint32_t vertex_count,
    uint32_t instance_count,
    uint32_t first_vertex,
    uint32_t first_instance
) {
    CmdDraw *c =
        cmd_push(cmd, CMD_DRAW, sizeof(*c));

    if (!c)
        return;

    c->vertex_count = vertex_count;
    c->instance_count = instance_count;
    c->first_vertex = first_vertex;
    c->first_instance = first_instance;
}

void cmd_draw_indexed(
    CommandBuffer *cmd,
    uint32_t index_count,
    uint32_t instance_count,
    uint32_t first_index,
    int32_t vertex_offset,
    uint32_t first_instance
) {
    CmdDrawIndexed *c =
        cmd_push(cmd, CMD_DRAW_INDEXED, sizeof(*c));

    if (!c)
        return;

    c->index_count = index_count;
    c->instance_count = instance_count;
    c->first_index = first_index;
    c->vertex_offset = vertex_offset;
    c->first_instance = first_instance;
}

/* ------------------------------------------------------------------------- */
/* Compute                                                                   */
/* ------------------------------------------------------------------------- */

void cmd_dispatch(
    CommandBuffer *cmd,
    uint32_t x,
    uint32_t y,
    uint32_t z
) {
    CmdDispatch *c =
        cmd_push(cmd, CMD_DISPATCH, sizeof(*c));

    if (!c)
        return;

    c->x = x;
    c->y = y;
    c->z = z;
}
