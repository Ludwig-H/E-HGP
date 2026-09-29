"""Faits de la VM G4 pour le plan GPU de la v10 (Python systeme, sans dependance) : GPU et pilote (nvidia-smi),
boite a outils CUDA (nvcc dans le PATH ou sous /usr/local/cuda*), compilateurs, cmake, CPU (drapeaux AVX-512),
memoire et disques. Ecrit vm_facts.json et vm_facts.txt dans --out ; ne modifie rien sur la VM.

  python3 vm_facts.py --out DIR
Code 0 meme si des outils manquent (leur absence est un fait) ; 2 si --out manque.
"""
import glob
import json
import os
import shutil
import subprocess
import sys


def run(cmd, timeout=60):
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        return {'cmd': ' '.join(cmd), 'code': r.returncode, 'stdout': r.stdout[-8000:], 'stderr': r.stderr[-2000:]}
    except (OSError, subprocess.TimeoutExpired) as e:
        return {'cmd': ' '.join(cmd), 'code': None, 'error': repr(e)}


def main():
    if '--out' not in sys.argv:
        print('REFUS : --out DIR attendu')
        return 2
    out = sys.argv[sys.argv.index('--out') + 1]
    os.makedirs(out, exist_ok=True)
    nvcc = shutil.which('nvcc')
    cuda_dirs = sorted(glob.glob('/usr/local/cuda*'))
    nvcc_candidates = ([nvcc] if nvcc else []) + [d + '/bin/nvcc' for d in cuda_dirs if os.path.exists(d + '/bin/nvcc')]
    facts = {
        'nvidia_smi_query': run(['nvidia-smi', '--query-gpu=name,driver_version,compute_cap,memory.total,'
                                 'clocks.max.sm,power.limit', '--format=csv']),
        'nvidia_smi': run(['nvidia-smi']),
        'cuda_dirs': cuda_dirs,
        'nvcc_in_path': nvcc,
        'nvcc_versions': [run([c, '--version']) for c in nvcc_candidates],
        'gxx': run(['g++', '--version']),
        'clang': run(['clang++', '--version']),
        'cmake': run(['cmake', '--version']),
        'docker': run(['docker', '--version']),
        'lscpu': run(['lscpu']),
        'avx512': run(['grep', '-m1', '-o', 'avx512[a-z_]*', '/proc/cpuinfo']),
        'free': run(['free', '-g']),
        'df': run(['df', '-h']),
        'uname': run(['uname', '-a']),
        'os_release': run(['cat', '/etc/os-release']),
        'ldconfig_cuda': run(['sh', '-c', 'ldconfig -p | grep -i -E "cuda|nvrtc|cublas" | head -40']),
        'dpkg_cuda': run(['sh', '-c', 'dpkg -l | grep -i -E "cuda|nvidia" | head -60']),
    }
    with open(os.path.join(out, 'vm_facts.json'), 'w') as f:
        json.dump(facts, f, indent=1)
    with open(os.path.join(out, 'vm_facts.txt'), 'w') as f:
        for k, v in facts.items():
            f.write('== %s\n' % k)
            if isinstance(v, dict):
                f.write((v.get('stdout') or v.get('error') or '') + (v.get('stderr') or '') + '\n')
            else:
                f.write(json.dumps(v, indent=1) + '\n')
    q = facts['nvidia_smi_query']
    print('GPU :', (q.get('stdout') or q.get('error') or '').strip().replace('\n', ' | '))
    print('nvcc :', nvcc, cuda_dirs)
    return 0


if __name__ == '__main__':
    sys.exit(main())
