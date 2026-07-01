import type { AppState } from "../app";
import { Feed } from "../feed";
import type { Icon } from "../head-foot";
import { el } from "../helpers";
import { MESSAGE, SETTINGS } from "../macros";
import { render_input } from "./input";
import { Msg } from "./msg";


export class MsgState {

    root:   HTMLDivElement;
    feed:   HTMLDivElement;
    input:  HTMLDivElement;
    name:   string              = MESSAGE;
    is_temp:    boolean         = false;

    msgs:   Feed<Msg>;

    icons:  Icon[] = [];

    constructor(app: AppState) {
        this.root = el("div", {
            id: "messaging",
            className: "messaging_root"
        });

        this.feed = el("div", {
            className: "messaging_feed"
        });
        this.root.appendChild(this.feed);

        let sent = false;
        this.msgs = new Feed<Msg>(70, (_page: number) => {
            let msgs: Msg[] = [];
            if (sent) return msgs;

            for (let i = 0; i < this.msgs.items.length; i++) {
                msgs.push(new Msg(this.msgs.items.length + i));
            }
            sent = true;
            return msgs;
        });

        for (let i = 0; i < 15; i++) {
            this.msgs.add_item(new Msg(i));
        }

        this.input = render_input(this.msgs);
        this.root.appendChild(this.input);

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

    attach(app: AppState): void {
        app.body.appendChild(this.root);
        this.msgs.attach(this.feed);
        app.footer.reload_icons(this.icons);
    };

    remove():void {
        this.root.remove();
        this.msgs.remove();
    }
};
