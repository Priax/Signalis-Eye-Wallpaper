import QtQuick
import QtQuick.Window
import org.kde.plasma.plasmoid

WallpaperItem {
    id: root

    Scene {
        anchors.fill: parent
        fps: root.configuration.Fps
        idleFps: root.configuration.IdleFps
        brightness: root.configuration.Brightness
        controller.followBus: true
        controller.screenX: Screen.virtualX
        controller.screenY: Screen.virtualY
        controller.screenWidth: Screen.width
        controller.screenHeight: Screen.height
    }
}
