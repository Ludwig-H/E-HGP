# L2 : préciser les attributions, sans changer les mesures

Contrelecture du commit `e1f91d3b6`, source de sonde `a2c2fccfd`.
Proposition documentaire uniquement, non appliquée au produit ni aux reçus
publiés. Pins, calculs et empreintes proposées dans `capture.json` ; le
patch README L2 / PLAN s'applique et produit exactement les octets épinglés
sur une copie temporaire. Aucun moteur, build, GCP ou lecture de coordonnées.

- Les deux refus Lyon ne publient qu'une ligne `exit` avec
  `resource_exhausted/memory_budget`. Pas de mémoire ni d'étage. La sonde
  retourne au premier refus de `run_wall` (full_probe.cpp:298), puis de
  validation, avant `print_pass` (:305). Elle ne permet pas de décider si
  le refus vient du catalogue, d'un autre étage ou du contrôle final.
  Le budget concerné est celui de l'hôte, la voie jouée étant CPU.
- Sur les **deux succès Paris**, C porte bien le plus haut pic. Les
  rapports 99 987 683 104 / 26 819 742 820 = 3,728137… et
  147 151 303 928 / 38 327 850 820 = 3,839279… sont exacts. Leur
  dénominateur est l'usage du budget Session en fin de C, entrées/nuage/
  index compris (sonde:17–19,194–195), pas la taille isolée du catalogue.
  Ces ratios ne mesurent pas la mémoire des deux refus Lyon.
- `wide_leaf` sur ETH3D 16 828 368 sites établit le dépassement de la
  capacité 256 de ce parcours quantifié u21 : `catalogue.hpp:45–51`,
  `traversal_driver.hpp:173`, paramètres par défaut conservés par la sonde
  (:205–207). Le seul exit n'établit ni position près du scanner, ni
  densité sous-millimétrique, ni nombre exact de sites dans la feuille.
- Les succès restent deux passes initiales uniques. Le mur FULL exclut
  validation et destruction ; les secondes des refus sont des durées
  processus, sans ligne de mur FULL. Rien ne devient chaud ou GPU.

La projection initiale « catalogue à0,6–0,7s/M ⇒ environ80s » est une
substitution comptable entre entrées différentes, pas un chrono d'une
implantation en flux. Même dans cette hypothèse, diviser seulement G et T
par trois donnerait 42,94–44,40s, au-dessus de29,10s pour2s/M. Le patch
présente ce calcul comme conditionnel, sans loi de croissance ni gain
promis. Il conserve les données mesurées et l'intérêt de traiter les
grandes feuilles sur cette entrée réelle.

La provenance/arrêt est séparée dans
[session_l2_provenance](../session_l2_provenance/README.md) ; l'admission des
bruts est indépendante. Les archives primaires et leurs hashes restent
inchangés. Le constructeur peut porter ces précisions dans un erratum et
le PLAN vivant ; le patch fourni vise seulement à rendre les corrections
concrètes et relisibles au pin cité.
