#!/usr/bin/python3
import hashlib
import json
import os
import pathlib
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
GAME = pathlib.Path(ARGS[0] if ARGS else '~/.local/share/Steam/steamapps/common/SIGNALIS').expanduser()

ASSETRIPPER_URL = ('https://github.com/AssetRipper/AssetRipper/releases/download/2.0.0/'
                   'AssetRipper_linux_x64.tar.xz')
ASSETRIPPER_SHA256 = '6f5d4352d36795b944243eb3ec790ec37dbca6d6756e52605bc8616e042d384e'
BALSAM = '/usr/lib64/qt6/bin/balsam'

COLLECTION = 'sharedassets5.assets'
MESHES = {
    35: ('EyeShine', {0: 'EyeShine_0'}),
    37: ('FacePlane', {0: 'FacePlane_0'}),
    38: ('EyeWhite', {0: 'EyeWhite_0'}),
    41: ('HairPlane', {0: 'HairPlane_0'}),
    42: ('EyeWhiteBack', {0: 'EyeWhiteBack_0'}),
    43: ('FaceLid_Half', {0: 'FaceLid_Half_0'}),
    44: ('FaceLid', {0: 'FaceLid_0'}),
    45: ('FaceLid_Down', {0: 'FaceLid_Down_0'}),
    46: ('FaceLid_Down_Transition', {0: 'FaceLid_Down_Transition_0'}),
    47: ('EyePupil_noArmature', {0: 'EyePupil_noArmature_0', 1: 'EyePupil_noArmature_1'}),
}
TEXTURES = {
    18: ('eyecloseup_eyeWhite', 'eyecloseup_eyeWhite.png'),
    20: ('eyecloseup_hair', 'eyecloseup_hair.png'),
    23: ('eyecloseup_shine', 'eyecloseup_shine.png'),
    27: ('eyecloseup_face', 'eyecloseup_face.png'),
    28: ('eyecloseup_lid', 'eyecloseup_lid.png'),
    29: ('eyecloseup_pupil', 'eyecloseup_pupil.png'),
    30: ('Circular_light_blue', 'circular_light_blue.png'),
}


def fail(msg):
    sys.exit(f'extract-assets: {msg}')


def free_port():
    with socket.socket() as s:
        s.bind(('127.0.0.1', 0))
        return s.getsockname()[1]


class Ripper:
    def __init__(self, workdir):
        archive = workdir / 'ar.tar.xz'
        print('Téléchargement d\'AssetRipper 2.0.0…')
        urllib.request.urlretrieve(ASSETRIPPER_URL, archive)
        if hashlib.sha256(archive.read_bytes()).hexdigest() != ASSETRIPPER_SHA256:
            fail('empreinte SHA-256 d\'AssetRipper inattendue, archive refusée')
        with tarfile.open(archive) as t:
            t.extractall(workdir / 'ar', filter='data')
        exe = workdir / 'ar/AssetRipper.GUI.Free'
        exe.chmod(0o755)
        self.base = f'http://127.0.0.1:{free_port()}'
        self.proc = subprocess.Popen(
            [str(exe), '--headless', '--port', self.base.rsplit(':', 1)[1],
             '--log-path', str(workdir / 'ar.log')],
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
        print('Lecture du jeu (une à deux minutes)…')
        data = urllib.parse.urlencode({'path': str(folder)}).encode()
        urllib.request.urlopen(self.base + '/LoadFolder', data=data, timeout=900)

    def get(self, path, **params):
        query = urllib.parse.urlencode(params)
        return urllib.request.urlopen(f'{self.base}{path}?{query}', timeout=120).read()

    def find_collection(self, name):
        for i in range(200):
            p = json.dumps({'B': {'P': [0]}, 'I': i}, separators=(',', ':'))
            try:
                html = self.get('/Collections/View', Path=p).decode()
            except OSError:
                continue
            if f'<title>{name}</title>' in html:
                return i
        fail(f'collection {name} introuvable: le jeu a-t-il changé?')

    def asset(self, coll, pid, kind, **extra):
        p = json.dumps({'C': {'B': {'P': [0]}, 'I': coll}, 'D': pid}, separators=(',', ':'))
        return self.get(f'/Assets/{kind}', Path=p, **extra)

    def check_name(self, coll, pid, expected):
        yaml = self.asset(coll, pid, 'Yaml').decode()
        name = next((l.split(':', 1)[1].strip() for l in yaml.splitlines() if l.strip().startswith('m_Name:')), None)
        if name != expected:
            fail(f'asset {pid}: attendu « {expected} », trouvé « {name} »: le jeu a-t-il changé?')


def expected_files():
    for _, outputs in MESHES.values():
        for dest in outputs.values():
            yield CONTENTS / 'models' / f'{dest}.mesh'
    for _, dest in TEXTURES.values():
        yield CONTENTS / 'textures' / dest


def main():
    if IF_MISSING and all(f.exists() for f in expected_files()):
        return
    if not (GAME / 'SIGNALIS_Data').is_dir():
        fail(f'jeu introuvable dans {GAME} (donne le dossier en argument)')
    if not os.access(BALSAM, os.X_OK):
        fail(f'{BALSAM} manquant: installe qt6-qtquick3d-devel')

    cache = pathlib.Path(os.environ.get('XDG_CACHE_HOME', '~/.cache')).expanduser()
    cache.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='signalis-extract-', dir=cache) as tmp:
        tmp = pathlib.Path(tmp)
        ripper = Ripper(tmp)
        try:
            ripper.load(GAME)
            coll = ripper.find_collection(COLLECTION)

            models = CONTENTS / 'models'
            models.mkdir(parents=True, exist_ok=True)
            for pid, (name, outputs) in MESHES.items():
                ripper.check_name(coll, pid, name)
                glb = tmp / f'{name}.glb'
                glb.write_bytes(ripper.asset(coll, pid, 'Model.glb'))
                out = tmp / f'balsam_{name}'
                subprocess.run([BALSAM, '-o', str(out), str(glb)], check=True,
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                for sub, dest in outputs.items():
                    src = out / f'meshes/meshes_{sub}__mesh.mesh'
                    if not src.exists():
                        fail(f'balsam n\'a pas produit le sous-mesh {sub} de {name}')
                    shutil.copyfile(src, models / f'{dest}.mesh')
                print(f'  modèle  {name}')

            textures = CONTENTS / 'textures'
            textures.mkdir(parents=True, exist_ok=True)
            for pid, (name, dest) in TEXTURES.items():
                ripper.check_name(coll, pid, name)
                (textures / dest).write_bytes(ripper.asset(coll, pid, 'Image', Extension='png'))
                print(f'  texture {name}')
        finally:
            ripper.close()
    print('Assets extraits. AssetRipper et les fichiers intermédiaires sont supprimés.')


if __name__ == '__main__':
    main()
