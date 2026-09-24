#!/usr/bin/env python3
"""Coupes LiDAR 8 000 des juges q2/q3 (R-20), regenerees depuis les trames sans sol versionnees du recu v8.

`scene_cases` de bench/run_lidar_scaling.py controle chaque trame contre son MANIFEST v8 et ecrit les
emboites ; seules les coupes de 8 000 sites sont gardees, sous le nom `lidar_<scene>_8000.u32le`, et
seulement si leur empreinte egale la valeur epinglee (celle des campagnes des juges de l'auditeur C). Les
octets derives de SemanticKITTI ne vont jamais dans le depot versionne : le dossier de sortie doit etre hors
du depot, ou ignore par git. Un fichier deja present et conforme est garde ; sinon tout est regenere dans un
dossier temporaire voisin, puis renomme.

Usage : lidar_judge_cuts.py <dossier_de_sortie>
Code 0 conforme (ligne `lidar_judge_cuts ok ...`) ; 1 empreinte, MANIFEST ou entree/sortie ; 2 usage.
"""
import hashlib
import importlib.util
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
PINNED = {  # empreintes des coupes emboitees 8 000 (regen_inputs.EXPECTED de l'auditeur C)
    '00': 'cf84866390ede2e53a20b4166e8dc1a14cb5791eb78b6efba147941b7cd61c54',
    '01': 'd141e843dcdd3c56d3b396431c9699d94b45b586bc318de70f7aeb77049a6d2a',
    '02': '255f6433034a6d9e1531dce11cb880725c3bf3e46b6b66078596341dbd7f8d5e',
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def outside_versioning(out):
    """'yes' si `out` est hors du depot, ou ignore par git ; 'no' s'il serait versionne ; 'error:...' sinon.

    Un depot sans git (instantane) n'a rien de versionne : `out` y est hors versionnement.
    """
    try:
        out.relative_to(ROOT)
    except ValueError:
        return 'yes'
    probe = out / 'probe.u32le'
    try:
        result = subprocess.run(['git', '-C', str(ROOT), 'check-ignore', '-q', str(probe)], check=False,
                                capture_output=True, text=True)
    except OSError as error:  # git absent : pas de verdict sur le versionnement, refus
        return 'error:git_unavailable:' + type(error).__name__
    if result.returncode == 0:
        return 'yes'
    if result.returncode == 1:
        return 'no'
    repo = subprocess.run(['git', '-C', str(ROOT), 'rev-parse', '--git-dir'], check=False, capture_output=True, text=True)
    if repo.returncode != 0:  # ROOT n'est pas un depot git : rien n'y est versionne
        return 'yes'
    return 'error:git_check_ignore_rc=%d:%s' % (result.returncode, result.stderr.strip()[:200])


def conforming(out):
    for scene, want in PINNED.items():
        path = out / ('lidar_s%s_8000.u32le' % scene)
        if not path.is_file() or sha(path.read_bytes()) != want:
            return False
    return True


def regenerate(out):
    spec = importlib.util.spec_from_file_location('rls', ROOT / 'morsehgp3D_v9/bench/run_lidar_scaling.py')
    rls = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(rls)
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=out) as work_dir:
        work = Path(work_dir)
        staged = []
        for scene, want in PINNED.items():
            tag = 's' + scene
            rls.scene_cases(scene, tag, work)  # refuse une trame differente de son MANIFEST v8
            raw = (work / (tag + '_nested_8000.u32le')).read_bytes()
            got = sha(raw)
            if got != want:
                print('lidar_judge_cuts DIFFERENT scene=%s sha256=%s pinned=%s' % (scene, got, want))
                return False
            target = out / ('lidar_s%s_8000.u32le' % scene)
            tmp = work / (target.name + '.part')
            tmp.write_bytes(raw)
            staged.append((tmp, target))
        for tmp, target in staged:
            os.replace(tmp, target)
    return True


def main(argv):
    if len(argv) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    out = Path(argv[1]).resolve()
    try:
        verdict = outside_versioning(out)
        if verdict == 'no':
            print('lidar_judge_cuts REFUSED output inside the versioned repository: %s' % out)
            return 1
        if verdict != 'yes':
            print('lidar_judge_cuts REFUSED %s' % verdict)
            return 1
        if not conforming(out) and not regenerate(out):
            return 1
        if not conforming(out):
            print('lidar_judge_cuts DIFFERENT after regeneration')
            return 1
    except Exception as error:  # MANIFEST refuse, lecture ou ecriture : jamais un succes
        print('lidar_judge_cuts ERROR %s: %s' % (type(error).__name__, error))
        return 1
    print('lidar_judge_cuts ok ' + ' '.join('s%s=%s' % (scene, want[:16]) for scene, want in PINNED.items()))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
