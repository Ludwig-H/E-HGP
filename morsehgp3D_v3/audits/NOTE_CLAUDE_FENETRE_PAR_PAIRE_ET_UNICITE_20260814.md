# Note de Claude : la fenêtre décidée par paire, et l'unicité de l'événement

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=mesure_exploratoire`,
`public_status=not_claimed`.

Cette note répond à
[`AUDIT_LIVE_BLOCK_JUNG_CREDITS_TAU_783A789_20260814.md`](AUDIT_LIVE_BLOCK_JUNG_CREDITS_TAU_783A789_20260814.md)
et apporte trois mesures que l'audit ne contient pas. Aucun de ses chiffres ne
qualifie un SLO ; ils servent à décider quelle quantité doit être gatée.

## 1. Corrections reçues et appliquées

L'audit live a relu mon worktree pendant que je le modifiais. Ses deux P0 sont
réparés au HEAD courant :

- **le refus de juger n'est plus un accord**. `--juge-bjd=1` publiait
  `groupes=158 sautes=98` avec `OK`, `fenetre_finale=OUI` et le code zéro. Le
  statut est désormais `COMPLET` ou `PARTIEL`, et un seul claim non jugé rend le
  code `3`. Porte : `mhgp3v_bjd_refus_juge_partiel` ;
- **les modes vacuaires sont refusés avant calcul**. `--bjd-groupes` sans
  `--vwave` rendait zéro avec `essais=0`. Il rend maintenant le code `2`, comme
  `--juge-bjd` sans groupes et un mutant sans juge. Portes
  `mhgp3v_bjd_refus_sans_vwave` et `mhgp3v_bjd_refus_mutant_sans_juge`.

La fixture `seven_collinear_plus_reused_pair` est gravée comme famille de
réfutation `collinear_seven`, aux coordonnées exactes de l'audit, à côté de
`two_lines`. Avec `a=(0,0,0)`, `b=(10,0,0)` et `z_i=(i,0,0)`, la différence
entre rayon carré et distance carrée vaut exactement `i(10-i)>0` pour tout
centre d'une sphère par `a,b` : sept témoins universels, profondeur exactement
sept, seuil q4 à huit. Aucun terminal ne doit fermer.

Le sujet réparé forme **zéro** groupe sur cette fixture : ses cinquante-cinq
témoins libres sont tous déjà crédités, donc la banque est vide. C'est le
comportement exact attendu. Sous `--inject-bjd-reutilise`, un terminal ferme et
le juge de fermeture le réfute : code `4`. Deux portes déterministes,
`mhgp3v_bjd_fixture_collineaire` et son mutant.

Les planchers de non-vacuité sont devenus **explicites** (`--min-bjd-groupes`,
`--min-bjd-fermetures`) précisément parce que la fixture exige de ne rien
fermer : un plancher câblé dans le sujet aurait rendu cette absence
indistinguable d'une panne.

Restent ouverts, et je ne les conteste pas : le typage `INVALID_OR_UNKNOWN` de
`bjd_lane_box`, le cap de somme de poids dans `dual_lane`, le reçu d'identités
sur `spid` plutôt que sur des rangs, la séparation complète des deux pools, et
le remplacement du glouton par `tau(F)`.

## 2. Le crédit de groupe : chiffres définitifs

Mon propre titre de raccord est retiré. Le gain annoncé à `12,8 %` valait
`0,9 %` :

| famille, `n=1500` | `E4` central | `+SOC64` | `+BJD` |
|---|---:|---:|---:|
| `eight_clusters` | 1 071 162 | 922 141 | 914 118 |

L'audit a raison sur le coût : le certificat est placé **après** la descente,
donc il ne retire aucune visite. Je confirme sa mesure par la mienne, sur cinq
tailles et deux familles : les pentes de `sum_E4` restent

```text
eight_clusters central : 1,917  1,904  1,907  1,900
eight_clusters +SOC64  : 1,847  1,856  1,898  1,942
eight_clusters +BJD    : 1,847  1,857  1,901  1,946
uniform        +BJD    : 1,162  1,193  1,093  1,065
```

Le gain est une constante de dix-neuf pour cent qui **s'érode** : la pente
remonte vers `1,95` quand `n` croît. Aucun renforcement du prédicat ne changera
cet exposant. Je considère cette voie close et je ne demande pas de rampe G4
pour elle.

## 3. La faute de métrique : `sum_E4` n'est pas un coût

`sum_E4` est la **masse logique** des paires ouvertes. Ce n'est le coût que si
les paires sont développées — ce que toute l'architecture interdit. J'ai donc
mesuré séparément le nombre d'**enregistrements physiques** ouverts, sur la même
configuration et les mêmes graines :

| `eight_clusters` q4 | 1 500 | 3 000 | 6 000 | 12 500 | 25 000 | pentes |
|---|---:|---:|---:|---:|---:|---|
| masse ouverte | 914 118 | 3 287 809 | 11 910 722 | 48 066 852 | 185 163 358 | 1,85 → **1,95** |
| rectangles ouverts | 100 316 | 235 929 | 531 588 | 1 211 785 | 2 543 020 | 1,23 → **1,07** |

Les deux quantités divergent : la masse converge vers l'exposant deux, les
enregistrements vers l'exposant un. C'est le comportement normal d'une WSPD —
représenter un nombre quadratique de paires par un nombre quasi-linéaire de
rectangles est exactement ce pour quoi elle existe. Les rectangles ouverts
deviennent simplement plus gros : masse par rectangle `9,1` puis `73`.

`PROPOSITION.md` §11 le dit déjà — « *sparse qualifie seulement les
enregistrements physiques restant factorisés* » — mais la quantité gatée est
restée la masse, y compris dans le critère de mort de l'étape un et dans
`check_rampe_pentes.py`.

## 4. L'unicité de l'événement, et ce qu'elle réfute

L'observation suivante vient de Louis. Un support ne définit pas une famille
d'événements : il en définit **un seul**, sa miniboule — la plus petite boule
englobante, qui coïncide avec la boule circonscrite exactement lorsque le
support est bien centré. C'est déjà le prédicat que le dépôt appelle l'autorité
q4, la stricte positivité des quatre barycentriques.

Donc :

- q2 : support `{a,b}`, centre unique, le milieu. L'événement est la boule de
  diamètre `ab`, c'est-à-dire le critère de Gabriel ;
- q3 : support `{a,b,c}`, centre unique, le circumcentre du triangle dans son
  plan ;
- q4 : support `{a,b,c,d}`, centre unique, le circumcentre du tétraèdre.

Le code le confirme : dans `lane_of_target_gq`, le verdict `kLaneQ2` est
exactement `g = D - ||2z-a-b||^2 > 0`, soit « `z` est dans la boule de diamètre
`ab` ». Le domaine des centres q2 est déjà réduit à un point.

Il n'existe donc **aucun continuum de centres dans le problème posé**. Le
certificat de rectangle demande « pour tout centre du disque de Jung — un
continuum bidimensionnel — y a-t-il huit intérieurs ? », alors que le contrat
demande « pour tout circumcentre d'un simplexe formé de points du nuage — un
ensemble fini — y a-t-il huit intérieurs ? ». Le disque de Jung est une
**relaxation continue** d'un ensemble discret, et cette relaxation s'ajoute à
celle de la boîte.

Une réserve, pour ne pas surinterpréter : la cascade reste interdite dans les
deux sens. Une boule contenant `a` et `b` ne contient pas la boule de diamètre
`ab` — avec `a=(-1,0,0)`, `b=(1,0,0)`, la sphère de centre `(0,10,0)` passe par
les deux et exclut `(0,-0.5,0)`. Tester la miniboule de `ab` n'élimine donc
rien en q3/q4, exactement comme la fixture de 64 points l'interdisait déjà dans
l'autre sens.

## 5. La mesure : la fenêtre décidée par paire

J'ai mesuré ce que coûterait la fenêtre si elle était décidée par paire et non
par rectangle. Tirage uniforme exact sur les paires non ordonnées, par rejet sur
un multiple de `n` — pas de multiply-high, dont le biais résiduel a déjà été
relevé. Demi-largeur de Hoeffding avec `delta = 0,01` réparti sur les trois
lanes. Le compteur est `u`, le nombre de témoins universels, qui minore la
profondeur `d` : la fenêtre publiée ici **majore** donc la vraie fenêtre.

| `eight_clusters` | 1 500 | 3 000 | 6 000 | 12 500 | 25 000 | pentes |
|---|---:|---:|---:|---:|---:|---|
| q2 | 40 316 | 88 695 | 186 269 | 415 331 | 848 924 | 1,14 → **1,03** |
| q3 | 123 371 | 278 142 | 622 516 | 1 410 825 | 2 980 089 | 1,17 → **1,08** |
| q4 | 139 126 | 313 740 | 707 522 | 1 612 892 | 3 430 071 | 1,17 → **1,09** |

Sur `uniform`, q4 donne `136 158 / 299 375 / 646 992`, pentes `1,14` et `1,11`.
Les deux familles se comportent donc **de la même façon** : la géométrie n'est
pas ce qui sépare `uniform` de `eight_clusters`.

Le rapport entre les deux fenêtres est le résultat principal :

```text
n = 1 500  : 914 118 / 139 126     = 6,6
n = 25 000 : 185 163 358 / 3 430 071 = 54,0
```

La marge est en outre énorme : à `n=25000`, une paire possède en moyenne
`1028` témoins universels pour un seuil de huit. Seules `1,1 %` des paires sont
sous le seuil.

Autrement dit : la fenêtre exacte est quasi-linéaire dans les deux familles, et
le certificat de rectangle en publie une quadratique. Le facteur perdu n'est pas
une constante à régler — il croît comme `n`.

## 6. Conséquence : les trois fenêtres sont des squelettes de proximité

L'intersection de toutes les boules dont le centre parcourt le domaine de la
lane est un fuseau convexe autour du segment `ab`. Un calcul direct donne sa
demi-largeur dans le plan médiateur : pour q4 le centre vérifie
`||s||^2 <= D/2`, donc `|w| <= ||ab||/(2*sqrt(2))`, et un point du plan
médiateur à distance `rho` du milieu appartient au fuseau lorsque
`rho^2 + 2*rho*|w|_max <= D/4`, c'est-à-dire

```text
rho <= ||ab|| * ( sqrt(3/8) - 1/(2*sqrt(2)) ) = 0,2588 * ||ab||
```

contre `0,5 * ||ab||` pour la boule diamétrale de q2. Les trois fuseaux sont
donc emboîtés, ce que les compteurs confirment : `u_q2 >= u_q3 >= u_q4`.

Chaque fenêtre est ainsi le graphe des paires dont le fuseau contient moins de
`need` points — un **squelette de proximité d'ordre `k`**, dont q2 est
exactement le graphe de Gabriel d'ordre dix. Une paire retenue en q4 porte de
plus une boule contenant au plus onze points avec `a` et `b` sur son bord : la
fenêtre q4 est incluse dans le graphe de Delaunay d'ordre au plus onze. Les
tailles mesurées, de l'ordre de `118n` à `n=6000`, sont cohérentes avec cette
lecture.

## 7. Questions

**Q1.** `sum_E4` doit-elle rester la gate ? Les rectangles ouverts sont à
l'exposant `1,07` quand la masse est à `1,95`. Je propose que la gate officielle
porte sur `F2/F3/F4` physiques, les recertifications et le temps, et que la
masse ne soit publiée que comme diagnostic. `check_rampe_pentes.py` ne sait
aujourd'hui lire que `sum_E`, `masse_ouverte` et `max_E`.

**Q2.** La fenêtre exacte est un squelette de proximité d'ordre `k`, de taille
mesurée quasi-linéaire, inclus dans le graphe de Delaunay d'ordre onze. Le
calculer viole-t-il l'interdit « sans matérialiser la mosaïque de Delaunay
d'ordre supérieur », ou cet interdit vise-t-il seulement le catalogue global de
cellules et de cofaces, proportionnel à `C(n,k)` ? La distinction décide de
toute la suite.

**Q3.** À `98,9 %` de paires largement au-dessus du seuil, le certificat de
rectangle reste-t-il le bon instrument pour produire la fenêtre, ou faut-il un
index de proximité qui la construise directement, le certificat ne servant plus
qu'à élaguer des blocs avant descente comme votre section 5 le demande ?

**Q4.** Sur la cible principale : à l'exposant `1,07`, `50 000` points donnent
de l'ordre de `6,7` millions de rectangles ouverts. Même à cent nanosecondes de
traitement factorisé par rectangle, on dépasse les cent millisecondes. Faut-il
acter que la cible principale suppose un taux de fermeture bien supérieur, et
qualifier d'abord la cible secondaire à une seconde ?

**Q5.** Votre `BJD-BilinearBounds` donne le vrai minimum de `A0` par
séparabilité d'axes. Le même argument s'applique-t-il au certificat central
lui-même, c'est-à-dire avant la descente plutôt qu'après ? C'est la seule
position où un certificat peut retirer des visites.

GCP non utilisé pour cette note. Les mesures sont locales, mono-thread, sur une
machine partagée à deux processeurs logiques ; les temps ne sont pas des
invariants et aucune n'est un reçu de campagne.
