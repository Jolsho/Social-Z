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
import { Product } from "./product";

export class MarketState {
    is_temp:    boolean         = false;
    root:       HTMLDivElement;
    name:       string          = MARKET;

    products:      VirtFeed<Product>      = new VirtFeed<Product>();

    icons:      Icon[] = [];

    constructor(app: AppState) {
        this.root = el('div', { 
            id: 'social', 
            className: "social_root"
        });
        this.root.addEventListener("click", this.handler);

        for (let i = 0; i < 50; i++) {
            this.products.add_item(new Product(app));
        }

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
