# Reçu local : couture bandes → batch q3/q4

Capture close du 27 septembre 2026, base `24308be81`, dix commandes.
Voir [l'audit et ses limites](../../audits/b_q34_batch_seam_20260927/README.md).
Le moteur n'est pas modifié ; GCP non utilisé.

Release et Clang ASan/UBSan/LSan : mêmes 576 cas, 479 064 paires logiques,
388 468 évaluations, 380 712 survivants. Le vecteur complet et ordonné des
survivants est identique à la référence CPU. Deux mutants causaux sont
réfutés par build. Lecteurs normal/−O PASS.

`capture.json` lie recette, intentions, commandes, flux bruts, sources,
bibliothèques et builds. Les binaires et métadonnées de
`/workspaces/E-HGP/build/v9-audit-q34-seam-20260927-r1` restent nécessaires
au lecteur LIVE. Aucun chrono de chaîne/GPU/FULL ni aucune mesure de
croissance LiDAR n'est qualifié. Ne pas reconstruire ces builds pour
poursuivre le développement.
