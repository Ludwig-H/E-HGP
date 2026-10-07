#!/usr/bin/env python3
"""Temoin WIT-T1-CARRE : le certificat combinatoire LEM-T1 exige l'inclusion S dans F.

Constat CST-0101 de l'auditeur (7 octobre 2026) : << S = S*(b) et F inclus dans P_b >> ne suffit pas a certifier
B(F) = b. Enonce juste : si S est un support de b et si S est inclus dans F, lui-meme inclus dans P_b, alors B(F) = b
(le rayon de B(F) est au moins celui de B(S) = b, et F tient dans la boule fermee b ; unicite de la plus petite boule).

Python nu (fractions, itertools, json), aucune dependance, aucun flottant, aucun assert (tient sous python3 -O).
Codes : 0 conforme ; 2 usage faux ou fixture invalide ; 3 fait grave viole ; 4 mutant tue.
Usage : test_witness_t1.py [--mutant=sans_inclusion] [--fixture=CHEMIN]
"""

import itertools
import json
import sys
from fractions import Fraction
from pathlib import Path

DEFAULT_FIXTURE = Path(__file__).resolve().parent / "fixtures" / "wit_t1_carre.json"
MUTANTS = ("sans_inclusion",)


class Violation(Exception):
  pass


def sub(a, b):
  return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
  return sum(x * y for x, y in zip(a, b))


def cross(a, b):
  return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def det3(r):
  return dot(r[0], cross(r[1], r[2]))


def circumball(pts):
  """Plus petite sphere passant par pts, centre dans leur enveloppe affine ; None si pts est degenere."""
  a = tuple(Fraction(x) for x in pts[0])
  if len(pts) == 1:
    return a, Fraction(0)
  rows = [sub(tuple(Fraction(x) for x in p), a) for p in pts[1:]]
  if len(pts) == 2:
    x = tuple(c / 2 for c in rows[0])
  elif len(pts) == 3:
    u, v = rows
    w = cross(u, v)
    ww = dot(w, w)
    if ww == 0:
      return None
    t = tuple(dot(u, u) * vi - dot(v, v) * ui for ui, vi in zip(u, v))
    x = tuple(c / (2 * ww) for c in cross(t, w))
  else:
    d = det3(rows)
    if d == 0:
      return None
    rhs = [dot(r, r) / 2 for r in rows]
    x = []
    for j in range(3):
      m = [list(r) for r in rows]
      for i in range(3):
        m[i][j] = rhs[i]
      x.append(det3([tuple(r) for r in m]) / d)
    x = tuple(x)
  return tuple(ai + xi for ai, xi in zip(a, x)), dot(x, x)


def meb(points, part):
  """Plus petite boule englobante exacte de points[part], par enumeration des supports de 1 a 4 sites."""
  best = None
  for size in range(1, min(4, len(part)) + 1):
    for tup in itertools.combinations(part, size):
      ball = circumball([points[i] for i in tup])
      if ball is None:
        continue
      center, r2 = ball
      if all(dot(sub(points[i], center), sub(points[i], center)) <= r2 for i in part):
        if best is None or r2 < best[1]:
          best = (center, r2)
  return best


def certify(ball, proposal, part, check_inclusion):
  """Regle LEM-T1 : B(part) = b si proposal = S*(b), proposal inclus dans part et part inclus dans P_b."""
  if tuple(sorted(proposal)) != tuple(ball["canonical_support"]):
    return False  # la cle ne trouve rien : chemin exact
  if check_inclusion and not set(proposal) <= set(part):
    return False
  return set(part) <= set(ball["interior"]) | set(ball["shell"])


def check(fixture, mutant):
  points = [tuple(Fraction(c) for c in p) for p in fixture["points"]]
  ball = fixture["ball"]
  b = (tuple(Fraction(c) for c in ball["center"]), Fraction(ball["squared_radius"]))
  everything = list(range(len(points)))
  closed = set(ball["interior"]) | set(ball["shell"])
  facts = 0

  # 1. la boule declaree est la plus petite boule du nuage ; interieur et coquille exacts
  if meb(points, everything) != b:
    raise Violation("boule declaree differente de la plus petite boule du nuage")
  for i, p in enumerate(points):
    d2 = dot(sub(p, b[0]), sub(p, b[0]))
    side = "interior" if d2 < b[1] else "shell" if d2 == b[1] else None
    if side is not None and i not in ball[side]:
      raise Violation(f"site {i} mal classe ({side})")
  facts += 1

  # 2. supports minimaux declares (canonique et autres) : chacun a b pour plus petite boule, aucune partie propre non
  supports = [tuple(ball["canonical_support"])] + [tuple(s) for s in ball["other_minimal_supports"]]
  for s in supports:
    if meb(points, list(s)) != b:
      raise Violation(f"{s} n'est pas un support de b")
    for size in range(1, len(s)):
      for t in itertools.combinations(s, size):
        if meb(points, list(t)) == b:
          raise Violation(f"{s} n'est pas minimal")
  facts += 1

  # 3. cas graves : rayon de la partie, decision avec et sans le test d'inclusion
  for case in fixture["cases"]:
    part = case["part"]
    expected = (tuple(Fraction(c) for c in case["part_center"]), Fraction(case["part_squared_radius"]))
    if meb(points, part) != expected:
      raise Violation(f"cas {case['name']} : plus petite boule de la partie differente de l'attendu")
    if certify(ball, case["proposal"], part, True) != case["certified"]:
      raise Violation(f"cas {case['name']} : decision de LEM-T1 differente de l'attendu")
    if certify(ball, case["proposal"], part, False) != case["certified_without_inclusion_test"]:
      raise Violation(f"cas {case['name']} : decision sans test d'inclusion differente de l'attendu")
    facts += 1

  # 4. enonce juste sur toutes les parties du temoin : aucune certification fausse ; S inclus dans F inclus dans P_b
  #    entraine B(F) = b pour chaque support minimal S
  pairs = 0
  for size in range(1, len(points) + 1):
    for part in itertools.combinations(everything, size):
      truth = meb(points, list(part)) == b
      if certify(ball, ball["canonical_support"], list(part), mutant != "sans_inclusion") and not truth:
        raise Violation(f"certification fausse : F = {part}, B(F) differente de b")
      for s in supports:
        if set(s) <= set(part) <= closed:
          pairs += 1
          if not truth:
            raise Violation(f"lemme faux sur S = {s}, F = {part}")
  facts += 1
  return facts, pairs


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
    if fixture.get("kind") != "mhgp12_exact_witness" or fixture.get("witness_id") != "WIT-T1-CARRE":
      raise ValueError("fixture inattendue")
  except (OSError, ValueError, KeyError) as error:
    print(f"wit_t1_refus : {error}", file=sys.stderr)
    return 2
  try:
    facts, pairs = check(fixture, mutant)
  except Violation as violation:
    if mutant is not None:
      print(f"mutant_tue {mutant} : {violation}")
      return 4
    print(f"wit_t1_viole : {violation}", file=sys.stderr)
    return 3
  if mutant is not None:
    print(f"mutant_survit {mutant}", file=sys.stderr)
    return 3
  print(f"wit_t1_carre_ok faits={facts} couples_lemme={pairs}")
  return 0


if __name__ == "__main__":
  sys.exit(main(sys.argv))
