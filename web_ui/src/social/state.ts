import { el } from "../helpers";
import type { AppState } from "../app";
import { VirtFeed } from "../feed";
import { Post } from "./post";
import { MESSAGE, SETTINGS, SOCIAL } from "../macros";
import type { Icon } from "../head-foot";

export class SocialState {
    is_temp:    boolean         = false;
    root:       HTMLDivElement;
    name:       string          = SOCIAL;

    posts:      VirtFeed<Post>      = new VirtFeed<Post>();

    icons:      Icon[] = [];

    constructor(app: AppState) {
        this.root = el('div', { 
            id: 'social', 
            className: "social_root"
        });
        this.root.addEventListener("click", this.handler);

        for (let i = 0; i < 50; i++) {
            this.posts.add_item(new Post(app));
        }

        this.icons= [
            {
                id: "FOOTER" + SETTINGS,
                src: "icons/i_mandalla.png", 
                handler: () => app.render_page(SETTINGS),
            },
            {
                id: "FOOTER" + MESSAGE,
                src: "icons/i_gear.png", 
                handler: () => app.render_page(MESSAGE),
            }
        ];
    };

    handler(e: PointerEvent) {
          const target = e.target as HTMLElement;

          if (target.matches(".menu-toggle")) {
            //toggleMenu();
          }

          if (target.matches(".profile-btn")) {
            //openProfile();
          }
    }

    attach(app: AppState): void {
        app.body.appendChild(this.root);
        this.posts.attach(this.root);
        app.footer.reload_icons(this.icons);
    };

    remove(): void { 
        this.root.remove();
    }

};
