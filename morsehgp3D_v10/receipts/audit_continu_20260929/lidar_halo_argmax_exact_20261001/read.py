"""Read-only external-hash-first rejudge; never imports either diagnostic producer."""
import datetime
import hashlib
import json
import math
from pathlib import Path
import re
import stat
import sys

FILES = {'README.txt', 'check.py', 'read.py', 'normal.json', 'optimized.json', 'run_receipt.json'}
ARCHIVE_SHA = 'd7aa259033edb4ffebd60b359087c4873fd670f155efc651c4c8e7923fbdb741'
COLS = ('eta_quarter_lower', 'eta_quarter_upper', 'eta_one_lower', 'eta_one_upper')


def need(ok, why):
    if not ok:
        raise ValueError(why)


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


def verify_external(group):
    for name, pin in group.items():
        p = Path(name)
        need(p.is_file() and p.stat().st_size == pin['size'] and sha(p) == pin['sha256'], 'LIVE external pin '+name)


def main():
    need(len(sys.argv) == 3, 'root external_manifest_sha')
    root, external = Path(sys.argv[1]), sys.argv[2]
    for p in (root, *root.parents):
        need(stat.S_ISDIR(p.lstat().st_mode), 'nonregular root ancestor')
    mf = root/'MANIFEST.sha256'
    need(stat.S_ISREG(mf.lstat().st_mode) and re.fullmatch('[a-f0-9]{64}', external) is not None and sha(mf) == external,
         'external manifest BEFORE JSON/import')
    manifest = mf.read_bytes()
    need((root/'MANIFEST_SHA256').read_text() == external+'\n', 'anchor')
    need({p.name for p in root.iterdir()} == FILES | {'MANIFEST.sha256', 'MANIFEST_SHA256'}, 'closed inventory')
    listed = {}
    for line in manifest.decode().splitlines():
        m = re.fullmatch('([a-f0-9]{64})  (.+)', line)
        need(m is not None and m[2] in FILES and m[2] not in listed, 'manifest line')
        listed[m[2]] = m[1]
    need(set(listed) == FILES, 'complete manifest')
    for name in FILES | {'MANIFEST_SHA256'}:
        need(stat.S_ISREG((root/name).lstat().st_mode), 'nonregular payload')
    need(all(sha(root/n) == h for n, h in listed.items()), 'payload hash')
    need((root/'normal.json').read_bytes() == (root/'optimized.json').read_bytes(), 'normal/-O bytes')
    a, run = load(root/'normal.json'), load(root/'run_receipt.json')
    need(a['schema'] == 'mhgp10_halo_argmax_exact_v1' and a['status'] == 'PASS' and a['engine_used'] is False and
         a['scope'] == 'selected_argmax_anchors_exact_only_not_global_halo_maxima_not_classes_not_alpha_not_growth_not_G4', 'result scope')
    need(run['schema'] == 1 and run['normal_optimized_byte_equal'] is True and run['native_engine_launched'] is False and
         run['gcp_used'] is False and run['scope'] == 'exact_selected_argmax_anchors_only' and len(run['commands']) == 2, 'command scope')
    origin = Path(run['commands'][0]['argv'][-1]).parent
    for i, c in enumerate(run['commands']):
        expected = ['timeout', '10s', 'python3', '-B'] + (['-O'] if i else []) + [str(origin/'check.py')]
        need(c['argv'] == expected and type(c['exit']) is int and c['exit'] == 0 and
             c['combined_output_file'] == ('optimized.json' if i else 'normal.json') and
             math.isfinite(c['wall_time_seconds']) and 0 <= c['wall_time_seconds'] < 10, 'command/exit/wall')
    fmt = '%Y-%m-%d %H:%M:%S UTC'
    need(datetime.datetime.strptime(run['ended']['current_time'], fmt) >=
         datetime.datetime.strptime(run['started']['current_time'], fmt), 'UTC ordering')
    need(a['external_pins_before'] == a['external_pins_after'] and a['runtime_pins_before'] == a['runtime_pins_after'], 'before/after pins')
    verify_external(a['external_pins_before'])
    verify_external(a['runtime_pins_before'])
    need(a['runtime_pins_before'][str(origin/'check.py')]['sha256'] == listed['check.py'], 'captured exact producer')
    archive = Path(a['archive_path'])
    need(sha(archive/'manifest.json') == a['archive_manifest_sha256'] == run['archive_manifest_sha256'] == ARCHIVE_SHA, 'original manifest')
    original = load(archive/'capture_normal.json')
    raw_cases = {(r['input']['key'], r['K']): r for r in original['results'] if r['input']['part'] == 'full'}
    need(len(raw_cases) == 12 and len(a['cases']) == a['case_count'] == 12, '12 case inventory')
    need({(c['key'], c['K']) for c in a['cases']} == set(raw_cases), 'case identity')
    # Numerical import only after closed archive and every LIVE pin have passed.
    import numpy as np
    need(np.__version__ == a['numpy_version'], 'numpy version')
    scans = evaluations = 0
    for c in a['cases']:
        r = raw_cases[c['key'], c['K']]
        folder = Path(r['input']['folder'])
        p = np.fromfile(folder/'full.u32le', dtype='<u4').reshape((-1, 3)).astype(np.int64)
        ids = np.fromfile(folder/'full.site_ids.u32le', dtype='<u4')
        vector = np.fromfile(archive/r['vector_file'], dtype='<u8').reshape((-1, 5))
        n, K = len(p), c['K']
        need(n == len(ids) == len(vector) == c['n'] == r['n'] and K in (5, 10) and
             bool(np.all((p >= 0) & (p < 2**18))), 'u18 full input')
        need(c['kind'] == r['input']['kind'] and c['frame'] == r['input']['frame'] and
             c['vector_sha256'] == sha(archive/r['vector_file']) == r['vector_sha256'], 'case provenance')
        need([v['column'] for v in c['maxima']] == list(COLS), 'four maxima')
        selected = set()
        for j, m in enumerate(c['maxima'], 1):
            index = int(np.argmax(vector[:, j]))
            selected.add(index)
            sorted_column = np.sort(vector[:, j])
            q = {str(x): int(sorted_column[max(0, math.ceil(x*n/1000)-1)]) for x in (0, 250, 500, 750, 900, 950, 990, 999, 1000)}
            need(m == {'column': COLS[j-1], 'anchor_index': index, 'site_id': int(ids[index]), 'quantiles_nearest_rank': q}, 'argmax/quantiles')
        need([v['anchor_index'] for v in c['unique_exact_anchors']] == sorted(selected), 'closed anchor inventory')
        for v in c['unique_exact_anchors']:
            index = v['anchor_index']
            dx, dy, dz = p[:, 0]-p[index, 0], p[:, 1]-p[index, 1], p[:, 2]-p[index, 2]
            d2 = dx*dx+dy*dy+dz*dz
            dk = int(np.sort(d2)[K-1])
            cs = [int(np.count_nonzero(4*d2 <= 5*dk)), int(np.count_nonzero(d2 <= 5*dk)),
                  int(np.count_nonzero(d2 <= 2*dk)), int(np.count_nonzero(d2 <= 8*dk))]
            expected = {'anchor_index': index, 'site_id': int(ids[index]), 'xyz_mm': [int(x) for x in p[index]],
                        'sites_scanned': n, 'exact_dK2': dk, 'exact_counts': cs, 'archive_row': [int(x) for x in vector[index]]}
            need(v == expected and v['archive_row'] == [dk, *cs] and dk > 0, 'exhaustive selected anchor')
            scans += 1
            evaluations += n
    bound = 3*(2**18-1)**2
    need(a['column_maximum_controls'] == 48 and scans == a['unique_anchor_scans'] == 28 and
         evaluations == a['site_distance_evaluations'] == 2124208 and a['u18_max_squared_distance'] == bound and
         a['largest_coefficient_product_bound'] == 8*bound < a['int64_max'] == 2**63-1, 'nonvacuity/arithmetic bound')
    verify_external(a['external_pins_after'])
    verify_external(a['runtime_pins_after'])
    need(all(sha(root/n) == h for n, h in listed.items()) and mf.read_bytes() == manifest, 'changed during read')
    print('HALO_ARGMAX_EXACT_OK cases=12 columns=48 unique_anchors=28 evaluations=2124208 engine_replayed=0')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError, KeyError, IndexError, TypeError) as e:
        print('REFUS '+str(e), file=sys.stderr)
        sys.exit(2)
