"""Mesures locales appariees base (HEAD 895680ff) / nouvelle voie ; identite octet pour octet des sorties."""
import hashlib, json, os, subprocess, sys, time
from pathlib import Path

S = Path('/tmp/claude-0/-home-user-E-HGP/9ba697b8-f2d3-5b88-ac66-6fca14011635/scratchpad')
DATA, OUT = S / 'data', S / 'meas'
BUILDS = {'base': S / 'build/base/mhgp11_full_bench', 'new': S / 'meas_new_bench'}
BUDGET = str(8 * 1024**3)

def run(build, case, mode, workers, kmax=5):
    out = OUT / f'{build}_{case}_m{mode}_w{workers}_k{kmax}.bin'
    argv = [str(BUILDS[build]), str(DATA / f'{case}.u32le'), str(DATA / f'{case}.ids.u32le'), str(out),
            str(kmax), '16', '256', '0', str(2**32 - 1), BUDGET, str(workers), str(mode)]
    started = time.monotonic()
    proc = subprocess.run(argv, capture_output=True, text=True, timeout=900)
    elapsed = time.monotonic() - started
    rows = [json.loads(l) for l in proc.stdout.splitlines() if l.startswith('{')]
    phases = {r['phase']: r for r in rows}
    digest = hashlib.sha256(out.read_bytes()).hexdigest() if out.exists() else None
    if out.exists(): out.unlink()
    full, dom = phases.get('full', {}), phases.get('domain', {})
    rec = {'build': build, 'case': case, 'mode': mode, 'workers': workers, 'kmax': kmax, 'rc': proc.returncode,
           'status': full.get('status'), 'process_s': round(elapsed, 3), 'output_sha256': digest,
           'wall_ms': full.get('wall_ns', 0) / 1e6, 'domain_ms': full.get('domain_ns', 0) / 1e6,
           'forest_ms': full.get('forest_ns', 0) / 1e6, 'cpu_s': full.get('cpu_seconds'),
           'peak_reserved_bytes': full.get('peak_reserved_bytes'),
           'single_pass_ms': dom.get('single_pass_ns', 0) / 1e6, 'sort_ms': dom.get('sort_ns', 0) / 1e6,
           'prefix_ms': dom.get('prefix_ns', 0) / 1e6, 'assembly_ms': dom.get('assembly_ns', 0) / 1e6,
           'compact_ms': dom.get('compact_ns', 0) / 1e6, 'balls': dom.get('catalogue_balls'),
           'catalogue_work': dom.get('catalogue_work'),
           'phases_ms': {k: v / 1e6 for k, v in full.get('phases', {}).items()},
           'orders': [{'k': o['k'], 'births': o['births'], 'nodes': o['nodes'],
                       'timings_ms': {k: v / 1e6 for k, v in o['timings'].items()},
                       'sweep_ms': o['vertical_parallel']['vertical_sweep_ns'] / 1e6,
                       'work': {k: o['work'].get(k) for k in ('descent_steps', 'population_hits', 'catalogue_hits',
                                'census_calls', 'part_meb_presentations', 'census_point_tests', 'unions',
                                'plateaus', 'vertical_reuses')}} for o in full.get('orders', [])]}
    print(json.dumps({k: rec[k] for k in ('build', 'case', 'mode', 'workers', 'kmax', 'status', 'wall_ms',
                                          'domain_ms', 'forest_ms', 'cpu_s')}), flush=True)
    return rec

def main():
    plan = []
    lidar = ['lidar_ng00', 'lidar_ng01', 'lidar_ng02']
    for rep in range(3):
        for case in lidar:
            plan += [('base', case, 2047, 4), ('new', case, 2047, 4), ('new', case, 16379, 4)]
    for case in ['uniform_u18_n8000', 'uniform_u18_n16000', 'uniform_u18_n32000']:
        plan += [('base', case, 2047, 4), ('new', case, 16379, 4)]
    plan += [('base', 'lidar_ng00', 2047, 1), ('new', 'lidar_ng00', 16379, 1)]
    records = [run(*p) for p in plan]
    (OUT / 'records.json').write_text(json.dumps(records, indent=1) + '\n')

if __name__ == '__main__':
    main()
