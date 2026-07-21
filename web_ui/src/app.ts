/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import { Header, Footer } from "./head-foot";
import { el } from "./helpers";
import { APP, BODY, SOCIAL } from "./macros";

export interface Page {
    name: string;
    is_temp: boolean;
    attach(app: AppState): void;
    remove(): void;
};

export class AppState {
    pages:  Page[] = [];

    current: string = SOCIAL;
    previous: string = SOCIAL;

    root:   HTMLDivElement;

    header: Header;
    body:   HTMLDivElement;
    footer: Footer;

    constructor() {
        let root = document.querySelector<HTMLDivElement>(`#${APP}`);
        if (!root) {
            root = el('div', {
                id: APP, 
                className: "app_root",
            });
            document.body.appendChild(root);
        }
        this.root = root;

        this.header = new Header();
        this.root.appendChild(this.header.root);

        this.body = el('div', {
            id: BODY,
            className: "app_body",
        });
        this.root.appendChild(this.body);

        this.footer = new Footer();
        this.root.appendChild(this.footer.root);

        let value = 0;
        let startX = 0;

        document.addEventListener("touchstart", (e) => {
            startX = e.touches[0].clientX;
        });

        document.addEventListener("touchend", (e) => {
            const endX = e.changedTouches[0].clientX;
            const deltaX = endX - startX;

            const SWIPE_THRESHOLD = 25;

            let next = value;
            if (deltaX > SWIPE_THRESHOLD) {
                next = Math.max(value-1, 0);
            } else if (deltaX < -SWIPE_THRESHOLD) {
                next = Math.min(value+1, this.pages.length - 1);
            }
            if (next != value) {
                value = next;
                this.render_page(this.pages[value].name);
            }
        });
    }

    render_temp_page(page: Page) {
        this.pages.push(page);
        this.render_page(page.name);
    }

    render_page(page_name?: string) {
        if (!page_name) page_name = this.current;

        for (const page of this.pages) {
            if (page.name == this.current) {
                page.remove();
                if (page.is_temp) {
                    this.pages = this.pages.filter(p => 
                        p.name != page.name
                    );
                } else {
                    this.previous = page.name;
                }
            }
        }

        for (const page of this.pages) {
            if (page.name == page_name) {
                page.attach(this);
                this.current = page_name;
                this.header.change_page(page_name);
            }
        }

    }
};
