import { el } from "./helpers";

export interface VirtRenderable {
    SIZE: number;
    extra: number;
    render(
        feed: HTMLDivElement, 
        y_pos: number, 
        grow: (extra: number) => void
    ): void;

    remove(): void;
};

export class VirtFeed<T extends VirtRenderable> {
    scrollable: HTMLDivElement;
    renderable: HTMLDivElement;

    items:      T[]     = [];
    scroll_pos: number  = 0;
    page:       number  = 1;

    extras:     [number, number][]  = [];


    mounted:    Map<number, T> = new Map<number, T>();

    constructor() {
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
    }

    add_item(item: T) {
        this.items.push(item);
        let new_height = this.items.length * this.items[0].SIZE;
        this.renderable.style.height = `${new_height}px`
    }

    attach(root: HTMLDivElement): void{
        root.appendChild(this.scrollable);
        this.scrollable.scrollTop = this.scroll_pos;
        this.render_items();
    }

    render_items() {
        const buffer = 3;

        if (this.items.length <= 0) return;

        const start = Math.max(0, Math.floor(this.scroll_pos / this.items[0].SIZE - buffer));
        const end = Math.floor(start + (this.scrollable.clientHeight / this.items[0].SIZE)  + buffer * 2);

        // 1. Remove items that are no longer visible
        for (const [index, item] of this.mounted) {
            if (index < start || index >= end) {
                item.remove();
                this.mounted.delete(index);
            }
        }

        // 2. Render items that should be visible
        let accum_extra = 0;
        let extras_idx = 0;
        for (const [idx, ex] of this.extras) {
            if (idx >= start) break;
            extras_idx++;
            accum_extra += ex;
        }

        for (let i = start; i < end && i < this.items.length; i++) {
            if (!this.mounted.has(i)) {
                const item = this.items[i];
                let grow_item = (extra: number) => {
                    let ex_idx = this.extras.findIndex(([idx, _]) => idx >= i);

                    if (ex_idx < this.extras.length && ex_idx !== -1) {
                        let [idx, ex] = this.extras[ex_idx];

                        if (idx == i) {
                            ex += extra;

                            if (ex <= 0) {
                                this.extras.splice(ex_idx, 1);
                            } else {
                                this.extras[ex_idx][1] = ex;
                            }
                        } else {
                            // insert before first larger index
                            this.extras.splice(ex_idx, 0, [i, extra]);
                        }

                    } else {
                        this.extras.push([i, extra]);
                    }

                    this.mounted.clear();
                    this.render_items();
                };

                const y_off = (i * this.items[0].SIZE) + accum_extra;
                item.render(this.renderable, y_off, grow_item);
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
    root: HTMLDivElement;
    render(): HTMLDivElement;
    remove(): void;
};

export class Feed<T extends Renderable> {
    scrollable: HTMLDivElement;
    scroll_pos: number      = 0;

    spacer    : HTMLDivElement;

    items:      T[]         = [];
    prev_h:     number      = 0;
    page:       number      = 1;
    end:        number      = 0;

    load_items: (page: number) => T[];

    constructor(bottom_pad: number, load_items: (page: number) => T[]) {
        this.load_items = load_items;

        this.spacer = el("div");
        this.spacer.style.height = `${bottom_pad}px`;

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
        for(let i = this.items.length - 1; i >= this.end; i--) {
            let item = this.items[i].render();
            this.scrollable.appendChild(item);
        }

        if (this.end == 0) {
            this.scrollable.appendChild(this.spacer);
        }
        this.scrollable.scrollTop = this.scrollable.scrollHeight - this.prev_h;
    }
};
