"""Tranche 2 : memes mesures pour la nouvelle voie seule ; identite contre les empreintes de la tranche 1."""
import json, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))
import measure

S = measure.S
measure.BUILDS['new2'] = S / 'meas_new2_bench'
base = json.loads((S / 'meas/records.json').read_text())
expected = {r['case']: r['output_sha256'] for r in base}
plan = []
for rep in range(3):
    for case in ['lidar_ng00', 'lidar_ng01', 'lidar_ng02']:
        plan.append(('new2', case, 16379, 4))
for case in ['uniform_u18_n8000', 'uniform_u18_n16000', 'uniform_u18_n32000']:
    plan.append(('new2', case, 16379, 4))
plan.append(('new2', 'lidar_ng00', 16379, 1))
records = [measure.run(*p) for p in plan]
bad = [r for r in records if r['output_sha256'] != expected[r['case']] or r['rc'] != 0]
(S / 'meas/records_r2.json').write_text(json.dumps(records, indent=1) + '\n')
print('identite', 'OK' if not bad else 'ECHEC %d' % len(bad))
