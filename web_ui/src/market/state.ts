/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import type { AppState } from "../app";
import { VirtFeed } from "../feed";
import type { Icon } from "../head-foot";
import { el } from "../helpers";
import { MARKET, MESSAGE, SETTINGS } from "../macros";
import { Product, type RawProduct } from "./product";

export class MarketState {
    is_temp:    boolean         = false;
    root:       HTMLDivElement;
    name:       string          = MARKET;

    products:      VirtFeed<Product>;
    has_more:   boolean         = true;
    is_loading: boolean         = false;
    page:       number          = 0;

    icons:      Icon[] = [];

    constructor(app: AppState) {
        this.root = el('div', { 
            id: 'social', 
            className: "social_root"
        });
        this.root.addEventListener("click", this.handler);

        this.products = new VirtFeed<Product>(async () => {
            if (!this.has_more || this.is_loading) return;

            this.is_loading = true;

            if (this.page <= 3) {
                for (let i = 0; i < 10; i++) {
                    let p: RawProduct = {
                        from: `PERson ${this.page}${i}`,
                        text: "this is a message",
                        created_at: "6/7/28",
                        media: "",
                        price: (i + 1) * 69,
                    };
                    this.products.add_item(new Product(app, p));
                }
            } else {
                this.has_more = false;
            }

            // let resp = await fetch(`/marketplace/products?page=${this.page}`, {
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
        this.products.attach(this.root);
        app.footer.reload_icons(this.icons);
    };

    remove(): void { 
        this.root.remove();
    }

};
