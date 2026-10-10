#!/usr/bin/env python3
"""Comparaison descriptive sur rapports épinglés et JSONL O ; aucun moteur/payload."""
import argparse
import hashlib
import json
import statistics
import subprocess
from pathlib import Path

DOC = '7feecde3042ca271f0bea1cb0238eee1bf06f64e'
V11 = 'ac081a06f'
O = 'aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66'
BASE11 = 'morsehgp3D_v11/receipts/developpement_20261007/filtre_g1_avx2/claudeg1/'
AUDIT11 = 'morsehgp3D_v11/receipts/audit_geant_v11_20261007/rapports/'
AUDIT12 = 'morsehgp3D_v12/receipts/'
SOURCES = {
    'cpu11': (DOC, BASE11 + 'gpu_ab_report_ab_k5_16_cpu.json'),
    'gpu5_11': (DOC, BASE11 + 'gpu_ab_report_ab_k5_24_gpu.json'),
    'gpu10_11': (DOC, BASE11 + 'gpu_ab_report_ab_k10_24_gpu.json'),
    'probe11': (V11, 'morsehgp3D_v11/bench/full_probe.cpp'),
    'pilot11': (V11, 'morsehgp3D_v11/bench/gpu_ab.py'),
    'probe12': (O, 'morsehgp3D_v12/bench/full_probe.cpp'),
    'pilot12': (O, 'morsehgp3D_v12/microbancs/mes_full/pilote_full.py'),
    'probeB3': ('81b0883d1', 'morsehgp3D_v12/bench/full_probe.cpp'),
    'pilotB3': ('81b0883d1', 'morsehgp3D_v12/microbancs/mes_t2d_b3/pilote_t2d_b3.py'),
    'H': (DOC, AUDIT11 + 'H_chiffres_qualification_mesure.md'),
    'G': (DOC, AUDIT11 + 'G_verification_locale.md'),
    'mesure': (DOC, 'morsehgp3D_v12/docs/MESURE.md'),
    'leaf_prior': (DOC, AUDIT12 + 'audit_reponses_20261008/cpu_feuilles_finition/README.md'),
    'O_results': (DOC, AUDIT12 + 'audit_reponses_20261010/fullo_temps/results.json'),
    'B3_results': (DOC, AUDIT12 + 'audit_reponses_20261010/b3b_stats/results.json'),
}


def need(ok, why):
    if not ok:
        raise ValueError(why)


def pin(raw):
    return [len(raw), hashlib.sha256(raw).hexdigest()]


def run(args):
    src, source_pins, raw_pins = {}, {}, {}
    for name, (commit, path) in SOURCES.items():
        raw = subprocess.check_output(['git', '-C', str(args.repo), 'show', commit + ':' + path])
        src[name] = raw
        source_pins[name] = [commit, path, *pin(raw)]
    need(b'Nuage refait hors chrono FULL' in src['probe11'] and b'Stopwatch full_clock, index_clock;' in src['probe11'], 'frontière v11')
    need(b'kProbeBlockCache = u64{4} << 30' in src['probe11'], 'cache v11')
    need(b'u32 leaf = 24' in src['probe12'] and b'cache = u64{8} << 30' in src['probe12'], 'défauts v12')
    need(b'const auto t0 = Clock::now();\n  auto cloud = prepare_cloud' in src['probe12'], 'frontière v12')
    need(b'--leaf=' not in src['pilot12'] and b'--cache=' not in src['pilot12'], 'défauts surchargés')
    need(src['probeB3'] == src['probe12'] and b'--leaf=' not in src['pilotB3'] and b'--cache=' not in src['pilotB3'], 'défauts B3b différents')
    reports = {k: json.loads(src[k]) for k in ('cpu11', 'gpu5_11', 'gpu10_11', 'O_results', 'B3_results')}
    v11 = {}
    for key, mode, leaf, reps in [('cpu11', 'base:cpu', 16, 6), ('gpu5_11', 'base:gpu', 24, 6), ('gpu10_11', 'base:gpu', 24, 3)]:
        report = reports[key]
        need(report['workers'] == '48' and report['leaf'] == leaf and report['reps'] == reps and report['warm_passes'] == 10, 'réglages v11')
        need(report['modes'][mode] == ('802811' if key == 'cpu11' else '868347:400' if key == 'gpu5_11' else '868347'), 'mode v11')
        v11[key] = {}
        entries = [c for c in report['warm'] if c['mode'] == mode]
        need(len(entries) == 3, 'trois processus chauds v11')
        for entry in entries:
            passes = entry['passes']; frame = entry['frame'].removeprefix('lidar_')
            need(entry['code'] == 0 and len(passes) == 10 and [p['pass'] for p in passes] == list(range(1, 11)) and all(p['status'] == 'ok' for p in passes), 'passes v11')
            selected = report['warm_medians_ms']['warm|lidar_' + frame + '|w48|' + mode]
            for field in ('wall', 'domain', 'forest'):
                need(statistics.median(p[field + '_ns'] for p in passes[1:]) / 1e6 == selected[field + '_ms'], 'médiane v11')
            v11[key][frame] = {field: statistics.median(p[field + '_ns'] for p in passes[1:]) for field in ('wall', 'domain', 'forest')}
    current, cpu_minus_p = {}, {}
    for tag, group, nprocess, npasses in [('k5', 'k5_appareil', 5, 10), ('cpu', 'k5_cpu', 3, 5), ('k10', 'k10_appareil', 3, 5)]:
        current[group] = {}
        for index, frame in enumerate(('ng00', 'ng01', 'ng02')):
            hot = []
            for process in range(nprocess):
                name = '%s_%s_r%d.jsonl' % (tag, frame, process)
                raw = (args.full_raw / name).read_bytes(); raw_pins[name] = pin(raw)
                rows = [json.loads(line) for line in raw.splitlines()]
                full = [r for r in rows if r.get('phase') == 'full']
                need(rows[-1] == {'phase': 'exit', 'status': 'ok', 'reason': 'none'}, 'sortie O')
                need(len(full) == npasses and [r['pass'] for r in full] == list(range(npasses)), 'passes O')
                need(all(r['trame'] == frame and r['threads'] == 48 and r['coord_bits'] == 21 and r['status'] == 'ok' and r['kmax'] == (10 if tag == 'k10' else 5) and r['voie'] == ('cpu' if tag == 'cpu' else 'device') for r in full), 'régime O')
                need(all(r['etapes_schema'] == 'recouvert' and 0 <= sum(r['etapes_ns'].values()) <= r['wall_ns'] for r in full), 'partition O')
                hot.extend(full[1:])
            expected = reports['O_results']['full']['ng'][group][index]
            wall = statistics.median(r['wall_ns'] for r in hot)
            need(expected[0] == frame and wall == expected[2], 'médiane O déjà admise')
            current[group][frame] = wall
            if tag == 'cpu':
                cpu_minus_p[frame] = dict(hot_passes=len(hot), full_minus_P_ns=statistics.median(r['wall_ns'] - r['etapes_ns']['P'] for r in hot), C_ns=statistics.median(r['etapes_ns']['C'] for r in hot), full_minus_C_ns=statistics.median(r['wall_ns'] - r['etapes_ns']['C'] for r in hot))
    comparisons = []
    b3 = reports['B3_results']
    for frame in ('ng00', 'ng01', 'ng02'):
        item = b3['frames'][frame]
        cles = statistics.median(item['process_medians_ns'][b3['arms'].index('cles')])
        need(cles == item['arm_medians_ns']['cles'], 'médiane des médianes B3 clés')
        old_cpu, old_gpu5, old_gpu10 = [v11[k][frame]['wall'] for k in ('cpu11', 'gpu5_11', 'gpu10_11')]
        nc, ng, nt = [current[k][frame] for k in ('k5_cpu', 'k5_appareil', 'k10_appareil')]
        minus = cpu_minus_p[frame]
        comparisons.append(dict(frame=frame, sites=item['sites'], v11_cpu_K5_ns=old_cpu, v12_O_cpu_K5_ns=nc, cpu_ratio_v12_v11=nc / old_cpu, v11_gpu_K5_ns=old_gpu5, v12_O_gpu_K5_ns=ng, gpu_K5_ratio_v12_v11=ng / old_gpu5, v12_B3_keys_gpu_K5_ns=cles, B3_keys_ratio_v12_v11=cles / old_gpu5, v11_gpu_K10_ns=old_gpu10, v12_O_gpu_K10_ns=nt, gpu_K10_ratio_v12_v11=nt / old_gpu10, cpu_favorable_subtraction={**minus, 'ratio_minus_P_to_v11': minus['full_minus_P_ns'] / old_cpu, 'v11_domain_ns': v11['cpu11'][frame]['domain'], 'v11_forest_ns': v11['cpu11'][frame]['forest']}))
    return dict(schema='audit.v11_v12_comparison.capture.v1', source_files=source_pins, O_raw=raw_pins), dict(schema='audit.v11_v12_comparison.v1', scope='descriptive_unpaired_different_timing_boundaries', comparisons=comparisons, CPU_K10_ng_measured=False, v11_baseline_37_frames=False, O_native_source=O, B3_keys_measured_source='81b0883d1', B3_keys_adopted_source='2aaed1847', v11_stats='one process per frame and mode; median of nine hot passes', O_stats='K5 GPU five processes x nine hot passes; CPU K5/GPU K10 three x four; pooled medians', B3_stats='median of ten process medians, seven hot passes each')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--full-raw', type=Path, required=True)
    parser.add_argument('--write', action='store_true')
    args = parser.parse_args()
    capture, result = run(args)
    for name, obj in [('capture.json', capture), ('results.json', result)]:
        path = Path(__file__).resolve().parent / name
        if args.write:
            path.write_text(json.dumps(obj, ensure_ascii=False, separators=(',', ':')) + '\n')
        else:
            need(obj == json.loads(path.read_bytes()), name + ' divergent')
    print(json.dumps({'status': 'ok', 'O_journals': len(capture['O_raw']), 'frames': 3, 'CPU_ratios': [r['cpu_ratio_v12_v11'] for r in result['comparisons']], 'CPU_minus_P_ratios': [r['cpu_favorable_subtraction']['ratio_minus_P_to_v11'] for r in result['comparisons']]}))


if __name__ == '__main__':
    main()
