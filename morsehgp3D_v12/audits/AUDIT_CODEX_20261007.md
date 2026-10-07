# Audit Codex — état courant v12

7 octobre 2026. Dernier pin éprouvé : **`274592a30`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Auditeur du développeur v12.

**Dernière relecture : contrat T2, lecteur de transition, corrections et microbancs G1/M7.**
[Rapport et témoins](../receipts/audit_t2_20261007/README.md) ; états au [registre unique](CONSTATS.md).

- **Corrections confirmées** : débordement du lecteur, collision fichier/dossier et provenance
  (`0224`–`0226` clos). Huit fichiers binaires synthétiques, 21 publications avec six pannes injectées.
  Les clôtures développeur `0003/0004/0014` sont contre-vérifiées. Les traces existantes corroborent
  `0024` : course du pool v10 gelé, porte désormais mono-fil ; 45 prises corrigées conformes.
- **T2 : mécanismes recevables sous conditions.** Lecture des racines de cellules déjà traitées,
  saut vers k sites strictement intérieurs et adaptateur de test : arguments favorables.
  Dix nuages, 35 ordres × trois politiques concordent avec l’oracle de définition ; six cibles inertes
  sont exercées. Les portes du mémo, de l’historique d’attache et des verticales natives restent à livrer.
- **Trois réserves nouvelles** : le lecteur fusionne deux identités de trame non ASCII (`0227`) ;
  profondeur d’attache et sites examinés ne sont pas invariants par changement de parcours (`0228`) ;
  le lemme hors catalogue exige k≥2 ou un rayon positif (`0229`). Aucun échec géométrique de T2
  n’est déduit de ces témoins.
- **Transition catalogue livrée (`0113`)** : 40 catalogues Fraction indépendants, 660 boules,
  coordonnées jusqu’à u32, indices permutés ; omissions et doublons unilatéraux détectés.
  Le contrôle catalogue ne qualifie pas les sorties `supports/cover`, Kruskal ni FULL.
- **G1/M7 relus sur six journaux** : 83,02 % des anciens census saturés K5 sont certifiables par
  les voisins, mais 28,7–30,4 % des cibles changent. Gain réel et coût de préparation restent à mesurer.
  M7 local attribue 41,60 % des cycles aux sondes ; ce n’est pas une mesure G4.
  G-L3 reste non adopté.
- **Juge G1 encore incomplet (`0018`)** : changer un octet de route masque un census saturé,
  code 0 et zéro écart ; sélectionner un ordre absent rend également un bilan vide de code 0.
  Ces témoins ne démontrent pas un défaut dans les prises historiques.

**Avant le natif T2** : traiter les cellules inertes et relire leur racine courante ; contrôler la
décroissance avant chaque `continuer` ; appliquer NUM-GARDE avant les prédicats mixtes de G-L3 ;
ne pas confondre census saturé et absence du catalogue ; spécifier le codage naissance/cellule
dans les cibles de quatre octets. Fixer l’ordre des opérations ou reclasser les compteurs concernés.

**Livraison ultérieure `1b40c0411`** : corrections des outils de données, du cache et des juges M2/M4/M5/M6
signalées au registre comme livrées, à contre-vérifier. Les 69 anciennes découpes restent à régénérer.

**Suite** : contre-vérifier ces corrections, puis les premiers raccords catalogue/tour et le juge EMST.
Tests synthétiques bornés et lecture de journaux seulement ; aucun GCP, LiDAR recalculé, matrice native
ou nouveau contrat D6/catalogue/FULL/100 ms acquis. Quatre fichiers courants ; détails sous `receipts/`.
Contrôle : `python morsehgp3D_v12/tools/check_constats.py` (structure, tailles et liens).
