import { el } from "../helpers";

export function render_post_header(
    container: HTMLDivElement, 
    from: string, 
    t: Date
) { 
    let post_header = el("div", {
        className: "post_header",
    });
    container.appendChild(post_header);

    post_header.appendChild(el("div", {
        className: "profile_pic"
    }));


    post_header.appendChild(el("h1", {
        textContent: from
    }));

    post_header.appendChild(el("p", {
        textContent: `${t.getMonth()} / ${t.getDay()} / ${t.getFullYear() - 2000}`,
    }));

    let icon_container = el("div", {
        className: "icon_container",
    });
    post_header.appendChild(icon_container);

    let arrow = el("img", {
        src: "icons/i_heart.png",
    });
    arrow.addEventListener("click", () => {
    });
    icon_container.appendChild(arrow);
}
