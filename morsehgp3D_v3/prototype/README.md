# Carte du code `prototype/`

Cadre : `phase=exploration_v3_hors_registre`,
`profile=quantized_u16_input_only`,
`public_status=not_claimed`.

Ce fichier existe pour une raison précise : **tout ce qui compile n'est pas sur
la route.** Les sources de ce dossier sont toutes atteignables depuis le build
et couvertes par des CTests ; cela ne dit rien de leur actualité.

Vingt-trois sources portant une idée fermée ont été supprimées le 15 août 2026,
avec leurs cibles et leurs cent quatre-vingt-douze CTests :

- **le parcours de l'arrangement relevé** — son portage GPU (`device_wavefront_*`)
  et les deux profils de volume (`scale_profile`, `flats_scale_probe`) qui l'ont
  précisément fermé ;
- **les cellules de centres** — `centre_cell_source.cpp`, `cell_credits`,
  `cell_prune`, leur juge et leurs deux scripts, supersédés par `CKPairTape` ;
- **le préfixe fini de voisins** comme autorité exhaustive (`prefix_index_gate`,
  `prefix_mass_probe`), interdit par le registre de la racine ;
- **la génération locale certifiée par cône** (`certified_locality_probe`), faux
  vert de saturation ;
- **la source directe par arrangement** (`direct_source.cpp`), dont l'en-tête
  disait déjà « la route passe ailleurs » ;
- les deux probes de groupes coniques et le cœur commun de Jung.

Leur substance est dans [`../audits/PISTES_FERMEES.md`](../audits/PISTES_FERMEES.md) ;
le reste est dans l'historique Git.

L'autorité reste [`../audits/AUDIT_ETAT_COURANT.md`](../audits/AUDIT_ETAT_COURANT.md).
Le mémo des pistes fermées est [`../audits/PISTES_FERMEES.md`](../audits/PISTES_FERMEES.md).

## La route courante, en un paragraphe

`CKPairTape -> Lane2 / Lane3 / Lane4`, **trois producteurs autonomes**. Une
partition Callahan–Kosaraju écrit `binom(X,2)` en rectangles disjoints depuis
l'index Morton ; chaque lane possède ensuite son propre fuseau d'ancre
(`W2 : H>0` ; `W3 : H>0, 3H^2>Xi` ; `W4 : H>0, 2H^2>Xi`), son propre census et
sa propre preuve de complétude. Aucune lane ne lit la sortie d'une autre : le
rang n'est pas héréditaire, et une fixture de 64 points porte un q4 de rang `4`
dont les six arêtes et les quatre faces ont rang `12`.

Contrat et preuves : [`../audits/NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md`](../audits/NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md).

## Les chantiers

| chantier | fichiers pivots | rôle |
| --- | --- | --- |
| **source par ancre** | `anchor_source.cpp`, `anchor_pipeline.hpp`, `anchor_envelope.hpp` | producteur complet, quadratique par construction — **vérité terrain** à petite taille, jamais la route de production |
| **WSPD / WST** | `wspd_front.hpp`, `wspd_wavefront.hpp`, `wst3_probe.cpp`, `rect_front.hpp` | partition des paires, extension ternaire par la lentille, produit quaternaire |
| **noyau q4** | `q4seed_axis_topr4.hpp`, `q4seed_axis_topr4_probe.cpp`, `lane_grid.hpp` | `Q4SeedAxisTopR4` : sélection best-first axiale, le chantier le plus récent |
| **certificats de bloc** | `midball_block.hpp`, `corner8_ball.hpp`, `block_jung_dual.hpp`, `soc64_rect.hpp`, `spindle_cone.hpp` | prédicats `ALL` exacts ou suffisants ; deux sont exacts, aucun ne décide `NONE` |
| **device** | `axis_device_*`, `anchor_source_kernel.cu`, `faceowner_device_*` | mêmes fonctions compilées pour deux cibles ; la parité est une propriété de construction, pas un accord à espérer |
| **dimensionnement** | `lane_source_scale_probe.cpp`, `caps_admissible_probe.cpp` | mesure de l'objet (J0) |
| **dominance 432** | `directional_dominance.hpp`, `directional_dominance_probe.cpp` | préfiltre d'ancre par sous-cône et hauteur — fermeture croissante avec `n` |
| **préfiltre combiné** | `combined_prefilter_probe.cpp` | `h_coeur + h_a + h_b` sur un rectangle CK ; trois comptes prouvés disjoints, décision par histogramme et non par paire |
| **cœur-boule** | `spindle_core_ball.hpp`, `core_ball_probe.cpp` | le cœur d'un rectangle en **forme close** — boule inscrite tangente au fuseau — et les deux primitives sphère–boîte qui rendent le comptage `O(1)` par sous-arbre ; contient aussi la **boule d'apex** de `h_a`, gardée mais non adoptée |
| **oracles et juges** | `../oracle/*`, `q4_brute_oracle.cpp`, `anchored_catalogue.hpp` | arithmétique volontairement différente de la production |

## Ce qui reste, et qui porte encore une idée fermée

Deux fichiers, gardés pour une raison chacun.

| fichier | idée qu'il porte | pourquoi il reste |
| --- | --- | --- |
| `order_k_bfs.hpp` | parcours de l'arrangement relevé par BFS, dont **les trois énoncés fondateurs sont faux hors position simple** | il reste le **sujet** de `../oracle/oracle_main.cpp`, qui en tire `OrderKStatistics` et `order_k_catalogue`. Le supprimer casse le juge |
| `center_cover_mass_probe.cpp` | prune de masse par « huit témoins universels par patch » : pentes `2,104` puis `1,896`, NO-GO avant G4 | le binaire héberge toute la suite `p1a_*`, étrangère à la piste fermée |

Trois autres restent alors que leur piste figure au registre d'abandon de la
racine, parce que le registre autorise explicitement d'en conserver « un oracle
borné » : la tour de boules saturées (`saturated_fold*`, `hybrid_fold_validated`),
`order_k_flats.hpp` — qui alimente le juge `gamma_forest_judge` reconstruisant
`Gamma_k` depuis la définition du manuscrit — et `prefix_index.hpp`, dont
`saturated_fold_hybrid.hpp` a encore besoin. Aucun n'est un chemin de
production ; tous sont bornés et hors du produit.

## Dominance directionnelle — rouverte le 15 août 2026

`directional_dominance` avait été retiré avec les cellules de centres, au motif
qu'il partageait leur cible de compilation. C'était une erreur de classement :
ce n'est pas une pièce des cellules-centres mais un **préfiltre de paires**, et
son prédicat n'a jamais été réfuté — seule la « gate à trois voies » qui le
mettait en concurrence avec deux autres certificats l'a été, pour cause d'ELF,
de cutoffs et d'univers différents.

Le groupe octaédrique d'ordre 48 ramène toute direction à la chambre canonique ;
neuf triangles de rayons entiers `(3,i,j)` la subdivisent, d'où `432` sous-cônes
avec `min cos^2(gamma) = 9/11` entre deux rayons d'un même sous-cône. Si `d=b-a`
et `s=z-a` sont dans le même sous-cône et que la hauteur `tau(d)` dépasse le
seuil, `z` est un **témoin universel** de l'ancre `ab` prise comme arête
maximale. Le prédicat est **exact et fail-open** : il ne ferme jamais à tort,
mais il ne voit pas tous les témoins.

Ce qui le distingue : pour un couple (ancre, sous-cône), les huitième, neuvième
et dixième plus petites hauteurs suffisent — la décision porte sur un
**intervalle de hauteurs**, jamais sur une paire. Et sa fermeture croît avec la
taille du nuage, ce qu'aucun autre certificat du dossier ne fait :

| famille | lane | `n=2 000` | `n=8 000` |
| --- | --- | ---: | ---: |
| `uniform` | q2 | `0,92 %` | `32,7 %` |
| `uniform` | q4 | `3,13 %` | `41,6 %` |
| `eight_clusters` | q2 | `0,09 %` | `20,0 %` |
| `eight_clusters` | q4 | `0,60 %` | `27,1 %` |

Ces chiffres sont un diagnostic `counter-only`, sans reçu épinglé ni pente à
trois points. Ils ne qualifient rien ; ils justifient de rouvrir la piste.

## Préfiltre combiné — le chantier ouvert le 15 août 2026

`combined_prefilter_probe.cpp` mesure la proposition suivante : minorer
`|P inter W_q(a,b)|` par la somme de trois comptes de témoins **prouvés
disjoints**, calculés une fois par rectangle de la partition CK.

- `h_coeur` — témoins universels du rectangle entier, hors `A` et hors `B` ;
- `h_a` — témoins de `A` universels sur `{a} x B` ;
- `h_b` — témoins de `B` universels sur `A x {b}`.

La disjonction est acquise deux fois : par convention, et automatiquement,
puisque pour `z` dans `A` le choix `a = z` donne `H = 0` — un point de `A` n'est
donc jamais certifié témoin du cœur. La somme minore le compte vrai, le filtre
est fail-open, et l'ancre meurt dès qu'elle atteint `h_q`.

Trois propriétés font l'intérêt du montage :

1. **La décision ne touche jamais une paire.** `h_coeur` ne dépend que du
   rectangle, `h_a` que de `a`, `h_b` que de `b` ; le compte des survivantes se
   lit sur un histogramme de `h_b` à onze cases à `s_max=11`. Une fois les `h`
   connus, coût `O(|A|+|B|)` sans matérialiser `A x B`. Leur calcul courant
   reste `O(|A|^2+|B|^2)` à cause des deux auto-jointures.
2. **`H` est exact, `Xi` ne l'est pas.** `H` est bilinéaire par axe, son minimum
   est à un des quatre coins du rectangle plan — c'est exact. Pour `Xi`, la
   convexité en `a` — car `(b-a) x (z-a) = b x z - b x a - a x z` est affine, le
   terme `a x a` étant nul — place le maximum à un **sommet**, ce qui vaut
   beaucoup mieux qu'une enveloppe d'intervalles (fermeture q4 `91,0 %` contre
   `47,6 %`). Mais le contre-audit du 15 août a raison : `xi_max_over_box`
   maximise **séparément** chaque composante du produit vectoriel, et ces trois
   maxima ne sont pas atteints au même point. C'est un majorant sûr, **pas** le
   maximum, et la borne reste donc conservatrice.
   
   `corner64_all_lane` lève exactement cette perte : c'est
   `soc64_rect.hpp::corner512_all_lane` privé de ses huit coins de témoin
   confondus — le témoin étant ici un **point** — et de la recomposition des
   seize coins de `A` et `B` à chaque site. Il décide **exactement** l'enveloppe
   continue des deux boîtes, donc aucune borne tirée des seules AABB ne peut le
   battre. Mesure appariée, `n=4 000`, `s=6`, `K=10` : résiduel q4 `-38 %`
   (`terrain`), `-44 %` (`uniform`), `-46 %` (`eight_clusters`), pour `+17` à
   `+18 %` de temps. Réservé au mode `--coeur=corner64` tant que la substitution
   n'est pas reçue.
3. **Une seule descente pour les trois lanes**, les fuseaux étant emboîtés, et
   une seule évaluation de `(H, Xi)` par point, ni l'un ni l'autre ne dépendant
   de l'arité.

Le probe est `counter-only` : il compte des ancres survivantes, jamais des
supports. Un rectangle non décidé compte toutes ses paires comme survivantes,
donc la mesure **majore** le résiduel. Contrat et preuves : `../PROPOSITION.md`
section 6bis.

## Cœur-boule — le changement de primitive du 15 août 2026

Les trois fuseaux sont des lieux angulaires — `W_q = { z : angle(a,z,b) >
theta_q }` avec `90°`, `120°`, `125,264°` — et la boule centrée au milieu de
`[a,b]` de rayon `kappa_q |ab|` y est inscrite **ouverte** et tangentiellement,
donc
optimale : `kappa_2 = 1/2`, `kappa_3 = sqrt(3)/6`, `kappa_4 = sin(15°)`. Le cœur
d'un rectangle admet le rayon sûr :

`R_q = kappa_q (d - r_A - r_B) - (r_A + r_B)/2`.

Ce rayon décorrèle deux pires cas. Sans boucle supplémentaire, la borne
couplée plus grande est
`kappa_q d-sqrt((4kappa_q^2+1)(r_A^2+r_B^2)/2)` ; dans le régime séparé, elle
est exacte parmi les boules de même centre lorsque `r_A=r_B`. Elle n'est pas
encore implémentée.

La séparation du dossier porte sur l'écart
`d-r_A-r_B >= s max(r_A,r_B)`. Le seuil `s>1/kappa_q`, soit
`2,000 / 3,464 / 3,864`, est une garantie uniforme au pire cas, pas une
équivalence pour chaque rectangle.

### Ce que la journée du 15 août a réellement retenu

Quatre choses adoptées, et autant de réfutations gravées.

| poste | verdict | chiffre |
| --- | --- | --- |
| `corner64` au cœur | **adopté** | résiduel q4 `-38` à `-46 %`, temps `+18 %` |
| autorité à 8 coins pour `h_a` | **adopté** | `ha_somme +5` à `+7 %`, temps `+2 %` |
| fusion des trois lanes | **adopté** | un seul parcours des coins décide q2/q3/q4 |
| dual-tree pour `h_a` | reçu comme **transformation sémantique**, pas comme optimisation | mêmes valeurs, mais `+0,22` à `+30 %` d'évaluations contre une baseline elle aussi fusionnée |
| borne couplée du cœur | **adopté** | `+4` à `+21 %` de témoins, une soustraction |
| boule d'apex pour `h_a` | réfuté par la mesure | `-11` points de fermeture, plus lent |
| cœur-boule `ALL` | dominé par `corner64` | fermeture identique, plus lent |
| boule extérieure `NONE` | dominée par `h_any_upper` | **zéro** coupe nette |
| sphère des points | marge quasi nulle | `rayon serré / rayon AABB = 0,96` |

Et un fait d'échelle, corrigé : **l'arbitrage de séparation est presque
invariant** une fois le cap neutralisé. Sur `terrain`, `s=8` retire `17,503 %`
du résiduel à `n=8 000` et `17,238 %` de la masse **jugée** à `n=32 000` ; le
facteur `6,4` que j'avais publié venait à `99,052 %` de la masse hors
`cap-cellule=512`, pas des certificats.

### `h_a` par boule d'apex : verte à `s=6`, fausse à `s=1`, non adoptée

Le chemin `--ha=boule` inscrit une boule dans le **cône suffisant** d'apex `a`
de demi-ouverture
`gamma_q = theta'_q - arcsin((r_B + 2 r_A)/D)`. Ce cône n'est pas la région
exacte de `h_a` : le remplacement de `|z-a|` par `2r_A` le rétrécit. Les portes
à `s=6` donnent `oracle_faux_morts=0`, mais la preuve doit aussi tester
explicitement `gamma_q>0` avant de mettre son sinus au carré.

La mesure `n=4000,s=6` rend ce chemin plus lent et lui fait perdre jusqu'à
`11` points de fermeture q4 sur `eight_clusters`; il reste donc non adopté.
Elle ne change pas la borne de pire cas : sortir après `h_q` **succès** ne borne
pas le nombre d'échecs, et l'auto-jointure reste `O(|A|^2)` dans le pire cas.
Le compteur `travail_ha` compare en outre des visites de nœuds à des tests de
paires; seul le temps de paroi est homogène. Détail et verdict dans le ré-audit
lié plus bas.

`spindle_core_ball.hpp` implémente un sous-approché rationnel du rayon et les
deux primitives sphère--boîte strictes. `core_ball_probe.cpp` reçoit la sûreté
du certificat et la parité descente/balayage direct sur trois petits nuages.
Ce n'est pas encore son intégration dans `h_coeur+h_a+h_b`.

Le plancher nominal est sûr, mais le commentaire selon lequel `+1` serait
absorbé par la stricte est faux : une distance de grille est une racine carrée
d'entier, pas nécessairement un entier. La solution sans perte finale est un
rayon fixe sous-approché (Q30) comparé par carrés en `i128`, jamais un plafond.

Avec les sphères circonscrites aux AABB utilisées par le probe, le cœur-boule
est un sous-certificat de Corner64 : il peut créditer en bloc, mais un nœud
hors de la boule doit encore passer par Corner512/Corner64. Le test
`h_q`-ième voisin strictement à l'intérieur signifie seulement que le
cœur-boule suffit à tuer uniformément le rectangle ; ce n'est pas une
équivalence avec toute mort d'ancre. La petite boule ne conserve pas non plus
les valeurs de `h_a/h_b` sans fallback. Ré-audit, rayon couplé plus fort,
autorité cône--boule et auto-jointure constructive :
[`../audits/AUDIT_REAUDIT_PREFILTRE_COMBINE_COEUR_BOULE_41DFD2C_20260815.md`](../audits/AUDIT_REAUDIT_PREFILTRE_COMBINE_COEUR_BOULE_41DFD2C_20260815.md).

## Réfutations gravées dans le code

Trois fichiers portent une réfutation qu'il ne faut surtout pas « nettoyer » :

- `anchored_catalogue.hpp` — le certificat « si `2 r_max` des supports déjà
  trouvés n'atteint pas le premier voisin exclu, aucun support n'a pu être
  manqué » est **faux**. Contre-exemple reproduit : 22 points, graine `4242`,
  `69` sphères au lieu de `70`. Il alimente `mhgp3v_false_certificate_regression`.
- `gabriel_degree_gate.cpp` — **aucune borne de degré Gabriel** ne découle de la
  dimension ni de `smax`. Fixture u16 permanente : étoile à **treize** feuilles
  de rang fermé exactement `2`. Tout cap de degré 12 est réfuté.
- `anchor_pipeline.hpp` — le filtre `theta` de l'enveloppe mobile est conservé
  **désarmé**, avec un compteur qui démontre qu'il est identiquement nul sur une
  ancre vivante, doublé d'un plancher de non-vacuité. C'est l'autorité de la
  réfutation, pas un reliquat.
