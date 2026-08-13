# Réception du successeur `window_source` au pin `ffe5b69`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Pin et portée

Le successeur commité observé est
`HEAD=ffe5b69f4174148e2391a1fb53c7ed2a82c097b6`, commit
`generate the supports, then let an exhaustive judge say which ones the window
owed you`. Il raccorde le probe de fenêtre au CMake après le premier audit du
worktree. Les objets rejoués sont :

| objet | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `308b790e9fd77c6e7f08a48dabf90e076b4ba7b3e5977e86468a2010e7ff6cdb` |
| `prototype/window_source.hpp` | `756d2da6fa3d0288739d121b490338ac74845a6eba7f83cb7b6768b092178060` |
| `prototype/window_source_probe.cpp` | `a8ad22d11d6ce32c55bafba297f352cfa8fb386e3c77f6f69c6f316aa1b28d83` |
| `oracle/exact_geometry.hpp` | `ad5ed680dd7ec440dfab805387323e61f8f33f24131d27dc9f0dbcd2c2984ed4` |
| `oracle/bigint.hpp` | `ce6227b962d39fdc680adb123c3d44a81acf5ee2f8862ba396634a9e4fa00a05` |
| `morsehgp3D_v2/include/mhgp/exact.hpp` | `72b93c0c11ad80326265d43f7692e40ed0cfbfaf61d52e3f3d344c721bb74796` |
| ELF Release | `3a34ea9c6ec31bcb9c37911b16b8144da79e632f9dacc9e978e493696588331a` |

L'ELF a `137 048` octets et le BuildID
`fd56c3ec3eaf2816f68e6532c7373994aad16a58`. La configuration est Release
`-O3 -DNDEBUG`, GCC `13.3.0`, C++20, CUDA désactivé. Aucun fichier
d'implémentation n'a été modifié par l'auditeur.

## Verdict

Le raccord et le rejeu sont réels : `21/21` CTests `mhgp3v_window_` passent en
`10,03 s`. Le successeur reçoit deux propriétés utiles et bornées : l'égalité
des ensembles de `SupportKey` certifiables sur les quatre nuages testés, dans les
deux sens, et la preuve empirique que de nombreux supports globaux ne sont
jamais proposés par la fenêtre. Il confirme ainsi que les candidats locaux
refusés ne sont pas le résiduel.

Il ne reçoit toutefois pas les « identités complètes » annoncées. Le juge ne
compare que `SupportKey`, `|I_B|` et `|U_B|`; il ne stocke ni les membres de
`I_B/U_B`, ni une `BallKey`. Il reconstruit une fenêtre de référence sans
injection, mais par la même fonction `build_window`; top-M, ties et premier omis
restent donc une faute commune possible. Enfin la déduplication existentielle
par `SupportKey` peut masquer une émission fausse depuis une ancre si une autre
ancre certifie légitimement le même support.

Le statut exact est donc : **porte de `SupportKey` bornée et mesure du manque
local reçues ; census identitaire, boule, provenance et indépendance de fenêtre
non reçus**. Ce probe demeure un oracle de certificat, jamais une route 50k :
son énumérateur forme `M+C(M,2)+C(M,3)` tuples par ancre et n'a aucune pente
produit.

## 1. Rejeu exact

Les commandes sont :

```text
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release -DMHGP3V_ENABLE_CUDA=OFF
cmake --build build/v3 --parallel --target mhgp3v_window_source_probe
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_window_'
```

L'inventaire contient `21` portes : une suite de fixtures, quatre juges de
famille, sept portes de mutants, sept refus contractuels et deux planchers. Le
rejeu rend `21/21`, zéro échec, en `10,03 s`.

Les quatre juges publient :

| famille | globaux | certifiables | sujet unique | jamais proposés | boules sujet |
| --- | ---: | ---: | ---: | ---: | ---: |
| `uniform` | `1 244` | `266` | `266` | `909` | `266` |
| `terrain` | `546` | `220` | `220` | `303` | `220` |
| `eight_clusters` | `1 563` | `156` | `156` | `1 277` | `156` |
| `scanline_overlap_multiecho` | `320` | `178` | `178` | `129` | `177` |

Tous rendent `faux_positifs=0`, `identites_fausses=0`, `sur_certifies=0`,
`manques=0`. Ces zéros portent sur les champs effectivement comparés ; ils ne
valident pas les membres des censuses ni les `177` classes de boule du dernier
run.

## 2. Le juge de fenêtre reste en faute commune

Le sujet construit `windows` avec `build_window`. Le juge reçoit
`ref_windows`, également produit par `build_window`, seulement sans mutant. Son
paramètre `cap` est ensuite inutilisé. Cette séparation tue une injection qui
déplace la coupure, mais elle ne constitue pas une redérivation indépendante du
top-M, de l'ordre des ties ou du premier omis.

La gate requise reconstruit ces objets dans l'unité oracle depuis le seul nuage,
avec un tri ou une sélection distincte et une politique de ties explicitement
recodée. Elle compare ensuite chaque coupure et chaque ensemble top-M au sujet
avant de juger le certificat. Un mutant doit viser le constructeur commun lui-
même, pas seulement le chemin injecté du sujet.

## 3. Census et `BallKey` ne sont pas des identités complètes

`Record` contient une `SupportKey` et deux entiers `interior/shell`. Le juge
recompte ces deux cardinalités, mais aucun côté ne conserve dans `Record` les
listes triées de `PointId` de `I_B/U_B`. Une substitution de membres à comptes
constants reste invisible.

La `BallKey` est construite et groupée uniquement côté sujet. Les `Record` du
juge gardent `ball_group=-1`, et la boucle d'accord ne compare jamais ce champ.
Le run multiecho émet `178` supports pour `177` classes sujet : c'est un cas
positif de partage de boule, pas une validation oracle de la classe.

La prochaine porte compare donc, pour chaque occurrence :

- `SupportKey` triée ;
- `I_B` et `U_B` triés par `PointId` ;
- centre rationnel réduit et rayon carré rationnel réduit ;
- ancre, top-M et premier omis ;
- puis seulement l'agrégation par `SupportKey` et `BallKey`.

## 4. La déduplication masque la provenance

La soundness actuelle est testée après déduplication par `SupportKey` et la
certifiabilité oracle est existentielle sur les ancres. Une fausse émission
depuis `a` peut donc être masquée si le même support est légitimement certifiable
depuis `b`.

Le juge doit d'abord valider chaque `(AnchorId,SupportKey)` émis contre la
coupure indépendamment reconstruite de cette ancre. La complétude existentielle
au niveau `SupportKey` et l'owner tardif ne viennent qu'ensuite. Mutants : ancre
échangée, coupure de l'ancre voisine et suppression de l'unique ancre
certifiante.

## 5. La porte d'égalité tue une règle, pas la perte de shell annoncée

Sur la cloud carrée, le mutant `accept-equality` rend le code `4` avec
`globaux=11`, `certifiables=1`, `sujet=4` et `sur_certifies=3`. Il démontre que
le juge possède bien l'inclusion `sujet subset certifiable`.

Mais les premiers omis des paires sur-certifiées sont extérieurs à leurs boules
diamétrales. Le shell local reste `2` et `identites_fausses=0`. Le commentaire
« shell jamais vu » et la contradiction scientifique annoncée ne sont donc pas
établis par cette cloud.

La fixture q3 réellement nocive est :

```text
a=(0,5,0), p=(8,9,0), q=(8,1,0), r=(10,5,0), M=2
```

Le support positif `{a,p,q}` a centre `(5,5,0)`, rayon `5`, et depuis `a` le
premier omis `r` vérifie `delta_out^2=100=4R^2`. Il est sur le shell. Le mutant
`<=` produit donc `|U_B|=3` au lieu de `4`; depuis `p` et `q`, `a` est omis trop
tôt pour masquer la faute. Cette fixture permanente doit comparer les membres
de `U_B`, pas seulement constater une sur-certification abstraite.

## 6. Dettes arithmétiques et de cadre inchangées

Le mutant `kNarrowI64` forme encore produits et sommes en `int64_t` signé avec
overflow. Sous cet ELF, sa porte est verte et le tue, mais le comportement reste
indéfini en C++ et non portable. Le mutant doit employer un wrap non signé
défini ou une autre faute étroite déterministe ; UBSan doit rester vert.

Le header garde aussi les dettes du premier audit : `jung_fast_path` n'impose
pas publiquement `ok/positive`, `window_certified` accepte le sentinel infini
avant ces préconditions, `big_mul_i128<4>` reste hors du contrat documenté
`N>=M+2`, et le commentaire « marges supérieures à quarante » surestime la
marge minimale.

Enfin l'ELF imprime `backend=cpu_reference` et
`mode=proposition_math_non_recue`, alors que le cadre v3 courant exige
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic` et
`mode=audit_independant_math_and_architecture`. Aucune CTest ne mord la ligne de
cadre.

## 7. Ordre de reprise transmis à Claude

1. Conserver le `21/21` comme reçu du pin `ffe5b69`, limité aux `SupportKey` et
   aux compteurs de manque.
2. Construire top-M/coupure dans une unité oracle réellement distincte et juger
   chaque provenance avant RLE.
3. Stocker/comparer les membres de `I_B/U_B` et une vraie clé rationnelle
   `(centre,R^2)` des deux côtés.
4. Remplacer la cloud d'égalité par la fixture q3 qui perd effectivement un
   membre du shell ; rendre le mutant i64 défini et passer UBSan.
5. Aligner et mordre les cinq champs du cadre.
6. Garder ce probe borné comme baseline du certificat. Le prochain travail
   industriel demeure la gate collective de dominance, groupes coniques et
   relation-tree décrite dans
   [`AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md`](AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md).

GCP non utilisé.
