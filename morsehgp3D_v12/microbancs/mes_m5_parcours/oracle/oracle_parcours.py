#!/usr/bin/env python3
"""Oracle du parcours des boites du catalogue de la v11, en entiers Python exacts (microbanc MES-M5, hors produit).

Reecriture independante, a arithmetique volontairement autre (entiers de taille arbitraire, aucun type borne), des regles
de morsehgp3D_v11/src/catalogue/boxes.cpp (moteur gele ac081a06f) : racine = tous les sites et leur enveloppe
[min, max+1) ; noeud = reservoir des min(|L|, 3K) sites les plus proches du centre de la boite (cle : somme des
(2x - lo - hi)^2, puis rang dans la liste parente), filtre G1 (un site est retire s'il a K dominateurs stricts parmi les
temoins, examines dans l'ordre avec arret au K-ieme), boite ajustee = enveloppe de la liste inter boite, coupe au
milieu de l'axe le plus large (le premier a egalite), feuille si |liste| <= taille de feuille ou largeur <= 1, refus si
une feuille depasse max_leaf ou si une profondeur depasse 3B. Il ecrit et relit le format MHGP12TR v1
(include/mhgp12/traversal/format.hpp).

Deux usages :
  oracle_parcours.py --check D.bin [--report R.json]
      recalcule le parcours depuis le nuage et les parametres du vidage D (v11 ou oracle) et compare TOUT : statut,
      grand livre, noeuds (boites, chemins, tests, candidats, retenus, genres, empreintes de listes), feuilles, sites ;
  oracle_parcours.py --make --cloud C.u32le --kmax K --leaf L [--max-leaf M] --bits B --out D.bin
      ecrit le vidage de reference d'un nuage (u32 petit-boutistes x, y, z entrelaces, ordre SiteIdx = ordre du fichier).
Bibliotheque standard seule ; aucune garde par assert (tient sous python3 -O). Codes : 0 conforme, 1 ecart, 2 refus.
"""

import argparse
import json
import struct
import sys

MAGIC = b'MHGP12TR'
VERSION = 1
PRODUCER_ORACLE = 2
STATUS_OK, STATUS_WIDE_LEAF, STATUS_DEPTH = 0, 1, 2
KIND_EMPTY, KIND_LEAF, KIND_SPLIT = 0, 1, 2
HEADER = struct.Struct('<8s16Q15Q')
NODE = struct.Struct('<3q3q2QQQIIII')
LEAF = struct.Struct('<QII3q3q2Q')
FNV_BASIS = 14695981039346656037
FNV_PRIME = 1099511628211
MASK64 = (1 << 64) - 1


class Refusal(Exception):
  def __init__(self, status):
    Exception.__init__(self, status)
    self.status = status


def fnv1a(h, data):
  for b in data:
    h ^= b
    h = (h * FNV_PRIME) & MASK64
  return h


def list_fnv(sites):
  if not sites:
    return 0
  return fnv1a(FNV_BASIS, struct.pack('<%dI' % len(sites), *sites))


def walk(xs, ys, zs, kmax, leaf_size, max_leaf, bits):
  """Parcours de la v11 : (statut, noeuds, feuilles, sites, grand livre) ; noeuds et feuilles en ordre prefixe."""
  pts = list(zip(xs, ys, zs))
  n = len(pts)
  nodes, leaves, sites = [], [], []
  ledger = {'nodes': 0, 'leaves': 0, 'filter_tests': 0, 'max_depth': 0, 'max_leaf': 0}
  max_depth = 3 * bits

  def key(p, lo, hi):
    return sum((2 * p[a] - lo[a] - hi[a]) ** 2 for a in range(3))

  def terms(p, lo, hi):
    d = [p[a] - lo[a] for a in range(3)]
    return [2 * (hi[a] - lo[a]) * d[a] for a in range(3)], sum(v * v for v in d)

  def dominated_by(x, y):  # y domine strictement x sur la fermeture de la boite
    right = sum(max(0, x[0][a] - y[0][a]) for a in range(3))
    return x[1] - y[1] > right

  def process(parent, lo, hi, depth, path):
    if depth > max_depth:
      raise Refusal(STATUS_DEPTH)
    ledger['nodes'] += 1
    ledger['max_depth'] = max(ledger['max_depth'], depth)
    cap = min(len(parent), 3 * kmax)
    order = sorted(range(len(parent)), key=lambda i: (key(pts[parent[i]], lo, hi), i))[:cap]
    witnesses = [terms(pts[parent[i]], lo, hi) for i in order]
    kept, tests = [], 0
    for s in parent:
      x = terms(pts[s], lo, hi)
      found, i = 0, 0
      while i < len(witnesses) and found < kmax:
        if dominated_by(x, witnesses[i]):
          found += 1
        i += 1
      tests += i
      if found < kmax:
        kept.append(s)
    ledger['filter_tests'] += tests
    node = {'lo': list(lo), 'hi': list(hi), 'path': list(path), 'tests': tests, 'candidates': len(parent),
            'count': 0, 'depth': depth, 'kind': KIND_EMPTY, 'fnv': 0}
    nodes.append(node)
    if not kept:
      return
    alo = [max(min(pts[s][a] for s in kept), lo[a]) for a in range(3)]
    ahi = [min(max(pts[s][a] for s in kept) + 1, hi[a]) for a in range(3)]
    if any(alo[a] >= ahi[a] for a in range(3)):
      return
    node['count'] = len(kept)
    node['fnv'] = list_fnv(kept)
    axis = 0
    for a in (1, 2):
      if ahi[a] - alo[a] > ahi[axis] - alo[axis]:
        axis = a
    width = ahi[axis] - alo[axis]
    if len(kept) <= leaf_size or width <= 1:
      node['kind'] = KIND_LEAF
      ledger['leaves'] += 1
      ledger['max_leaf'] = max(ledger['max_leaf'], len(kept))
      if len(kept) > max_leaf:
        raise Refusal(STATUS_WIDE_LEAF)
      leaves.append({'begin': len(sites), 'm': len(kept), 'depth': depth, 'lo': alo, 'hi': ahi, 'path': list(path)})
      sites.extend(kept)
      return
    node['kind'] = KIND_SPLIT
    middle = alo[axis] + width // 2
    left_hi, right_lo = list(ahi), list(alo)
    left_hi[axis] = middle
    right_lo[axis] = middle
    process(kept, alo, left_hi, depth + 1, path)
    right = list(path)
    right[depth // 64] |= 1 << (63 - depth % 64)
    process(kept, right_lo, ahi, depth + 1, right)

  lo = [min(c) for c in (xs, ys, zs)]
  hi = [max(c) + 1 for c in (xs, ys, zs)]
  try:
    process(list(range(n)), lo, hi, 0, [0, 0])
  except Refusal as r:
    return r.status, [], [], [], {'nodes': 0, 'leaves': 0, 'filter_tests': 0, 'max_depth': 0, 'max_leaf': 0}
  return STATUS_OK, nodes, leaves, sites, ledger


def pad8(b):
  return b + b'\0' * ((8 - len(b) % 8) % 8)


def encode(header, xs, ys, zs, nodes, leaves, sites):
  body = HEADER.pack(MAGIC, VERSION, header['producer'], header['coord_bits'], header['kmax'], header['leaf_size'],
                     header['max_leaf'], len(xs), len(nodes), len(leaves), len(sites), header['status'],
                     header['ledger']['nodes'], header['ledger']['leaves'], header['ledger']['filter_tests'],
                     header['ledger']['max_depth'], header['ledger']['max_leaf'], *([0] * 15))
  for col in (xs, ys, zs):
    body += pad8(struct.pack('<%dI' % len(col), *col))
  body += pad8(b''.join(NODE.pack(*n['lo'], *n['hi'], *n['path'], n['tests'], n['fnv'], n['candidates'], n['count'],
                                  n['depth'], n['kind']) for n in nodes))
  body += pad8(b''.join(LEAF.pack(l['begin'], l['m'], l['depth'], *l['lo'], *l['hi'], *l['path']) for l in leaves))
  body += pad8(struct.pack('<%dI' % len(sites), *sites))
  return body + struct.pack('<Q', fnv1a(FNV_BASIS, body))


def decode(data):
  """Lecture du vidage (controles structurels minimaux : le lecteur strict est celui du C++)."""
  if len(data) < HEADER.size + 8:
    raise ValueError('vidage trop court')
  f = HEADER.unpack_from(data, 0)
  if f[0] != MAGIC or f[1] != VERSION:
    raise ValueError('en-tete MHGP12TR v1 invalide')
  if struct.unpack_from('<Q', data, len(data) - 8)[0] != fnv1a(FNV_BASIS, data[:-8]):
    raise ValueError('FNV-1a invalide')
  (producer, bits, kmax, leaf_size, max_leaf, n_sites, n_nodes, n_leaves, n_sites_leaf, status,
   l_nodes, l_leaves, l_tests, l_depth, l_leaf) = f[2:17]
  at = HEADER.size
  cols = []
  for _ in range(3):
    cols.append(list(struct.unpack_from('<%dI' % n_sites, data, at)))
    at += (4 * n_sites + 7) // 8 * 8
  nodes = []
  for i in range(n_nodes):
    v = NODE.unpack_from(data, at + i * NODE.size)
    nodes.append({'lo': list(v[0:3]), 'hi': list(v[3:6]), 'path': list(v[6:8]), 'tests': v[8], 'fnv': v[9],
                  'candidates': v[10], 'count': v[11], 'depth': v[12], 'kind': v[13]})
  at += (NODE.size * n_nodes + 7) // 8 * 8
  leaves = []
  for i in range(n_leaves):
    v = LEAF.unpack_from(data, at + i * LEAF.size)
    leaves.append({'begin': v[0], 'm': v[1], 'depth': v[2], 'lo': list(v[3:6]), 'hi': list(v[6:9]),
                   'path': list(v[9:11])})
  at += (LEAF.size * n_leaves + 7) // 8 * 8
  sites = list(struct.unpack_from('<%dI' % n_sites_leaf, data, at))
  header = {'producer': producer, 'coord_bits': bits, 'kmax': kmax, 'leaf_size': leaf_size, 'max_leaf': max_leaf,
            'status': status, 'ledger': {'nodes': l_nodes, 'leaves': l_leaves, 'filter_tests': l_tests,
                                         'max_depth': l_depth, 'max_leaf': l_leaf}}
  return header, cols, nodes, leaves, sites


def check(path):
  with open(path, 'rb') as f:
    data = f.read()
  header, (xs, ys, zs), nodes, leaves, sites = decode(data)
  status, n2, l2, s2, ledger = walk(xs, ys, zs, header['kmax'], header['leaf_size'], header['max_leaf'],
                                    header['coord_bits'])
  diffs = []
  if status != header['status']:
    diffs.append('statut %d / %d' % (status, header['status']))
  if ledger != header['ledger']:
    diffs.append('grand livre %s / %s' % (ledger, header['ledger']))
  if len(n2) != len(nodes):
    diffs.append('noeuds %d / %d' % (len(n2), len(nodes)))
  for i, (a, b) in enumerate(zip(n2, nodes)):
    if a != b:
      diffs.append('noeud %d : %s / %s' % (i, a, b))
      break
  if l2 != leaves:
    diffs.append('feuilles differentes (%d / %d)' % (len(l2), len(leaves)))
  if s2 != sites:
    diffs.append('sites des feuilles differents')
  return {'dump': path, 'producer': header['producer'], 'sites': len(xs), 'kmax': header['kmax'],
          'leaf_size': header['leaf_size'], 'max_leaf': header['max_leaf'], 'coord_bits': header['coord_bits'],
          'status': status, 'ledger': ledger, 'nodes': len(n2), 'leaves': len(l2), 'identity': not diffs,
          'differences': diffs[:5]}


def make(cloud_path, kmax, leaf, max_leaf, bits, out_path):
  with open(cloud_path, 'rb') as f:
    raw = f.read()
  if len(raw) == 0 or len(raw) % 12 != 0:
    raise ValueError('nuage : taille non multiple de 12 octets')
  vals = struct.unpack('<%dI' % (len(raw) // 4), raw)
  xs, ys, zs = list(vals[0::3]), list(vals[1::3]), list(vals[2::3])
  if not 1 <= bits <= 32 or any(v >= (1 << bits) for v in vals):
    raise ValueError('coordonnee hors du profil')
  if len(set(zip(xs, ys, zs))) != len(xs):
    raise ValueError('sites non distincts (decision D8)')
  if not (1 <= kmax <= 12 and kmax + 3 <= leaf <= max_leaf <= 1024):
    raise ValueError('parametres hors domaine')
  status, nodes, leaves, sites, ledger = walk(xs, ys, zs, kmax, leaf, max_leaf, bits)
  header = {'producer': PRODUCER_ORACLE, 'coord_bits': bits, 'kmax': kmax, 'leaf_size': leaf, 'max_leaf': max_leaf,
            'status': status, 'ledger': ledger}
  with open(out_path, 'wb') as f:
    f.write(encode(header, xs, ys, zs, nodes, leaves, sites))
  return {'out': out_path, 'sites': len(xs), 'status': status, 'ledger': ledger, 'nodes': len(nodes),
          'leaves': len(leaves)}


def main(argv):
  ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
  ap.add_argument('--check')
  ap.add_argument('--make', action='store_true')
  ap.add_argument('--cloud')
  ap.add_argument('--kmax', type=int)
  ap.add_argument('--leaf', type=int)
  ap.add_argument('--max-leaf', type=int, default=256)
  ap.add_argument('--bits', type=int)
  ap.add_argument('--out')
  ap.add_argument('--report')
  try:
    args = ap.parse_args(argv[1:])
  except SystemExit:
    return 2
  try:
    if args.check:
      result = check(args.check)
    elif args.make and args.cloud and args.out and args.kmax and args.leaf and args.bits:
      result = make(args.cloud, args.kmax, args.leaf, args.max_leaf, args.bits, args.out)
      result['identity'] = True
    else:
      return 2
  except (OSError, ValueError, struct.error) as e:
    print('oracle_refus : %s' % e, file=sys.stderr)
    return 2
  text = json.dumps(result, sort_keys=True)
  if args.report:
    with open(args.report, 'w', encoding='utf-8') as f:
      f.write(text + '\n')
  print(text)
  return 0 if result.get('identity') else 1


if __name__ == '__main__':
  sys.exit(main(sys.argv))
