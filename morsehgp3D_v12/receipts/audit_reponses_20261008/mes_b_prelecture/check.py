#!/usr/bin/env python3
"""Temoins synthetiques du lecteur MES-B ; aucun moteur, donnees ou acces distant."""
import argparse
import copy
import hashlib
import importlib.util
import json
import multiprocessing
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).parent

def need(ok, message):
    if not ok:
        raise ValueError(message)

def module(name):
    spec = importlib.util.spec_from_file_location(name, HERE / (name + '.py'))
    obj = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(obj)
    return obj

def fixture(m):
    case = dict(nom='scene', k=5, voie='appareil', passes=1, fils=48)
    full = {k: 0 for k in m.INT_KEYS}
    full.update(phase='full', trame='a', voie='device', status='ok', coord_bits=21,
                kmax=5, threads=48, sites=1_000_000, wall_ns=1_000_000_000,
                etapes_ns={k: 0 for k in m.STAGES}, c_ns={k: 0 for k in m.C_KEYS},
                g_ns={k: 0 for k in m.G_KEYS}, hors_mur_ns={k: 0 for k in m.OUT_KEYS},
                pic_octets=16_000_000, cpu_ns=1_000_000_000, rss_max_octets=16_000_000,
                appareil_octets=100, pic_appareil_octets=100, full_sha256='a' * 64)
    rows = [dict(phase='open', status='ok', reason='none', wall_ns=1, budget_appareil='separe'),
            full, {'phase': 'liberation', 'pass': 0, 'liberation_ns': 1},
            dict(phase='exit', status='ok', reason='none')]
    return case, rows

def wire(rows):
    return '\n'.join(json.dumps(r, sort_keys=True) for r in rows) + '\n'

def overall(criteria):
    return ('tenu' if all(criteria[k]['etat'] != 'non evalue' for k in ('B1', 'B2'))
            and all(v['etat'] in ('tenu', 'non evalue') for v in criteria.values()) else 'non tenu')

def long_label(m):
    name = 'scene_synthetique_00000000000000000001'
    used = set()
    first = m.label_of(name, used)
    second = m.label_of(name, used)
    need(first != second and len(first) <= 23 and len(second) <= 23, 'etiquettes')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pilot', type=Path)
    args = parser.parse_args()
    cap = json.loads((HERE / 'capture.json').read_bytes())
    for row in (HERE / 'SHA256SUMS').read_text().splitlines():
        digest, name = row.split('  ', 1)
        need(hashlib.sha256((HERE / name).read_bytes()).hexdigest() == digest, 'recu modifie : ' + name)
    old, fixed = module('source_excerpt'), module('fixed_excerpt')
    case, rows = fixture(old)
    samples = {'valid': wire(rows)}
    samples['duplicate_cpu'] = samples['valid'].replace('"cpu_ns": 1000000000',
                                                       '"cpu_ns": 7, "cpu_ns": 1000000000')
    for name, index, key, value in [('boolean_free_pass', 2, 'pass', False),
                                   ('open_bad_reason', 0, 'reason', 'device_fault'),
                                   ('u64_overflow', 1, 'cpu_ns', 1 << 64)]:
        changed = copy.deepcopy(rows)
        changed[index][key] = value
        samples[name] = wire(changed)
    states = {}
    for name, text in samples.items():
        a = old.parse_output(0, text, case, 1_000_000, 'a')['etat']
        b = fixed.parse_output(0, text, case, 1_000_000, 'a')['etat']
        need(a == 'ok' and b == ('ok' if name == 'valid' else 'illisible'), name)
        states[name] = [a, b]
    # Only our child is terminated, never a developer process. The source identity and the suffix formula also
    # establish that no iteration can change the label for this name.
    ctx = multiprocessing.get_context('fork')
    child = ctx.Process(target=long_label, args=(old,))
    child.start()
    child.join(0.3)
    blocked = child.is_alive()
    if blocked:
        child.terminate()
        child.join(1)
    need(blocked and not child.is_alive(), 'boucle attendue')
    long_label(fixed)
    results = []
    for k, sites in ((10, 2_000_000), (5, 12_000_000)):
        good = dict(nom='scene_ok', k=5, voie='appareil', etat='ok', raison='', sites=1_000_000,
                    passes=[rows[1]])
        failure = dict(nom='scene_refusee', k=k, voie='appareil', etat='refus',
                       raison='resource_exhausted/memory_budget', sites=sites, passes=[])
        a, b = overall(old.verdicts([good, failure], [])), overall(fixed.verdicts([good, failure], []))
        need((a, b) == ('tenu', 'non tenu'), 'omission refus K%d' % k)
        results.append(dict(k=k, sites=sites, before=a, after=b))
    # An unavailable CPU sample cannot be encoded as the number zero before unsigned subtraction.
    cpu_wrap = (0 - 1_000_000_000) % (1 << 64)
    need(cpu_wrap == 18446744072709551616, 'sous-flux CPU')
    patch_verified = False
    if args.pilot:
        path = 'morsehgp3D_v12/microbancs/mes_b_scenes/pilote_b.py'
        body = args.pilot.read_bytes()
        need(hashlib.sha256(body).hexdigest() == cap['sources_sha256'][path], 'pilote source different')
        with tempfile.TemporaryDirectory(prefix='audit_mes_b_') as folder:
            target = Path(folder) / path
            target.parent.mkdir(parents=True)
            target.write_bytes(body)
            patch = (HERE / 'corrections.patch').resolve()
            subprocess.run(['git', 'apply', '--check', str(patch)], cwd=folder, check=True)
            subprocess.run(['git', 'apply', str(patch)], cwd=folder, check=True)
            compile(target.read_bytes(), str(target), 'exec')
            namespace = {'__name__': 'audit_not_main'}
            exec(compile(target.read_bytes(), str(target), 'exec'), namespace)
            for name, text in samples.items():
                need(namespace['parse_output'](0, text, case, 1_000_000, 'a')['etat'] == states[name][1],
                     'patch lecteur ' + name)
            used = set()
            label = 'scene_synthetique_00000000000000000001'
            need(namespace['label_of'](label, used) != namespace['label_of'](label, used), 'patch etiquettes')
            need(overall(namespace['verdicts']([good, failure], [])) == 'non tenu', 'patch verdict')
        patch_verified = True
    print(json.dumps(dict(ok=True, readers=states, original_label_loop=True, fixed_label_unique=True,
                         omitted_refusals=results, cpu_error_delta=cpu_wrap,
                         patch_applied_to_pinned_source=patch_verified, native_executed=False), sort_keys=True))

if __name__ == '__main__':
    main()
