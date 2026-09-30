/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "wrld/camera.h"
#include "wrld/input/input.h"
#include "sz_client/client.h"
#include "wrld/bvh/bvh.h"

typedef void (*HandleFn) (EntityID, struct Client*);
typedef struct {
    HandleFn key_pres;
    HandleFn click;
    HandleFn hover;
    HandleFn update;
    HandleFn destroy;
} Handler;


typedef enum {
    MEDIA = 0,
    THREAD,
    MARKET,
    TWO_DIM,

    DEFAULT,

    THREE_DIM,
} ViewState;

typedef struct {

    int     scroll_height;
    int     anchor_item_idx;

} ScrollState;


typedef struct {
    BVH         bvh;
    Handler     handlers[MAX_HANDLER_ID];
    
    InputState  input;

    Camera      camera;
    float       screen_x;
    float       screen_y;

    EntityID    focused;

    ViewState   view_state;

    ScrollState scrolls[TWO_DIM];

} World;

int wrld_init(World* wrld);

static inline void wrld_register_handler(World* wrld, uint8_t h_idx, Handler h) {
    wrld->handlers[h_idx] = h;
}

static inline int wrld_new_entity(World* wrld, EntityID eid, AABB bounds) {
    if (!bvh_insert(&wrld->bvh, eid, bounds, 0.0f)) return CLIENT_ERR;
    return CLIENT_OK;
}


/*
 *  What are the data types though?
 *      Everything is an entity.
 *      Only entitize the "visible" buffer of posts.
 *
 *
 *  So you have feed which is rendered all the time.
 *  Each time there is a scroll event(+initial load)
 *      Recalculate which posts are visible and update buffers to reflect that.
 *      Queue a fetch_command if necessary.
 *
 *
 * POST STRUCTURE:
 *      POST == Top level Transform + (x, y, w, h)
 *
 *          Static Relative {
 *              Profile Picture
 *              User Name
 *              Date/Time
 *              Image/Video Display
 *              Description/Text
 *          }
 *              -> these things just have x, y, w, h
 *                  relative to the parent
 *                  This way we can just move top level posts.
 *                  We dont have to move everything else in them.
 *
 *          (
 *              Profile Picture
 *              Image/Video Display
 *          )
 *              -> Backdrop Shape overlayed with Texture
 *
 *  
 *  TEXT::
 *      UTF-8 stream => becomes { Glyph_idx, x, y } stream
 *          -> in order to get x, y we need to keep running totals.
 *          -> start == within the parent += all the characters before it 
 *          -> keep track of characters per line. 
 *              ( once off bound go to last '_', increment Y, and start from there again. )
 *          In the end the parent transform will put it in the right space globally.
 *
 *          -> we do this for the entire text, given its not beyond some large bound.
 *              -> to expand post, we can just increase the amount of glyph instances.
 *
 *      THREE STAGES OF DATA
 *          1) RAW_TEXT
 *          2) Layout Calculated Glyph Instances (CPU)
 *          3) Visible Subset of Glyph Instances (GPU)
 *      Bring in raw data, and when its near rendering calculate layout and store in Glyph Instance buffer.
 *      When its actually visible in the feed container copy it to the GPU along with siblings.
 *      Glyph Instance buffer should be a ring buffer so we can remove and add easily.
 *
 *
 *      RAW_TEXT == stored in Store*
 *          When its post is up the data is brought from store (or retrieved from server).
 *          Then its processed and added to the Glyph Instance buffer.
 *
 *
 *  IMAGES
 *      AS for Images and what not, we can do something similar.
 *      We can have Image Instance buffer. 
 *      Which tracks the texture_(layer || idx) and what not.
 *      Fragment shader can then make an image circular and we can do profile images in the same pass.
 *      Or we can do icons the same way as well...
 *
 *
 *  RENDER ALGORITHM
 *
 *          "Which entities need to be rendered?"
 *              [ Entity IDs ]
 *
 *          "How should each entity be rendered?" 
 *              [ TransformID, MeshID, MaterialID ]
 *              [ uint32_t, uint16_t, uint16_t ]
 *              4 + 2 + 2 = 8 bytes
 *
 *          Group by archetype
 *              MaterialID
 *                  -> MeshID
 *
 *          Execute Draw commands
 *              -> Either build list of visible Transforms and upload.
 *              -> MMAP to GPU memory and update individuals.
 *                  call draw with Transform Idxs instead of transforms.
 *
 *
 *
 *      Hit detection has to do with with a mouse hitting some object.
 *      We can just raytrace to find the nearest object it intersects.
 *          Then we create an event which describes the event.
 *          { mouse(right or left), enum(uint8), pos(x,y,z) }
 *
 *              Then have a processor which can decipher what it is.
*/
