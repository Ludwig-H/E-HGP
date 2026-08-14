#!/usr/bin/env python3
"""Analyseur versionne de rampe : la porte de croissance, en arithmetique exacte.

Specification : audits/AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md
section 2.1 (« un analyseur versionne qui exige quatre tailles ordonnees,
recalcule les trois logarithmes en base deux avec une regle rationnelle ou une
tolerance enregistree, puis rend un code type ») et
audits/AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md sections 2 et
8 (finalite des fenetres, matrice exacte des codes, identite exclusive).

Cadre : phase=exploration_v3_hors_registre, backend=analyse_de_recu,
        profile=quantized_u16_input_only, mode=porte_de_croissance,
        public_status=not_claimed.

---------------------------------------------------------------------------
POURQUOI CET ANALYSEUR EXISTE

La rampe G4 du pin `35fcea8` a publie quarante `code=0` et une provenance
substantielle, mais :

  * le script lancait UNE taille par processus et ne calculait AUCUNE pente ;
    `--max-slope=9` etait donc inerte et le verdict `1,7` a ete pose a la main
    dans une note, apres coup ;
  * le controle de completude comptait les lignes `^code=` sans exiger leur
    VALEUR, ni exiger exactement dix fichiers ;
  * le filtre `sed` supprimait `fenetre_finale`, et le probe rend zero meme
    avec `pending>0` : trois fenetres `terrain` publiees etaient en fait des
    SURENSEMBLES.

Une pente calculee dans un document n'est pas une porte. Celle-ci l'est.

---------------------------------------------------------------------------
LA REGLE DE PENTE EST EXACTE, ET C'EST LE POINT

Aucun logarithme flottant n'est evalue. Pour deux points (n1,m1) et (n2,m2) et
un seuil rationnel `p/q`, la pente vaut

    pente = log(m2/m1) / log(n2/n1)

et l'inegalite `pente <= p/q` equivaut, pour n2 > n1 >= 1 et m1, m2 >= 1, a

    q * log(m2/m1) <= p * log(n2/n1)
    (m2/m1)^q <= (n2/n1)^p
    m2^q * n1^p <= m1^q * n2^p

qui est une comparaison d'ENTIERS. Python les calcule exactement, quelle que
soit leur taille. Il n'existe donc aucune tolerance a enregistrer, aucun
arrondi a documenter, et une pente EXACTEMENT au seuil est decidee sans
ambiguite : elle passe, puisque la porte refuse `pente > seuil` strictement.

La valeur decimale imprimee est un CONFORT DE LECTURE. Elle n'est jamais
consultee pour decider.

---------------------------------------------------------------------------
CODES DE SORTIE
  0  la rampe passe toutes les portes armees
  1  desaccord interne du recu (identite exclusive des masses violee)
  2  campagne refusee avant tout calcul (fichier, taille, format, argument)
  3  porte violee (pente, code non nul, pending positif, fenetre non finale)
"""

import argparse
import math
import os
import re
import sys
from fractions import Fraction

SCHEMA = "RampSlopeAnalyzer-v1"

# ---------------------------------------------------------------------------
# LECTURE. Le format est celui de `mhgp3v_wspd_wavefront_probe`, lignes 1631 et
# 1668 du probe. Les motifs sont ANCRES : une ligne partielle est un refus, pas
# une valeur par defaut.
# ---------------------------------------------------------------------------
RE_ENTETE = re.compile(r"^=== (?P<famille>\S+) raffine=(?P<raffine>\d+) n=(?P<n>\d+)\s*$")
RE_PRINCIPALE = re.compile(r"^n=(?P<n>\d+) famille=(?P<famille>\S+) .*\bmasse=(?P<masse>\d+)/(?P<total>\d+)")
RE_FENETRE = re.compile(
    r"^fenetre q(?P<lane>\d) : terminaux_ouverts=(?P<ouverts>\d+) "
    r"masse_ouverte=(?P<masse_ouverte>\d+) \([-0-9.]+% de C\(n,2\)\) "
    r"sum_E=(?P<sum_e>\d+) max_E=(?P<max_e>\d+) .*"
    r"\| masses : fermee=(?P<fermee>\d+) pendante=(?P<pendante>\d+) "
    r"ouverte=(?P<ouverte>\d+) terminaux fermes=(?P<fermes>\d+) \| pending=(?P<pending>\d+)"
)
RE_CODE = re.compile(r"^code=(?P<code>-?\d+)\s*$")
RE_FINALE = re.compile(r"^fenetre_finale=(?P<verdict>OUI|NON)")


class Refus(Exception):
    """Refus avant tout calcul : code 2."""


class Desaccord(Exception):
    """Contradiction interne du recu : code 1."""


class Porte(Exception):
    """Porte armee violee : code 3."""


class Mesure:
    __slots__ = ("n", "code", "lanes", "finale", "total")

    def __init__(self, n):
        self.n = n
        self.code = None
        self.lanes = {}
        self.finale = None
        self.total = None


def lire_fichier(chemin):
    """Rend la liste ordonnee des mesures d'un fichier de rampe."""
    if not os.path.isfile(chemin):
        raise Refus("fichier absent : %s" % chemin)
    mesures = []
    courante = None
    with open(chemin, "r", encoding="utf-8", errors="replace") as fh:
        for brut in fh:
            ligne = brut.rstrip("\n")
            m = RE_ENTETE.match(ligne)
            if m is not None:
                courante = Mesure(int(m.group("n")))
                mesures.append(courante)
                continue
            if courante is None:
                continue
            m = RE_PRINCIPALE.match(ligne)
            if m is not None:
                if int(m.group("n")) != courante.n:
                    raise Refus(
                        "%s : entete n=%d mais ligne principale n=%s"
                        % (chemin, courante.n, m.group("n"))
                    )
                courante.total = int(m.group("total"))
                continue
            m = RE_FENETRE.match(ligne)
            if m is not None:
                lane = int(m.group("lane"))
                courante.lanes[lane] = {
                    "ouverts": int(m.group("ouverts")),
                    "masse_ouverte": int(m.group("masse_ouverte")),
                    "sum_E": int(m.group("sum_e")),
                    "max_E": int(m.group("max_e")),
                    "fermee": int(m.group("fermee")),
                    "pendante": int(m.group("pendante")),
                    "ouverte": int(m.group("ouverte")),
                    "pending": int(m.group("pending")),
                }
                continue
            m = RE_FINALE.match(ligne)
            if m is not None:
                courante.finale = m.group("verdict")
                continue
            m = RE_CODE.match(ligne)
            if m is not None:
                if courante.code is not None:
                    raise Refus("%s : deux lignes code= pour n=%d" % (chemin, courante.n))
                courante.code = int(m.group("code"))
                continue
    return mesures


# ---------------------------------------------------------------------------
# LA REGLE DE PENTE, EN ENTIERS. Aucun flottant ne decide.
# ---------------------------------------------------------------------------
def pente_depasse(n1, m1, n2, m2, seuil):
    """Vrai si log(m2/m1)/log(n2/n1) > seuil, decide exactement.

    `seuil` est une Fraction p/q avec q > 0. La comparaison equivalente est
    m2^q * n1^p > m1^q * n2^p, entre entiers exacts.

    Les cas degeneres sont explicites : une masse nulle n'a pas de logarithme,
    et deux tailles egales n'ont pas de pente.
    """
    if n2 <= n1:
        raise Refus("tailles non strictement croissantes : %d puis %d" % (n1, n2))
    if m1 <= 0 or m2 <= 0:
        raise Refus("masse nulle ou negative : %d puis %d" % (m1, m2))
    p, q = seuil.numerator, seuil.denominator
    if q <= 0:
        raise Refus("seuil de denominateur non positif")
    if p < 0:
        raise Refus("seuil negatif")
    gauche = (m2 ** q) * (n1 ** p)
    droite = (m1 ** q) * (n2 ** p)
    return gauche > droite


def pente_lisible(n1, m1, n2, m2):
    """Valeur decimale, POUR LA LECTURE SEULEMENT."""
    return math.log(m2 / m1) / math.log(n2 / n1)


# ---------------------------------------------------------------------------
# FIXTURES GRAVEES. Elles exercent les quatre defauts que la rampe precedente
# ne pouvait pas detecter, plus la frontiere exacte.
# ---------------------------------------------------------------------------
def fixtures():
    echecs = []

    def verifie(nom, obtenu, attendu):
        if obtenu != attendu:
            echecs.append("%s : obtenu %r, attendu %r" % (nom, obtenu, attendu))

    # 1. La pente EXACTEMENT au seuil passe. Tailles doublees, masses
    # quadruplees : la pente vaut exactement 2, et le seuil vaut exactement 2.
    verifie("seuil-exact-egalite", pente_depasse(1000, 7, 2000, 28, Fraction(2)), False)
    # 2. Un point au-dessus du seuil, si petit soit-il, est refuse. La meme
    # rampe avec une unite de masse en plus depasse strictement.
    verifie("seuil-exact-depasse", pente_depasse(1000, 7, 2000, 29, Fraction(2)), True)
    # 3. Le seuil `1,7` de la note de route est un rationnel 17/10, et il est
    # decide sans flottant. 2^1.7 = 3,2490... : un facteur 3,25 depasse, un
    # facteur 3,24 non.
    verifie("seuil-17-sur-10-depasse",
            pente_depasse(1000, 10000, 2000, 32490, Fraction(17, 10)), False)
    verifie("seuil-17-sur-10-refuse",
            pente_depasse(1000, 10000, 2000, 32491, Fraction(17, 10)), True)
    # 4. La pente reelle mesuree sur `eight_clusters` profondeur quatre,
    # dernier intervalle : 139 835 768 -> 525 902 961 entre 25 000 et 50 000.
    # Elle depasse 1,7 ; elle ne depasse pas 2.
    verifie("eight-clusters-r4-depasse-1.7",
            pente_depasse(25000, 139835768, 50000, 525902961, Fraction(17, 10)), True)
    verifie("eight-clusters-r4-sous-2",
            pente_depasse(25000, 139835768, 50000, 525902961, Fraction(2)), False)
    # 5. La pente reelle de `uniform` profondeur quatre, dernier intervalle du
    # meme recu : 4 790 377 -> 9 976 128. Elle ne depasse pas 1,35 et depasse
    # 1,05 — les deux cotes sont graves, pour qu'un seuil deplace se voie.
    verifie("uniform-r4-sous-1.35",
            pente_depasse(25000, 4790377, 50000, 9976128, Fraction(27, 20)), False)
    verifie("uniform-r4-au-dessus-de-1.05",
            pente_depasse(25000, 4790377, 50000, 9976128, Fraction(21, 20)), True)
    # 6. Les degeneres sont des REFUS, pas des valeurs.
    for nom, args in (
        ("tailles-egales", (1000, 7, 1000, 8)),
        ("tailles-decroissantes", (2000, 7, 1000, 8)),
        ("masse-nulle", (1000, 0, 2000, 8)),
    ):
        try:
            pente_depasse(*args, Fraction(2))
            echecs.append("%s : aurait du etre refuse" % nom)
        except Refus:
            pass

    if echecs:
        for e in echecs:
            sys.stderr.write("FIXTURE REFUTEE %s\n" % e)
        return 3
    print("fixtures accord=OUI refutees=0 gravees=12 schema=%s" % SCHEMA)
    return 0


def principal(argv):
    ap = argparse.ArgumentParser(description="Analyseur versionne de rampe (%s)" % SCHEMA)
    ap.add_argument("--repertoire", help="dossier contenant les fichiers rampe_*.txt")
    ap.add_argument("--familles", default="uniform,terrain,eight_clusters,"
                                          "scanline_single_pass,scanline_overlap_multiecho")
    ap.add_argument("--profondeurs", default="0,4")
    ap.add_argument("--tailles", default="6250,12500,25000,50000")
    ap.add_argument("--lane", type=int, default=4, choices=(2, 3, 4))
    ap.add_argument("--metrique", default="sum_E", choices=("sum_E", "masse_ouverte", "max_E"))
    ap.add_argument("--max-pente", default="", help="seuil rationnel, ex. 17/10 ou 1.35")
    ap.add_argument("--exige-pending-zero", action="store_true")
    ap.add_argument("--exige-fenetre-finale", action="store_true")
    ap.add_argument("--exige-code-zero", action="store_true")
    ap.add_argument("--fixtures", action="store_true")
    args = ap.parse_args(argv)

    if args.fixtures:
        if args.repertoire:
            raise Refus("--fixtures ne prend aucun recu")
        return fixtures()
    if not args.repertoire:
        raise Refus("--repertoire est obligatoire hors --fixtures")

    familles = [x for x in args.familles.split(",") if x]
    profondeurs = [int(x) for x in args.profondeurs.split(",") if x]
    tailles = [int(x) for x in args.tailles.split(",") if x]
    if len(tailles) < 2:
        raise Refus("il faut au moins deux tailles pour une pente")
    if sorted(tailles) != tailles or len(set(tailles)) != len(tailles):
        raise Refus("les tailles doivent etre strictement croissantes")

    seuil = None
    if args.max_pente:
        texte = args.max_pente.strip()
        try:
            seuil = Fraction(texte)
        except ValueError:
            raise Refus("--max-pente n'est pas un rationnel exact : %r" % texte)
        if seuil <= 0:
            raise Refus("--max-pente doit etre strictement positif")

    attendus = len(familles) * len(profondeurs)
    presents = sorted(
        f for f in os.listdir(args.repertoire) if f.startswith("rampe_") and f.endswith(".txt")
    )
    if len(presents) != attendus:
        raise Refus(
            "matrice incomplete : %d fichiers rampe_*.txt presents, %d exiges (%d familles x %d profondeurs)"
            % (len(presents), attendus, len(familles), len(profondeurs))
        )

    violations = []
    desaccords = []
    lignes = []
    for famille in familles:
        for raffine in profondeurs:
            nom = "rampe_%s_r%d.txt" % (famille, raffine)
            chemin = os.path.join(args.repertoire, nom)
            mesures = lire_fichier(chemin)
            vues = [m.n for m in mesures]
            if vues != tailles:
                raise Refus("%s : tailles %r, attendu %r" % (nom, vues, tailles))
            for m in mesures:
                if m.code is None:
                    raise Refus("%s : n=%d ne publie aucun code de sortie" % (nom, m.n))
                if args.exige_code_zero and m.code != 0:
                    violations.append("%s n=%d : code=%d" % (nom, m.n, m.code))
                if args.lane not in m.lanes:
                    raise Refus("%s : n=%d ne publie pas la fenetre q%d" % (nom, m.n, args.lane))
                lane = m.lanes[args.lane]
                # IDENTITE EXCLUSIVE. C'est la seule verification qui puisse
                # rendre 1 : elle refute le RECU lui-meme, pas la performance.
                if m.total is not None:
                    somme = lane["fermee"] + lane["pendante"] + lane["ouverte"]
                    if somme != m.total:
                        desaccords.append(
                            "%s n=%d q%d : fermee+pendante+ouverte=%d != masse=%d"
                            % (nom, m.n, args.lane, somme, m.total)
                        )
                if lane["masse_ouverte"] != lane["ouverte"] + lane["pendante"]:
                    desaccords.append(
                        "%s n=%d q%d : masse_ouverte=%d != ouverte+pendante=%d"
                        % (nom, m.n, args.lane, lane["masse_ouverte"],
                           lane["ouverte"] + lane["pendante"])
                    )
                if args.exige_pending_zero and lane["pending"] != 0:
                    violations.append(
                        "%s n=%d q%d : pending=%d, masse pendante=%d — fenetre non finale"
                        % (nom, m.n, args.lane, lane["pending"], lane["pendante"])
                    )
                if args.exige_fenetre_finale and m.finale != "OUI":
                    violations.append(
                        "%s n=%d : fenetre_finale=%s" % (nom, m.n, m.finale or "ABSENT")
                    )

            valeurs = [m.lanes[args.lane][args.metrique] for m in mesures]
            pentes = []
            for i in range(len(tailles) - 1):
                depasse = seuil is not None and pente_depasse(
                    tailles[i], valeurs[i], tailles[i + 1], valeurs[i + 1], seuil
                )
                pentes.append((pente_lisible(tailles[i], valeurs[i], tailles[i + 1],
                                             valeurs[i + 1]), depasse))
                if depasse:
                    violations.append(
                        "%s q%d %s : pente %d->%d au-dessus de %s (%d -> %d)"
                        % (nom, args.lane, args.metrique, tailles[i], tailles[i + 1],
                           seuil, valeurs[i], valeurs[i + 1])
                    )
            lignes.append(
                "%-28s q%d %-13s %s | %s"
                % (
                    "%s/r%d" % (famille, raffine),
                    args.lane,
                    args.metrique,
                    " ".join("%.6f%s" % (v, "!" if d else "") for v, d in pentes),
                    " ".join(str(v) for v in valeurs),
                )
            )

    print("analyseur=%s familles=%d profondeurs=%d tailles=%s lane=q%d metrique=%s seuil=%s"
          % (SCHEMA, len(familles), len(profondeurs), ",".join(str(t) for t in tailles),
             args.lane, args.metrique, seuil if seuil is not None else "AUCUN"))
    for ligne in lignes:
        print(ligne)

    if desaccords:
        for d in desaccords:
            sys.stderr.write("DESACCORD : %s\n" % d)
        return 1
    if violations:
        for v in violations:
            sys.stderr.write("PORTE VIOLEE : %s\n" % v)
        return 3
    print("rampe accord=OUI violations=0")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(principal(sys.argv[1:]))
    except Refus as exc:
        sys.stderr.write("REFUS : %s\n" % exc)
        sys.exit(2)
    except Desaccord as exc:
        sys.stderr.write("DESACCORD : %s\n" % exc)
        sys.exit(1)
    except Porte as exc:
        sys.stderr.write("PORTE VIOLEE : %s\n" % exc)
        sys.exit(3)
