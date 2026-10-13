/* Native source controller. Account keys remain inside the local TV service. */
var BASE = "http://127.0.0.1:8787/api/";

function request(path, body, done) {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (xhr.readyState !== 4) return;
        var response;
        try { response = JSON.parse(xhr.responseText); }
        catch (_) { done("The local service returned unreadable data."); return; }
        if (xhr.status < 200 || xhr.status >= 300) {
            done(response.error || "The local service could not complete the request.");
            return;
        }
        done(null, response);
    };
    xhr.onerror = function() { done("The local Bloob service is unavailable."); };
    xhr.open(body ? "POST" : "GET", BASE + path);
    if (body) xhr.setRequestHeader("Content-Type", "application/json");
    xhr.send(body ? JSON.stringify(body) : null);
}

function score(candidate) {
    var claims = candidate.claims || {};
    var scoreByResolution = { "4K": 40, "1080p": 30, "720p": 15, "480p": 5, "SD": 5 };
    var points = scoreByResolution[claims.resolution] || 10;
    var hdr = claims.hdr || [];
    var audio = claims.audio || [];
    if (hdr.indexOf("DV") >= 0) points += 6;
    else if (hdr.length) points += 5;
    if (audio.indexOf("Atmos") >= 0) points += 4;
    if (candidate.sourceType === "REMUX") points -= 6;
    if (["CAM", "TS", "SCR"].indexOf(candidate.sourceType) >= 0) points -= 60;
    if (candidate.kind === "magnet") {
        if (candidate.seeders === 0) points -= 30;
        else if (candidate.seeders !== undefined) {
            points += Math.min(15, 5 * Math.log(candidate.seeders + 1) / Math.LN10);
            if (candidate.seeders < 5) points -= 8;
        }
    }
    return points;
}

function rank(candidates) {
    return candidates.slice().sort(function(a, b) { return score(b) - score(a); });
}

function recommendable(candidate) {
    return ["CAM", "TS", "SCR"].indexOf(candidate.sourceType) < 0 &&
        !(candidate.kind === "magnet" && candidate.seeders === 0);
}

function label(candidate) {
    var claims = candidate.claims || {};
    var parts = [claims.resolution || "Video"];
    if (claims.hdr && claims.hdr.length) parts.push(claims.hdr.indexOf("DV") >= 0 ? "Dolby Vision" : claims.hdr[0]);
    if (claims.audio && claims.audio.indexOf("Atmos") >= 0) parts.push("Atmos");
    return parts.join("  ·  ");
}

function detail(candidate) {
    var claims = candidate.claims || {};
    var parts = [candidate.sourceType, claims.videoCodec, claims.channels ? claims.channels + " audio" : ""];
    if (candidate.sizeBytes) parts.push((candidate.sizeBytes / 1073741824).toFixed(1) + " GB");
    if (candidate.seeders !== undefined) parts.push(candidate.seeders + " seeders");
    return parts.filter(function(value) { return !!value; }).join("  ·  ");
}

/* Every continuation checks active() so Back cancels UI and polling work. */
function resolve(candidate, hooks) {
    function fail(error) { if (hooks.active()) hooks.error(error); }
    function call(path, body, then) {
        request(path, body, function(error, response) {
            if (!hooks.active()) return;
            if (error) { fail(error); return; }
            then(response);
        });
    }
    function finish(response) {
        if (response.state === "ready" && response.url) {
            hooks.ready(response.url);
        } else if (response.state === "qualities" && response.streams && response.streams.length) {
            hooks.choose("quality", response.streams, function(option) {
                hooks.progress("Getting the selected version…");
                call("link/quality", { id: response.id, stream: option.id }, finish);
            });
        } else if (response.state === "waiting" && response.delayed) {
            hooks.progress("Preparing the stream…");
            var attempts = 0;
            function pollDelayed() {
                if (!hooks.active()) return;
                call("link/delayed", { id: response.delayed }, function(next) {
                    if (next.state === "ready") finish(next);
                    else if (next.state === "error" || ++attempts >= 60) fail("The stream could not be prepared.");
                    else hooks.schedule(4000, pollDelayed);
                });
            }
            hooks.schedule(4000, pollDelayed);
        } else fail("No playable link was returned.");
    }
    function unlock(link) {
        hooks.progress("Getting the stream…");
        call("link/unlock", { link: link }, finish);
    }
    function files(id) {
        hooks.progress("Finding the video file…");
        call("magnet/files", { id: id }, function(response) {
            var options = (response.files || []).filter(function(file) {
                return !/(^|[\s._-])(sample|trailer|extras?)([\s._-]|$)/i.test(file.name);
            });
            if (!options.length) options = response.files || [];
            if (!options.length) { fail("This release contains no video file."); return; }
            options.sort(function(a, b) { return b.size - a.size; });
            var wanted = candidate.fileName ? candidate.fileName.toLowerCase().split("/").pop() : "";
            var exact = options.filter(function(file) { return file.name.toLowerCase() === wanted; });
            if (exact.length === 1) unlock(exact[0].link);
            else if (options.length === 1 || options[0].size >= options[1].size * 1.5) unlock(options[0].link);
            else hooks.choose("file", options, function(option) { unlock(option.link); });
        });
    }
    if (candidate.kind === "link") { unlock(candidate.value); return; }
    hooks.progress("Checking release availability…");
    call("magnet/add", { magnet: candidate.value }, function(response) {
        if (response.ready) { files(response.id); return; }
        var attempts = 0;
        function poll() {
            if (!hooks.active()) return;
            call("magnet/status", { id: response.id }, function(status) {
                if (status.ready) files(response.id);
                else if (status.failed || ++attempts >= 150) fail("This release is unavailable or took too long.");
                else {
                    hooks.progress(status.status || "Waiting for the release…");
                    hooks.schedule(4000, poll);
                }
            });
        }
        hooks.schedule(4000, poll);
    });
}
