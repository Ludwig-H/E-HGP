"""Lecteurs des sorties de l'executable mhgp11, en bibliotheque standard (Python 3.10 nu, aucun assert).

Specification de la sortie parametree, paragraphes 6.1 a 6.7 (tranche S5) :
  - manifeste.json, schema ehgp.v11.output.v1 : objet JSON a cles en ORDRE FIXE, sans espace, entiers decimaux, termine
    par un saut de ligne ; sa forme canonique est controlee octet pour octet (json.dumps, separators=(',', ':')) ;
  - full.mhgp11ful1, format MHGP11FUL1 : decode par bench/full_semantic.py (decodeur strict de la v11, deja juge par
    les portes de la tour), dont les comptes par ordre sont recoupes avec le manifeste.

Toute violation leve ValueError (full_semantic.need). Usage :

    import mhgp11_formats as formats
    manifest = formats.read_manifest(raw_bytes)          # dict ordonne, schema et forme canonique controles
    report = formats.check_directory(path, bits)         # manifeste, inventaire, tailles, sha256, decodage
    words = formats.full_point_ids(data)                 # [(decalage, PointId)] dans l'ordre des sites (Morton)

L'empreinte tree_k_sha256 est la signature version 2 de docs/SORTIES.md, paragraphe 8 (geometrie des sites, puis par
noeud parent, rang dense, genre, naissance en SiteIdx et enfants). Elle n'est pas recalculable depuis MHGP11FUL1, qui
porte les centres des naissances et non leur support S* ni les rangs denses ; elle le sera depuis MHGP11SP (tranche
S7). Le lecteur en controle la forme ; la porte mhgp11_api_session_tree_digest la recalcule depuis la tour en memoire
et la compare au champ publie, les portes du CLI la comparent entre appels (fils, permutations, reetiquetages).
"""
import hashlib
import json
import os
import struct

import full_semantic
from catalogue_semantic import need

SCHEMA = 'ehgp.v11.output.v1'
MANIFEST = 'manifeste.json'
PENDING = '.pending'
FULL_NAME = 'full.mhgp11ful1'
FULL_MAGIC = b'MHGP11FUL1'
MAX_DECIMAL = 64

TOP_KEYS = ('schema', 'output', 'status', 'public_status', 'coord_bits', 'k', 'parameters', 'inputs', 'files',
            'tree_k_sha256', 'counts')
PARAMETER_KEYS = ('budget_bytes', 'grid_step', 'origin')
INPUT_KEYS = ('name', 'bytes', 'sha256')
FILE_KEYS = ('name', 'format', 'version', 'bytes', 'sha256')
FULL_COUNT_KEYS = ('sites', 'points', 'orders')
ORDER_KEYS = ('k', 'births', 'nodes', 'edges', 'root')
WORD = struct.Struct('<Q')


def _pairs(pairs):
    keys = [key for key, _ in pairs]
    need(len(set(keys)) == len(keys), 'manifeste : cle en double')
    return dict(pairs)


def _reject(token):
    raise ValueError('manifeste : nombre non entier ou constante %s' % token)


def _integer(value, what, low=0, high=(1 << 64) - 1):
    need(type(value) is int and low <= value <= high, 'manifeste : %s hors domaine (%r)' % (what, value))
    return value


def _digest(value, what):
    need(type(value) is str and len(value) == 64 and all(c in '0123456789abcdef' for c in value),
         'manifeste : empreinte %s mal formee' % what)
    return value


def _keys(obj, keys, what):
    need(type(obj) is dict and tuple(obj.keys()) == keys, 'manifeste : cles de %s %r, attendu %r' %
         (what, tuple(obj.keys()) if type(obj) is dict else obj, keys))


def is_decimal(text, signed):
    """Decimal exact du CLI : chiffres, au plus un point suivi d'un chiffre, '-' initial pour une origine."""
    if type(text) is not str or not 0 < len(text) <= MAX_DECIMAL:
        return False
    body = text[1:] if signed and text.startswith('-') else text
    whole, dot, fraction = body.partition('.')
    digits = '0123456789'
    return (whole != '' and all(c in digits for c in whole) and
            (dot == '' or (fraction != '' and all(c in digits for c in fraction))))


def _parameters(parameters):
    _keys(parameters, PARAMETER_KEYS, 'parameters')
    budget, step, origin = (parameters[key] for key in PARAMETER_KEYS)
    need(budget is None or (type(budget) is int and 0 < budget < 1 << 64), 'manifeste : budget_bytes')
    need(step is None or (is_decimal(step, False) and step.strip('0.') != ''), 'manifeste : grid_step')
    need(origin is None or (type(origin) is list and len(origin) == 3 and all(is_decimal(c, True) for c in origin)),
         'manifeste : origin')


def _full_counts(counts, k):
    _keys(counts, FULL_COUNT_KEYS, 'counts')
    sites = _integer(counts['sites'], 'sites', 1, (1 << 32) - 2)
    need(counts['points'] == sites, 'manifeste : points differents des sites (poids un)')
    orders = counts['orders']
    need(type(orders) is list and len(orders) == k, 'manifeste : un compte par ordre')
    for expected, order in enumerate(orders, 1):
        _keys(order, ORDER_KEYS, 'counts.orders')
        need(order['k'] == expected, 'manifeste : ordres 1..k')
        births = _integer(order['births'], 'births', 1)
        nodes = _integer(order['nodes'], 'nodes', births, 2 * births - 1)
        need(order['edges'] == nodes - 1 and _integer(order['root'], 'root') < nodes, 'manifeste : arbre')
        need(expected != 1 or births == sites, 'manifeste : naissances de l\'ordre 1')


def read_manifest(raw):
    """Manifeste lu depuis ses octets : schema, types, ordre des cles et forme canonique (octet pour octet)."""
    need(type(raw) is bytes and raw.endswith(b'\n') and raw.count(b'\n') == 1, 'manifeste : une ligne terminee')
    try:
        text = raw.decode('ascii')
        manifest = json.loads(text, object_pairs_hook=_pairs, parse_float=_reject, parse_constant=_reject)
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ValueError('manifeste illisible : %s' % error)
    need(json.dumps(manifest, separators=(',', ':'), ensure_ascii=True) + '\n' == text,
         'manifeste : forme non canonique')
    _keys(manifest, TOP_KEYS, 'manifeste')
    need((manifest['schema'], manifest['output'], manifest['status'], manifest['public_status']) ==
         (SCHEMA, 'full', 'complete', 'not_claimed'), 'manifeste : schema, sortie ou statuts')
    need(manifest['coord_bits'] in (18, 21, 24) and type(manifest['coord_bits']) is int, 'manifeste : coord_bits')
    k = _integer(manifest['k'], 'k', 1, 12)
    _parameters(manifest['parameters'])
    inputs = manifest['inputs']
    need(type(inputs) is list and len(inputs) == 2, 'manifeste : deux entrees')
    for name, entry in zip(('points', 'ids'), inputs):
        _keys(entry, INPUT_KEYS, 'inputs')
        need(entry['name'] == name, 'manifeste : nom d\'entree')
        _integer(entry['bytes'], 'bytes')
        _digest(entry['sha256'], 'd\'entree')
    need(inputs[0]['bytes'] == 3 * inputs[1]['bytes'], 'manifeste : 12 et 4 octets par point')
    files = manifest['files']
    need(type(files) is list and len(files) == 1, 'manifeste : un fichier pour la sortie full')
    _keys(files[0], FILE_KEYS, 'files')
    need((files[0]['name'], files[0]['format'], files[0]['version']) == (FULL_NAME, 'MHGP11FUL1', 1),
         'manifeste : fichier full')
    _integer(files[0]['bytes'], 'bytes', 42)
    _digest(files[0]['sha256'], 'de fichier')
    _digest(manifest['tree_k_sha256'], 'tree_k_sha256')
    _full_counts(manifest['counts'], k)
    need(inputs[1]['bytes'] == 4 * manifest['counts']['points'], 'manifeste : points et octets d\'entree')
    return manifest


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def check_directory(path, bits):
    """Dossier publie : inventaire exact (fichiers du manifeste et manifeste), D.pending absent, tailles et sha256 des
    fichiers, decodage strict de MHGP11FUL1 et comptes recoupes. Rend le manifeste, son sha256 et le decodage."""
    need(os.path.isdir(path) and not os.path.islink(path), 'dossier publie absent : %s' % path)
    need(not os.path.lexists(path.rstrip('/') + PENDING), 'D.pending present a cote de D')
    with open(os.path.join(path, MANIFEST), 'rb') as handle:
        raw = handle.read()
    manifest = read_manifest(raw)
    need(manifest['coord_bits'] == bits, 'manifeste : profil %d, attendu %d' % (manifest['coord_bits'], bits))
    names = sorted([MANIFEST] + [entry['name'] for entry in manifest['files']])
    need(sorted(os.listdir(path)) == names, 'inventaire du dossier %r, attendu %r' % (sorted(os.listdir(path)), names))
    entry = manifest['files'][0]
    file_path = os.path.join(path, entry['name'])
    need(os.path.getsize(file_path) == entry['bytes'], 'taille du fichier full')
    need(sha256_file(file_path) == entry['sha256'], 'empreinte du fichier full')
    counts = manifest['counts']
    decoded = full_semantic.inspect(file_path, bits, manifest['k'], counts['sites'])
    need(decoded['raw_sha256'] == entry['sha256'] and decoded['points'] == counts['points'], 'decodage full')
    for order, expected in zip(decoded['orders'], counts['orders']):
        need((order['order'], order['births'], order['nodes'], order['edges'], order['root']) ==
             tuple(expected[key] for key in ORDER_KEYS), 'comptes de l\'ordre %d' % expected['k'])
    return dict(manifest=manifest, manifest_sha256=hashlib.sha256(raw).hexdigest(), decoded=decoded)


def full_point_ids(data):
    """Positions et valeurs des PointId d'un MHGP11FUL1 : [(decalage, PointId)] dans l'ordre des sites."""
    need(len(data) >= 42 and data[:10] == FULL_MAGIC, 'MHGP11FUL1 : signature')
    cursor, words = 10, []

    def word():
        nonlocal cursor
        need(cursor + 8 <= len(data), 'MHGP11FUL1 : mot tronque')
        value, = WORD.unpack_from(data, cursor)
        cursor += 8
        return value

    _bits, _kmax, sites, _points = (word() for _ in range(4))
    for _ in range(sites):
        weight = [word() for _ in range(4)][3]
        for _ in range(weight):
            offset = cursor
            words.append((offset, word()))
    return words
