"""Compile et execute la sonde CUDA (cuda_probe.cu) sur la VM G4, avec le nvcc de /usr/local/cuda (hors du PATH).
Python systeme, sans dependance. Ecrit cuda_probe.json, le journal de compilation et la sortie dans --out.

  python3 cuda_probe.py --out DIR --work DIR [--arch sm_120]
Codes de la sonde, propages tels quels : 0 ok, 1 ecart d'exactitude i128, 2 CUDA indisponible, 3 appel CUDA en echec,
5 duree mesuree invalide. Codes propres au lanceur : 4 nvcc introuvable, 6 compilation en echec, 7 sonde tuee par un
signal ou hors delai, 8 sortie incoherente (pas exactement une ligne JSON, ou champ status en desaccord avec le code).
Le code du processus et le contenu se lisent ensemble, jamais le champ status seul.
"""
import glob
import json
import os
import subprocess
import sys

STATUS_OF_CODE = {0: 'ok', 1: 'i128_mismatch', 2: 'no_cuda', 3: 'cuda_error', 5: 'timing_invalid'}


def arg(name, default=None):
    return sys.argv[sys.argv.index(name) + 1] if name in sys.argv else default


def check_output(code, stdout):
    """Code final : celui de la sonde si sa sortie est une seule ligne JSON dont le status correspond, 8 sinon."""
    lines = [line for line in stdout.splitlines() if line.strip()]
    if code not in STATUS_OF_CODE or len(lines) != 1:
        return 8
    try:
        rec = json.loads(lines[0])
    except ValueError:
        return 8
    if not isinstance(rec, dict) or rec.get('status') != STATUS_OF_CODE[code]:
        return 8
    return code


def main():
    out, work, arch = arg('--out'), arg('--work'), arg('--arch', 'sm_120')
    if not out or not work:
        print('REFUS : --out DIR --work DIR attendus')
        return 2
    os.makedirs(out, exist_ok=True)
    os.makedirs(work, exist_ok=True)
    cands = ['/usr/local/cuda/bin/nvcc'] + sorted(glob.glob('/usr/local/cuda-*/bin/nvcc'))
    nvcc = next((c for c in cands if os.path.exists(c)), None)
    if nvcc is None:
        print('nvcc introuvable')
        return 4
    src = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'cuda_probe.cu')
    exe = os.path.join(work, 'cuda_probe')
    cmd = [nvcc, '-O3', '-std=c++17', '-arch=' + arch, '-o', exe, src]
    r = subprocess.run(cmd, capture_output=True, text=True)
    with open(os.path.join(out, 'compile.txt'), 'w') as f:
        f.write(' '.join(cmd) + '\ncode %d\n%s\n%s' % (r.returncode, r.stdout, r.stderr))
    if r.returncode != 0:
        print('compilation en echec, voir compile.txt')
        return 6
    try:
        r = subprocess.run([exe], capture_output=True, text=True, timeout=600)
    except subprocess.TimeoutExpired:
        print('sonde hors delai (600 s)')
        return 7
    with open(os.path.join(out, 'cuda_probe.json'), 'w') as f:
        f.write(r.stdout)
    print(r.stdout.strip())
    if r.stderr:
        print(r.stderr[-2000:])
    if r.returncode < 0:
        print('sonde tuee par le signal %d' % -r.returncode)
        return 7
    code = check_output(r.returncode, r.stdout)
    if code == 8:
        print('sortie incoherente : code %d, sortie non conforme' % r.returncode)
    return code


if __name__ == '__main__':
    sys.exit(main())
