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


def scene(somme, mur, residuel, composantes=None, fin=None, layout="classic",
          kind_construit=None):
    """Texte de profil synthetique minimal, un seul K, join=0.

    `layout` est la route DEMANDEE (signee sur profil_kind) ; `kind_construit`
    le kind signe par la ligne de tete du stockage et par `stockage_foret K=`
    (par defaut le kind coherent avec la demande).
    """
    if kind_construit is None:
        kind_construit = "csr_facet_keys_v1" if layout == "csr" else "vector_component_delta_v1"
    comp = composantes or {}
    keys = ["init", "touch", "pre", "unite", "post_remplissage",
            "materialisation_tri_copie", "liveness", "partition", "liberation"]
    cols = " ".join("%s=%.3f" % (k, comp.get(k, 0.0)) for k in keys)
    if fin is None:
        fin = mur
    return "\n".join([
        "profil_kind=reduce_v2 fold_join=0 pic_workers_b=2 pic_reduce_actif=1 layout=%s" % layout,
        "forest_layout=%s forest_storage_kind=%s csr_fallback=0 ordres_publies=1 "
        "ordres_storage_conformes=1" % (layout, kind_construit),
        "digest_forest_K1=" + "ab" * 32,
        "cardinalites K=1 events=1 facets=1 deltas=0 attachments=0 fusions=0 nodes=1",
        "profil_reduce K=1 %s somme=%.3f mur_reduce_interne=%.3f residuel=%.3f "
        "reduce_interne_debut=0.000 reduce_interne_fin=%.3f a_debut=0.000 a_fin=0.000 "
        "duree_digest_foret_k_ms=0.000" % (cols, somme, mur, residuel, fin),
        "profil_intern K=1 alloc_empreintes=0.000 offsets_diffusion=0.000 intern_tri=0.000 "
        "fusion_et_lib_parts=0.000 remap_et_lib_pools=0.000",
        "stockage_foret K=1 kind=%s deltas=0 cles_parents=0 cles_nes=0 meta=0/0 offsets=0/0 "
        "parents=0/0 nes=0/0 csr_capacity_growths=0 octets_possedes=0 exact=0 "
        "offset_dernier_parents=0 offset_dernier_nes=0" % kind_construit,
    ]) + "\n"


def juge(txt, layout="classic"):
    """rc du juge reel : None si accepte, code de sortie sinon."""
    try:
        profil_gate.check_profile_output(txt, 0, liveness=False, layout=layout)
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
    # 1bis/2bis. DENTS ISOLEES (audit : la scene 1 est aussi tuee par la
    # fermeture, donc un seuil somme regresse a 0.009 ne rougirait pas) :
    # (a) somme faussee de 0.008 avec fermeture EXACTE (residuel = mur) —
    #     seule la dent somme (0.0051) peut tuer, et son message est exige ;
    # (b) fermeture faussee de 0.007 avec somme EXACTE — seule la dent de
    #     fermeture (0.006) peut tuer ; (c) frontiere honnete : derive 0.005.
    import contextlib, io
    def juge_msg(txt):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = juge(txt)
        return rc, buf.getvalue()
    rc, msg = juge_msg(scene(somme=0.008, mur=0.010, residuel=0.010))
    if rc != 1 or "somme imprimee != somme des neuf composantes" not in msg:
        print("CONTRE-FIXTURE NON CAUSALE : derive de somme 0.008 a fermeture exacte "
              "non tuee PAR LA DENT SOMME (rc=%s, msg=%r)" % (rc, msg.strip()[:80]))
        echecs += 1
    else:
        print("contre-fixture tuee par la dent somme seule (derive 0.008, fermeture exacte)")
    comp = {"unite": 0.009}
    rc, msg = juge_msg(scene(somme=0.009, mur=0.010, residuel=0.008, composantes=comp, fin=0.010))
    if rc != 1 or "fermeture somme_recalculee+residuel != mur_reduce_interne" not in msg:
        print("CONTRE-FIXTURE NON CAUSALE : fermeture 0.007 a somme exacte non tuee PAR LA DENT "
              "FERMETURE (rc=%s, msg=%r)" % (rc, msg.strip()[:80]))
        echecs += 1
    else:
        print("contre-fixture tuee par la dent fermeture seule (ecart 0.007, somme exacte)")
    comp = {"unite": 0.009}
    rc = juge(scene(somme=0.014, mur=0.015, residuel=0.006, composantes=comp, fin=0.015))
    if rc is not None:
        print("FAUX ROUGE : derive de somme 0.005 (sous 0.0051) refusee (rc=%s)" % rc)
        echecs += 1
    else:
        print("frontiere honnete acceptee (derive de somme 0.005)")
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
    # 4. KIND CONSTRUIT (retour auditeur KeyCSR) : une sortie qui signe la
    #    demande layout=csr mais dont les kinds construits sont classiques
    #    (ligne de tete OU stockage_foret) DOIT etre refusee ; la sortie csr
    #    coherente reste acceptee (pas de faux rouge).
    comp = {k: 0.001 for k in ("init", "touch", "pre", "unite", "post_remplissage",
                               "materialisation_tri_copie", "liveness", "partition",
                               "liberation")}
    rc = juge(scene(somme=0.013, mur=0.015, residuel=0.002, composantes=comp, fin=0.015,
                    layout="csr"), layout="csr")
    if rc is not None:
        print("FAUX ROUGE : sortie csr coherente (kind construit csr) refusee (rc=%s)" % rc)
        echecs += 1
    else:
        print("sortie csr coherente acceptee (kind construit csr)")
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        rc = juge(scene(somme=0.013, mur=0.015, residuel=0.002, composantes=comp, fin=0.015,
                        layout="csr", kind_construit="vector_component_delta_v1"), layout="csr")
    msg = buf.getvalue()
    if rc != 1 or "forest_storage_kind=csr_facet_keys_v1 absent" not in msg:
        print("CONTRE-FIXTURE NON TUEE : layout=csr demande, kinds construits classiques "
              "acceptes (rc=%s, msg=%r)" % (rc, msg.strip()[:80]))
        echecs += 1
    else:
        print("contre-fixture tuee : demande csr, kind construit classique (ligne de tete)")
    txt = scene(somme=0.013, mur=0.015, residuel=0.002, composantes=comp, fin=0.015, layout="csr")
    txt = txt.replace("stockage_foret K=1 kind=csr_facet_keys_v1",
                      "stockage_foret K=1 kind=vector_component_delta_v1")
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        rc = juge(txt, layout="csr")
    msg = buf.getvalue()
    if rc != 1 or "stockage_foret sans kind construit csr_facet_keys_v1" not in msg:
        print("CONTRE-FIXTURE NON TUEE : ligne de tete csr mais stockage_foret classique "
              "accepte (rc=%s, msg=%r)" % (rc, msg.strip()[:80]))
        echecs += 1
    else:
        print("contre-fixture tuee : ligne de tete csr, stockage_foret K=1 classique")
    if echecs:
        return 1
    print("contre-fixtures du profil : seuils 0.0051/0.006 causaux, kind construit exige")
    return 0


if __name__ == "__main__":
    sys.exit(main())
