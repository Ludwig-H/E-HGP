"""Compile et execute la sonde CUDA (cuda_probe.cu) sur la VM G4, avec le nvcc de /usr/local/cuda (hors du PATH).
Python systeme, sans dependance. Ecrit cuda_probe.json, le journal de compilation et la sortie dans --out.

  python3 cuda_probe.py --out DIR --work DIR [--arch sm_120]
Codes : ceux de la sonde (0 exactitude i128 tenue, 1 ecart, 2 CUDA indisponible) ; 3 si la compilation echoue ;
4 si nvcc est introuvable.
"""
import glob
import os
import subprocess
import sys


def arg(name, default=None):
    return sys.argv[sys.argv.index(name) + 1] if name in sys.argv else default


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
        return 3
    r = subprocess.run([exe], capture_output=True, text=True, timeout=600)
    with open(os.path.join(out, 'cuda_probe.json'), 'w') as f:
        f.write(r.stdout)
    print(r.stdout.strip())
    if r.stderr:
        print(r.stderr[-2000:])
    return r.returncode


if __name__ == '__main__':
    sys.exit(main())
