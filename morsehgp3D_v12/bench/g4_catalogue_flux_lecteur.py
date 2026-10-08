#!/usr/bin/env python3
"""Lecteur strict des sorties natives des sondes de la session G4 T2-d-C (bench/catalogue_probe.cpp et
bench/full_probe.cpp), pour le juge g4_catalogue_flux_judge.py (prelecture d'admission de l'auditeur Codex du 8
octobre, receipts/audit_reponses_20261008/t2d_c_admission) : chaque execution est liee a sa commande. Options de la
commande produites ici, une seule source pour le pilote qui les joue et pour le juge qui les exige ; sequence exacte
des lignes (open, puis par passe catalogue, sorties, digest, digest_complet dans l'ordre de la sonde, puis exit) ;
statuts et raisons ; indices de passe consecutifs ; voie, profil, K, feuille, fils ; cles exactes de chaque ligne et
de chaque bloc (grand livre, diagnostics, appareil) ; entiers u64 hors booleens. Un champ absent n'est jamais une
mesure nulle. Bibliotheque standard, Python 3.10 nu, aucun assert.
"""
import re

COORD_BITS = 21
LEAF = 24
# Schema des lignes de la sonde du catalogue (bench/catalogue_probe.cpp), cles exactes.
LEDGER_KEYS = ('nodes', 'leaves', 'filter_tests', 'max_depth', 'max_leaf', 'dominance_tests', 'prefixes', 'judged',
               'census_tests', 'emitted', 'incidences', 'q4_candidates', 'q4_levels', 'region_pair_tests',
               'region_pair_rejects', 'region_line_tests', 'region_line_rejects', 'region_line_evaluations',
               'region_line_cache_hits', 'region_line_fallbacks')
DIAG_KEYS = ('levels', 'tasks', 'candidates', 'leaves_narrow', 'leaves_medium', 'leaves_wide', 'leaves_exact',
             'leaves_virtual_warp', 'leaves_rewritten', 'max_leaf_span', 'traversal_ns', 'count_ns', 'fill_ns',
             'levels_ns', 'sort_ns', 'assemble_ns', 'table_ns', 'peak_bytes', 'chains_repaired', 'chain_elements')
DEVICE_KEYS = ('batches', 'replayed_leaves', 'replayed_balls', 'replayed_wide', 'replayed_span', 'rewritten_device',
               'rewritten_host', 'device_bytes', 'pinned_bytes', 'allocations', 'arena_bytes', 'transfer_ns',
               'transfer_h2d_bytes', 'transfer_d2h_bytes', 'transfer_ops', 'publish_ns')
CAT_BASE_KEYS = frozenset(('phase', 'path', 'pass', 'status', 'reason', 'coord_bits', 'kmax', 'leaf', 'threads',
                           'sites', 'wall_ns'))
CAT_OK_KEYS = CAT_BASE_KEYS | {'balls', 'incidences', 'levels', 'ledger', 'diagnostics', 'device'}
CAT_INTS = ('pass', 'coord_bits', 'kmax', 'leaf', 'threads', 'sites', 'wall_ns')
COUNTS = ('sites', 'balls', 'incidences', 'levels')
SORTIES_KEYS = frozenset(('phase', 'pass', 'outputs_ns', 'outputs_bytes', 'stream_chunks'))
CAT_OPEN_KEYS = frozenset(('phase', 'status', 'reason', 'open_ns'))
EXIT_KEYS = frozenset(('phase', 'status', 'reason'))
REFUSALS = ('invalid_input', 'unsupported_degeneracy', 'resource_exhausted')
# Schema des lignes de la sonde FULL (bench/full_probe.cpp, main 902041f66), comme le lecteur strict de MES-B.
FULL_KEYS = frozenset(('phase', 'pass', 'trame', 'voie', 'status', 'coord_bits', 'kmax', 'threads', 'sites',
                       'wall_ns', 'etapes_ns', 'c_ns', 'g_ns', 'hors_mur_ns', 'pic_octets', 'cpu_ns',
                       'rss_max_octets', 'appareil_octets', 'epinglee_octets', 'pic_appareil_octets',
                       'memoire_octets', 'full_sha256'))
FULL_BLOCKS = {'etapes_ns': ('P', 'C', 'G', 'raccord', 'TMVR', 'T', 'M', 'V', 'R'),
               'c_ns': ('parcours', 'feuilles', 'emission', 'fin_etage', 'transferts', 'publication'),
               'g_ns': ('tables', 'resolution'), 'hors_mur_ns': ('validation', 'empreinte')}
FULL_INTS = ('pass', 'coord_bits', 'kmax', 'threads', 'sites', 'wall_ns', 'pic_octets', 'appareil_octets',
             'epinglee_octets', 'pic_appareil_octets')
MEM_STAGES = ('P', 'C', 'G', 'raccord', 'TMVR')
FULL_OPEN_KEYS = frozenset(('phase', 'status', 'reason', 'wall_ns', 'budget_appareil'))
DEVICE_OPEN_LINE = 'device_open : voie appareil jouee sur 9 temoins'
BUDGET_OPEN_LINE = re.compile(r'^device_open_budget : refus sous budget serre joues sur '
                              r'l\'appareil \(\d+ appels\)$')


def is_int(x):
    """Entier u64, jamais un booleen."""
    return type(x) is int and 0 <= x < (1 << 64)


def is_hex(x):
    return type(x) is str and re.fullmatch(r'[0-9a-f]{64}', x) is not None


def catalogue_spec(path, k, threads, passes, digest=False, complet=False, sorties=False, cache=0):
    return {'path': path, 'k': k, 'leaf': LEAF, 'threads': threads, 'passes': passes, 'digest': digest,
            'complet': complet, 'sorties': sorties, 'cache': cache}


def catalogue_options(spec):
    """Options de la sonde du catalogue pour une commande (apres les deux fichiers d'entree) : une seule source pour
    le pilote, qui les joue, et pour le juge, qui les exige."""
    opts = ['--k=%d' % spec['k'], '--leaf=%d' % spec['leaf'], '--threads=%d' % spec['threads'],
            '--passes=%d' % spec['passes']]
    opts += ['--device'] if spec['path'] == 'device' else []
    opts += ['--digest'] if spec['digest'] else []
    opts += ['--digest-complet'] if spec['complet'] else []
    opts += ['--sorties'] if spec['sorties'] else []
    opts += ['--cache=%d' % spec['cache']] if spec['cache'] else []
    return opts


def full_options(k, threads, passes):
    """Options de la sonde FULL apres --trame (voie appareil, empreinte FUL1)."""
    return ['--k=%d' % k, '--threads=%d' % threads, '--passes=%d' % passes, '--device', '--digest']


def check_catalogue_row(row, spec, p):
    """'' si la ligne << catalogue >> reussie de la passe p est conforme et liee a la commande, sinon la raison."""
    if type(row) is not dict or set(row) != CAT_OK_KEYS:
        return 'cles de la passe %d' % p
    if not all(is_int(row[k]) for k in CAT_INTS + ('balls', 'incidences', 'levels')):
        return 'entier u64 attendu (passe %d)' % p
    if row['phase'] != 'catalogue' or row['path'] != spec['path'] or row['pass'] != p or row['status'] != 'ok' or \
            row['reason'] != 'none' or row['coord_bits'] != COORD_BITS or row['kmax'] != spec['k'] or \
            row['leaf'] != spec['leaf'] or row['threads'] != spec['threads']:
        return 'passe %d hors commande (voie, indice, statut, profil, K, feuille ou fils)' % p
    for name, keys in (('ledger', LEDGER_KEYS), ('diagnostics', DIAG_KEYS), ('device', DEVICE_KEYS)):
        block = row[name]
        if type(block) is not dict or set(block) != set(keys) or not all(is_int(block[k]) for k in keys):
            return 'bloc %s incomplet ou non entier (passe %d)' % (name, p)
    return ''


def read_tail(rows, i, parsed):
    """Fin d'une sortie a partir de la ligne i : la ligne exit seule, conforme."""
    if i != len(rows) - 1:
        return 'lignes hors sequence apres la passe %d' % len(parsed['passes'])
    end = rows[i]
    if type(end) is not dict or set(end) != EXIT_KEYS or end['phase'] != 'exit' or type(end['status']) is not str or \
            type(end['reason']) is not str:
        return 'ligne exit absente ou hors schema'
    parsed['exit'] = end
    return ''


EXTRAS = (('sorties', 'sorties', 'sorties'), ('digest', 'digest', 'digests'), ('digest_complet', 'complet',
                                                                                  'complets'))


def check_extra(row, phase, p):
    """Lignes << sorties >>, << digest >> et << digest_complet >> de la passe p."""
    if phase == 'sorties':
        ok = set(row) == SORTIES_KEYS and all(is_int(row[k]) for k in ('pass', 'outputs_ns', 'outputs_bytes',
                                                                         'stream_chunks')) and row['pass'] == p
    elif phase == 'digest':
        ok = set(row) == {'phase', 'catalogue_sha256'} and is_hex(row['catalogue_sha256'])
    else:
        ok = set(row) == {'phase', 'niveaux_sha256', 'table_sha256', 'table_ecarts'} and \
            is_hex(row['niveaux_sha256']) and is_hex(row['table_sha256']) and is_int(row['table_ecarts'])
    return '' if ok else 'ligne %s hors schema (passe %d)' % (phase, p)


def read_passes(rows, i, parsed, spec):
    """Passes a partir de la ligne i : ligne catalogue (reussie, ou une passe en echec qui clot la sortie), puis les
    lignes demandees dans l'ordre de la sonde ; rend (indice suivant, raison)."""
    for p in range(spec['passes']):
        row = rows[i] if i < len(rows) else None
        if type(row) is not dict or row.get('phase') != 'catalogue':
            return i, ''
        if row.get('status') != 'ok':
            if set(row) != CAT_BASE_KEYS or row['pass'] != p or type(row['reason']) is not str or \
                    type(row['status']) is not str or row['reason'] == 'none':
                return i, 'passe %d en echec hors schema' % p
            parsed['failed'] = row
            return i + 1, ''
        why = check_catalogue_row(row, spec, p)
        if why:
            return i, why
        parsed['passes'].append(row)
        i += 1
        for phase, flag, bucket in EXTRAS:
            nxt = rows[i] if i < len(rows) else None
            if not spec[flag] or type(nxt) is not dict or nxt.get('phase') != phase:
                continue
            why = check_extra(nxt, phase, p)
            if why:
                return i, why
            parsed[bucket].append(nxt)
            i += 1
    return i, ''


def read_catalogue(run, spec):
    """(etat, raison, lu) d'une sonde du catalogue liee a sa commande : 'ok' (code 0, passes completes), 'invariant'
    (code 3, cause lue), 'refus' (code 2, cause lue), 'signal', 'delai' ou 'illisible'."""
    parsed = {'open': None, 'passes': [], 'sorties': [], 'digests': [], 'complets': [], 'exit': None, 'failed': None}
    if type(run) is not dict:
        return 'illisible', 'execution absente', parsed
    if run.get('timeout') is True:
        return 'delai', 'delai', parsed
    code = run.get('code')
    if type(code) is int and code < 0:
        return 'signal', 'signal %d' % -code, parsed
    if type(code) is not int or run.get('bad_lines') != 0 or run.get('options') != catalogue_options(spec):
        return 'illisible', 'code, lignes illisibles ou options differentes de la commande', parsed
    rows, i = run.get('rows'), 0
    if type(rows) is not list or not rows:
        return 'illisible', 'sortie vide', parsed
    if spec['path'] == 'device':
        o = rows[0]
        if type(o) is not dict or set(o) != CAT_OPEN_KEYS or o['phase'] != 'open' or o['status'] != 'ok' or \
                o['reason'] != 'none' or not is_int(o['open_ns']):
            return 'illisible', 'ligne open absente, hors schema ou en echec', parsed
        parsed['open'], i = o, 1
    i, why = read_passes(rows, i, parsed, spec)
    why = why or read_tail(rows, i, parsed)
    if why:
        return 'illisible', why, parsed
    return verdict_of_exit(code, parsed, spec)


def complete_pass(parsed, spec, p):
    """La passe p (p < 0 : aucune) a toutes ses lignes demandees."""
    if p < 0:
        return True
    return len(parsed['passes']) > p and (not spec['sorties'] or len(parsed['sorties']) > p) and \
        (not spec['digest'] or len(parsed['digests']) > p) and (not spec['complet'] or len(parsed['complets']) > p)


def verdict_of_exit(code, parsed, spec):
    end, done, failed = parsed['exit'], len(parsed['passes']), parsed['failed']
    if code == 0:
        if end['status'] == 'ok' and end['reason'] == 'none' and failed is None and done == spec['passes'] and \
                complete_pass(parsed, spec, done - 1):
            return 'ok', '', parsed
        return 'illisible', 'code 0 sans toutes les passes demandees', parsed
    cause = '%s/%s' % (end['status'], end['reason'])
    if end['status'] == 'ok' or end['reason'] == 'none':
        return 'illisible', 'echec sans cause lisible', parsed
    if failed is not None:  # passe en echec : les precedentes completes, meme cause que la sortie
        coherent = failed['status'] == end['status'] and failed['reason'] == end['reason'] and \
            complete_pass(parsed, spec, done - 1)
    else:  # echec apres la ligne catalogue de la derniere passe lue (empreinte de cette passe)
        coherent = done >= 1 and complete_pass(parsed, spec, done - 2) and not complete_pass(parsed, spec, done - 1)
    if not coherent:
        return 'illisible', 'echec hors sequence (%s)' % cause, parsed
    if code == 3 and end['status'] == 'invariant_violated':
        return 'invariant', cause, parsed
    if code == 2 and end['status'] in REFUSALS:
        return 'refus', cause, parsed
    return 'illisible', 'code %s et sortie %s incoherents' % (code, cause), parsed


def check_full_row(row, case, k, threads, p, sites=None):
    """'' si la ligne full de la passe p est conforme et liee a la commande (memes regles que le lecteur partage de
    main, microbancs/outils/lecteur_full.py, a partir de a2c2fccfd ; cpu_ns et rss_max_octets peuvent etre null,
    jamais lus ici) ; sites : nombre attendu (identite), ou None."""
    if type(row) is not dict or set(row) != FULL_KEYS or not all(is_int(row[x]) for x in FULL_INTS):
        return 'ligne full hors schema (passe %d)' % p
    if row['phase'] != 'full' or row['pass'] != p or row['status'] != 'ok' or row['trame'] != case or \
            row['voie'] != 'device' or row['kmax'] != k or row['threads'] != threads or \
            row['coord_bits'] != COORD_BITS or not is_hex(row['full_sha256']) or row['wall_ns'] == 0 or \
            row['sites'] == 0 or (sites is not None and row['sites'] != sites):
        return 'passe full %d hors commande (voie, K, fils, profil, trame, sites ou mur nul)' % p
    for name, keys in FULL_BLOCKS.items():
        block = row[name]
        if type(block) is not dict or set(block) != set(keys) or not all(is_int(block[x]) for x in keys):
            return 'bloc %s mal forme (passe %d)' % (name, p)
    mem = row['memoire_octets']
    if type(mem) is not dict or set(mem) != set(MEM_STAGES) or any(
            type(mem[s]) is not list or len(mem[s]) != 2 or not all(is_int(v) for v in mem[s]) or mem[s][0] > mem[s][1]
            for s in MEM_STAGES) or max(mem[s][1] for s in MEM_STAGES) != row['pic_octets']:
        return 'memoire par etage mal formee ou incoherente avec pic_octets (passe %d)' % p
    st, g = row['etapes_ns'], row['g_ns']
    if sum(st[x] for x in MEM_STAGES) > row['wall_ns'] or st['T'] + st['M'] + st['V'] + st['R'] > st['TMVR'] or \
            g['tables'] + g['resolution'] > st['G']:
        return 'etages non inclus dans le mur (passe %d)' % p
    if not all(x is None or is_int(x) for x in (row['cpu_ns'], row['rss_max_octets'])):
        return 'cpu_ns ou rss_max_octets ni entier ni null (passe %d)' % p
    return ''


def read_full(run, case, k, threads, passes, sites=None):
    """(etat, raison, empreintes, murs, C) d'une sonde FULL : open, (full, liberation) par passe, exit."""
    bad = ('illisible', None, [], [], [])
    if type(run) is not dict or run.get('timeout') is True or run.get('code') != 0 or run.get('bad_lines') != 0 or \
            run.get('options') != full_options(k, threads, passes) or type(run.get('rows')) is not list:
        return ('illisible', 'execution FULL absente ou hors commande') + bad[2:]
    rows = run['rows']
    if len(rows) != 2 * passes + 2:
        return ('illisible', 'nombre de lignes FULL') + bad[2:]
    o, end = rows[0], rows[-1]
    if type(o) is not dict or set(o) != FULL_OPEN_KEYS or o['phase'] != 'open' or o['status'] != 'ok' or \
            o['reason'] != 'none' or o['budget_appareil'] != 'partage' or not is_int(o['wall_ns']):
        return ('illisible', 'ligne open FULL') + bad[2:]
    if type(end) is not dict or set(end) != EXIT_KEYS or end['phase'] != 'exit' or end['status'] != 'ok' or \
            end['reason'] != 'none':
        return ('illisible', 'ligne exit FULL') + bad[2:]
    digests, walls, c_ns = [], [], []
    for p in range(passes):
        full, free = rows[1 + 2 * p], rows[2 + 2 * p]
        why = check_full_row(full, case, k, threads, p, sites)
        if why:
            return ('illisible', why) + bad[2:]
        if type(free) is not dict or set(free) != {'phase', 'pass', 'liberation_ns'} or free['phase'] != 'liberation' \
                or free['pass'] != p or not is_int(free['liberation_ns']):
            return ('illisible', 'ligne liberation (passe %d)' % p) + bad[2:]
        digests.append(full['full_sha256'])
        walls.append(full['wall_ns'])
        c_ns.append(full['etapes_ns']['C'])
    return 'ok', '', digests, walls, c_ns
