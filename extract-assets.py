#!/usr/bin/python3
import hashlib
import html
import json
import os
import pathlib
import platform
import re
import shutil
import socket
import subprocess
import sys
import tarfile
import tempfile
import time
import urllib.parse
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parent
CONTENTS = ROOT / 'native/wallpaper/contents'
ARGS = [a for a in sys.argv[1:] if a != '--if-missing']
IF_MISSING = '--if-missing' in sys.argv[1:]

ASSETRIPPER_RELEASE = 'https://github.com/AssetRipper/AssetRipper/releases/download/2.0.0/'
ASSETRIPPER_BUILDS = {
    'x86_64': ('AssetRipper_linux_x64.tar.xz',
               '6f5d4352d36795b944243eb3ec790ec37dbca6d6756e52605bc8616e042d384e'),
    'aarch64': ('AssetRipper_linux_arm64.tar.xz',
                '8031752621d7fb27addab6108bad8bfbd43546bbb07b36912a987dfb4029c914'),
}
MACHINE_ALIASES = {'amd64': 'x86_64', 'arm64': 'aarch64'}

STEAM_ROOTS = [
    '~/.local/share/Steam',
    '~/.steam/steam',
    '~/.var/app/com.valvesoftware.Steam/.local/share/Steam',
    '~/snap/steam/common/.local/share/Steam',
]
GAME_DIR = 'SIGNALIS'

PREFERRED_COLLECTION = 'sharedassets5.assets'
MESHES = {
    'EyeShine': {0: 'EyeShine_0'},
    'FacePlane': {0: 'FacePlane_0'},
    'EyeWhite': {0: 'EyeWhite_0'},
    'HairPlane': {0: 'HairPlane_0'},
    'EyeWhiteBack': {0: 'EyeWhiteBack_0'},
    'FaceLid_Half': {0: 'FaceLid_Half_0'},
    'FaceLid': {0: 'FaceLid_0'},
    'FaceLid_Down': {0: 'FaceLid_Down_0'},
    'FaceLid_Down_Transition': {0: 'FaceLid_Down_Transition_0'},
    'EyePupil_noArmature': {0: 'EyePupil_noArmature_0', 1: 'EyePupil_noArmature_1'},
}
TEXTURES = {
    'eyecloseup_eyeWhite': 'eyecloseup_eyeWhite.png',
    'eyecloseup_hair': 'eyecloseup_hair.png',
    'eyecloseup_shine': 'eyecloseup_shine.png',
    'eyecloseup_face': 'eyecloseup_face.png',
    'eyecloseup_lid': 'eyecloseup_lid.png',
    'eyecloseup_pupil': 'eyecloseup_pupil.png',
    'Circular_light_blue': 'circular_light_blue.png',
}

ROW = re.compile(r'<tr data-class="([^"]+)"><td>(-?\d+)</td><td>[^<]*</td><td><a [^>]*>([^<]*)</a>')


def fail(msg):
    sys.exit(f'extract-assets: {msg}')


def find_qt_tool(name):
    for candidate in (f'{name}6', f'{name}-qt6'):
        path = shutil.which(candidate)
        if path:
            return path
    qtpaths = shutil.which('qtpaths6')
    if qtpaths:
        for key in ('QT_HOST_BINS', 'QT_INSTALL_BINS', 'QT_HOST_LIBEXECS'):
            result = subprocess.run([qtpaths, '--query', key], capture_output=True, text=True)
            candidate = pathlib.Path(result.stdout.strip()) / name
            if result.stdout.strip() and os.access(candidate, os.X_OK):
                return str(candidate)
    return shutil.which(name)


def steam_libraries():
    seen = set()
    for root in STEAM_ROOTS:
        root = pathlib.Path(root).expanduser()
        libraries = [root]
        vdf = root / 'steamapps/libraryfolders.vdf'
        if vdf.is_file():
            text = vdf.read_text(errors='replace')
            libraries += [pathlib.Path(p.replace('\\\\', '\\')) for p in re.findall(r'"path"\s+"([^"]+)"', text)]
        for library in libraries:
            key = os.path.realpath(library)
            if key not in seen:
                seen.add(key)
                yield library


def find_game():
    if ARGS:
        return pathlib.Path(ARGS[0]).expanduser()
    if os.environ.get('SIGNALIS_DIR'):
        return pathlib.Path(os.environ['SIGNALIS_DIR']).expanduser()
    for library in steam_libraries():
        game = library / 'steamapps/common' / GAME_DIR
        if (game / 'SIGNALIS_Data').is_dir():
            return game
    fail('SIGNALIS introuvable dans les bibliothèques Steam: donne son dossier en argument ou via SIGNALIS_DIR')


def assetripper_build():
    machine = platform.machine().lower()
    machine = MACHINE_ALIASES.get(machine, machine)
    if machine not in ASSETRIPPER_BUILDS:
        fail(f'processeur {machine} non pris en charge par AssetRipper')
    return ASSETRIPPER_BUILDS[machine]


def safe_extract(archive, dest):
    with tarfile.open(archive) as t:
        try:
            t.extractall(dest, filter='data')
            return
        except TypeError:
            pass
        root = os.path.realpath(dest)
        for member in t.getmembers():
            target = os.path.realpath(os.path.join(dest, member.name))
            if os.path.commonpath([root, target]) != root or not (member.isfile() or member.isdir()):
                fail(f'archive AssetRipper suspecte: {member.name}')
        t.extractall(dest)


def free_port():
    with socket.socket() as s:
        s.bind(('127.0.0.1', 0))
        return s.getsockname()[1]


class Ripper:
    def __init__(self, workdir):
        filename, sha256 = assetripper_build()
        archive = workdir / filename
        print(f'Téléchargement d\'AssetRipper 2.0.0 ({filename})…')
        urllib.request.urlretrieve(ASSETRIPPER_RELEASE + filename, archive)
        if hashlib.sha256(archive.read_bytes()).hexdigest() != sha256:
            fail('empreinte SHA-256 d\'AssetRipper inattendue, archive refusée')
        safe_extract(archive, workdir / 'ar')
        exe = workdir / 'ar/AssetRipper.GUI.Free'
        exe.chmod(0o755)
        port = free_port()
        self.base = f'http://127.0.0.1:{port}'
        self.proc = subprocess.Popen(
            [str(exe), '--headless', '--port', str(port), '--log-path', str(workdir / 'ar.log')],
            cwd=workdir / 'ar', stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        for _ in range(60):
            try:
                urllib.request.urlopen(self.base + '/', timeout=2)
                break
            except OSError:
                time.sleep(1)
        else:
            fail('AssetRipper ne démarre pas')

    def close(self):
        self.proc.terminate()
        self.proc.wait(timeout=30)

    def load(self, folder):
        print(f'Lecture du jeu dans {folder} (une à deux minutes)…')
        data = urllib.parse.urlencode({'path': str(folder)}).encode()
        urllib.request.urlopen(self.base + '/LoadFolder', data=data, timeout=900)

    def get(self, path, **params):
        query = urllib.parse.urlencode(params)
        return urllib.request.urlopen(f'{self.base}{path}?{query}', timeout=120).read()

    def collections(self):
        found = []
        for i in range(500):
            path = json.dumps({'B': {'P': [0]}, 'I': i}, separators=(',', ':'))
            try:
                page = self.get('/Collections/View', Path=path).decode()
            except OSError:
                break
            title = re.search(r'<title>([^<]*)</title>', page)
            rows = [(cls, int(pid), html.unescape(name)) for cls, pid, name in ROW.findall(page)]
            found.append((i, title.group(1) if title else '', rows))
        return found

    def asset(self, coll, pid, kind, **extra):
        path = json.dumps({'C': {'B': {'P': [0]}, 'I': coll}, 'D': pid}, separators=(',', ':'))
        return self.get(f'/Assets/{kind}', Path=path, **extra)


def locate(collections, cls, name):
    matches = [(i, pid) for i, title, rows in collections for c, pid, n in rows if c == cls and n == name]
    preferred = [(i, pid) for i, pid in matches if collections[i][1] == PREFERRED_COLLECTION]
    if len(preferred) == 1:
        return preferred[0]
    if len(matches) == 1:
        return matches[0]
    if not matches:
        fail(f'{cls} « {name} » introuvable: le jeu a-t-il changé?')
    fail(f'{cls} « {name} » trouvé {len(matches)} fois, impossible de choisir')


def expected_files():
    for outputs in MESHES.values():
        for dest in outputs.values():
            yield CONTENTS / 'models' / f'{dest}.mesh'
    for dest in TEXTURES.values():
        yield CONTENTS / 'textures' / dest


def main():
    if IF_MISSING and all(f.exists() for f in expected_files()):
        return
    if sys.version_info < (3, 8):
        fail('Python 3.8 ou plus récent requis')
    game = find_game()
    if not (game / 'SIGNALIS_Data').is_dir():
        fail(f'{game} ne contient pas SIGNALIS_Data')
    balsam = find_qt_tool('balsam')
    if not balsam:
        fail('balsam (Qt 6 Quick 3D) introuvable: installe le paquet de développement de Qt Quick 3D')

    cache = pathlib.Path(os.environ.get('XDG_CACHE_HOME', '~/.cache')).expanduser()
    cache.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='signalis-extract-', dir=cache) as tmp:
        tmp = pathlib.Path(tmp)
        ripper = Ripper(tmp)
        try:
            ripper.load(game)
            collections = ripper.collections()

            models = CONTENTS / 'models'
            models.mkdir(parents=True, exist_ok=True)
            for name, outputs in MESHES.items():
                coll, pid = locate(collections, 'Mesh', name)
                glb = tmp / f'{name}.glb'
                glb.write_bytes(ripper.asset(coll, pid, 'Model.glb'))
                out = tmp / f'balsam_{name}'
                subprocess.run([balsam, '-o', str(out), str(glb)], check=True,
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                for sub, dest in outputs.items():
                    src = out / f'meshes/meshes_{sub}__mesh.mesh'
                    if not src.exists():
                        fail(f'balsam n\'a pas produit le sous-mesh {sub} de {name}')
                    shutil.copyfile(src, models / f'{dest}.mesh')
                print(f'  modèle  {name}')

            textures = CONTENTS / 'textures'
            textures.mkdir(parents=True, exist_ok=True)
            for name, dest in TEXTURES.items():
                coll, pid = locate(collections, 'Texture2D', name)
                (textures / dest).write_bytes(ripper.asset(coll, pid, 'Image', Extension='png'))
                print(f'  texture {name}')
        finally:
            ripper.close()
    print('Assets extraits. AssetRipper et les fichiers intermédiaires sont supprimés.')


if __name__ == '__main__':
    main()
