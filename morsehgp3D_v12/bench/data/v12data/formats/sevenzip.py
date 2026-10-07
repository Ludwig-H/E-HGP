"""Archives 7z : delegue a py7zr (paquet optionnel) ou au binaire 7z / 7za / 7zz ; refus explicite sinon.

La decompression n'est jamais reimplementee. Extraction membre par membre dans un dossier neuf (donnees non fiables :
chemins verifies, aucun lien).
"""
from __future__ import annotations

import shutil
import subprocess
from pathlib import Path


def _binary():
    for name in ('7zz', '7z', '7za'):
        path = shutil.which(name)
        if path:
            return path
    return None


def list_members(archive: Path) -> list:
    try:
        import py7zr
    except ImportError:
        py7zr = None
    if py7zr is not None:
        with py7zr.SevenZipFile(archive, mode='r') as z:
            return [(info.filename, info.uncompressed) for info in z.list() if not info.is_directory]
    binary = _binary()
    if binary is None:
        raise RuntimeError('7z illisible : ni py7zr ni binaire 7z/7za/7zz')
    out = subprocess.run([binary, 'l', '-slt', str(archive)], capture_output=True, text=True, check=True).stdout
    members, name, size = [], None, None
    for line in out.splitlines():
        if line.startswith('Path = '):
            name = line[7:]
        elif line.startswith('Size = '):
            size = int(line[7:] or 0)
        elif line.startswith('Attributes = ') and name is not None:
            if 'D' not in line[13:]:
                members.append((name, size))
            name = None
    return [m for m in members if m[0] != str(archive)]


def extract(archive: Path, members: list, dest: Path) -> list:
    dest.mkdir(parents=True, exist_ok=True)
    for name in members:
        target = (dest / name).resolve()
        if not str(target).startswith(str(dest.resolve())):
            raise RuntimeError('membre suspect : ' + name)
    try:
        import py7zr
    except ImportError:
        py7zr = None
    if py7zr is not None:
        with py7zr.SevenZipFile(archive, mode='r') as z:
            z.extract(path=dest, targets=list(members))
    else:
        binary = _binary()
        if binary is None:
            raise RuntimeError('7z illisible : ni py7zr ni binaire 7z/7za/7zz')
        subprocess.run([binary, 'x', '-y', '-o' + str(dest), str(archive), *members], capture_output=True,
                       check=True)
    out = []
    for name in members:
        path = dest / name
        if not path.is_file() or path.is_symlink():
            raise RuntimeError('membre non extrait ou symbolique : ' + name)
        out.append(path)
    return out
