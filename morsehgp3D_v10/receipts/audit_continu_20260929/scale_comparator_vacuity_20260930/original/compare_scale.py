"""Compare les CSV de scale_run ancien/nouveau (colonnes deterministes a 1 fil) et la coherence de chaque ligne `ok` du
nouveau avec ses deux JSON natifs (.calls.jsonl). Verificateur adverse.

  python3 compare_scale.py <ancien.csv> <nouveau.csv>
"""
import csv
import json
import sys

TIMING = {'catalogue_s', 'tower_s', 'wall_s', 'cpu_s', 'max_rss_kb'}


def main():
    old_path, new_path = sys.argv[1], sys.argv[2]
    with open(old_path, newline='') as f:
        r = csv.DictReader(f)
        old_fields, old = r.fieldnames, list(r)
    with open(new_path, newline='') as f:
        r = csv.DictReader(f)
        new_fields, new = r.fieldnames, list(r)
    det = [c for c in new_fields if c not in TIMING]
    diffs = []
    for a, b in zip(old, new):
        for c in det:
            if a.get(c) != b.get(c):
                diffs.append((a.get('file'), c, a.get(c), b.get(c)))
    calls = [json.loads(l) for l in open(new_path + '.calls.jsonl')]
    coherent = []
    for row in new:
        mine = [c for c in calls if c['file'] == row['file'] and str(c['k']) == row['k']]
        cat = [json.loads([l for l in c['stdout'].splitlines() if l.strip().startswith('{')][-1])
               for c in mine if c['call'] == 'catalogue']
        tw = [json.loads([l for l in c['stdout'].splitlines() if l.strip().startswith('{')][-1])
              for c in mine if c['call'] == 'tower']
        ok = len(cat) == 1 and len(tw) == 1
        if ok and row['status'] == 'ok':
            orders = tw[0].get('orders', [])
            ok = (str(cat[0]['balls']) == row['balls'] == str(tw[0]['balls']) and
                  str(cat[0].get('judged')) == row['cat_judged'] and
                  str(sum(o['nodes'] for o in orders)) == row['tower_nodes_all'] and
                  str(sum(o['steps'] for o in orders)) == row['tower_steps_all'] and
                  str(tw[0].get('tower_s')) == row['tower_s'])
        coherent.append((row['file'], row['k'], row['status'], ok))
    out = dict(lignes_ancien=len(old), lignes_nouveau=len(new), entetes_egaux=old_fields == new_fields,
               colonnes_deterministes=len(det), ecarts=diffs, lignes_coherentes_avec_json_natifs=coherent,
               appels=len(calls))
    print(json.dumps(out, ensure_ascii=False, indent=1))
    return 0 if not diffs and old_fields == new_fields and all(c[3] for c in coherent) else 1


if __name__ == '__main__':
    sys.exit(main())
