/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import { el } from "../helpers";

export type RawPost = {
    from:       string;
    text:       string;
    created_at: string;
    media:      string;
};

export class Post {
    SIZE:       number = 80;
    element:    HTMLDivElement;

    constructor(_idx: number, p: RawPost) {

        this.element = el("div", { className: "post" });

        let flex_container = el("div", { className: "post_container" });
        this.element.appendChild(flex_container);

        let post_header = el("div", { className: "post_header", });
        flex_container.appendChild(post_header);

        post_header.appendChild(el("div", {className: "profile_pic"}));
        post_header.appendChild(el("h1", {textContent: p.from}));
        post_header.appendChild(el("p", {textContent: p.created_at,}));

        let icon_container = el("div", { className: "icon_container", });
        post_header.appendChild(icon_container);

        let arrow = el("img", { src: "icons/i_heart.png", });
        arrow.addEventListener("click", () => { /* TODO */ });
        icon_container.appendChild(arrow);

        let text_container = el("div", { className: "post_text_container", });
        flex_container.appendChild(text_container);

        let desc = el('p', { textContent: p.text });
        desc.classList.add("clamped");
        desc.addEventListener("click", () => { desc.classList.toggle("clamped"); });
        text_container.appendChild(desc);


        if (!!p.media) {
            let post_img_container = el("div", { className: "post_body", });
            flex_container.appendChild(post_img_container);

            let post_img = el("img", { src: "https://picsum.photos/500/500" });
            post_img_container.appendChild(post_img);
        }
    }
}

