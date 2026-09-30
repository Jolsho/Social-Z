/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "wrld/wrld.h"
#include "client.h"
#include "wrld/camera.h"

int wrld_init(World* wrld) {
    int r;

    if ((r = bvh_init(&wrld->bvh)) != CLIENT_OK) {
        return r;
    }

    memset(&wrld->input, 0, sizeof(InputState));

    memset(&wrld->handlers, 0, sizeof(Handler) * MAX_HANDLER_ID);


    // TODO -- somehow get access to this screen dimensions?
    
    camera_init(
        &wrld->camera,
        (Vec3){0.0f, 2.0f, 5.0f},
        1.0471976f,             // 60 degree fov
        (float)16 / (float)9    // aspect ratio
    );

    return CLIENT_OK;
}


void wrld_derive_ray(World* wrld, Ray* ray) {
    if (wrld->view_state < TWO_DIM) {

        ray->origin = (Vec3) {
            wrld->input.mouse.x, 
            wrld->input.mouse.y, 
            wrld->camera.position.v[2]
        };

        if (wrld->view_state == MEDIA)
            ray->direction = (Vec3){ 0, 0, -1 };

        else if (wrld->view_state == THREAD)
            ray->direction = (Vec3){ 0, 0, 1 };

        else if (wrld->view_state == MARKET)
            ray->direction = (Vec3){ -1, 0, 0 };

        else
            ray->direction = (Vec3){ 1, 0, 0 };

    } else {
        *ray = camera_ray(&wrld->camera, wrld->screen_x, wrld->screen_y);
    }
}


bool client_update_wrld(struct Client* cli) {

    World* wrld = &cli->wrld;

    EntityID id;
    HandlerID h_id;
    GlobalID g_id;
    Handler handle;

    Ray ray;
    wrld_derive_ray(wrld, &ray);

    size_t hit_cnt = 4;
    EntityID hits[hit_cnt];

    bvh_raycast(&wrld->bvh, ray, hits, &hit_cnt);

    /*
     *  Its not about btn/key pressed
     *      That can just initiate something like (is_dragging & start_mouse)
     *
     *  Most of the time you are checking just btns/keys
     *
     *  Then you check btns/keys release to actuallly trigger click events
     *      depends on if dragging or something.
    */

    // focused object get first pass
    id = wrld->focused;
    if (id != ENTITY_ID_INVALID && (g_id = entity_global(id)) > 0) {
        if ((h_id = entity_handler(id)) < MAX_HANDLER_ID) {
            handle = wrld->handlers[h_id];

            if (
                wrld->input.mouse.btns_pressed > 0 && 
                handle.click != NULL
            )
                handle.click(id, cli);

            if (handle.key_pres != NULL) 
                handle.key_pres(id, cli);
        }
    }

    // Handle the rest of the objects being pointed at.
    for (int i = 0; i < hit_cnt; i++) {
        id = hits[i];

        // IF POINTING AT ITEM
        if ((g_id = entity_global(id)) > 0) {

            if ((h_id = entity_handler(id)) < MAX_HANDLER_ID) {

                handle = wrld->handlers[h_id];

                if (handle.hover != NULL) 
                    handle.hover(id, cli);

                if (
                    wrld->input.mouse.btns_pressed > 0 && 
                    handle.click != NULL
                )
                    handle.click(id, cli);

                if (
                    wrld->input.keys_pressed[0] > 0 ||
                    wrld->input.keys_pressed[1] > 0 && 
                    handle.key_pres != NULL
                )
                    handle.key_pres(id, cli);

            }
        }
    }


    // Remaining global input handling
    // being like movement and what not
    if (
        wrld->input.keys_pressed[0] > 0 ||
        wrld->input.keys_pressed[1] > 0
    ) {

        if (input_key_was_released(&wrld->input, KEY_ESCAPE)) {
            // TODO --> close everything safelly
            return false;
        }
    }


    // HANDLE SCROLLING A FEED
    if (wrld->view_state < TWO_DIM) {
        if (wrld->input.mouse.wheel != 0) {
            ScrollState* ss = &wrld->scrolls[wrld->view_state];
            ss->scroll_height += wrld->input.mouse.wheel;

            for (
                int i = ss->anchor_item_idx; 
                i < feed_get_size(&cli->post_feed); 
                wrld->input.mouse.wheel > 0 ? i++ : i--
            ) {
                //  TODO
                // if (post_offset <=  ss->offset <= post_offset + height,)
                // Dont know how to determine the height of the fucking thing.
                // have an array of heights?
                // i mean it should be that and if we dont have the height
                // that initiates entitizing the thing.
                //
                // this may cause a fetch.
                // in the case we are fetching and try to render beyond.
                // add a loading icon past last post and hold scroll_offset there.
            }
        }
    }

    return true;
}
