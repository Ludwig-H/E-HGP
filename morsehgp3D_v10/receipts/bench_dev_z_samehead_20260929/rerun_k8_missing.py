"""Complete kcover_dev_k8cap.csv : les deux scenes dev perdues a K = 8 par le refus rank_order (corrige en 2d8be1bfc),
recalculees avec le meme chemin (kcover_dev.run_unit, entree cap, sklearn compris) et le binaire corrige."""
import csv
import json
import sys

sys.argv = ['kcover_dev.py', '--syn', '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/bench/synthetic',
            '--entry', 'cap']
import kcover_dev  # noqa: E402

specs = json.load(open('k8_missing_specs.json'))
rows = []
for s in specs:
    rows += kcover_dev.run_unit(s, '/workspaces/E-HGP/build/v10-bench-8b8d66f6e/build', [8])
with open('kcover_dev_k8cap_missing.csv', 'w', newline='') as h:
    w = csv.DictWriter(h, fieldnames=kcover_dev.COLS)
    w.writeheader()
    for r in rows:
        w.writerow(r)
print(len(rows), 'lignes')
