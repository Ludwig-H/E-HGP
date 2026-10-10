#!/usr/bin/env python3
"""Prélecture de sources A6c et modèles bornés ; aucune exécution native."""
import argparse
import hashlib
import importlib.util
import itertools
import json
import subprocess
from pathlib import Path


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def cohorts():
    checks = 0
    for n in range(1, 11):
        for boundaries in itertools.product((0, 1), repeat=n - 1):
            rank = [0]
            for b in boundaries:
                rank.append(rank[-1] + b)
            for grain in range(1, 7):
                counts = [0] * n
                for begin in range(0, n, grain):
                    end = min(n, begin + grain)
                    first = begin
                    while 0 < first < end and rank[first] == rank[first - 1]:
                        first += 1
                    lo = first
                    while lo < end:
                        hi = lo + 1
                        while hi < n and rank[hi] == rank[lo]:
                            hi += 1
                        for v in range(lo, hi):
                            counts[v] += 1
                        lo = hi
                need(counts == [1] * n, 'cohorte omise ou écrite deux fois')
                checks += 1
    # Frontières de la vraie taille de morceau ; on raisonne par intervalles.
    grain = 16384
    edge_checks = 0
    for lens in ((1, grain, 1), (grain + 1, grain - 1), (2 * grain + 1,),
                 (grain - 1, 2, grain, 1), (grain, grain, 1)):
        start = 0
        ranges = []
        for length in lens:
            ranges.append((start, start + length, start // grain))
            start += length
        need(sum(hi - lo for lo, hi, _ in ranges) == sum(lens), 'couverture frontière')
        need(all(owner * grain <= lo < (owner + 1) * grain for lo, _, owner in ranges), 'propriétaire')
        edge_checks += 1
    return {'exhaustive_partition_cases': checks, 'grain_16384_interval_cases': edge_checks}


def histories():
    totals = [0, 0]

    def visit(parent, sizes, events):
        roots = [i for i, p in enumerate(parent) if p == i]
        if len(roots) == 1:
            depth_reverse = [0] * len(parent)
            for loser, survivor in reversed(events):
                depth_reverse[loser] = min(255, depth_reverse[survivor] + 1)
            depth_paths = []
            for v in range(len(parent)):
                d, x = 0, v
                while parent[x] != x and d < 255:
                    d, x = d + 1, parent[x]
                depth_paths.append(d)
            need(depth_reverse == depth_paths, 'historiques différents')
            need(max(depth_paths) <= len(parent).bit_length() - 1, 'borne union par taille')
            totals[0] += 1
            totals[1] += len(parent)
            return
        for x, y in itertools.permutations(roots, 2):
            s, l = (y, x) if sizes[x] < sizes[y] else (x, y)
            p, z = parent[:], sizes[:]
            p[l], z[s] = s, sizes[s] + sizes[l]
            visit(p, z, events + [(l, s)])

    for n in range(1, 6):
        visit(list(range(n)), [1] * n, [])
    return {'complete_size_union_histories_n1_to_5': totals[0], 'node_depth_comparisons': totals[1]}


def closure(helpers, wait):
    # Les actes atomiques SC imposent cet ordre total. Travail et libération
    # sont intercalés sans supposer une fin immédiate de l'aide admise.
    start = ((0,) * helpers, (False,) * helpers, 0, 0, False, False)
    seen, todo = {start}, [start]
    unsafe = terminals = 0
    while todo:
        state = todo.pop()
        pcs, allowed, pc_k, active, closed, freed = state
        if pc_k == 3 and all(x == 4 for x in pcs):
            terminals += 1
        for actor in range(helpers + 1):
            ps, al = list(pcs), list(allowed)
            k, a, c, f = pc_k, active, closed, freed
            if actor == helpers:
                if k == 0:
                    c, k = True, 1
                elif k == 1 and (a == 0 or not wait):
                    k = 2
                elif k == 2:
                    f, k = True, 3
                else:
                    continue
            else:
                h = actor
                if ps[h] == 0:
                    a += 1
                elif ps[h] == 1:
                    al[h] = not c
                elif ps[h] == 2:
                    if al[h] and f:
                        unsafe += 1
                elif ps[h] == 3:
                    a -= 1
                else:
                    continue
                ps[h] += 1
            nxt = (tuple(ps), tuple(al), k, a, c, f)
            if nxt not in seen:
                seen.add(nxt)
                todo.append(nxt)
    return {'states': len(seen), 'unsafe_work_transitions': unsafe, 'terminal_states': terminals}


def source_checks(snapshot, repo, pins):
    src = {}
    for path, digest in pins['candidate_files'].items():
        data = (snapshot / path).read_bytes()
        need(sha(data) == digest, 'source candidate modifiée : ' + path)
        committed = subprocess.check_output(['git', 'show', pins['candidate_commit'] + ':' + path], cwd=repo)
        need(sha(committed) == digest, 'snapshot différent du candidat livré : ' + path)
        src[Path(path).name] = data.decode()
    for path, digest in pins['r1_files'].items():
        data = subprocess.check_output(['git', 'show', pins['r1_commit'] + ':' + path], cwd=repo)
        need(sha(data) == digest, 'pin R1 modifié : ' + path)
    run, kernel, pipe = src['pipeline_run.cpp'], src['forest_kernel.cpp'], src['pipeline.cpp']
    anchors = {
        'slice_release': 'std::atomic_ref<u8>(p.g_flags[i][slice]).store(flag, std::memory_order_release);' in run,
        'slice_acquire': 'std::atomic_ref<u8>(p.g_flags[i][s]).load(std::memory_order_acquire)' in run,
        'leaf_release': 'std::atomic_ref<u32>(leaves[p]).store(find_read(c, node), std::memory_order_release);' in kernel,
        'leaf_acquire': 'std::atomic_ref<u32>(leaves[p]).load(std::memory_order_acquire)' in kernel,
        'element_release': 'p.kernel_slice[i].store(s + 1, std::memory_order_release);' in run,
        'element_acquire': 'p.kernel_slice[i].load(std::memory_order_acquire) * tower_detail::kCellGrain' in run,
        'last_withdrawal_preserved': 'const bool last = p.in_flight.fetch_sub(1, std::memory_order_acq_rel) == 1;' in run,
        'hint_sc_guard': all(x in run for x in (
            'p.hint_active[i].fetch_add(1, std::memory_order_seq_cst)',
            'p.hint_closed[i].load(std::memory_order_seq_cst)',
            'p.hint_active[i].fetch_sub(1, std::memory_order_seq_cst)',
            'p.hint_closed[i].store(true, std::memory_order_seq_cst)',
            'p.hint_active[i].load(std::memory_order_seq_cst) != 0')),
        'no_hint_when_off': 'if (!p.chain[i]) return false;' in run,
        'admit_before_decision': pipe.index('r.budget.admit(r.diag.admitted_bytes)') < pipe.index('p.chain[i] = chain_engaged'),
        'unconditional_number_scratch': 'if (p.orders >= 2) bytes += u64{p.threads} * widest * (sizeof(num::Sphere) + 4);' in pipe,
    }
    need(all(anchors.values()), 'ancre source absente')
    return {'candidate_files_checked': len(pins['candidate_files']), 'r1_files_checked': len(pins['r1_files']),
            'source_anchors': anchors}


def publication(repo, pins):
    path = repo / pins['prefix_model']['path']
    need(sha(path.read_bytes()) == pins['prefix_model']['sha256'], 'modèle de préfixe modifié')
    spec = importlib.util.spec_from_file_location('a6_prefix_fragment_pinned', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    relaxed, bridge = module.verify(), module.verify(leaf_bridge=True)
    need(relaxed['accepted'] and not bridge['accepted'] and bridge['H_happens_before_U'], 'pont de préfixe')
    return {'events': bridge['events'], 'relaxed_fragment_accepted_by_checked_relations': relaxed['accepted'],
            'leaf_release_acquire_fragment_accepted': bridge['accepted'],
            'helper_parent_read_happens_before_future_kernel_write': bridge['H_happens_before_U'],
            'independent_formal_cpp_tool': False, 'geometric_reachability_proved': False}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('snapshot', type=Path)
    parser.add_argument('repo', type=Path)
    parser.add_argument('--write', action='store_true')
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    pins = json.loads((here / 'capture.json').read_text())
    result = source_checks(args.snapshot, args.repo, pins)
    result['prefix_publication_fragment'] = publication(args.repo, pins)
    result['cohort_ownership'] = cohorts()
    result['history_depth'] = histories()
    result['hint_close_sc'] = {}
    for n in (1, 2):
        good, mutant = closure(n, True), closure(n, False)
        need(good['unsafe_work_transitions'] == 0 and good['terminal_states'] > 0, 'garde SC')
        need(mutant['unsafe_work_transitions'] > 0, 'mutant attente non discriminé')
        result['hint_close_sc'][str(n)] = {'product_guard_model': good, 'wait_removed_model': mutant}
    result['scope'] = 'source_and_bounded_python_models_only_no_native_or_performance_qualification'
    encoded = json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True) + '\n'
    if args.write:
        (here / 'results.json').write_text(encoded)
    else:
        need(json.loads((here / 'results.json').read_text()) == result, 'résultat attendu différent')
    print(encoded, end='')


if __name__ == '__main__':
    main()
