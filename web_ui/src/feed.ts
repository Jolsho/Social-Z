/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import { el } from "./helpers";

export interface VirtRenderable {
    SIZE: number;
    element: HTMLDivElement;
};

export class VirtFeed<T extends VirtRenderable> {
    scrollable: HTMLDivElement;
    renderable: HTMLDivElement;

    items:      T[]     = [];
    offsets:    number[] = [];
    start:      number  = 0;
    end:        number  = 0;

    scroll_pos: number  = 0;

    extras:     [number, number][]  = [];

    observer:   ResizeObserver;

    mounted:    Map<number, T> = new Map<number, T>();
    fetch_more: () => Promise<void>;

    constructor(fetch_more: () => Promise<void>) {
        this.fetch_more = fetch_more;

        this.scrollable = el("div", {
            id: "feed_scrollable",
            className: "feed_scrollable",
        });

        this.scrollable.addEventListener("scroll", () => {
            this.scroll_pos = this.scrollable.scrollTop;

            this.render_items();
        });


        this.renderable = el("div", {
            id: "feed_renderable",
            className: "feed_renderable",
        });
        this.scrollable.appendChild(this.renderable);

        this.observer = new ResizeObserver(entries => {
            for (const entry of entries) {
                const element = entry.target as HTMLElement;
                const idx = Number(element.dataset.index);

                const item = this.items[idx];

                const newSize = entry.contentRect.height;
                const dif = newSize - item.SIZE;

                if (dif === 0) continue;

                item.SIZE = newSize;

                for (let i = idx + 1; i < this.items.length; i++) {
                    this.offsets[i] += dif;
                    if (i > this.start && i <= this.end) {
                        this.items[i].element.style.top = `${this.offsets[i]}px`;
                    }
                }

                this.renderable.style.height =
                    `${this.offsets[this.items.length - 1] + this.items.at(-1)!.SIZE}px`;
            }
        });
    }

    add_item(item: T) {
        let new_offset = 60;
        if (this.items.length > 0) {
            const idx = this.items.length - 1;
            new_offset = this.offsets[idx] + this.items[idx].SIZE;
        }
        this.items.push(item);
        this.offsets.push(new_offset)
        this.renderable.style.height = `${new_offset + item.SIZE}px`
        item.element.dataset.index = (this.items.length - 1).toString();
    }

    attach(root: HTMLDivElement): void {
        root.appendChild(this.scrollable);
        this.scrollable.scrollTop = this.scroll_pos;
        this.render_items();
    }

    async render_items() {
        if (this.items.length <= 0) {
            const pre = this.items.length;
            await this.fetch_more();
            if (pre == this.items.length) return;
        }

        let top = this.scroll_pos - this.scrollable.clientHeight;
        while (top < this.offsets[this.start] && this.start > 0) {
            this.start--;
        }

        while (top > this.offsets[this.start] && this.start < this.items.length) {
            this.start ++;
        }

        const bottom = this.scroll_pos + (2 * this.scrollable.clientHeight);

        this.end = this.start;
        while (true) {
            while (
                this.end < this.items.length &&
                this.offsets[this.end] < bottom
            ) {
                this.end++;
            }

            if (this.end < this.items.length - 1) {
                break;
            }

            const before = this.items.length;
            await this.fetch_more();

            if (this.items.length === before) {
                break;
            }
        }


        while (
            this.end < this.items.length &&
            this.offsets[this.end] < this.scroll_pos + (2 * this.scrollable.clientHeight)
        ) {
            if (this.end == this.items.length - 1) {
                const pre = this.items.length;
                await this.fetch_more();
                if (pre == this.items.length) break;
            }
            this.end++;
        }



        // 1. Remove items that are no longer visible
        for (const [index, item] of this.mounted) {
            if (index < this.start || index >= this.end) {
                this.observer.unobserve(item.element);
                item.element?.remove();
                this.mounted.delete(index);
            }
        }

        // 2. Render items that should be visible
        let accum_extra = 0;
        let extras_idx = 0;
        for (const [idx, ex] of this.extras) {
            if (idx >= this.start) break;
            extras_idx++;
            accum_extra += ex;
        }

        for (let i = this.start; i < this.end && i < this.items.length; i++) {
            if (!this.mounted.has(i)) {
                const item = this.items[i];

                const y_off = this.offsets[i] + accum_extra;
                this.observer.observe(item.element);

                item.element.style.top =       `${y_off}px`;
                if (!item.element.isConnected) {
                    this.renderable.appendChild(item.element);
                }

                this.mounted.set(i, item);

                while (extras_idx < this.extras.length) { 
                    let [idx, extra] = this.extras[extras_idx];
                    if (idx > i) break;
                    accum_extra += extra;
                    extras_idx++;
                }
            }
        }
    }
};

export interface Renderable {
    element: HTMLDivElement;
};

export class Feed<T extends Renderable> {
    scrollable: HTMLDivElement;
    scroll_pos: number      = 0;

    top_spacer    : HTMLDivElement;
    bottom_spacer    : HTMLDivElement;

    items:      T[]         = [];
    prev_h:     number      = 0;
    page:       number      = 1;
    end:        number      = 0;

    load_items: (page: number) => T[];

    constructor(bottom_pad: number, load_items: (page: number) => T[]) {
        this.load_items = load_items;

        this.top_spacer = el("div");
        this.top_spacer.style.height = `70px`;

        this.bottom_spacer = el("div");
        this.bottom_spacer.style.height = `${bottom_pad}px`;

        this.scrollable = el("div", {
            id: "feed_scrollable",
            className: "feed_scrollable",
        });

        this.scrollable.addEventListener("scroll", () => {

            this.scroll_pos = this.scrollable.scrollTop;
            if (this.scrollable.scrollTop <= 50 && this.page > 0) {

                let items = this.load_items(this.page);
                this.prev_h = this.scrollable.scrollHeight;
                if (items.length > 0) {
                    this.items.push(...items) 
                    this.page++;
                } else {
                    this.page = -1;
                    return;
                }

                this.render_items();
            }
        });
    }

    add_item(item: T) {
        this.items.push(item);
    }

    attach(root: HTMLDivElement): void {
        root.appendChild(this.scrollable);
        if (this.scrollable.children.length == 0) {
            this.render_items();
        }
        requestAnimationFrame(() => {
            this.scrollable.scrollTop = this.scroll_pos;
        });
    }

    remove(): void {
        this.scrollable.remove();
    }

    render_items() {
        if (this.items.length <= 0) return;

        this.scrollable.appendChild(this.top_spacer);

        for(let i = this.items.length - 1; i >= this.end; i--) {
            this.scrollable.appendChild(this.items[i].element);
        }

        if (this.end == 0) {
            this.scrollable.appendChild(this.bottom_spacer);
        }
        this.scrollable.scrollTop = this.scrollable.scrollHeight - this.prev_h;
    }
};
