/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import type { AppState } from "./app";
import { el } from "./helpers";
import { HEADER, FOOTER } from "./macros";

export class Header {
    root: HTMLDivElement;
    static MAX_ICON: number = 5;

    pages: HTMLDivElement;
    current: string = "";

    constructor() {
        this.root = el('div', {
            id: HEADER,
            className: "app_header"
        });

        this.root.appendChild(el("img", {
            src: "HEADER.png"
        }));

        this.pages = el("div", {
            className: "page_container",
        });
    }

    load_pages(app: AppState) {
        const heights = [30, 50];
        this.current = app.pages[0].name;

        for (let i = 0; i < app.pages.length; i++) {
            let d = el("div", { 
                className: "page", 
                id: app.pages[i].name
            });

            d.addEventListener("click", () => {
                app.render_page(app.pages[i].name);
            });

            if (heights.length - i  == 0) {
                d.style.height = (70) + "%";
                d.classList.add("selected");
                this.current = app.pages[i].name;
            } else if (i < heights.length) {
                d.style.height = heights[i] + "%";
            } else {
                d.style.height = heights[(app.pages.length - 1) - i] + "%";
            }

            this.pages.appendChild(d);
        }
        this.root.appendChild(this.pages);
    }

    change_page(page: string) {
        let old_page = this.pages.querySelector("#" + this.current);
        let new_page = this.pages.querySelector("#" + page);
        if (old_page && new_page) {
            old_page.classList.remove("selected");
            new_page.classList.add("selected");
            this.current = page;
        }
    }
};

export class Icon {
    src: string;
    id: string;
    handler: () => void;

    constructor(id: string, src: string, handler: () => void) {
        this.id = id;
        this.src = src;
        this.handler = handler;
    }
};

export class Footer {
    root: HTMLDivElement;
    static MAX_ICON: number = 2;

    icon_container: HTMLDivElement;
    icons: Icon[] = [];

    constructor() {
        this.root = el('div', {
            id: FOOTER, 
            className: "app_footer" 
        });

        this.root.addEventListener("click", (e) => this.handler(e));


        this.icon_container = el('div', {
            className: "footer_icon_container"
        });
        this.root.appendChild(this.icon_container);
    }

    handler(e: PointerEvent) {
        const target = e.target as HTMLElement;

        for (const icon of this.icons) {
            if (target.id == icon.id) {
                icon.handler();
                return;
            }
        }
    }


    reload_icons(icons: Icon[]) {
        this.icon_container.innerHTML = '';
        icons.forEach((icon, idx) => this.load_icon(icon, idx));
    }

    load_icon(icon: Icon, idx: number) {
        if (idx > Footer.MAX_ICON) return;
        let backdrop = el('div', {
            className: "backdrop",
        });
        if (idx % 2 == 0) backdrop.classList.add("flipped");

        backdrop.appendChild(el('img', {
            src: "foot.png"
        }));
        backdrop.appendChild(el('img', {
            id: icon.id, 
            className: "hoverable_icon icon",
            src: icon.src,
        }));
        this.icon_container.appendChild(backdrop);
        this.icons.push(icon);
    }
};
