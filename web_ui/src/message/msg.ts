/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import { el } from "../helpers";

export class Msg {
    root: HTMLDivElement;
    is_own: boolean = false;

    constructor(idx: number) {
        if (idx % 3 == 0) {
            this.is_own = true;
        }

        this.root = el("div", { className: "message_container" });

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

        this.root.appendChild(bubble);

        if (this.is_own) {
            this.root.style.justifyContent = "flex-end";
            bubble.classList.add("own")
        }
    }

    render(): HTMLDivElement  {
        return this.root;
    }

    remove(): void {
        if (!this.root) return;
        this.root.remove()
    }
};

