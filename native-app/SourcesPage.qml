import QtQuick 2.4
import "BunaSources.js" as Sources

Item {
    id: page
    property var item: null
    property string videoId: ""
    property string mode: "loading"
    property string message: "Finding sources…"
    property var ranked: []
    property var options: []
    property int selected: 0
    property bool paired: false
    property string pinCode: ""
    property string pinSite: ""
    property var pendingCandidate: null
    property var continueChoice: null
    property int generation: 0
    property int firstVisible: Math.max(0, selected - 3)
    property var visibleCandidates: ranked.slice(firstVisible, firstVisible + 6)
    signal play(string url)
    signal back()

    function open(content, episode) {
        generation += 1;
        item = content;
        videoId = episode || "";
        selected = 0;
        ranked = [];
        options = [];
        mode = "loading";
        message = "Finding sources…";
        Sources.request("account", null, function(error, account) {
            if (page.mode === "closed") return;
            if (!error) page.paired = account.paired;
        });
        var requestGeneration = generation;
        Sources.request("sources/search", { type: content.type, id: content.id, videoId: videoId },
            function(error, result) {
                if (requestGeneration !== page.generation) return;
                if (error) { page.mode = "error"; page.message = error; return; }
                page.ranked = Sources.rank(result.sources || []);
                page.mode = "list";
                page.message = page.ranked.length ? "Choose a source" : "No sources found for this title.";
            });
    }

    function handle(key) {
        if (key === "back") {
            if (mode === "choice" || mode === "resolving" || mode === "pairing") {
                generation += 1;
                pollTimer.stop();
                mode = "list";
                message = "Choose a source";
            } else {
                generation += 1;
                pollTimer.stop();
                mode = "closed";
                back();
            }
            return;
        }
        if (key === "up") selected = Math.max(0, selected - 1);
        if (key === "down") selected = Math.min(Math.max(0,
            (mode === "choice" ? options : ranked).length - 1), selected + 1);
        if (key === "enter") {
            if (mode === "list" && ranked.length) chooseCandidate(ranked[selected]);
            else if (mode === "choice" && options.length && continueChoice) {
                var next = continueChoice;
                var option = options[selected];
                mode = "resolving";
                next(option);
            } else if (mode === "error") open(item, videoId);
        }
    }

    function chooseCandidate(candidate) {
        pendingCandidate = candidate;
        if (!paired) { beginPairing(); return; }
        beginResolve(candidate);
    }

    function beginPairing() {
        mode = "pairing";
        message = "Connecting AllDebrid…";
        var requestGeneration = generation;
        Sources.request("pin/start", {}, function(error, result) {
            if (requestGeneration !== page.generation) return;
            if (error) { page.mode = "error"; page.message = error; return; }
            page.pinCode = result.pin;
            page.pinSite = result.url;
            page.message = "Enter this code on your phone";
            function checkPin() {
                if (requestGeneration !== page.generation || page.mode !== "pairing") return;
                Sources.request("pin/check", {}, function(checkError, account) {
                    if (requestGeneration !== page.generation || page.mode !== "pairing") return;
                    if (checkError) { page.message = checkError; return; }
                    if (account.paired) {
                        page.paired = true;
                        page.beginResolve(page.pendingCandidate);
                    } else page.schedule(5000, checkPin);
                });
            }
            page.schedule(5000, checkPin);
        });
    }

    function schedule(ms, callback) {
        pollTimer.stop();
        pollTimer.interval = ms;
        pollTimer.callback = callback;
        pollTimer.start();
    }

    function beginResolve(candidate) {
        var requestGeneration = generation;
        mode = "resolving";
        message = "Preparing source…";
        Sources.resolve(candidate, {
            active: function() { return requestGeneration === page.generation && page.mode !== "closed"; },
            schedule: function(ms, callback) { page.schedule(ms, callback); },
            progress: function(label) { page.message = label; },
            error: function(label) { page.mode = "list"; page.selected = 0; page.message = label; },
            ready: function(url) { page.mode = "closed"; page.play(url); },
            choose: function(kind, choices, continuation) {
                page.options = choices;
                page.continueChoice = continuation;
                page.selected = 0;
                page.mode = "choice";
                page.message = kind === "file" ? "Choose the video file" : "Choose the stream version";
            }
        });
    }

    Timer {
        id: pollTimer
        repeat: false
        property var callback: null
        onTriggered: {
            var next = callback;
            callback = null;
            if (next) next();
        }
    }

    anchors.fill: parent
    visible: false

    Rectangle {
        anchors.fill: parent
        color: "#d9080d16"
    }
    Text {
        x: 100; y: 165; width: 1450
        text: page.item ? page.item.name : "Sources"
        color: "#ffffff"
        font.pixelSize: 72
        font.weight: Font.Bold
        elide: Text.ElideRight
    }
    Text {
        x: 103; y: 268; width: 1600
        text: page.message
        color: page.mode === "error" ? "#ffb8b8" : "#d1dce7"
        font.pixelSize: 29
        elide: Text.ElideRight
    }
    Rectangle {
        x: 1460; y: 178; width: 350; height: 56; radius: 28
        color: page.paired ? "#245447" : "#34465b"
        Text {
            anchors.centerIn: parent
            text: page.paired ? "AllDebrid connected" : "Connect AllDebrid"
            color: "#ffffff"
            font.pixelSize: 23
        }
    }
    Item {
        x: 102; y: 356; width: 1700; height: 620
        visible: page.mode === "list"
        Repeater {
            model: page.visibleCandidates
            delegate: Rectangle {
                property int actualIndex: page.firstVisible + index
                x: 0; y: index * 103
                width: 1650; height: 88; radius: 13
                color: page.selected === actualIndex ? "#f3f7fb" : "#243142"
                border.width: page.selected === actualIndex ? 0 : 1
                border.color: "#445770"
                Text {
                    x: 24; y: 12; width: 670
                    text: Sources.label(modelData) +
                          (actualIndex === 0 && Sources.recommendable(modelData) ? "  ·  Recommended" : "")
                    color: page.selected === actualIndex ? "#101925" : "#ffffff"
                    font.pixelSize: 26
                    font.weight: Font.Bold
                    elide: Text.ElideRight
                }
                Text {
                    x: 24; y: 51; width: 1540
                    text: Sources.detail(modelData) + "   " + modelData.release
                    color: page.selected === actualIndex ? "#4b596a" : "#abb9c8"
                    font.pixelSize: 19
                    elide: Text.ElideRight
                }
            }
        }
    }
    Item {
        x: 103; y: 386; width: 1600; height: 550
        visible: page.mode === "choice"
        Repeater {
            model: page.options.slice(Math.max(0, page.selected - 3), Math.max(0, page.selected - 3) + 6)
            delegate: Rectangle {
                property int actualIndex: Math.max(0, page.selected - 3) + index
                x: 0; y: index * 95; width: 1610; height: 80; radius: 12
                color: actualIndex === page.selected ? "#ffffff" : "#243142"
                Text {
                    anchors.fill: parent; anchors.leftMargin: 23; anchors.rightMargin: 23
                    verticalAlignment: Text.AlignVCenter
                    text: modelData.name || ((modelData.quality || "Video") + "  ·  " + (modelData.ext || "stream"))
                    color: actualIndex === page.selected ? "#101925" : "#ffffff"
                    font.pixelSize: 25
                    elide: Text.ElideRight
                }
            }
        }
    }
    Item {
        x: 103; y: 390; width: 1550; height: 430
        visible: page.mode === "pairing"
        Text {
            x: 0; y: 0
            text: page.pinSite.replace(/^https?:\/\//, "")
            color: "#cbd7e3"
            font.pixelSize: 36
        }
        Text {
            x: 0; y: 88
            text: page.pinCode
            color: "#ffffff"
            font.pixelSize: 110
            font.weight: Font.Bold
            font.letterSpacing: 10
        }
        Text {
            x: 0; y: 260
            text: "Waiting for confirmation…"
            color: "#cbd7e3"
            font.pixelSize: 27
        }
    }
    Text {
        x: 104; y: 1010
        text: "Back to return"
        color: "#90a2b5"
        font.pixelSize: 22
    }
}
