#!/usr/bin/env python3
"""Relecture de B3b : sources Git, substitutions en mémoire et modèle Python public ; aucun moteur."""
import ast
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
V12 = 'morsehgp3D_v12/'


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def run(repo):
    cap = json.loads((HERE / 'capture.json').read_text())
    pins = cap['pins']

    def command(*args):
        return subprocess.check_output(['git', *args], cwd=repo)

    def read(pin, path):
        return command('show', pins.get(pin, pin) + ':' + V12 + path)

    def files(pin, area):
        return command('ls-tree', '-r', '--name-only', pins[pin], '--', V12 + area).decode().splitlines()

    def changed(a, b, area):
        return command('diff', '--name-only', pins[a], pins[b], '--', V12 + area).decode().splitlines()

    previous = changed('r1', 'b3', 'src/')
    current = changed('a6c', 'b3b', 'src/')
    need(current == previous and len(current) == 9, 'same native patch paths')
    need(files('a6c', 'src/') == files('b3b', 'src/'), 'native source inventory')
    for path in files('b3b', 'src/'):
        rel = path[len(V12):]
        expected = 'b3' if path in current else 'a6c'
        need(read('b3b', rel) == read(expected, rel), 'composition ' + rel)
    arms_path = 'microbancs/mes_t2d_b3/bras_t2d_b3.json'
    arms = json.loads(read('b3b', arms_path))
    need(arms['base'] == pins['a6c'][:9], 'base')
    need(arms['bras'] == json.loads(read('b3', arms_path))['bras'], 'same arms')
    rewritten, count, arm_files = {}, 0, 0
    for name, arm in arms['bras'].items():
        rewritten[name] = {}
        for path, item in arm['fichiers'].items():
            before = read('a6c', path)
            need(sha(before) == item['sha256_avant'], 'preimage ' + path)
            text = before.decode()
            for sub in item['substitutions']:
                need(text.count(sub['cherche']) == 1, 'unique substitution')
                text = text.replace(sub['cherche'], sub['remplace'])
                count += 1
            need(sha(text.encode()) == item['sha256_apres'], 'postimage ' + path)
            if name == 'apres':
                need(text.encode() == read('b3b', path), 'product arm ' + path)
            rewritten[name][path] = text
            arm_files += 1
    transfer = rewritten['transfert']
    need(set(rewritten['cles']) - set(transfer) == {'src/catalogue/table.cpp'}, 'lookup ablation')
    need(all(text == rewritten['cles'][p] == rewritten['apres'][p] for p, text in transfer.items()),
         'retention common to transfer and keys arms')
    complete = read('b3b', 'src/catalogue/finish_driver.hpp').decode()
    sliced = read('b3b', 'src/catalogue/finish_slices.hpp').decode()
    need('take_segment(b, a.table.keys[ct], out.table_keys, in.balls' in complete, 'complete D2H keys')
    body = sliced.split('Outcome slice_take(', 1)[1].split('// Une tranche', 1)[0]
    need('table_keys' not in body and 'segments[4]' in body, 'sliced key D2H absent')
    need('host_table(out.balls.span(), in.sites, out.table_offsets, out.table_values, out.table_keys' in sliced,
         'sliced host construction')
    pilot_path = 'microbancs/mes_t2d_b3/pilote_t2d_b3.py'
    def functions(pin):
        return {n.name: ast.dump(n, include_attributes=False) for n in ast.parse(read(pin, pilot_path)).body
                if isinstance(n, ast.FunctionDef)}
    need(functions('b3') == functions('b3b'), 'all pilot functions unchanged')
    gate_paths = ['src/tower/pipeline.cpp', 'tests/tower/pipeline_unit.cpp', 'tests/tower/pipeline_fault.cpp']
    need(all(read('a6c', p) == read('b3b', p) for p in gate_paths), 'CST-0244/0245 affected files unchanged')
    manifests = {}
    for name, expected in [('tower', 74), ('catalogue', 38)]:
        manifest = json.loads(read('b3b', 'tests/mutants/' + name + '.json'))
        need(manifest['plancher'] == expected and len(manifest['mutants']) == expected, 'mutant manifest ' + name)
        manifests[name] = expected
    old_tower = {m['id']: m for m in json.loads(read('a6c', 'tests/mutants/tower.json'))['mutants']}
    tower = {m['id']: m for m in json.loads(read('b3b', 'tests/mutants/tower.json'))['mutants']}
    need(set(tower) - set(old_tower) == {'cles_garde_hors_nuage', 'balayage_dernier_oublie'}, 'two B3 gates')
    need([name for name in old_tower if old_tower[name] != tower[name]] == ['table_s_etoile_queue_partielle'],
         'only old tower mutant re-anchor')
    # Reuse the immutable seven-case public synthetic witness, with the new pilot/dependencies pinned in Git.
    witness_root = 'receipts/audit_reponses_20261008/b3_identite_admission/'
    witness = read('b3b', witness_root + 'check.py')
    need(sha(witness) == cap['synthetic_witness_sha256'], 'public witness pin')
    deps = json.loads(read('b3b', witness_root + 'capture.json'))['sources']
    with tempfile.TemporaryDirectory(prefix='audit-b3b-public-') as td:
        temp = Path(td)
        sources = {}
        for path in deps:
            data = read('b3b', path)
            target = temp / 'source' / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            sources[path] = dict(bytes=len(data), sha256=sha(data))
        need(sources == cap['synthetic_source_pins'], 'synthetic source pins')
        (temp / 'capture.json').write_text(json.dumps(dict(sources=sources)))
        target = temp / 'witness.py'
        target.write_bytes(witness)
        spec = importlib.util.spec_from_file_location('b3b_public_witness', target)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        result = module.run(temp)
        need(result == json.loads(read('b3b', witness_root + 'results.json')), 'same seven witness outcomes')
    return dict(schema='ehgp.audit.b3b_prelecture.v1', native_runs=0, native_source_files=len(files('b3b', 'src/')),
                b3_native_files=9, arm_files=arm_files, exact_substitutions=count,
                unchanged_pilot_functions=len(functions('b3b')), manifests=manifests,
                active_chain_memory_and_failure_gates_fixed=False, synthetic_cases=result['cases'],
                full_identity_logs_not_replayed=25, resolution_logs_not_replayed=10,
                key_storage_bytes_per_ball=16, complete_gpu_key_D2H_bytes_per_ball=16,
                sliced_gpu_key_D2H_bytes_per_ball=0, source_composition_exact=True,
                campaign_timing_qualified=False)


if __name__ == '__main__':
    need(len(sys.argv) == 2, 'usage: check.py DEPOT_GIT')
    result = run(Path(sys.argv[1]))
    expected = HERE / 'results.json'
    if expected.exists():
        need(result == json.loads(expected.read_text()), 'saved results changed')
    print(json.dumps(result, ensure_ascii=False, indent=2))
