# Contre-audit du cœur commun — pin `ec2fbab` et successeur live

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

La partition dual-tree des paires et la géométrie conservatrice du cœur sont
une bonne microgate Q7. Le pin ne reçoit toutefois ni la fermeture ni la route :
`smax` est ignoré, trois injections ne modélisent aucun faux prune, l'arrondi
supérieur réellement unsafe n'a pas de fixture, le compteur dit « vide » pour
« sous-plein q4 », aucun ID n'est authentifié, le juge peut masquer une mauvaise
sélection et le probe rematérialise `n^2` puis rescane tout le nuage par bloc.

Verdict : **mathématique de cœur admise sous hypothèses ; diagnostic CPU borné ;
NO-GO 50 k/G4**.

## Pin logiciel et rejeu

Le probe a d'abord été observé dans le worktree du parent
`22700778af0d14bd4e25c614bf901ccf427946f2`, puis commis sans changement dans
`ec2fbab71dad5dbdfcb92e9f405b9b7e869f9e94`, commit
`run the third road, and let the measurement retire it`. Les objets rejoués
portent :

| objet | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `d1b3cbbe20e7fa198c6132159e79705ac7677a70cac9eedbfd68dfa1fb2cf8bd` |
| `prototype/common_core_probe.cpp` | `db9ef4e2cd2f98c6fcc69f3be2d808203464df711692792d8aeb831ed1a43950` |
| ELF Release | `774d4c1020702ba556fc0c18d1818cffee04f00db3278c4567d17bffdf835eb8` |

L'ELF a `70 992` octets, Build ID
`e792b822c35a6ee3db5df2981b4d91b1d62b5a1d`. Release, CUDA désactivé. Les
sept CTests `mhgp3v_coeur_` rendent `7/7` en `0,67 s`. Aucun mutant n'est armé ;
les sept portes sont trois runs et quatre refus/planchers.

La note Claude qui interprète ce pin est
[`NOTE_CLAUDE_GATE_TROIS_VOIES_20260813.md`](NOTE_CLAUDE_GATE_TROIS_VOIES_20260813.md),
SHA-256 `4cd3e88dea7dddee7a7b42a4b3ca421b6cea345d7a46647df5dfbe008309454d`.
La réponse architecturale est séparée dans
[`AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md`](AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md).

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
constantes. Le reproducteur mordant sur l'ELF pincé est :

```text
./build/v3/mhgp3v_common_core_probe --points=400 --family=terrain --seed=3 --judge --smax=34
```

Il rend code `0`, ferme `320` relations q4 avec `accord=320` et publie
`occupation_hwm=16`. L'autorité q4 exige pourtant `h=34+1-4=31`; aucune de ces
fermetures n'est justifiée par ce reçu. C'est un faux vert commun au sujet et au
juge. Refuser `smax!=11` avant calcul ou dériver `h=smax+1-q` partout, avec une
fixture occupée `smax=11/12/34`.

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
- `coeur-bord-interieur` est **sound**, donc impropre comme mutant. Soit
  `u=z-(a+b)/2` et `D=||b-a||`. Même sur la frontière `||u||=D/4`, on a
  `H=D^2/4-||u||^2=3D^2/16` et
  `Q=||(b-a) cross u||^2<=D^4/16`; ainsi
  `2H^2=18D^4/256>D^4/16>=Q`. Le site est encore q4 strict, donc aussi q3/q2.
  L'injection doit être retirée ou renommée ablation sound.
- L'arrondi du rayon vers le **haut** est au contraire unsafe. Fixture entière :
  `a=(100,100,100)`, `b=(105,100,100)`, puis, relativement à `a`, les dix sites
  `(1,1,0),(1,-1,0),(1,0,1),(1,0,-1)`,
  `(2,1,1),(2,1,-1),(2,-1,1),(2,-1,-1)` et
  `(3,-1,1),(3,-1,-1)`. Le rayon exact vaut `5/4`; le floor entier vaut `1` et le ceil
  mutant vaut `2`. Les dix sites entrent dans le cœur mutant, mais chacun échoue
  q4 : pour les trois types, `2H^2-Q` vaut respectivement `-7`, `-18` et `-18`.
  L'ancienne soustraction de deux laisse alors huit faux crédits et peut fermer
  le bloc. Graver cette fixture sur deux feuilles singleton et un juge q4 exact.
- `coeurs_vides` est incrémenté dès que l'occupation utilisable est inférieure
  à huit. Il mesure des cœurs **q4 sous-pleins**, pas des cœurs vides. Le claim
  CMake `2 306 cœurs parfaitement vides` est directement réfuté par son propre
  run `eight_clusters n=600` : `emis=2306`, `vides=2306`, mais
  `occupation_hwm=7`. Au moins un cœur n'est donc pas vide. Publier l'histogramme
  exact `0,1,...,>=h`, `underfull_q2/q3/q4` et les masses de blocs séparément.

## Juge et reçu incomplets

Le probe ne conserve ni les IDs du cœur, ni une antichaîne de plages/nœuds, ni
leur digest. Le juge recompte tous les témoins universels du nuage pour chaque
paire fermée. Des témoins extérieurs au cœur peuvent ainsi masquer une erreur de
range query ou de crédit. À petit `n`, conserver les IDs triés et rejouer chacun
dans le cœur strict ; à grand `n`, produire une antichaîne de nœuds disjoints,
cardinalités et choix canonique des `h` premiers IDs.

Le cœur fermé de rayon `D/4` implique déjà q4 strict par l'inégalité précédente,
donc a fortiori q3 et q2. Cette implication rend la lane q2 sûre, mais elle doit
encore être gravée et jugée séparément. Le profil impose aussi le rejet des
positions colocalisées avant LBVH.

Les occupations `1..h-1` ne sont pas nécessairement du travail perdu : chaque
occupant du cœur est un témoin individuel commun à toutes les paires du bloc. Il
peut compléter des témoins de dominance ou des crédits coniques si un ledger
conserve les `PointId` et impose la disjonction des ensembles membres. Publier
l'histogramme et les IDs avant de conclure qu'un cœur sous-plein est inutile.

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

## Successeur `d3329fe` partiellement réparé

Après le pin, `HEAD` a avancé à
`d3329fea4b595b7bbd283e509b0fa1955fcc3b06`. Le successeur
`prototype/common_core_probe.cpp`, SHA-256
`4660fe37e83e284b2748dcb9b437e010578718775c7774743c33d58ca4aefe42`, retire
les deux faux mutants séparation/count, supprime le retrait de deux, sépare
`occupancy_zero` des sous-plénitudes et vérifie la multiplicité de chaque paire.
Ces réparations répondent au contre-audit. Son `CMakeLists.txt` porte le SHA-256
`a667047d8bf7702473b9915b2d06c601d533b6935d9d678d44ceb568ebf13f8b`.

Reconfiguré en Release/CUDA OFF, ce successeur construit et rend `7/7` CTests
`mhgp3v_coeur_` en `1,20 s`; ELF SHA-256
`e32075e39450a01e8ce2a79ded9e265a0aa65d82fcfff383034a2f4e460a2da4`, Build
ID `15728d7d527365c252d6d9d06d76a642f264c519`. Ce vert ne transfère pas une
réception : les sept portes restent trois mesures et quatre refus/planchers,
sans exécuter un mutant.

Le P0 `smax` persiste. Sur ce nouvel ELF, le reproducteur `terrain n=400,
smax=34` ferme désormais `452` relations q4 avec `accord=452`, tandis que
`occupation_hwm=16<31`. `coeur-bord-interieur` reste présenté comme mutant bien
qu'il soit sound; l'arrondi supérieur unsafe n'a pas la fixture ci-dessus. Le
compteur `underfull` inclut aussi les zéros malgré son commentaire « non vide » :
publier soit des classes disjointes, soit définir explicitement leur emboîtement.

La multiplicité ajoute une quatrième matrice byte `n*n` aux trois lanes, soit
environ `10 GB` à `n=50 000`, avant l'arbre et les sorties. Elle est légitime
chez le juge borné, jamais dans la route. Enfin le reçu imprime encore
`backend=cpu_reference mode=proposition_math_non_recue`, et non les cinq champs
du cadre v3 de cet audit; aucune porte n'assert cette ligne.

GCP non utilisé.
