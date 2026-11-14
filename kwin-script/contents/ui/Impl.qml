import QtQuick
import org.kde.kwin

Item {
    id: root

    property string covered: "-"
    property bool showingDesktop: false
    readonly property bool allCovered: covered !== "" && covered !== "-"
                                       && covered.split(";").length === Workspace.screens.length

    DBusCall {
        id: sendCursor
        service: "org.priax.SignalisEye"
        path: "/"
        dbusInterface: "org.priax.SignalisEye"
        method: "Cursor"
    }

    DBusCall {
        id: queryShowing
        service: "org.kde.KWin"
        path: "/KWin"
        dbusInterface: "org.freedesktop.DBus.Properties"
        method: "Get"
        arguments: ["org.kde.KWin", "showingDesktop"]
        onFinished: function(rv) {
            root.showingDesktop = rv[0] === true;
            recompute.restart();
        }
    }

    Timer {
        id: showingChanged
        interval: 20
        onTriggered: queryShowing.call()
    }

    DBusCall {
        id: sendCovered
        service: "org.priax.SignalisEye"
        path: "/"
        dbusInterface: "org.priax.SignalisEye"
        method: "Covered"
    }

    Timer {
        id: flush
        interval: 33
        property bool pending: false
        onTriggered: {
            if (root.allCovered) {
                pending = true;
                return;
            }
            pending = false;
            const p = Workspace.cursorPos;
            sendCursor.arguments = [Math.round(p.x), Math.round(p.y)];
            sendCursor.call();
        }
    }

    onAllCoveredChanged: {
        if (!allCovered && flush.pending)
            flush.start();
    }

    function isShown(w, desk, act) {
        if (w.minimized || w.hidden || w.desktopWindow || w.deleted)
            return false;
        if (!w.onAllDesktops && w.desktops.indexOf(desk) < 0)
            return false;
        return w.activities.length === 0 || w.activities.indexOf(act) >= 0;
    }

    function coveredScreens() {
        if (showingDesktop)
            return "";
        const desk = Workspace.currentDesktop, act = Workspace.currentActivity;
        const rects = [];
        const order = Workspace.stackingOrder;
        for (let k = 0; k < order.length; k++)
            if (isShown(order[k], desk, act))
                rects.push(order[k].frameGeometry);
        const out = [];
        const screens = Workspace.screens;
        for (let s = 0; s < screens.length; s++) {
            const g = screens[s].geometry;
            let all = true;
            for (let i = 0; i < 24 && all; i++) {
                for (let j = 0; j < 14 && all; j++) {
                    const x = g.x + (i + 0.5) * g.width / 24, y = g.y + (j + 0.5) * g.height / 14;
                    all = rects.some(r => x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height);
                }
            }
            if (all)
                out.push([g.x, g.y, g.width, g.height].join(","));
        }
        return out.join(";");
    }

    function publishCovered(force) {
        const s = coveredScreens();
        if (s !== covered || force) {
            covered = s;
            sendCovered.arguments = [s];
            sendCovered.call();
        }
    }

    Timer {
        id: recompute
        interval: 100
        onTriggered: root.publishCovered(false)
    }

    Timer {
        interval: 10000
        repeat: true
        running: true
        onTriggered: root.publishCovered(true)
    }

    function watch(w) {
        w.frameGeometryChanged.connect(recompute.restart);
        w.minimizedChanged.connect(recompute.restart);
        w.desktopsChanged.connect(recompute.restart);
        w.hiddenByShowDesktopChanged.connect(showingChanged.restart);
        w.hiddenChanged.connect(recompute.restart);
    }

    Connections {
        target: Workspace
        function onCursorPosChanged() {
            if (!flush.running)
                flush.start();
        }
        function onWindowAdded(w) { root.watch(w); recompute.restart(); }
        function onWindowRemoved() { recompute.restart(); }
        function onCurrentDesktopChanged() { recompute.restart(); }
        function onCurrentActivityChanged() { recompute.restart(); }
        function onScreensChanged() { recompute.restart(); }
    }

    Component.onCompleted: {
        const ws = Workspace.windows;
        for (let k = 0; k < ws.length; k++)
            watch(ws[k]);
        publishCovered(true);
        queryShowing.call();
    }
}
