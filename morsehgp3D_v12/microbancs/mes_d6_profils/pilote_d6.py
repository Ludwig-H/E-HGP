#!/usr/bin/env python3
"""MES-D6 : cout des profils de coordonnees u24 et u32 face a u21 (decision D6, constat CST-0207), sur trames reelles.

Mesure publiee, sans regle d'adoption : la v12 vise au moins u21, puis u24 et u32 ; il faut savoir ce que coute un
profil elargi sur les memes trames. Le pilote construit le produit a chaque profil demande (cibles
mhgp12_catalogue_probe et mhgp12_tower_probe, Release), puis joue, par tours alternes, pour chaque trame et chaque
couple (profil, dilatation) admissible : la sonde du catalogue puis celle de l'etage G de la tour, P passes dans un
processus neuf. Dilatations : facteur 1 (memes coordonnees), 8 (trois bits de plus, une grille 8 fois plus fine) et
2048 (onze bits de plus), retenues seulement si la plus grande coordonnee dilatee tient dans le profil.

Controles (jamais un temps) :
  - identite : a facteur 1, le catalogue et la resolution sont les memes aux trois profils (meme objet, meme entree) :
    empreinte de la resolution (en-tete exclu par la sonde) et SHA-256 de l'export du catalogue PRIVE de son en-tete de
    64 octets (qui porte les bits du profil), export fait au premier tour seulement puis efface ;
  - invariance par dilatation : les compteurs de l'objet de chaque ordre (naissances, cellules, cellules inertes et
    etendues, representants) et le nombre de boules et d'incidences du catalogue ne changent pas (tous les predicats
    sont homogenes, l'ordre des positions est conserve) ;
  - catalogue : une empreinte par passe, constante ; G : une empreinte de la derniere passe seulement.
Temps : mediane des passes 2..P de chaque prise ; par tour, rapport au couple (21, 1) de la meme trame ; publie la
mediane, le minimum et le maximum des rapports sur les tours.

Usage : pilote_d6.py --src DOSSIER_V12 --racine DOSSIER --donnees DOSSIER --sortie DOSSIER [--cas ng00,ng01,ng02]
                     [--profils 21,24,32] [--k 5] [--fils 48] [--passes 5] [--tours 3] [--jobs 44] [--delai 900]
Sorties : <sortie>/mes_d6.json et mes_d6.md, lignes brutes sous <sortie>/brut/. Codes : 0 rendu et controles
conformes ; 1 controle viole (identite ou invariance) ; 2 usage ou entree illisible ; 3 construction impossible.
Bibliotheque standard seule (Python 3.10 nu), aucun assert.
"""
import argparse
import array
import hashlib
import json
import os
import re
import shutil
import signal
import statistics
import struct
import subprocess
import sys

FACTORS = (1, 8, 2048)
OBJECT_KEYS = ('births', 'cells', 'inert_cells', 'extended_cells', 'representatives')


def build(src, root, bits, jobs):
    folder = os.path.join(root, 'p%d' % bits)
    configure = ['cmake', '-S', src, '-B', folder, '-DCMAKE_BUILD_TYPE=Release', '-DMHGP12_COORD_BITS=%d' % bits]
    targets = ['cmake', '--build', folder, '-j', str(jobs), '--target', 'mhgp12_catalogue_probe', 'mhgp12_tower_probe']
    for argv in (configure, targets):
        if subprocess.run(argv, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode != 0:
            return None
    return folder


def dilate(data, case, factor, folder):
    """Ecrit l'entree dilatee de la trame ; rend (chemin xyz, chemin ids, plus grande coordonnee)."""
    xyz = os.path.join(data, 'lidar_%s.u32le' % case)
    ids = os.path.join(data, 'lidar_%s.ids.u32le' % case)
    values = array.array('I')
    with open(xyz, 'rb') as handle:
        values.frombytes(handle.read())
    if sys.byteorder != 'little':
        values.byteswap()
    largest = max(values) * factor
    if factor == 1:
        return xyz, ids, largest
    if largest >= 1 << 32:
        return None, None, largest
    scaled = array.array('I', (v * factor for v in values))
    if sys.byteorder != 'little':
        scaled.byteswap()
    path = os.path.join(folder, 'lidar_%s_x%d.u32le' % (case, factor))
    with open(path, 'wb') as out:
        out.write(scaled.tobytes())
    return path, ids, largest


def run(argv, delay, raw_path):
    proc = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
    try:
        out, _err = proc.communicate(timeout=delay)
        code = proc.returncode
    except subprocess.TimeoutExpired:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        out, _err = proc.communicate()
        code = 'expire'
    with open(raw_path, 'wb') as handle:
        handle.write(out)
    # Les octets bruts restent disponibles ; aucune ligne invalide n'est sautee.
    def unique_object(pairs):
        row = {}
        for key, value in pairs:
            if key in row:
                raise ValueError('cle JSON repetee')
            row[key] = value
        return row

    def bad_constant(_value):
        raise ValueError('constante JSON non finie')

    try:
        lines = [json.loads(raw, object_pairs_hook=unique_object, parse_constant=bad_constant)
                 for raw in out.decode('utf-8').splitlines()]
        if not all(type(row) is dict for row in lines):
            raise ValueError('ligne JSON non objet')
    except (ValueError, UnicodeError):
        lines = []  # refus par summarize, meme si le processus a rendu zero
    return code, lines


# Schema produit par bench/catalogue_probe.cpp et bench/tower_probe.cpp. Adaptation declaree du 8 octobre (raccord de
# la voie appareil du catalogue, commit 8ba7d7287) : la ligne du catalogue porte desormais `path` ("cpu" ici), les
# diagnostics chains_repaired et chain_elements, et un objet `device` de seize compteurs, tous nuls sur la voie CPU
# (meme regle que bench/g4_catalogue_schema.py).
LEDGER_KEYS = ('nodes leaves filter_tests max_depth max_leaf dominance_tests prefixes judged census_tests emitted '
               'incidences q4_candidates q4_levels region_pair_tests region_pair_rejects region_line_tests '
               'region_line_rejects region_line_evaluations region_line_cache_hits region_line_fallbacks').split()
CAT_DIAG_KEYS = ('levels tasks candidates leaves_narrow leaves_medium leaves_wide leaves_exact leaves_virtual_warp '
                'leaves_rewritten max_leaf_span traversal_ns count_ns fill_ns levels_ns sort_ns assemble_ns '
                'table_ns peak_bytes chains_repaired chain_elements').split()
DEVICE_KEYS = ('batches replayed_leaves replayed_balls replayed_wide replayed_span rewritten_device rewritten_host '
               'device_bytes pinned_bytes allocations arena_bytes transfer_ns transfer_h2d_bytes transfer_d2h_bytes '
               'transfer_ops publish_ns').split()
G_DIAG_KEYS = 'count_ns fill_ns tables_ns resolve_ns workspace_bytes table_bytes peak_bytes'.split()
G_GC_DIAG_KEYS = G_DIAG_KEYS + 'prepare_ns setup_ns workspace_ns joins_ns orders_ns reste_ns'.split()
WORK_KEYS = ('probes first_probe_hits probe_hits_after_steps route_t1 route_cert_table route_cert_census '
             'route_fallback_table route_fallback_census fallback_no_proposal fallback_not_in_part '
             'fallback_certificate census_saturated census_complete census_sites census_sites_max census_nodes '
             'jumps_catalogue jumps_census inert_steps cell_stops birth_stops controls max_chain').split()


def uint(value, bits=64):
    return type(value) is int and 0 <= value < 1 << bits


def hex_sha(value):
    return type(value) is str and re.fullmatch(r'[0-9a-f]{64}', value) is not None


def integer_record(row, names):
    return type(row) is dict and set(row) == set(names) and all(uint(row[name]) for name in names)


def g_diagnostic_schema(diag, kmax):
    # Deux schemas fermes : G initial et Gc. Les tableaux portent K demande, meme si sites < K.
    if type(diag) is not dict:
        return None
    if set(diag) == set(G_DIAG_KEYS) | {'order_ns'}:
        scalar, arrays, schema = G_DIAG_KEYS, {'order_ns': kmax}, 'G'
    elif set(diag) == set(G_GC_DIAG_KEYS) | {'order_ns', 'pass_ns', 'table_ns', 'join_ns'}:
        scalar, arrays, schema = G_GC_DIAG_KEYS, dict(order_ns=kmax, pass_ns=kmax,
                                                     table_ns=kmax - 1, join_ns=kmax - 1), 'Gc'
    else:
        return None
    if not all(uint(diag[key]) for key in scalar) or any(
            type(diag[key]) is not list or len(diag[key]) != length or
            not all(uint(value) for value in diag[key]) for key, length in arrays.items()):
        return None
    return schema


def summarize(code, lines, phase, digest_key, expected=None, exported=False):
    # expected vient de la commande, jamais des lignes observees. Un refus n'a pas de temps chaud.
    result = dict(code=code, ok=False, chaud_ms=None, passes_ms=[], empreinte=None,
                  boules=None, incidences=None, objet={}, configuration=expected, admission='protocole invalide')
    if type(code) is not int or code != 0 or type(lines) is not list or not lines or \
            not all(type(row) is dict for row in lines) or type(expected) is not dict:
        return result
    if set(expected) != {'passes', 'coord_bits', 'kmax', 'threads', 'leaf'} or \
            not all(uint(value) for value in expected.values()) or \
            not 2 <= expected['passes'] <= 1000 or expected['coord_bits'] not in (21, 24, 32) or \
            not 1 <= expected['kmax'] <= 12 or not 1 <= expected['threads'] <= 256 or expected['leaf'] != 24:
        return result
    if (phase, digest_key) not in (('catalogue', 'catalogue_sha256'), ('tour_g', 'resolution_sha256')) or \
            type(exported) is not bool or (phase == 'tour_g' and exported):
        return result
    end = dict(phase='exit', status='ok', reason='none')
    if phase == 'tour_g':
        end['order'] = 0
    if lines[-1] != end or (phase == 'tour_g' and not uint(lines[-1].get('order'))):
        return result
    at, stages, digests, objects = 0, [], [], {}
    g_schema = None
    common = 'phase pass status reason coord_bits kmax threads sites wall_ns'.split()
    extra = ['path', 'leaf', 'balls', 'incidences', 'levels', 'ledger', 'diagnostics', 'device'] \
        if phase == 'catalogue' else ['order', 'diagnostics']
    for number in range(expected['passes']):
        if at >= len(lines) - 1:
            return result
        row = lines[at]
        at += 1
        if set(row) != set(common + extra) or row['phase'] != phase or row['status'] != 'ok' or \
                row['reason'] != 'none' or \
                not all(uint(row[key]) for key in common[1:] if key not in ('status', 'reason')):
            return result
        if row['pass'] != number or any(row[key] != expected[key] for key in ('coord_bits', 'kmax', 'threads')) or \
                not 1 <= row['sites'] <= 0xFFFFFFFF or (stages and row['sites'] != stages[0]['sites']):
            return result
        if phase == 'catalogue':
            if row['path'] != 'cpu' or not integer_record(row['device'], DEVICE_KEYS) or \
                    any(row['device'][key] != 0 for key in DEVICE_KEYS):
                return result
            if not all(uint(row[key]) for key in ('leaf', 'balls', 'incidences', 'levels')) or \
                    row['leaf'] != expected['leaf'] or not uint(row['balls'], 32) or \
                    not integer_record(row['ledger'], LEDGER_KEYS) or \
                    not integer_record(row['diagnostics'], CAT_DIAG_KEYS):
                return result
            if stages and any(row[key] != stages[0][key] for key in ('balls', 'incidences', 'levels')):
                return result
            if at >= len(lines) - 1 or set(lines[at]) != {'phase', digest_key} or \
                    lines[at]['phase'] != 'digest' or not hex_sha(lines[at][digest_key]):
                return result
            digests.append(lines[at][digest_key])
            at += 1
        else:
            schema = g_diagnostic_schema(row['diagnostics'], expected['kmax'])
            if not uint(row['order']) or row['order'] != 0 or schema is None or \
                    (g_schema is not None and g_schema != schema):
                return result
            g_schema = schema
        stages.append(row)
    if phase == 'tour_g':
        for k in range(1, min(expected['kmax'], stages[0]['sites']) + 1):
            if at >= len(lines) - 1:
                return result
            row = lines[at]
            at += 1
            if set(row) != {'phase', 'k', 'objet', 'travail'} or row['phase'] != 'ordre' or \
                    not uint(row['k']) or row['k'] != k or not integer_record(row['objet'], OBJECT_KEYS):
                return result
            work = row['travail']
            if type(work) is not dict or set(work) != set(WORK_KEYS) | {'chaines'} or \
                    not all(uint(work[key]) for key in WORK_KEYS) or type(work['chaines']) is not list or \
                    len(work['chaines']) != 16 or not all(uint(value) for value in work['chaines']):
                return result
            objects[k] = row['objet']
        if objects[1]['births'] != stages[0]['sites'] or at >= len(lines) - 1 or \
                set(lines[at]) != {'phase', digest_key} or lines[at]['phase'] != 'digest' or \
                not hex_sha(lines[at][digest_key]):
            return result
        digests.append(lines[at][digest_key])
        at += 1
    export_record = None
    if exported:
        if at >= len(lines) - 1:
            return result
        export_record = lines[at]
        if set(export_record) != {'phase', 'cat_bin_sha256', 'bytes'} or export_record['phase'] != 'export' or \
                not hex_sha(export_record['cat_bin_sha256']) or not uint(export_record['bytes']) or \
                export_record['bytes'] < 64 or export_record['cat_bin_sha256'] != digests[-1]:
            return result
        at += 1
    if at != len(lines) - 1 or len(set(digests)) != 1:
        return result
    walls = [row['wall_ns'] for row in stages]
    result.update(ok=True, chaud_ms=statistics.median(walls[1:]) / 1e6, passes_ms=[w / 1e6 for w in walls],
                  empreinte=digests[0], boules=stages[0].get('balls'), incidences=stages[0].get('incidences'),
                  niveaux=stages[0].get('levels'), sites=stages[0]['sites'], objet=objects,
                  export=export_record, admission='conforme')
    return result


def body_sha(path, expected=None):
    """MHGP12DP v1 catalogue : structure seulement, pas oracle geometrique ni validation du CSR.

    Disposition de src/catalogue/export.cpp et tests/catalogue/catalogue_dump.py.
    Lecture bornee par blocs ; SHA des sections seules, en-tete exclu comme auparavant.
    """
    digest, whole = hashlib.sha256(), hashlib.sha256()
    try:
        with open(path, 'rb') as handle:
            header = handle.read(64)
            if len(header) != 64 or header[:8] != b'MHGP12DP':
                return None
            version, kind, bits, kmax, order, sections, sites = struct.unpack_from('<6IQ', header, 8)
            name = header[40:64].rstrip(b'\0')
            if (version, kind, order, sections) != (1, 1, 0, 5) or bits not in (21, 24, 32) or \
                    not 1 <= kmax <= 12 or not 1 <= sites <= 0xFFFFFFFF or len(name) >= 24 or \
                    any(not 0x20 <= byte <= 0x7e for byte in name):
                return None
            if expected is not None and (bits, kmax, sites, name.decode('ascii')) != \
                    (expected['coord_bits'], expected['kmax'], expected['sites'], expected['frame']):
                return None
            whole.update(header)
            counts = {}
            total = 64
            for tag, size in (('SITEXYZ', 12), ('BALLS', 32), ('POPOFF', 8), ('POPVAL', 4), ('NLEVELS', 8)):
                section = handle.read(24)
                if len(section) != 24:
                    return None
                got_size, reserved, count = struct.unpack_from('<IIQ', section, 8)
                if section[:8] != tag.encode('ascii').ljust(8, b'\0') or got_size != size or reserved != 0:
                    return None
                counts[tag] = count
                if (tag == 'SITEXYZ' and count != sites) or \
                        (tag == 'POPOFF' and count != counts['BALLS'] + 1) or (tag == 'NLEVELS' and count != 1):
                    return None
                if expected is not None and ((tag == 'BALLS' and count != expected['balls']) or \
                        (tag == 'POPVAL' and count != expected['incidences'])):
                    return None
                digest.update(section)
                whole.update(section)
                total += 24
                left = count * size
                while left:
                    block = handle.read(min(left, 1 << 20))
                    if not block:
                        return None
                    if tag == 'NLEVELS' and (len(block) != 8 or (expected is not None and \
                            struct.unpack('<Q', block)[0] != expected['levels'])):
                        return None
                    digest.update(block)
                    whole.update(block)
                    total += len(block)
                    left -= len(block)
                padding = handle.read((-count * size) % 8)
                if padding != b'\0' * ((-count * size) % 8):
                    return None
                digest.update(padding)
                whole.update(padding)
                total += len(padding)
            if handle.read(1) or (expected is not None and
                    (whole.hexdigest() != expected['sha256'] or total != expected['bytes'])):
                return None
    except (OSError, ValueError, KeyError, TypeError, struct.error):
        return None
    return digest.hexdigest()


def take(binaries, inputs, combo, args, raw_dir, round_index):
    bits, factor = combo
    xyz, ids, _largest = inputs[combo[1]]
    common = ['--k=%d' % args.k, '--threads=%d' % args.fils, '--passes=%d' % args.passes, '--digest']
    tag = '%s_p%d_x%d_t%d' % (args.case_now, bits, factor, round_index)
    export = os.path.join(args.racine, 'export_' + tag)
    extra = ['--out=' + export, '--frame=' + args.case_now] if round_index == 0 and factor == 1 else []
    code_c, lines_c = run([os.path.join(binaries[bits], 'mhgp12_catalogue_probe'), xyz, ids] + common + extra,
                          args.delai, os.path.join(raw_dir, tag + '_catalogue.jsonl'))
    expected = dict(passes=args.passes, coord_bits=bits, kmax=args.k, threads=args.fils, leaf=24)
    catalogue = summarize(code_c, lines_c, 'catalogue', 'catalogue_sha256', expected, bool(extra))
    body = None
    if extra and catalogue['ok']:
        meta = catalogue['export']
        body = body_sha(os.path.join(export, 'cat.bin'), dict(
            coord_bits=bits, kmax=args.k, frame=args.case_now, sites=catalogue['sites'], balls=catalogue['boules'],
            incidences=catalogue['incidences'], levels=catalogue['niveaux'],
            sha256=meta['cat_bin_sha256'], bytes=meta['bytes']))
    shutil.rmtree(export, ignore_errors=True)
    catalogue['corps_sha256'] = body
    if extra and body is None:
        catalogue.update(ok=False, chaud_ms=None, admission='export absent ou invalide')
    code_t, lines_t = run([os.path.join(binaries[bits], 'mhgp12_tower_probe'), xyz, ids] + common, args.delai,
                          os.path.join(raw_dir, tag + '_tour.jsonl'))
    tower = summarize(code_t, lines_t, 'tour_g', 'resolution_sha256', expected)
    if catalogue['ok'] and tower['ok'] and catalogue['sites'] != tower['sites']:
        tower.update(ok=False, chaud_ms=None, admission='nombre de sites different du catalogue')
    return dict(cas=args.case_now, profil=bits, facteur=factor, tour=round_index, catalogue=catalogue, tour_g=tower)


def checks(entries):
    """Identite a facteur 1 entre profils ; invariance des comptes de l'objet par dilatation ; prises conformes."""
    problems = []
    for e in entries:
        for stage in ('catalogue', 'tour_g'):
            if not e[stage]['ok']:
                problems.append('%s p%d x%d tour %d : %s non conforme (code %s)' % (
                    e['cas'], e['profil'], e['facteur'], e['tour'], stage, e[stage]['code']))
    for case in sorted(set(e['cas'] for e in entries)):
        mine = [e for e in entries if e['cas'] == case and e['catalogue']['ok'] and e['tour_g']['ok']]
        native = [e for e in mine if e['facteur'] == 1]
        if len(set(e['tour_g']['empreinte'] for e in native)) > 1:
            problems.append('%s : empreintes de la resolution differentes entre profils a facteur 1' % case)
        bodies = set(e['catalogue']['corps_sha256'] for e in native if e['tour'] == 0)
        if len(bodies) != 1 or None in bodies:
            problems.append('%s : exports du catalogue (en-tete exclu) differents entre profils a facteur 1' % case)
        shapes = set(json.dumps([e['catalogue']['boules'], e['catalogue']['incidences'], e['tour_g']['objet']],
                                sort_keys=True) for e in mine)
        if len(shapes) > 1:
            problems.append('%s : comptes de l\'objet changes par profil ou dilatation' % case)
    return problems


def ratios(entries):
    rows = []
    for case in sorted(set(e['cas'] for e in entries)):
        combos = sorted(set((e['profil'], e['facteur']) for e in entries if e['cas'] == case))
        for combo in combos:
            row = dict(cas=case, profil=combo[0], facteur=combo[1])
            for stage in ('catalogue', 'tour_g'):
                values = []
                for e in entries:
                    if e['cas'] != case or (e['profil'], e['facteur']) != combo or e[stage]['chaud_ms'] is None:
                        continue
                    base = [b for b in entries if b['cas'] == case and b['tour'] == e['tour'] and b['profil'] == 21
                            and b['facteur'] == 1 and b[stage]['chaud_ms']]
                    if base:
                        values.append(e[stage]['chaud_ms'] / base[0][stage]['chaud_ms'])
                times = [e[stage]['chaud_ms'] for e in entries if e['cas'] == case and
                         (e['profil'], e['facteur']) == combo and e[stage]['chaud_ms'] is not None]
                row[stage] = dict(chaud_ms=statistics.median(times) if times else None,
                                  rapport=statistics.median(values) if values else None,
                                  rapport_min=min(values) if values else None,
                                  rapport_max=max(values) if values else None)
            rows.append(row)
    return rows


def markdown(rows, problems, args):
    lines = ['# MES-D6 : profils u21, u24 et u32 sur trames reelles', '',
             'K = %d, %d fils, %d passes par prise, %d tours alternes ; temps chaud = mediane des passes 2..P ; '
             'rapport au couple (21, x1) de la meme trame et du meme tour.' % (args.k, args.fils, args.passes,
                                                                                args.tours), '',
             '| trame | profil | dilatation | catalogue (ms) | rapport | etage G (ms) | rapport |',
             '| --- | ---: | ---: | ---: | ---: | ---: | ---: |']
    for r in rows:
        cells = []
        for stage in ('catalogue', 'tour_g'):
            s = r[stage]
            cells.append('-' if s['chaud_ms'] is None else '%.1f' % s['chaud_ms'])
            cells.append('-' if s['rapport'] is None else '%.3f [%.3f ; %.3f]' % (s['rapport'], s['rapport_min'],
                                                                                 s['rapport_max']))
        lines.append('| %s | %d | x%d | %s | %s | %s | %s |' % (r['cas'], r['profil'], r['facteur'], *cells))
    lines += ['', 'Controles : %s.' % ('conformes' if not problems else '%d ecart(s)' % len(problems))]
    lines += ['- ' + p for p in problems]
    return '\n'.join(lines) + '\n'


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ('--src', '--racine', '--donnees', '--sortie'):
        parser.add_argument(name, required=True)
    parser.add_argument('--cas', default='ng00,ng01,ng02')
    parser.add_argument('--profils', default='21,24,32')
    for name, default in (('--k', 5), ('--fils', 48), ('--passes', 5), ('--tours', 3), ('--jobs', 44),
                          ('--delai', 900)):
        parser.add_argument(name, type=int, default=default)
    try:
        args = parser.parse_args(argv[1:])
        cases = [c for c in args.cas.split(',') if c]
        profiles = [int(p) for p in args.profils.split(',')]
    except (SystemExit, ValueError):
        return 2
    if not cases or 21 not in profiles or any(p not in (21, 24, 32) for p in profiles) or args.passes < 2 or \
            args.tours < 1 or not 1 <= args.k <= 12 or args.fils < 1:
        return 2
    if len(cases) != len(set(cases)) or len(profiles) != len(set(profiles)) or args.jobs < 1 or args.delai < 1:
        return 2
    os.makedirs(os.path.join(args.sortie, 'brut'), exist_ok=True)
    inputs_dir = os.path.join(args.racine, 'entrees')
    os.makedirs(inputs_dir, exist_ok=True)
    binaries = {}
    for bits in profiles:
        folder = build(args.src, args.racine, bits, args.jobs)
        if folder is None:
            return 3
        binaries[bits] = folder
    entries = []
    for case in cases:
        try:
            inputs = {f: dilate(args.donnees, case, f, inputs_dir) for f in FACTORS}
        except (OSError, ValueError, OverflowError):
            return 2
        # La comparaison D6 exige sa reference (21, x1) pour chaque trame.
        if inputs[1][0] is None or inputs[1][2] >= 1 << 21:
            return 2
        combos = [(bits, f) for bits in profiles for f in FACTORS
                  if inputs[f][0] is not None and inputs[f][2] < 1 << bits]
        args.case_now = case
        for round_index in range(args.tours):
            shift = round_index % len(combos)
            for combo in combos[shift:] + combos[:shift]:
                entries.append(take(binaries, inputs, combo, args, os.path.join(args.sortie, 'brut'), round_index))
                with open(os.path.join(args.sortie, 'mes_d6.json'), 'w', encoding='utf-8') as out:
                    json.dump(dict(mesure='MES-D6', prises=entries), out, indent=1, sort_keys=True)
    problems = checks(entries)
    rows = ratios(entries)
    with open(os.path.join(args.sortie, 'mes_d6.json'), 'w', encoding='utf-8') as out:
        json.dump(dict(mesure='MES-D6', prises=entries, rapports=rows, controles=problems), out, indent=1,
                  sort_keys=True)
    with open(os.path.join(args.sortie, 'mes_d6.md'), 'w', encoding='utf-8') as out:
        out.write(markdown(rows, problems, args))
    print(json.dumps(dict(prises=len(entries), ecarts=len(problems)), sort_keys=True))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
