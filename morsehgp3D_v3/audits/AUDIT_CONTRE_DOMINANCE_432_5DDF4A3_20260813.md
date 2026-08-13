# Contre-audit du premier probe dominance 432

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict court

Le certificat de hauteur par sous-cône est une avancée mathématique réelle : il
ferme un intervalle de cibles après un top-`h`, et son cutoff direct peut être
prouvé par une réduction finie aux rayons. Le probe committé n'est toutefois
pas encore une gate industrielle ni même un sujet exact sur tout son domaine
CLI : `smax!=11` produit un faux positif, le mutant cible--témoin lit une case
hors du préfixe top-`h` valide, les mesures mêlent des cutoffs et le ledger reste
quadratique.

Verdict : **primitive mathématique admise sous gate finie ; probe borné
diagnostique ; NO-GO 50 k/G4 et aucune pente reçue**.

## Pin observé

Le pin documentaire et logiciel est `HEAD=5ddf4a3f163d505cb140c5dba9b9481bfc48b8d4`,
commit `a boundary mutant that picks the same order as the reference is not a
gate`. Le build et le rejeu ci-dessous portent sur :

| objet | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `3008389f9299cdf8193cae56fcbf2ac687c5e503ca507a337c511115b87a0e42` |
| `prototype/directional_dominance.hpp` | `2e33685d6d66c8e3d8d3a1ed81a9a6f80dca89bf7c7b377ca96bce5c08cfe173` |
| `prototype/directional_dominance_probe.cpp` | `4116e78844958baf031f4d3d34e985822a7ee12edf47d07818d777fd466b616d` |
| ELF Release | `9b0bb5190e25797ddec3d81cee21b0fa67326fa88a248416537c807976edc60c` |

Configuration locale : Release `-O3 -DNDEBUG`, GCC, CUDA désactivé. L'ELF a
`80 184` octets et le Build ID
`85991c9d177df867838e40f274d1b8b177fb7397`.

La note de mesures auditée est
[`NOTE_CLAUDE_DOMINANCE_432_MESURES_20260813.md`](NOTE_CLAUDE_DOMINANCE_432_MESURES_20260813.md),
SHA-256 `4853e144399e2792a8ed034a9d041f3b614407effcc71983ffa2c77e1b95ea51`.

## Ce qui est admis

### Le cutoff direct est un certificat suffisant

Sur la section `tau=3`, pour deux directions `u,v`, poser `A=u dot v`,
`V=||v||^2`, `Q=||u cross v||^2`, et `c=2` pour q4 ou `c=3` pour q3. Le seuil
exact à directions fixées est :

$$\frac{x}{y}>T_c(u,v)=\frac{V}{A-\sqrt{\frac{Q}{c}}}.$$

Pour `lambda>0`, la fonction
`F_lambda=A-sqrt(Q/c)-lambda*V` est concave en `u` à `v` fixé, puis concave
en `v` à `u` fixé. Avec `lambda` choisi comme l'inverse du maximum des neuf
seuils aux paires de sommets, les minima successifs sont donc atteints aux
sommets et `F_lambda>=0` sur le produit des triangles. L'énumération exacte
des neuf paires place le pire cas aux deux rayons extrêmes de chaque cellule.
Après élimination de la racine, le code reçoit exactement la condition :

$$xP-yB>0\quad\text{et}\quad c(xP-yB)^2>Cx^2.$$

Il ne faut pas décomposer cette preuve en faux minorant `u dot v>=P` : dans
`U10`, `(3,1,0) dot (3,1,0)=10<P=11`. La frontière est celle de l'enveloppe
uniforme worst-case de la cellule, pas celle de chaque couple de directions.

Deux vérifications indépendantes soutiennent la preuve sans la remplacer : un
scan de toutes les directions canoniques entières de hauteur au plus `32` a
testé environ `1,45` million de fermetures q3/q4 sans faux spindle ; le selftest
rejoué reçoit la fixture d'égalité `U00`, marge zéro puis fermeture à la hauteur
entière suivante.

Gate manquante : générer dans le test les neuf valeurs `(A,V,Q)` par cellule,
prouver exactement pour `c=2,3` que chaque seuil est au plus celui de la table,
puis vérifier l'inclusion dans le spindle exact sur un domaine rationnel borné.

### Le théorème d'intervalle est la bonne direction

Pour une ancre et une cellule half-open, un top-`h` de hauteurs suffit à fermer
en bloc tout suffixe de cibles. Cela autorise une future ordonnance par
dominance/range-count sans énumérer chaque paire. Le probe actuel mesure la
masse de cet ensemble ; il n'implémente pas encore cette ordonnance.

## P0 — domaine `smax` faux

Le header fige `kNeed={10,9,8}`, qui n'est vrai que pour `smax=11`, tandis que
la CLI accepte tout `smax` de `4` à `34` et transmet cette valeur au juge. Le
reproducteur exact sur l'ELF pincé est :

```text
./build/v3/mhgp3v_directional_dominance_probe --points=100 --family=terrain --seed=3 --judge --smax=34
```

Il rend code `1`. La lane q4 publie `3` fermetures dirigées ; le juge en accepte
`2` et réfute `(46,60)`, donc `accord=NON`. Ce n'est pas un manque de puissance
mais un faux prune nominal.

Réparation à transmettre à Claude : soit refuser toute valeur autre que `11`
avant calcul, soit dériver `h=smax+1-q`, dimensionner le top-`h` jusqu'au besoin
maximal et laisser toute banque sous-pleine au résiduel. Graver au minimum
`smax=4,11,12,34` contre le juge, plus le refus hors domaine.

## P0 — le mutant cible--témoin est impossible et son vert vient d'un zéro gratuit

Le code mutant incrémente `have` sans insérer la cible dans la ligne top-`h`,
puis lit `tau[need-1]`. Quand la cellule avait `need-1` entrées, cette case est
hors du préfixe valide. L'allocation `new TopK()` value-initialise toutefois
l'agrégat : la case vaut zéro, il ne s'agit donc pas d'une UB. Le mutant ajoute
en pratique un témoin gratuit de hauteur `0`, pas la cible. Le rejeu Release
passe le CTest et publie `50` désaccords pour cette faute artificielle ; il ne
reçoit pas la propriété annoncée.

Un mutant correctement implémenté est au contraire structurellement inerte :
si la cible entre dans le top-`h`, alors le nouveau seuil est au moins sa propre
hauteur, tandis que tout cutoff reçu exige un rapport strictement supérieur à
`1`. Si `h` vrais témoins existaient déjà, l'ajout ne change pas la fermeture.
Il faut retirer cette porte ou tester la duplication d'un `PointId` dans une
accumulation de crédits où elle peut effectivement faire passer `h-1` à `h`.

## Frontières : convention, pas exactitude géométrique

Le mutant initial de frontière était mécaniquement identique au chemin normal ;
une énumération des `511` triples non nuls de `{0,...,7}^3` donnait zéro owner
différent. Le successeur choisit maintenant la dernière permutation valide et
diverge réellement. Il reste sound parce que la direction de bord appartient
aux fermetures des cellules adjacentes.

Le ledger counter-only **dépend** néanmoins de cette convention : changer
l'owner change les pools top-`h` et les seuils, ce que les `232 273` fermetures
différentielles de la note rendent observable. C'est acceptable pour un
certificat suffisant à convention fixée ; seule la sortie après consommation
exacte du résiduel doit être invariante. Une pleine équivariance octaédrale de
l'owner est impossible sur les stabilisateurs : permuter `x/y` fixe la direction
`(1,1,0)` mais échange ses cellules adjacentes.

La gate doit donc fixer des CellId attendus sur rayons, faces et diagonales,
tester la convention d'axes et l'invariance aux `PointId/workers`, puis comparer
l'autre owner comme **ablation**, non comme mutant de soundness.

Le pin n'est d'ailleurs pas conforme à l'autorité documentaire actuelle, qui
attribue toute égalité au plus petit identifiant de sous-cône. Le chemin normal
rend `cell_of(3,1,0)=1`, alors que ce rayon est partagé par `U00` id `0` et
`U10` id `1`. De même, les fixtures de convention actuelles donnent
`(3,1,1)->3`, `(3,3,1)->6`, `(3,3,3)->8`; la diagonale interne envoie
`(12,5,1)` vers `U10` et `(12,5,2)` vers `D10`. La géométrie reste sound, mais
la canonicité n'est pas reçue : soit le code applique le plus petit CellId, soit
une autorité ultérieure amende explicitement la convention et ses digests.

Deux autres injections sont mal qualifiées : avec le cutoff direct, le facteur
`2` est plus conservateur que tous les seuils q3/q4, donc sound et non tuable ;
la cellule voisine est en revanche tuable. Fixture exacte : `a=(0,0,0)`,
`b=(96,0,0)` dans `U00`, et huit sites `s_k=k(6,3,1)`, `k=1,...,8`, dans
`U10`. Le mutant emprunte `tau_h=48` puis applique la forme `U00` : il ferme q4
car `2*336^2>18*96^2`. Mais pour `s_8`, `H=1 664`,
`R=5 898 240` et `2H^2=5 537 792<R`; seuls les sept premiers sites sont
témoins, donc le prune est faux. Cette fixture doit remplacer le différentiel
générique sans désaccord.

## Mesures : séries incompatibles et compte mal nommé

La première table de la note mélange la forme directe à `n=2 000/4 000` et la
forme radiale à `n=8 000/12 500` ; `eight_clusters n=8 000` est même la valeur
radiale. Elle ne forme donc aucune rampe. La croissance d'un compte brut, ni
même celle d'une fraction fermée, ne prouve que le résiduel physique est sparse.
Il faut un même ELF, cutoff, seed et schéma aux tailles `2k/4k/8k/12.5k`, puis
les pentes des requêtes, records et octets résiduels.

`cellules_sous_pleines` n'est pas un compte de cellules : il est incrémenté
dans la boucle cible/lane et mesure des **cibles dirigées tombant dans une
cellule sous-pleine**. À `n=200`, l'univers de couples ancre--cellule vaut
`200*432=86 400`, pas `39 800`. Renommer le compteur et publier séparément
cellules sous-pleines, masse cible et quantiles par ancre.

La sélection des plus petites marges est un stress utile, pas la seule zone où
une faute de classification ou d'algèbre peut se produire. Conserver aussi un
échantillon stratifié par cellule, lane, marge et ancre, avec plancher de
tirages par strate.

## Le probe reste quadratique

Le sujet exécute explicitement deux passages sur toutes les paires dirigées,
puis trois lanes, et matérialise trois bitsets de `C(n,2)` bits. À `50 000`, ces
bitsets seuls réservent environ `468,75 MB`. Cela est acceptable comme mesure
bornée, pas comme la gate counter-only industrielle définie par l'audit
collectif.

La prochaine gate doit construire réellement les index de dominance, effectuer
count--scan sur les intervalles et garder le résiduel sous forme factorisée. Elle
publie travail physique, bytes et HWM, avec deux pentes au plus `1,35`. La masse
sémantique `residual_pair_mass`, potentiellement quadratique, reste distincte
des `residual_node_records` physiques. Aucun G4 ne précède cette porte.

## Rejeu local

La commande ciblée :

```text
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_dominance_'
```

rend `15/15` en `22,65 s`. Le selftest publie `6 858` directions, `432`
cellules et la marge stricte nulle. Sur `uniform n=2 000`, le juge tire `800`
fermetures parmi `168 414` et trouve zéro désaccord. Cela ne transfère aucune
preuve aux autres fermetures, ne reçoit pas les pentes et n'efface ni le faux
`smax=34` ni la fausse porte du témoin gratuit de hauteur zéro.

GCP non utilisé.
