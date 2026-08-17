# Note de Claude — ordre des niveaux U192 et découplage q4 gravés

Date : 17 août 2026. Clôt les points 2 (contre-audit `489c617`) et 1 (audit
`bc5b05d`) de votre ordre, après le `Q3Event` matérialisé du commit
précédent.

## 1. Comparateur exact des niveaux (489c617, § 2)

`compare_level` par produits croisés `num1·den2 ? num2·den1` en **U192**
(trois mots, `mul_level_192` : 128×128 → 192, précondition prouvée
`num < 2^101`, `den < 2^70`, produit `< 2^171`). Porte
`mhgp4_q3_level_cmp_accord` :

- 2 381 niveaux récoltés (tous les triangles aigus de `uniform n=32`,
  équilatéral maximal M=65535, presque-rectangle aigu) ;
- 2 833 390 paires jugées contre l'oracle obigint 384 bits : 0 désaccord ;
- antisymétrie et canonicité (égalité ⟺ mêmes fractions réduites)
  vérifiées sur chaque paire ;
- **10 plateaux** observés (l'équilatéral maximal en produit par symétrie) :
  le plancher `plateaux >= 1` garde vivant le cas qui imposera les
  macro-lots ;
- fixture de largeur gravée : produits croisés égaux sur les 128 bits bas,
  différents au seul mot haut ;
- mutant `level-trunc-hi` (mot haut tronqué) tué par cette fixture (code 4).

Le contrat macro-lot (racines gelées avant le lot, toutes les multifusions
du plateau ensemble, pas de chronologie binaire artificielle) est gravé en
commentaire de `compare_level` et dans le plan de tests ; son juge viendra
avec la forêt, qui n'existe pas encore.

## 2. Fixture bloquante `q4_source_independent_from_q3` (bc5b05d, § 2)

Vos 13 points gravés tels quels (`tests/q4_source_fixture_test.cpp`),
toutes les valeurs re-vérifiées par les prédicats de production :

- `n3 = 9` (les neuf `z_i` dans `W_3`, x et y hors des deux fuseaux,
  vérifié plutôt que supposé) ⟹ ancre q3 MORTE (`h_3 = 9`) ;
- `n4 = 0` ⟹ ancre q4 VIVANTE (`h_4 = 8`) ;
- tétraèdre : longueurs 40000/39600 gravées, owner `EdgeKey(0,1)`,
  circumcentre entier `(200,230,300)`, `R² = 14900`, les neuf distances
  carrées gravées (15025..15901) toutes strictement extérieures,
  profondeur q4 = 0 ;
- faces `abx`/`aby` strictement aiguës et chaque `z_i` intérieur à leurs
  circum-boules (`q3_power < 0`) : le tétraèdre est bien invisible depuis
  les événements q3 comme depuis les ancres q3 vivantes ;
- mutant `q4-seeds-from-q3-live` (une source qui ne sème que depuis les
  ancres q3 vivantes) : perd le support, code 4.

37 portes CTest vertes. Prochaine étape selon votre ordre : `AcuteSeed`
extrait en amont du census q3 (bc5b05d § 3.1), puis l'ouverture q4 réelle
depuis la lane q4 du front partagé (paquet `base4`, cover d'arête à
coefficient 4, sélection axiale) — la fixture ci-dessus deviendra alors une
porte de bout en bout sur le vrai pipeline, au lieu du simulacre de source
qu'elle juge aujourd'hui (le champ `supports` du test est encore un
branchement simulé, dit honnêtement).
