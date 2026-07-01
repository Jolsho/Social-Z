import type { AppState } from "./app";
import { VirtFeed } from "./feed";
import { el } from "./helpers";
import { Post } from "./social/post";

export class PersonPage {
    name:       string      = "person";
    is_temp:    boolean     = true;

    posts:      VirtFeed<Post>      = new VirtFeed<Post>();

    root:       HTMLDivElement;

    constructor(name: string, app: AppState) {
        this.root = el("div", {
            className: "person_page_root"
        });


        let profile = el("div", {
            className: "person_profile"
        });
        this.root.appendChild(profile);
        for (let i = 0; i < 50; i++) {
            this.posts.add_item(new Post(app));
        }
        this.posts.attach(profile);

        let heading = el("div", {
            className: "profile_heading"
        });
        profile.appendChild(heading);

        heading.appendChild(el("img", {
            className: "search_icon",
            src: "icons/icons_search.png"
        }))

        let info = el("div", {
            className: "profile_info"
        });
        heading.appendChild(info);

        info.appendChild(el("h1", {
            textContent: name
        }))


        let img_container = el("div", {
            className: "profile_image_container"
        });
        img_container.appendChild(el("img", {
            src: "https://picsum.photos/500/500"
        }));
        heading.appendChild(img_container);


        /*
         *  TODO 
         *      Links to other accounts.
         *
         *      And a feed.
         *      What they have posted recently (not expired yet locally)
         *      And what you have saved from them.
         *      Have view of this in the database
         *
         *      But you do need a way to get older stuff.
         *      Like you need a way to search for stuff.
         *      And I think that would need to be asking their node.
         *      Just to be honest I think that is the only way.
         */
    }

    attach(app: AppState): void {
        app.body.appendChild(this.root);

    }
    
    remove(): void {
        this.root.remove();
    }
}
