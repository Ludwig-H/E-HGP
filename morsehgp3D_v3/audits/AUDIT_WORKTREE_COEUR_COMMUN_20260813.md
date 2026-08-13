# Contre-audit du worktree cœur commun

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

La partition dual-tree des paires et la géométrie conservatrice du cœur sont
une bonne microgate Q7. Le worktree ne reçoit toutefois ni la fermeture ni la
route : `smax` est encore ignoré, deux mutants sont inertes/sound, le compteur
dit « vide » pour « sous-plein q4 », aucun ID n'est authentifié, le juge peut
masquer une mauvaise sélection et le probe rematérialise `n^2` puis rescane tout
le nuage par bloc.

Verdict : **mathématique de cœur admise sous hypothèses ; diagnostic CPU borné ;
NO-GO 50 k/G4**.

## Pin du worktree et rejeu

Le parent est `HEAD=22700778af0d14bd4e25c614bf901ccf427946f2`. Le delta Claude
observé et construit porte :

| objet | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `d1b3cbbe20e7fa198c6132159e79705ac7677a70cac9eedbfd68dfa1fb2cf8bd` |
| `prototype/common_core_probe.cpp` | `db9ef4e2cd2f98c6fcc69f3be2d808203464df711692792d8aeb831ed1a43950` |
| ELF Release | `774d4c1020702ba556fc0c18d1818cffee04f00db3278c4567d17bffdf835eb8` |

L'ELF a `70 992` octets, Build ID
`e792b822c35a6ee3db5df2981b4d91b1d62b5a1d`. Release, CUDA désactivé. Les
sept CTests `mhgp3v_coeur_` rendent `7/7` en `0,67 s`. Aucun mutant n'est armé ;
les sept portes sont trois runs et quatre refus/planchers.

## Ce qui est admis

Pour deux nœuds ayant des boules englobantes certifiées, `S=r_A+r_B` et
`d=||c_B-c_A||`, la condition dirigée `d_lb>3S_ub` construit bien le cœur
ouvert de rayon `(d_lb-3S_ub)/4`. Tout site strictement dans ce cœur est
intérieur à toute circumboule q3/q4 admissible dont l'arête `ab` est maximale.
La condition seule ne ferme rien : il faut encore `h=smax+1-q` IDs uniques.

La recursion `A==B` vers `(L,L),(R,R),(L,R)` est la bonne partition des paires
non ordonnées. L'identité `pair_mass_covered=C(n,2)` est utile. Les rayons AABB
sont majorés et la distance des centres minorée ; le rayon du cœur est arrondi
vers le bas et sa frontière reste exclue. Ces choix sont fail-open.

L'identité scalaire de masse ne suffit pas, seule, à exclure une omission
compensée par un doublon. Chez le juge borné, développer une multiplicité par
`PairId` et exiger exactement `1`; sur le produit, conserver la preuve
structurelle et des records de blocs disjoints.

## P0 — `smax` faux vert partagé

Comme dans les deux probes précédents, `kNeed={10,9,8}` est figé tandis que la
CLI accepte `[4,34]`. Le sujet et le juge emploient tous deux les mêmes
constantes. Un run `--smax=34` peut donc rester `accord=OUI` sans exiger les
31 IDs de q4. Sur `eight_clusters n=400`, le run rend code `0` et zéro
fermeture ; ce cas ne mord pas et ne reçoit rien. Graver la fixture occupée avec
`smax=11/12`, ou refuser `smax!=11` avant calcul.

## Les endpoints n'ont pas à être retranchés

Sous `d>3S`, aucun endpoint des deux blocs ne peut appartenir au cœur strict.
Pour `a` dans A, `||a-m_0||>=d/2-r_A`, valeur strictement supérieure à
`(d-3S)/4`; idem pour B. Les PointId du scan sont déjà distincts. Retrancher
deux à l'occupation est donc une perte de rappel, non une obligation
d'exactitude. Le mutant `coeur-compte-sans-distinction`, qui ne retranche pas,
reste sound. Sur `terrain n=200`, il ferme `436` relations q4, toutes acceptées
par le juge, puis survit en code `3`.

Le no-op `rank=0` ne reçoit aucune identité. La bonne gate vérifie par IDs que
`core intersect (A union B)` est vide, plutôt que d'enlever arbitrairement deux.

## Mutants et compteurs mal qualifiés

- `coeur-separation-deux` change le test de bloc en `d>2S`, mais le rayon
  emploie encore `d-3S`. Dans la bande ajoutée le numérateur est non positif et
  le code continue sans fermeture ni descente ; il peut donc perdre des
  fermetures que des descendants auraient trouvées. Au-delà de `3S`, il est
  identique. C'est une ablation fail-open, pas un mutant de soundness.
- `coeur-bord-interieur` et le rayon arrondi vers le haut demandent des fixtures
  exactes de frontière. Sur `terrain n=200`, ils survivent avec respectivement
  `268` et `436` fermetures q4 toutes acceptées ; une famille générique n'est
  pas une gate.
- `coeurs_vides` est incrémenté dès que l'occupation utilisable est inférieure
  à huit. Il mesure des cœurs **q4 sous-pleins**, pas des cœurs vides. Le claim
  CMake `2 306 cœurs parfaitement vides` n'est donc pas soutenu. Publier
  `occupancy_zero`, `underfull_q2/q3/q4` et leurs masses séparément.

## Juge et reçu incomplets

Le probe ne conserve ni les IDs du cœur, ni une antichaîne de plages/nœuds, ni
leur digest. Le juge recompte tous les témoins universels du nuage pour chaque
paire fermée. Des témoins extérieurs au cœur peuvent ainsi masquer une erreur de
range query ou de crédit. À petit `n`, conserver les IDs triés et rejouer chacun
dans le cœur strict ; à grand `n`, produire une antichaîne de nœuds disjoints,
cardinalités et choix canonique des `h` premiers IDs.

Le q2 utilise silencieusement le cœur q3/q4 plus petit. C'est conservateur, mais
doit être prouvé et jugé comme lane séparée, ou rester hors du claim Q7. Le
profil impose aussi le rejet des positions colocalisées avant LBVH.

## Pas encore une route 50 k

Le probe alloue trois matrices `n*n` en octets, scanne les `n` points pour
chaque bloc séparé, développe ensuite chaque bloc en `ma*mb` paires et, sous
juge, rescane encore `n` témoins par paire. La CLI plafonne `n=4 000`. Aucun cap
de travail/octet, HWM réel, résiduel factorisé ou digest n'est publié.

La microgate demandée reste simple : deux fixtures avec les mêmes blocs et le
même `d_lb>3S_ub`, l'une à cœur vide, l'autre avec huit puis neuf IDs stricts ;
mutants `h-1`, ID dupliqué, frontière et mauvaise borne. Ensuite seulement,
remplacer le scan global par une vraie range query LBVH avec reçu et mesurer
`node_visits`, `boundary_tests`, bytes/HWM et pentes. Une WSPD choisit les blocs
globaux ; le LBVH répond à l'occupation. Aucun des deux ne garantit un cœur
occupé ni une requête sublinéaire.

GCP non utilisé.
