#!/usr/bin/env python3
"""Juge du pilote serie C (§ 5.13) — grammaire des records, stub ET device.

Verifie sur la sortie du pilote : les repeat+1 records exacts (echauffement
`retenue=NON` + repetitions ABBA/BAAB), la parite par RECALCUL de l'egalite
des signatures canoniques (le booleen `parite=` imprime n'est qu'une
redondance), les formules d'octets (H2D boules = 112*nb_total, sentinelles =
100*nb_total, D2H = 100*nb_total), les temps finis et les lots. La meme
fonction `juger` sert a la porte stub locale et au validateur G4 : les
champs publies deviennent causaux AVANT toute depense device.

Modes :
  pilote_juge.py --binaire <chemin> [args du pilote...]  juge une execution
  pilote_juge.py --contre-fixtures                       flux falsifies integres,
                                                         chacun DOIT etre refuse
Jamais d'`assert` (doit tenir sous python3 -O).
"""

import math
import re
import subprocess
import sys

FLOAT_KEYS = [
    "mur_cpu_ms", "mur_route_device_ms", "prefiltre_census_cpu_ms",
    "route_device_etage_ms", "wire_ms", "setup_alloc_ms", "h2d_ms",
    "kernels_ms", "d2h_ms", "rebuild_ms",
]
INT_KEYS = [
    "repetition", "nb_total", "lot_effectif", "h2d_octets_index",
    "h2d_octets_boules", "h2d_octets_sentinelles", "d2h_octets", "lots",
]
HEX_KEYS = ["digest_all", "signature_cpu", "signature_device"]
STR_KEYS = ["retenue", "ordre", "parite"]
ALL_KEYS = FLOAT_KEYS + INT_KEYS + HEX_KEYS + STR_KEYS
HEX64 = re.compile(r"^[0-9a-f]{64}$")


def parse_record(line):
    """Un record => dict, ou une chaine d'erreur."""
    fields = {}
    for tok in line.split():
        if "=" not in tok:
            return "jeton sans '=' : %r" % tok
        k, v = tok.split("=", 1)
        if k in fields:
            return "champ duplique %s" % k
        fields[k] = v
    for k in ALL_KEYS:
        if k not in fields:
            return "champ manquant %s" % k
    rec = {}
    for k in FLOAT_KEYS:
        try:
            x = float(fields[k])
        except ValueError:
            return "champ %s non numerique : %r" % (k, fields[k])
        if not math.isfinite(x) or x < 0.0:
            return "champ %s non fini ou negatif : %r" % (k, fields[k])
        rec[k] = x
    for k in INT_KEYS:
        if not re.match(r"^\d+$", fields[k]):
            return "champ %s non entier : %r" % (k, fields[k])
        rec[k] = int(fields[k])
    for k in HEX_KEYS:
        if not HEX64.match(fields[k]):
            return "champ %s hors hex64 : %r" % (k, fields[k])
        rec[k] = fields[k]
    for k in STR_KEYS:
        rec[k] = fields[k]
    return rec


ENTETE = re.compile(r"^pilote_serie_c device=(.+) sm=(\d+\.\d+) arch_compilees=(\S+) "
                    r"famille=(\S+) n=(\d+) graine=(\d+) fils=(\d+) lot=(\d+)$")


def entete(text):
    """L'en-tete du pilote, EXACTEMENT une : dict, ou une chaine d'erreur."""
    hits = [ENTETE.match(ln) for ln in text.splitlines() if ln.startswith("pilote_serie_c ")]
    if len(hits) != 1 or hits[0] is None:
        return "en-tete pilote_serie_c absente, multiple ou hors grammaire"
    m = hits[0]
    return {"device": m.group(1), "sm": m.group(2), "arch": m.group(3),
            "famille": m.group(4), "n": int(m.group(5)), "graine": int(m.group(6)),
            "fils": int(m.group(7)), "lot": int(m.group(8))}


def juger(text, ordre_base, repeat=4, min_lots=0,
          famille=None, n=None, graine=None, fils=None, arch=None):
    """None si conforme, sinon la premiere non-conformite (chaine).

    § 5.15.2 : l'en-tete est PARSEE exactement et liee aux attentes fournies
    (famille, n, graine, fils) — une en-tete annoncant une autre entree ne
    reste plus verte — et chaque record verifie
    lot_effectif = min(lot, nb_total)."""
    if ordre_base not in ("cpu-device", "device-cpu"):
        return "ordre de base inconnu : %r" % ordre_base
    inverse = "device-cpu" if ordre_base == "cpu-device" else "cpu-device"
    lines = text.splitlines()
    tete = entete(text)
    if not isinstance(tete, dict):
        return tete
    for nom, attendu in (("famille", famille), ("n", n), ("graine", graine),
                         ("fils", fils), ("arch", arch)):
        if attendu is not None and tete[nom] != attendu:
            return "en-tete : %s=%r != attendu %r" % (nom, tete[nom], attendu)
    records = []
    for ln in lines:
        if ln.startswith("repetition="):
            r = parse_record(ln)
            if not isinstance(r, dict):
                return r
            records.append(r)
    if len(records) != repeat + 1:
        return "%d records au lieu de %d" % (len(records), repeat + 1)
    for i, rec in enumerate(records):
        who = "record %d" % i
        if rec["repetition"] != i:
            return "%s : indice de repetition %d" % (who, rec["repetition"])
        attendu_ret = "NON" if i == 0 else "OUI"
        if rec["retenue"] != attendu_ret:
            return "%s : retenue=%s (attendu %s)" % (who, rec["retenue"], attendu_ret)
        if i == 0:
            attendu_ordre = ordre_base
        else:
            m = (i - 1) % 4
            attendu_ordre = ordre_base if m in (0, 3) else inverse
        if rec["ordre"] != attendu_ordre:
            return "%s : ordre=%s hors sequence ABBA (attendu %s)" % (
                who, rec["ordre"], attendu_ordre)
        # Parite RECALCULEE depuis les signatures, jamais crue sur parole.
        if rec["signature_cpu"] != rec["signature_device"]:
            return "%s : signatures CPU/device divergentes" % who
        if rec["parite"] != "OUI":
            return "%s : parite=%s" % (who, rec["parite"])
        nb = rec["nb_total"]
        if nb <= 0:
            return "%s : nb_total nul" % who
        if rec["h2d_octets_boules"] != 112 * nb:
            return "%s : h2d_octets_boules=%d != 112*nb_total" % (who, rec["h2d_octets_boules"])
        if rec["h2d_octets_sentinelles"] != 100 * nb:
            return "%s : h2d_octets_sentinelles=%d != 100*nb_total" % (who, rec["h2d_octets_sentinelles"])
        if rec["d2h_octets"] != 100 * nb:
            return "%s : d2h_octets=%d != 100*nb_total" % (who, rec["d2h_octets"])
        if rec["h2d_octets_index"] <= 0:
            return "%s : h2d_octets_index nul" % who
        le = rec["lot_effectif"]
        if le < 1 or le > nb:
            return "%s : lot_effectif=%d hors [1, nb_total]" % (who, le)
        if le != min(tete["lot"], nb):
            return "%s : lot_effectif=%d != min(lot=%d, nb_total=%d)" % (who, le, tete["lot"], nb)
        if rec["lots"] != (nb + le - 1) // le:
            return "%s : lots=%d != ceil(nb_total/lot_effectif)" % (who, rec["lots"])
        if min_lots > 0 and rec["lots"] < min_lots:
            return "%s : lots=%d < min_lots=%d" % (who, rec["lots"], min_lots)
        if rec["mur_cpu_ms"] <= 0.0 or rec["mur_route_device_ms"] <= 0.0:
            return "%s : mur nul (chrono non termine ?)" % who
        # FERMETURE DES CHRONOS (§ 5.14.3), tolerance d'impression %.1f :
        # l'etage device couvre la somme de ses six composantes, et chaque
        # mur enveloppe son sous-etage — six composantes a 1000 sous un
        # etage a 1 doivent mourir ici.
        etages = (rec["wire_ms"] + rec["setup_alloc_ms"] + rec["h2d_ms"] +
                  rec["kernels_ms"] + rec["d2h_ms"] + rec["rebuild_ms"])
        if rec["route_device_etage_ms"] + 0.4 < etages:
            return ("%s : etage device (%.1f ms) < somme des composantes (%.1f ms)"
                    % (who, rec["route_device_etage_ms"], etages))
        if rec["mur_cpu_ms"] + 0.1 < rec["prefiltre_census_cpu_ms"]:
            return "%s : mur CPU < son sous-etage prefiltre+census" % who
        if rec["mur_route_device_ms"] + 0.1 < rec["route_device_etage_ms"]:
            return "%s : mur route device < son etage device" % who
    # STABILITE ENTRE REPETITIONS d'une meme entree (§ 5.14.3) : objet,
    # signatures et volumes IDENTIQUES sur les cinq records — seul le temps
    # varie.
    stables = ("digest_all", "signature_cpu", "nb_total", "lot_effectif",
               "h2d_octets_index", "h2d_octets_boules", "h2d_octets_sentinelles",
               "d2h_octets", "lots")
    for i, rec in enumerate(records[1:], 1):
        for k in stables:
            if rec[k] != records[0][k]:
                return "record %d : %s differe de l'echauffement (instabilite)" % (i, k)
    return None


# ---- Contre-fixtures : chaque falsification DOIT etre refusee. Le flux de
# base est synthetique et conforme ; chaque scene ne falsifie QU'UN aspect.
def flux_de_base(repeat=4, ordre_base="cpu-device"):
    sig = "ab" * 32
    dig = "cd" * 32
    inverse = "device-cpu" if ordre_base == "cpu-device" else "cpu-device"
    out = ["pilote_serie_c device=stub sm=0.0 arch_compilees=stub famille=uniform n=200 graine=3 fils=4 lot=17"]
    for i in range(repeat + 1):
        if i == 0:
            ordre = ordre_base
        else:
            m = (i - 1) % 4
            ordre = ordre_base if m in (0, 3) else inverse
        out.append(
            "repetition=%d retenue=%s ordre=%s parite=OUI "
            "mur_cpu_ms=10.0 mur_route_device_ms=5.0 prefiltre_census_cpu_ms=4.0 "
            "route_device_etage_ms=4.5 wire_ms=1.0 setup_alloc_ms=0.1 h2d_ms=0.5 "
            "kernels_ms=1.5 d2h_ms=0.5 rebuild_ms=0.4 nb_total=100 lot_effectif=17 "
            "h2d_octets_index=1234 h2d_octets_boules=11200 h2d_octets_sentinelles=10000 "
            "d2h_octets=10000 lots=6 digest_all=%s signature_cpu=%s signature_device=%s"
            % (i, "NON" if i == 0 else "OUI", ordre, dig, sig, sig))
    return "\n".join(out) + "\n"


def contre_fixtures():
    base = flux_de_base()
    attentes = {"famille": "uniform", "n": 200, "graine": 3, "fils": 4, "arch": "stub"}
    ok = juger(base, "cpu-device", repeat=4, min_lots=2, **attentes)
    if ok is not None:
        print("CONTRE-FIXTURE : le flux de base est refuse a tort (%s)" % ok)
        return 1
    sig2 = "ef" * 32
    scenes = [
        ("record manquant", "\n".join(base.splitlines()[:-1]) + "\n"),
        ("echauffement retenu", base.replace("repetition=0 retenue=NON", "repetition=0 retenue=OUI", 1)),
        ("ordre hors ABBA", base.replace("repetition=2 retenue=OUI ordre=device-cpu",
                                         "repetition=2 retenue=OUI ordre=cpu-device", 1)),
        ("signatures divergentes", base.replace("signature_device=" + "ab" * 32,
                                                "signature_device=" + sig2, 1)),
        ("octets boules falsifies", base.replace("h2d_octets_boules=11200",
                                                 "h2d_octets_boules=11201", 1)),
        ("octets d2h falsifies", base.replace("d2h_octets=10000", "d2h_octets=9999", 1)),
        ("lots incoherents", base.replace("lots=6", "lots=5")),
        ("chrono non fini", base.replace("mur_route_device_ms=5.0", "mur_route_device_ms=nan", 1)),
        ("mur nul", base.replace("mur_cpu_ms=10.0", "mur_cpu_ms=0.0", 1)),
        ("plancher lots", None),  # min_lots=7 sur le flux de base (lots=6)
        ("digest instable", "\n".join(
            ln.replace("digest_all=" + "cd" * 32, "digest_all=" + "aa" * 32)
            if ln.startswith("repetition=4 ") else ln
            for ln in base.splitlines()) + "\n"),
        ("champ manquant", base.replace(" lots=6", "", 1)),
        # § 5.14.3 : six composantes totalisant 1000 ms sous un etage a 1 ms.
        ("etage device sous ses composantes",
         base.replace("route_device_etage_ms=4.5 wire_ms=1.0",
                      "route_device_etage_ms=1.0 wire_ms=500.0", 1)
             .replace("kernels_ms=1.5", "kernels_ms=500.0", 1)),
        ("mur device sous son etage",
         base.replace("mur_route_device_ms=5.0 prefiltre_census_cpu_ms=4.0 route_device_etage_ms=4.5",
                      "mur_route_device_ms=3.0 prefiltre_census_cpu_ms=4.0 route_device_etage_ms=4.5", 1)),
        ("mur cpu sous son sous-etage",
         base.replace("mur_cpu_ms=10.0 mur_route_device_ms=5.0 prefiltre_census_cpu_ms=4.0",
                      "mur_cpu_ms=2.0 mur_route_device_ms=5.0 prefiltre_census_cpu_ms=4.0", 1)),
        # Les scenes d'instabilite isolent la dent de stabilite : le champ
        # mute n'a PAS de contrainte par record (l'index n'a qu'un plancher,
        # la paire de signatures reste egale) — seule la comparaison a
        # l'echauffement peut tuer.
        ("volume d'index instable entre repetitions", "\n".join(
            ln.replace("h2d_octets_index=1234", "h2d_octets_index=1235")
            if ln.startswith("repetition=3 ") else ln
            for ln in base.splitlines()) + "\n"),
        ("signatures instables entre repetitions (paire restee egale)", "\n".join(
            ln.replace("signature_cpu=" + "ab" * 32, "signature_cpu=" + "ef" * 32)
              .replace("signature_device=" + "ab" * 32, "signature_device=" + "ef" * 32)
            if ln.startswith("repetition=2 ") else ln
            for ln in base.splitlines()) + "\n"),
        # § 5.15.2 : une en-tete annoncant une AUTRE entree ne reste plus
        # verte, et lot_effectif est lie a min(lot, nb_total).
        ("en-tete d'une autre famille", base.replace("famille=uniform n=200", "famille=terrain n=200", 1)),
        ("en-tete d'un autre n", base.replace("famille=uniform n=200", "famille=uniform n=300", 1)),
        ("en-tete d'autres fils", base.replace("fils=4 lot=17", "fils=8 lot=17", 1)),
        ("lot d'en-tete incoherent avec lot_effectif", base.replace("fils=4 lot=17", "fils=4 lot=200", 1)),
        ("en-tete dupliquee", base.replace(
            "pilote_serie_c device=stub",
            "pilote_serie_c device=stub sm=0.0 arch_compilees=stub famille=uniform n=200 graine=3 fils=4 lot=17\npilote_serie_c device=stub", 1)),
        # § 5.16 : l'architecture compilee est COMPAREE (86 sous une attente
        # 120/stub meurt) ; graine, parite imprimee et grammaire hex64 aussi.
        ("arch_compilees d'une autre architecture", base.replace("arch_compilees=stub ", "arch_compilees=86 ", 1)),
        ("en-tete d'une autre graine", base.replace("graine=3 fils=4", "graine=4 fils=4", 1)),
        ("parite=NON imprimee malgre des signatures egales", base.replace("parite=OUI", "parite=NON", 1)),
        ("digest hors grammaire hex64", base.replace("digest_all=" + "cd" * 32, "digest_all=" + "cd" * 31 + "c", 1)),
        ("signature hors grammaire hex64", base.replace("signature_cpu=" + "ab" * 32, "signature_cpu=" + "AB" * 32, 1)),
    ]
    rc = 0
    for nom, flux in scenes:
        if flux is None:
            verdict = juger(base, "cpu-device", repeat=4, min_lots=7, **attentes)
        else:
            verdict = juger(flux, "cpu-device", repeat=4, min_lots=2, **attentes)
        if verdict is None:
            print("CONTRE-FIXTURE NON TUEE : %s" % nom)
            rc = 1
        else:
            print("contre-fixture tuee : %s (%s)" % (nom, verdict))
    return rc


def main(argv):
    if len(argv) >= 2 and argv[1] == "--contre-fixtures":
        return contre_fixtures()
    if len(argv) >= 3 and argv[1] in ("--fichier", "--binaire"):
        # --fichier (§ 5.14.3) : mode FICHIER pour le runner — juge la
        # sortie DEJA ecrite d'un pilote, APRES chaque famille et AVANT la
        # suivante (fail-fast). --binaire : execute puis juge. Grammaire
        # IDENTIQUE ; l'identite attendue (§ 5.15.2) vient des arguments
        # --family/--n/--seed/--threads quand ils sont fournis.
        cible = argv[2]
        args = argv[3:]
        ordre_base, repeat, min_lots = "cpu-device", 4, 0
        attendu = {"famille": None, "n": None, "graine": None, "fils": None, "arch": None}
        for a in args:
            if a.startswith("--ordre="):
                ordre_base = a.split("=", 1)[1]
            elif a.startswith("--repeat="):
                repeat = int(a.split("=", 1)[1])
            elif a.startswith("--min-lots="):
                min_lots = int(a.split("=", 1)[1])
            elif a.startswith("--family="):
                attendu["famille"] = a.split("=", 1)[1]
            elif a.startswith("--n="):
                attendu["n"] = int(a.split("=", 1)[1])
            elif a.startswith("--seed="):
                attendu["graine"] = int(a.split("=", 1)[1])
            elif a.startswith("--threads="):
                attendu["fils"] = int(a.split("=", 1)[1])
            elif a.startswith("--arch="):
                attendu["arch"] = a.split("=", 1)[1]
        if argv[1] == "--fichier":
            # § 5.18.4 : en mode fichier (runner, validateur) l'architecture
            # attendue est OBLIGATOIRE — sans elle un pilote 86 passerait la
            # famille suivante avant le rejeu final.
            if attendu["arch"] is None:
                print("REFUS : --arch=<architecture attendue> est obligatoire en mode --fichier")
                return 2
            try:
                with open(cible, encoding="utf-8", errors="replace") as fh:
                    texte = fh.read()
            except OSError as e:
                print("REFUS : fichier illisible (%s)" % e)
                return 1
        else:
            p = subprocess.run([cible] + args, capture_output=True, text=True, timeout=550)
            if p.returncode != 0:
                print("REFUS : pilote rc=%d\n%s%s" % (p.returncode, p.stdout, p.stderr))
                return 1
            texte = p.stdout
        verdict = juger(texte, ordre_base, repeat=repeat, min_lots=min_lots, **attendu)
        if verdict is not None:
            print("REFUS : %s\n%s" % (verdict, texte if argv[1] == "--binaire" else ""))
            return 1
        print("pilote juge conforme (%d records, ordre_base=%s)" % (repeat + 1, ordre_base))
        return 0
    print("usage : pilote_juge.py --binaire <pilote> [args] | --fichier <sortie> [args] | --contre-fixtures")
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))

