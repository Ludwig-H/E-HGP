# Plafond de tout proposeur de témoins du front q2

Auditeur B, 15 septembre 2026. Cadre : `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Sources du moteur épinglées au commit `2741d614`
(`git archive 2741d614 morsehgp3D_v8/src morsehgp3D_v8/bench morsehgp3D_v8/CMakeLists.txt morsehgp3D_v8/cmake`,
bibliothèque `mhgp8_p0` en Release). Mesures propres de l'audit, jamais une
qualification de vitesse ni un statut public.

## Question

La note constructeur `docs/P0_SURPROPOSITION_TEMOINS_Q2.md` (15 septembre 2026)
propose d'élargir la fenêtre historique de K propositions de `Front::filter`
à L = 2K ou 4K rangs autour du même pivot, sans nouvelle descente d'index.
Avant toute campagne, l'audit répond à trois questions mesurables :

1. **Plafond.** Quelle part des rectangles émis par le front (donc non rejetés
   par la fenêtre historique) admet au moins K sites z vérifiant le prédicat du
   front `H_min(A.box, B.box, {z}) > 0` ? Aucun proposeur de sites ponctuels
   testés contre les boîtes ne peut rejeter les autres, ni au produit lui-même,
   ni à un ancêtre du front : les boîtes d'un ancêtre contiennent celles du
   descendant, donc sa borne minimale est plus petite.
2. **Certificat de bloc.** Sur le chemin de descente du front vers le milieu du
   produit (mêmes distances, mêmes égalités que `Front::filter`), quelle part des
   rectangles possède un nœud à borne conjointe `A.box × B.box × Z.box`
   strictement positive et de taille au moins K ? C'est l'alternative « bloc »
   évoquée en fin de note.
3. **Fenêtres.** Quelle part des rectangles émis serait rejetée par la fenêtre
   L = 2K, puis L = 4K, avec la formule de fenêtre du front, les sauts des rangs
   de A et B et l'arrêt à K crédits stricts ? La fenêtre L = K rejouée doit
   rejeter zéro rectangle émis : c'est le contrôle de cohérence avec le front.

## Méthode

`witness_ceiling.cpp` reconstruit l'index `make_q2_cloud_index`, lance
`run_wspd_front` en mode `MidpointSamples` et, pour chaque rectangle portant la
voie q2, calcule :

- le nombre U de sites universels de boîte, plafonné à K, par parcours préordre
  de l'index avec `Q2JointPreparedBounds(A.box, B.box).bounds(z.box)` : un nœud
  à `minimum4 > 0` compte toute sa plage, un nœud à `maximum4 <= 0` est sauté,
  sinon on descend ; les feuilles sont des singletons, donc jamais indécises ;
- la taille du plus haut nœud du chemin du front à borne conjointe strictement
  positive (0 si aucun) ;
- les crédits stricts des fenêtres L = K, 2K, 4K (chaque site z est testé par
  la même borne conjointe, réduite au singleton, signe identique à `h_minimum`).

La masse d'un rectangle est `|A| × |B|`, c'est-à-dire ses candidats du census.
`run_plafond.py` grave `PLAFOND_PROPOSEUR_CHECKS.json` (empreintes du harnais et
du lanceur, commit du moteur, une entrée par configuration, contrôles :
fenêtre K à zéro rejet, chaque proposeur borné par le plafond, histogrammes
totalisant rectangles et masse). Aucun `assert` ; rejoué sous `python3 -O`.

Reproduction :

```bash
g++ -std=c++20 -O2 -Wall -Wextra -I<pin>/morsehgp3D_v8/src -I<pin>/morsehgp3D_v8/bench \
    witness_ceiling.cpp <pin>/build/libmhgp8_p0.a -o witness_ceiling
python3 -O run_plafond.py --binary ./witness_ceiling --engine-commit 2741d614
```

## Limites

- Les parts sont mesurées sur les rectangles **émis** ; le filtre du front
  s'applique à tous les produits visités, dont le coût n'est pas mesuré ici.
  Le plafond borne ce que toute extension peut retirer de l'entrée du census,
  pas le temps du front.
- Le prédicat est celui du front (boîtes de A et B) ; un site intérieur à
  toutes les boules des paires réelles mais non certifié par les boîtes n'est
  pas compté. Le plafond est donc celui des proposeurs qui gardent ce prédicat.
- Le pivot rejoué suit le chemin `MidpointSamples` du front à `2741d614` ; une
  autre règle de pivot déplace les fenêtres, pas le plafond ni le bloc.

## Résultats (`PLAFOND_PROPOSEUR_CHECKS.json`, graine 3, front `MidpointSamples`)

Parts de rectangles émis (voie q2) rejetables, puis part de la masse de
candidats correspondante. « Fenêtre K » vaut 0 partout, comme attendu.

| Famille | n | K | s | Rectangles | Masse | Plafond rect. / masse | Bloc rect. / masse | Fenêtre 2K rect. / masse | Fenêtre 4K rect. / masse |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| uniform | 8 000 | 10 | 8 | 1 966 080 | 3 194 249 | 86,3 % / 90,9 % | 22,6 % / 33,1 % | 63,0 % / 72,4 % | 71,8 % / 79,7 % |
| uniform | 8 000 | 5 | 8 | 1 038 074 | 1 413 285 | 86,4 % / 89,6 % | 23,3 % / 30,4 % | 58,6 % / 65,2 % | 71,4 % / 76,8 % |
| uniform | 8 000 | 10 | 12 | 2 356 825 | 2 986 552 | 88,3 % / 90,5 % | 26,7 % / 32,9 % | 67,0 % / 71,9 % | 75,2 % / 79,2 % |
| uniform | 16 000 | 10 | 8 | 4 532 284 | 7 347 458 | 87,7 % / 91,9 % | 17,4 % / 26,3 % | 62,4 % / 71,4 % | 75,1 % / 82,2 % |
| uniform | 32 000 | 10 | 8 | 10 180 690 | 17 325 368 | 88,8 % / 92,9 % | 21,2 % / 30,5 % | 63,2 % / 72,7 % | 75,2 % / 82,9 % |
| uniform | 32 000 | 5 | 8 | 4 610 698 | 6 433 954 | 87,2 % / 90,4 % | 29,7 % / 38,0 % | 61,5 % / 68,5 % | 71,0 % / 76,9 % |
| clusters | 8 000 | 10 | 8 | 1 210 070 | 29 728 389 | 80,5 % / 5,0 % | 15,2 % / 1,3 % | 55,1 % / 3,7 % | 65,2 % / 4,2 % |
| clusters | 8 000 | 5 | 8 | 732 748 | 28 933 876 | 82,7 % / 2,8 % | 18,8 % / 0,8 % | 53,8 % / 1,9 % | 67,4 % / 2,4 % |
| clusters | 32 000 | 10 | 8 | 7 579 029 | 460 078 566 | 86,2 % / 2,4 % | 18,0 % / 0,7 % | 59,2 % / 1,8 % | 72,0 % / 2,1 % |
| clusters | 32 000 | 5 | 8 | 3 747 458 | 453 056 753 | 85,3 % / 1,0 % | 26,7 % / 0,4 % | 58,8 % / 0,7 % | 68,8 % / 0,8 % |
| terrain | 8 000 | 10 | 8 | 432 884 | 935 699 | 68,2 % / 82,1 % | 13,7 % / 26,2 % | 45,3 % / 61,9 % | 57,2 % / 73,2 % |
| terrain | 8 000 | 5 | 8 | 244 044 | 398 032 | 69,3 % / 79,0 % | 21,6 % / 33,0 % | 46,6 % / 58,5 % | 55,5 % / 66,8 % |
| rows | 8 000 | 10 | 8 | 69 677 | 16 091 188 | 2,1 % / 0,0 % | 0,0 % / 0,0 % | 2,1 % / 0,0 % | 2,1 % / 0,0 % |
| rows | 8 000 | 5 | 8 | 39 971 | 16 039 970 | 0,0 % / 0,0 % | 0,0 % / 0,0 % | 0,0 % / 0,0 % | 0,0 % / 0,0 % |

Coût par rectangle émis (évaluations de borne conjointe) : descente exacte
plafonnée à K de 48 à 91 selon la famille et n (82 à uniform 8k K = 10, 91 à
32k) ; chemin du front 13 à 15 nœuds ; fenêtres K + 2K + 4K rejouées ensemble
25 à 63 tests de sites, une seule fenêtre 4K en coûte au plus 4K.

Lecture :

- **Le plafond est élevé et monte avec n sur les nuages génériques** : 86 à
  89 % des rectangles émis à uniform portent au moins K sites universels de
  boîte, 68 à 69 % à terrain. Le filtre historique les laisse passer parce que
  ses K propositions doivent toutes réussir. À uniform 8k K = 10, la masse
  plafonnée (2 904 816) recouvre 99,7 % des 2 914 705 candidats que le
  census rejette ensuite selon le reçu constructeur cité par la note.
- **La fenêtre 4K atteint 80 à 85 % du plafond** (72 à 75 % des rectangles à
  uniform, 55 à 57 % à terrain), la fenêtre 2K 70 à 76 %. Le supplément 2K → 4K
  vaut encore 9 à 13 points. Ce sont des parts de rectangles ; la masse suit
  d'un peu plus haut sur uniform et terrain.
- **Sur les amas, le gain est en rectangles, pas en masse** : 95 à 99 % des
  candidats vivent dans les rectangles amas × amas sans aucun témoin universel
  (boîtes larges, milieu vide). Ils restent au Pool et au census quel que soit
  le proposeur ; l'extension n'y change que le nombre d'appels.
- **Sur les rangées, tout proposeur est un pur surcoût** : plafond 0 à 2 %,
  masse 0 %. C'est la contre-fixture de la note (« extension efficace contre
  simple surcoût ») : uniform et rows sur le même harnais.
- **Le certificat de bloc du chemin du milieu est faible** : 14 à 30 % des
  rectangles ; le plus haut nœud positif du chemin a le plus souvent 2 à 9
  sites (histogrammes `block_hist` du reçu). Il ne remplace pas la fenêtre,
  mais il ne coûte qu'une borne par nœud du chemin déjà parcouru et peut la
  précéder.
- **La descente exacte plafonnée à K atteint le plafond entier** pour 48 à 91
  bornes par rectangle, soit l'ordre d'une fenêtre 4K à K = 10 ; elle n'a pas
  de rang à sauter et ne dépend pas d'un pivot. C'est une option mesurée, pas
  une recommandation : son coût croît lentement avec n et se paie sur tous
  les produits visités par le front.
