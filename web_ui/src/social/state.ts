/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import { el } from "../helpers";
import type { AppState } from "../app";
import { VirtFeed } from "../feed";
import { Post, type RawPost } from "./post";
import { MESSAGE, SETTINGS, SOCIAL } from "../macros";
import type { Icon } from "../head-foot";

import {module} from "../wasm/wasm.ts"

export class SocialState {
    is_temp:    boolean         = false;
    root:       HTMLDivElement;
    name:       string          = SOCIAL;

    posts:      VirtFeed<Post>;
    has_more:   boolean         = true;
    is_loading: boolean         = false;
    page:       number          = 0;

    icons:      Icon[]          = [];

    constructor(app: AppState) {
        this.posts = new VirtFeed<Post>(async () => {

            if (!this.has_more || this.is_loading) return;

            this.is_loading = true;

            if (this.page <= 3) {
                for (let i = 0; i < 10; i++) {
                    let p: RawPost = {
                        from: `PERson ${this.page}${i}`,
                        text: "this is a message",
                        created_at: "6/7/28",
                        media: "",
                    };
                    this.posts.add_item(new Post((i + 1) * (this.page + 1), p));
                }
            } else {
                this.has_more = false;
            }

            // let resp = await fetch(`/social/posts?page=${this.page}`, {
            //     method: "GET", credentials: "include",
            // });
            //
            // if (!resp.ok) {
            //     this.has_more = false;
            //     this.is_loading = false;
            //     return;
            // }
            this.page++;

            // await resp.bytes();

            // TODO -- 
            //      this is where you pass to wasm.
            //      so it can parse the request.
            //      and then it can give us an iterator.
            //      this also means post doesnt have fields
            //      like it has the Post* struct.
            //      or whatever. then it calls get methods.
            //      but that is it...
            
            // this.has_more = count > 0;

            this.is_loading = false;
        });

        this.root = el('div', { id: 'social', className: "social_root" });
        this.root.addEventListener("click", this.handler);

        this.icons= [
            {
                id: "FOOTER" + SETTINGS,
                src: "icons/i_mandalla.png", 
                handler: () => app.render_page(SETTINGS),
            },
            {
                id: "FOOTER" + MESSAGE,
                src: "icons/i_gear.png", 
                handler: () => app.render_page(MESSAGE),
            }
        ];
    };

    handler(e: PointerEvent) {
          const target = e.target as HTMLElement;

          if (target.matches(".menu-toggle")) {
            //toggleMenu();
          }

          if (target.matches(".profile-btn")) {
            //openProfile();
          }
    }

    attach(app: AppState): void {
        app.body.appendChild(this.root);
        this.posts.attach(this.root);
        app.footer.reload_icons(this.icons);
    };

    remove(): void { 
        this.root.remove();
    }
};
