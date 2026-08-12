# Note Claude — le mur est le census, et le front est inerte sur les amas

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles`,
`profile=quantized_u16_input_only`,
`mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Cette note rapporte trois choses : l'exécution de l'ordre 1 du contre-audit
[`AUDIT_REPONSES_CLAUDE_CHAMBRES_NIVEAUX_CUTTING_20260812.md`](AUDIT_REPONSES_CLAUDE_CHAMBRES_NIVEAUX_CUTTING_20260812.md),
la première mesure de `eight_clusters` avec ledger de masse (ordre 7 partiel),
et une attribution de coût qui déplace la cible. Elle ne reçoit aucune porte,
ne qualifie aucun SLO et ne modifie aucun statut public. Elle demande un
contre-audit.

## 1. Ordre 1 exécuté : `theta` est démontré redondant, puis désarmé

Le compteur demandé existe et il est **causal**, pas décoratif.

`theta_only_prunes_on_live` compte les sites que le filtre `theta` retire **en
plus** de `Uhigh<0`, sur une ancre restée vivante. Il vaut `0` sur les cinq
familles et les deux moteurs. Un second compteur,
`theta_anchors_active`, compte les ancres vivantes sur lesquelles `theta` a
réellement été armé : sans lui, un zéro ne distinguerait pas la redondance
démontrée d'un filtre jamais atteint.

| commande | ancres vivantes armées | prunes en plus |
| --- | --- | --- |
| `uniform n=120 seed=1` | `5 176` | `0` |
| `terrain n=140 seed=5` | `3 461` | `0` |
| `eight_clusters n=80 seed=3` | `2 651` | `0` |
| `uniform n=500 seed=4 --inject=theta-no-fail-open` | `38 034` | `2 557 917` |

La dernière ligne est le point important : le compteur **peut** être non nul,
et il l'est massivement dès que la direction du fail-open est cassée. Le zéro
des trois premières lignes est donc une démonstration, pas une absence.

Trois conséquences sont désormais gravées dans le sujet :

1. le filtre est **hors du chemin par défaut**. Il n'est plus une décision : sa
   sélection des `smax-2` plus grandes bornes inférieures coûtait un
   `nth_element` par ancre côté hôte et un balayage `O(site_count*(smax-2))`
   par ancre côté pipeline device, pour un `kept` bit-à-bit identique.
   `--theta-audit` le réarme ;
2. le compteur non nul hors mutant est un **refus en code 3**, avec le
   diagnostic « le théorème de redondance est falsifié ». Le théorème est donc
   falsifiable à chaque exécution, pas seulement documenté ;
3. un mutant `theta-no-fail-open` **sans** `--theta-audit` est refusé en code 2 :
   casser un filtre désarmé ne prouve rien.

Six portes nouvelles : trois portes de redondance avec plancher
`--min-theta-active` (uniform, terrain, eight_clusters), une porte de causalité
du compteur qui exige `prunes_en_plus_sur_ancre_vivante=[1-9]` en code 4, et
deux refus contractuels. Aucun gain de temps n'est attribué à ce retrait :
la machine de développement a deux cœurs et les campagnes se chevauchent. Le
retrait est justifié par le théorème — sortie identique, travail strictement
inférieur — jamais par un chronomètre.

## 2. `eight_clusters` : le certificat de front est **inerte**, et le
producteur est cubique

C'est la première mesure de cette famille. Elle est plus mauvaise que ce que le
contre-audit anticipait.

| `n` | `candidate_pairs` | `front_witness_prunes` | q4 paires parcourues | `interior_tests` | `wall_s` |
| --- | --- | --- | --- | --- | --- |
| `100` | `4 950` | `0` | `2 446 467` | `3 797 765` | `0,685` |
| `150` | `11 175` | `0` | `8 370 933` | `13 077 276` | `4,291` |
| `200` | `19 900` | `0` | `17 892 952` | `27 515 787` | `6,855` |
| `300` | `44 850` | `0` | `55 220 207` | `90 381 632` | `26,633` |
| `400` | `79 800` | `0` | `101 314 513` | `163 917 080` | `41,554` |
| `500` | `124 750` | `0` | `191 538 784` | `334 430 649` | `92,458` |

Deux faits, tous deux exacts et non interprétatifs :

- `candidate_pairs` vaut **exactement** `C(n,2)` à chaque taille. Le certificat
  de front ne retire pas une seule paire ;
- `front_witness_prunes` vaut **exactement zéro** à chaque taille.

La cause est géométrique et n'a rien d'un défaut d'implémentation. Pour une
ancre `a` d'un amas et un nœud `B` d'un autre amas, la boule témoin commune est
centrée en `z_0=(a+c_B)/2`, donc **dans le vide inter-amas**. Aucun `PointId` ne
peut la peupler, quel que soit son rayon. Le certificat cherche ses témoins
là où il n'y en a pas. C'est exactement le motif que le titre du commit
`9bcd137` nommait, mais la garde de densité ne fait qu'**éviter de payer** cette
recherche : elle ne tue pas la paire.

Les pentes log-log sont `2,59` à `2,71` sur les paires q4, `2,78` sur les
`interior_tests` et `3,05` sur le temps. Le producteur par ancre est donc
**cubique** sur cette famille. Un facteur `100` de taille coûterait de l'ordre
de `10^5` en travail : `50 000` points est hors d'atteinte par plusieurs ordres
de grandeur, sur n'importe quel matériel.

Cette mesure est gravée en porte permanente
`mhgp3v_anchor_eight_clusters_front_inerte` : elle **exige** que le plancher
d'un seul prune soit refusé en code 3. Elle deviendra rouge le jour où une
route fermera réellement ces ancres, et c'est le signal attendu. Deux portes
d'exactitude `mhgp3v_anchor_eight_clusters[_pipeline]` reçoivent par ailleurs
l'accord `--verify` sur cette famille dans les deux moteurs — la famille était
versionnée mais aucune porte ne l'exerçait.

## 3. `uniform` : le degré candidat n'a pas convergé, et le census domine

| `n` | `coord` | `candidate_pairs/n` | q4 paires | `interior_tests` | supports/`n` | h.w. `kept` | h.w. `lens` | `wall_s` |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `500` | `79` | `226,30` | `28 157 247` | `34 310 829` | `277,86` | `394` | `175` | `17,745` |
| `1 000` | `100` | `358,89` | `76 592 176` | `91 950 046` | `308,77` | `515` | `191` | `41,633` |
| `2 000` | `125` | `479,46` | `197 010 077` | `236 221 639` | `337,94` | `748` | `227` | `147,069` |

La densité est constante par construction : `n/coord^3` vaut
`1,014e-3`, `1,000e-3`, `1,024e-3`. Le degré candidat par point devrait donc
être constant. Il croît : `226`, `359`, `479`. Il est déjà `2,06` fois la
baseline pointwise `232,4` que le contre-audit a établie, et il monte encore.
Les temps sont contaminés par la charge et ne sont pas qualifiables ; les
compteurs, eux, sont exacts.

L'attribution de coût est nette. À `n=2 000` :

$$\frac{236\,221\,639}{337{,}94\times2000}=349{,}5\approx\overline{\left\vert\mathrm{kept}\right\vert}.$$

Autrement dit `interior_tests` est, à la fraction près,
`nombre de supports x taille de la liste kept`. Le census **rebalaie la liste
`kept` entière pour chaque support accepté**. Et `kept` croît :
`394`, `515`, `748`, soit une pente `0,46`.

Même en supposant que `kept` sature à sa valeur observée `750` — ce qui n'est
pas démontré — l'extrapolation strictement linéaire du nombre de supports donne
à `50 000` points environ `1,7e7` supports et donc `1,3e10` prédicats de
puissance `i128`. Ce seul poste, sans les paires q4, sans le front, sans le
fold et sans le payload, est déjà hors d'un budget d'une seconde. C'est un
diagnostic d'extrapolation, pas une preuve asymptotique.

**Le mur n'est donc pas la déduplication des supports, ni l'émission. C'est le
census.** Le contre-audit avait raison de placer la reprise sur
`carrier block -> center/rank cover -> microtuile` : la cutting signée
supprime précisément ce balayage, puisque chaque patch hérite ses
`always_inside` par identité et ne recense plus que sa liste de conflits `C_K`.

## 4. Ce que cette note ne dit pas

Elle ne publie aucun `warm_e2e`, ne construit ni `BallActivation`, ni gateways,
ni resolver, ni fold, ni payload. Aucune mesure device n'existe : le noyau
n'a toujours jamais été exécuté. Le point `uniform n=4 000` n'est pas encore
clos. Aucun juge indépendant n'existe encore : l'ordre 2 du contre-audit n'est
pas exécuté, et `--verify` partage toujours les primitives du sujet.

## 5. Questions à l'auditeur

1. **Priorité.** L'attribution ci-dessus déplace la cible du `C(nlens,2)` vers
   le census. Faut-il malgré tout exécuter l'ordre 2 (juge indépendant
   `(S,I_B,U_B)`) avant l'ordre 5 (cutting signée), sachant que le juge
   qualifiera un producteur dont on sait déjà qu'il ne tiendra pas l'échelle ?
   Ma lecture est que le juge doit venir d'abord, parce que la cutting sera
   jugée contre lui ; je demande confirmation.
2. **Front inter-amas.** Existe-t-il un certificat de mort d'ancre qui ne
   cherche **pas** ses témoins dans la boule de milieu ? Pour `a` et `b` dans
   deux amas séparés par du vide, tout témoin universel est nécessairement dans
   ce vide, donc aucun certificat de type « dix témoins » ne peut fermer la
   paire. Il me semble que la seule fermeture possible est par le **budget** :
   `always_inside` dépasse `smax-3` dès que l'amas de `a` est dense. Le
   compteur `anchors_disk_dead` monte effectivement, mais trop tard — après la
   liste de sites. Faut-il porter un test de budget **avant** la liste de
   sites, à partir du seul compte de masse du nœud, et est-il exact ?
3. **`kept` croissant.** La liste `kept` doit-elle saturer dans le régime
   volumique, ou sa croissance en `n^{0,46}` est-elle structurelle ? Elle est
   dimensionnée par le préfixe `|z-a|^2 <= 1,5 D^2` de l'ancre la plus longue
   survivante ; si aucune ancre longue ne meurt, ce préfixe croît avec le
   nuage. Cela me paraît être le même défaut que le front inerte, vu depuis
   l'aval.
4. **Porte de mur.** La porte `mhgp3v_anchor_eight_clusters_front_inerte` grave
   un défaut au lieu d'une propriété. Est-ce la forme que l'auditeur veut, ou
   faut-il un compteur dédié et un plancher nommé plutôt qu'un refus détourné
   du plancher de prunes ?

GCP non utilisé pour cette note.
