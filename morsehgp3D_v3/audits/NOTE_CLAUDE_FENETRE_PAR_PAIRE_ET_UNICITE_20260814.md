# Note de Claude : la fenêtre décidée par paire, et l'unicité de l'événement

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=mesure_exploratoire`,
`public_status=not_claimed`.

> [!CAUTION]
> **Contre-audit au `HEAD=694920a`.** Le titre et les conclusions « fenêtre
> exacte », « squelette quasi-linéaire » et « inclus dans le graphe de Delaunay
> d'ordre onze » ne sont pas reçus. q2 échantillonne bien la profondeur de la
> boule diamétrale sous le domaine régulier. q3/q4 comptent seulement `U`, les
> témoins **individuellement** intérieurs à tout centre du domaine Jung. Si `D`
> est la profondeur collective minimale et `C` le compte au centre canonique,
> alors `U<=D<=C`, mais `U<D` est possible : huit groupes couvrants ferment une
> paire alors qu'aucun singleton n'est universel. La fenêtre `U<h` est donc un
> **majorant échantillonné** de la fenêtre continue `D<h`, elle-même relaxation
> des seuls circumcentres réalisables. Elle ne prouve ni l'existence d'une
> sphère peu profonde ni l'inclusion Delaunay revendiquée. Les crochets
> Hoeffding supposent en outre un modèle aléatoire non reçu pour le préfixe
> SplitMix à seed fixe. Verdict et corrections :
> [`AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md`](AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md)
> et [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

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

Le sujet réparé forme **zéro** groupe sur cette fixture : cinquante-cinq visites
de feuilles témoins sont rejetées comme déjà créditées, donc la banque est
vide. Ce ne sont ni cinquante-cinq IDs ni cinquante-cinq bases ; le nuage ne
possède que sept témoins. C'est le comportement exact attendu. Sous
`--inject-bjd-reutilise`, un terminal ferme et le juge de fermeture le réfute :
code `4`. Deux portes déterministes,
`mhgp3v_bjd_fixture_collineaire` et son mutant.

Les planchers de non-vacuité sont devenus **explicites** (`--min-bjd-groupes`,
`--min-bjd-fermetures`) précisément parce que la fixture exige de ne rien
fermer : un plancher câblé dans le sujet aurait rendu cette absence
indistinguable d'une panne.

Restent ouverts, et je ne les conteste pas : le typage `INVALID_OR_UNKNOWN` de
`bjd_lane_box`, le cap de somme de poids dans `dual_lane`, le reçu d'identités
sur `spid` plutôt que sur des rangs, la séparation complète des deux pools, et
le remplacement du glouton par `tau(F)`.

## 2. Le crédit de groupe : diagnostics locaux

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

À `n=1500`, BJD retire `14,7 %` face au central seul mais seulement `0,87 %`
supplémentaire après SOC64. Les pentes observées remontent vers `1,95` quand
`n` croît ; elles ne prouvent ni une constante de dix-neuf pour cent, ni un
exposant asymptotique. Le placement post-descente reste néanmoins une voie
close pour économiser les visites, et ne justifie pas une rampe G4.

## 3. La faute de métrique : `sum_E4` n'est pas un coût

`sum_E4` est la **masse logique** des paires ouvertes. Ce n'est le coût que si
les paires sont développées — ce que toute l'architecture interdit. J'ai donc
mesuré séparément le nombre d'**enregistrements physiques** ouverts, sur la même
configuration et les mêmes graines :

| `eight_clusters` q4 | 1 500 | 3 000 | 6 000 | 12 500 | 25 000 | pentes |
|---|---:|---:|---:|---:|---:|---|
| masse ouverte | 914 118 | 3 287 809 | 11 910 722 | 48 066 852 | 185 163 358 | 1,85 → **1,95** |
| rectangles ouverts | 100 316 | 235 929 | 531 588 | 1 211 785 | 2 543 020 | 1,23 → **1,07** |

Sur cette rampe, les deux quantités divergent : la dernière pente locale vaut
`1,95` pour la masse et `1,07` pour les records, tandis que la pente
bout-à-bout des records reste voisine de `1,15`. Une WSPD peut représenter une
masse quadratique par un nombre quasi linéaire de rectangles, mais cinq tailles
ne prouvent ici aucune convergence asymptotique du sous-ensemble ouvert ni de
son consommateur. Les rectangles deviennent plus gros sur la rampe : masse par
rectangle `9,1` puis `73`.

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

Il n'existe donc **aucun continuum dans la source des supports complets**. Le
prune d'une ancre partielle quantifie en revanche légitimement sur le continuum
du disque de Jung : il demande si toute complétion possible est déjà profonde,
alors que la source finale ne visite que les circumcentres des vrais simplexes
du nuage. Le disque de Jung est une **relaxation continue** de cet ensemble
fini, et cette relaxation s'ajoute à celle de la boîte.

Une réserve, pour ne pas surinterpréter : la cascade reste interdite dans les
deux sens. Une boule contenant `a` et `b` ne contient pas la boule de diamètre
`ab` — avec `a=(-1,0,0)`, `b=(1,0,0)`, la sphère de centre `(0,10,0)` passe par
les deux et exclut `(0,-0.5,0)`. Tester la miniboule de `ab` ne fournit donc
aucune règle générale d'élimination q3/q4. Les IDs du segment ouvert restent
une exception exacte : ils appartiennent au cœur affine de toutes les sphères
incidentes. La fixture de 64 points interdit déjà la cascade dans l'autre sens.

## 5. La mesure : le majorant d'ancres décidé par paire

J'ai mesuré ce que coûterait la fenêtre si elle était décidée par paire et non
par rectangle. Le rejet sur un multiple de `n` est sans biais **si** ses mots
u64 sont i.i.d. uniformes — pas de multiply-high, dont le biais résiduel a déjà
été relevé. Le préfixe SplitMix à seed fixe ne reçoit toutefois ni ce modèle
aléatoire ni l'indépendance requise par Hoeffding. Le compteur est `u`, le
nombre de témoins universels, qui minore la profondeur `d` : la fenêtre publiée
ici **majore** donc la vraie fenêtre.

| `eight_clusters` | 1 500 | 3 000 | 6 000 | 12 500 | 25 000 | pentes |
|---|---:|---:|---:|---:|---:|---|
| q2 | 40 316 | 88 695 | 186 269 | 415 331 | 848 924 | 1,14 → **1,03** |
| q3 | 123 371 | 278 142 | 622 516 | 1 410 825 | 2 980 089 | 1,17 → **1,08** |
| q4 | 139 126 | 313 740 | 707 522 | 1 612 892 | 3 430 071 | 1,17 → **1,09** |

Sur `uniform`, q4 donne `136 158 / 299 375 / 646 992`, pentes `1,14` et `1,11`.
Sur ces tailles, seeds et ce diagnostic seulement, les deux familles montrent
des pentes proches. Cela ne reçoit pas l'affirmation générale que la géométrie
ne sépare jamais `uniform` de `eight_clusters`.

Le rapport entre les deux fenêtres est le résultat principal :

```text
n = 1 500  : 914 118 / 139 126     = 6,6
n = 25 000 : 185 163 358 / 3 430 071 = 54,0
```

La marge est en outre énorme : à `n=25000`, une paire possède en moyenne
`1028` témoins universels pour un seuil de huit. Seules `1,1 %` des paires sont
sous le seuil.

Autrement dit : le **majorant par cœur universel** est quasi linéaire sur ces
tailles dans les deux familles, tandis que le certificat de rectangle publie
une masse presque quadratique. Ce signal justifie un générateur direct à
falsifier ; il ne prouve pas encore la taille de la fenêtre événementielle
exacte.

## 6. Conséquence : trois sur-squelettes de proximité

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

Chaque fenêtre échantillonnée est le graphe des paires dont le fuseau contient
moins de `need` points universels. q2 est exactement le seuil
`I(B_ab)<10` sur les paires tirées, soit l'ordre neuf avec la convention
« au plus k intérieurs », hors extra-shell. Pour q3/q4, ce graphe est un
**sur-squelette de proximité** : tout événement retenu possède une ancre dans
ce graphe, mais une ancre du graphe peut avoir une profondeur collective élevée
et ne porter aucun événement. Il n'est donc pas prouvé inclus dans un graphe
de Delaunay d'ordre onze. Les tailles de l'ordre de `118n` à `n=6000` restent
un signal empirique utile, pas une borne du squelette exact.

## 7. Questions

**Q1.** `sum_E4` doit-elle rester la gate ? Les rectangles ouverts sont à
l'exposant `1,07` quand la masse est à `1,95`. Je propose que la gate officielle
porte sur `F2/F3/F4` physiques, les recertifications et le temps, et que la
masse ne soit publiée que comme diagnostic. `check_rampe_pentes.py` ne sait
aujourd'hui lire que `sum_E`, `masse_ouverte` et `max_E`.

**Q2.** Un tape global de ce sur-ensemble d'ancres, suivi d'une source
edge-local des événements shallow, viole-t-il l'interdit « sans matérialiser la
mosaïque de Delaunay d'ordre supérieur », ou cet interdit vise-t-il le
catalogue global de cellules, cofaces et incidences ? La distinction décide de
toute la suite ; l'inclusion Delaunay du sampler n'est pas supposée.

**Q3.** À `98,9 %` de paires largement au-dessus du seuil, le certificat de
rectangle reste-t-il le bon instrument pour produire la fenêtre, ou faut-il un
index de proximité qui la construise directement, le certificat ne servant plus
qu'à élaguer des blocs avant descente comme votre section 5 le demande ?

**Q4.** Sur la cible principale : prolonger la dernière pente `1,07` de
`2,543` millions à `n=25000` donne environ `5,3` millions de rectangles ouverts
à `50000`. Même à cent nanosecondes de
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
