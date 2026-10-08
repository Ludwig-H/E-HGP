#!/usr/bin/env python3
"""Metadata/JSON and abstract ownership only; never starts an engine or CUDA."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def ctest(path, total, skipped):
    text = path.read_text()
    rows = re.findall(r'^\s*(\d+)/(\d+) Test\s+#\d+:\s+(\S+)\s+\.+(.*?)\s+\d+\.\d+ sec\s*$', text, re.M)
    need(len(rows) == total and {int(r[0]) for r in rows} == set(range(1, total + 1)), 'CTest selection')
    need(all(int(r[1]) == total for r in rows), 'CTest denominator')
    passed = sum('Passed' in r[3] for r in rows)
    skip = [r[2] for r in rows if 'Skipped' in r[3]]
    need(passed + len(skip) == total and len(skip) == skipped, 'CTest outcomes')
    need(f'100% tests passed, 0 tests failed out of {total}' in text, 'CTest footer')
    return {'selected': total, 'passed': passed, 'skipped': skip,
            'budget_gates': [r[2] for r in rows if any(k in r[2] for k in ('pipeline_budget', 'finish_budget', 'stream_staging', 'device_open_budget'))]}


def reader_delta(evidence):
    old, new = module(evidence / 'old_reader.py', 'old'), module(evidence / 'reader.py', 'new')
    # Structural synthetic row, no geometry/performance/physical-memory claim.
    row = dict(phase='full', pass_=0, trame='synthetic', voie='device', status='ok', coord_bits=21,
               kmax=5, threads=3, sites=8, wall_ns=900, pic_octets=128, cpu_ns=None,
               rss_max_octets=None, appareil_octets=0, epinglee_octets=0, pic_appareil_octets=0,
               full_sha256='a' * 64, memoire_octets={s: [128, 128] for s in new.MEM_STAGES})
    row['pass'] = row.pop('pass_')
    row.update({n: {k: 1 for k in keys} for n, keys in new.FULL_BLOCKS.items()})
    row['etapes_ns'].update(G=10, TMVR=10)
    cases = {'nominal': row}
    for name in ('wall_zero', 'sites_zero', 'wrong_sites', 'used_over_peak', 'peak_mismatch', 'stages_over_wall', 'tmvr_over_envelope', 'g_over_envelope'):
        r = copy.deepcopy(row)
        if name == 'wall_zero':
            r['wall_ns'] = 0
            r['etapes_ns'] = {x: 0 for x in r['etapes_ns']}
            r['g_ns'] = {x: 0 for x in r['g_ns']}
        elif name == 'sites_zero': r['sites'] = 0
        elif name == 'wrong_sites': r['sites'] = 9
        elif name == 'used_over_peak': r['memoire_octets']['C'] = [129, 128]
        elif name == 'peak_mismatch': r['pic_octets'] = 129
        elif name == 'stages_over_wall': r['wall_ns'] = 22
        elif name == 'tmvr_over_envelope': r['etapes_ns']['TMVR'] = 3
        elif name == 'g_over_envelope': r['etapes_ns']['G'] = 1
        cases[name] = r
    out = {}
    for name, r in cases.items():
        a = old.check_full_row(r, 'synthetic', 5, 3, 0)
        b = new.check_full_row(r, 'synthetic', 5, 3, 0, 8)
        need(a == '' and (b == '') == (name == 'nominal'), 'reader delta: ' + name)
        out[name] = {'before': 'accepted', 'after': 'accepted' if b == '' else 'refused'}
    return out


def ownership_model():
    """One exclusive array, separate live-allocation and reservation sets.

    Abstract CUDA success/free semantics; not CUDA error propagation or a C++ execution.
    The stream is drained before any owner is dropped. No caller retains an alias.
    """
    class Owner:
        def __init__(self, old):
            self.cap = old
            self.ptr = 1 if old else None
            self.live = {1: old} if old else {}
            self.res = old
            self.seq = 1
            self.freed = set()
            self.queued = {1} if old else set()

        def free(self, ptr):
            need(ptr in self.live and ptr not in self.freed and ptr not in self.queued, 'free ownership/drain')
            self.freed.add(ptr)
            del self.live[ptr]

        def grow(self, n, keep, limit, fault=None):
            if n <= self.cap: return 'ok'
            target = max(n, self.cap + self.cap // 2 + 1024)
            if not keep and self.ptr is not None:
                if fault == 'release_sync': return 'device_fault'
                self.queued.clear()
                self.free(self.ptr)
                self.ptr, self.cap, self.res = None, 0, 0
            if self.res + target > limit or fault == 'reserve': return 'memory_budget'
            self.res += target
            if fault == 'malloc':
                self.res -= target
                return 'memory_budget'
            self.seq += 1
            fresh = self.seq
            self.live[fresh] = target
            if fault in ('copy', 'adopt_sync'):
                # Temporary cleanup before release of its reservation; old owner survives.
                self.free(fresh)
                self.res -= target
                return 'device_fault'
            self.queued.clear()
            if self.ptr is not None: self.free(self.ptr)
            self.res -= self.cap
            self.ptr, self.cap = fresh, target
            return 'ok'

        def invariant(self):
            need(self.res == sum(self.live.values()) == self.cap, 'reservation/owner invariant')
            need((self.ptr is None and not self.live) or set(self.live) == {self.ptr}, 'single owner')

        def destroy(self):
            self.queued.clear()
            if self.ptr is not None: self.free(self.ptr)
            self.ptr, self.cap, self.res = None, 0, 0
            self.invariant()

    count = 0
    for old in (0, 1, 1024, 1536):
        for n in (0, old, old + 1, old + 1025):
            for keep in (0, old):
                for fault in (None, 'release_sync', 'reserve', 'malloc', 'copy', 'adopt_sync'):
                    x = Owner(old)
                    result = x.grow(n, keep, 1 << 30, fault)
                    x.invariant()
                    if result == 'memory_budget':
                        # A clean allocator refusal permits a new call with complete initialization.
                        need(x.grow(n, 0, 1 << 30) == 'ok', 'retry')
                        x.invariant()
                    # A sticky device fault is not claimed retryable; only best-effort destruction modeled.
                    x.destroy()
                    count += 1
    x = Owner(1024)
    first = x.grow(1025, 0, 6000 // 4)
    x.invariant()
    first_cap = x.cap
    second = x.grow(1025, 0, 6000 // 4)
    x.invariant()
    result = {'cases': count, 'retry_example_u32': {'budget_bytes': 6000, 'old_cap': 1024, 'n': 1025,
              'first_target': 2560, 'first_result': first, 'capacity_after_refusal': first_cap,
              'retry_result': second, 'retry_capacity': x.cap}}
    need(first == 'memory_budget' and first_cap == 0 and second == 'ok' and x.cap == 1025, 'retry example')
    x.destroy()
    return result


def review(evidence):
    cap = json.loads((HERE / 'capture.json').read_text())
    for name, info in cap['files'].items():
        b = (evidence / name).read_bytes()
        need(len(b) == info['bytes'] and hashlib.sha256(b).hexdigest() == info['sha256'], 'hash ' + name)
    with tempfile.TemporaryDirectory() as tmp:
        dest = Path(tmp) / 'morsehgp3D_v12/src/catalogue/device_cuda.cu'
        dest.parent.mkdir(parents=True)
        dest.write_bytes((evidence / 'base_cuda.cu').read_bytes())
        cp = subprocess.run(['git', 'apply', '--unsafe-paths', str(evidence / 'release.patch')], cwd=tmp, capture_output=True)
        need(cp.returncode == 0, 'patch application')
        need(dest.read_bytes() == (evidence / 'device_cuda.cu').read_bytes(), 'patch matches variant')
    ful = re.findall(r'^(\w+) K(5|10) (bbase902|b902) code=(\d+) ([0-9a-f]{64})\s*$', (evidence / 'ful1.log').read_text(), re.M)
    need(len(ful) == 18 and all(r[3] == '0' for r in ful), 'FUL1 runs')
    pairs = {}
    for name, k, arm, _, digest in ful:
        pair = pairs.setdefault(name + ':K' + k, {})
        need(arm not in pair, 'duplicate FUL1 arm')
        pair[arm] = digest
    need(len(pairs) == 9 and all(set(p) == {'bbase902', 'b902'} and len(set(p.values())) == 1 for p in pairs.values()), 'FUL1 identities')
    return {'ctest': {n: ctest(evidence / fn, total, skipped) for n, fn, total, skipped in
            [('cpu_u21', 'ctest_cpu.log', 706, 1), ('cuda_build_u21', 'ctest_cuda21.log', 56, 0), ('cuda_build_u24', 'ctest_cuda24.log', 56, 0)]},
            'ful1_cpu': {'pairs': 9, 'processes_code0': 18, 'hashes': {k: p['b902'] for k, p in pairs.items()}},
            'reader_delta': reader_delta(evidence), 'ownership_model': ownership_model(), 'patch_matches_variant': True}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    result = review(args.evidence.resolve())
    if args.check:
        need(result == json.loads((HERE / 'results.json').read_text()), 'result drift')
    print(json.dumps(result, indent=2, sort_keys=True))
