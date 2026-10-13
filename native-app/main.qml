import QtQuick 2.4
import QtMultimedia 5.12
import Eos.Window 0.1
import "BunaNavigation.js" as Buna
import "BunaCatalog.js" as Catalog

WebOSWindow {
    id: root
    appId: "com.muhib.bloob.native"
    title: "Bloob Native Preview"
    width: 1920
    height: 1080
    visible: true
    // The platform may composite video in a plane beneath the QML surface.
    color: "transparent"

    property var catalog: [
        { id: "tt0114709", name: "Toy Story", year: "1995", type: "movie", note: "A story about friendship, change, and finding your place." },
        { id: "tt0133093", name: "The Matrix", year: "1999", type: "movie", note: "A world beneath the world you know." },
        { id: "tt1375666", name: "Inception", year: "2010", type: "movie", note: "An idea can change everything." },
        { id: "tt0816692", name: "Interstellar", year: "2014", type: "movie", note: "A journey beyond the familiar." },
        { id: "tt0903747", name: "Breaking Bad", year: "2008", type: "series", note: "One decision changes the course of a life." },
        { id: "tt0944947", name: "Game of Thrones", year: "2011", type: "series", note: "Power has a price." },
        { id: "tt4574334", name: "Stranger Things", year: "2016", type: "series", note: "A mystery reaches beyond a small town." },
        { id: "tt1254207", name: "Big Buck Bunny", year: "2008", type: "movie", note: "The native playback test clip.", test: true }
    ]
    property var navState: Buna.initial()
    property var sectionItems: navState.section === 0 ? catalog : catalog.filter(function(item) {
        return item.type === (navState.section === 1 ? "movie" : "series");
    })
    property var currentItem: sectionItems.length ? sectionItems[Math.min(navState.focus, sectionItems.length - 1)] : null
    property string notice: ""
    property string selectedVideoId: ""
    property string playLabel: "Play"
    property string playerTitle: "Big Buck Bunny"
    property bool playerIsDemo: false

    // Diagnostic launch parameter for the bundled, freely licensed test clip.
    Component.onCompleted: {
        if (params && params.bunaDemo) {
            var start = Buna.initial();
            start.page = "player";
            start.focus = catalog.length - 1;
            navState = start;
            playerIsDemo = true;
            demoPlayer.source = "http://127.0.0.1:8787/media/big-buck-bunny-480p-30sec.mp4";
            demoPlayer.play();
        } else if (params && params.bunaSources) {
            navState = Buna.withPage(Buna.initial(), "sources");
            sourcesPage.open(catalog[0], "");
        } else {
            Catalog.load(function(items) { root.catalog = items; });
        }
    }

    function art(item, size) {
        return item ? "https://images.metahub.space/background/" + size + "/" + item.id + "/img" : "";
    }

    function handleKey(key) {
        if (navState.page === "sources") {
            sourcesPage.handle(key);
            return;
        }
        var before = navState;
        var selectedItem = currentItem;
        var next = Buna.route(before, key, sectionItems.length, currentItem && currentItem.test === true);
        navState = next;
        if (before.page === "home" && next.page === "details") {
            selectedVideoId = "";
            playLabel = selectedItem && selectedItem.type === "series" ? "Loading episodes…" : "Play";
            if (selectedItem && selectedItem.type === "series") {
                Catalog.firstEpisode(selectedItem, function(id, label) {
                    if (root.currentItem !== selectedItem || root.navState.page !== "details") return;
                    root.selectedVideoId = id;
                    root.playLabel = label;
                });
            }
        }
        if (next.effect === "play") {
            playerIsDemo = true;
            playerTitle = selectedItem ? selectedItem.name : "Big Buck Bunny";
            demoPlayer.source = "http://127.0.0.1:8787/media/big-buck-bunny-480p-30sec.mp4";
            demoPlayer.play();
        } else if (next.effect === "stop") {
            demoPlayer.stop();
            demoPlayer.source = "";
        } else if (next.effect === "toggle") {
            if (demoPlayer.playbackState === MediaPlayer.PlayingState) demoPlayer.pause();
            else demoPlayer.play();
        } else if (next.effect === "seek-back" || next.effect === "seek-forward") {
            if (demoPlayer.seekable) demoPlayer.seek(Math.max(0, Math.min(demoPlayer.duration,
                demoPlayer.position + (next.effect === "seek-back" ? -10000 : 10000))));
        } else if (next.effect === "sources") {
            if (selectedItem && selectedItem.type === "series" && !selectedVideoId) {
                notice = "Episodes are still loading";
                noticeTimer.restart();
            } else {
                navState = Buna.withPage(next, "sources");
                sourcesPage.open(selectedItem, selectedVideoId);
            }
        }
    }

    MediaPlayer {
        id: demoPlayer
        autoPlay: false
        loops: 1
        onPlaybackStateChanged: console.log("BunaMedia state", playbackState)
        onStatusChanged: {
            console.log("BunaMedia status", status, "video", hasVideo,
                        "position", position, "duration", duration);
            if (status === MediaPlayer.EndOfMedia && root.navState.page === "player" && root.playerIsDemo)
                demoLoopTimer.start();
        }
        onError: {
            if (error !== MediaPlayer.NoError) {
                console.log("BunaMedia error", error)
                root.notice = "Playback: " + errorString;
                noticeTimer.restart();
            }
        }
    }

    Timer {
        id: noticeTimer
        interval: 3000
        onTriggered: root.notice = ""
    }

    Timer {
        id: demoLoopTimer
        interval: 1200
        repeat: false
        onTriggered: {
            if (root.navState.page !== "player" || !root.playerIsDemo) return;
            demoPlayer.stop();
            demoPlayer.seek(0);
            demoPlayer.play();
        }
    }

    Item {
        id: inputSurface
        anchors.fill: parent
        focus: true
        Component.onCompleted: forceActiveFocus()
        Keys.onPressed: {
            var key = "";
            if (event.key === Qt.Key_Left) key = "left";
            else if (event.key === Qt.Key_Right) key = "right";
            else if (event.key === Qt.Key_Up) key = "up";
            else if (event.key === Qt.Key_Down) key = "down";
            else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) key = "enter";
            else if (event.key === Qt.Key_Escape || event.key === Qt.Key_Back || event.key === 461) key = "back";
            if (key) {
                root.handleKey(key);
                event.accepted = true;
            }
        }

        Rectangle {
            anchors.fill: parent
            visible: root.navState.page !== "player"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#17263b" }
                GradientStop { position: 0.55; color: "#0c1521" }
                GradientStop { position: 1.0; color: "#05070b" }
            }
        }

        Image {
            id: backdrop
            visible: root.navState.page !== "player"
            x: 620
            y: 0
            width: 1300
            height: 780
            source: root.art(root.currentItem, "medium")
            sourceSize.width: 1080
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
            opacity: status === Image.Ready ? 0.52 : 0
            Behavior on opacity { NumberAnimation { duration: 180 } }
        }

        Rectangle {
            x: 0; y: 0; width: 1920; height: 850
            visible: root.navState.page !== "player"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#b9080b12" }
                GradientStop { position: 0.58; color: "#99080b12" }
                GradientStop { position: 1.0; color: "#080b12" }
            }
        }

        Text {
            x: 96; y: 54
            visible: root.navState.page !== "player"
            text: "BLOOB"
            color: "#ffffff"
            font.family: "Sans"
            font.pixelSize: 30
            font.weight: Font.Bold
            font.letterSpacing: 4
        }

        Row {
            x: 1120; y: 53
            visible: root.navState.page !== "player"
            spacing: 14
            Repeater {
                model: ["Home", "Movies", "TV Shows"]
                delegate: Rectangle {
                    width: navText.implicitWidth + 52
                    height: 54
                    radius: 27
                    color: root.navState.zone === "nav" && root.navState.nav === index ? "#ffffff" : "transparent"
                    Text {
                        id: navText
                        anchors.centerIn: parent
                        text: modelData
                        color: root.navState.zone === "nav" && root.navState.nav === index ? "#08101a" :
                               root.navState.section === index ? "#ffffff" : "#a9b5c4"
                        font.pixelSize: 24
                        font.weight: Font.DemiBold
                    }
                }
            }
        }

        Item {
            visible: root.navState.page === "home"
            anchors.fill: parent

            Text {
                x: 96; y: 267
                text: root.currentItem ? root.currentItem.name : "Bloob"
                color: "#ffffff"
                font.pixelSize: 88
                font.weight: Font.Bold
                font.family: "Sans"
            }
            Text {
                x: 101; y: 390
                text: root.currentItem ? (root.currentItem.year + "  ·  " + (root.currentItem.type === "movie" ? "Film" : "Series")) : ""
                color: "#c7d1db"
                font.pixelSize: 25
            }
            Text {
                x: 100; y: 462; width: 700
                text: root.currentItem ? root.currentItem.note : ""
                color: "#d2dce6"
                font.pixelSize: 27
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }
            Rectangle {
                x: 100; y: 583; width: 226; height: 70; radius: 14
                color: root.navState.zone === "hero" ? "#ffffff" : "#5cffffff"
                Text {
                    anchors.centerIn: parent
                    text: "Explore"
                    color: root.navState.zone === "hero" ? "#09131e" : "#ffffff"
                    font.pixelSize: 27
                    font.weight: Font.DemiBold
                }
            }
            Text {
                x: 100; y: 745
                text: root.navState.section === 0 ? "Featured" : root.navState.section === 1 ? "Films" : "Series"
                color: "#ffffff"
                font.pixelSize: 30
                font.weight: Font.Bold
            }
            Item {
                x: 96; y: 805; width: 1730; height: 270
                clip: true
                Repeater {
                    model: root.sectionItems
                    delegate: Item {
                        id: card
                        property bool selected: root.navState.zone === "rows" && root.navState.focus === index
                        x: (index - root.navState.focus) * 386 + (root.navState.focus > 0 ? 25 : 0)
                        y: selected ? 0 : 9
                        width: 348; height: 220
                        Behavior on x { NumberAnimation { duration: 145; easing.type: Easing.OutCubic } }
                        Behavior on y { NumberAnimation { duration: 105 } }
                        Rectangle {
                            x: 0; y: 0; width: 348; height: 196; radius: 13
                            color: "#243346"
                            border.width: card.selected ? 4 : 1
                            border.color: card.selected ? "#ffffff" : "#4a6077"
                            clip: true
                            Image {
                                anchors.fill: parent
                                source: Math.abs(index - root.navState.focus) <= 3 ? root.art(modelData, "small") : ""
                                sourceSize.width: 480
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                cache: true
                            }
                            Rectangle {
                                x: 0; y: 106; width: 348; height: 90
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#00101928" }
                                    GradientStop { position: 1.0; color: "#df07101a" }
                                }
                            }
                            Text {
                                x: 17; y: 145; width: 312
                                text: modelData.name
                                color: "#ffffff"
                                elide: Text.ElideRight
                                font.pixelSize: 24
                                font.weight: Font.Bold
                            }
                        }
                        Text {
                            x: 2; y: 202
                            text: modelData.year + "  ·  " + (modelData.type === "movie" ? "Film" : "Series")
                            color: card.selected ? "#ffffff" : "#a2b1c2"
                            font.pixelSize: 18
                        }
                    }
                }
            }
        }

        Item {
            visible: root.navState.page === "details"
            anchors.fill: parent
            Text {
                x: 100; y: 182
                text: root.currentItem ? root.currentItem.name : ""
                color: "#ffffff"
                font.pixelSize: 84
                font.weight: Font.Bold
            }
            Text {
                x: 104; y: 304
                text: root.currentItem ? (root.currentItem.year + "  ·  " + (root.currentItem.type === "movie" ? "Film" : "Series")) : ""
                color: "#c7d1db"
                font.pixelSize: 25
            }
            Text {
                x: 102; y: 403; width: 740
                text: root.currentItem ? root.currentItem.note : ""
                color: "#d2dce6"
                font.pixelSize: 30
                wrapMode: Text.WordWrap
                maximumLineCount: 4
                elide: Text.ElideRight
            }
            Rectangle {
                x: 102; y: 573; width: 278; height: 76; radius: 15
                color: "#ffffff"
                Text {
                    anchors.centerIn: parent
                    text: root.playLabel
                    color: "#08101a"
                    font.pixelSize: 28
                    font.weight: Font.Bold
                }
            }
            Text {
                x: 104; y: 690
                text: "Press Back to return to your shelf"
                color: "#aab9c9"
                font.pixelSize: 22
            }
        }

        SourcesPage {
            id: sourcesPage
            visible: root.navState.page === "sources"
            onBack: {
                root.navState = Buna.withPage(root.navState, "details");
            }
            onPlay: {
                root.playerIsDemo = false;
                root.playerTitle = root.currentItem ? root.currentItem.name : "Video";
                root.navState = Buna.withPage(root.navState, "player");
                demoPlayer.source = url;
                demoPlayer.play();
            }
        }

        Item {
            visible: root.navState.page === "player"
            anchors.fill: parent
            VideoOutput {
                anchors.fill: parent
                source: demoPlayer
                fillMode: VideoOutput.PreserveAspectFit
            }
            Rectangle {
                x: 0; y: 820; width: 1920; height: 260
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#00000000" }
                    GradientStop { position: 1.0; color: "#d9000000" }
                }
            }
            Text {
                x: 96; y: 915
                text: (demoPlayer.playbackState === MediaPlayer.PlayingState ? "Playing" :
                       demoPlayer.playbackState === MediaPlayer.PausedState ? "Paused" : "Stopped") + " · " + root.playerTitle
                color: "#ffffff"
                font.pixelSize: 32
                font.weight: Font.Bold
            }
            Rectangle {
                x: 96; y: 46; width: 1050; height: 123; radius: 12
                visible: root.playerIsDemo
                color: "#dc14202d"
                Text {
                    x: 20; y: 14; width: 1010
                    text: "NATIVE PLAYER DIAGNOSTIC"
                    color: "#ffffff"
                    font.pixelSize: 22
                    font.weight: Font.Bold
                }
                Text {
                    x: 20; y: 52; width: 1010
                    text: "state " + demoPlayer.playbackState + "   status " + demoPlayer.status +
                          "   video " + demoPlayer.hasVideo + "   audio " + demoPlayer.hasAudio +
                          "   available " + demoPlayer.availability
                    color: "#cbd8e4"
                    font.pixelSize: 20
                }
                Text {
                    x: 20; y: 82; width: 1010
                    text: "time " + Math.floor(demoPlayer.position / 1000) + "s / " +
                          Math.floor(demoPlayer.duration / 1000) + "s   buffer " +
                          Math.round(demoPlayer.bufferProgress * 100) + "%   error " +
                          demoPlayer.error + " " + demoPlayer.errorString
                    color: "#cbd8e4"
                    font.pixelSize: 20
                    elide: Text.ElideRight
                }
            }
            Rectangle {
                x: 96; y: 994; width: 1728; height: 6; radius: 3
                color: "#6bffffff"
                Rectangle {
                    width: demoPlayer.duration > 0 ? parent.width * Math.min(1, demoPlayer.position / demoPlayer.duration) : 0
                    height: parent.height; radius: 3; color: "#ffffff"
                }
            }
        }

        Rectangle {
            x: 590; y: 945; width: 740; height: 64; radius: 16
            color: "#e8eef5"
            visible: root.notice.length > 0
            Text {
                anchors.centerIn: parent
                text: root.notice
                color: "#0a1420"
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }
        }
    }
}
