# Signalis Eye

Fond d'écran animé pour KDE Plasma sous Wayland.

La scène est reconstruite en Qt Quick 3D à partir des modèles et textures du jeu, avec les réglages
de la scène `MainMenu` (caméra, mouvements de l'œil, ondulation des cheveux lue dans le shader du jeu,
vignette, luminosité). Le rendu s'arrête quand le bureau est caché par des fenêtres ou que l'écran est verrouillé.

## Assets

Ce dépôt ne contient **aucun fichier du jeu**.
Vous pouvez les extraire via le script `extract-assets.py`.
Il faut donc posséder SIGNALIS.

## Prérequis (Fedora)

```
sudo dnf install cmake gcc-c++ qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtquick3d-devel
```

Plus le jeu, installé via Steam (par défaut dans `~/.local/share/Steam/steamapps/common/SIGNALIS`,
sinon le préciser avec `SIGNALIS_DIR=...`).

## Installation

```
./install.sh
```

Puis choisir « Signalis Eye » dans Configurer le bureau et le fond d'écran.
À la toute première installation, se déconnecter puis se reconnecter: Plasma doit trouver le module QML
(`~/.config/plasma-workspace/env/signalis-eye.sh`) et KWin charger son script.

Après une modification du fond d'écran ou du plugin:

```
./install.sh && systemctl --user restart plasma-plasmashell.service
```

Le script KWin, lui, est rechargé directement par `install.sh`.

## Organisation

- `native/wallpaper/`: le paquet Plasma (scène QML, shaders, réglages)
- `native/bus/`: plugin QML en C++. `EyeBus` reçoit de KWin le curseur et les écrans masqués par D-Bus,
  `EyeController` contient toute la logique de l'œil et ne publie une image que si elle change
- `kwin-script/`: script KWin qui envoie la position du curseur et les écrans couverts
- `native/test/`: outils hors Plasma, `grab` (capture) et `bench` (mesure CPU),
  compilés avec `cmake -S . -B build && cmake --build build`, qui produit aussi
  `build/compile_commands.json` pour clangd

## Réglages

Images par seconde, maximum au repos, et luminosité

## Licence

Le code est sous licence MIT (voir `LICENSE`). Elle ne couvre pas les assets de SIGNALIS,
qui restent la propriété de rose-engine et ne font pas partie du dépôt.
