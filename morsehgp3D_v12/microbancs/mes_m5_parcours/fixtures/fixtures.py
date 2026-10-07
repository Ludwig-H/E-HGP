#!/usr/bin/env python3
"""Fixtures synthetiques du microbanc MES-M5 (parcours des boites) : nuages, entrees et vidages de reference.

Toutes les fixtures sont synthetiques (aucune donnee SemanticKITTI) et deterministes ; elles se regenerent en quelques
secondes. Deux familles :
  - u21, reference = v11 gelee (outil mhgp12_traversal_dump), recontrolee par l'oracle Python (--check) :
      coquille24_k2_l16    24 permutations signees de (1,2,2), echelle 2^18, translation 2^19, K2/16 : profondeur 60
                           (temoin CST-0205 de l'auditeur) ;
      coquille48_k5_l24    48 permutations signees de (1,2,3), echelle 2^18, translation 3*2^18, K5/24 : profondeur 63 ;
      coquille48_k5_l8_m8  meme nuage, K5, feuille 8, max_leaf 8 : refus wide_leaf (aucun prefixe publie) ;
  - u32, reference = oracle Python (entiers exacts ; la v11 n'existe pas au-dela de 24 bits) :
      coquille48_u32_k5_l24 la coquille de 48 sites a l'echelle 2^29 autour de (2^31)^3 : listes de sites a ~2^31.9
                           de boites de plus en plus petites ; repere du parent s >= 32 (voie large i128), repere de la
                           seule boite de l'enfant etroit : tue le mutant repere_enfant (CONTRAT_NUMERIQUE § 7) ;
      bord_u32_k2_l5       sites (0,0,0) et (2^32-1,0,0) et 14 autres : boite racine [0, 2^32), fermeture a 33 bits
                           (CST-0204) ;
      uniforme_u32_k3_l8   300 sites uniformes dans [0, 2^32)^3 (graine fixe) : voies large et native dans un meme
                           parcours.
Les sites d'une fixture u32 sont ranges dans l'ordre d'une cle de Morton exacte sur 96 bits (ordre SiteIdx du vidage).

Usage : fixtures.py --out DOSSIER [--dump-tool mhgp12_traversal_dump] [--only nom,nom]
  ecrit <nom>.u32le / <nom>.ids.u32le, puis <nom>.bin (reference) et fixtures.json (manifeste : parametres, empreintes,
  controles de l'oracle). Sans --dump-tool, les fixtures u21 sont sautees et declarees absentes au manifeste.
Bibliotheque standard seule (Python 3.10 nu), aucune garde par assert. Codes : 0 conforme, 1 controle en echec, 2 refus.
"""

import argparse
import hashlib
import itertools
import json
import os
import random
import struct
import subprocess
import sys
from pathlib import Path

sys.dont_write_bytecode = True  # aucun __pycache__ dans les sources (paquet de session)
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'oracle'))
import oracle_parcours as oracle  # noqa: E402  (meme microbanc)


def shell(base, scale, offset):
  return sorted({tuple(offset + sign * x * scale for sign, x in zip(signs, p))
                 for p in set(itertools.permutations(base))
                 for signs in itertools.product((-1, 1), repeat=3)})


def morton96(p):
  key = 0
  for bit in range(32):
    for a in range(3):
      key |= ((p[a] >> bit) & 1) << (3 * bit + a)
  return key


def fixtures():
  out = []
  out.append({'name': 'coquille24_k2_l16', 'bits': 21, 'kmax': 2, 'leaf': 16, 'max_leaf': 256,
              'points': shell((1, 2, 2), 1 << 18, 1 << 19), 'reference': 'v11', 'expected_depth': 60})
  s48 = shell((1, 2, 3), 1 << 18, 3 << 18)
  out.append({'name': 'coquille48_k5_l24', 'bits': 21, 'kmax': 5, 'leaf': 24, 'max_leaf': 256, 'points': s48,
              'reference': 'v11', 'expected_depth': 63})
  out.append({'name': 'coquille48_k5_l8_m8', 'bits': 21, 'kmax': 5, 'leaf': 8, 'max_leaf': 8, 'points': s48,
              'reference': 'v11', 'expected_status': oracle.STATUS_WIDE_LEAF})
  big = sorted(shell((1, 2, 3), 1 << 29, 1 << 31), key=morton96)
  out.append({'name': 'coquille48_u32_k5_l24', 'bits': 32, 'kmax': 5, 'leaf': 24, 'max_leaf': 256, 'points': big,
              'reference': 'oracle', 'must_kill': ['repere_enfant']})
  rng = random.Random(20261007)
  edge = [(0, 0, 0), ((1 << 32) - 1, 0, 0)]
  while len(edge) < 16:
    p = (rng.randrange(1 << 32), rng.randrange(1 << 8), rng.randrange(1 << 8))
    if p not in edge:
      edge.append(p)
  out.append({'name': 'bord_u32_k2_l5', 'bits': 32, 'kmax': 2, 'leaf': 5, 'max_leaf': 256,
              'points': sorted(edge, key=morton96), 'reference': 'oracle', 'expected_root_frame_bits': 33})
  rng = random.Random(7)
  uni = set()
  while len(uni) < 300:
    uni.add((rng.randrange(1 << 32), rng.randrange(1 << 32), rng.randrange(1 << 32)))
  out.append({'name': 'uniforme_u32_k3_l8', 'bits': 32, 'kmax': 3, 'leaf': 8, 'max_leaf': 256,
              'points': sorted(uni, key=morton96), 'reference': 'oracle'})
  return out


def sha256_file(path):
  h = hashlib.sha256()
  with open(path, 'rb') as f:
    for block in iter(lambda: f.read(1 << 20), b''):
      h.update(block)
  return h.hexdigest()


def write_input(fx, folder):
  pts = fx['points']
  xyz = folder / (fx['name'] + '.u32le')
  ids = folder / (fx['name'] + '.ids.u32le')
  with open(xyz, 'wb') as f:
    f.write(b''.join(struct.pack('<3I', *p) for p in pts))
  with open(ids, 'wb') as f:
    f.write(struct.pack('<%dI' % len(pts), *range(len(pts))))
  return xyz, ids


def main(argv):
  ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
  ap.add_argument('--out', required=True)
  ap.add_argument('--dump-tool')
  ap.add_argument('--only')
  try:
    args = ap.parse_args(argv[1:])
  except SystemExit:
    return 2
  folder = Path(args.out)
  folder.mkdir(parents=True, exist_ok=True)
  only = set(args.only.split(',')) if args.only else None
  manifest, code = [], 0
  for fx in fixtures():
    if only is not None and fx['name'] not in only:
      continue
    pts = fx['points']
    if len(set(pts)) != len(pts) or any(not 0 <= v < (1 << fx['bits']) for p in pts for v in p):
      print('fixture invalide : %s' % fx['name'], file=sys.stderr)
      return 2
    xyz, ids = write_input(fx, folder)
    entry = {k: v for k, v in fx.items() if k != 'points'}
    entry['sites'] = len(pts)
    entry['input_sha256'] = sha256_file(xyz)
    dump = folder / (fx['name'] + '.bin')
    if dump.exists():
      dump.unlink()
    if fx['reference'] == 'v11':
      if not args.dump_tool:
        entry['present'] = False
        manifest.append(entry)
        continue
      cmd = [args.dump_tool, str(xyz), str(ids), str(fx['kmax']), str(fx['leaf']), str(dump), '--max-leaf',
             str(fx['max_leaf'])]
      proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
      entry['dump_tool_code'] = proc.returncode
      summary = None
      for line in proc.stdout.decode(errors='replace').splitlines():
        try:
          summary = json.loads(line)
        except ValueError:
          pass
      entry['dump_tool'] = summary
      if proc.returncode != 0 or not dump.is_file():
        entry['present'] = False
        code = 1
        manifest.append(entry)
        continue
    else:
      oracle.make(str(xyz), fx['kmax'], fx['leaf'], fx['max_leaf'], fx['bits'], str(dump))
    entry['present'] = True
    entry['dump_sha256'] = sha256_file(dump)
    check = oracle.check(str(dump))
    entry['oracle_check'] = check
    ok = check['identity']
    if 'expected_depth' in fx:
      ok = ok and check['ledger']['max_depth'] == fx['expected_depth']
    if 'expected_status' in fx:
      ok = ok and check['status'] == fx['expected_status']
    if 'expected_root_frame_bits' in fx:
      lo = [min(p[a] for p in pts) for a in range(3)]
      hi = [max(p[a] for p in pts) + 1 for a in range(3)]
      ok = ok and max(hi[a] - lo[a] for a in range(3)).bit_length() == fx['expected_root_frame_bits']
    entry['controls_ok'] = ok
    if not ok:
      code = 1
    manifest.append(entry)
  with open(folder / 'fixtures.json', 'w', encoding='utf-8') as f:
    json.dump({'schema': 'ehgp.v12.mes_m5.fixtures.v1', 'fixtures': manifest}, f, indent=1, sort_keys=True)
  print(json.dumps({'fixtures': len(manifest), 'present': sum(1 for e in manifest if e.get('present')),
                    'controls_ok': all(e.get('controls_ok', False) for e in manifest if e.get('present')),
                    'code': code}))
  return code


if __name__ == '__main__':
  sys.exit(main(sys.argv))
