"""Read-only verification of existing whole-frame/capteur partitions.

No KITTI bytes are written or copied. This validates the represented grid
and mappings, not the semantic correctness of the frozen ground mask.
"""
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[3]
GROUND = ROOT / 'morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6'
RAW = ROOT / 'morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i'
PARTS = ('full', 'half_x_neg', 'half_x_nonneg', 'quarter_x_neg_y_neg',
         'quarter_x_neg_y_nonneg', 'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')
RELATIONS = (('full', 'half_x_neg'), ('full', 'half_x_nonneg'),
             ('half_x_neg', 'quarter_x_neg_y_neg'), ('half_x_neg', 'quarter_x_neg_y_nonneg'),
             ('half_x_nonneg', 'quarter_x_nonneg_y_neg'), ('half_x_nonneg', 'quarter_x_nonneg_y_nonneg'))


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read(path):
    def pairs(items):
        result = {}
        for key, value in items:
            need(key not in result, 'duplicate JSON key')
            result[key] = value
        return result
    return json.loads(Path(path).read_text(), object_pairs_hook=pairs)


def points(path):
    raw = Path(path).read_bytes()
    need(len(raw) % 12 == 0, 'incomplete XYZ record')
    value = tuple(struct.iter_unpack('<III', raw))
    need(len(set(value)) == len(value) and all(max(p) <= 262143 for p in value), 'u18 site domain/uniqueness')
    return value


def words(path):
    raw = Path(path).read_bytes()
    need(len(raw) % 4 == 0, 'incomplete ID word')
    return tuple(x[0] for x in struct.iter_unpack('<I', raw))


def point_hash(value):
    """FNV identity of this probe: version=1, n, XYZ as u64LE words."""
    digest = 14695981039346656037
    for word in (1, len(value), *(x for point in value for x in point)):
        for byte in struct.pack('<Q', word):
            digest = ((digest ^ byte) * 1099511628211) & ((1 << 64)-1)
    return f'{digest:016x}'


def dataset(directory, part, entry, pins):
    path = directory / entry['points_file']
    need(path.name == part + '.u32le' and path.parent == directory and not path.is_symlink(), 'dataset path')
    need(sha(path) == entry['points_sha256'], 'dataset point pin')
    value = points(path)
    need(len(value) == entry['sites'] and path.stat().st_size == entry['point_bytes'], 'dataset size')
    pins[str(path)] = sha(path)
    return value, dict(path=str(path), n=len(value), sha256=pins[str(path)],
                      point_hash=point_hash(value), profile='quantized_u18_1mm_frozen_input', part=part)


def grounded(scene):
    need(scene in ('00', '01', '02'), 'unknown scene')
    directory = GROUND / ('scene_' + scene + '_grid')
    manifest_path = directory / 'MANIFEST.json'
    manifest = read(manifest_path)
    need(manifest['parameters'] == {'precision_mm': '1', 'profile': 'grid'}, '1mm grid required')
    partition = manifest['partition']
    need(partition['subsampling'] is False and partition['inherited_original_partitions'] is True and
         partition['mask_scope'] == 'whole_original_frame_before_spatial_partition', 'whole-mask partition contract')
    need(partition['planes'] == [{'axis': 'x', 'value_metres': 0}, {'axis': 'y', 'value_metres': 0}], 'sensor planes')
    origin = partition['encoded_sensor_origin']
    need(len(origin) == 3 and all(type(x) is int for x in origin), 'encoded common sensor origin')
    need(set(manifest['datasets']) == set(PARTS), 'all seven pieces required')
    pins = {str(manifest_path): sha(manifest_path)}
    clouds, ids, originals, metadata = {}, {}, {}, {}
    for part in PARTS:
        entry = manifest['datasets'][part]
        clouds[part], metadata[part] = dataset(directory, part, entry, pins)
        metadata[part].update(scene=scene, ground='nonground', sequence='08')
        for stem, destination in (('site_ids', ids), ('original_site_ids', originals)):
            path = directory / entry[stem + '_file']
            need(path.name == part + '.' + stem + '.u32le' and not path.is_symlink(), 'ID map path')
            need(sha(path) == entry[stem + '_sha256'], 'ID map pin')
            destination[part] = words(path)
            need(len(destination[part]) == len(clouds[part]) and
                 all(a < b for a, b in zip(destination[part], destination[part][1:])), 'increasing IDs')
            pins[str(path)] = sha(path)
    need(ids['full'] == tuple(range(len(clouds['full']))), 'full retained-site IDs')
    for part in PARTS:
        expected = tuple(i for i, p in enumerate(clouds['full'])
                         if (part == 'full' or ((p[0] < origin[0]) == ('x_neg' in part))) and
                            ('quarter' not in part or ((p[1] < origin[1]) == part.endswith('y_neg'))))
        need(ids[part] == expected, 'exact sensor piece membership')
        need(clouds[part] == tuple(clouds['full'][i] for i in expected), 'point identity across pieces')
        need(originals[part] == tuple(originals['full'][i] for i in expected), 'original identity across pieces')
    for parent, children in (('full', PARTS[1:3]), ('half_x_neg', PARTS[3:5]), ('half_x_nonneg', PARTS[5:7])):
        need(set(ids[children[0]]).isdisjoint(ids[children[1]]) and
             set(ids[parent]) == set(ids[children[0]]) | set(ids[children[1]]), 'disjoint complete spatial split')
    return metadata, pins, dict(scene=scene, sequence='08', origin=origin,
                               n={part: len(clouds[part]) for part in PARTS},
                               all_seven_pieces_verified=True, no_subsampling=True,
                               ground_quality_judged=False, mask_recomputed=False,
                               raw_grid_boundary_changes=manifest['raw_preparation']['boundaries'])


def raw_full():
    directory = RAW / 'scene_00_000000_grid'
    manifest_path = directory / 'MANIFEST.json'
    manifest = read(manifest_path)
    need(manifest['parameters'] == {'precision_mm': '1', 'profile': 'grid'}, 'raw 1mm grid required')
    pins = {str(manifest_path): sha(manifest_path)}
    _, metadata = dataset(directory, 'full', manifest['datasets']['full'], pins)
    metadata.update(scene='b00', ground='raw', sequence='08')
    return metadata, pins
