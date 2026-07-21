/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import type { AppState } from "../app";
import { el } from "../helpers";
import { render_post_header } from "./post_head";

export class Post {
    SIZE: number = 520;
    extra: number = 0;
    root: HTMLDivElement;

    from: string = "PERSON";
    description: string = `
        some fake description of a post
        SOME FAKE DESCRIPTION OF A POST
        some fake description of a post
        SOME FAKE DESCRIPTION OF A POST
        some fake description of a post
        SOME FAKE DESCRIPTION OF A POST
        some fake description of a post
        SOME FAKE DESCRIPTION OF A POST
        some fake description of a post
        SOME FAKE DESCRIPTION OF A POST $$$
        `
    date: Date = new Date();
    type: string = "img";

    grow: (extra: number) => void = () => {};

    constructor(_app: AppState) {
        this.root = el("div", {
            className: "post"
        });

        let flex_container = el("div", {
            className: "post_container"
        });
        this.root.appendChild(flex_container);

        render_post_header(flex_container, this.from, this.date);

        if (this.type == "img") {
            let post_img_container = el("div", {
                className: "post_img",
            });
            flex_container.appendChild(post_img_container);

            let post_img = el("img", {
                src: "https://picsum.photos/500/500"
            });
            post_img_container.appendChild(post_img);

            let post_desc = el("p", {
                textContent: this.description,
            });
            let dropped = false;
            post_desc.addEventListener("click", () => {
                let e = 0;
                if (!dropped) {
                    let original_height = post_desc.clientHeight;
                    post_desc.style.webkitLineClamp = "8";
                    if (post_desc.clientHeight == original_height) {
                        post_desc.style.webkitLineClamp = "4";
                        return;
                    }
                    e = 4 * 16;
                } else {
                    post_desc.style.webkitLineClamp = "4";
                    e = -4 * 16;
                }
                dropped = !dropped;
                this.extra += e;
                this.grow(e);
            });
            post_img_container.appendChild(post_desc);
        }
    }

    render(
        feed: HTMLDivElement, y_pos: number, 
        grow: (extra: number) => void
    ): void {
        this.grow = grow;
        this.root.style.top =       `${y_pos}px`;
        this.root.style.height =    `${this.SIZE + this.extra}px`;

        if (this.root.isConnected) return;
        feed.appendChild(this.root);
    }

    remove(): void {
        this.root?.remove();
    }
}

