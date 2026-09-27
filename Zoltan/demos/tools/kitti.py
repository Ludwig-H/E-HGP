"""Accès aux trames SemanticKITTI sans télécharger les archives complètes.

Les archives officielles (KITTI odometry velodyne, 84,8 Go ; labels
SemanticKITTI, 179 Mo) acceptent les requêtes HTTP partielles : on lit le
répertoire central du zip, puis uniquement les membres demandés. Les octets
lus sont mis en cache dans ``Zoltan/demos/_cache/`` (ignoré par git) et
vérifiés par sha256 quand l'empreinte est connue. Aucun octet KITTI n'est
versionné dans le dépôt (licence CC BY-NC-SA).
"""
from __future__ import annotations

import hashlib
import io
import os
import time
import urllib.request
import zipfile
from pathlib import Path

import numpy as np

VELODYNE_ZIP = 'https://s3.eu-central-1.amazonaws.com/avg-kitti/data_odometry_velodyne.zip'
LABELS_ZIP = 'https://www.semantic-kitti.org/assets/data_odometry_labels.zip'
CACHE = Path(__file__).resolve().parents[1] / '_cache'

THING = {10: 'car', 11: 'bicycle', 13: 'bus', 15: 'motorcycle', 16: 'on-rails', 18: 'truck',
         20: 'other-vehicle', 30: 'person', 31: 'bicyclist', 32: 'motorcyclist',
         252: 'car', 253: 'bicyclist', 254: 'person', 255: 'motorcyclist', 256: 'on-rails',
         257: 'bus', 258: 'truck', 259: 'other-vehicle'}
STUFF = {0: 'unlabeled', 1: 'outlier', 40: 'road', 44: 'parking', 48: 'sidewalk', 49: 'other-ground',
         50: 'building', 51: 'fence', 52: 'other-structure', 60: 'lane-marking', 70: 'vegetation',
         71: 'trunk', 72: 'terrain', 80: 'pole', 81: 'traffic-sign', 99: 'other-object'}
# Classes « void » de l'évaluation panoptique SemanticKITTI (learning_map -> 0) : retirées avant
# l'appariement, donc exclues de l'IoU (elles restent dans le nuage et dans la hiérarchie).
VOID = (0, 1, 52, 99)
FR = {'car': 'voiture', 'bicycle': 'vélo', 'bus': 'bus', 'motorcycle': 'moto', 'truck': 'camion',
      'other-vehicle': 'autre véhicule', 'person': 'piéton', 'bicyclist': 'cycliste',
      'motorcyclist': 'motard', 'unlabeled': 'non étiqueté', 'outlier': 'aberrant', 'road': 'route',
      'parking': 'parking', 'sidewalk': 'trottoir', 'other-ground': 'autre sol', 'building': 'bâtiment',
      'fence': 'clôture', 'other-structure': 'autre structure', 'lane-marking': 'marquage',
      'vegetation': 'végétation', 'trunk': 'tronc', 'terrain': 'terrain', 'pole': 'poteau',
      'traffic-sign': 'panneau', 'other-object': 'autre objet', 'on-rails': 'tram'}


def class_name(sem: int) -> str:
    return THING.get(int(sem)) or STUFF.get(int(sem), str(int(sem)))


class HttpRangeFile(io.RawIOBase):
    """Fichier distant en lecture seule, par blocs, via l'en-tête Range."""

    def __init__(self, url: str, block: int = 1 << 20):
        self.url, self.block, self.pos = url, block, 0
        with urllib.request.urlopen(urllib.request.Request(url, method='HEAD'), timeout=60) as r:
            self.size = int(r.headers['Content-Length'])
        self.cache: dict[int, bytes] = {}

    def seekable(self): return True
    def readable(self): return True
    def tell(self): return self.pos

    def seek(self, off, whence=0):
        self.pos = off if whence == 0 else (self.pos + off if whence == 1 else self.size + off)
        return self.pos

    def _fetch(self, a: int, b: int) -> bytes:
        for attempt in range(6):
            try:
                req = urllib.request.Request(self.url, headers={'Range': f'bytes={a}-{b - 1}'})
                with urllib.request.urlopen(req, timeout=120) as r:
                    if r.status != 206:  # un serveur qui ignore Range renverrait l'archive entière
                        raise IOError(f'HTTP {r.status} au lieu de 206 (Range ignoré)')
                    data = r.read(b - a + 1)
                if len(data) != b - a:
                    raise IOError('short read')
                return data
            except Exception:
                if attempt == 5:
                    raise
                time.sleep(2 ** attempt)
        raise IOError('unreachable')

    def read(self, n=-1):
        if n is None or n < 0:
            n = self.size - self.pos
        n = min(n, self.size - self.pos)
        if n <= 0:
            return b''
        if n > 4 * self.block:
            data = self._fetch(self.pos, self.pos + n)
        else:
            out, p, end = [], self.pos, self.pos + n
            while p < end:
                k = p // self.block
                if k not in self.cache:
                    a = k * self.block
                    self.cache[k] = self._fetch(a, min(a + self.block, self.size))
                    if len(self.cache) > 64:
                        self.cache.pop(next(iter(self.cache)))
                blk = self.cache[k]
                q = min(end, (k + 1) * self.block)
                out.append(blk[p - k * self.block:q - k * self.block])
                p = q
            data = b''.join(out)
        self.pos += len(data)
        return data

    def readinto(self, b):
        d = self.read(len(b))
        b[:len(d)] = d
        return len(d)


_zips: dict[str, zipfile.ZipFile] = {}


def _zip(url: str) -> zipfile.ZipFile:
    if url not in _zips:
        local = CACHE / Path(url).name
        _zips[url] = zipfile.ZipFile(local if local.is_file() else HttpRangeFile(url))
    return _zips[url]


def _member(url: str, name: str, cached: Path) -> bytes:
    if cached.is_file():
        return cached.read_bytes()
    data = _zip(url).read(name)
    cached.parent.mkdir(parents=True, exist_ok=True)
    tmp = cached.with_suffix(cached.suffix + '.part')
    tmp.write_bytes(data)
    os.replace(tmp, cached)
    return data


def _labels(seq: str, frame: str) -> bytes:
    return _member(LABELS_ZIP, f'dataset/sequences/{seq}/labels/{frame}.label', CACHE / 'labels' / f'{seq}_{frame}.label')


def labels_digest(seq: str, frame: str) -> str:
    return hashlib.sha256(_labels(seq, frame)).hexdigest()


def load_frame(seq: str, frame: str, sha256: str | None = None, labels_sha256: str | None = None):
    """Retourne (xyzi float32 (n,4), label uint32 (n,), sha256 du .bin) ; vérifie les empreintes données."""
    raw = _member(VELODYNE_ZIP, f'dataset/sequences/{seq}/velodyne/{frame}.bin',
                  CACHE / 'velodyne' / f'{seq}_{frame}.bin')
    digest = hashlib.sha256(raw).hexdigest()
    if sha256 is not None and digest != sha256:
        raise ValueError(f'{seq}/{frame}: sha256 {digest} != {sha256}')
    lab = _labels(seq, frame)
    if labels_sha256 is not None and hashlib.sha256(lab).hexdigest() != labels_sha256:
        raise ValueError(f'{seq}/{frame}: labels sha256 différent de {labels_sha256}')
    xyzi = np.frombuffer(raw, np.float32).reshape(-1, 4)
    label = np.frombuffer(lab, np.uint32)
    if len(label) != len(xyzi):
        raise ValueError('labels/points length mismatch')
    return xyzi, label, digest
