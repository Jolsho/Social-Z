/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import type { Feed } from "../feed";
import { el } from "../helpers"
import type { Msg } from "./msg";

export function render_input(feed: Feed<Msg>): HTMLDivElement {
    let root = el("div", {
        className: "input_root",
    });

    let input = el("textarea", {
        id: "text_input",
        className: "textinput"
    });
    input.rows = 1;
    let height = input.clientHeight;

    input.addEventListener("input", () => {
        input.style.height = "auto";
        input.style.height = input.scrollHeight - 15 + "px";
        if (input.clientHeight != height) {
            height = input.clientHeight;
            let bottom = feed.scrollable.scrollHeight - feed.scrollable.clientHeight;
            if (bottom - feed.scroll_pos < (feed.bottom_spacer.clientHeight / 2)) {
                feed.bottom_spacer.style.height = `${Math.max(height + 30, 70)}px`;
                requestAnimationFrame(() => {
                    feed.scroll_pos = feed.scrollable.scrollHeight - feed.scrollable.clientHeight;
                    feed.scrollable.scrollTop = feed.scroll_pos;
                });
            }
        }
    })
    root.appendChild(input);


    return root;
}
