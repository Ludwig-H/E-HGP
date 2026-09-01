# MorseHGP3D v6 — PORTE D'IDENTITE ET DE STRUCTURE DU PROFIL (§ 5.10 de
# REPONSE_AUDITEURS_MULTICPU_V6, contre-lectures 9cafe7b6 -> e32262d3).
#
# Ce qui est ATTESTE (projection deterministe NOMMEE, jamais « l'objet » ni
# tous les digests — batch_levels et le ForestResult complet restent hors
# preuve) : les lignes `digest_all=`, `digest_forest_K*` et
# `cardinalites K=` sont IDENTIQUES entre le binaire normal, le binaire de
# profil (cible CMake explicite — l'identite de build est signee par la
# cible), la variante vivacite si fournie, et les deux ordonnancements
# join 0/1.
#
# Structure du profil : `profil_kind` et `fold_join` signes ; ensembles de K
# COHERENTS entre lignes foret, cardinalites, profil_reduce, profil_intern
# (et profil_vivantes en variante) — l'ensemble exact K1..kmax releve de la
# porte exact-K du juge, pas d'ici ; temps finis non negatifs (intern
# compris) ; fermeture somme + residuel = mur_reduce_interne (bornes
# INTERNES du corps de reduce_fold) ; plancher de durees cumulees
# strictement positives (des records par defaut ne passent plus).
#
# CAUSALITE de fold_join : pour chaque K,
# a_debut <= a_fin <= reduce_interne_debut <= reduce_interne_fin ; sous
# join=1, reduce_interne_fin(K) <= a_debut(K+1) ET
# pic_reduce_actif == pic_workers_b == 1 — ignorer fold_join ne laisse plus
# la porte verte.
#
# DISCRIMINATION des builds (e32262d3) : les sorties du binaire normal ne
# contiennent AUCUNE ligne profil_* — passer le binaire de profil aux deux
# arguments echoue.
#
# Vivacite (si 3e argument) : profil_kind=reduce_v2+liveness exactement, un
# profil_vivantes par K, pic_intra_lot > 0 sur cette fixture et
# frontiere_max <= pic_intra_lot.
#
# Codes : 0 conforme ; 1 desaccord ; 2 refus ; 3 plancher. Aucun assert
# (doit tenir sous python3 -O).
import math
import subprocess
import sys


def run(binp, join):
    cmd = [binp, "--family=uniform", "--n=400", "--seed=3", "--threads=4",
           "--fold-join=%d" % join, "--digest"]
    p = subprocess.run(cmd, capture_output=True, text=True)
    if p.returncode != 0:
        print("REFUS : %s rc=%d" % (" ".join(cmd), p.returncode))
        sys.exit(2)
    # CONTRE-ENVELOPPE stderr (8e contre-lecture 99eec23d) : le contrat
    # § 5.10 imprime le profil par print_run sur STDOUT apres run_pipeline —
    # toute ligne profil_* sur stderr est une fuite d'I/O de worker.
    for line in p.stderr.splitlines():
        if line.startswith("profil_"):
            print("DESACCORD : ligne profil_* sur STDERR (fuite d'I/O) : %s" % line)
            sys.exit(1)
    return p.stdout


def projection(txt):
    return sorted(l for l in txt.splitlines()
                  if l.startswith("digest_all=") or l.startswith("digest_forest_K") or
                  l.startswith("cardinalites K="))


def fail(msg):
    print("DESACCORD : %s" % msg)
    sys.exit(1)


def k_of(line):
    # Deux formes : `... K=<n> ...` (cardinalites, profil_*) et
    # `digest_forest_K<n>=...`.
    if "K=" in line:
        s = line.split("K=", 1)[1]
    elif "_K" in line:
        s = line.split("_K", 1)[1]
    else:
        fail("K illisible dans : %s" % line)
    num = ""
    for ch in s:
        if not ch.isdigit():
            break
        num += ch
    if not num:
        fail("K illisible dans : %s" % line)
    return int(num)


def kset(txt, prefix):
    return sorted(k_of(l) for l in txt.splitlines() if l.startswith(prefix))


# SCHEMAS CONTRACTUELS par ligne (7e contre-lecture 9041c191 : une ligne
# `profil_intern K=n` sans aucune colonne passait — seule la finitude des
# champs PRESENTS etait verifiee ; les champs sont desormais exiges
# EXACTEMENT).
SCHEMA_REDUCE = {"K", "init", "touch", "pre", "unite", "post_remplissage",
                 "materialisation_tri_copie", "liveness", "partition", "liberation",
                 "somme", "mur_reduce_interne", "residuel", "reduce_interne_debut",
                 "reduce_interne_fin", "a_debut", "a_fin", "duree_digest_foret_k_ms"}
SCHEMA_INTERN = {"K", "alloc_empreintes", "offsets_diffusion", "intern_tri",
                 "fusion_et_lib_parts", "remap_et_lib_pools"}
SCHEMA_VIVANTES = {"K", "pic_intra_lot", "frontiere_max", "moyenne_frontiere_pct"}


def parse_rows(txt, prefix, schema):
    rows = []
    for line in txt.splitlines():
        if not line.startswith(prefix):
            continue
        fields = {}
        for tok in line.split()[1:]:
            name, _, val = tok.partition("=")
            try:
                fields[name] = float(val)
            except ValueError:
                fail("champ non numerique %s (%s)" % (name, line))
        if set(fields) | {"K"} != schema:
            fail("schema inattendu (%s) : champs %s, attendus %s" %
                 (prefix, sorted(set(fields) | {"K"}), sorted(schema)))
        for name, v in fields.items():
            if not math.isfinite(v):
                fail("champ non fini %s (%s)" % (name, line))
            if name != "residuel" and v < 0:
                fail("champ negatif %s (%s)" % (name, line))
        rows.append((k_of(line), fields, line))
    return rows


def check_profile_output(txt, join, liveness):
    kind = [l for l in txt.splitlines() if l.startswith("profil_kind=")]
    expected_kind = "profil_kind=reduce_v2+liveness" if liveness else "profil_kind=reduce_v2"
    if len(kind) != 1 or not kind[0].startswith(expected_kind + " "):
        fail("profil_kind attendu '%s' absent ou multiple (join=%d)" % (expected_kind, join))
    if ("fold_join=%d" % join) not in kind[0]:
        fail("fold_join non signe (join=%d)" % join)
    pics = {}
    for tok in kind[0].split():
        name, _, val = tok.partition("=")
        if name in ("pic_workers_b", "pic_reduce_actif"):
            pics[name] = int(val)
    kf = kset(txt, "digest_forest_K")
    kc = kset(txt, "cardinalites K=")
    kr = kset(txt, "profil_reduce K=")
    ki = kset(txt, "profil_intern K=")
    if not kf or kf != kc or kf != kr or kf != ki:
        fail("ensembles de K incoherents foret=%s cartes=%s reduce=%s intern=%s (join=%d)" %
             (kf, kc, kr, ki, join))
    if kf != sorted(set(kf)):
        fail("K non uniques (join=%d)" % join)
    rows = parse_rows(txt, "profil_reduce K=", SCHEMA_REDUCE)
    parse_rows(txt, "profil_intern K=", SCHEMA_INTERN)  # schema exact + temps finis non negatifs
    by_k = {}
    for k, f, line in rows:
        if abs(f["somme"] + f["residuel"] - f["mur_reduce_interne"]) > 0.005:
            fail("fermeture somme+residuel != mur_reduce_interne (%s)" % line)
        if f["residuel"] < -0.005:
            fail("residuel negatif (%s)" % line)
        if not (f["a_debut"] <= f["a_fin"] <= f["reduce_interne_debut"] <= f["reduce_interne_fin"]):
            fail("chaine A -> reduce_interne non ordonnee (%s)" % line)
        # PLANCHER PAR K (9041c191) : un record sans duree ne passe pas ; et
        # ATTRIBUTION NON NULLE (99eec23d : neuf composantes a zero avec
        # residuel == mur restaient vertes) — des que le mur est mesurable a
        # l'arrondi %.3f pres, la somme des fenetres doit etre positive.
        if f["mur_reduce_interne"] <= 0.0 and f["somme"] <= 0.0:
            fail("plancher : record K=%d sans duree (join=%d)" % (k, join))
        if f["mur_reduce_interne"] >= 0.002 and f["somme"] <= 0.0:
            fail("attribution nulle : mur=%.3f mais somme=0 (K=%d, join=%d)" %
                 (f["mur_reduce_interne"], k, join))
        by_k[k] = f
    if join == 1:
        # CAUSALITE : B(K) joint avant A(K+1), un seul worker et une seule
        # reduction en vol. join=0 reste PERMISSIF : la porte ne prouve PAS
        # qu'un chevauchement se produit (l'imposer sur un temps ou un pic la
        # rendrait sensible au scheduler).
        for k in sorted(by_k)[:-1]:
            if by_k[k]["reduce_interne_fin"] > by_k[k + 1]["a_debut"] + 0.005:
                fail("join=1 : reduce_interne_fin(K=%d) > a_debut(K=%d)" % (k, k + 1))
        if pics.get("pic_workers_b") != 1 or pics.get("pic_reduce_actif") != 1:
            fail("join=1 : pics attendus a 1 (workers_b=%s reduce_actif=%s)" %
                 (pics.get("pic_workers_b"), pics.get("pic_reduce_actif")))
    if liveness:
        kv = kset(txt, "profil_vivantes K=")
        if kv != kf:
            fail("profil_vivantes : ensemble de K different (%s vs %s)" % (kv, kf))
        for k, f, line in parse_rows(txt, "profil_vivantes K=", SCHEMA_VIVANTES):
            if f["pic_intra_lot"] <= 0:
                fail("vivacite : pic_intra_lot nul (%s)" % line)
            if f["frontiere_max"] > f["pic_intra_lot"]:
                fail("vivacite : frontiere > pic intra-lot (%s)" % line)


def main():
    if len(sys.argv) not in (3, 4):
        print("REFUS : usage profil_gate.py <mhgp6> <mhgp6_profile> [<mhgp6_profile_liveness>]")
        sys.exit(2)
    normal, prof = sys.argv[1], sys.argv[2]
    live = sys.argv[3] if len(sys.argv) == 4 else None
    outs = {}
    for tag, binp in [("normal", normal), ("profil", prof)] + ([("vivacite", live)] if live else []):
        for join in (0, 1):
            outs[(tag, join)] = run(binp, join)
    ref = projection(outs[("normal", 0)])
    if len(ref) < 3:
        print("PLANCHER : projection absente du run de reference")
        sys.exit(3)
    for key, txt in outs.items():
        if projection(txt) != ref:
            fail("projection differente pour %s" % (key,))
    # DISCRIMINATION des builds : le binaire normal n'emet AUCUNE ligne
    # profil_* — donner le binaire de profil aux deux arguments echoue ici.
    for join in (0, 1):
        leaked = [l for l in outs[("normal", join)].splitlines() if l.startswith("profil_")]
        if leaked:
            fail("binaire 'normal' emet du profil (%s) — mauvais binaire passe en premier argument" % leaked[0])
    for join in (0, 1):
        check_profile_output(outs[("profil", join)], join, liveness=False)
        if live:
            check_profile_output(outs[("vivacite", join)], join, liveness=True)
    # Surface CLI du refus SEULEMENT (le CLI n'appelle jamais print_run apres
    # un refus : la preuve causale sur le RunResult est la porte COMPILEE
    # mhgp6_profil_contrat_echec_k2).
    cmd = [prof, "--family=uniform", "--n=400", "--seed=3", "--threads=4",
           "--mem-budget=4096", "--digest"]
    p = subprocess.run(cmd, capture_output=True, text=True)
    if p.returncode != 2:
        fail("refus attendu rc=2 sous budget minuscule, obtenu rc=%d" % p.returncode)
    if [l for l in p.stdout.splitlines() if l.startswith("profil_") or l.startswith("digest_")]:
        fail("le refus emet une surface provisoire sur stdout")
    print("porte du profil : projection deterministe nommee (digest_all + digest_forest_K* + "
          "cardinalites K=*) identique, builds discrimines, structure/planchers/causalite fold_join valides")
    sys.exit(0)


main()
