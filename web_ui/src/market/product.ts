/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import type { AppState } from "../app";
import { el } from "../helpers";

export type RawProduct = {
    from:       string;
    text:       string;
    created_at: string;
    media:      string;
    price:      number;
};

export class Product {
    SIZE: number = 200;
    element: HTMLDivElement;

    constructor(_app: AppState, p: RawProduct) {
        this.element = el("div", {
            className: "post"
        });

        let flex_container = el("div", {
            className: "post_container"
        });
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


        let post_body = el("div", { className: "post_body", });
        post_body.appendChild(el("img", { src: "https://picsum.photos/500/500" }));

        let price_container = el("div", { className: "post_price", })
        price_container.appendChild(el("div", { className: "filter", }));
        price_container.appendChild(el("h1", { textContent: `$${p.price}` }));
        post_body.appendChild(price_container);

        let desc = el('p', { textContent: p.text });
        desc.classList.add("clamped");
        desc.addEventListener("click", () => { desc.classList.toggle("clamped"); });
        post_body.appendChild(desc);

        flex_container.appendChild(post_body);
    }

    render(feed: HTMLDivElement, y_pos: number): void {
        this.element.style.top =       `${y_pos}px`;
        //this.element.style.height =    `${this.SIZE + this.extra}px`;

        if (this.element.isConnected) return;
        feed.appendChild(this.element);
    }

    remove(): void {
        this.element?.remove();
    }
}

