"""LIVE CenterRegion/J2/FullDomain reader. Code0 means coherent evidence, including a failed campaign.

Original local receipt and one original archive are mandatory. No native/cloud execution. Canonical payloads
were removed: their recorded hashes are compared, not recomputed. Historical readers remain unchanged.
"""
import importlib.util
import json
from pathlib import Path
import re
import sys
import tarfile

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('region_meb_reader', HERE.parent / 'meb_20261002/check.py')
meb = importlib.util.module_from_spec(spec)
spec.loader.exec_module(meb)
q4, old, profiles, asan = meb.q4, meb.old, meb.profiles, meb.asan
need, js, sha, fields, BASE = old.need, old.js, old.sha, old.fields, old.BASE
SOURCE = '7f1922c7743d8682e2665a491b01d32e8f2d546c'
CONTRACT_SHA = 'fa774965266fa2f2aa0c7256e771378e15306378445e0c99b1ba3ece04211236'
REGION = {'region_pair_tests', 'region_pair_rejects', 'region_line_tests', 'region_line_rejects'}
# Private imported module instances only: extend the old nine-field contract explicitly for source 7f1922c.
profiles.LOGICAL |= REGION
NEW_TARGETS = {'mhgp11_num_center_region_probe', 'mhgp11_tower_domain', 'mhgp11_tower_domain_fault'}
asan.BINARIES |= NEW_TARGETS
asan.TARGETS |= NEW_TARGETS
asan.BUILD_FILES |= {'CMakeFiles/%s.dir/%s' % (t, f) for t in NEW_TARGETS for f in ('flags.make', 'link.txt')}
EXPECTED = dict(gcc_release=292, mutants=18, gcc_asan_ubsan=217, gcc_tsan=217, clang_release=0,
                bits21=217, bits24=217, poison=218, style=2, gcc_asan_ubsan18=73)
UNIT = {prefix + group: (group, count, floor) for prefix, rows in [
    ('mhgp11_num_unit_', [('region_domain', 29, 28), ('region_pair', 28, 28), ('region_line', 154, 130)]),
    ('mhgp11_catalogue_region_', [('pair_region', 13, 13), ('line_region', 15, 13), ('obtuse_region', 7, 6)]),
    ('mhgp11_tower_domain_', [('context', 18, 18), ('lookup', 216, 200), ('global_support', 17, 17),
       ('ownership', 17, 16), ('refusals', 18, 18), ('capacity', 21, 20), ('concurrency', 9, 9),
       ('permutation', 131, 100)]), ('mhgp11_tower_domain_fault_', [('starvation', 34, 20)])]
    for group, count, floor in rows}
INVENTORY = {'mhgp11_catalogue_region_inventaire': 3, 'mhgp11_tower_domain_inventaire': 8,
             'mhgp11_tower_domain_fault_inventaire': 1}
FRACTION = 'mhgp11_num_center_region_fraction'
MODEL = 'mhgp11_num_center_region_model'
ORACLE = dict(cases=371, checks=865, contacts=55, degenerate=18, disjoint=163, intersects=137,
              pairs=106, permutations=123, refused=53, triangles=212)
REQUIRED = set(UNIT) | set(INVENTORY) | {n+s for n in (FRACTION, MODEL) for s in ('', '_opt')}
ERRORS = (old.foundation.Refusal, meb.previous.old.foundation.Refusal, asan.old.foundation.Refusal,
          OSError, ValueError, KeyError, TypeError, IndexError, old.foundation.ET.ParseError, tarfile.TarError)


def contract():
    raw = (HERE / 'contract.json').read_bytes()
    need(sha(raw) == CONTRACT_SHA, 'contrat_epingle')
    value = js(raw)
    need(value['source_commit'] == SOURCE, 'contrat_source')
    return value


def selection(config, data, pins, original_prefix=BASE):
    if 'tests' not in config:
        return
    path = BASE + config['name'] + '/tests.json'
    names = sorted(t['name'] for t in js(data[path]))
    key = original_prefix + config['name'] + '/tests.json'
    need(sha(json.dumps(names, separators=(',', ':')).encode()) == pins['selection_sha256'][key],
         'inventaire_source_epingle')
    if config['status'] == 'ok':
        need(config['tests']['selected'] == EXPECTED[config['name']], 'nombre_portes_source')


def new_gates(config, data, bits, supplement=False):
    if 'tests' not in config:
        need(config['status'] != 'ok', 'portes_absentes')
        return 0
    cases = {c.get('name'): c for c in old.foundation.ET.fromstring(
        data[BASE + config['name'] + '/junit.xml']).iter('testcase')}
    required = {n for n in REQUIRED if not supplement or not n.startswith('mhgp11_catalogue_')}
    if config['status'] == 'ok':
        need(required <= set(cases), 'nouvelles_portes_absentes')
    passed, outputs = 0, {}
    for name in sorted(required & set(cases)):
        case = cases[name]
        if case.get('status') != 'run' or case.find('failure') is not None or case.find('skipped') is not None:
            continue
        lines = (case.findtext('system-out') or '').splitlines()
        need('run_expect_verdict conforme' in lines, 'verdict_porte')
        if name in UNIT:
            group, count, floor = UNIT[name]
            need('test %s controles=%d echecs=0 plancher=%d' % (group, count, floor) in lines and
                 'mhgp11_test_ok tests=1 controles=%d' % count in lines, 'controle_natif_' + name)
        elif name in INVENTORY:
            need('inventaire_ok tests=%d' % INVENTORY[name] in lines, 'inventaire_porte')
        else:
            values = [js(line) for line in lines if line.startswith('{')]
            need(len(values) == 1, 'oracle_unique')
            value = outputs[name] = values[0]
            if name.removesuffix('_opt') == FRACTION:
                expected = dict(ORACLE, bits=bits)
                need(all(type(value[k]) is int and value[k] == v for k, v in expected.items()) and
                     old.HEX.fullmatch(value['input_sha256']), 'oracle_fraction')
            else:
                need(value['native_calls'] == 0 and value['profiles'] == [dict(ORACLE, bits=b, corruptions=26,
                     fixed_facts=39) for b in (18, 21, 24)], 'modele_independant')
        passed += 1
    for name in (FRACTION, MODEL):
        if name in outputs and name + '_opt' in outputs:
            need(outputs[name] == outputs[name + '_opt'], 'oracle_normal_opt')
    if config['status'] == 'ok':
        need(passed == len(required), 'nouvelles_portes_non_passees')
    return passed


def mutants(config, data, pins):
    if 'tests' not in config:
        need(config['status'] != 'ok', 'mutants_absents')
        return
    prefix = BASE + 'mutants/'
    cases = {c.get('name'): c for c in old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase')}
    for module, identities in pins['mutants'].items():
        name = 'mhgp11_mutants_' + module
        need(name in cases, 'module_mutants_absent')
        case = cases[name]
        if case.get('status') != 'run' or case.find('failure') is not None or case.find('skipped') is not None:
            need(config['status'] != 'ok', 'mutants_non_passes')
            continue
        lines = meb.previous.full_test_output(data, prefix, name, case.findtext('system-out') or '')
        n, construction = len(identities), sum(v == 'construction' for v in identities.values())
        expected = 'mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=%d plancher=%d'
        need(expected % (module, n, n, construction, n) in lines, 'mutants_non_causaux')
        for identity, kind in identities.items():
            verdict = 'construction' if kind == 'construction' else '(code|ligne)'
            need(sum(bool(re.fullmatch(re.escape(identity) + r'\s+TUE\s+' + verdict, s)) for s in lines) == 1,
                 'mort_mutant_' + identity)


def matrices(data, pins):
    counts, builds, code = q4.judge_matrix(data)
    for config in js(data[BASE+'summary.json'])['configurations']:
        name = config['name']
        selection(config, data, pins)
        if name == 'mutants':
            mutants(config, data, pins)
        elif name != 'style' and config['status'] != 'absent':
            bits = profiles.CONFIG_BITS[name]
            meb.previous.gates(config, data, bits)
            meb.gates(config, data, bits)
            new_gates(config, data, bits)
            if config['status'] == 'ok':
                need(NEW_TARGETS | {'mhgp11_catalogue_region'} <= set(builds[name]), 'nouveaux_binaires_absents')
                cache = builds[name]['CMakeCache.txt']['text'].splitlines()
                for key, active in [('MHGP11_SANITIZE', name == 'gcc_asan_ubsan'),
                                    ('MHGP11_TSAN', name == 'gcc_tsan'), ('MHGP11_POISON', name == 'poison')]:
                    need([s.split('=', 1)[1] for s in cache if s.startswith(key+':')] ==
                         ['ON' if active else 'OFF'], 'profil_instrumentation')
    return counts, builds, code


def supplement(data, pins):
    summary = js(data[BASE+'summary.json'])
    configs = summary['configurations']
    need(summary['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
         summary['requested'] == [asan.NAME] and len(configs) == 1 and configs[0]['name'] == asan.NAME and
         summary['statuses'] == {asan.NAME: configs[0]['status']}, 'inventaire_supplement')
    need({p[len(BASE):].split('/')[0] for p in data if p.startswith(BASE) and '/' in p[len(BASE):]} == {asan.NAME},
         'dossiers_supplement')
    config = configs[0]
    counts = old.foundation.judge_config(config, data)
    selection(config, data, pins, q4.SUPPLEMENT)
    asan.provenance(config, data)
    meb.previous.gates(config, data, 18)
    meb.gates(config, data, 18)
    new_gates(config, data, 18, True)
    code = 0 if config['status'] == 'ok' and not summary.get('signals') else 1
    need(type(summary['exit_code']) is int and summary['exit_code'] == code and
         summary['conforming'] is (code == 0), 'verdict_supplement')
    return counts, code


def report_judge(report, *args):
    for row in report['runs']:
        if row['status'] != 'ok':
            continue
        need(row['stderr'] == '', 'stderr_succes')
        values = row['events'][1]['logical']
        need(set(values) == profiles.LOGICAL and all(type(v) is int and 0 <= v < 2**64 for v in values.values()),
             'travail_region_entier')
        need(values['region_pair_rejects'] <= values['region_pair_tests'] <= 3*values['prefixes'] and
             values['region_line_rejects'] <= values['region_line_tests'] <= 3*values['prefixes'] and
             values['region_pair_rejects'] + values['region_line_rejects'] <= values['prefixes'], 'rejets_region')
    verdict = q4.judge_report(report, *args)
    # A checkpoint may precede insertion of the last failed K5's omission, never an older omission.
    required = {profiles.unit(dict(r, kmax=10)) for r in report['runs'][:-1]
                if r['kmax'] == 5 and r['status'] not in ('ok', 'pending_semantic')}
    need(required <= {profiles.unit(r) for r in report['not_run']}, 'omission_ancienne_perdue')
    return verdict


def command_metadata(meta, deadline):
    need(meta['requested_timeout_seconds'] == str(deadline) and meta['group_closed'] == '1' and
         meta['streams_truncated'] == '0' and meta['residual_group_killed'] in ('0', '1'), 'plan_fermeture')
    effective, elapsed = int(meta['effective_timeout_seconds']), float(meta['wall_seconds'])
    need(0 < effective <= deadline and profiles.finite(elapsed), 'duree_commande')
    need(meta['status'] in ('ok', 'failed', 'timeout', 'interrupted'), 'statut_commande')
    if meta['status'] == 'timeout':
        need(int(meta['exit_code']) in (124, 137) and elapsed >= effective, 'delai_commande')


def check(folder):
    pins = contract()
    receipt, worker, data = old.read_capture(folder)
    need(folder.name == 'region1' and receipt['commit'] == SOURCE, 'source_region1')
    need(all(receipt[k] is True for k in ('private_key_deleted', 'oslogin_key_removed', 'reserve_released')),
         'nettoyage_session')
    need((Path(receipt['raw_receipt_local']).parent/'DONE').read_text().strip() ==
         ('0' if receipt['status'] == 'completed' else '3'), 'cloture_controleur_DONE')
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')} == set(q4.COMMANDS), 'commandes')
    metas = [fields(data['results/cmd/'+n+'/meta.txt']) for n in q4.COMMANDS]
    for meta, deadline in zip(metas, (600, 180, 820)):
        command_metadata(meta, deadline)
    manifest, manifest_hash = old.inputs(folder, receipt)
    for prefix, filename in ((BASE, 'matrix.json'), (q4.SUPPLEMENT, 'asan18.json'),
                              (q4.BENCH+'files/', 'profiles.json')):
        source = prefix + ('profiles.json' if filename == 'profiles.json' else 'summary.json')
        need((folder/filename).read_bytes() == data[source], 'copie_'+filename)
    counts, builds, code = matrices(data, pins)
    mapped = {BASE+p[len(q4.SUPPLEMENT):]: v for p, v in data.items() if p.startswith(q4.SUPPLEMENT)}
    extra, extra_code = supplement(mapped, pins)
    q4.command(metas[0], code)
    q4.command(metas[1], extra_code)
    need(code == extra_code == 0, 'banc_sans_qualification')
    report = js((folder/'profiles.json').read_bytes())
    hashes = {name: sha(data[BASE+name+'/build_provenance.json']) for name in profiles.PROFILES.values()}
    verdict = report_judge(report, manifest, manifest_hash, sha(data[BASE+'summary.json']), builds, hashes,
                           sha(data[q4.SUPPLEMENT+'summary.json']))
    q4.command(metas[2], (0 if verdict['conforming'] else 1) if report['complete'] else None)
    good = q4.session(receipt, worker, metas)
    need(good is verdict['conforming'] and receipt['preserved_failure'] is (not good), 'bilan_session')
    states = {s: sum(r['status'] == s for r in report['runs']) for s in sorted({r['status'] for r in report['runs']})}
    print('%s coherence=ok campagne=%s source=%s' % (folder.name, 'CONFORME' if good else 'ECHEC', SOURCE))
    print('  matrice=%d/%d ASan18=%d/%d mutants=154 profils=18/21/24' %
          (sum(v[1] for v in counts.values()), sum(v[0] for v in counts.values()), extra[1], extra[0]))
    print('  tentatives=%d statuts=%s omissions_causales=%d sans_resultat_persistant=%d comparaisons_egales=%d' %
          (len(report['runs']), json.dumps(states, sort_keys=True), len(report['not_run']),
           36-len(report['runs'])-len(report['not_run']), verdict['equal']))
    return not good


if __name__ == '__main__':
    try:
        folders = [Path(p) for p in sys.argv[1:]] or [HERE/'region1']
        failed = sum(check(folder) for folder in folders)
        print('captures_coherentes=%d campagnes_echouees=%d' % (len(folders), failed))
    except ERRORS as error:
        print('REFUS '+(str(error) if isinstance(error, old.foundation.Refusal) else type(error).__name__))
        sys.exit(1)
