# Première occurrence — qualification structurelle close

27 septembre 2026. [Prototype, preuve et limites](../../audits/b_full_first_parent_20260927/README.md).
Pas de moteur modifié, pas de GPU/GCP, pas de mesure de performance.

`r1/capture.json` : 16 commandes closes, premier essai PASS.
Release et ASan/UBSan/LSan ont chacun 9 647 entrées, 19 294 comparaisons
champ à champ, 805 acceptations et 8 842 refus. 5 642 passages rapides
et 13 622 replis attestent que les deux chemins sont exercés ; les 30
autres comparaisons sont refusées à la forme/domaine.

Huit exécutions mutantes (quatre par build) réfutées sémantiquement avec
code 1 et stdout attendu, stderr vide. Digest commun des gates
`10980430703755123391`. Lecteurs normal et `-O` PASS après clôture.

Les sources, bibliothèques de types partagées et dépendances compilées,
commandes, options sanitizer et sorties sont épinglées. Les deux binaires
du build `/workspaces/E-HGP/build/v9-audit-full-first-parent-20260927-r1`
sont nécessaires au lecteur LIVE et ne doivent plus être reconstruits.

L'absence de continuation est testée globalement à chaque appel, sinon
repli intégral. Les tests ne qualifient ni la géométrie ni les verticales,
ni l'ordonnancement CPU/GPU ; aucun temps de la capture antérieure sur
vrais drafts ne qualifie ce nouveau prototype.
