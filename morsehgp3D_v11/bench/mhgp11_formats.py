"""Lecteurs des sorties de l'executable mhgp11, en bibliotheque standard (Python 3.10 nu, aucun assert).

Specification de la sortie parametree, paragraphes 6.1 a 6.7 (tranche S5), et docs/SORTIES.md, paragraphes 6 et 8
(tranche S7) :
  - manifeste.json, schema ehgp.v11.output.v1 : objet JSON a cles en ORDRE FIXE, sans espace, entiers decimaux, termine
    par un saut de ligne ; sa forme canonique est controlee octet pour octet (json.dumps, separators=(',', ':')) ;
  - full.mhgp11ful1, format MHGP11FUL1 : decode par bench/full_semantic.py (decodeur strict de la v11, deja juge par
    les portes de la tour), dont les comptes par ordre sont recoupes avec le manifeste ;
  - supports.mhgp11sp, format MHGP11SP version 1 : decode par read_supports (en-tete, colonnes, bourrage nul), qui
    DERIVE tout ce que le paragraphe 6 declare derive (enfants, postordre, tailles de sous-arbre, rattachement, q_min,
    niveaux et centres exacts, feuilles de K = 1, comptes par boule et par support) et CONTROLE tout ce qu'il y declare
    controle (voir read_supports) ; check_directory recompte les agregats du manifeste et recalcule tree_k_sha256
    (signature version 2) depuis le seul fichier, independamment du moteur ;
  - points.mhgp11pt, format MHGP11PT version 1 (tranche S9, docs/SORTIES.md, paragraphe 7) : decode par read_points,
    qui controle la forme, la pendaison (t <= Q < M, proprietaire vivant au plancher) et l'arbre de points, et, en
    lecture exacte (exact=True), le plancher, le drapeau strict et l'ordre des plateaux par sommes de racines exactes
    (radical_sign, ecrit a neuf) ; check_directory recompte les comptes du manifeste. tree_k_sha256 n'en est pas
    recalculable (S* absent) : les portes le comparent a celui de --sortie=supports ;
  - etiquettes.mhgp11et, format MHGP11ET version 1 (tranche S10, docs/SORTIES.md, paragraphe 7) : decode par read_flat
    (en-tete, taille exacte, etiquettes >= -1) ; check_directory recoupe l'en-tete avec la selection du manifeste et
    le bruit avec ses comptes. Les etiquettes valent le plus petit PointId de leur cluster : seules les portes, qui
    connaissent les entrees, le controlent.

Toute violation leve ValueError (full_semantic.need). Usage :

    import mhgp11_formats as formats
    manifest = formats.read_manifest(raw_bytes)          # dict ordonne, schema et forme canonique controles
    report = formats.check_directory(path, bits)         # manifeste, inventaire, tailles, sha256, decodage
    words = formats.full_point_ids(data)                 # [(decalage, PointId)] dans l'ordre des sites (Morton)

L'empreinte tree_k_sha256 est la signature version 2 de docs/SORTIES.md, paragraphe 8 (geometrie des sites, puis par
noeud parent, rang dense, genre, naissance en SiteIdx et enfants). Elle n'est pas recalculable depuis MHGP11FUL1, qui
porte les centres des naissances et non leur support S* ni les rangs denses ; elle l'est depuis MHGP11SP
(tree_signature), et check_directory l'exige egale au champ publie pour la sortie supports. Pour full, le lecteur en
controle la forme ; la porte mhgp11_api_session_tree_digest la recalcule depuis la tour en memoire
et la compare au champ publie, la porte mhgp11_cli_tree_signature compare le champ publie par l'executable a une
serialisation independante (fixtures de l'auditeur), les autres portes du CLI la comparent entre appels (fils,
permutations, reetiquetages).
"""
import array
import hashlib
import json
import math
import os
import struct
import sys

import full_semantic
from catalogue_semantic import need

SCHEMA = 'ehgp.v11.output.v1'
MANIFEST = 'manifeste.json'
PENDING = '.pending'
FULL_NAME = 'full.mhgp11ful1'
FULL_MAGIC = b'MHGP11FUL1'
SUPPORTS_NAME = 'supports.mhgp11sp'
SUPPORTS_MAGIC = b'MHGP11SP'
POINTS_NAME = 'points.mhgp11pt'
FLAT_NAME = 'etiquettes.mhgp11et'
FLAT_HEADER = 8 + 6 * 8
NONE = (1 << 32) - 1
# Fichier de donnees de chaque sortie : nom, format, taille minimale (en-tete).
OUTPUT_FILES = {'full': (FULL_NAME, 'MHGP11FUL1', 42), 'supports': (SUPPORTS_NAME, 'MHGP11SP', 136),
                'points': (POINTS_NAME, 'MHGP11PT', 144), 'plat': (FLAT_NAME, 'MHGP11ET', FLAT_HEADER)}
MAX_DECIMAL = 64

TOP_KEYS = ('schema', 'output', 'status', 'public_status', 'coord_bits', 'k', 'parameters', 'inputs', 'files',
            'tree_k_sha256', 'counts')
FLAT_COUNT_KEYS = ('sites', 'nodes', 'clusters', 'selected', 'noise', 'decisions', 'exact', 'equalities',
                   'unbracketed')
FLAT_METHODS = ('eom', 'feuilles')
PARAMETER_KEYS = ('budget_bytes', 'grid_step', 'origin')
# Sortie plat : mcs, z et selection s'ajoutent aux parametres.
FLAT_PARAMETER_KEYS = PARAMETER_KEYS + ('mcs', 'z', 'selection')
INPUT_KEYS = ('name', 'bytes', 'sha256')
FILE_KEYS = ('name', 'format', 'version', 'bytes', 'sha256')
FULL_COUNT_KEYS = ('sites', 'points', 'orders')
ORDER_KEYS = ('k', 'births', 'nodes', 'edges', 'root')
# Manifeste de la sortie supports, version 2 (arbre couvrant d'ordre K, 6 octobre 2026) : sans liaison interne ni
# agregat de Q_b (la version 1 portait aussi multi_support_balls, max_supports_per_ball, kparties_reliees, cofaces).
SUPPORT_COUNT_KEYS = ('sites', 'nodes', 'births', 'merges', 'balls', 'roles', 'supports', 'arities', 'extended_shells',
                      'prior')
ROLE_KEYS = ('birth', 'merge', 'internal')
SPANNING_ROLE_KEYS = ('birth', 'merge')
ARITY_KEYS = ('2', '3', '4')
SUM_MAX_KEYS = ('sum', 'max')
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


def _parameters(parameters, flat=False):
    _keys(parameters, FLAT_PARAMETER_KEYS if flat else PARAMETER_KEYS, 'parameters')
    budget, step, origin = (parameters[key] for key in PARAMETER_KEYS)
    if flat:
        _integer(parameters['mcs'], 'mcs', 2, (1 << 32) - 1)
        _integer(parameters['z'], 'z', 1, 3)
        need(parameters['selection'] in FLAT_METHODS, 'manifeste : selection')
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


def _supports_counts(counts, k):
    _keys(counts, SUPPORT_COUNT_KEYS, 'counts')
    _keys(counts['roles'], SPANNING_ROLE_KEYS, 'counts.roles')
    _keys(counts['arities'], ARITY_KEYS, 'counts.arities')
    sites = _integer(counts['sites'], 'sites', max(k, 1), (1 << 32) - 2)
    nodes = _integer(counts['nodes'], 'nodes', 1)
    need(_integer(counts['births'], 'births', 1) + _integer(counts['merges'], 'merges') == nodes,
         'manifeste : naissances et fusions')
    need(k != 1 or counts['births'] == sites, 'manifeste : feuilles de l\'ordre 1')
    balls = _integer(counts['balls'], 'balls')
    births = _integer(counts['roles']['birth'], 'birth')
    need(births == (0 if k == 1 else counts['births']) and births + _integer(counts['roles']['merge'], 'merge') == balls,
         'manifeste : roles (une naissance par noeud de naissance, puis des fusions)')
    supports = _integer(counts['supports'], 'supports', balls, balls)
    need(sum(_integer(counts['arities'][key], 'arite') for key in ARITY_KEYS) == supports, 'manifeste : arites')
    need(_integer(counts['extended_shells'], 'extended_shells') <= balls, 'manifeste : coquilles etendues')
    _integer(counts['prior'], 'prior')


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
    need((manifest['schema'], manifest['status'], manifest['public_status']) == (SCHEMA, 'complete', 'not_claimed') and
         manifest['output'] in OUTPUT_FILES, 'manifeste : schema, sortie ou statuts')
    output = manifest['output']
    need(manifest['coord_bits'] in (18, 21, 24) and type(manifest['coord_bits']) is int, 'manifeste : coord_bits')
    k = _integer(manifest['k'], 'k', 1, 12)
    _parameters(manifest['parameters'], output == 'plat')
    inputs = manifest['inputs']
    need(type(inputs) is list and len(inputs) == 2, 'manifeste : deux entrees')
    for name, entry in zip(('points', 'ids'), inputs):
        _keys(entry, INPUT_KEYS, 'inputs')
        need(entry['name'] == name, 'manifeste : nom d\'entree')
        _integer(entry['bytes'], 'bytes')
        _digest(entry['sha256'], 'd\'entree')
    need(inputs[0]['bytes'] == 3 * inputs[1]['bytes'], 'manifeste : 12 et 4 octets par point')
    files = manifest['files']
    need(type(files) is list and len(files) == 1, 'manifeste : un fichier pour la sortie %s' % output)
    _keys(files[0], FILE_KEYS, 'files')
    name, form, smallest = OUTPUT_FILES[output]
    need((files[0]['name'], files[0]['format'], files[0]['version']) == (name, form, 2 if output == 'supports' else 1),
         'manifeste : fichier %s' % output)
    _integer(files[0]['bytes'], 'bytes', smallest)
    _digest(files[0]['sha256'], 'de fichier')
    _digest(manifest['tree_k_sha256'], 'tree_k_sha256')
    if output == 'full':
        _full_counts(manifest['counts'], k)
        need(inputs[1]['bytes'] == 4 * manifest['counts']['points'], 'manifeste : points et octets d\'entree')
    elif output == 'points':
        _points_counts(manifest['counts'], k)
        need(inputs[1]['bytes'] == 4 * manifest['counts']['sites'], 'manifeste : points et octets d\'entree')
    elif output == 'plat':
        _flat_counts(manifest['counts'], k)
        need(inputs[1]['bytes'] == 4 * manifest['counts']['sites'], 'manifeste : points et octets d\'entree')
        need(files[0]['bytes'] == FLAT_HEADER + 8 * manifest['counts']['sites'], 'manifeste : taille de MHGP11ET')
    else:
        _supports_counts(manifest['counts'], k)
        need(inputs[1]['bytes'] == 4 * manifest['counts']['sites'], 'manifeste : points et octets d\'entree')
    return manifest


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def check_directory(path, bits, exact=True):
    """Dossier publie : inventaire exact (fichiers du manifeste et manifeste), D.pending absent, tailles et sha256 des
    fichiers, decodage strict de MHGP11FUL1 ou de MHGP11SP et comptes recoupes ; pour supports, agregats du manifeste
    recomptes et tree_k_sha256 recalcule depuis le fichier. Rend le manifeste, son sha256 et le decodage (dict de
    full_semantic.inspect, ou SupportsFile)."""
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
    need(os.path.getsize(file_path) == entry['bytes'], 'taille du fichier %s' % manifest['output'])
    need(sha256_file(file_path) == entry['sha256'], 'empreinte du fichier %s' % manifest['output'])
    counts = manifest['counts']
    if manifest['output'] == 'plat':
        with open(file_path, 'rb') as handle:
            et = read_flat(handle.read())
        parameters = manifest['parameters']
        need((et['k'], et['mcs'], et['z'], FLAT_METHODS[et['selection']]) ==
             (manifest['k'], parameters['mcs'], parameters['z'], parameters['selection']),
             'MHGP11ET : en-tete et parametres du manifeste')
        need(et['n'] == counts['sites'] and et['labels'].count(-1) == counts['noise'],
             'MHGP11ET : points ou bruit du manifeste')
        return dict(manifest=manifest, manifest_sha256=hashlib.sha256(raw).hexdigest(), decoded=et)
    if manifest['output'] == 'points':
        with open(file_path, 'rb') as handle:
            pt = read_points(handle.read(), bits, exact)
        need(pt.k == manifest['k'], 'MHGP11PT : K du fichier et du manifeste')
        recount = pt.manifest_counts()
        need(recount == counts, 'manifeste : comptes publies %r, recomptes depuis MHGP11PT %r' % (counts, recount))
        return dict(manifest=manifest, manifest_sha256=hashlib.sha256(raw).hexdigest(), decoded=pt)
    if manifest['output'] == 'supports':
        with open(file_path, 'rb') as handle:
            sp = read_supports(handle.read(), bits)
        need(sp.k == manifest['k'], 'MHGP11SP : K du fichier et du manifeste')
        recount = sp.manifest_counts()
        need(recount == counts, 'manifeste : comptes publies %r, recomptes depuis MHGP11SP %r' % (counts, recount))
        need(sp.tree_signature() == manifest['tree_k_sha256'],
             'manifeste : tree_k_sha256 different de la signature version 2 recalculee depuis MHGP11SP')
        return dict(manifest=manifest, manifest_sha256=hashlib.sha256(raw).hexdigest(), decoded=sp)
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


# ---------------------------------------------------------------- MHGP11SP version 1 (tranche S7)

SP_HEADER = 8 + 16 * 8
SHELL_CAPACITY = 24
ROLE_BIRTH, ROLE_MERGE, ROLE_INTERNAL = 0, 1, 2
KIND_SITE, KIND_BIRTH, KIND_MERGE = 0, 1, 2


def _pad8(size):
    return (size + 7) & ~7


def _column(data, at, count, width, what):
    """Colonne petit-boutiste de `count` valeurs de `width` octets a `at`, bourrage nul jusqu'a 8 ; rend (valeurs,
    decalage suivant)."""
    end = at + count * width
    padded = _pad8(end)
    need(padded <= len(data), 'MHGP11SP : colonne %s tronquee' % what)
    need(not any(data[end:padded]), 'MHGP11SP : bourrage non nul apres la colonne %s' % what)
    code = {1: 'B', 2: 'H', 4: 'I'}[width]
    values = array.array(code)
    need(values.itemsize == width, 'MHGP11SP : type %s de %d octets attendu' % (code, width))
    values.frombytes(data[at:end])
    if sys.byteorder != 'little' and width > 1:
        values.byteswap()
    return values, padded


def _cross(u, v):
    return (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])


def _dot(u, v):
    return u[0] * v[0] + u[1] * v[1] + u[2] * v[2]


def _sub(u, v):
    return (u[0] - v[0], u[1] - v[1], u[2] - v[2])


def sphere_of(points):
    """Sphere circonscrite minimale d'un support S* de 2 a 4 points, en entiers : centre C / D (D > 0) et rayon carre
    num / D^2, num = |D a - C|^2. Rend (C, D, num), ou None si S* n'est pas un support positif (points confondus,
    alignes, coplanaires, triangle non strictement aigu, centre hors du tetraedre)."""
    a = points[0]
    if len(points) == 2:
        b = points[1]
        if a == b:
            return None
        center, den = (a[0] + b[0], a[1] + b[1], a[2] + b[2]), 2
    elif len(points) == 3:
        b, c = points[1], points[2]
        u, v = _sub(b, a), _sub(c, a)
        w = _cross(u, v)
        if w == (0, 0, 0) or _dot(u, v) <= 0 or _dot(_sub(a, b), _sub(c, b)) <= 0 or _dot(_sub(a, c), _sub(b, c)) <= 0:
            return None
        uu, vv = _dot(u, u), _dot(v, v)
        n = _cross((uu * v[0] - vv * u[0], uu * v[1] - vv * u[1], uu * v[2] - vv * u[2]), w)
        den = 2 * _dot(w, w)
        center = (den * a[0] + n[0], den * a[1] + n[1], den * a[2] + n[2])
    else:
        b, c, d = points[1], points[2], points[3]
        u, v, t = _sub(b, a), _sub(c, a), _sub(d, a)
        det = _dot(u, _cross(v, t))
        if det == 0:
            return None
        vt, tu, uv = _cross(v, t), _cross(t, u), _cross(u, v)
        uu, vv, tt = _dot(u, u), _dot(v, v), _dot(t, t)
        n = tuple(uu * vt[j] + vv * tu[j] + tt * uv[j] for j in range(3))
        den = 2 * det
        center = tuple(den * a[j] + n[j] for j in range(3))
        if den < 0:
            den, center = -den, tuple(-x for x in center)
    num = sum((den * a[j] - center[j]) ** 2 for j in range(3))
    if len(points) == 4 and not _inside(points, center, den):
        return None
    return center, den, num


def _inside(points, center, den):
    """Centre C / D (D > 0) strictement interieur au tetraedre : meme signe strict que le sommet oppose, sur chaque
    face."""
    for face, opposite in (((0, 1, 2), 3), ((0, 1, 3), 2), ((0, 2, 3), 1), ((1, 2, 3), 0)):
        p, q, r = (points[i] for i in face)
        normal = _cross(_sub(q, p), _sub(r, p))
        side = _dot(normal, _sub(points[opposite], p))
        here = _dot(normal, tuple(center[j] - den * p[j] for j in range(3)))
        if side * here <= 0:
            return False
    return True


def is_positive_support(points, sphere):
    """Q est un support positif de la sphere (C, D, num) : ses sites sont sur la sphere, affinement independants, et le
    centre est dans l'interieur relatif de leur enveloppe convexe. Entiers exacts (equivalent aux predicats en
    Fraction : centre C / D, D > 0)."""
    center, den, num = sphere
    if any(sum((den * p[j] - center[j]) ** 2 for j in range(3)) != num for p in points):
        return False
    if len(points) == 2:
        a, b = points
        return a != b and all(den * (a[j] + b[j]) == 2 * center[j] for j in range(3))
    if len(points) == 3:
        a, b, c = points
        w = _cross(_sub(b, a), _sub(c, a))
        if w == (0, 0, 0) or _dot(w, tuple(center[j] - den * a[j] for j in range(3))) != 0:
            return False
        # equidistant et coplanaire : le centre est le circoncentre, interieur si et seulement si le triangle est aigu
        return (_dot(_sub(b, a), _sub(c, a)) > 0 and _dot(_sub(a, b), _sub(c, b)) > 0 and
                _dot(_sub(a, c), _sub(b, c)) > 0)
    a, b, c, d = points
    return _dot(_sub(b, a), _cross(_sub(c, a), _sub(d, a))) != 0 and _inside(points, center, den)


def _less_level(a, b):
    """Niveaux (num, D) : num / D^2 < num' / D'^2."""
    return a[0] * b[1] * b[1] < b[0] * a[1] * a[1]


def _same_level(a, b):
    return a[0] * b[1] * b[1] == b[0] * a[1] * a[1]


def _less_center(a, b):
    """Centres (C, D) en ordre lexicographique exact des coordonnees."""
    for j in range(3):
        left, right = a[0][j] * b[1], b[0][j] * a[1]
        if left != right:
            return left < right
    return False


def _binomial_row(n):
    return [math.comb(n, i) for i in range(n + 1)]


def closure(m, supports):
    """N_0 .. N_m : nombre de parties a j sites d'une coquille de m sites qui contiennent un support (supports donnes
    par leurs sites, sous-ensembles de la coquille). Les sites de coquille hors de tout support sont interchangeables :
    on compte les parties independantes (sans support) de la reunion des supports, puis on complete."""
    union = sorted(set(s for q in supports for s in q))
    u = len(union)
    need(u <= m, 'MHGP11SP : supports sur %d sites distincts pour une coquille de %d' % (u, m))
    bit = dict((s, 1 << i) for i, s in enumerate(union))
    edges = tuple(sorted(set(sum(bit[s] for s in q) for q in supports)))
    memo = {}

    def independent(free, edges):
        # parties sans support de l'ensemble `free` (masque), par taille
        if not edges:
            return _binomial_row(bin(free).count('1'))
        key = (free, edges)
        if key in memo:
            return memo[key]
        v = edges[0] & -edges[0]
        rest = free & ~v
        without = independent(rest, tuple(e for e in edges if not e & v))
        shrunk = []
        empty = False
        for e in edges:
            if e & v:
                e &= ~v
                if e == 0:
                    empty = True
                    break
            shrunk.append(e)
        out = list(without)
        if not empty:
            with_v = independent(rest, tuple(sorted(set(shrunk))))
            out += [0] * (len(with_v) + 1 - len(out))
            for i, value in enumerate(with_v):
                out[i + 1] += value
        memo[key] = out
        return out

    free = independent((1 << u) - 1, edges)
    containing = [math.comb(u, i) - (free[i] if i < len(free) else 0) for i in range(u + 1)]
    return [sum(containing[i] * math.comb(m - u, j - i) for i in range(min(j, u) + 1) if j - i <= m - u)
            for j in range(m + 1)]


def ball_counts(k, p, m, supports):
    """Comptes derives d'une boule (docs/SORTIES.md, paragraphe 6) : kparties_reliees, compressed_parts,
    strict_traces, cofaces, gabriel_cofaces, et par support cofaces et gabriel_cofaces."""
    t = k - p
    n = closure(m, supports) if len(supports) > 1 or len(supports[0]) != m else [int(j == m) for j in range(m + 1)]
    comb = math.comb
    per = [comb(p + m - len(q), k + 1 - len(q)) if k + 1 >= len(q) else 0 for q in supports]
    gab = [comb(m - len(q), t + 1 - len(q)) if t + 1 >= len(q) else 0 for q in supports]
    return dict(kparties_reliees=comb(p + m, k), compressed_parts=comb(m, t), strict_traces=comb(m, t) - n[t],
                cofaces=sum(comb(p, k + 1 - j) * n[j] for j in range(m + 1) if 0 <= k + 1 - j <= p),
                gabriel_cofaces=n[t + 1] if t + 1 <= m else 0, cofaces_support=per, gabriel_cofaces_support=gab)


class SupportsFile:
    """MHGP11SP decode, derive et controle (read_supports). Colonnes brutes (array) et derives :
      x, y, z, point_id (par SiteIdx) ; parent, rank, kind, ball_count (par noeud) ; ball_rank, prior_count,
      support_count, role, p, m (par boule) ; arity, sites ; prior ;
      children[v] (croissants), post[v], size[v], node_of[b] (rattachement), first_ball[v], ball_at[b] (indice du
      premier support), site_at[s] (indice du premier site du support s), prior_at[b] ; qmin[b] ; sphere[b] (C, D,
      num) ; leaf_site[i] (K = 1 : ligne de SITES de la feuille i) ; counts[b] (comptes derives, a la demande)."""

    def points_of(self, s):
        """Lignes de SITES du support d'indice s."""
        at = self.site_at[s]
        return tuple(self.sites[at:at + self.arity[s]])

    def supports_of(self, b):
        first = self.ball_at[b]
        return [self.points_of(s) for s in range(first, first + self.support_count[b])]

    def position(self, site):
        return (self.x[site], self.y[site], self.z[site])

    def level(self, b):
        """Niveau exact de la boule b : rayon carre de la sphere de S*, en Fraction."""
        from fractions import Fraction
        _center, den, num = self.sphere[b]
        return Fraction(num, den * den)

    def center(self, b):
        from fractions import Fraction
        center, den, _num = self.sphere[b]
        return tuple(Fraction(c, den) for c in center)

    def counts(self, b):
        """Comptes derives de la boule b (ball_counts) ; components : prior_count (fusion), 1 (interne), 0
        (naissance)."""
        out = ball_counts(self.k, self.p[b], self.m[b], self.supports_of(b))
        out['components'] = (self.prior_count[b] if self.role[b] == ROLE_MERGE else
                             int(self.role[b] == ROLE_INTERNAL))
        return out

    def manifest_counts(self):
        """Agregats du manifeste version 2 (docs/SORTIES.md, paragraphe 8), recomptes depuis le fichier."""
        roles, arities = [0, 0], [0, 0, 0]
        for value in self.role:
            need(value in (ROLE_BIRTH, ROLE_MERGE), 'MHGP11SP : role %d hors de l\'arbre couvrant' % value)
            roles[value] += 1
        for value in self.arity:
            arities[value - 2] += 1
        births = sum(1 for value in self.kind if value != KIND_MERGE)
        return dict(sites=self.n, nodes=self.N, births=births, merges=self.N - births, balls=self.B,
                    roles=dict(zip(SPANNING_ROLE_KEYS, roles)), supports=self.S,
                    arities=dict(zip(ARITY_KEYS, arities)),
                    extended_shells=sum(1 for b in range(self.B) if self.m[b] > self.qmin[b]), prior=self.A)

    def tree_signature(self):
        """tree_k_sha256, signature version 2 (docs/SORTIES.md, paragraphe 8), depuis le seul fichier : geometrie,
        puis par noeud parent, rang, genre, naissance (ligne de site a K = 1, S* sinon) et enfants croissants."""
        geometry = hashlib.sha256()
        geometry.update(b'MHGP11GX' + struct.pack('<QQ', self.bits, self.n))
        xyz = array.array('I', [0]) * (3 * self.n)
        xyz[0::3], xyz[1::3], xyz[2::3] = self.x, self.y, self.z
        if sys.byteorder != 'little':
            xyz.byteswap()
        geometry.update(xyz.tobytes())
        out = hashlib.sha256()
        out.update(b'MHGP11TK' + struct.pack('<QQQQQ', 2, self.bits, self.k, self.n, self.N))
        out.update(geometry.digest())
        parts = []
        for v in range(self.N):
            kind = self.kind[v]
            if kind == KIND_SITE:
                birth = (self.leaf_site[v],)
            elif kind == KIND_BIRTH:
                birth = self.points_of(self.ball_at[self.first_ball[v]])
            else:
                birth = ()
            children = self.children[v]
            parts.append(struct.pack('<IIBB%dI' % len(birth), self.parent[v], self.rank[v], kind, len(birth), *birth))
            parts.append(struct.pack('<I%dI' % len(children), len(children), *children))
            if len(parts) >= 4096:
                out.update(b''.join(parts))
                parts = []
        out.update(b''.join(parts))
        return out.hexdigest()


def morton(point):
    key = 0
    for bit in range(24):
        key |= ((((point[0] >> bit) & 1) << (3 * bit)) | (((point[1] >> bit) & 1) << (3 * bit + 1)) |
                (((point[2] >> bit) & 1) << (3 * bit + 2)))
    return key


def read_supports(data, bits):
    """Decode MHGP11SP (version 2, arbre couvrant d'ordre K : naissances et fusions, S* seul, sans colonne
    support_count ; ou version 1, W_K entiere et Q_b, fichiers anterieurs au 6 octobre 2026) et controle, sans la
    coquille (docs/SORTIES.md, paragraphe 6). En version 2, roles birth et merge seulement, S = B, m <= 255, et pas de
    contre-epreuve strict_traces (Q_b non publie). Controles communs :
      - en-tete : magie, version 1, profil, 1 <= K <= min(12, n), decalages egaux a la disposition des colonnes, taille
        totale egale a celle du fichier ; bourrage nul ;
      - sites : coordonnees dans le profil, positions distinctes, lignes en ordre de Morton strict ;
      - arbre bien forme : une racine, la derniere ; parent de numero plus grand, rang strictement plus grand ;
        naissances d'abord (sans enfant), puis fusions (au moins deux enfants) ; genre 0 exactement aux feuilles de
        K = 1 ; numerotation canonique (naissances par (niveau, centre), fusions par (niveau, plus petite naissance)) ;
      - boules : W_K (p + q_min - 1 <= K <= p + m, m <= 24), triees dans chaque noeud par (rang, S* lexicographique
        complete par kNone) ; premiere boule propre au rang du noeud ; rangs coherents avec les niveaux exacts (egalite
        et ordre) ;
      - supports : arite 2 a 4, sites croissants, ordre (arite, SiteIdx) strict, premier support d'arite q_min, S*
        support positif de sa sphere, chaque support positif de la sphere de S* (entiers exacts) ; coquille reguliere :
        un seul support ;
      - roles (lemmes B et C) : naissance au rang d'un noeud de genre 1, premiere et seule de ses boules a ce rang ;
        fusion au rang d'un noeud de genre 2, branches non vides incluses dans ses enfants, reunion des branches des
        fusions d'un noeud egale a ses enfants ; interne strictement entre le rang du noeud et celui de son parent (ou
        au-dessus de la racine), sans branche publiee ; strict_traces nul si et seulement si naissance.
    La completude de Q_b n'est pas verifiable (coquille non publiee). Leve ValueError a la premiere violation."""
    need(type(data) in (bytes, bytearray) and len(data) >= SP_HEADER and data[:8] == SUPPORTS_MAGIC,
         'MHGP11SP : signature')
    words = struct.unpack_from('<16Q', data, 8)
    version, coord_bits, k, n, nodes, root, balls, supports, arities, prior = words[:10]
    need(version in (1, 2), 'MHGP11SP : version %d' % version)
    need(coord_bits == bits and bits in (18, 21, 24), 'MHGP11SP : profil %d, attendu %d' % (coord_bits, bits))
    need(1 <= k <= 12 and k <= n < NONE and 1 <= nodes < NONE, 'MHGP11SP : K, n ou N hors domaine')
    layout = [SP_HEADER]
    layout.append(layout[-1] + 4 * _pad8(4 * n))
    layout.append(layout[-1] + 3 * _pad8(4 * nodes) + _pad8(nodes))
    layout.append(layout[-1] + 2 * _pad8(4 * balls) + (_pad8(2 * balls) if version == 1 else 0) + 3 * _pad8(balls))
    layout.append(layout[-1] + _pad8(supports) + _pad8(4 * arities))
    layout.append(layout[-1] + _pad8(4 * prior))
    need(list(words[10:16]) == layout, 'MHGP11SP : decalages %r, attendu %r' % (list(words[10:16]), layout))
    need(layout[-1] == len(data), 'MHGP11SP : taille %d, en-tete %d' % (len(data), layout[-1]))
    f = SupportsFile()
    f.bits, f.k, f.n, f.N, f.root, f.B, f.S, f.Z, f.A = bits, k, n, nodes, root, balls, supports, arities, prior
    f.version = version
    need(version == 1 or supports == balls, 'MHGP11SP 2 : un support par boule (S = B)')
    at = layout[0]
    f.x, at = _column(data, at, n, 4, 'x')
    f.y, at = _column(data, at, n, 4, 'y')
    f.z, at = _column(data, at, n, 4, 'z')
    f.point_id, at = _column(data, at, n, 4, 'point_id')
    f.parent, at = _column(data, at, nodes, 4, 'parent')
    f.rank, at = _column(data, at, nodes, 4, 'rank')
    f.kind, at = _column(data, at, nodes, 1, 'kind')
    f.ball_count, at = _column(data, at, nodes, 4, 'ball_count')
    f.ball_rank, at = _column(data, at, balls, 4, 'balls.rank')
    f.prior_count, at = _column(data, at, balls, 4, 'prior_count')
    if version == 1:
        f.support_count, at = _column(data, at, balls, 2, 'support_count')
    else:
        f.support_count = array.array('H', [1]) * balls
    f.role, at = _column(data, at, balls, 1, 'role')
    f.p, at = _column(data, at, balls, 1, 'p')
    f.m, at = _column(data, at, balls, 1, 'm')
    f.arity, at = _column(data, at, supports, 1, 'arity')
    f.sites, at = _column(data, at, arities, 4, 'sites')
    f.prior, at = _column(data, at, prior, 4, 'prior')
    need(at == len(data), 'MHGP11SP : colonnes et taille')
    _check_sites(f)
    _check_tree(f)
    _check_balls(f)
    _check_roles(f)
    _check_canonical(f)
    return f


def _check_sites(f):
    limit = 1 << f.bits
    points = list(zip(f.x, f.y, f.z))
    need(all(c < limit for p in points for c in p), 'MHGP11SP : coordonnee hors du profil')
    keys = [morton(p) for p in points]
    need(all(keys[i] < keys[i + 1] for i in range(len(keys) - 1)), 'MHGP11SP : sites hors de l\'ordre de Morton')
    need(len(set(f.point_id)) == f.n, 'MHGP11SP : PointId en double (un PointId par site, refus duplicate_point_id)')
    f.points = points
    if f.k == 1:
        f.leaf_site = sorted(range(f.n), key=lambda s: points[s])


def _check_tree(f):
    n_nodes = f.N
    need(f.root == n_nodes - 1 and f.parent[f.root] == NONE, 'MHGP11SP : racine')
    children = [[] for _ in range(n_nodes)]
    for v in range(n_nodes - 1):
        up = f.parent[v]
        need(v < up < n_nodes and f.rank[v] < f.rank[up], 'MHGP11SP : parent %r du noeud %d (numero, rang)' % (up, v))
        children[up].append(v)  # croissants : v croissant
    f.children = children
    births = sum(1 for c in children if not c)
    need(all(not children[v] for v in range(births)) and all(len(children[v]) >= 2 for v in range(births, n_nodes)),
         'MHGP11SP : naissances d\'abord, puis fusions d\'au moins deux enfants')
    for v in range(n_nodes):
        want = KIND_MERGE if v >= births else (KIND_SITE if f.k == 1 else KIND_BIRTH)
        need(f.kind[v] == want, 'MHGP11SP : genre %d du noeud %d, attendu %d' % (f.kind[v], v, want))
    need(f.k != 1 or births == f.n, 'MHGP11SP : feuilles de K = 1')
    f.births = births
    # postordre (enfants par numero croissant) et tailles de sous-arbre : enfants de numero plus petit
    post = [0] * n_nodes
    size = [1] * n_nodes
    for v in range(n_nodes):
        for c in children[v]:
            size[v] += size[c]
    order, stack = [], [(f.root, 0)]
    while stack:
        v, i = stack.pop()
        if i < len(children[v]):
            stack.append((v, i + 1))
            stack.append((children[v][i], 0))
        else:
            order.append(v)
    need(len(order) == n_nodes, 'MHGP11SP : arbre non connexe')
    for j, v in enumerate(order):
        post[v] = j
    f.post, f.size, f.postorder = post, size, order
    for v in range(n_nodes):
        need(f.kind[v] == KIND_SITE or f.ball_count[v] >= 1, 'MHGP11SP : noeud %d sans boule propre' % v)
        need(f.kind[v] != KIND_SITE or (f.ball_count[v] == 0 and f.rank[v] == 0), 'MHGP11SP : feuille %d' % v)
    need(sum(f.ball_count) == f.B and sum(f.support_count) == f.S and sum(f.arity) == f.Z and
         sum(f.prior_count) == f.A, 'MHGP11SP : sommes des colonnes et en-tete')


def _check_balls(f):
    k, n = f.k, f.n
    node_of, first_ball = [0] * f.B, [0] * f.N
    b = 0
    for v in f.postorder:
        first_ball[v] = b
        for _ in range(f.ball_count[v]):
            node_of[b] = v
            b += 1
    f.node_of, f.first_ball = node_of, first_ball
    ball_at, site_at = [0] * f.B, [0] * f.S
    s = z = 0
    for b in range(f.B):
        ball_at[b] = s
        s += f.support_count[b]
    for s in range(f.S):
        site_at[s] = z
        z += f.arity[s]
    f.ball_at, f.site_at = ball_at, site_at
    pts = f.points
    qmin, sphere, star = [0] * f.B, [None] * f.B, [None] * f.B
    levels = {}
    for b in range(f.B):
        count, p, m = f.support_count[b], f.p[b], f.m[b]
        need(count >= 1, 'MHGP11SP : boule %d sans support' % b)
        first = ball_at[b]
        previous = None
        for s in range(first, first + count):
            a = f.arity[s]
            need(2 <= a <= 4, 'MHGP11SP : arite %d' % a)
            sites = f.sites[site_at[s]:site_at[s] + a]
            need(all(sites[i] < sites[i + 1] for i in range(a - 1)) and sites[-1] < n,
                 'MHGP11SP : sites du support %d non croissants ou hors de SITES' % s)
            key = (a, tuple(sites))
            need(previous is None or previous < key, 'MHGP11SP : supports de la boule %d hors de l\'ordre' % b)
            previous = key
        q = f.arity[first]
        qmin[b] = q
        need(f.role[b] in ((ROLE_BIRTH, ROLE_MERGE, ROLE_INTERNAL) if f.version == 1 else (ROLE_BIRTH, ROLE_MERGE)),
             'MHGP11SP : role %d' % f.role[b])
        need(q <= m <= (SHELL_CAPACITY if f.version == 1 else 255) and p + q - 1 <= k <= p + m,
             'MHGP11SP : boule %d hors de W_K (p=%d, q=%d, m=%d, K=%d)' % (b, p, q, m, k))
        need(m > q or count == 1, 'MHGP11SP : coquille reguliere a plusieurs supports (boule %d)' % b)
        star_sites = tuple(f.sites[site_at[first]:site_at[first] + q])
        made = sphere_of([pts[i] for i in star_sites])
        need(made is not None, 'MHGP11SP : S* de la boule %d n\'est pas un support positif' % b)
        sphere[b], star[b] = made, star_sites
        for s in range(first + 1, first + count):
            q_pts = [pts[i] for i in f.sites[site_at[s]:site_at[s] + f.arity[s]]]
            need(is_positive_support(q_pts, made), 'MHGP11SP : support %d non positif sur la sphere de S*' % s)
        need(len(set(f.sites[site_at[first]:site_at[first + count - 1] + f.arity[first + count - 1]])) <= m,
             'MHGP11SP : supports sur plus de m sites (boule %d)' % b)
        rank = f.ball_rank[b]
        need(rank >= 1, 'MHGP11SP : rang nul pour une boule')
        level = (made[2], made[1])
        if rank in levels:
            need(_same_level(levels[rank], level), 'MHGP11SP : deux niveaux au rang %d' % rank)
        else:
            levels[rank] = level
    ordered = sorted(levels)
    need(all(_less_level(levels[ordered[i]], levels[ordered[i + 1]]) for i in range(len(ordered) - 1)),
         'MHGP11SP : rangs dans un autre ordre que les niveaux')
    f.qmin, f.sphere, f.star, f.levels = qmin, sphere, star, levels
    # boules d'un noeud : (rang, S* complete par kNone) strictement croissants ; premiere au rang du noeud
    for v in range(f.N):
        own = range(first_ball[v], first_ball[v] + f.ball_count[v])
        keys = [(f.ball_rank[b], star[b] + (NONE,) * (4 - len(star[b]))) for b in own]
        need(all(keys[i] < keys[i + 1] for i in range(len(keys) - 1)), 'MHGP11SP : boules du noeud %d non triees' % v)
        need(not keys or keys[0][0] == f.rank[v], 'MHGP11SP : premiere boule du noeud %d hors de son rang' % v)


def _check_roles(f):
    prior_at, a = [0] * f.B, 0
    for b in range(f.B):
        prior_at[b] = a
        a += f.prior_count[b]
    f.prior_at = prior_at
    regular = {}
    for v in range(f.N):
        own = range(f.first_ball[v], f.first_ball[v] + f.ball_count[v])
        kind, rank = f.kind[v], f.rank[v]
        top = None if v == f.root else f.rank[f.parent[v]]
        union = set()
        merges = births = 0
        for b in own:
            role, r, count = f.role[b], f.ball_rank[b], f.prior_count[b]
            branches = list(f.prior[prior_at[b]:prior_at[b] + count])
            if role == ROLE_BIRTH:
                need(kind == KIND_BIRTH and r == rank and count == 0 and b == own[0],
                     'MHGP11SP : naissance %d hors du rang ou du noeud de naissance %d' % (b, v))
                births += 1
            elif role == ROLE_MERGE:
                need(kind == KIND_MERGE and r == rank and count >= 1 and
                     all(branches[i] < branches[i + 1] for i in range(count - 1)) and
                     set(branches) <= set(f.children[v]), 'MHGP11SP : fusion %d, branches %r, noeud %d' %
                     (b, branches, v))
                union.update(branches)
                merges += 1
            else:
                need(r > rank and (top is None or r < top) and count == 0,
                     'MHGP11SP : boule interne %d hors de la vie du noeud %d' % (b, v))
            if f.version == 2:
                continue  # arbre couvrant : Q_b non publie, le role se juge par les rangs seuls
            key = (f.p[b], f.m[b], f.qmin[b])
            if f.support_count[b] == 1 and f.qmin[b] == f.m[b]:
                if key not in regular:
                    regular[key] = ball_counts(f.k, f.p[b], f.m[b], [tuple(range(f.m[b]))])['strict_traces']
                strict = regular[key]
            else:
                strict = ball_counts(f.k, f.p[b], f.m[b], f.supports_of(b))['strict_traces']
            need((strict == 0) == (role == ROLE_BIRTH), 'MHGP11SP : boule %d, role %d et strict_traces %d'
                 % (b, role, strict))
        need(kind != KIND_BIRTH or births == 1, 'MHGP11SP : naissance %d sans boule de naissance' % v)
        need(kind != KIND_MERGE or (merges >= 1 and union == set(f.children[v])),
             'MHGP11SP : fusion %d : reunion des branches differente des enfants' % v)


def _check_canonical(f):
    """Numerotation canonique : naissances par (niveau, centre), fusions par (niveau, plus petite naissance)."""
    if f.k >= 2:
        for v in range(f.births - 1):
            a, b = f.first_ball[v], f.first_ball[v + 1]
            la, lb = (f.sphere[a][2], f.sphere[a][1]), (f.sphere[b][2], f.sphere[b][1])
            ca, cb = (f.sphere[a][0], f.sphere[a][1]), (f.sphere[b][0], f.sphere[b][1])
            need(_less_level(la, lb) or (_same_level(la, lb) and _less_center(ca, cb)),
                 'MHGP11SP : naissances %d et %d hors de l\'ordre (niveau, centre)' % (v, v + 1))
    low = list(range(f.N))
    for v in range(f.births, f.N):
        low[v] = min(low[c] for c in f.children[v])
    for v in range(f.births, f.N - 1):
        need((f.rank[v], low[v]) < (f.rank[v + 1], low[v + 1]),
             'MHGP11SP : fusions %d et %d hors de l\'ordre (niveau, plus petite naissance)' % (v, v + 1))


# ---------------------------------------------------------------- MHGP11PT version 1 (tranche S9)

PT_HEADER = 8 + 17 * 8
POINTS_COUNT_KEYS = ('sites', 'nodes', 'qualification', 'kappa', 'levels', 'plateaus', 'blocks', 'root_blocks',
                     'delayed', 'strict')


def qualification(k):
    """Seuil de qualification m(K) des points : 1 a K = 1, K + 1 sinon (docs/HIERARCHIE_POINTS.md, paragraphe 9)."""
    return 1 if k == 1 else k + 1


def _points_counts(counts, k):
    _keys(counts, POINTS_COUNT_KEYS, 'counts')
    sites = _integer(counts['sites'], 'sites', k if k == 1 else k + 1, (1 << 32) - 2)
    _integer(counts['nodes'], 'nodes', 1)
    need(counts['qualification'] == qualification(k) and counts['kappa'] == 1, 'manifeste : qualification ou kappa')
    _integer(counts['levels'], 'levels', 1)
    plateaus = _integer(counts['plateaus'], 'plateaus', 1)
    blocks = _integer(counts['blocks'], 'blocks', 1, 2 * sites - 1)
    need(1 <= _integer(counts['root_blocks'], 'root_blocks') <= blocks and plateaus <= 2 * sites + counts['nodes'],
         'manifeste : blocs racines ou plateaux')
    need(_integer(counts['strict'], 'strict') <= _integer(counts['delayed'], 'delayed') <= sites,
         'manifeste : sites stricts et retardes')


def _flat_counts(counts, k):
    _keys(counts, FLAT_COUNT_KEYS, 'counts')
    sites = _integer(counts['sites'], 'sites', k if k == 1 else k + 1, (1 << 32) - 2)
    _integer(counts['nodes'], 'nodes', 1)
    clusters = _integer(counts['clusters'], 'clusters', 0, 2 * sites)
    need(_integer(counts['selected'], 'selected') <= clusters and _integer(counts['noise'], 'noise') <= sites,
         'manifeste : clusters retenus ou bruit')
    decisions = _integer(counts['decisions'], 'decisions', 0, clusters)
    need(_integer(counts['equalities'], 'equalities') <= _integer(counts['exact'], 'exact') <= decisions,
         'manifeste : decisions, replis exacts et egalites')
    _integer(counts['unbracketed'], 'unbracketed')


def read_flat(data):
    """Decode MHGP11ET version 1 (docs/SORTIES.md, paragraphe 7) : magie, version 1, n >= 1, 1 <= K <= 12, mcs >= 2,
    z dans 1..3, selection 0 (EOM) ou 1 (feuilles), taille 56 + 8 n exactement, etiquettes >= -1 (PointId u32 ou
    bruit). Rend dict(n, k, mcs, z, selection, labels) ; leve ValueError a la premiere violation."""
    need(type(data) in (bytes, bytearray) and len(data) >= FLAT_HEADER and data[:8] == b'MHGP11ET', 'MHGP11ET : signature')
    version, n, k, mcs, z, selection = struct.unpack_from('<6Q', data, 8)
    need(version == 1 and 1 <= n < NONE and 1 <= k <= 12 and 2 <= mcs < (1 << 32) and 1 <= z <= 3 and
         selection in (0, 1), 'MHGP11ET : en-tete')
    need(len(data) == FLAT_HEADER + 8 * n, 'MHGP11ET : taille')
    labels = list(struct.unpack_from('<%dq' % n, data, FLAT_HEADER))
    need(all(-1 <= label <= NONE for label in labels), 'MHGP11ET : etiquette hors de -1..2^32-1')
    return dict(n=n, k=k, mcs=mcs, z=z, selection=selection, labels=labels)


def _sqrt_bounds(value, bits):
    """[lo, hi] encadrant sqrt(value) (Fraction >= 0) a 2^-bits pres, par racine entiere."""
    from fractions import Fraction
    n, d = value.numerator, value.denominator
    s = math.isqrt((n * d) << (2 * bits))
    return Fraction(s, d << bits), Fraction(s + 1, d << bits)


def radical_sign(terms, budget=8192):
    """Signe exact de la somme de c sqrt(f) (c entier, f Fraction >= 0), ecrit a neuf pour le lecteur : classes de
    carres (f / g carre parfait : meme classe, radicaux de classes distinctes independants sur Q), puis encadrements
    de precision croissante ; ValueError si une somme non nulle reste non separee dans le budget."""
    from fractions import Fraction
    classes = []
    for coef, value in terms:
        if not coef or not value:
            continue
        for entry in classes:
            ratio = value / entry[0]
            a, b = math.isqrt(ratio.numerator), math.isqrt(ratio.denominator)
            if a * a == ratio.numerator and b * b == ratio.denominator:
                entry[1] += coef * Fraction(a, b)
                break
        else:
            classes.append([value, Fraction(coef)])
    classes = [(value, coef) for value, coef in classes if coef]
    if not classes:
        return 0
    if len(classes) == 1:
        return 1 if classes[0][1] > 0 else -1
    bits = 96
    while bits <= budget:
        lo = hi = Fraction(0)
        for value, coef in classes:
            a, b = _sqrt_bounds(value, bits)
            lo += coef * (a if coef > 0 else b)
            hi += coef * (b if coef > 0 else a)
        if lo > 0:
            return 1
        if hi < 0:
            return -1
        bits *= 2
    raise ValueError('somme de radicaux non separee')


class PointsFile:
    """MHGP11PT decode et controle (read_points). Colonnes : x, y, z, point_id (par SiteIdx) ; ranks, num, den
    (LEVELS) ; parent, rank (par noeud) ; t, M, Q, owner, floor, strict (par site) ; plateau_t, plateau_M,
    plateau_Q, block_plateau, block_parent, site_block, site_plateau (arbre de points). level(r) : niveau exact
    (Fraction) d'un rang reference ; date(s) : (l_t, l_M, l_Q) du site s."""

    def level(self, r):
        need(r in self.levels, 'MHGP11PT : rang %d non reference' % r)
        return self.levels[r]

    def date(self, s):
        return (self.level(self.t[s]), self.level(self.M[s]), self.level(self.Q[s]))

    def plateau(self, p):
        return (self.level(self.plateau_t[p]), self.level(self.plateau_M[p]), self.level(self.plateau_Q[p]))

    def manifest_counts(self):
        return dict(sites=self.n, nodes=self.N, qualification=self.m, kappa=self.kappa, levels=self.L,
                    plateaus=self.P, blocks=self.Bk, root_blocks=sum(1 for p in self.block_parent if p == NONE),
                    delayed=sum(1 for value in self.M if value != 0), strict=sum(self.strict))


def compare_dates(a, b):
    """Ordre exact de deux dates (l_t, l_M, l_Q) : sqrt t + sqrt M - sqrt Q."""
    return radical_sign([(1, a[0]), (1, a[1]), (-1, a[2]), (-1, b[0]), (-1, b[1]), (1, b[2])])


def read_points(data, bits, exact=True):
    """Decode MHGP11PT version 1 et controle (docs/SORTIES.md, paragraphe 7) :
      - en-tete : magie, version 1, profil, 1 <= K <= 12, m = m(K), kappa = 1, K < n a K >= 2, W du profil, decalages
        egaux a la disposition des colonnes, taille egale a celle du fichier ; bourrage nul ;
      - sites : coordonnees dans le profil, ordre de Morton strict, PointId distincts ;
      - LEVELS : rangs strictement croissants, le rang 0 de niveau nul, denominateurs positifs, niveaux strictement
        croissants en valeur exacte (l'ordre des rangs est celui des niveaux) ;
      - arbre : une racine, la derniere ; parent de numero et de rang plus grands ; naissances d'abord, puis fusions
        d'au moins deux enfants ; rangs des noeuds references ;
      - pendaison : sans rival M = Q = 0, plancher t, non strict ; avec rival t <= Q < M et t <= plancher <= M ;
        proprietaire vivant au plancher ; avec exact, plancher et drapeau strict certifies (sqrt l_plancher <= date,
        egalite si et seulement si non strict) ;
      - arbre de points : plateaux references, de la forme (r, 0, 0) ou d'une date (M > Q) ; blocs de parent de numero
        et de plateau plus grands, au moins deux enfants pour un bloc de fusion, une entree a sa creation pour un bloc
        d'entree ; entree de chaque site au plateau (plancher, 0, 0) si non strict, a un plateau de sa date sinon (egalite
        exacte avec exact) ; avec exact, plateaux strictement croissants en valeur exacte.
    Leve ValueError a la premiere violation."""
    from fractions import Fraction
    need(type(data) in (bytes, bytearray) and len(data) >= PT_HEADER and data[:8] == b'MHGP11PT', 'MHGP11PT : signature')
    words = struct.unpack_from('<17Q', data, 8)
    version, coord_bits, k, m, kappa, n, nlevels, width, nodes, plateaus, blocks = words[:11]
    need(version == 1 and coord_bits == bits and bits in (18, 21, 24), 'MHGP11PT : version ou profil')
    need(1 <= k <= 12 and m == qualification(k) and kappa == 1, 'MHGP11PT : K, m ou kappa')
    need(1 <= n < NONE and (k == 1 or k < n) and 1 <= nodes < NONE and nlevels >= 1, 'MHGP11PT : n, N ou L')
    need(width == (4 if bits == 24 else 3), 'MHGP11PT : mots par niveau')
    layout = [PT_HEADER]
    layout.append(layout[-1] + 4 * _pad8(4 * n))
    layout.append(layout[-1] + _pad8(4 * nlevels) + 2 * 8 * width * nlevels)
    layout.append(layout[-1] + 2 * _pad8(4 * nodes))
    layout.append(layout[-1] + 5 * _pad8(4 * n) + _pad8(n))
    layout.append(layout[-1] + 3 * _pad8(4 * plateaus) + 2 * _pad8(4 * blocks) + 2 * _pad8(4 * n))
    need(list(words[11:17]) == layout and layout[-1] == len(data), 'MHGP11PT : decalages ou taille')
    f = PointsFile()
    f.bits, f.k, f.m, f.kappa, f.n, f.L, f.W, f.N, f.P, f.Bk = bits, k, m, kappa, n, nlevels, width, nodes, plateaus, \
        blocks
    at = layout[0]
    for name in ('x', 'y', 'z', 'point_id'):
        values, at = _column(data, at, n, 4, name)
        setattr(f, name, values)
    f.ranks, at = _column(data, at, nlevels, 4, 'levels.rank')
    limbs = struct.unpack_from('<%dQ' % (2 * width * nlevels), data, at)
    at += 16 * width * nlevels
    f.levels = {}
    for j, r in enumerate(f.ranks):
        num = sum(limbs[j * width + w] << (64 * w) for w in range(width))
        den = sum(limbs[(nlevels + j) * width + w] << (64 * w) for w in range(width))
        need(den > 0, 'MHGP11PT : denominateur nul')
        f.levels[r] = Fraction(num, den)
    for name in ('parent', 'rank'):
        values, at = _column(data, at, nodes, 4, name)
        setattr(f, name, values)
    for name in ('t', 'M', 'Q', 'owner', 'floor'):
        values, at = _column(data, at, n, 4, name)
        setattr(f, name, values)
    f.strict, at = _column(data, at, n, 1, 'strict')
    for name, count in (('plateau_t', plateaus), ('plateau_M', plateaus), ('plateau_Q', plateaus),
                        ('block_plateau', blocks), ('block_parent', blocks), ('site_block', n), ('site_plateau', n)):
        values, at = _column(data, at, count, 4, name)
        setattr(f, name, values)
    need(at == len(data), 'MHGP11PT : colonnes et taille')
    _check_points_sites(f)
    _check_points_levels(f)
    _check_points_tree(f)
    _check_hanging(f, exact)
    _check_point_tree(f, exact)
    return f


def _check_points_sites(f):
    limit = 1 << f.bits
    points = list(zip(f.x, f.y, f.z))
    need(all(c < limit for p in points for c in p), 'MHGP11PT : coordonnee hors du profil')
    keys = [morton(p) for p in points]
    need(all(keys[i] < keys[i + 1] for i in range(len(keys) - 1)), 'MHGP11PT : sites hors de l\'ordre de Morton')
    need(len(set(f.point_id)) == f.n, 'MHGP11PT : PointId en double')


def _check_points_levels(f):
    ranks = list(f.ranks)
    need(ranks[0] == 0 and f.levels[0] == 0, 'MHGP11PT : rang 0 de niveau nul')
    need(all(ranks[i] < ranks[i + 1] and f.levels[ranks[i]] < f.levels[ranks[i + 1]] for i in range(len(ranks) - 1)),
         'MHGP11PT : rangs ou niveaux non strictement croissants')


def _check_points_tree(f):
    need(f.parent[f.N - 1] == NONE, 'MHGP11PT : racine')
    children = [0] * f.N
    for v in range(f.N - 1):
        up = f.parent[v]
        need(v < up < f.N and f.rank[v] < f.rank[up], 'MHGP11PT : parent du noeud %d' % v)
        children[up] += 1
    births = sum(1 for c in children if c == 0)
    need(all(children[v] == 0 for v in range(births)) and all(children[v] >= 2 for v in range(births, f.N)),
         'MHGP11PT : naissances puis fusions d\'au moins deux enfants')
    need(all(r in f.levels for r in f.rank), 'MHGP11PT : rang de noeud non reference')
    f.births = births


def _check_hanging(f, exact):
    for s in range(f.n):
        t, big_m, q, owner, floor, strict = f.t[s], f.M[s], f.Q[s], f.owner[s], f.floor[s], f.strict[s]
        need(all(r in f.levels for r in (t, big_m, q, floor)) and owner < f.N and strict in (0, 1),
             'MHGP11PT : site %d hors domaine' % s)
        if big_m == 0:
            need(q == 0 and floor == t and strict == 0, 'MHGP11PT : site %d sans rival' % s)
        else:
            need(t <= q < big_m and t <= floor <= big_m, 'MHGP11PT : site %d : t <= Q < M, t <= plancher <= M' % s)
        up = f.parent[owner]
        need(f.rank[owner] <= floor and (up == NONE or f.rank[up] > floor),
             'MHGP11PT : proprietaire du site %d non vivant au plancher' % s)
        if exact and big_m != 0:
            sign = radical_sign([(1, f.levels[t]), (1, f.levels[big_m]), (-1, f.levels[q]), (-1, f.levels[floor])])
            need(sign >= 0 and (sign > 0) == (strict == 1), 'MHGP11PT : plancher ou drapeau strict du site %d' % s)


def _check_point_tree(f, exact):
    plateau_of = []
    for p in range(f.P):
        t, big_m, q = f.plateau_t[p], f.plateau_M[p], f.plateau_Q[p]
        need(all(r in f.levels for r in (t, big_m, q)) and ((big_m == 0 and q == 0) or q < big_m),
             'MHGP11PT : plateau %d' % p)
        plateau_of.append((t, big_m, q))
        if exact and p > 0:
            need(compare_dates(f.plateau(p - 1), f.plateau(p)) < 0, 'MHGP11PT : plateaux %d et %d' % (p - 1, p))
    kids = [0] * f.Bk
    for b in range(f.Bk):
        up = f.block_parent[b]
        need(f.block_plateau[b] < f.P and (up == NONE or (b < up < f.Bk and f.block_plateau[b] < f.block_plateau[up])),
             'MHGP11PT : bloc %d' % b)
        if up != NONE:
            kids[up] += 1
    created = [False] * f.Bk
    for s in range(f.n):
        b, p = f.site_block[s], f.site_plateau[s]
        need(b < f.Bk and p < f.P and f.block_plateau[b] <= p, 'MHGP11PT : entree du site %d' % s)
        created[b] = created[b] or f.block_plateau[b] == p
        if f.strict[s] == 0:
            need(plateau_of[p] == (f.floor[s], 0, 0), 'MHGP11PT : plateau du site non strict %d' % s)
        else:
            need(plateau_of[p][1] != 0 and (plateau_of[p] == (f.t[s], f.M[s], f.Q[s]) or
                                            (exact and compare_dates(f.plateau(p), f.date(s)) == 0)),
                 'MHGP11PT : plateau du site strict %d' % s)
    need(all(kids[b] >= 2 or (kids[b] == 0 and created[b]) for b in range(f.Bk)),
         'MHGP11PT : bloc ni fusion d\'au moins deux blocs ni cree par une entree')
