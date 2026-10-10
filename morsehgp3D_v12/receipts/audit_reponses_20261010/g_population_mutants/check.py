#!/usr/bin/env python3
"""Compagnon des deux ancres : contrôle Python du manifeste et modèle causal ; aucun test natif."""
from pathlib import Path
import hashlib
import importlib.util
import json
import subprocess
import sys
import tempfile
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
PREFIX = 'morsehgp3D_v12/'
IDS = ('recherche_sans_egalite_des_sites', 'recherche_premiere_place_du_seau')


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def apply(root, patch):
    subprocess.run(['git', 'apply', '--check', '-'], input=patch, cwd=root, check=True, capture_output=True)
    subprocess.run(['git', 'apply', '-'], input=patch, cwd=root, check=True, capture_output=True)


def official_check(runner, source, manifest_path, expected):
    checked = subprocess.run([sys.executable, '-B', *(['-O'] if sys.flags.optimize else []), str(runner),
                              '--manifest', str(manifest_path), '--source', str(source), '--check'],
                             check=True, capture_output=True, text=True)
    need(checked.stdout.strip() == expected and not checked.stderr, 'official check-only verdict')
    return expected


def composition(repo, cap, product_patch, companion, module):
    spec = cap['composition']
    t1 = subprocess.check_output(['git', 'show', spec['t1_receipt_git'] + ':' + spec['t1_patch_path']], cwd=repo)
    need(sha(t1) == spec['t1_patch_sha256'], 'immutable T1 patch')
    with tempfile.TemporaryDirectory(prefix='g-population-t1-composition-') as td:
        root = Path(td)
        source = root / PREFIX
        # The existing pin list must also match at this later integration base.
        # Catalogue-specific files extend it; neither list contains runtime data.
        pins = dict(cap['sources'])
        need(not (pins.keys() & spec['additional_sources'].keys()), 'nonduplicated pins')
        pins.update(spec['additional_sources'])
        for path, digest in pins.items():
            data = subprocess.check_output(['git', 'show', spec['source_git'] + ':' + PREFIX + path], cwd=repo)
            need(sha(data) == digest, 'composition source ' + path)
            target = source / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        apply(root, product_patch)
        apply(root, companion)
        apply(root, t1)
        need(sha((source / 'src/tower/populations.cpp').read_bytes()) == spec['population_postimage_sha256'],
             'population postimage')
        need(sha((source / 'src/tower/resolve.cpp').read_bytes()) == spec['resolve_postimage_sha256'],
             'T1 postimage')
        need(sha((source / 'tests/mutants/tower.json').read_bytes()) == cap['proposed_manifest_sha256'],
             'composed tower manifest')
        outcomes = {}
        runner = source / 'tests/mutants/run_mutants.py'
        for name, floor, count, edits in (('tower', 73, 73, 77), ('catalogue', 38, 38, 38)):
            manifest_path = source / ('tests/mutants/' + name + '.json')
            manifest = module.load_manifest(manifest_path)
            need(manifest['plancher'] == floor and len(manifest['mutants']) == count, 'composition floor')
            # Official reader enforces exactly one occurrence at EVERY sequential edit,
            # including an `aussi` anchor exposed/changed by its preceding edit.
            module.check_patterns(source, manifest['mutants'])
            checked_edits = sum(1 + len(m['aussi']) for m in manifest['mutants'])
            need(checked_edits == edits, 'composition edit count')
            verdict = official_check(runner, source, manifest_path,
                                     f'manifeste_ok module={name} mutants={count} plancher={floor}')
            outcomes[name] = dict(mutants=count, floor=floor, edits=edits,
                                  secondary_edits=edits-count, every_sequential_anchor_once=True,
                                  official_check=verdict)
    return dict(source_git=spec['source_git'], applied_in_temporary_copy=True,
                source_files_pinned=len(pins), postimages_verified=True,
                patch_order=['g_population_egalite', 'g_population_mutants', 't1_support_population'],
                manifests=outcomes, native_qualification=False)


def causal_model():
    # Triangle witness's three pair populations; the hash is deliberately masked to zero.
    rows = [(0, (0, 1), 0, 2), (0, (0, 2), 1, 0), (0, (1, 2), 2, 1)]

    def find(query, mode):
        lo, hi, steps = 0, len(rows), 0
        if mode == 'first_only':
            return (rows[lo][2:] if lo < hi and rows[lo][1] == query else None), int(lo < hi)
        while lo < hi:
            mid = lo + (hi - lo) // 2
            steps += 1
            compare = (rows[mid][1] > query) - (rows[mid][1] < query)
            if (rows[mid][0] == 0 if mode == 'hash_only' else compare == 0):
                return rows[mid][2:], steps
            if compare < 0:
                lo = mid + 1
            else:
                hi = mid
        return None, steps

    results, wrong = [], dict(hash_only=0, first_only=0)
    for query in ((0, 1), (0, 2), (1, 2), (0, 3), (2, 3)):
        expected = next((r[2:] for r in rows if r[1] == query), None)
        correct, steps = find(query, 'exact')
        need(correct == expected and steps <= 2, 'model witness')
        result = dict(query=list(query), expected=expected, exact=correct)
        for mode in wrong:
            got, count = find(query, mode)
            need(count <= 2, 'bounded termination')
            wrong[mode] += got != expected
            result[mode] = got
        results.append(result)
    need(wrong == dict(hash_only=4, first_only=2), 'causal wrong responses')
    return dict(mask=0, rows=3, queries=5, wrong_responses=wrong,
                first_only_no_loop=True, examples=results)


def run(repo):
    cap = json.loads((HERE / 'capture.json').read_text())
    sources = {}
    for path, h in cap['sources'].items():
        data = subprocess.check_output(['git', 'show', cap['source_git'] + ':' + PREFIX + path], cwd=repo)
        need(sha(data) == h, 'source ' + path)
        sources[path] = data
    product_patch = subprocess.check_output(['git', 'show', cap['parent_receipt_git'] + ':' + cap['parent_patch_path']], cwd=repo)
    need(sha(product_patch) == cap['parent_patch_sha256'], 'immutable parent patch')
    companion = (HERE / 'proposition.patch').read_bytes()
    need(sha(companion) == cap['companion_patch_sha256'], 'companion patch')
    with tempfile.TemporaryDirectory(prefix='g-population-mutant-anchors-') as td:
        root = Path(td)
        source = root / PREFIX
        for path, data in sources.items():
            target = source / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        runner = source / 'tests/mutants/run_mutants.py'
        spec = importlib.util.spec_from_file_location('population_mutant_reader', runner)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        manifest_path = source / 'tests/mutants/tower.json'
        original = json.loads(manifest_path.read_text())
        original_schema = module.load_manifest(manifest_path)
        module.check_patterns(source, original_schema['mutants'])
        apply(root, product_patch)
        broken = []
        for mutant in original_schema['mutants']:
            try:
                module.mutated_files(source, mutant)
            except module.Refusal as error:
                need(error.code == 3, 'anchor failure class')
                broken.append(mutant['id'])
        need(broken == list(IDS), 'exactly two invalidated anchors')
        apply(root, companion)
        data = manifest_path.read_bytes()
        need(sha(data) == cap['proposed_manifest_sha256'], 'proposed manifest')
        candidate = json.loads(data)
        need(candidate['plancher'] == original['plancher'] == 73 and len(candidate['mutants']) == 73, 'same floor')
        need(candidate.keys() == original.keys(), 'same manifest schema')
        need({k: v for k, v in candidate.items() if k != 'mutants'} ==
             {k: v for k, v in original.items() if k != 'mutants'}, 'same manifest metadata')
        changed = []
        for before, after in zip(original['mutants'], candidate['mutants']):
            need(before['id'] == after['id'] and before.get('porte') == after.get('porte'), 'same IDs and gates')
            if before != after:
                changed.append(after['id'])
                need({k for k in before if before[k] != after[k]} <= {'cherche', 'remplace', 'note'}, 'anchor-only edit')
        need(changed == list(IDS), 'only two manifest entries changed')
        judged = module.load_manifest(manifest_path)
        module.check_patterns(source, judged['mutants'])  # includes sequential `aussi` substitutions
        modifications = sum(1 + len(m.get('aussi', [])) for m in judged['mutants'])
        for mutant in judged['mutants']:
            if mutant['id'] not in IDS:
                continue
            mutated = module.mutated_files(source, mutant)['src/tower/populations.cpp']
            body = mutated.split('std::optional<PopulationHit> PopulationTable::find(', 1)[1].split('u32 PopulationTable::candidate(', 1)[0]
            if mutant['id'] == IDS[0]:
                need('if (key_at(mid) == key) return hit_at(mid);' in body, 'hash-only mutant')
            else:
                need('while (' not in body and 'if (lo < hi && compare_at(lo, key, f) == 0) return hit_at(lo);' in body,
                     'first-only mutant terminates without a loop')
        official_check(runner, source, manifest_path, 'manifeste_ok module=tower mutants=73 plancher=73')
        composed = composition(repo, cap, product_patch, companion, module)
    return json.loads(json.dumps(dict(native_runs=0, source_git=cap['source_git'], parent_patch_unchanged=True,
                invalidated_before_companion=broken, mutants=73, floor=73, edits_checked=modifications,
                secondary_edits_checked=modifications-73, same_ids_and_gates=True,
                unchanged_manifest_entries=71, official_check='manifeste_ok module=tower mutants=73 plancher=73',
                causal_model=causal_model(), composition=composed, native_mutants_executed=False)))


if __name__ == '__main__':
    need(len(sys.argv) == 2, 'usage: check.py DEPOT_GIT')
    result = run(Path(sys.argv[1]))
    need(result == json.loads((HERE / 'results.json').read_text()), 'stored result')
    print(json.dumps(result, ensure_ascii=False, indent=2))
