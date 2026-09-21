# Régime réel des rectangles WSPD : où se trouve le travail à 8k–256k

14 septembre 2026, après **1bf806f0**. Audit indépendant v8.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

> **Statut au 21 septembre 2026 (auditeur B)** : note historique de la première lecture (sources 1bf806f0), conservée pour son reçu [wspd_regime_20260914/](wspd_regime_20260914/WSPD_REGIME_CHECKS.json). La convention de séparation et le régime pur v4 sont repris dans [SEPARATION_20260914.md](SEPARATION_20260914.md) ; aucune pente n'est à tirer d'ici pour les sources actuelles.

Les cinq tranches P0 mesurent le résidu q2 d'**un** rectangle dont chaque
facteur porte 8 000 à 32 000 sites (grilles, nappes, `skew`). Aucune WSPD
n'est encore générée en v8. Cette note mesure donc, sur le front WSPD pur
de la v4 (même arbre radix de Morton, même prédicat de séparation entier,
sans élimination par témoins), la distribution réelle des tailles de
facteurs sur les familles du plan de test. Le but est de situer le régime
que P0 optimise, pas de qualifier un moteur.

## 1. Ce qui a été mesuré

Le harnais [wspd_factor_hist.cpp](wspd_regime_20260914/wspd_factor_hist.cpp)
reproduit `wspd_wavefront` comme le probe
`morsehgp3D_v4/bench/wspd_scaling_probe.cpp`, puis classe chaque rectangle
terminal par `max(|A|,|B|)` : 1, 2–7, 8–63, 64–1023, ≥1024. Il vérifie le
ledger de masse exact (somme des `|A||B|` égale au nombre de paires de
positions distinctes) et rend un code non nul sinon. Le runner
[run_wspd_regime.py](wspd_regime_20260914/run_wspd_regime.py) compile en
`-Werror`, exécute la matrice, épingle par SHA-256 le harnais et les trois
en-têtes v4 consommés, et écrit le reçu
[WSPD_REGIME_CHECKS.json](wspd_regime_20260914/WSPD_REGIME_CHECKS.json)
(21 exécutions, sorties brutes, rapports par doublement, digest stable hors
temps). Le rejeu `python3 -O --skip-large` rend les mêmes comptes sur les
19 configurations rejouées.

```bash
python3 -B morsehgp3D_v8/audits/wspd_regime_20260914/run_wspd_regime.py --build-dir /tmp/wspd_regime_build --output /tmp/wspd_regime.json
python3 -B -O morsehgp3D_v8/audits/wspd_regime_20260914/run_wspd_regime.py --build-dir /tmp/wspd_regime_build --skip-large --output /tmp/wspd_regime_O.json
```

Graine 3, `s=8` au sens v4 : le code teste exactement `d ≥ (s+2)·R_max`
sur les boîtes serrées, avec `d` la distance des centres et `R_max` la plus
grande demi-diagonale ; cela implique `d − r_A − r_B ≥ s·max(r_A,r_B)` sans
équivalence quand les deux rayons diffèrent (précision due à l'auditeur
indépendant, avec l'exemple entier A={(0,0,0),(2,0,0)}, B={(10,0,0)} : la
seconde formule passe à s=8, le code refuse). Emprise par défaut de chaque
famille.

## 2. Résultats

Rectangles terminaux du front pur, `s=8` v4, familles du plan de test :

| Famille | n | Rectangles | Rect./point | Σ(|A|+|B|) | Plus grand facteur | Rect. avec `max ≤ 7` | Masse de paires dans `max ≥ 64` |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| uniform | 8 000 | 3 435 133 | 429 | 1,78·10⁷ | 46 | 93,9 % | 0 % |
| uniform | 16 000 | 8 331 756 | 521 | 5,08·10⁷ | 69 | 89,9 % | 0,00 % |
| uniform | 32 000 | 19 574 390 | 612 | 1,41·10⁸ | 146 | 86,0 % | 1,1 % |
| terrain | 8 000 | 682 962 | 85 | 5,78·10⁶ | 186 | 81,1 % | 16,9 % |
| terrain | 16 000 | 1 467 396 | 92 | 1,51·10⁷ | 329 | 77,8 % | 44,3 % |
| terrain | 32 000 | 3 082 791 | 96 | 3,84·10⁷ | 694 | 75,0 % | 67,2 % |
| eight_clusters | 8 000 | 1 654 169 | 207 | 9,22·10⁶ | 84 | 90,4 % | 1,7 % |
| eight_clusters | 16 000 | 4 368 608 | 273 | 2,51·10⁷ | 269 | 93,1 % | 21,7 % |
| eight_clusters | 32 000 | 11 062 220 | 346 | 6,68·10⁷ | 314 | 94,1 % | 53,0 % |

Aucun rectangle n'a de facteur d'au moins 1 024 sites dans ces neuf
configurations ; aucun facteur ne dépasse 694 sites. Les facteurs de la
taille des fixtures P0 (8 000 à 32 000 sites) n'apparaissent pas.

Série uniforme prolongée, `s=8` v4 :

| n | Rectangles | Rect./point | Rapport rectangles au doublement | Rapport Σ facteurs au doublement | Plus grand facteur |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | 3 435 133 | 429 | — | — | 46 |
| 16 000 | 8 331 756 | 521 | 2,43 | 2,85 | 69 |
| 32 000 | 19 574 390 | 612 | 2,35 | 2,77 | 146 |
| 64 000 | 44 943 695 | 702 | 2,30 | 2,73 | 293 |
| 128 000 | 100 186 048 | 783 | 2,23 | 2,58 | 480 |
| 256 000 | 219 063 683 | 856 | 2,19 | 2,52 | 977 |

Le nombre de rectangles par point croît d'environ 90 par doublement sur
tout l'intervalle : le front pur n'est pas linéaire en n dans cette plage,
et la somme des tailles de facteurs croît plus vite encore (exposant
observé proche de 1,4). Scinder la plus grande **cellule de Morton** au
lieu du plus grand diamètre de boîte serrée (`--split=level`) donne 12 à
15 % de rectangles **de plus** aux quatre tailles 8k–64k, avec les mêmes
rapports au doublement : la règle de scission n'explique pas cette
croissance. La cause reste à établir (effets de bord de l'emprise finie,
quantification u16 ou constante de packing encore loin de sa saturation) ;
le constructeur avait déjà noté que la borne O(s³n) n'est pas démontrée
pour ce trie.

Effet de `s`, uniforme 32k, convention v4 :

| s | Rectangles | Σ(|A|+|B|) | Plus grand facteur |
| ---: | ---: | ---: | ---: |
| 8 | 19 574 390 | 1,41·10⁸ | 146 |
| 10 | 29 690 097 | 1,88·10⁸ | 85 |
| 12 | 41 402 177 | 2,35·10⁸ | 62 |
| 14 | 54 323 619 | 2,80·10⁸ | 52 |
| 16 | 68 159 956 | 3,23·10⁸ | 44 |
| 18 | 82 677 809 | 3,63·10⁸ | 36 |

## 3. Trois conséquences pour la v8

**La convention de séparation v8 n'est pas celle de la v4/v7.** La factory
`prepare_rectangle` exige `gap(boîtes) ≥ s·max(diam)` avec `diam` la
diagonale entière, soit une distance de centres `d ≥ 2sR` en rayons de
boîte, là où le prédicat v4 exige `d ≥ (s+2)R`. Avec `g ≤ d ≤ g + 2R`
(`g` distance des boîtes), la convention v8 à `s=8` est encadrée par les
conventions v4 à `s=16` et `s=14` (encadrement confirmé par l'auditeur
indépendant) : entre 54 et 68 millions de rectangles à 32k uniforme au lieu
de 19,6 millions, sur le même arbre, les mêmes graines et la même scission
du front pur, avec des facteurs encore plus petits (au plus 52 puis 44
sites). Cet encadrement ne prédit pas le futur front v8 avec élimination
précoce ; il dit seulement que les deux `s` ne désignent pas le même
objet. Avant toute comparaison
« s8/10/12 » du plan de refonte, fixer une seule définition entière de la
séparation, la citer dans le futur constructeur WSPD et calibrer `s` sur
cette définition ; sinon les mesures P0 (qui vérifient seulement la
séparation d'un rectangle fixe) et les futures mesures WSPD ne parleront
pas du même objet. Le lemme des tubes exige `D ≥ 10R` et le code le
vérifie directement, indépendamment de `s` : ce point-là est sain.

**Le régime dominant est le très grand nombre de très petits rectangles.**
Sur les familles du plan de test, 75 à 94 % des rectangles ont deux
facteurs d'au plus sept sites, et la masse de paires se concentre dans les
classes 8–63 et 64–1023. Le contrat « toute la tour à 50k sous une
seconde » se traduit, pour le seul front, par environ 33 millions de
rectangles à `s=8` v4 (interpolation 32k–64k), soit un budget de l'ordre de
30 ns par rectangle en équivalent séquentiel : tout coût fixe par
rectangle (copie ou validation de n sites, construction d'un index, tri
d'un facteur, allocation) est disqualifiant pour cette population, comme
le constructeur l'écrit déjà pour la copie de n sites. Le chemin des
petits facteurs doit donc être un chemin scalaire sans préparation, et le
front lui-même (séparation, cœur, scission) est le premier poste massif à
concevoir résident et plat. Les tranches P0 s'appliquent aux quelques
milliers de rectangles à facteurs moyens et aux rectangles racines des
nuages en amas très séparés (le triplet « deux amas » v7, absent des
familles ci-dessus) ; leur intérêt dépend donc de la famille visée et
doit être établi par la somme sur tous les rectangles d'une WSPD, pas sur
un rectangle isolé.

**Le critère de clôture de P0 exige un pilote WSPD réel.** Le plan de
refonte demande de juger P0 sur le travail total, coût aval compris. Sans
pilote WSPD v8, il n'existe aucune mesure de la somme des résidus, des
visites et des sorties sur les rectangles réellement produits à 8k/16k/32k.
La passation courante place déjà le partage nuage/index avant la suite ;
cette note appuie cet ordre et propose d'y ajouter, dès la première
tranche WSPD, les compteurs du présent reçu (rectangles, Σ facteurs, plus
grand facteur, masse par classe) et les mêmes séries 8k→64k, afin de
détecter tout de suite une croissance non linéaire du front v8.

## 4. Limites

Front pur v4 : l'élimination précoce par témoins du front fusionné v7
réduisait le nombre de rectangles terminaux (754 686 au lieu de 3 435 133
sur l'uniforme 8k, d'après `WSPD_Q2_Q3_Q4.md` §7) ; la répartition par
classes après élimination n'est pas mesurée ici. Une seule graine, emprise
par défaut, machine partagée : les temps du reçu sont indicatifs, les
comptes sont déterministes. Aucun résultat v8 n'est requalifié ; aucune
borne théorique n'est établie ou réfutée par ces séries finies.
