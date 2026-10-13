/* Fetch only the first catalog window. Keep the UI's small fixture on failure. */
function load(done) {
    var pending = 2;
    var movies = [];
    var series = [];

    function finish() {
        pending -= 1;
        if (pending !== 0) return;
        if (!movies.length && !series.length) return;
        var merged = [];
        var length = Math.max(movies.length, series.length);
        for (var i = 0; i < length; i += 1) {
            if (i < movies.length) merged.push(movies[i]);
            if (i < series.length) merged.push(series[i]);
        }
        merged.push({ id: "tt1254207", name: "Big Buck Bunny", year: "2008", type: "movie",
            note: "The native playback test clip.", test: true });
        done(merged);
    }

    function fetchType(type, receive) {
        var request = new XMLHttpRequest();
        request.onreadystatechange = function() {
            if (request.readyState !== 4) return;
            if (request.status === 200) {
                try {
                    var metas = JSON.parse(request.responseText).metas || [];
                    var items = [];
                    for (var i = 0; i < metas.length && items.length < 10; i += 1) {
                        var item = metas[i];
                        if (!item.id || item.id.indexOf("tt") !== 0 || !item.background) continue;
                        items.push({ id: item.id, name: item.name || "Untitled",
                            year: item.releaseInfo || "", type: type,
                            note: item.description || "Explore this title." });
                    }
                    receive(items);
                } catch (error) { receive([]); }
            } else receive([]);
            finish();
        };
        request.open("GET", "https://v3-cinemeta.strem.io/catalog/" + type + "/top.json");
        request.send();
    }

    fetchType("movie", function(items) { movies = items; });
    fetchType("series", function(items) { series = items; });
}

function firstEpisode(item, done) {
    if (!item || item.type !== "series") { done("", "Play"); return; }
    var request = new XMLHttpRequest();
    request.onreadystatechange = function() {
        if (request.readyState !== 4) return;
        if (request.status !== 200) { done("", "Episodes unavailable"); return; }
        try {
            var videos = JSON.parse(request.responseText).meta.videos || [];
            videos = videos.filter(function(video) { return video.season > 0 && video.id; });
            videos.sort(function(a, b) { return a.season - b.season ||
                (a.episode || a.number || 0) - (b.episode || b.number || 0); });
            if (!videos.length) { done("", "Episodes unavailable"); return; }
            var first = videos[0];
            done(first.id, "Play S" + first.season + " E" + (first.episode || first.number || 1));
        } catch (_) { done("", "Episodes unavailable"); }
    };
    request.onerror = function() { done("", "Episodes unavailable"); };
    request.open("GET", "https://v3-cinemeta.strem.io/meta/series/" + item.id + ".json");
    request.send();
}
