"""Small host-only proof; no GPU, no bandwidth or throughput measurement."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
SOURCE = Path('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/bench/g4/cuda_probe.cu')
EXPECTED = '3ea56187e2e406c45ac24aca997bbc99f76c0391f6d1a195820777ad42daeb11'
def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()
if sha(SOURCE) != EXPECTED:
    raise RuntimeError('Source changed; stop without transferring the proof')
src = SOURCE.read_text()
replica = (ROOT / 'host.cpp').read_text()
for bits in (64, 128):
    start = src.index('  uint64_t x = ', src.index('__global__ void loop%d(' % bits))
    stop = src.index('\n}', start)
    body = src[start:stop].replace('a[i]', 'input').replace('out[i] = ', 'return ')
    if body not in replica:
        raise RuntimeError('Host replica differs from CUDA recurrence')
receipt = {'kind': 'host_unsigned_replica_only_not_device_or_engine',
           'cuda_source': str(SOURCE), 'cuda_source_sha256': sha(SOURCE),
           'replica_sha256': sha(ROOT / 'host.cpp'), 'runner_sha256': sha(Path(__file__)),
           'source_equations_checked': True, 'commands': []}
def run(args):
    p = subprocess.run(args, text=True, capture_output=True, timeout=20)
    receipt['commands'].append({'argv': args, 'returncode': p.returncode,
                                'stdout': p.stdout, 'stderr': p.stderr})
    return p
run(['g++', '--version'])
build = run(['g++', '-std=c++17', '-O1', '-fsanitize=undefined',
             '-fno-sanitize-recover=undefined', '-o', str(ROOT / 'host'), str(ROOT / 'host.cpp')])
ok = build.returncode == 0
loops = compares = 0
if ok:
    p = run([str(ROOT / 'host')])
    ok = p.returncode == 0 and not p.stderr
    mask64, mask128 = (1 << 64) - 1, (1 << 128) - 1
    for line in p.stdout.splitlines():
        fields = line.split()
        if fields[0] == 'L':
            v, t, got64, got128 = map(int, fields[1:])
            x, acc = v & mask64, 0
            for i in range(t):
                acc = (acc + x * (x ^ i)) & mask64
                x = (x >> 1) ^ acc
            want64 = acc
            x, acc = v & mask64, 0
            for i in range(t):
                acc = (acc + x * (x ^ i)) & mask128
                x = ((acc >> 7) & mask64) ^ x
            want128 = (acc & mask64) ^ (acc >> 64)
            ok &= (got64, got128) == (want64, want128)
            loops += 1
        else:
            a, b, c, d, got = map(int, fields[1:])
            want = (a * b > c * d) - (a * b < c * d)
            ok &= got == want
            compares += 1
    ok &= loops == 48 and compares == 4096
receipt.update(loop_cases=loops, product_comparisons=compares, status='pass' if ok else 'fail',
               cuda_source_sha256_after=sha(SOURCE))
ok &= receipt['cuda_source_sha256_after'] == EXPECTED
(ROOT / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps({k: receipt[k] for k in ['status', 'kind', 'loop_cases', 'product_comparisons',
                                       'cuda_source_sha256', 'cuda_source_sha256_after']}))
sys.exit(0 if ok else 1)
