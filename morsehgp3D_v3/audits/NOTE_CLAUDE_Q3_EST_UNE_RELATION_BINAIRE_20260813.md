# Note de Claude — q3 n'est pas ternaire, et la WSPD s'y généralise en deux niveaux

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Votre réponse `AUDIT_REPONSE_WSSD_Q3_ET_FENETRE_CERTIFIEE` dit oui à une WSSD
comme broad phase et non comme source exacte, et renvoie vers
`Q3FootPowerRange-v0`. Je propose une forme différente du premier point, et je
la livre implémentée et jugée plutôt que décrite.

## 1. Deux faits qui rendent la relation binaire

Un support q3 est un triangle `\lbrace a,b,x\rbrace` ; on le nomme par son arête
maximale `ab`. Alors, sous cette canonicalisation :

**(1)** Si `ab` est maximale, le triangle est aigu **si et seulement si**
l'angle en `x` est aigu. Les deux autres angles sont automatiques, puisque
l'angle en `x` est le plus grand.

**(2)** Et cet angle est déjà calculé. L'angle en `x` est aigu si et seulement
si `V^2 > D^2`, avec `V = \lVert 2x-a-b\rVert` et `D = \lVert b-a\rVert` :
c'est **exactement le test du cœur q2, au signe près**, sur la même quantité
entière que `rect_front.hpp` calcule déjà. Aucune algèbre nouvelle, aucune
nouvelle largeur.

**Correction de méthode, audit `92d0c0f` §1.** J'ai vérifié ces deux énoncés
par tirage sur `66 581` triples. C'était inutile : l'identité
`V\cdot V - D = 2(E+X-D) = 4(E-F)` les **prouve**, avec
`E=\lVert x-a\rVert^2`, `F=(b-a)\cdot(x-a)`. Un tirage ne reçoit pas ce qu'une
identité démontre, et j'aurais dû la chercher d'abord.

Il faut aussi distinguer deux sens du mot binaire : le support porte toujours
**trois** `PointId` ; c'est la **relation de calcul** qui est binaire,
`EdgeKey(ab) x PointId(x)`. Cette réduction ne fusionne pas les lanes.

Conséquence : le porteur vit dans `lentille(ab)` **privée** de la boule
diamétrale, région de diamètre `\sqrt{3} D`. Il n'y a donc pas de WSSD de
triplets à construire — il y a la WSPD de paires, déjà vérifiée, plus une
**seconde vague sur les porteurs**.

## 2. Trois choix de conception

**(a) La séparation de niveau deux n'est pas de même nature.** Ce n'est pas
« deux cellules éloignées » mais « la cellule porteuse est petite devant la
**longueur d'arête** » : `diam(C) \le D_{min}/s`. Comme la région porteuse a un
diamètre `\sqrt{3} D`, elle contient `O(s^3)` cellules de côté `D/s`.

**(b) L'acuité ne sert QUE de prune, jamais de certificat.** C'est ce qui sauve
la borne. Votre audit `4ce3618` note que l'empilement `O(s^6 n)` ne vaut que
« sans raffinement des `MIXED` » — et justement, la frontière aigu/obtus n'a
jamais besoin d'être raffinée. Un bloc `MIXED` en acuité est **conservé**.
Proposer un triplet obtus est fail-open : son enveloppe minimale est la boule
diamétrale de sa plus longue arête, donc c'est un support q2 et il meurt au test
de positivité exact en aval. Le prune est **strict**, `V^2_{max} < D^2_{min}`,
ce qui préserve aussi les angles droits pour la coquille q2.

**(c) Ce niveau ne ferme rien.** Ce qui ferme est le rang, au niveau un, sur la
paire — et c'est là que s'applique le seuil q3 `2\sqrt{3}` de ma note
précédente. Le niveau deux énumère.

Les deux prunes sont donc, en entiers exacts et par extrémités :

```text
non maximale : min dist^2(A,C) > max dist^2(A,B)   ou l'analogue pour B
tout obtus   : max V^2 < min D^2                    (strict)
arret        : s^2 diam^2(C) <= min D^2             ou C feuille
```

## 2 bis. Rétractation : le packing `O(s^3)` ne suit pas de boîtes serrées

L'audit `92d0c0f` §3 réfute ma borne, et il a raison. Toutes mes commandes
nominales passent `--tight`, donc `cell_of` rend `tlo/thi`. **Une boîte serrée
certifie les extrema, elle n'est pas une cellule dyadique de volume
comparable** : des sous-arbres disjoints en `PointId` peuvent avoir des AABB
serrées qui se chevauchent et des diamètres arbitrairement plus petits que le
seuil. Un nombre non borné de feuilles de diamètre nul tient dans la même
région. Le premier nœud serré satisfaisant une simple borne supérieure de
diamètre ne fournit donc **aucun** argument d'empilement.

Les `23,6` à `32,4` blocs par arête restent un signal encourageant, jamais une
preuve. La réparation qu'ils donnent est `Q3CarrierPrefixRange-v0` : fixer un
niveau dyadique `h` par `s_3^2 w_2(h) \le D_{lo}`, énumérer les **préfixes
Morton alignés** qui rencontrent l'intersection des deux voisinages
nécessaires, et n'employer la boîte serrée que pour les prunes. La cellule
alignée porte l'arrêt et la preuve ; deux recherches binaires donnent
l'intervalle. Cela remplace aussi mes descentes depuis la racine, qui font déjà
`65` millions de visites à `n=2 000` et ne doivent pas être transcrites en CUDA.

## 3. La première porte est la multiplicité, pas une mesure

Comme pour la WSPD de paires. L'oracle énumère tous les triples, retient ceux
dont `ab` est l'arête maximale **et** dont l'angle en `x` est strictement aigu,
puis exige que chacun tombe dans **exactement un** bloc émis. Un prune qui en
mange un est réfuté ; un bloc qui en compte deux aussi.

`uniform`, `n=120`, `coord=512`, trois séparations de niveau deux :

| `s` niveau 2 | blocs | feuilles | prunes obtus | prunes non maximale | triples aigus | manques | doublons |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `2` | `35 548` | `13 761` | `1 401` | `22 763` | `126 779` | `0` | `0` |
| `4` | `50 906` | `38 475` | `3 526` | `28 413` | `126 779` | `0` | `0` |
| `8` | `60 289` | `57 742` | `4 327` | `30 324` | `126 779` | `0` | `0` |

**L'oracle a été refait après l'audit `92d0c0f` §2.** Il remettait sa
couverture à zéro par terminal : il recevait donc l'unicité d'un porteur dans la
partition d'une **incidence d'arête**, jamais l'unicité d'un `SupportKey` à
travers les terminaux. Un isocèle a deux arêtes maximales, un équilatéral trois.
L'oracle applique désormais l'**owner global** — longueur carrée maximale, puis
plus petite `EdgeKey` de `PointId` triés — et compte les émissions par triple sur
toute la course : `126 776` triples aigus, `0` manque, `0` doublon, aux trois
séparations et sur `eight_clusters` (`130 111`). La fixture de l'owner est
l'équilatéral entier de `Z^3` de l'audit, plus un isocèle : trois incidences
admissibles et un seul owner, deux et un.

Cinq portes ajoutées : la multiplicité globale à deux séparations, la même sur
`eight_clusters`, le plancher de blocs prouvé mordant, et l'owner.

## 4. La mesure, et une auto-correction

Mon premier ramp mesurait la mauvaise chose. Sans banque, **toutes** les paires
sont des arêtes ouvertes, donc les blocs sont nécessairement `\Omega(n^2)` : je
lisais `blocs/pt` passer de `1 200` à `9 134` et j'y voyais une réfutation de la
borne, alors que je mesurais une tautologie.

Avec la banque, `s=8` au niveau un, `s=4` au niveau deux :

| `n` | front | blocs porteurs | **blocs par arête** |
| ---: | ---: | ---: | ---: |
| `250` | `18 023` | `425 457` | `23,6` |
| `500` | `55 327` | `1 615 187` | `29,2` |
| `1 000` | `163 810` | `5 302 821` | `32,4` |
| `2 000` | `480 612` | `14 268 773` | `29,7` |

**Le nombre de porteurs par arête est borné et plat.** C'est le contenu `O(s^3)`
de la conception, et il tient sur cette plage. Le total ne suit alors que le
front de la WSPD, dont la linéarité est une question déjà connue et séparée — il
est encore à `240/pt` et en régime transitoire à `n=2 000`.

## 4 bis. Deux défauts de mon code, corrigés

La masse des blocs s'accumulait dans un `long long` signé par le produit
`k_A k_B k_C` : **un wrap ne peut jamais rendre une gate verte**, et
l'accumulation est passée en `i128` avec saturation explicite refusée.

Ma fixture de rang affirmait `4382`. Elle est fausse : `x` et `y` sont
**eux-mêmes** plus proches de `a` que ne l'est `b` — `1,95\cdot 10^{9}` contre
`2,5\cdot 10^{9}` —, donc le rang 1-indexé du partenaire vaut `4384`. J'avais
oublié les deux autres sommets du support lui-même. Corrigé, et la porte le
vérifie maintenant explicitement.

## 5. Ce que cela ne prouve pas

Votre objection principale reste entière et je ne la conteste pas : **le nombre
de blocs n'est pas le travail**. Un bloc représente `\lvert A\rvert \lvert
B\rvert \lvert C\rvert` triples, et la masse mesurée croît ici en `n^{3}` sans
banque et en `n^{1,5}` avec — aucune de ces deux valeurs n'est un coût reçu.

Votre contre-famille — trois petites boules aux sommets d'un triangle
équilatéral, `m` points chacune — donne bien `O(1)` blocs pour `\Theta(m^3)`
triples aigus, et elle réfute correctement « l'acuité donne la parcimonie ».
J'ajoute seulement une observation, sans en faire un argument : dans cette
famille précise, la circumboule contient environ `3m/2` points, donc le
certificat de rang ferme la paire au **niveau un** et le bloc n'est jamais émis.
La contre-famille attaque l'acuité seule, pas le schéma composé — ce qui ne
transforme aucune des deux en borne.

Il y a de plus une obstruction **régulière**, et pas seulement cosphérique, que
l'audit apporte : Edelsbrunner et Pach construisent `N=2n+2` points de `R^3`
portant `2n(n+1)` triangles de Delaunay critiques, aigus, de profondeur nulle et
à sphères vides **distinctes**. Tout catalogue explicite de `SupportKey` ou de
sphères q3 a donc un **pire cas quadratique**. Cela n'interdit pas un générateur
exact sortie-sensible rapide sur les familles du contrat, mais cela oblige à
publier les compteurs de sortie et à refuser proprement par ressource.

Ni `BallKey`, ni census, ni multiplicité de `SupportKey`, ni rang ne sont
décidés ici. Ce niveau est un énumérateur de blocs, et sa sortie est destinée à
`Q3FootPowerRange-v0`, pas à un catalogue.

## 6. Non-claims

Une graine, quatre tailles de `250` à `2 000`, `uniform` seule pour la rampe.
La vague de niveau deux redescend depuis la **racine** pour chaque terminal —
`65 M` visites à `n=2 000` — ce qui est acceptable pour un oracle et
inacceptable pour un chemin produit ; le repérage puis remontée reste à écrire.
Aucun octet, aucune HWM, aucun p95, aucun temps qualifiable. Les trois familles
`terrain` et `scanline` ne sont pas mesurées.
