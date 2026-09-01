#!/usr/bin/env python3
"""Contre-fixture AUTONOME des seuils du profil (§ 5.13 de l'auditeur).

La scene historique — neuf composantes a zero, `somme=0.008`,
`residuel=0.012`, `mur_reduce_interne=0.020` — etait ACCEPTEE par les seuils
0.009/0.014. Avec les seuils serres (0.0051 pour la somme recalculee, 0.006
pour la fermeture), elle DOIT etre refusee ; et une scene aux arrondis %.3f
honnetes DOIT rester acceptee (le serrage ne fabrique pas de faux rouges).
Le juge exerce est le vrai `check_profile_output` de tests/profil_gate.py,
importe — jamais une reimplementation. Aucun assert (python3 -O).
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import profil_gate  # noqa: E402


def scene(somme, mur, residuel, composantes=None, fin=None):
    """Texte de profil synthetique minimal, un seul K, join=0."""
    comp = composantes or {}
    keys = ["init", "touch", "pre", "unite", "post_remplissage",
            "materialisation_tri_copie", "liveness", "partition", "liberation"]
    cols = " ".join("%s=%.3f" % (k, comp.get(k, 0.0)) for k in keys)
    if fin is None:
        fin = mur
    return "\n".join([
        "profil_kind=reduce_v2 fold_join=0 pic_workers_b=2 pic_reduce_actif=1",
        "digest_forest_K1=" + "ab" * 32,
        "cardinalites K=1 events=1 facets=1 deltas=0 attachments=0 fusions=0 nodes=1",
        "profil_reduce K=1 %s somme=%.3f mur_reduce_interne=%.3f residuel=%.3f "
        "reduce_interne_debut=0.000 reduce_interne_fin=%.3f a_debut=0.000 a_fin=0.000 "
        "duree_digest_foret_k_ms=0.000" % (cols, somme, mur, residuel, fin),
        "profil_intern K=1 alloc_empreintes=0.000 offsets_diffusion=0.000 intern_tri=0.000 "
        "fusion_et_lib_parts=0.000 remap_et_lib_pools=0.000",
    ]) + "\n"


def juge(txt):
    """rc du juge reel : None si accepte, code de sortie sinon."""
    try:
        profil_gate.check_profile_output(txt, 0, liveness=False)
    except SystemExit as e:
        return e.code if e.code is not None else 0
    return None


def main():
    echecs = 0
    # 1. La scene § 5.13 exacte : DOIT etre refusee (rc=1).
    rc = juge(scene(somme=0.008, mur=0.020, residuel=0.012))
    if rc != 1:
        print("CONTRE-FIXTURE NON TUEE : composantes nulles somme=0.008 "
              "residuel=0.012 mur=0.020 acceptees (rc=%s)" % rc)
        echecs += 1
    else:
        print("contre-fixture tuee : enveloppe a composantes nulles (somme)")
    # 2. Fermeture seule falsifiee (somme coherente, residuel gonfle) : refus.
    comp = {"unite": 0.009}
    rc = juge(scene(somme=0.009, mur=0.030, residuel=0.012, composantes=comp, fin=0.030))
    if rc != 1:
        print("CONTRE-FIXTURE NON TUEE : fermeture faussee de 0.009 acceptee (rc=%s)" % rc)
        echecs += 1
    else:
        print("contre-fixture tuee : fermeture somme+residuel != mur")
    # 3. Arrondis %.3f honnetes : neuf composantes de 0.001, somme=0.013
    #    (derive 0.004 < 0.0051), residuel=0.002, mur=0.015 (fermeture 0.004
    #    < 0.006) — DOIT rester acceptee, le serrage ne cree pas de faux rouge.
    comp = {k: 0.001 for k in ("init", "touch", "pre", "unite", "post_remplissage",
                               "materialisation_tri_copie", "liveness", "partition",
                               "liberation")}
    rc = juge(scene(somme=0.013, mur=0.015, residuel=0.002, composantes=comp, fin=0.015))
    if rc is not None:
        print("FAUX ROUGE : la scene aux arrondis honnetes est refusee (rc=%s)" % rc)
        echecs += 1
    else:
        print("scene aux arrondis honnetes acceptee (pas de faux rouge)")
    if echecs:
        return 1
    print("contre-fixtures du profil : seuils 0.0051/0.006 causaux")
    return 0


if __name__ == "__main__":
    sys.exit(main())
