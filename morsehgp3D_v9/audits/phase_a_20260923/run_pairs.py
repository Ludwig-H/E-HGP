import hashlib
import json
import os
import pathlib
import subprocess
import time

base = pathlib.Path('/tmp/mhgp9_phase_a_sidecar')
scene = pathlib.Path(os.environ.get(
    'MHGP9_AUDIT_SCENE',
    '/workspaces/E-HGP/morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le',
))
catalogue = base / 'scene00.catalogue.bin'
reference = base / 'baseline.canonical'
record = []
for pair, modes in enumerate(('AB', 'BA', 'AB'), 1):
    for mode in modes:
        executable = base / ('build_tower_baseline' if mode == 'A' else 'build_tower_fast')
        output = base / f'pair_{pair}_{mode}.canonical'
        command = [str(executable), str(scene), str(catalogue), str(output)]
        t = time.monotonic()
        result = subprocess.run(command, text=True, capture_output=True)
        wall = time.monotonic() - t
        (base / f'pair_{pair}_{mode}.stdout').write_text(result.stdout)
        (base / f'pair_{pair}_{mode}.stderr').write_text(result.stderr)
        if result.returncode != 0:
            raise RuntimeError(f'{pair} {mode}: status {result.returncode}: {result.stderr}')
        match = subprocess.run(['cmp', '-s', str(reference), str(output)]).returncode == 0
        row = dict(pair=pair, mode=mode, wall_s=wall, summary=result.stdout.strip(),
                   size=output.stat().st_size, exact_cmp=match)
        record.append(row)
        print(json.dumps(row), flush=True)
        if not match:
            raise RuntimeError(f'{pair} {mode}: canonical output differs')
        output.unlink()
(base / 'pair_summary.json').write_text(json.dumps(record, indent=2))
