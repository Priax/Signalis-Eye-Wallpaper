#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

./extract-assets.py --if-missing "${SIGNALIS_DIR:-$HOME/.local/share/Steam/steamapps/common/SIGNALIS}"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build --target signaliseyebus -j >/dev/null
Q=~/.local/lib64/qml/SignalisEye
mkdir -p "$Q"
for f in qmldir libsignaliseyebus.so signaliseyebus.qmltypes; do
    cp "build/native/bus/$f" "$Q/.$f.new"
    mv "$Q/.$f.new" "$Q/$f"
done

mkdir -p ~/.config/plasma-workspace/env
cat >~/.config/plasma-workspace/env/signalis-eye.sh <<'EOF'
export QML_IMPORT_PATH="$HOME/.local/lib64/qml${QML_IMPORT_PATH:+:$QML_IMPORT_PATH}"
EOF

/usr/lib64/qt6/bin/qsb --glsl "100es,120,150" --hlsl 50 --msl 12 \
    -o native/wallpaper/contents/shaders/vignette.frag.qsb native/wallpaper/contents/shaders/vignette.frag

kpackagetool6 --type KWin/Script -u kwin-script >/dev/null 2>&1 || kpackagetool6 --type KWin/Script -i kwin-script >/dev/null
kwriteconfig6 --file kwinrc --group Plugins --key signaliseyecursorEnabled true
qdbus-qt6 org.kde.KWin /Scripting org.kde.kwin.Scripting.unloadScript signaliseyecursor >/dev/null || true
qdbus-qt6 org.kde.KWin /KWin reconfigure

kpackagetool6 --type Plasma/Wallpaper -u native/wallpaper 2>/dev/null || kpackagetool6 --type Plasma/Wallpaper -i native/wallpaper

echo "Installé. Fond d'écran ou plugin modifié: systemctl --user restart plasma-plasmashell.service"
