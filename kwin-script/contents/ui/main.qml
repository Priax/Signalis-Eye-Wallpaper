import QtQuick

Item {
    Loader {
        source: Qt.resolvedUrl("Impl.qml") + "?v=" + Date.now()
    }
}
