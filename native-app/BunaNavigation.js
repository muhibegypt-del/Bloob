/* Original Buna focus and page coordinator. A key produces one new state. */
function initial() {
    return { page: "home", zone: "hero", section: 0, nav: 0, focus: 0, effect: "" };
}

function withPage(current, page) {
    return { page: page, zone: current.zone, section: current.section,
        nav: current.nav, focus: current.focus, effect: "" };
}

function route(current, key, count, canPlay) {
    var next = {
        page: current.page,
        zone: current.zone,
        section: current.section,
        nav: current.nav,
        focus: current.focus,
        effect: ""
    };
    if (current.page === "details") {
        if (key === "back") next.page = "home";
        else if (key === "enter") {
            if (canPlay) {
                next.page = "player";
                next.effect = "play";
            } else next.effect = "sources";
        }
        return next;
    }
    if (current.page === "player") {
        if (key === "back") {
            next.page = "details";
            next.effect = "stop";
        } else if (key === "enter") next.effect = "toggle";
        else if (key === "left") next.effect = "seek-back";
        else if (key === "right") next.effect = "seek-forward";
        return next;
    }
    if (key === "back") {
        if (next.zone === "rows") next.zone = "hero";
        else if (next.zone === "nav") next.zone = "hero";
        return next;
    }
    if (next.zone === "nav") {
        if (key === "left") next.nav = Math.max(0, next.nav - 1);
        else if (key === "right") next.nav = Math.min(2, next.nav + 1);
        else if (key === "down") next.zone = "hero";
        else if (key === "enter") {
            next.section = next.nav;
            next.focus = 0;
            next.zone = "hero";
        }
    } else if (next.zone === "hero") {
        if (key === "up") next.zone = "nav";
        else if (key === "down") next.zone = "rows";
        else if (key === "left") next.focus = Math.max(0, next.focus - 1);
        else if (key === "right") next.focus = Math.min(Math.max(0, count - 1), next.focus + 1);
        else if (key === "enter") next.page = "details";
    } else {
        if (key === "up") next.zone = "hero";
        else if (key === "left") next.focus = Math.max(0, next.focus - 1);
        else if (key === "right") next.focus = Math.min(Math.max(0, count - 1), next.focus + 1);
        else if (key === "enter") next.page = "details";
    }
    return next;
}
