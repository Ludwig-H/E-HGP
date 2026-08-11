# Audit courant de MorseHGP3D v3

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Fraîcheur

`HEAD` audité : `8c21b7a056c947af2c0aa4c1fd9d5848447a1ba4`. Le
worktree audité contient un delta non committé : les sources ci-dessous et la
documentation v3 ont été lues telles quelles, sans modifier le code de
prototype. `prototype/cloud_family_gate.cpp` et
`prototype/exact_ray_sweep.hpp` sont encore non suivis par Git.

| objet | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `4e102d8cb64555ddbae997a884fbe5285b99fce5af4bfeefaed17791d74e67c2` |
| `prototype/cloud_families.hpp` | `1a3e3027c2e0880e6ff381fc80b707b9ec88dbf573579aac535cfc80bb307b54` |
| `prototype/cloud_family_gate.cpp` | `183be43bb65804ce311ddad3a5d2235dfdfa068cdd79e7942b3bc699ae051a36` |
| `prototype/pair_selfjoin_probe.cpp` | `510c8306c7c99aa65b01506f7d2d3eac7317ff4e6f7de2f94f3ad60b19e583ac` |
| `prototype/pair_anchor_probe.cpp` | `8ae41d20dffcc3cbb1fc6d85d088fd4e74e9303681afba02727af6968e9e2733` |
| `prototype/exact_ray_sweep.hpp` | `ed07335f49993e883ced53d9d9489d674249e6d036478966281afaa7df209443` |
| `prototype/admissible_pair_probe.cpp` | `f58f7e46801cebb58498a2de7c5746a2fdf171d2c354c8cc7f8eef4ea4c460a3` |
| `oracle/gamma_forest_judge.cpp` | `6a500b219e8cd946e1edecd6c80c854ba4a10882936d1f47a2792a9fd2bc3520` |
| `prototype/cell_source_mass_probe.cpp` | `7442af487265033849c97d0ade7c9810fd6b7e5acf506580779d76b4d0139bde` |
| `prototype/direct_source.cpp` | `7acb7a9c01d0ca4a18491203f0558b07c0b828354bddd187d7a297f1143302dc` |
| `prototype/faceowner_device_qualification.cpp` | `20b2eb2e8e01d839ba413ca5b52b054f4cdf0decd7a99a484d169e3d70bcccc3` |
| `prototype/parallel_catalogue_gate.cpp` | `da34a6f3a8736eae9116935f3e66c5ac2067463373c45d9db4b910d25f4ecafb` |
| `prototype/postings_join_gate.cpp` | `f9b0bc4485315c1e39331c902c6acc5746ac95700696aefa12269a419dc68c3c` |
| `prototype/prefix_mass_probe.cpp` | `c5381b1d3569f8d9391021fa0b4f85c58a18dfb537687ff92837404fa96ef0b9` |
| `prototype/saturated_fold.hpp` | `1fc70e9644a7394e7a14758b61916723ddeaf87455f230694515fe018cdbab58` |
| `prototype/saturated_pipeline.cpp` | `dd54c231c4ecfef182136f1564f14cc924dc935d8d2cd803dccd28f3dfc6d8a6` |
| `audits/check_gate_d_fold_f0.py` | `192f272ef2f88fa8925522736d05cebf480d99f9f2076bef3f81b73ee89d40fb` |
| binaire Release q2 | `719b1ce1e628814807f72110de2ab3bae44da9f93f613329415db0b14f03c9b7` |
| binaire Release ancres | `bdc50ea335bc03419d988c2453af800ad31554cbde175ba16fa23279e0c5181e` |
| binaire ASan/UBSan ancres | `fc672abc3ad1054e8b083ba8983c9c0aa69eb66c3bbf3df05f6c0c4001f8988a` |
| binaire Release cardinalité | `a9dc601794fb9a669818df2d1faf9c7c6443536667248d86ee24740a9591cee0` |

Toute modification d'un de ces octets rend les résultats locaux ci-dessous
historiques et exige un nouveau pincement. Cet audit est l'unique autorité
mutable du statut v3; les autres notes sont des preuves durables ou des reçus
explicitement datés.

## Verdict

Le contrat n'est pas rempli. À 50 000 points et `K=10`, le plan de tests fixe
un p95 `warm_e2e<100 ms` principal et `warm_e2e<1 s` secondaire. Les deux
seuils portent sur `BenchmarkOutputContract-v1`, qui comprend dix forêts, les
applications verticales, les lots et le certificat minimal copiés côté hôte.
Le payload horizontal `hgp_reduced_normalized_h0_v3` est une série distincte :
même mesuré sous une seconde, il ne fermerait pas ce SLO officiel.

Aucun exécutable v3 ne mesure aujourd'hui ce pipeline complet. Le seul
pipeline assemblé construit un catalogue CPU exhaustif; son chrono catalogue
et fold s'arrête avant la construction des forêts. Les composants CUDA
qualifient des briques isolées et effectuent encore allocations, transferts ou
synchronisations par appel ou par ordre. Il n'existe ni LBVH Morton/Yao48
produit, ni source q3/q4 complète et parcimonieuse, ni payload officiel
résident de bout en bout.

La directive normative de la spécification §1.1, datée du 7 août, impose un
chemin industriel sans budget configurable : objet complet ou échec sur une
ressource physique réelle. Le vocabulaire plus ancien de budgets configurés et
`budget_exhausted` dans le plan de tests §14.6 ne peut être lu que comme contrat
de probes diagnostiques tant que cette contradiction externe n'est pas
réparée; il ne peut ni tronquer ni censurer une sortie produit.

Le verrou est algorithmique avant d'être CUDA. Le cœur exact q3/q4 courant
laisse un résiduel bien plus mince que son travail, mais il repart encore de la
racine pour chaque recherche de témoins. Sur les tailles diagnostiques
800/1 600/2 400, les huit couples lane/famille ont deux pentes successives de
visites supérieures à 1,35. Ce n'est pas la porte formelle, qui exige
12 500/25 000/50 000, mais c'est une alerte structurelle suffisante pour
interdire le port de ce rescan. Même l'extrapolation la plus favorable reste à
plusieurs milliards de visites à 50 k.

## État des tests du worktree pincé

La configuration Release CPU enregistre 321 CTests, dont 75 tests préfixés
`mhgp3v_pair_anchor_`, 4 `mhgp3v_cloud_family_` et les 2 doubles q2 scanline.
La reconstruction Release complète passe, y compris la cible ASan/UBSan des
ancres. Le dernier rejeu ciblé suivant passe `81/81` en 10,81 s :

```bash
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --target mhgp3v_pair_anchor_probe \
  mhgp3v_pair_anchor_probe_san mhgp3v_cloud_family_gate \
  mhgp3v_pair_selfjoin_probe --parallel 4
ctest --test-dir build/v3 --output-on-failure \
  -R '^mhgp3v_(pair_anchor_|cloud_family_|pair_selfjoin_q2_scanline)'
```

Une matrice supplémentaire passe `320/320` : quatre familles, graines 1 à 20,
lanes q3/q4, modes `depth` et `combined`, `n=24`, feuilles 2 et oracle actif.
La porte Python modifiée `mhgp3v_gate_d_fold_f0` passe aussi `2/2` en 4,15 s,
avec et sans `python3 -O`.
Ces verts qualifient seulement les propriétés locales exercées sur les octets
pincés. Ils ne qualifient ni la complétude d'une source 50 k, ni la complexité,
ni le pipeline, ni le statut public. Aucun passage global `321/321` n'est
revendiqué.

## Résultats mathématiques et logiciels locaux

### Cardinalité du générateur

Le contre-exemple permanent
`scanline_overlap_multiecho, n=12500, coord=707, seed=20260810` révélait une
émission de 12 501 points. La porte directe du générateur exerce désormais les
quatre familles, plusieurs tailles et graines, ce contre-exemple, un mutant
d'overshoot et une insuffisance explicite. Sur le worktree pincé, le contrat
« exactement `n`, ou refus avant construction » est fermé localement. Le
fichier de porte reste non suivi et doit accompagner le code au commit.

### Self-join q2

Le prédicat ponctuel reste exact : `w` est strictement intérieur à la boule
diamétrale de `x,y` si et seulement si
`(w-x) dot (w-y)<0`. Les bornes AABB `U4` et `L4`, les témoins distincts et la
partition triangulaire fournissent des prunes sûrs et un ledger pair-à-pair.
Cette preuve ne rend pas le parcours industriel.

Les reçus 50 k déjà pincés suffisent à refuser le self-join courant comme
source produit : selon la famille, 53 à 724 millions de visites `L4`, 86
millions à 1,365 milliard de tests ponctuels et 3,60 à 14,85 millions de paires
terminales, pour 6,672 à 120,303 s en count-only CPU avant census q3/q4 et
fold. La route à construire reste Yao48 fail-open sur un vrai LBVH Morton,
suivi d'un classifieur terminal et d'un census fermé multi-ordre. Le self-join
reste oracle, falsificateur de masse ou second prune.

### Degré Gabriel

Le kissing number 12 ne borne pas le degré q2. Dans l'espace euclidien, pour
`p=0` et autant de points distincts `q_i=R u_i` que souhaité sur une sphère
centrée en `p`, tout tiers vérifie l'inégalité stricte suivante :

$$\Phi_{p,q_i}(q_j)=R^{2}\left(1-\cos\theta_{ij}\right)>0.$$

Le degré reste arbitraire dans le bucket exact de rang fermé 11 en ajoutant
neuf témoins communs strictement intérieurs. Sur la grille u16 finie, les caps
triviaux sont `n-1` et `2^48-1`; deux constructions à treize voisins suffisent
à réfuter le cap 12 aux rangs exacts 2 et 11. `smax=11` borne seulement le
contenu d'un record fermé, pas le nombre de records incidents. La preuve est
dans
[`AUDIT_DEGRE_GABRIEL_KISSING_SMAX11_20260811.md`](AUDIT_DEGRE_GABRIEL_KISSING_SMAX11_20260811.md).
Le calcul indépendant de leurs produits scalaires confirme les rangs fermés 2
et 11 annoncés, mais aucune gate exécutable ne grave encore ces constructions.
La baseline de Poisson de degré moyen 80 reste une moyenne de modèle, jamais un
cap, une queue ou une garantie de temps.

### Cœur de Jung q3/q4

Pour une paire distincte certifiée arête maximale d'un support propre positif,
`g>0` puis `3g^2>4Q` en q3 ou `g^2>2Q` en q4 certifient un témoin strict pour
tout le disque de Jung. Neuf ou huit `PointId` distincts autorisent seulement
la tombstone H0 correspondante. Les égalités restent fail-open, sauf
l'inégalité large q4 explicitement prouvée sûre sous la garde `D^2>0`.

Le worktree ajoute `L4` et hérite au plus neuf identifiants déjà universels.
Il ne maintient pas une frontière persistante de nœuds ambigus : la correction
des frères d'extrémités libérés est obtenue par un nouveau parcours depuis la
racine. Les campagnes différentielles, mutants, planchers de non-vacuité,
contacts, paire colocalisée et égalité rationnelle passent dans la série
ciblée. Les certificats transportent encore des positions dans la permutation
de l'arbre; tout reçu exporté doit publier des `PointId` stables ou engager
cette permutation. Le rejeu partage certaines primitives avec le sujet et ne
remplace donc pas un juge mathématique entièrement indépendant.

Un coût caché reste dans le chemin accepté en bloc. Après avoir crédité un
nœud en `O(1)` par sa cardinalité, `count_universal_block` et
`count_universal_pair` réénumèrent toutes ses positions dans `harvest_`, même
hors génération de certificats. Ce travail `O(|node|)` n'apparaît ni dans
`node_visits` ni dans `point_tests`; l'héritage et le reçu n'ont besoin que des
9/8 identifiants distincts du seuil. Il faut plafonner cette matérialisation au
seuil effectif, publier un compteur propre et ne jamais remplir le vecteur de
rejeu lorsqu'aucun certificat n'est demandé. Cette correction réduit une
constante et répare la télémétrie; elle ne change pas le verdict d'exposant.

### Profondeur fermée

Le noyau partagé implémente exactement
`delta=always+m-max_open`. Il trie les rayons par angle entier, inclut les
rayons confondus dans l'arc semi-ouvert et exclut l'antipode. Le juge borné
collecte directement depuis les points, choisit une autre base et minimise la
demi-boule fermée en temps quadratique. La fixture u16 extrême non colinéaire,
les grands produits, ASan/UBSan, les comparaisons de tri et tous les octets de
scratch sont maintenant exercés ou comptés.

Le contrat de l'API partagée doit toutefois être explicite. Elle reçoit des
rayons sans leurs `PointId` et ne peut donc empêcher qu'un même identifiant soit
injecté deux fois; ses produits `i128` ne sont sûrs que sous les bornes des
adaptateurs u16 reçus, pas pour des `int64` arbitraires. Chaque adaptateur doit
garantir un rayon par `PointId` distinct, accepter des directions confondues
portées par des identifiants différents et engager sa borne d'amplitude.

Ce durcissement ne corrige pas le coût dominant : chaque paire survivante
collecte encore ses témoins depuis la racine et le noyau alloue `rays`, `dir`
et `count` par appel. Le pire cas reste `O(sum m_ab log m_ab)`, donc
`O(n^3 log n)`.

Une composition exacte encore absente peut améliorer le résiduel sans changer
ce verdict. Si `C` contient `c` témoins Jung universels distincts et si la
profondeur est calculée sur les témoins diamétraux `P` privés de `C`, toute
sphère admissible contient au moins `c+delta(P minus C)` points stricts. La
porte doit retirer réellement `C`, dédupliquer les `PointId` et tuer les
mutants de double comptage. Ce prune peut gagner des cas où ni le cœur ni la
profondeur seuls n'atteignent 9/8; il ne supprime pas la collecte par paire.

### Directions exactes sans rescan par paire

Deux certificats méritent un prototype séparé du self-join actuel :

- pour une ancre `p`, un témoin `w` et un nœud AABB de cibles `q`, poser
  `s=w-p`, `d=q-p` et `A=d dot s-||s||^2`. Le minimum de `A` est linéaire et le
  maximum de `||d cross s||^2`, fonction convexe, est atteint à l'un des huit
  coins. Les tests `A_min>0` puis `3*A_min^2>cross_max` en q3 ou
  `2*A_min^2>cross_max` en q4 certifient donc le témoin pour tout le nœud. Une
  banque Jung--Yao de 9/8 identifiants peut remplacer des rescans par une preuve
  de range; une boîte indécise descend;
- pour une paire exacte, chaque point définit le demi-plan fermé des centres du
  disque de Jung où sa marge est négative ou nulle. Si l'intersection du disque
  et des mauvais demi-plans d'un groupe est vide, le groupe garantit un témoin
  intérieur. Helly réduit chaque groupe à trois `PointId` au plus; neuf ou huit
  groupes disjoints ferment la lane. Le packing greedy est sûr mais incomplet
  et son échec conserve la paire.

La preuve, le solveur rationnel et la borne de 180 bits sont dans
[`NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md`](NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md).
Ces certificats sont ponctuels ou par nœud cible; ils ne remplacent pas la
fermeture globale de l'univers implicite. Le center-cover 64 patches de
`P15-HOCUDA-P1` reste le candidat de complétude par blocs pour q3/q4. Sa première
tranche `P15-HOCUDA-P1a` est plus étroite : elle profile seulement le prune q4,
n'émet aucune ancre et ne peut établir la complétude de P1. Les approches
doivent être comparées par compteurs, sans partager leurs sorts.

## Mesure de complexité du cœur pincé

Les compteurs ci-dessous proviennent du binaire Release pincé, en mode `core`,
feuilles 8, graine 20260810 et oracle désactivé. Les deux exposants sont
`log(V2/V1)/log(n2/n1)`. Les chronos sous charge ne sont pas rapportés; les
masses et ledgers sont déterministes.

| famille | lane | exp. 800→1 600 | exp. 1 600→2 400 | visites projetées à 50 k |
| --- | --- | ---: | ---: | ---: |
| uniform | q3 | 2,186 | 2,294 | `1,73e11 [1,54e11;2,13e11]` |
| uniform | q4 | 2,278 | 2,412 | `2,81e11 [2,42e11;3,63e11]` |
| terrain | q3 | 1,567 | 1,839 | `5,19e9 [3,83e9;8,73e9]` |
| terrain | q4 | 1,626 | 1,915 | `7,89e9 [5,71e9;1,37e10]` |
| scanline simple | q3 | 1,556 | 1,527 | `2,65e9 [2,51e9;2,74e9]` |
| scanline simple | q4 | 1,647 | 1,538 | `3,87e9 [3,14e9;4,37e9]` |
| multiecho | q3 | 1,931 | 2,255 | `2,29e10 [1,59e10;4,26e10]` |
| multiecho | q4 | 1,979 | 2,292 | `3,10e10 [2,18e10;5,64e10]` |

La projection part des visites à 2 400 et emploie la pente sécante 800→2 400;
la fourchette emploie les deux pentes locales. Ce n'est ni une preuve, ni un
intervalle de confiance, ni la gate formelle aux tailles contractuelles. Elle
suffit à montrer que même le scénario le plus favorable conserve des milliards
de visites par lane. Le problème est la répétition du parcours de témoins, pas
seulement le nombre d'ancres finalement conservées.

## Gain marginal de la profondeur

Sur `n=800`, même graine et mêmes feuilles, le mode `combined` produit :

| famille | lane | paires soumises au sweep | prunes de profondeur | taux marginal | visites de collecte | comparaisons de tri |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| uniform | q3 | 59 011 | 6 583 | 11,16 % | 2 661 219 | 3 875 650 |
| uniform | q4 | 64 250 | 12 971 | 20,19 % | 3 026 026 | 4 972 539 |
| terrain | q3 | 25 570 | 28 | 0,11 % | 676 454 | 831 942 |
| terrain | q4 | 26 370 | 130 | 0,49 % | 711 278 | 1 029 239 |
| scanline simple | q3 | 25 410 | 243 | 0,96 % | 680 324 | 844 012 |
| scanline simple | q4 | 25 992 | 683 | 2,63 % | 711 724 | 1 005 333 |
| multiecho | q3 | 29 201 | 142 | 0,49 % | 843 371 | 1 172 360 |
| multiecho | q4 | 30 106 | 407 | 1,35 % | 886 256 | 1 396 701 |

La profondeur est mathématiquement utile sur `uniform`, mais son rendement est
faible sur les trois familles structurées. Elle doit rester un filtre terminal
adaptatif ou un repli exact, avec une porte de gain marginal; la porter avant
la source par blocs ferait payer le tri sur une mauvaise architecture.

## Reçu G4 existant

L'unique reçu G4/50 k reste une session CPU mass-only sur une machine G4; le
GPU n'a pas été utilisé. Après le prune de cellules, les masses vont de 465
millions à 2,86 milliards en q2, de 14,7 à 132 milliards en q3 et de
`3,30e11` à `9,97e12` en q4. Le catalogue exhaustif atteint 675,4 s à seulement
6 250 points. Ces mesures réfutent l'énumération combinadique; elles ne
mesurent ni la nouvelle source, ni un kernel, ni un payload.

Le détail et la certification `TERMINATED` sont dans
[`NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md`](NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md).
Aucune nouvelle session G4 n'est justifiée aujourd'hui : le code P1a et son
différentiel rationnel `n<=32` manquent. Une fois ces portes locales, Release et
sanitizer fermées, le protocole P1a autorise directement un unique profil G4
q4 à 50 k, sans palier intermédiaire; ce profil mass-only ne qualifie aucun SLO.
Toute campagne de pipeline exige en plus un harness `warm_e2e` au payload nommé.

## Commentaires de code encore périmés

Le nettoyage des anciennes notes est effectif, mais plusieurs commentaires du
code pincé ne désignent toujours pas une autorité vivante :

- `CMakeLists.txt:4` annonce « uniquement M1 » alors que le projet construit de
  nombreux prototypes;
- `CMakeLists.txt:90-95` et `prototype/scale_profile.cpp:1-7` présentent le
  nombre de sommets d'arrangement comme l'unique mesure qui décide 100 ms. Ce
  probe n'émet pas `BenchmarkOutputContract-v1`, ne mesure pas `warm_e2e` et
  emploie l'ancien `flat_catalogue`;
- `prototype/cloud_families.hpp:3` annonce deux familles alors que l'enum et la
  porte en exercent quatre;
- `prototype/pair_anchor_probe.cpp:155` cite `REPONSE_AUDIT_ANCRES`, et les
  commentaires des cellules citent « réponse pont/q4 §6 » : ces autorités
  n'existent pas dans l'arbre;
- `prototype/order_k_flats.hpp:62,461,633,929` cite des numéros de sections qui
  n'existent plus dans les audits condensés associés.

L'auditeur ne modifie pas le code de Claude. Ces commentaires doivent être
remplacés côté code par un invariant intemporel ou un lien exact vers une
autorité conservée; recréer les anciennes notes serait une régression.

## Ordre d'implémentation recommandé

1. Conserver les self-joins et la profondeur comme oracles/falsificateurs. Dans
   le probe, plafonner `harvest_` au seuil, compter `root_restarts` et rendre le
   contrat `PointId`/amplitude du sweep explicite. Ce sont des corrections de
   télémétrie et de frontière, pas une promotion du rescan.
2. Construire une unique disposition `(MortonKey, PointId)` et un LBVH exact
   résidents, partagés par toutes les lanes. Aucun `cudaMalloc`, upload, D2H ou
   `synchronize` n'est admis par paire, tuile, vague ou ordre.
3. Pour q2, implémenter Yao48 en banques `48x10`, strict et fail-open, puis le
   classifieur terminal et le census fermé en une passe multi-ordre
   `count--scan--fill` avec offsets 64 bits. Fermer le ledger sur
   `candidate+certified_pruned+unresolved=C(n,2)`. Le contrat complet est dans
   [`NOTE_SOLUTION_SOURCE_Q2_YAO48_LBVH_U16_20260811.md`](NOTE_SOLUTION_SOURCE_Q2_YAO48_LBVH_U16_20260811.md).
4. Pour q4, implémenter d'abord le falsificateur de masse local
   `P15-HOCUDA-P1a` déjà spécifié : partition canonique
   `T(N)=T(L) dot-union (L cross R) dot-union T(R)`, couverture du domaine de
   centres de Jung par 64 patches, range-query témoin collective et prune d'un
   bloc seulement lorsque chaque patch faisable possède huit `PointId`
   stricts certifiés. Sinon partager le bloc; une microtuile terminale compte
   sans arène globale de paires. L'identité minimale est
   `pruned_mass+microtile_mass=C(n,2)`.
5. Après le différentiel `n<=32`, exécuter le protocole P1a gardé directement à
   50 k et mesurer `Q`, visites patch--nœud, masse microtuile, queue et équilibre
   CTA. Une majorité de masse aux feuilles, un rescan par paire ou la tranche
   source--cover hors de son enveloppe classe P1 no-go; le P1 complet rejettera
   en plus source--cover plus cordes au-dessus de 400 ms. Les portes
   12 500/25 000/50 000 restent requises pour les autres routes de source.
6. Sur les ancres admises seulement, comparer banque Jung--Yao, groupes de
   Helly, composition cœur--profondeur et profondeur seule. Chaque gain est
   rapporté net de collecte, tri et allocation; tout greedy reste incomplet et
   retombe fail-open. Construire ensuite range-report q3 et niveaux shallow q4.
7. Construire `BallActivation`, census, resolver, fold et reconstruction des
   verticales, un composant à la fois contre Gamma exhaustif borné. Installer
   le mélange équilibré de huit amas du benchmark officiel et les deux harnesses
   nommés, horizontal diagnostique et `BenchmarkOutputContract-v1`.
8. Appliquer la gate 12 500/25 000/50 000, puis porter seulement les routes
   admises avec arènes device et mémoire hôte épinglée préallouées. Mesurer 30
   répétitions du pipeline officiel complet; une insuffisance physique refuse
   atomiquement et aucun cap ne tronque une frontière ou une sortie.

Aucun tableau global de paires, tuples, cellules, faces, cofaces ou incidences
n'entre dans le chemin produit.

GCP non utilisé.
