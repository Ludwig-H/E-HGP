"""Petits mutants du juge catalogue, sans executer ni modifier le produit."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
from itertools import product
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source = args.source / 'tests/oracle/test_catalogue_oracle.py'
    before = hashlib.sha256(source.read_bytes()).hexdigest()
    spec = importlib.util.spec_from_file_location('catalogue_judge', source)
    judge = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(judge)
    sites = list(product((0, 2), repeat=3))
    weights = [1] * len(sites)
    ref = judge.Ref(sites)
    expected = judge.expected(ref, weights, 3)
    dense = {level: i for i, level in enumerate(sorted({b.level for b, _, _, _ in expected}))}
    got = []
    for b, p, u, weighted in expected:
        shell = sorted((sites[i] for i in b.U), key=judge.morton)
        support, _ = judge.canonical_support(shell, b.qmin, b.center, b.level)
        got.append(dict(rank=dense[b.level], q=b.qmin, p=p, u=u,
                        flags=int(len(shell) > b.qmin) | (2 * int(weighted)), level=b.level,
                        sup=list(support), I=sorted((sites[i] for i in b.I), key=judge.morton), U=shell))
    got.sort(key=lambda b: (b['level'], [judge.morton(p) for p in b['sup']]))
    published = dict(balls=len(got), levels=len(dense))
    baseline = judge.judge(ref, weights, 3, got, published)
    results = {}
    reverse = list(reversed(copy.deepcopy(got)))
    results['reverse_export_order'] = judge.judge(ref, weights, 3, reverse, published)
    for field in ('I', 'U'):
        mutant = copy.deepcopy(got)
        target = next(b for b in mutant if b[field] and (field == 'I' or len(b['U']) > b['q']))
        target[field].append(target[field][0])
        results['duplicate_' + field] = judge.judge(ref, weights, 3, mutant, published)
    reorder = copy.deepcopy(got)
    target = next(b for b in reorder if len(b['U']) > b['q'])
    target['U'].reverse()
    results['reverse_shell_order'] = judge.judge(ref, weights, 3, reorder, published)
    rank = copy.deepcopy(got)
    for b in rank:
        b['rank'] += 1
    results['rank_offset_positive_control'] = judge.judge(ref, weights, 3, rank, published)
    support = copy.deepcopy(got)
    target = next(b for b in support if len(b['U']) == 8)
    target['sup'] = [(2, 0, 0), (0, 2, 2)]
    results['noncanonical_support_positive_control'] = judge.judge(ref, weights, 3, support, published)
    after = hashlib.sha256(source.read_bytes()).hexdigest()
    report = dict(source=str(source), before_sha256=before, after_sha256=after,
                  input=sites, K=3, baseline_error=baseline, balls=len(got), mutants=results,
                  interpretation='null = mutant non rejete par judge ; controles positifs doivent donner un ecart')
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n')
    print(json.dumps(report, ensure_ascii=False))
    if before != after or baseline is not None:
        return 3
    return 1 if results['duplicate_I'] is None or results['duplicate_U'] is None else 0


if __name__ == '__main__':
    sys.exit(main())
