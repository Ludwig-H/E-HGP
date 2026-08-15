#!/usr/bin/env python3
"""Porte : les deux balayages du `W`-vivant doivent rendre le MEME compte.

Specification : audits/AUDIT_REAUDIT_DUAL_TREE_COEUR_BOULE_SEPARATION_EB1B52A_20260815.md
section P0.5 (« Passer ce compte en deux passes, avec budget `n*|S|`, paires
`D>0` et scan point--point multi-lane unique ») et section 6.4 (« `corner8_lane`
evalue huit fois le meme coin quand `b` est ponctuel ; la boucle exterieure
recalcule actuellement le scan pour q2, q3 et q4 »).

Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
        profile=quantized_u16_input_only, mode=porte_metamorphique,
        public_status=not_claimed.

---------------------------------------------------------------------------
POURQUOI CETTE PORTE EXISTE

Le balayage du `W`-vivant a ete reecrit : trois passes independantes sur `z`,
une par lane, sont devenues une passe unique alimentant trois compteurs, et le
predicat `corner8_lane(a, Box(b), z)` — huit coins identiques quand `b` est
ponctuel — est devenu `pair_lane(a, b, z)`.

Une reecriture de balayage et une reecriture de CE QUE LE BALAYAGE COMPTE se
ressemblent dans un diff. Elles ne se ressemblent pas ici : l'ancien balayage
est conserve sous `--vivant=legacy`, et cette porte exige que les trois comptes
`q2/q3/q4` coincident A L'UNITE sur plusieurs familles. Un desaccord rend 1.

Ce que la porte ne dit PAS : que le compte est le bon. Les deux balayages
peuvent partager une erreur de definition — c'est l'enumeration exhaustive
bornee (`--oracle`) qui repond a cette question-la, pas celle-ci.

---------------------------------------------------------------------------
LES PLANCHERS, CONTRE LE VERT PAR VACUITE

Un accord sur `0 = 0` ne prouve rien. Trois planchers :

  * `--min-vivantes=N` : chaque lane doit compter au moins `N` `W`-vivantes ;
  * `--min-paires=N`   : le balayage fusionne doit avoir reellement balaye au
                         moins `N` paires ;
  * la masse hors cap doit etre nulle, sans quoi le probe lui-meme rend 3 et la
    porte le propage.

---------------------------------------------------------------------------
CODES DE SORTIE (convention v3)

  0 : accord sur toutes les familles, planchers tenus
  1 : desaccord entre les deux balayages — le defaut que la porte cherche
  2 : refus avant calcul (probe introuvable, argument invalide, famille vide)
  3 : plancher viole, ou probe non nul (masse hors cap, refus interne)
"""

import argparse
import re
import subprocess
import sys

# `vraivivant q2_vivantes=17479 q2_mou=1.088 ... paires=66760 travail=... evals=...`
_VIVANTES = re.compile(r"q(\d)_vivantes=(\d+)")
_PAIRES = re.compile(r"\bpaires=(\d+)")
_EVALS = re.compile(r"\bevals=(\d+)")
_TRAVAIL = re.compile(r"\btravail=(\d+)")


def lance(probe, args):
    """Rend (comptes, paires, travail, evals) ou leve RuntimeError."""
    cp = subprocess.run([probe] + args, capture_output=True, text=True)
    if cp.returncode != 0:
        raise RuntimeError(
            "probe code=%d : %s" % (cp.returncode, cp.stderr.strip()[:400]))
    ligne = None
    for l in cp.stdout.splitlines():
        if l.startswith("vraivivant"):
            ligne = l
            break
    if ligne is None:
        raise RuntimeError("aucune ligne `vraivivant` dans la sortie")
    comptes = {int(q): int(v) for q, v in _VIVANTES.findall(ligne)}
    if sorted(comptes) != [2, 3, 4]:
        raise RuntimeError("lanes manquantes : %s" % sorted(comptes))
    for nom, rx in (("paires", _PAIRES), ("travail", _TRAVAIL), ("evals", _EVALS)):
        if rx.search(ligne) is None:
            raise RuntimeError("champ `%s` absent du recu" % nom)
    return (comptes, int(_PAIRES.search(ligne).group(1)),
            int(_TRAVAIL.search(ligne).group(1)),
            int(_EVALS.search(ligne).group(1)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--probe", required=True)
    ap.add_argument("--familles", default="uniform,terrain,eight_clusters")
    ap.add_argument("--points", type=int, default=300)
    ap.add_argument("--separation", type=int, default=8)
    ap.add_argument("--smax", type=int, default=11)
    ap.add_argument("--seed", type=int, default=3)
    ap.add_argument("--min-vivantes", type=int, default=1)
    ap.add_argument("--min-paires", type=int, default=1)
    # `--inject` n'est transmis qu'au balayage FUSIONNE : les deux mutants
    # `vivant-*` vivent la, et le balayage legacy sert alors de reference non
    # mutee. Injecter des deux cotes rendrait la porte aveugle.
    ap.add_argument("--inject", default="")
    a = ap.parse_args()

    familles = [f for f in a.familles.split(",") if f]
    if not familles:
        print("REFUS : aucune famille")
        return 2
    if a.points < 4:
        print("REFUS : --points=%d trop petit pour porter un temoin" % a.points)
        return 2

    desaccords = 0
    planchers = 0
    for fam in familles:
        base = ["--points=%d" % a.points, "--family=%s" % fam,
                "--seed=%d" % a.seed, "--separation=%d" % a.separation,
                "--smax=%d" % a.smax]
        try:
            mut = ["--inject=%s" % a.inject] if a.inject else []
            leg, _, leg_t, leg_e = lance(a.probe, base + ["--vivant=legacy"])
            fus, fus_p, fus_t, fus_e = lance(a.probe, base + ["--vivant=fusion"] + mut)
        except (RuntimeError, OSError) as exc:
            print("REFUS %s : %s" % (fam, exc))
            return 2 if isinstance(exc, OSError) else 3

        for q in (2, 3, 4):
            if leg[q] != fus[q]:
                print("DESACCORD %s q%d : legacy=%d fusion=%d"
                      % (fam, q, leg[q], fus[q]))
                desaccords += 1
            elif fus[q] < a.min_vivantes:
                print("PLANCHER %s q%d : %d < %d"
                      % (fam, q, fus[q], a.min_vivantes))
                planchers += 1
        if fus_p < a.min_paires:
            print("PLANCHER %s paires : %d < %d" % (fam, fus_p, a.min_paires))
            planchers += 1
        # Le rapport `evals` est le fait que la section 6.4 predisait a huit :
        # on le PUBLIE, sans en faire une porte, parce qu'il depend du nuage.
        print("famille=%s q2=%d q3=%d q4=%d paires=%d "
              "visites_legacy=%d visites_fusion=%d "
              "evals_legacy=%d evals_fusion=%d redondance=%.3f gain=%.3f"
              % (fam, fus[2], fus[3], fus[4], fus_p, leg_t, fus_t, leg_e, fus_e,
                 leg_e / leg_t if leg_t else 0.0,
                 leg_e / fus_e if fus_e else 0.0))

    if desaccords:
        print("balayages accord=NON desaccords=%d" % desaccords)
        return 1
    if planchers:
        print("balayages accord=OUI planchers_violes=%d" % planchers)
        return 3
    print("balayages accord=OUI familles=%d desaccords=0 planchers_violes=0"
          % len(familles))
    return 0


if __name__ == "__main__":
    sys.exit(main())
