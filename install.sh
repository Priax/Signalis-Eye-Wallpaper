#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

qt_tool() {
    local name=$1 dir key
    for candidate in "${name}6" "${name}-qt6"; do
        command -v "$candidate" && return
    done
    if command -v qtpaths6 >/dev/null; then
        for key in QT_HOST_BINS QT_INSTALL_BINS QT_HOST_LIBEXECS; do
            dir=$(qtpaths6 --query "$key")
            if [ -n "$dir" ] && [ -x "$dir/$name" ]; then
                echo "$dir/$name"
                return
            fi
        done
    fi
    command -v "$name" && return
    echo "install.sh: outil Qt 6 introuvable: $name" >&2
    return 1
}
QSB=$(qt_tool qsb)
QDBUS=$(qt_tool qdbus)

./extract-assets.py --if-missing ${SIGNALIS_DIR:+"$SIGNALIS_DIR"}

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build --target signaliseyebus -j >/dev/null
Q=~/.local/lib64/qml/SignalisEye
mkdir -p "$Q"
for f in qmldir libsignaliseyebus.so signaliseyebus.qmltypes; do
    cp "build/native/bus/SignalisEye/$f" "$Q/.$f.new"
    mv "$Q/.$f.new" "$Q/$f"
done

mkdir -p ~/.config/plasma-workspace/env
cat >~/.config/plasma-workspace/env/signalis-eye.sh <<'EOF'
export QML_IMPORT_PATH="$HOME/.local/lib64/qml${QML_IMPORT_PATH:+:$QML_IMPORT_PATH}"
EOF

"$QSB" --glsl "100es,120,150" --hlsl 50 --msl 12 \
    -o native/wallpaper/contents/shaders/vignette.frag.qsb native/wallpaper/contents/shaders/vignette.frag

kpackagetool6 --type KWin/Script -u kwin-script >/dev/null 2>&1 || kpackagetool6 --type KWin/Script -i kwin-script >/dev/null
kwriteconfig6 --file kwinrc --group Plugins --key signaliseyecursorEnabled true
"$QDBUS" org.kde.KWin /Scripting org.kde.kwin.Scripting.unloadScript signaliseyecursor >/dev/null || true
"$QDBUS" org.kde.KWin /KWin reconfigure

kpackagetool6 --type Plasma/Wallpaper -u native/wallpaper 2>/dev/null || kpackagetool6 --type Plasma/Wallpaper -i native/wallpaper

echo "Installé. Fond d'écran ou plugin modifié: systemctl --user restart plasma-plasmashell.service"
