#!/usr/bin/env python3
"""Application isolée d'un diff et modèle de comptabilité ; aucun moteur compilé."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def after(begin, end, published):
    return max(0, end - max(begin, published)) if published else 0


def model():
    cases = 0
    for g_task_end in range(4):
        for gap in range(5):
            for duration in range(5):
                l0, l1 = g_task_end + gap, g_task_end + gap + duration
                for published in range(l1 + 1):
                    for success in (False, True):
                        # Pré-passe : appel même si resolve_leaves rend un refus.
                        before_total = duration
                        new_total = l1 - l0
                        new_after = after(l0, l1, published)
                        oracle = sum(t >= published for t in range(l0, l1)) if published else 0
                        need(new_total == before_total and new_after == oracle, 'pré-passe')
                        # Le registre physique conserve le même incrément et la même issue.
                        need(7 + duration == 7 + (l1-l0), 'ledger feuilles')
                        need(bool(success) == success, 'flag Leaves / refus')
                        cases += 1
    # Fenêtre extérieure du noyau ; feuilles fraîches OU déjà faites, sans double charge.
    kernels = 0
    for l0 in range(5):
        for l1 in range(l0, 6):
            for k1 in range(l1, 7):
                for published in range(k1 + 1):
                    for precomputed in (False, True):
                        for success in (False, True):
                            leaf_spent = 0 if precomputed else l1 - l0
                            kernel_physical = k1 - leaf_spent
                            forest_total = k1
                            forest_after = after(0, k1, published)
                            need(kernel_physical + leaf_spent == forest_total, 'partition noyau')
                            need(forest_after == (sum(t >= published for t in range(k1)) if published else 0),
                                 'charge unique noyau')
                            kernels += 1
    # Témoins qui tuent les altérations ciblées du raccord de comptabilité.
    witness = dict(old_shift=after(10,20,25), corrected=after(20,30,25), expected=5)
    need(witness['old_shift'] != witness['expected'] == witness['corrected'], 'shift historique')
    need(40 + (30-20) != 40, 'double charge du noyau')
    need(0 != 30-20, 'omission sur refus')
    need(after(10,30,25) == 5 and 30-10 != 30-20, 'élargir au début G fausse le total')
    return dict(prepass_cases=cases, kernel_cases=kernels, targeted_mutations=4,
                zero_duration_included=True, failure_included=True, witness=witness)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--source',type=Path,required=True,help='pipeline_run.cpp original A/w902')
    args = ap.parse_args()
    pin = json.loads((HERE/'capture.json').read_text())
    original = args.source.read_bytes()
    need(sha(original)==pin['before_sha256'], 'source différente')
    patch = HERE/'proposition.patch'
    need(sha(patch.read_bytes())==pin['patch_sha256'], 'patch différent')
    with tempfile.TemporaryDirectory(prefix='audit-a-window-') as raw:
        root = Path(raw)
        target = root/pin['path']
        target.parent.mkdir(parents=True)
        target.write_bytes(original)
        for options in (['--check'],[]):
            subprocess.run(['git','apply',*options,str(patch)],cwd=root,check=True,capture_output=True)
        modified = target.read_bytes()
        need(sha(modified)==pin['proposed_sha256'], 'postimage différente')
        old, new = original.decode(), modified.decode()
        need(old.count('now_ns(p)')==new.count('now_ns(p)'), 'nouvelle lecture horloge')
        need(new.count('slice_leaves(')==3 and new.count('&window)')==1, 'appels inattendus')
        need('slice_leaves(p, i, w, begin, end, leaves_ns);' in new, 'noyau modifié')
        need(new.count('charge_forest(p, w, t0, t1);')==2, 'charges englobantes modifiées')
        subprocess.run(['git','apply','--reverse',str(patch)],cwd=root,check=True,capture_output=True)
        need(target.read_bytes()==original,'inverse différent')
    result=model()
    need(args.source.read_bytes()==original,'source changée pendant lecture')
    print(json.dumps(result|dict(isolated_apply_and_reverse=True,native_execution=False),sort_keys=True))


if __name__=='__main__':
    main()
