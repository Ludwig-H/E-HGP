#!/usr/bin/env python3
"""Contre-lecture de metadonnees epinglees, aucun payload LiDAR lu."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import argparse

ROOT = Path(__file__).resolve().parents[4]
BASE = 'f601b36ace16bcc8f7ac9bc532e45ab5079f9667'
OLD = '1f7642e105aebd76632c58c63fdfd5b5c0824779'
PREFIX = 'morsehgp3D_v12/'
sources = {}


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def read(path, pin=BASE):
    data = subprocess.check_output(['git', 'show', pin + ':' + path], cwd=ROOT)
    sources[pin + ':' + path] = hashlib.sha256(data).hexdigest()
    return data.decode('utf-8')


def tables(text, old=False):
    crops, scenes = {}, {}
    for line in text.splitlines():
        cols = [c.strip().strip('`') for c in line.split('|')[1:-1]]
        if not cols or not re.fullmatch(r'(ign|eth3d|forinst|boreas)_[A-Za-z0-9_]+', cols[0]):
            continue
        name = cols[0]
        if re.search(r'_c[1248]M$', name):
            require(name not in crops, 'duplicate crop')
            require(len(cols) == (6 if old else 7), 'crop columns')
            nums = [int(s.replace(' ', '')) for s in cols[1:(3 if old else 4)]]
            if old:
                nums = [nums[0]] + nums
            target, count, returns = nums
            require(returns >= count >= target > 0, 'counts')
            require(re.fullmatch('[0-9a-f]{16}', cols[-1]) is not None, 'hash')
            crops[name] = dict(target=target, count=count, returns=returns,
                               side=cols[-3], bits=int(cols[-2]), sha16=cols[-1])
        elif len(cols) == 8:
            require(name not in scenes, 'duplicate scene')
            scenes[name] = cols
    return crops, scenes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--external-root', type=Path, help='dossier local des donnees : manifestes, stat et journal seulement')
    args = parser.parse_args()
    current, scenes = tables(read(PREFIX + 'docs/DONNEES.md'))
    old, old_scenes = tables(read(PREFIX + 'docs/DONNEES.md', OLD), old=True)
    require(len(current) == 69 and current.keys() == old.keys(), '69 same crop names')
    require(len(scenes) == 24 and scenes == old_scenes, '24 unchanged full scenes')
    groups = {}
    for name, row in current.items():
        previous = old[name]
        require(row['target'] == previous['count'], 'target unchanged')
        require(row['side'] == previous['side'] and row['bits'] == previous['bits'], 'side/bits')
        require(row['sha16'] != previous['sha16'], 'crop hash changed')
        require(row['count'] > row['target'], 'complete boundary increases sites')
        group = groups.setdefault(name.split('_')[0], {'crops': 0, 'excess': [], 'returns_added': []})
        group['crops'] += 1
        group['excess'].append(row['count'] - row['target'])
        group['returns_added'].append(row['returns'] - previous['returns'])
    for group in groups.values():
        for key in ('excess', 'returns_added'):
            group[key] = [min(group[key]), max(group[key])]
    rpath = PREFIX + 'receipts/g4_t2e_20261007/'
    receipt = json.loads(read(rpath + 'receipt.json'))
    files = {row['name']: row for row in receipt['data_files']}
    require(len(files) == len(receipt['data_files']), 'unique files')
    epath = rpath + 'resultats/cmd/004_mes_e/files/mes_e/'
    takes = json.loads(read(epath + 'mes_e.json'))['prises']
    require(len(takes) == 11, '11 takes')
    checked = []
    for take in takes:
        name, order = take['cas'], take['k']
        row = current[name]
        xyz, ids = files[name + '.u32le'], files[name + '.ids.u32le']
        require(xyz['sha256'].startswith(row['sha16']), 'uploaded hash vs table')
        require(xyz['size'] == 12 * row['count'] and ids['size'] == 4 * row['count'], 'uploaded lengths')
        require(take['code'] == 0 and take['expire'] is False, 'successful take')
        native = [json.loads(line) for line in read(epath + name + '_k' + str(order) + '.jsonl').splitlines()]
        cloud = [line for line in native if line.get('phase') == 'cloud']
        require(len(cloud) == 1, 'one cloud record')
        require(cloud[0]['sites'] == cloud[0]['points'] == row['count'] == take['sonde']['sites'], 'native sites')
        checked.append(dict(case=name, k=order, sites=row['count'], sha256=xyz['sha256'],
                            ids_sha256=ids['sha256']))
    external = audit_external(args.external_root, current, scenes, checked) if args.external_root else None
    print(json.dumps(dict(pin=BASE, previous_pin=OLD, crop_tables=groups,
                          unchanged_scenes=len(scenes), uploaded_unique_crops=len({t['case'] for t in checked}),
                          measured_takes=checked, source_sha256=sources, external=external,
                          scope='metadata_only_no_payload_no_new_timing'), indent=2, sort_keys=True))


def audit_external(root, tables_current, scene_tables, transferred):
    """Aucun open sur un payload : JSON, journal et listes SHA seulement ; payloads par stat."""
    observed, crop_rows, seen_crops = {}, [], set()
    local_pins = {}
    families = ('ign_lidarhd', 'eth3d', 'forinstance', 'boreas')
    journal_path = root / 'preparer.log'
    journal_bytes = journal_path.read_bytes()
    journal = journal_bytes.decode('utf-8')
    local_pins['preparer.log'] = hashlib.sha256(journal_bytes).hexdigest()
    rule_source = read(PREFIX + 'bench/data/crop_scenes.py')
    import ast
    rule_tree = ast.parse(rule_source)
    rule = next(ast.literal_eval(node.value) for node in rule_tree.body if isinstance(node, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == 'RULE' for t in node.targets))

    def meta(path, label):
        raw = path.read_bytes()
        local_pins[label] = hashlib.sha256(raw).hexdigest()
        return json.loads(raw)

    def variant_files(row):
        n = row['count']
        require(type(n) is int and n > 0, 'positive payload count')
        entries = [('coordinates', 'sha256', 12), ('point_ids', 'ids_sha256', 4)]
        if 'mult' in row:
            entries.append(('mult', 'mult_sha256', 4))
        result = {}
        for key, hash_key, size in entries:
            name, digest = row[key], row[hash_key]
            require(re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]*', name) is not None, 'simple payload name')
            require(re.fullmatch(r'[0-9a-f]{64}', digest) is not None, 'complete payload digest')
            result[name] = (digest, n * size)
        return result

    def files_of(rows, bundle=False):
        files = {}
        for row in rows:
            if bundle and row.get('bundled') == 'distinct':
                versions = [row['distinct']]
            else:
                versions = [row] + ([row['distinct']] if 'distinct' in row else [])
            for version in versions:
                for name, value in variant_files(version).items():
                    require(name not in files or files[name] == value, 'contradictory file')
                    files[name] = value
        return files

    for family in families:
        srcdir = root / 'data' / family
        bdir = root / 'bundles' / ('g4_' + family)
        src = meta(srcdir / 'manifest.json', 'data/' + family + '/manifest.json')
        bundle = meta(bdir / 'bundle_manifest.json', 'bundles/g4_' + family + '/bundle_manifest.json')
        require(src['schema'] == bundle['schema'] == 'mhgp12.benchmark_inputs.v1', 'schema')
        parents = {row['name']: row for row in src['cases']}
        require(len(parents) == len(src['cases']), 'unique parents')
        require(all(name in scene_tables for name in parents), 'scene table names')
        for name, parent in parents.items():
            table = scene_tables[name]
            require([int(s.replace(' ', '')) for s in table[1:4]] ==
                    [parent['count'], parent['n_distinct'], parent['duplicate_sites']], 'full scene counts')
            require(parent['count'] - parent['n_distinct'] == parent['duplicate_sites'], 'scene count identity')
            require(parent['bits_needed'] == int(table[5]) and parent['sha256'].startswith(table[6]),
                    'full scene bits/hash')
            if 'distinct' in parent:
                require(parent['distinct']['count'] == parent['n_distinct'] and
                        parent['distinct']['sha256'].startswith(table[7]), 'full scene distinct hash')
        generator = src['generator']
        script = read(PREFIX + 'bench/data/' + generator['script'])
        require(hashlib.sha256(script.encode()).hexdigest() == generator['script_sha256'], 'generator pin')
        for name, digest in generator['library_sha256'].items():
            library = read(PREFIX + 'bench/data/v12data/' + name)
            require(hashlib.sha256(library.encode()).hexdigest() == digest, 'generator library pin')
        allrows = src['cases'] + src['crops']
        bundled = {row['name']: row for row in bundle['cases']}
        require(len(bundled) == len(allrows) == len(bundle['cases']), 'complete bundle cases')
        require(bundle['sources'] == [str(srcdir / 'manifest.json')], 'bundle source pointer')
        for row in allrows:
            other = dict(bundled[row['name']])
            variant = other.pop('bundled', None)
            require(other == row, 'source/bundle record identity')
            require(variant == ('distinct' if 'distinct' in row else None), 'selected variant')
        for row in src['crops']:
            name = row['name']; require(name not in seen_crops, 'unique crop')
            seen_crops.add(name)
            tab, crop = tables_current[name], row['crop']
            parent = parents[row['parent']]
            parent = parent.get('distinct', parent)
            require(crop['rule'] == rule, 'correct closed-square crop rule')
            require(crop['parent_sites'] == parent['count'] and crop['parent_coordinates_sha256'] == parent['sha256'],
                    'parent count and hash')
            require(crop['target_sites'] == tab['target'] and row['count'] == tab['count'], 'crop counts vs table')
            require(row.get('returns', row['count']) == tab['returns'], 'returns vs table')
            require(row['bits_needed'] == tab['bits'] and row['sha256'].startswith(tab['sha16']), 'bits/hash vs table')
            require(format(2 * crop['radius_chebyshev_mm'] / 1000, '.1f') == tab['side'], 'side vs table')
            require(row['duplicate_sites'] == 0 and row['count'] < parent['count'], 'distinct proper crop')
            log_line = '%s : %d sites (taille visee %d), %d bits, rayon %d mm' % (
                name, row['count'], crop['target_sites'], row['bits_needed'], crop['radius_chebyshev_mm'])
            require(journal.count(log_line) == 1, 'one corrected crop generation log')
            crop_rows.append(dict(name=name, count=row['count'], target=crop['target_sites'],
                                  returns=row.get('returns', row['count']), sha256=row['sha256'],
                                  ids_sha256=row['ids_sha256'], mult_sha256=row.get('mult_sha256'),
                                  parent=row['parent'], parent_sha256=parent['sha256']))
        source_files = files_of(allrows)
        bundle_files = files_of(bundle['cases'], bundle=True)
        require(len(bundle_files) == bundle['files'], 'bundle file count')
        require(sum(v[1] for v in bundle_files.values()) == bundle['bytes'], 'bundle bytes')
        require(bundle['files'] + 2 <= 512 and bundle['bytes'] <= 8 * 2**30, 'bundle limits')
        sums_path = bdir / 'SHA256SUMS.txt'
        sums_raw = sums_path.read_bytes()
        local_pins['bundles/g4_' + family + '/SHA256SUMS.txt'] = hashlib.sha256(sums_raw).hexdigest()
        sums = {}
        for line in sums_raw.decode('ascii').splitlines():
            digest, name = line.split('  ')
            require(name not in sums, 'unique SHA256SUMS entry')
            sums[name] = digest
        require(sums == {name: value[0] for name, value in bundle_files.items()}, 'bundle checksum declarations')
        for name, (_, size) in source_files.items():
            stat = (srcdir / name).stat()
            require((srcdir / name).is_file() and stat.st_size == size, 'source presence/size')
        same_inode = 0
        for name, value in bundle_files.items():
            require(source_files[name] == value, 'source/bundle expected hash and size')
            source_stat, bundle_stat = (srcdir / name).stat(), (bdir / name).stat()
            require((bdir / name).is_file() and bundle_stat.st_size == value[1], 'bundle presence/size')
            same_inode += (source_stat.st_dev, source_stat.st_ino) == (bundle_stat.st_dev, bundle_stat.st_ino)
        require(same_inode == len(bundle_files), 'hard links source to bundle')
        expected_source = ' : %d cas, %d fichiers, 0 ecarts' % (len(allrows), len(source_files))
        expected_bundle = ' : %d cas, %d fichiers, 0 ecarts' % (len(allrows), len(bundle_files))
        require('verify_inputs ' + str(srcdir / 'manifest.json') + expected_source in journal, 'source verification log')
        require('verify_inputs ' + str(bdir / 'bundle_manifest.json') + expected_bundle in journal, 'bundle verification log')
        require('verify ' + family + ' code=0' in journal and 'verify bundle ' + family + ' code=0' in journal
                and 'bundle ' + family + ' code=0' in journal, 'successful preparation stages')
        observed[family] = dict(scenes=len(parents), crops=len(src['crops']), source_payloads=len(source_files),
                               bundle_payloads=len(bundle_files), bundle_bytes=bundle['bytes'],
                               same_inode_links=same_inode, source_log_gaps=0, bundle_log_gaps=0)
    require(seen_crops == set(tables_current), 'all 69 crops')
    indexed = {row['name']: row for row in crop_rows}
    for take in transferred:
        row = indexed[take['case']]
        require(row['sha256'] == take['sha256'] and row['ids_sha256'] == take['ids_sha256']
                and row['count'] == take['sites'], 'native transfer to full manifests')
    require(hashlib.sha256(journal_path.read_bytes()).hexdigest() == local_pins['preparer.log'], 'journal stable')
    for label, digest in local_pins.items():
        require(hashlib.sha256((root / label).read_bytes()).hexdigest() == digest, 'external metadata stable')
    return dict(manifests=8, families=observed, crops=sorted(crop_rows, key=lambda r: r['name']),
                metadata_sha256=local_pins, payload_reads=0, payload_rehashes=0, geometry_recomputed=False,
                corrected_crop_log_records=69, generator_scripts_matched=4, generator_libraries_matched=14,
                qualification='metadata_links_and_preparation_log_only')


if __name__ == '__main__':
    main()
