"""Exhaustive fixed-cut partitions, no HGP oracle and no assert."""
import json
import pathlib
import sys


def require(ok, why):
    if not ok:
        raise ValueError(why)


def canonical(blocks):
    return tuple(sorted((tuple(sorted(b, key=repr)) for b in blocks), key=repr))


def keep(partition, mcs):
    return canonical(b for b in partition if len(b) >= mcs)


def restricted_growth(n):
    a = [0]*n
    def visit(i, largest):
        if i == n:
            yield tuple(a)
            return
        for label in range(largest+2):
            a[i] = label
            yield from visit(i+1, max(largest, label))
    yield from visit(1, 0)


def from_labels(ids, labels):
    groups = {}
    for point, label in zip(ids, labels):
        groups.setdefault(label, []).append(point)
    return canonical(groups.values())


def main():
    require(len(sys.argv) == 2, 'one fixture path')
    fixture = json.loads(pathlib.Path(sys.argv[1]).read_text())
    ids = fixture['ids']
    require(ids == list('ABCDEF'), 'six fixed distinct IDs')
    target2, target3 = canonical(fixture['target_mcs2']), canonical(fixture['target_mcs3'])
    require(target2 == canonical(['AB', 'CD', 'EF']) and target3 == canonical(['ABC', 'DEF']), 'targets')
    rgs = list(restricted_growth(6))
    require(len(rgs) == 203 and len(set(rgs)) == 203, 'Bell6 inventory')
    witnesses2, witnesses3, both = [], [], []
    point_checks = label_checks = monotonicity_checks = 0
    for code in rgs:
        partition = from_labels(ids, code)
        c2, c3 = keep(partition, 2), keep(partition, 3)
        if c2 == target2:
            witnesses2.append(''.join(map(str, code)))
        if c3 == target3:
            witnesses3.append(''.join(map(str, code)))
        if c2 == target2 and c3 == target3:
            both.append(''.join(map(str, code)))
        for lower in range(1, 7):
            for upper in range(lower, 8):
                require(set(keep(partition, upper)).issubset(keep(partition, lower)), 'retention monotonicity')
                monotonicity_checks += 1
        for renamed in fixture['point_renamings']:
            require(len(renamed) == 6 and len(set(renamed)) == 6, 'bijective point renaming')
            mapping = dict(zip(ids, renamed))
            transformed = [[mapping[p] for p in b] for b in partition]
            for threshold in (2, 3):
                expected = canonical([[mapping[p] for p in b] for b in keep(partition, threshold)])
                require(keep(transformed, threshold) == expected, 'point renaming changed memberships')
                point_checks += 1
        for labels in fixture['cluster_labels']:
            require(len(labels) == 6 and len(set(labels)) == 6, 'distinct opaque cluster labels')
            require(from_labels(ids, [labels[i] for i in code]) == partition, 'label renaming changed partition')
            label_checks += 1
    require(witnesses2 == ['001122'] and witnesses3 == ['000111'] and both == [], 'conditional obstruction')
    compatibility = []
    for case in fixture['compatible_cases']:
        partition = canonical(case['partition'])
        require(sorted(p for b in partition for p in b) == ids, 'compatible partition inventory')
        for threshold in (2, 3):
            require(keep(partition, threshold) == canonical(case['mcs'+str(threshold)]), 'compatible target')
        compatibility.append(case['id'])
    result = dict(schema=1, scope='conditional_fixed_cut_retention_only_not_Pi2_or_user_choice',
                  partitions=203, all_rgs=[''.join(map(str, c)) for c in rgs],
                  witnesses_mcs2=witnesses2, witnesses_mcs3=witnesses3, witnesses_both=both,
                  point_renaming_checks=point_checks, opaque_label_checks=label_checks,
                  retention_monotonicity_checks=monotonicity_checks, compatible_cases=compatibility,
                  external_oracle=False, engine_used=False)
    print(json.dumps(result, sort_keys=True, separators=(',', ':')))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, KeyError, OSError, TypeError) as e:
        print('REFUS '+str(e))
        sys.exit(2)
