# Plans par facteurs q3/q4 — capture locale close

26 septembre 2026, base `92c709bc8`. Prototype séparé du moteur :
[sources et preuve](../../audits/b_q34_factor_plan_20260926/README.md).
`exploration_v9_hors_registre`, `cpu_reference`,
`quantized_u18_input_only`, `audit_q34_factor_plan`, `not_claimed`.
**Aucun GCP, aucun census ni FULL dans cette capture.**

## Ce qui est réellement mesuré

Le front WSPD actuel `MidpointSamples`, puis le filtre universel de
rectangle `Affine`, puis un plan de crédits sur chacun des deux facteurs.
P est la masse de paires **après** le filtre rectangle existant ; E est
l'union résiduelle q3/q4 après le nouveau plan. Les paires éliminées ne
sont jamais développées dans cette sonde. La suite du générateur et la
tour ne sont pas exécutées. Les graines synthétiques sont celles des
[FULL précédents](../q3_payload_local_20260926/README.md), hash vérifié.

Les témoins propres à A et B sont disjoints, huit coins stricts par crédit,
seuils K−1/K−2 séparés, cœur extérieur h=0. Les groupes portent conjointement
les deux crédits ; leur produit ne duplique aucune paire. Pas de tableau
de taille P ni de histogramme local quadratique. Un plan coûte
O(KF + J), F la somme des tailles des facteurs traités, J le nombre de
couples de classes non vides ; le résidu aval s'ajouterait.

`min-factor=2` choisit seulement où préparer ce plan. Le produit entier
reste dans le résidu lorsque le plan est omis ; ce n'est ni un plafond de
recherche ni un sous-échantillonnage. Le cas `min-factor=1` est aussi mesuré.
Le saut logique lorsque les deux facteurs réunis ne pourraient fournir
assez de témoins est comptabilisé séparément.

## Résultats : trames entières, une mesure par configuration

Grille 1 mm, sans sol par masque figé de la trame entière ; trois trames
000000/000100/000200 de la **seule séquence 08**, pas trois séquences.
Les temps sont du CPU local mono sur hôte partagé, pas des temps G4.

| Entrée | K / s | P | E | Paires retirées | Préparation plans, ms |
|---|---|---:|---:|---:|---:|
| sans sol 00, 39 885 sites | 5 / 8 | 23 686 751 | 9 122 704 | 61,49 % | 675,9 |
| sans sol 01, 35 551 sites | 5 / 8 | 11 960 420 | 5 661 491 | 52,66 % | 465,0 |
| sans sol 02, 45 845 sites | 5 / 8 | 22 722 345 | 10 498 526 | 53,80 % | 792,8 |
| sans sol 00 | 10 / 8 | 30 777 213 | 18 231 486 | 40,76 % | 1 299,8 |
| sans sol 00 | 5 / 10 | 18 254 639 | 7 732 782 | 57,64 % | 627,3 |
| sans sol 00 | 5 / 12 | 14 757 258 | 6 880 859 | 53,37 % | 586,0 |
| brut 00, 123 389 sites | 5 / 8 | 22 034 426 | 12 135 064 | 44,93 % | 1 251,6 |
| brut 00 | 10 / 8 | 37 868 819 | 24 549 380 | 35,17 % | 1 686,5 |

Le nombre de points supérieur du brut ne force pas davantage de paires :
ses points supplémentaires peuvent être des témoins éliminateurs. Ne pas
construire une courbe de croissance en mélangeant ces deux régimes.
À s10/s12, E baisse mais F du front et le coût total de cette sonde montent ;
cela ne justifie pas un changement du défaut s8.

Sur 00/K5/s8, F du front vaut 16 108 374 ; F des plans 1 820 907 ;
45 147 499 coins sont testés. Il reste 659 165 descripteurs de plans et
1 024 326 descripteurs de repli. La somme des capacités des plans finis
vaut **125,20 Mo**, contre un maximum par plan de 14 370 octets.
Le premier chiffre modélise leur conservation simultanée, le second leur
traitement séquentiel : aucun des deux n'est un pic RSS/VRAM global.
Index, entrée, scratch, capacités de construction et replis restent
distincts. Le format actuel, à multiples vecteurs, n'est pas un format GPU.

`min-factor=1` retire seulement 16 693 paires de plus (0,183 % du résidu),
mais F passe à 1 940 833, coins à 47 323 756, capacités cumulées à
138,17 Mo et préparation à 720,9 ms. Pas de port de ce réglage pour lui-même.

## Croissance, sans extrapolation de contrat

| Régime K5/s8 | E à 8k | E à 16k | E à 32k | Ratios aux doublements |
|---|---:|---:|---:|---|
| uniforme | 435 709 | 908 050 | 1 876 820 | 2,084 / 2,067 |
| terrain | 140 079 | 285 967 | 591 278 | 2,041 / 2,068 |
| huit amas | 2 091 410 | 7 787 691 | 30 699 080 | 3,724 / 3,942 |

Uniforme/terrain avaient déjà presque ces masses : le plan ne gagne
respectivement que 27 et 23 paires à 32k. Sur amas, P était
28 352 433 / 112 770 168 / 449 652 733. Le gain de constante est grand,
mais la fraction conservée se stabilise à 6,8 % et le dernier exposant
diagnostique vaut **1,979** : ce n'est pas une solution sous-quadratique
établie pour ce régime. Préparation amas : F ×2,003/×2,003, coins
×2,003/×2,002 ; le carré n'est pas dans cette préparation, il persiste
dans les paires encore à examiner.

Les sept morceaux capteur de 00 sont vérifiés par coordonnées, IDs et
unions disjointes : trame 39 885 ; moitiés 24 591/15 294 ; quarts
11 536/13 055/8 225/7 069. Pas de taille forcée ni de nouveau masque.
Les six pentes `log(E_parent/E_enfant)/log(n_parent/n_enfant)` sont entre
**1,004 et 1,820**, contre 0,934 à 2,391 pour P. Mais une pente de
**tests de coins vaut 2,230** (quart x−/y+ vers moitié x−).
Les populations sont inhomogènes : ni un exposant asymptotique ni une
qualification sous-quadratique de toute la chaîne ne découle de ces
quelques coupes. Toutes les masses et les six relations sont publiées
dans [summary.json](summary.json).

## Portes, traçabilité et relecture

42 commandes closes : deux constructions neuves Release/Clang ASan/UBSan,
deux gates, six échecs causaux de mutants, deux oracles Fraction, inventaire
hôte/compilateurs, **24 mesures**. La gate native compare 216 fronts,
18 280 rectangles, 28 344 paires, 1 142 106 prédicats ponctuels,
25 900 tests de boîtes ; 72 refus et tous les masques sont exercés.
Mutants : égalité créditée, double compte, confusion rang/ID ; chacun
échoue pour sa réponse géométrique attendue, pas pour un crash.
L'oracle mathématique indépendant couvre 17 576 puissances de boules,
3 672 interpolations et 19 664 produits de classes. ASan/UBSan/LSan actifs,
pas de TSan ni de gate CUDA dans ce lot.

Les builds `v9-audit-factor-plan-r2-20260926_release` et `_sanitize`
sous `/workspaces/E-HGP/build/` sont épinglés après capture. Les archives
gen liées, les sources avant/après compilation, les binaires et recettes,
les entrées et leurs mappings sont contrôlés par le lecteur **LIVE**.
Les bruts de chaque commande sont conservés, `capture.json` lie leur
intention, argv, code, durée externe et fermeture du groupe de processus.
Les quatre vérifications complémentaires sont dans `checks/` : lecteurs
normal/−O et dix mutations de recette refusées sous chacun des modes.

```bash
python3 -B morsehgp3D_v9/audits/b_q34_factor_plan_20260926/validate_recipe.py --readback morsehgp3D_v9/receipts/q34_factor_plan_20260926
python3 -O -B morsehgp3D_v9/audits/b_q34_factor_plan_20260926/validate_recipe.py --readback morsehgp3D_v9/receipts/q34_factor_plan_20260926
```

Préflights hors capture : une copie de variable de boucle refusée par
`-Werror` et une parenthèse Python manquante ont été corrigées avant gel.
Les essais smoke antérieurs ne sont pas l'autorité des résultats ci-dessus.
Aucune capture close ni moteur n'a été réécrit.

## Décision développeur

Conserver ce plan exact comme brique et contre-épreuve de sélectivité,
**sans port CPU aveugle ni activation par défaut**. Le gain net FULL reste
inconnu. Le filtre de paires S2 G4 représente environ 25–29 ms dans le
reçu de cache antérieur ; enlever ces paires ne supprime pas les lanes
survivantes ni les centaines de ms du reste de la chaîne. Le même
pourcentage de gain ne s'applique donc pas à FULL.

Suite : [arène GPU/bandes conjointes et témoins adaptatifs](../../audits/b_q34_factor_plan_20260926/NEXT.md),
en parallèle du front compact et de la construction événementielle FULL.
Les 100 ms, le brut à 1 s, K10 à 1 s et plusieurs séquences restent ouverts.
