#!/usr/bin/env python3
"""Pente LiDAR locale de la chaine v9 : sous-nuages emboites et sept morceaux.

Entrees : les trames sans sol a 1 mm du recu v8 lidar_ground_20260921 (deja
dans le depot, jamais recopiees ici). Pour chaque trame demandee :
- sous-nuages EMBOITES de 8 000, 16 000 et 32 000 sites : sites tries par
  distance horizontale au centre median (x, y), puis par rang d'entree ; ce
  sont des disques croissants autour du centre, PAS des coupes par plans du
  capteur ;
- les sept morceaux figes du recu v8 (entiere, deux moities, quatre quarts,
  coupes par plans du capteur), controles contre les empreintes de son
  MANIFEST.json.
Chaque nuage derive est ecrit dans un repertoire de travail hors depot (octets
KITTI derives, jamais versionnes) ; ses identifiants de sites retenus sont
epingles (SHA-256) et l'emboitement 8k < 16k < 32k est verifie. Chaque cas
passe par mhgp9_tower_probe (K, W, s, leviers par defaut) et n'est accepte
que complet : statut complete_relative, K_effective = K, ordres 1..K,
options conformes, schema de sonde attendu, et identite d'entree liee aux
octets fournis : FNV-1a 64 de la sonde recalcule ici, format u32le, grille
`1mm` passee explicitement, fils de la tour statique passes explicitement.
Les morceaux v8 sont controles sur leurs points ET leurs IDs de sites. Un
echec (y compris un JSON de code 0 mal forme) laisse un recu type (commande,
code, mur, raison, stdout/stderr) et arrete la campagne.

  run_lidar_scaling.py --probe build/v9-dev/mhgp9_tower_probe --scene 00 \\
      --k 5 --workers 8 --repeat 0 --work DIR --out DIR
  run_lidar_scaling.py --selftest CASE.json      # porte du lecteur (mutants)
  run_lidar_scaling.py --revalidate OUT_DIR       # rejuge une campagne archivee

La sortie `--out` doit etre vide (jamais d'ecrasement silencieux) ; l'identite
de chaque cas porte trame, K, s, W et repetition. Diagnostic de croissance
seulement (CLAUDE.md, TEST_PLAN 3.1) : ni le contrat de trame entiere, ni une
mesure G4. Aucun assert : tient sous python3 -O.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
V8 = ROOT / 'morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6'
PROBE_SCHEMA = 'mhgp9_tower_probe_v13'
# Schemas relus lors d'une revalidation d'archive (v12 : reçu du 23 septembre).
KNOWN_SCHEMAS = ('mhgp9_tower_probe_v12', PROBE_SCHEMA)
# Le schema de sonde d'une campagne est fixe par son RESUME, jamais par le JSON
# de cas relu (contre-audit B) : un resume v2 est une campagne de sonde v12, un
# resume v3 porte `probe_schema`.
SUMMARY_SCHEMA = 'mhgp9_lidar_scaling_v3'
SUMMARY_PROBE_SCHEMAS = {'mhgp9_lidar_scaling_v2': 'mhgp9_tower_probe_v12'}
PIECES = ('full', 'half_x_neg', 'half_x_nonneg', 'quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
          'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')
NESTED = (8000, 16000, 32000)
GRID = '1mm'
INPUT_KEYS = frozenset({'format', 'grid', 'sites', 'hash'})
FNV_PRIME = 1099511628211
FNV_MASK = (1 << 64) - 1
DEFAULT_LEVERS = dict(atlas_saturate_deep=True, q3_leaf_census=True, q34_dead_lanes=True, q34_witness_cache=True,
                      q34_dead_core=True, tower_meb_proposal=True)
# Travail publie : front, filtres (visites des DFS de temoins), covers et
# noyaux, preuves, graines et DFS q3/q4, atlas, balayages q4, catalogue, tour.
WORK_KEYS = (
    ('generator', 'q2_candidate_pairs'), ('generator', 'q2_accepted_pairs'),
    ('ledger', 'q34_input_rectangles'), ('ledger', 'witness_input_pair_mass'), ('ledger', 'witness_rect_node_visits'),
    ('ledger', 'expanded_pairs'), ('ledger', 'witness_pair_queries'), ('ledger', 'witness_pair_node_visits'),
    ('ledger', 'witness_cache_node_tests'), ('ledger', 'core_builds'), ('ledger', 'core_sites'),
    ('ledger', 'core_cover_node_visits'), ('ledger', 'cover_builds'), ('ledger', 'cover_sites'),
    ('ledger', 'cover_node_visits'), ('ledger', 'dead_core_uniform_tests'), ('ledger', 'dead_uniform_tests'),
    ('ledger', 'q3_edges'), ('ledger', 'q4_edges'), ('ledger', 'q3_seed_node_visits'), ('ledger', 'q3_seeds'),
    ('ledger', 'q3_census_point_tests'), ('ledger', 'q3_leaf_point_tests'), ('ledger', 'q4_domain_node_visits'),
    ('ledger', 'q4_cover_decomposition_node_visits'), ('ledger', 'q4_seed_node_visits'), ('ledger', 'q4_seeds'),
    ('ledger', 'q4_sweep_active_sites'), ('ledger', 'q4_sweep_events'), ('ledger', 'atlas_node_visits'),
    ('ledger', 'atlas_point_tests'), ('generator', 'q3_emitted'), ('generator', 'q4_emitted'),
    ('catalogue', 'balls'), ('tower_work', 'meb_calls'), ('tower_work', 'intruder_nodes'),
    ('tower_work', 'contributions'))


class Refusal(Exception):
    pass


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def read_u32(path, width):
    raw = path.read_bytes()
    if len(raw) % (4 * width):
        raise Refusal('not a u32le file of width %d: %s' % (width, path))
    return raw, [struct.unpack_from('<%dI' % width, raw, i) for i in range(0, len(raw), 4 * width)]


def input_fnv(raw):
    """FNV-1a 64 de mhgp9_tower_probe : n puis x, y, z en u64 LE (u32le seulement)."""
    if len(raw) % 12:
        raise Refusal('u32le input with an incomplete site record')
    h = 14695981039346656037
    zero5 = pow(FNV_PRIME, 5, 1 << 64)
    for byte in (len(raw) // 12).to_bytes(8, 'little'):
        h = ((h ^ byte) * FNV_PRIME) & FNV_MASK
    for (value,) in struct.iter_unpack('<I', raw):
        if value >= 1 << 18:
            raise Refusal('coordinate outside [0, 2^18)')
        h = ((h ^ (value & 255)) * FNV_PRIME) & FNV_MASK
        h = ((h ^ ((value >> 8) & 255)) * FNV_PRIME) & FNV_MASK
        h = ((h ^ (value >> 16)) * FNV_PRIME) & FNV_MASK
        h = (h * zero5) & FNV_MASK  # cinq octets nuls de poids fort
    return '%016x' % h


def portable(argument):
    """Chemin archive relatif a la racine du depot quand il y est (jamais le worktree de l'auteur)."""
    path = Path(argument)
    if path.is_absolute():
        try:
            return str(path.relative_to(ROOT))
        except ValueError:
            return argument
    return argument


def resolve_input(argument):
    """Entree d'un cas archive, re-ancree sur ce depot : un chemin absolu d'un autre checkout est
    relu a partir de son segment morsehgp3D_v8/ ; sinon il est refuse."""
    path = Path(argument)
    if not path.is_absolute():
        return ROOT / path
    parts = path.parts
    if 'morsehgp3D_v8' not in parts:
        raise Refusal('archived input path outside this repository: ' + argument)
    return ROOT.joinpath(*parts[parts.index('morsehgp3D_v8'):])


def nested_indices(points, size):
    xs = sorted(p[0] for p in points)
    ys = sorted(p[1] for p in points)
    cx, cy = xs[len(xs) // 2], ys[len(ys) // 2]
    order = sorted(range(len(points)), key=lambda i: ((points[i][0] - cx) ** 2 + (points[i][1] - cy) ** 2, i))
    return sorted(order[:size])


def number(value):
    return type(value) in (int, float) and math.isfinite(value) and value >= 0


def validate_probe(value, expected):
    """Vrai si la sortie est une tour complete de la sonde attendue, sur l'entree annoncee.

    `expected` : k, s, workers, static_threads, sites, fnv, grid, et le schema
    (v13 pour une mesure neuve ; celui de l'archive pour une revalidation). En
    v13, l'invariant d'Euler doit tenir sur les ordres min(K-2, n).
    """
    if type(value) is not dict:
        return False
    sites = expected['sites']
    source = value.get('input')
    options = value.get('options')
    orders = value.get('orders')
    times = value.get('times_ms')
    if type(source) is not dict or type(options) is not dict or type(orders) is not list or type(times) is not dict:
        return False
    schema = expected.get('schema', PROBE_SCHEMA)
    need = [schema in KNOWN_SCHEMAS, value.get('schema') == schema, value.get('status') == 'complete_relative',
            set(source) == INPUT_KEYS, source.get('format') == 'u32le', source.get('grid') == expected['grid'],
            source.get('sites') == sites, source.get('hash') == expected['fnv']]
    need += [options.get('K') == expected['k'], options.get('K_effective') == min(expected['k'], sites),
             options.get('s') == expected['s'], options.get('workers') == expected['workers'],
             options.get('tower_static_threads') == expected['static_threads'], options.get('run_tower') is True,
             options.get('levers') == DEFAULT_LEVERS]
    need += [[order.get('K') if type(order) is dict else None for order in orders] ==
             list(range(1, min(expected['k'], sites) + 1))]
    need += [all(type(value.get(section)) is dict and type(value[section].get(key)) is int for section, key in WORK_KEYS)]
    need += [type(value.get('tower_digest')) is str and len(value['tower_digest']) == 16,
             number(times.get('chain_total')), number(times.get('digest')), number(value.get('chain_cpu_s')),
             type(value.get('peak_rss_kb')) is int and value['peak_rss_kb'] > 0]
    if schema != 'mhgp9_tower_probe_v12':
        catalogue = value.get('catalogue')
        euler = catalogue.get('euler') if type(catalogue) is dict else None
        checkable = min(expected['k'] - 2, sites) if expected['k'] >= 3 else 0
        need += [type(euler) is dict and set(euler) == {'status', 'checkable_max_k', 'by_k'} and
                 euler['checkable_max_k'] == checkable and
                 euler['status'] == ('holds' if checkable else 'not_checkable') and
                 type(euler['by_k']) is list and len(euler['by_k']) == expected['k'] and
                 all(type(x) is int for x in euler['by_k']) and euler['by_k'][:checkable] == [1] * checkable]
    return all(need)


def row_of(name, sites, value):
    row = dict(case=name, sites=sites, digest=value['tower_digest'],
               chain_s=round(value['times_ms']['chain_total'] / 1000, 3),
               digest_s=round(value['times_ms']['digest'] / 1000, 3), cpu_s=value['chain_cpu_s'],
               peak_rss_kb=value['peak_rss_kb'])
    for section, key in WORK_KEYS:
        row[key] = value[section][key]
    return row


def run_case(args, name, path, provenance, out_dir, sites):
    argv = [str(args.probe), str(path), str(args.k), str(args.workers), '--s=' + str(args.s),
            '--static=' + str(args.workers), '--grid=' + GRID]
    expected = dict(k=args.k, s=args.s, workers=args.workers, static_threads=args.workers, sites=sites,
                    fnv=input_fnv(path.read_bytes()), grid=GRID)
    started = time.monotonic()
    completed = subprocess.run(argv, capture_output=True, text=True)
    elapsed = time.monotonic() - started
    text = completed.stdout
    begin, end = text.find('{'), text.rfind('}')
    record = dict(case=name, argv=[portable(a) for a in argv[1:]], exit_code=completed.returncode,
                  external_wall_s=round(elapsed, 3),
                  input=provenance)
    try:
        if completed.returncode != 0 or begin < 0:
            raise Refusal('probe exit code %d' % completed.returncode)
        value = json.loads(text[begin:end + 1])
        if not validate_probe(value, expected):
            raise Refusal('probe output not a complete tower with the expected input/options/schema')
        row = row_of(name, sites, value)
    except (Refusal, ValueError, KeyError, TypeError) as error:
        record.update(outcome='refused', reason=str(error), stdout=text[-4000:], stderr=completed.stderr[-4000:])
        (out_dir / (name + '.failure.json')).write_text(json.dumps(record, indent=1, sort_keys=True) + '\n')
        raise Refusal(name + ': ' + str(error))
    record.update(outcome='complete_relative', probe=value)
    (out_dir / (name + '.json')).write_text(json.dumps(record, indent=1, sort_keys=True) + '\n')
    return row


def slopes(rows, key):
    values = [(r['sites'], r[key]) for r in rows]
    return [round(math.log(b[1] / a[1]) / math.log(b[0] / a[0]), 3) if a[1] > 0 and b[1] > 0 else None
            for a, b in zip(values, values[1:])]


def check_piece(grid, data):
    """Points ET IDs d'un morceau v8 contre son MANIFEST ; rend (chemin, octets des points)."""
    path = grid / data['points_file']
    raw = path.read_bytes()
    id_raw = (grid / data['site_ids_file']).read_bytes()
    if sha(raw) != data['points_sha256'] or len(raw) != 12 * data['sites'] or \
            sha(id_raw) != data['site_ids_sha256'] or len(id_raw) != 4 * data['sites']:
        raise Refusal('piece differs from its v8 manifest: ' + data['points_file'])
    return path, raw


def scene_cases(scene, tag, work):
    """Cas d'une trame : emboites (ecrits dans `work`) puis morceaux v8 ; tout est controle."""
    grid = V8 / ('scene_' + scene + '_grid')
    manifest_raw = (grid / 'MANIFEST.json').read_bytes()
    manifest = json.loads(manifest_raw)
    full_raw, full = read_u32(grid / 'full.u32le', 3)
    ids_raw, ids = read_u32(grid / 'full.site_ids.u32le', 1)
    entry = manifest['datasets']['full']
    if sha(full_raw) != entry['points_sha256'] or sha(ids_raw) != entry['site_ids_sha256'] or \
            len(full) != entry['sites'] or len(ids) != len(full):
        raise Refusal('full frame differs from its v8 manifest')
    cases = []
    previous = None
    for size in NESTED:
        if size > len(full):
            continue
        chosen = nested_indices(full, size)
        if previous is not None and not set(previous) <= set(chosen):
            raise Refusal('nested subsets are not nested')
        previous = chosen
        name = tag + '_nested_%d' % size
        path = work / (name + '.u32le')
        raw = b''.join(struct.pack('<3I', *full[i]) for i in chosen)
        path.write_bytes(raw)
        id_raw = b''.join(struct.pack('<I', ids[i][0]) for i in chosen)
        cases.append((name, size, path, raw, dict(kind='nested_disc', parent_points_sha256=entry['points_sha256'],
                                                  points_sha256=sha(raw), site_ids_sha256=sha(id_raw), sites=size)))
    for piece in PIECES:
        data = manifest['datasets'][piece]
        path, raw = check_piece(grid, data)
        cases.append((tag + '_piece_' + piece, data['sites'], path, raw,
                      dict(kind='v8_sensor_piece', piece=piece, points_sha256=data['points_sha256'],
                           site_ids_sha256=data['site_ids_sha256'], sites=data['sites'])))
    return manifest_raw, manifest, cases


def expected_from_argv(argv, sites, raw):
    """Attentes d'un cas archive, tirees de sa ligne de commande (defauts de la sonde si absents)."""
    k, workers = int(argv[1]), int(argv[2])
    flags = dict(a.split('=', 1) for a in argv[3:] if a.startswith('--') and '=' in a)
    static = int(flags['--static']) if '--static' in flags else (workers if workers > 1 else 0)
    return dict(k=k, s=int(flags.get('--s', 8)), workers=workers, static_threads=static, sites=sites,
                fnv=input_fnv(raw), grid=flags.get('--grid', 'unspecified'))


def campaign_probe_schema(summary):
    """Schema de sonde d'une campagne, fixe par son resume."""
    if summary.get('schema') == SUMMARY_SCHEMA and summary.get('probe_schema') in KNOWN_SCHEMAS:
        return summary['probe_schema']
    if summary.get('schema') in SUMMARY_PROBE_SCHEMAS and 'probe_schema' not in summary:
        return SUMMARY_PROBE_SCHEMAS[summary['schema']]
    raise Refusal('campaign summary schema unknown: ' + str(summary.get('schema')))


def archived_expectations(record, sites, raw, schema):
    """Attentes d'un cas archive : sa commande et le schema de SA campagne."""
    return dict(expected_from_argv(record['argv'], sites, raw), schema=schema)


def mutants(value):
    """Sorties alterees qui doivent toutes etre refusees par validate_probe."""
    def edit(path, new):
        def apply(v):
            target = v
            for key in path[:-1]:
                target = target[key]
            if new is KeyError:
                del target[path[-1]]
            else:
                target[path[-1]] = new
        return apply
    rows = [
        ('input_hash_zero', edit(('input', 'hash'), '0000000000000000')),
        ('input_format_u16le', edit(('input', 'format'), 'u16le')),
        ('input_grid_other', edit(('input', 'grid'), 'other')),
        ('static_threads_zero', edit(('options', 'tower_static_threads'), 0)),
        ('input_extra_key', edit(('input', 'extra'), 1)),
        ('input_sites_minus_one', edit(('input', 'sites'), value['input']['sites'] - 1)),
        ('status_resource', edit(('status',), 'resource_exhausted')),
        ('schema_v11', edit(('schema',), 'mhgp9_tower_probe_v11')),
        ('schema_other_known', edit(('schema',), [x for x in KNOWN_SCHEMAS if x != value['schema']][0])),
        ('k_effective_minus_one', edit(('options', 'K_effective'), value['options']['K_effective'] - 1)),
        ('s_ten', edit(('options', 's'), 10)),
        ('workers_one', edit(('options', 'workers'), 1)),
        ('no_tower', edit(('options', 'run_tower'), False)),
        ('lever_off', edit(('options', 'levers', 'q34_dead_core'), False)),
        ('last_order_missing', lambda v: v['orders'].pop()),
        ('ledger_key_missing', edit(('ledger', 'core_sites'), KeyError)),
        ('ledger_key_float', edit(('ledger', 'core_sites'), 1.5)),
        ('digest_missing', edit(('tower_digest',), KeyError)),
        ('chain_total_missing', edit(('times_ms', 'chain_total'), KeyError)),
        ('digest_time_string', edit(('times_ms', 'digest'), '12')),
        ('cpu_negative', edit(('chain_cpu_s',), -1.0)),
        ('rss_missing', edit(('peak_rss_kb',), KeyError)),
        ('times_not_object', edit(('times_ms',), [])),
    ]
    return rows


def selftest(case_path):
    """Porte du lecteur sur un cas archive dont l'entree est un morceau v8 versionne."""
    record = json.loads(case_path.read_text())
    value = record['probe']
    argv = record['argv']
    raw = resolve_input(argv[0]).read_bytes()
    summaries = sorted(case_path.parent.glob('SUMMARY_*.json'))
    if len(summaries) != 1:
        raise Refusal('the archived case needs exactly one campaign summary beside it')
    expected = archived_expectations(record, record['input']['sites'], raw,
                                     campaign_probe_schema(json.loads(summaries[0].read_text())))
    if not validate_probe(value, expected):
        print('lidar_scaling_selftest cause=baseline_refused')
        return 1
    killed = 0
    table = mutants(value)
    for name, apply in table:
        mutated = json.loads(json.dumps(value))
        apply(mutated)
        if validate_probe(mutated, expected):
            print('lidar_scaling_selftest survivor=' + name)
        else:
            killed += 1
    # Variante v13 du meme cas (bloc Euler ajoute) dans une campagne v3 : le
    # schema attendu vient du resume, une retrogradation en v12 ne contourne
    # pas Euler ; longueur, types et statut de by_k sont exiges.
    k = expected['k']
    checkable = min(k - 2, expected['sites']) if k >= 3 else 0
    v13 = json.loads(json.dumps(value))
    v13['schema'] = PROBE_SCHEMA
    v13['catalogue']['euler'] = dict(status='holds' if checkable else 'not_checkable', checkable_max_k=checkable,
                                     by_k=[1] * checkable + [5] * (k - checkable))
    expected13 = dict(expected, schema=campaign_probe_schema(dict(schema=SUMMARY_SCHEMA, probe_schema=PROBE_SCHEMA)))
    if not validate_probe(v13, expected13):
        print('lidar_scaling_selftest cause=v13_baseline_refused')
        return 1
    table13 = [
        ('v13_downgraded_to_v12', lambda v: v.update(schema='mhgp9_tower_probe_v12')),
        ('v13_euler_short', lambda v: v['catalogue']['euler']['by_k'].pop()),
        ('v13_euler_text', lambda v: v['catalogue']['euler']['by_k'].__setitem__(k - 1, '5')),
        ('v13_euler_sum', lambda v: v['catalogue']['euler']['by_k'].__setitem__(0, 2)),
        ('v13_euler_vacuous', lambda v: v['catalogue']['euler'].update(status='not_checkable')),
        ('v13_euler_absent', lambda v: v['catalogue'].pop('euler')),
        ('v13_euler_extra_key', lambda v: v['catalogue']['euler'].update(extra=1)),
    ]
    for name, apply in table13:
        mutated = json.loads(json.dumps(v13))
        apply(mutated)
        if validate_probe(mutated, expected13):
            print('lidar_scaling_selftest survivor=' + name)
        else:
            killed += 1
    if campaign_probe_schema(dict(schema='mhgp9_lidar_scaling_v2')) != 'mhgp9_tower_probe_v12':
        print('lidar_scaling_selftest survivor=v2_campaign_schema')
    else:
        killed += 1
    try:
        campaign_probe_schema(dict(schema='mhgp9_lidar_scaling_v2', probe_schema=PROBE_SCHEMA))
        print('lidar_scaling_selftest survivor=v2_campaign_claims_v13')
    except Refusal:
        killed += 1
    # Entree alteree d'un octet : le FNV recalcule ne correspond plus.
    changed = bytearray(raw)
    changed[0] ^= 1
    total = len(table) + 5 + 7 + 2
    if not validate_probe(value, dict(expected, fnv=input_fnv(bytes(changed)))):
        killed += 1
    else:
        print('lidar_scaling_selftest survivor=input_byte_flipped')
    # Chemin archive depuis un autre checkout : re-ancre sur ce depot (memes
    # octets) ; hors d'un segment morsehgp3D_v8/, refuse.
    tail = Path(argv[0]).parts
    tail = tail[tail.index('morsehgp3D_v8'):] if 'morsehgp3D_v8' in tail else tail
    if resolve_input(str(Path('/elsewhere/checkout').joinpath(*tail))).read_bytes() == raw:
        killed += 1
    else:
        print('lidar_scaling_selftest survivor=foreign_absolute_path')
    try:
        resolve_input('/elsewhere/checkout/other/' + Path(argv[0]).name)
        print('lidar_scaling_selftest survivor=path_outside_repository')
    except Refusal:
        killed += 1
    # Morceau v8 dont l'empreinte des IDs est fausse : refuse avant calcul.
    grid = resolve_input(argv[0]).parent
    manifest = json.loads((grid / 'MANIFEST.json').read_text())
    piece = record['input']['piece']
    data = dict(manifest['datasets'][piece])
    try:
        check_piece(grid, data)
        data['site_ids_sha256'] = '0' * 64
        check_piece(grid, data)
        print('lidar_scaling_selftest survivor=piece_ids_sha')
    except Refusal:
        killed += 1
    # JSON de code 0 mal forme : row_of leve, run_case le convertit en refus type.
    try:
        broken = json.loads(json.dumps(value))
        del broken['times_ms']
        row_of('x', expected['sites'], broken)
        print('lidar_scaling_selftest survivor=row_of_malformed')
    except (KeyError, TypeError):
        killed += 1
    print('lidar_scaling_selftest mutants_killed=%d/%d' % (killed, total))
    return 0 if killed == total else 1


def revalidate(out_dir, work):
    """Rejuge une campagne archivee : entrees reconstruites, FNV, lecteur, lignes du resume."""
    report = dict(schema='mhgp9_lidar_scaling_revalidation_v1', cases=0, campaigns=[], failures=[])
    for summary_path in sorted(out_dir.rglob('SUMMARY_*.json')):
        summary = json.loads(summary_path.read_text())
        tag = summary_path.stem[len('SUMMARY_'):]
        bound = 's%s_k%d_s%d_w%d_r%d' % (summary['scene'], summary['k'], summary['s'], summary['workers'],
                                         summary['repeat'])
        if tag != bound:
            report['failures'].append(dict(case=tag, reasons=['summary_parameters_' + bound]))
        _, _, cases = scene_cases(summary['scene'], tag, work)
        schema = campaign_probe_schema(summary)
        rows = {row['case']: row for row in summary['rows']}
        checked = 0
        for name, size, _path, raw, provenance in cases:
            record = json.loads((summary_path.parent / (name + '.json')).read_text())
            expected = archived_expectations(record, size, raw, schema)
            reasons = []
            if record.get('outcome') != 'complete_relative' or record.get('exit_code') != 0:
                reasons.append('outcome')
            if record.get('input') != provenance:
                reasons.append('provenance')
            if (expected['k'], expected['s'], expected['workers']) != (summary['k'], summary['s'], summary['workers']):
                reasons.append('command_parameters')
            if not validate_probe(record.get('probe'), expected):
                reasons.append('probe_output')
            elif row_of(name, size, record['probe']) != rows.get(name):
                reasons.append('summary_row')
            if reasons:
                report['failures'].append(dict(case=name, reasons=reasons))
            checked += 1
        if checked != len(rows):
            report['failures'].append(dict(case=tag, reasons=['summary_rows_%d_cases_%d' % (len(rows), checked)]))
        report['cases'] += checked
        report['campaigns'].append(dict(tag=tag, cases=checked, summary_sha256=sha(summary_path.read_bytes())))
    print(json.dumps(report, indent=1, sort_keys=True))
    return 0 if report['cases'] > 0 and not report['failures'] else 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--selftest', type=Path)
    parser.add_argument('--revalidate', type=Path)
    parser.add_argument('--probe', type=Path)
    parser.add_argument('--scene', choices=('00', '01', '02'))
    parser.add_argument('--k', type=int, choices=(5, 10))
    parser.add_argument('--workers', type=int, default=8)
    parser.add_argument('--s', type=int, default=8)
    parser.add_argument('--repeat', type=int, default=0)
    parser.add_argument('--work', type=Path)
    parser.add_argument('--out', type=Path)
    args = parser.parse_args()
    try:
        if args.selftest is not None:
            return selftest(args.selftest)
        if args.work is None:
            raise Refusal('--work is required')
        args.work.mkdir(parents=True, exist_ok=True)
        if args.revalidate is not None:
            return revalidate(args.revalidate, args.work)
        if args.probe is None or args.scene is None or args.k is None or args.out is None:
            raise Refusal('--probe, --scene, --k and --out are required')
        if args.s < 8:
            raise Refusal('s < 8 is never measured')
        if args.out.exists() and any(args.out.iterdir()):
            raise Refusal('output directory is not empty: ' + str(args.out))
        args.out.mkdir(parents=True, exist_ok=True)
        tag = 's%s_k%d_s%d_w%d_r%d' % (args.scene, args.k, args.s, args.workers, args.repeat)
        manifest_raw, manifest, cases = scene_cases(args.scene, tag, args.work)
        probe_sha = sha(args.probe.read_bytes())
        head = subprocess.run(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
        trees = {name: subprocess.run(['git', '-C', str(ROOT), 'rev-parse', 'HEAD:morsehgp3D_v9/' + name],
                                      capture_output=True, text=True).stdout.strip() for name in ('src', 'bench')}
        dirty = subprocess.run(['git', '-C', str(ROOT), 'status', '--porcelain', '--', 'morsehgp3D_v9/src',
                                'morsehgp3D_v9/bench'], capture_output=True, text=True).stdout
        rows = []
        for name, size, path, _raw, provenance in cases:
            rows.append(run_case(args, name, path, provenance, args.out, size))
        nested_rows = sorted((r for r in rows if '_nested_' in r['case']), key=lambda r: r['sites'])
        summary = dict(schema=SUMMARY_SCHEMA, probe_schema=PROBE_SCHEMA, scene=args.scene, k=args.k,
                       workers=args.workers, s=args.s, repeat=args.repeat, public_status='not_claimed',
                       probe_sha256=probe_sha, git_head=head,
                       git_trees=trees, worktree_dirty_src_or_bench=bool(dirty.strip()),
                       v8_manifest_sha256=sha(manifest_raw), mask=manifest.get('mask'), profile=manifest.get('profile'),
                       rows=rows, nested_slopes={key: slopes(nested_rows, key)
                                                 for key in [k for _, k in WORK_KEYS] + ['cpu_s', 'chain_s']})
        (args.out / ('SUMMARY_' + tag + '.json')).write_text(json.dumps(summary, indent=1) + '\n')
        print(json.dumps(dict(case=tag, nested_slopes=summary['nested_slopes']), sort_keys=True))
        return 0
    except Refusal as error:
        print('refused: ' + str(error), file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
