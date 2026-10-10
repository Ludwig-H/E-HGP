#!/usr/bin/env python3
"""Proposition CST-0244 : application, motifs des mutants, modele de borne ; aucun natif."""
from pathlib import Path
import argparse
import hashlib
import itertools
import json
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
PREFIX = 'morsehgp3D_v12/'
BASE = 'aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66'
B3 = '81b0883d1'
FILES = ['src/tower/pipeline.cpp', 'src/tower/pipeline.hpp', 'tests/tower/pipeline_levers.cpp']


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args])


def model():
    checks = 0
    allocations = 0
    # One unit is sizeof(Sphere)+sizeof(u32); all unchanged terms are an arbitrary base.
    def supplement(widths, chain, threads):
        return threads * max([0] + [w for i, w in enumerate(widths) if i >= 1 and chain[i]])

    for orders in range(1, 6):
        for widths in itertools.product((1, 2, 7), repeat=orders):
            for chain in itertools.product((False, True), repeat=orders):
                for threads in (1, 3, 48):
                    got = supplement(widths, chain, threads)
                    expected = threads * max((widths[i] for i in range(1, orders) if chain[i]), default=0)
                    need(got == expected, 'selected-order bound')
                    need(got <= (threads * max(widths[1:]) if orders >= 2 else 0), 'no increased bound')
                    if not any(chain[1:]):
                        need(got == 0, 'no chunk buffer when no eligible order engaged')
                    if all(chain) and orders >= 2:
                        need(got == threads * max(widths[1:]), 'ON unchanged')
                    checks += 1
                if orders <= 3:
                    # Every concurrent worker either has no scratch or a cohort of one eligible order.
                    eligible = [0] + [widths[i] for i in range(1, orders) if chain[i]]
                    for live in itertools.product(eligible, repeat=3):
                        need(sum(live) <= supplement(widths, chain, 3), 'three-worker peak bounded')
                        allocations += 1
    # Guarded admission: changing only the numeric threshold is permitted if the effective choice is unchanged.
    def admit(sites, opened_threshold, later_threshold, orders):
        chain = [sites >= opened_threshold] * orders
        if any(x != (sites >= later_threshold) for x in chain):
            return 'tower_invariant', 0
        return 'ok', 1

    transitions = []
    for sites in (0, 1, 1000, 43899, 43900, 43901, (1 << 32) - 1):
        for before in (0, 1, 43900, 43901, (1 << 64) - 1):
            for after in (0, 1, 43900, 43901, (1 << 64) - 1):
                for orders in (1, 5, 12):
                    got = admit(sites, before, after, orders)
                    change = (sites >= before) != (sites >= after)
                    need(got == (('tower_invariant', 0) if change else ('ok', 1)), 'effective-choice guard')
                    transitions.append((change, got))
    witnesses = {
        'off_unconditional_reserve': (supplement((100, 9), (False, False), 48), 48 * 9),
        'order_one_included': (supplement((100, 9), (True, False), 48), 48 * 100),
        'inactive_widest_included': (supplement((1, 9, 3), (True, False, True), 48), 48 * 9),
        'one_worker_only': (supplement((1, 9), (True, True), 48), 9),
    }
    need(all(a != b for a, b in witnesses.values()), 'model mutation witnesses')
    need(admit(1000, 43900, 0, 5) == ('tower_invariant', 0) and
         admit(1000, 0, 43900, 5) == ('tower_invariant', 0), 'late flip both directions')
    return dict(bound_cases=checks, concurrent_allocations=allocations,
                transition_cases=len(transitions), changed_refused=sum(c for c, _ in transitions),
                unchanged_admitted=sum(not c for c, _ in transitions),
                numerical_counterexamples=witnesses, native_qualified=False)


def check_sources(repo, commit, patch):
    full_commit = git(repo, 'rev-parse', commit).decode().strip()
    manifest_name = PREFIX + 'tests/mutants/tower.json'
    manifest = json.loads(git(repo, 'show', full_commit + ':' + manifest_name))
    names = set(FILES)
    for mutant in manifest['mutants']:
        for change in [mutant] + mutant.get('aussi', []):
            names.add(change['fichier'])
    original = {n: git(repo, 'show', full_commit + ':' + PREFIX + n) for n in names}
    # The accounting of the sequential scratch and both width scans stay byte-identical.
    supporting = {n: git(repo, 'show', full_commit + ':' + PREFIX + n) for n in
                  ['src/tower/forest_build.cpp', 'src/tower/forest_births.cpp']}
    with tempfile.TemporaryDirectory(prefix='audit-a6c-patch-') as td:
        root = Path(td)
        for n, blob in original.items():
            target = root / PREFIX / n
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(blob)
        for args in [('apply', '--check'), ('apply',)]:
            subprocess.run(['git', *args, str(patch)], cwd=root, check=True, capture_output=True)
        updated = {n: (root / PREFIX / n).read_bytes() for n in names}
    need({n for n in names if updated[n] != original[n]} == set(FILES), 'exactly three files changed')
    cpp = updated[FILES[0]].decode()
    opening = cpp.split('Outcome open_session(', 1)[1].split('Outcome admit_session(', 1)[0]
    admission = cpp.split('Outcome admit_session(', 1)[1].split('Outcome play_session(', 1)[0]
    assignment = '    p.chain[i] = chain_engaged(r.index.cloud().sites(), r.chain_sites);'
    need(cpp.count(assignment) == 1 and opening.index(assignment) < opening.index('region_bytes('),
         'choice frozen before bound')
    guard = 'if (p.chain[i] != engaged) return fail(Reason::tower_invariant);'
    need(admission.index('const Stopwatch watch;') < admission.index(guard), 'guard duration inside open_ns')
    need(admission.index(guard) < admission.index('r.budget.admit(') < admission.index('staff('),
         'guard before admission and allocations')
    need('p.chain[i] =' not in admission, 'no chain redecision after admission')
    new_region = cpp.split('u64 region_bytes(', 1)[1].split('template <class T>', 1)[0]
    old_region = original[FILES[0]].decode().split('u64 region_bytes(', 1)[1].split('template <class T>', 1)[0]
    tail = '  for (u32 i = 0; i < p.orders; ++i) {'
    need(new_region.split(tail, 1)[1] == old_region.split(tail, 1)[1], 'other bound terms unchanged')
    need('  u64 widest = 0;' in new_region and 'if (p.chain[i]) widest' in new_region and
         'for (u32 i = 1; i < p.orders; ++i)' in new_region, 'eligible orders only')
    old_sizes = original[FILES[0]].decode().split('struct KernelBytes {', 1)[1].split('\n};', 1)[0]
    new_sizes = cpp.split('struct KernelBytes {', 1)[1].split('\n};', 1)[0]
    need(old_sizes == new_sizes, 'double scan intentionally unchanged')
    play = updated[FILES[2]].decode().split('u64 chain_sites) {', 1)[1].split('\n}', 1)[0]
    need(play.index('run->chain_sites = chain_sites;') < play.index('open_session(*run)'), 'helper threshold before open')
    checks = 0
    for mutant in manifest['mutants']:
        # Each mutant starts from an independent postimage. Multi-file substitutions retain their order.
        changed = dict(updated)
        for change in [mutant] + mutant.get('aussi', []):
            n, old, new = change['fichier'], change['cherche'].encode(), change['remplace'].encode()
            need(changed[n].count(old) == 1, 'mutant unique anchor ' + mutant['id'])
            changed[n] = changed[n].replace(old, new)
            checks += 1
    return dict(commit=full_commit, floor=manifest['plancher'], mutants=len(manifest['mutants']),
                substitutions=checks, all_unique=True,
                before_sha256={n: sha(original[n]) for n in FILES},
                after_sha256={n: sha(updated[n]) for n in FILES},
                supporting_sha256={n: sha(b) for n, b in supporting.items()})


def run(repo):
    patch = HERE / 'proposition.patch'
    sources = {c: check_sources(repo, c, patch) for c in (BASE, B3)}
    need(sources[BASE]['after_sha256'] == sources[B3]['after_sha256'], 'same postimage on A6c and B3b')
    need(sources[BASE]['mutants'] == 72 and sources[B3]['mutants'] == 74, 'manifest cohorts')
    return dict(patch_sha256=sha(patch.read_bytes()), applications=sources, model=model(),
                native_calls=0, cloud_calls=0, cst0245_closed=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('repo', type=Path)
    parser.add_argument('--write', action='store_true')
    args = parser.parse_args()
    result = run(args.repo.resolve())
    target = HERE / 'results.json'
    encoded = json.loads(json.dumps(result))
    if args.write:
        target.write_text(json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True) + '\n')
    else:
        need(encoded == json.loads(target.read_text()), 'stored result')
    print(json.dumps(dict(applications=2, mutants=[v['mutants'] for v in result['applications'].values()],
                         **result['model'])))
