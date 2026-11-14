import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Kirigami.FormLayout {
    property var configDialog
    property var wallpaperConfiguration

    property alias cfg_Fps: fps.value
    property alias cfg_IdleFps: idleFps.value
    property alias cfg_Brightness: brightness.value

    QQC2.SpinBox {
        id: fps
        Kirigami.FormData.label: "Images par seconde:"
        from: 10
        to: 144
    }

    QQC2.SpinBox {
        id: idleFps
        Kirigami.FormData.label: "Maximum au repos:"
        from: 5
        to: 60
    }

    QQC2.Slider {
        id: brightness
        Kirigami.FormData.label: "Luminosité:"
        from: 0
        to: 1
    }
}
