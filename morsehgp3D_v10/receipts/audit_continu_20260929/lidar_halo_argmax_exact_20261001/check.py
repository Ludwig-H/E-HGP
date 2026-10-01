"""Selected-anchor exhaustive integer diagnostic; never imports the archived halo."""
import hashlib
import json
import math
from pathlib import Path
import re
import stat
import sys

ARCHIVE = Path('/tmp/lidar-halo-locality.i8TpNR3i')
ARCHIVE_SHA = 'd7aa259033edb4ffebd60b359087c4873fd670f155efc651c4c8e7923fbdb741'
COLS = ('eta_quarter_lower', 'eta_quarter_upper', 'eta_one_lower', 'eta_one_upper')


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(p):
    h = hashlib.sha256()
    with open(p, 'rb') as f:
        for b in iter(lambda: f.read(1048576), b''):
            h.update(b)
    return h.hexdigest()


def pairs(values):
    out = {}
    for k, v in values:
        need(k not in out, 'duplicate JSON key')
        out[k] = v
    return out


def load(p):
    return json.loads(p.read_text(), object_pairs_hook=pairs,
                      parse_constant=lambda _: (_ for _ in ()).throw(ValueError('nonfinite JSON')))


def pins(paths):
    return {str(p): {'sha256': sha(p), 'size': p.stat().st_size} for p in sorted(paths)}


def stable(value):
    if isinstance(value, dict):
        return {k: stable(v) for k, v in value.items() if k not in ('volatile', 'python_optimize')}
    if isinstance(value, list):
        return [stable(v) for v in value]
    return value


def main():
    need(len(sys.argv) == 1, 'no arguments')
    for p in (ARCHIVE, *ARCHIVE.parents):
        need(stat.S_ISDIR(p.lstat().st_mode), 'archive ancestor')
    need(sha(ARCHIVE/'manifest.json') == ARCHIVE_SHA, 'external archive hash BEFORE JSON/import')
    manifest = load(ARCHIVE/'manifest.json')
    need(set(p.name for p in ARCHIVE.iterdir()) == set(manifest['files']) | {'manifest.json'}, 'archive inventory')
    need(manifest['closed_inventory_count_including_manifest'] == 47, 'archive inventory count')
    for name, info in manifest['files'].items():
        need(Path(name).name == name and stat.S_ISREG((ARCHIVE/name).lstat().st_mode), 'regular archive payload')
        need((ARCHIVE/name).stat().st_size == info['size'] and sha(ARCHIVE/name) == info['sha256'], 'archive payload pin')
    capture = load(ARCHIVE/'capture_normal.json')
    other = load(ARCHIVE/'capture_optimized.json')
    need(stable(capture) == stable(other), 'original normal/-O semantic equality')
    cases = [r for r in capture['results'] if r['input']['part'] == 'full']
    expected = {(kind, frame, K) for kind in ('nonground', 'raw') for frame in (0, 100, 200) for K in (5, 10)}
    need(len(cases) == 12 and {(r['input']['kind'], r['input']['frame'], r['K']) for r in cases} == expected,
         'exact twelve-case inventory')
    paths = {ARCHIVE/'manifest.json', ARCHIVE/'capture_normal.json', ARCHIVE/'capture_optimized.json', ARCHIVE/'halo.py'}
    for r in cases:
        folder = Path(r['input']['folder'])
        paths.update((folder/'MANIFEST.json', folder/'full.u32le', folder/'full.site_ids.u32le', ARCHIVE/r['vector_file']))
    original_inputs = manifest['live_pins']['inputs']
    for p in paths:
        if not p.is_relative_to(ARCHIVE):
            info = original_inputs[str(p)]
            need(p.stat().st_size == info['size'] and sha(p) == info['sha256'], 'original input pin')
    before = pins(paths)
    # Only numerical dependency used by this probe. No scipy, halo.py or HGP import.
    import numpy as np
    np.sum(np.array([0], dtype=np.int64), dtype=np.int64)
    runtime = {Path(sys.executable).resolve(), Path(__file__).absolute()}
    for mod in tuple(sys.modules.values()):
        name = getattr(mod, '__file__', None)
        if name and Path(name).is_file():
            runtime.add(Path(name).resolve())
    for line in Path('/proc/self/maps').read_text().splitlines():
        name = line.split()[-1]
        if name.startswith('/') and Path(name).is_file():
            runtime.add(Path(name).resolve())
    runtime_before = pins(runtime)
    points = {}
    output = []
    total = 0
    comparisons = 0
    for r in cases:
        s, K = r['input'], r['K']
        folder = Path(s['folder'])
        if str(folder) not in points:
            m = load(folder/'MANIFEST.json')
            need(m['profile'] == 'quantized_u32_fixed_grid_input_only' and m['parameters']['precision_mm'] == '1', '1mm grid profile')
            meta = m['datasets']['full']
            need(meta['points_file'] == 'full.u32le' and meta['site_ids_file'] == 'full.site_ids.u32le', 'full input filenames')
            p = np.fromfile(folder/'full.u32le', dtype='<u4').reshape((-1, 3)).astype(np.int64)
            ids = np.fromfile(folder/'full.site_ids.u32le', dtype='<u4')
            need(len(p) == len(ids) == meta['sites'] and len(np.unique(p, axis=0)) == len(p), 'distinct site inventory')
            need(bool(np.all((p >= 0) & (p < (1 << 18)))), 'u18 bound')
            need(sha(folder/'full.u32le') == meta['points_sha256'] and sha(folder/'full.site_ids.u32le') == meta['site_ids_sha256'], 'preparation pins')
            points[str(folder)] = (p, ids)
        p, site_ids = points[str(folder)]
        n = len(p)
        vector = np.fromfile(ARCHIVE/r['vector_file'], dtype='<u8').reshape((-1, 5))
        need(n == r['n'] == len(vector) and n >= K and r['columns'] == ['dK2', *COLS], 'vector schema')
        need(sha(ARCHIVE/r['vector_file']) == r['vector_sha256'], 'vector capture pin')
        maxima = []
        selected = set()
        for j, col in enumerate(COLS, 1):
            anchor = int(np.argmax(vector[:, j]))
            selected.add(anchor)
            sorted_counts = np.sort(vector[:, j])
            quantiles = {str(q): int(sorted_counts[max(0, math.ceil(q*n/1000)-1)]) for q in (0, 250, 500, 750, 900, 950, 990, 999, 1000)}
            need(quantiles['1000'] == int(vector[anchor, j]) == r['halos'][col]['maximum'], 'stored maximum')
            maxima.append({'column': col, 'anchor_index': anchor, 'site_id': int(site_ids[anchor]), 'quantiles_nearest_rank': quantiles})
        controls = []
        for anchor in sorted(selected):
            delta = p - p[anchor]
            squared = np.sum(delta * delta, axis=1, dtype=np.int64)
            need(len(squared) == n and int(squared[anchor]) == 0, 'full scan self')
            dk = int(np.partition(squared, K-1)[K-1])
            exact = [int(np.count_nonzero(4*squared <= 5*dk)), int(np.count_nonzero(squared <= 5*dk)),
                     int(np.count_nonzero(squared <= 2*dk)), int(np.count_nonzero(squared <= 8*dk))]
            archived = [int(v) for v in vector[anchor]]
            need([dk, *exact] == archived, 'EXACT SELECTED-ANCHOR MISMATCH '+s['key']+' K'+str(K)+' anchor'+str(anchor))
            need(dk > 0 and all(c >= K for c in exact), 'positive scale and self-inclusive K')
            controls.append({'anchor_index': anchor, 'site_id': int(site_ids[anchor]), 'xyz_mm': [int(v) for v in p[anchor]],
                             'sites_scanned': n, 'exact_dK2': dk, 'exact_counts': exact, 'archive_row': archived})
            total += 1
            comparisons += n
        output.append({'key': s['key'], 'kind': s['kind'], 'frame': s['frame'], 'K': K, 'n': n,
                       'maxima': maxima, 'unique_exact_anchors': controls, 'vector_sha256': r['vector_sha256']})
    need(12 <= total <= 48, 'bounded non-vacuous anchor inventory')
    need(before == pins(paths) and runtime_before == pins(runtime), 'source/input/runtime changed')
    for name, info in manifest['files'].items():
        need(sha(ARCHIVE/name) == info['sha256'], 'original archive changed')
    print(json.dumps({'schema': 'mhgp10_halo_argmax_exact_v1', 'status': 'PASS', 'scope': 'selected_argmax_anchors_exact_only_not_global_halo_maxima_not_classes_not_alpha_not_growth_not_G4',
                      'archive_path': str(ARCHIVE), 'archive_manifest_sha256': ARCHIVE_SHA, 'engine_used': False,
                      'cases': output, 'case_count': 12, 'column_maximum_controls': 48, 'unique_anchor_scans': total,
                      'site_distance_evaluations': comparisons, 'u18_max_squared_distance': 3*((1 << 18)-1)**2,
                      'largest_coefficient_product_bound': 8*3*((1 << 18)-1)**2, 'int64_max': (1 << 63)-1,
                      'external_pins_before': before, 'external_pins_after': pins(paths),
                      'runtime_pins_before': runtime_before, 'runtime_pins_after': pins(runtime),
                      'numpy_version': np.__version__}, sort_keys=True, indent=1))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError, KeyError, IndexError, TypeError) as e:
        print('REFUS '+str(e), file=sys.stderr)
        sys.exit(2)
