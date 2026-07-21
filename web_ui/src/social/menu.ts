/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import type { AppState } from "../app";
import { el } from "../helpers";

export class SocialMenu {
    name:       string  = "social_menu";
    is_temp:    boolean = true;

    root:       HTMLDivElement;

    constructor() {
        this.root = el("div", {
            className: "social_menu",
        });

        let options: [string, () => void][] = [
            ["Search", () => {console.log("1")}],
            ["Following", () => {console.log("2")}],
            ["Followers", () => {console.log("3")}],
            ["Privacy", () => {console.log("4")}],
        ];
        for (const [title, callback] of options) {
            let option_container = el("div", {
                className: "social_option"
            });
            option_container.appendChild(el("h1", {
                textContent: title,
                onclick: callback
            }));
            this.root.appendChild(option_container);
        }
    }

    attach(app: AppState): void {
        app.body.appendChild(this.root);
    }

    remove(): void {
        this.root.remove();
    }
}
