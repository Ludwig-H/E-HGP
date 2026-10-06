#!/usr/bin/env python3
"""Bounded replay of real hierarchy-IoU assignments and summary, stdlib only."""
import argparse
import ast
import copy
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace

CHECKS = 0


def need(ok, why):
    global CHECKS
    if not ok:
        raise RuntimeError(why)
    CHECKS += 1


class Vector:
    """Only the tolist adapter required by the real Evaluator and best_blocks."""
    def __init__(self, values):
        self.values = values

    def tolist(self):
        return list(self.values)


class OnePlateau:
    """One abstract block; all entries close together at its only plateau."""
    def __init__(self, count):
        self.block_parent = [-1]
        self.block_plateau = [0]
        self.block_merged = [False]
        self.site_plateau = Vector([0] * count)
        self.site_block = [0] * count
        self.levels = [0]

    def blocks(self):
        return 1


def compiled_parts(study, hierarchy):
    """Compile original AST nodes unchanged; never execute imports or one()/main()."""
    ev = next(n for n in ast.parse(hierarchy).body if isinstance(n, ast.ClassDef) and n.name == 'Evaluator')
    namespace = {}
    exec(compile(ast.Module(body=[ev], type_ignores=[]), '<real Evaluator>', 'exec'), namespace)
    # Summarize uses numpy only for scalar means of small lists. The adapter has those same values here.
    namespace.update(ph=SimpleNamespace(Evaluator=namespace['Evaluator']),
                     np=SimpleNamespace(mean=lambda values: sum(values) / len(values)))
    tree = ast.parse(study)
    definitions = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name in ('best_blocks', 'summarize')]
    exec(compile(ast.Module(body=definitions, type_ignores=[]), '<real study functions>', 'exec'), namespace)
    one = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'one')
    wanted = {'level_b_tower', 'level_b_hdbscan'}
    assignments = [n for n in one.body if isinstance(n, ast.Assign) and
                   isinstance(n.targets[0], ast.Subscript) and isinstance(n.targets[0].slice, ast.Constant) and
                   n.targets[0].slice.value in wanted]
    need(len(assignments) == 2, 'two real hierarchy-IoU assignments')
    return namespace, compile(ast.Module(body=assignments, type_ignores=[]), '<real one assignments>', 'exec')


def evaluated_record(parts, intersection, union, name, found):
    namespace, assignments = parts
    # Object is wholly contained in a single closed block: |G|=intersection, |C|=union.
    labels = Vector([0] * intersection + [-1] * (union - intersection))
    void = Vector([False] * union)
    tree = OnePlateau(union)
    out = dict(name=name, k=5, objects=1,
               lines={'T_eom1_mcs10': {'rows': [dict(found=found, merged=False,
                                                   fragmented=False, noise=False, iou=0.75)]}})
    env = dict(namespace, out=out, pt=tree, apt=tree, nobj=labels, obj=labels, nvoid=void, void=void, objects=1)
    exec(assignments, env)
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', default='/workspaces/E-HGP')
    args = parser.parse_args()
    base = Path(__file__).parent
    source_manifest = json.loads((base / 'sources.json').read_text())
    originals = {}
    for path, properties in source_manifest['files'].items():
        data = subprocess.check_output(['git', '-C', args.repo, 'show', source_manifest['pin'] + ':' + path])
        need(hashlib.sha256(data).hexdigest() == properties['sha256'], 'source SHA256: ' + path)
        originals[path] = data.decode()
    study_path = 'morsehgp3D_v11/bench/points_flat_study.py'
    hierarchy_path = 'morsehgp3D_v11/bench/points_hierarchy.py'
    with tempfile.TemporaryDirectory(prefix='mhgp11_population_patch_') as directory:
        target = Path(directory) / study_path
        target.parent.mkdir(parents=True)
        target.write_text(originals[study_path])
        command = ['git', 'apply', '--check', str((base / 'proposed.patch').resolve())]
        result = subprocess.run(command, cwd=directory, capture_output=True, text=True)
        need(result.returncode == 0, 'patch applicability: ' + result.stderr)
        result = subprocess.run(['git', 'apply', str((base / 'proposed.patch').resolve())], cwd=directory,
                                capture_output=True, text=True)
        need(result.returncode == 0, 'patch application in isolated directory: ' + result.stderr)
        patched_study = target.read_text()
    original = compiled_parts(originals[study_path], originals[hierarchy_path])
    patched = compiled_parts(patched_study, originals[hierarchy_path])
    summarize = original[0]['summarize']
    need(ast.dump(next(n for n in ast.parse(originals[study_path]).body if isinstance(n, ast.FunctionDef) and
                       n.name == 'summarize'), include_attributes=False) ==
         ast.dump(next(n for n in ast.parse(patched_study).body if isinstance(n, ast.FunctionDef) and
                       n.name == 'summarize'), include_attributes=False), 'strict summary predicate unchanged')
    cases = [('just_above_half', 10001, 20001), ('equal_half', 10000, 20000),
             ('just_below_half', 10000, 20001), ('above_decimal_rounding_band', 10002, 20001)]
    results, old_records, new_records = [], {}, {}
    key = 'T_eom1_mcs10|k=5'
    for name, numerator, denominator in cases:
        exact = Fraction(numerator, denominator)
        before = evaluated_record(original, numerator, denominator, name, False)
        after = evaluated_record(patched, numerator, denominator, name, False)
        need(before['level_b_tower'] == [round(float(exact), 4)], 'actual rounded tower value')
        need(after['level_b_tower'] == [float(exact)], 'unrounded actual evaluator value')
        need(before['level_b_hdbscan'] == before['level_b_tower'], 'same original A assignment')
        need(after['level_b_hdbscan'] == after['level_b_tower'], 'same patched A assignment')
        old_table = summarize([before], [10], ['eom1'])
        new_table = summarize([after], [10], ['eom1'])
        included = exact > Fraction(1, 2)
        need((key in new_table) == included, 'strict >1/2 boundary after patch')
        need((key in old_table) == (round(float(exact), 4) > 0.5), 'observed rounded admission')
        # This is the actual JSON serialization path; no formatting back to four decimals.
        serialized = json.loads(json.dumps(after))
        need(summarize([serialized], [10], ['eom1']) == new_table, 'JSON roundtrip preserves admission')
        results.append(dict(name=name, exact_iou=str(exact), original_stored=before['level_b_tower'][0],
                            patched_stored=after['level_b_tower'][0], exact_eligible=included,
                            original_in_population=key in old_table, patched_in_population=key in new_table))
        old_records[name], new_records[name] = before, after
    baseline_old = copy.deepcopy(old_records['above_decimal_rounding_band'])
    baseline_new = copy.deepcopy(new_records['above_decimal_rounding_band'])
    baseline_old['lines']['T_eom1_mcs10']['rows'][0]['found'] = True
    baseline_new['lines']['T_eom1_mcs10']['rows'][0]['found'] = True
    old_population = summarize([baseline_old, old_records['just_above_half']], [10], ['eom1'])[key]
    new_population = summarize([baseline_new, new_records['just_above_half']], [10], ['eom1'])[key]
    need(old_population['scenes'] == 1 and new_population['scenes'] == 2, 'material population count difference')
    need(old_population['all_found'] == 1.0 and new_population['all_found'] == 0.5,
         'population omission can change the reported rule score')
    secondary_old = copy.deepcopy(old_records['just_above_half'])
    secondary_new = copy.deepcopy(new_records['just_above_half'])
    for record, records in [(secondary_old, old_records), (secondary_new, new_records)]:
        record['objects'] = 2
        for side in ('level_b_tower', 'level_b_hdbscan'):
            record[side] += records['just_below_half'][side]
        record['lines']['T_eom1_mcs10']['rows'] *= 2
    old_secondary = summarize([baseline_old, secondary_old], [10], ['eom1'])[key]
    new_secondary = summarize([baseline_new, secondary_new], [10], ['eom1'])[key]
    need(old_secondary['scenes'] == new_secondary['scenes'] == 1, 'mixed pair remains outside primary population')
    need(old_secondary['objects_found_by_hierarchy'] == 1 and new_secondary['objects_found_by_hierarchy'] == 2,
         'secondary object population changes independently')
    print(json.dumps(dict(status='PASS_portable_reproduction__rounding_changes_population', checks=CHECKS,
        source_pin=source_manifest['pin'], patch_apply_check='PASS', native_executed=False, cases=results,
        synthetic_primary=dict(original_scenes=old_population['scenes'], patched_scenes=new_population['scenes'],
                               original_all_found=old_population['all_found'], patched_all_found=new_population['all_found']),
        synthetic_secondary=dict(original_objects=old_secondary['objects_found_by_hierarchy'],
                                 patched_objects=new_secondary['objects_found_by_hierarchy']),
        real_data_impact='not measured or established',
        scope='Real Evaluator, best_blocks, hierarchy-IoU assignments and summarize AST; stdlib tolist/scalar mean adapters. Abstract one-plateau inputs and synthetic result records only. No complete one(), numpy/scipy, native, benchmarks or cloud.'),
        sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
