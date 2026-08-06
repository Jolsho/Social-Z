/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import { el } from "../helpers";

export class Msg {
    element: HTMLDivElement;
    is_own: boolean = false;

    constructor(idx: number) {
        if (idx % 3 == 0) {
            this.is_own = true;
        }

        this.element = el("div", { className: "message_container" });

        let bubble = el("div", { className: "bubble" });

        let text = el("p", {
            textContent: `SOMETHING:: ${idx}`,
        });

        let i = 0;
        while(i < idx) {
            text.textContent += "ABC ";
            i++;
        }
        bubble.appendChild(text);

        this.element.appendChild(bubble);

        if (this.is_own) {
            this.element.style.justifyContent = "flex-end";
            bubble.classList.add("own")
        }
    }
};

