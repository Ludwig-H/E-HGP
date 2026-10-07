#!/usr/bin/env python3
"""Temoin WIT-FORME-NIVEAU : les octets du vidage MHGP11FUL1 dependent du departage de S*, pas son empreinte semantique.

Constat de l'agent de la tranche T2-b (7 octobre 2026) contre le contrat de la tour (paragraphe 1, commit 5f797660d,
corrige au commit 58d384721) : le vidage MHGP11FUL1 ecrit chaque niveau sous la forme NON reduite de la premiere boule
de son rang (sphere.cpp de la v11 gelee : paire |u|^2 / 4 ; triangle |u|^2 |v|^2 |v - u|^2 / (4 |u x v|^2)). A niveau
egal, l'ordre des boules suit le departage de S* : rangs de Morton en v11, positions triees en v12. Ici, a K = 2, une
paire et un triangle aigu ont le meme niveau 25 ; le triangle ouvre le rang en Morton, la paire en positions : formes
409600/16384 contre 100/4, meme valeur. MES-M0 se juge donc par l'empreinte semantique (rationnels reduits).

Python nu (fractions, itertools, json), aucune dependance, aucun flottant, aucun assert (tient sous python3 -O).
Codes : 0 conforme ; 2 usage faux ou fixture invalide ; 3 fait grave viole ; 4 mutant tue.
Usage : test_witness_forme.py [--mutant=ordre_unique] [--fixture=CHEMIN]
"""

import itertools
import json
import sys
from fractions import Fraction
from pathlib import Path

DEFAULT_FIXTURE = Path(__file__).resolve().parent / "fixtures" / "wit_forme_niveau.json"
MUTANTS = ("ordre_unique",)
COORD_BITS = 21  # cle de Morton du moteur : x au bit 0, y au bit 1, z au bit 2 de chaque niveau


class Violation(Exception):
  pass


def sub(a, b):
  return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
  return sum(x * y for x, y in zip(a, b))


def cross(a, b):
  return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def circumball(pts):
  """Sphere de plus petit rayon passant par 1 a 3 points (centre dans leur plan) ; None si les points sont alignes."""
  a = tuple(Fraction(x) for x in pts[0])
  if len(pts) == 1:
    return a, Fraction(0)
  u = sub(tuple(Fraction(x) for x in pts[1]), a)
  if len(pts) == 2:
    x = tuple(c / 2 for c in u)
  else:
    v = sub(tuple(Fraction(x) for x in pts[2]), a)
    w = cross(u, v)
    if dot(w, w) == 0:
      return None
    t = tuple(dot(u, u) * vi - dot(v, v) * ui for ui, vi in zip(u, v))
    x = tuple(c / (2 * dot(w, w)) for c in cross(t, w))
  return tuple(ai + xi for ai, xi in zip(a, x)), dot(x, x)


def meb(points, part):
  """Plus petite boule englobante exacte de points[part] (au plus trois points), par enumeration des supports."""
  best = None
  for size in range(1, len(part) + 1):
    for tup in itertools.combinations(part, size):
      ball = circumball([points[i] for i in tup])
      if ball is None:
        continue
      center, r2 = ball
      if all(dot(sub(points[i], center), sub(points[i], center)) <= r2 for i in part):
        if best is None or r2 < best[1]:
          best = (center, r2)
  return best


def form(points, support):
  """Forme non reduite du niveau ecrite par la v11 (sphere.cpp) : (numerateur, denominateur)."""
  a = points[support[0]]
  u = sub(points[support[1]], a)
  if len(support) == 2:
    return dot(u, u), 4
  v = sub(points[support[2]], a)
  w = cross(u, v)
  return dot(u, u) * dot(v, v) * dot(sub(v, u), sub(v, u)), 4 * dot(w, w)


def morton(p):
  key = 0
  for bit in range(COORD_BITS):
    for axis in range(3):
      key |= ((p[axis] >> bit) & 1) << (3 * bit + axis)
  return key


def first_in_rank(points, balls, convention):
  """Premiere boule du rang : S* compare comme liste triee de rangs de Morton (v11) ou de positions (v12), bourree
  par un element plus grand que tout."""
  ranks = {i: r for r, i in enumerate(sorted(range(len(points)), key=lambda i: (morton(points[i]), i)))}

  def key(ball):
    if convention == "morton":
      items = sorted(ranks[i] for i in ball["support"])
      return items + [len(points)] * (4 - len(items))
    items = sorted(points[i] for i in ball["support"])
    return items + [(1 << COORD_BITS,) * 3] * (4 - len(items))

  return min(balls, key=key)["name"], [ranks[i] for i in range(len(points))]


def check(fixture, mutant):
  points = [tuple(int(c) for c in p) for p in fixture["points"]]
  k = fixture["k"]
  level = Fraction(fixture["level"])
  balls = fixture["balls"]
  facts = 0

  # 1. chaque boule : centre et niveau exacts, plus petite boule de son support, support minimal (q_min), coquille
  #    egale au support (S* unique dans les deux conventions), boule positive de Cat_K (p + q_min <= K + 1)
  for ball in balls:
    support = ball["support"]
    declared = (tuple(Fraction(c) for c in ball["center"]), level)
    if meb(points, support) != declared:
      raise Violation(f"{ball['name']} : plus petite boule du support differente de la declaration")
    for size in range(1, len(support)):
      for part in itertools.combinations(support, size):
        if meb(points, list(part)) == declared:
          raise Violation(f"{ball['name']} : support non minimal")
    distances = [dot(sub(p, declared[0]), sub(p, declared[0])) for p in points]
    inside = [i for i, d in enumerate(distances) if d < level]
    shell = [i for i, d in enumerate(distances) if d == level]
    if shell != sorted(support) or len(support) < 2 or len(inside) + len(support) > k + 1:
      raise Violation(f"{ball['name']} : coquille {shell}, interieur {inside}, hors de Cat_{k}")
    facts += 1

  # 2. formes non reduites de la v11 : meme valeur, couples differents
  forms = {ball["name"]: form(points, ball["support"]) for ball in balls}
  for ball in balls:
    if forms[ball["name"]] != tuple(ball["form"]) or Fraction(*forms[ball["name"]]) != level:
      raise Violation(f"{ball['name']} : forme {forms[ball['name']]} differente de l'attendu")
  facts += 1

  # 3. premiere boule du rang dans chaque convention, rangs de Morton graves
  first_v11, ranks = first_in_rank(points, balls, "morton")
  first_v12, _ranks = first_in_rank(points, balls, "morton" if mutant == "ordre_unique" else "positions")
  if ranks != fixture["morton_ranks"]:
    raise Violation(f"rangs de Morton {ranks} differents de l'attendu")
  if first_v11 != fixture["first_in_rank"]["morton"] or first_v12 != fixture["first_in_rank"]["positions"]:
    raise Violation(f"premieres boules du rang : {first_v11} (Morton), {first_v12} (positions)")
  facts += 1

  # 4. consequence : la forme ecrite pour le rang change d'une convention a l'autre, pas sa valeur
  written_v11, written_v12 = forms[first_v11], forms[first_v12]
  if written_v11 == written_v12 or Fraction(*written_v11) != Fraction(*written_v12):
    raise Violation(f"formes ecrites {written_v11} et {written_v12} : la lecture refutee tiendrait")
  facts += 1
  return facts, written_v11, written_v12


def main(argv):
  mutant = None
  path = DEFAULT_FIXTURE
  for arg in argv[1:]:
    if arg.startswith("--mutant=") and arg.split("=", 1)[1] in MUTANTS:
      mutant = arg.split("=", 1)[1]
    elif arg.startswith("--fixture="):
      path = Path(arg.split("=", 1)[1])
    else:
      print(f"usage : {argv[0]} [--mutant={'|'.join(MUTANTS)}] [--fixture=CHEMIN]", file=sys.stderr)
      return 2
  try:
    fixture = json.loads(path.read_text(encoding="utf-8"))
    if fixture.get("kind") != "mhgp12_exact_witness" or fixture.get("witness_id") != "WIT-FORME-NIVEAU":
      raise ValueError("fixture inattendue")
  except (OSError, ValueError, KeyError) as error:
    print(f"wit_forme_refus : {error}", file=sys.stderr)
    return 2
  try:
    facts, v11, v12 = check(fixture, mutant)
  except (Violation, KeyError, TypeError, ValueError, ZeroDivisionError) as violation:
    if mutant is not None and isinstance(violation, Violation):
      print(f"mutant_tue {mutant} : {violation}")
      return 4
    print(f"wit_forme_viole : {violation}", file=sys.stderr)
    return 3
  if mutant is not None:
    print(f"mutant_survit {mutant}", file=sys.stderr)
    return 3
  print(f"wit_forme_niveau_ok faits={facts} forme_v11={v11[0]}/{v11[1]} forme_v12={v12[0]}/{v12[1]}")
  return 0


if __name__ == "__main__":
  sys.exit(main(sys.argv))
