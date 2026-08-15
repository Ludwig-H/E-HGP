# Contre-audit positif — propositions `W4`, seeds aigus et vrais supports q4

Date : 15 août 2026 UTC.

Note contre-auditée :
[`NOTE_AUDIT_Q4_PROPOSITIONS_VS_SORTIE_20260815.md`](NOTE_AUDIT_Q4_PROPOSITIONS_VS_SORTIE_20260815.md),
commit `eb42b5745ed2da66a66a9055625436ffab8222c4`.

Pins de contexte :

- code fonctionnel reçu jusqu'à `5ce2634cc6e1e5fa9dedc3b9736ce799802d40a5` ;
- proposition consolidée lue au pin courant ;
- compléments mathématiques de l'auditeur :
  [`NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md`](NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md)
  et
  [`CORRECTION_AUDITEUR_FORMULES_TWO_LINES_20260815.md`](CORRECTION_AUDITEUR_FORMULES_TWO_LINES_20260815.md).

Cadre : `phase=exploration_v3_hors_registre`, `backend=math_reference`,
`profile=quantized_u16_input_only`, `mode=counter_audit_positive`,
`public_status=not_claimed`. GCP non utilisé.

> [!NOTE]
> **Verdict court.** Je reçois le point central de l'autre auditeur. Le compteur
> `V_4` du broad phase est un nombre de **paires-ancrages `W4`-vivantes**, pas un
> nombre de tétraèdres, de `BallKey` ni d'événements HGP. Sur `two_lines`, cette
> proposition par paires est quadratique alors que la source q4 positive est
> vide. Le renommage des cinq étages et le déplacement de la frontière de
> matérialisation vers les seeds aigus possédés sont mathématiquement justes et
> architectuellement nécessaires.
>
> Trois précisions empêchent toutefois cette bonne correction de devenir une
> nouvelle simplification excessive :
>
> 1. la quadratique et la sphère vide doivent porter leur **domaine en `(m,H)`** ;
> 2. `root_groups <= 2*r4*seeds` borne des groupes de valeurs, pas le nombre
>    d'IDs d'apex, les touches BVH, les supports ni les octets ;
> 3. `Q4SeedAxisTopR4` ne supprime le produit interdit que si la recherche des
>    seeds et des extrêmes reste factorisée. Scanner tous les sites par seed
>    déplacerait simplement le coût d'une ligne de reçu à la suivante, activité
>    très populaire dans les pipelines compliqués.

## 1. Réception de la séparation des étages

La nomenclature proposée est la bonne :

```text
V4_pair_walive  : paires non fermées par W4Depth8
C4_carrier      : triples (owner edge, carrier aigu primaire possible)
M4_apex         : quadruplets carrier--apex avant positivité finale
W4_positive     : supports q4 affinement indépendants et bien centrés
H4_rank         : supports ayant passé census/rang/disposition
```

Il faut conserver en plus les unités physiques déjà prévues par
`PROPOSITION.md` :

```text
F4_block        : blocs physiques de scheduling
R4_bundle       : groupes de roots retenus
T4_site         : IDs/touches réellement parcourus ou reportés
N4_event        : centres/événements géométriques distincts
Z4_const        : lots de niveau constant
```

Sans cette seconde famille de compteurs, une masse logique factorisée peut être
confondue avec un coût physique faible, ou inversement. Les deux ont la fâcheuse
habitude d'être des entiers positifs, ce qui suffit souvent à déclencher une
comparaison illégitime.

La chaîne conceptuelle reçue est donc :

```text
NeutralPairTape
  -> AcuteCarrierGateway-q4
  -> Q4SeedAxisTopR4
  -> owner6 + barycentriques strictes + primary
  -> BallKey/RLE
  -> census/rang
  -> fold HGP
```

Lane4 reconstruit ses propres carriers géométriques. Elle ne consomme ni une
sortie Lane3, ni son rang, ni son cutoff.

## 2. `two_lines` : résultat exact et domaine

Posons

```text
A_i=(i,0,0),
B_j=(0,j,H),
1<=i,j<=m,
n=2m.
```

Le résultat utilisé par la note est exact dans le domaine suivant :

```text
2 (m-1)^2 <= H^2+1.
```

Alors aucune paire croisée ne possède de témoin ponctuel q4 et

```text
V4_pair_walive = m^2 + 16m - 72
                 = n^2/4 + 8n - 72
```

pour `m>=8`.

Avec `H=65535`, cela couvre

```text
m<=46341,
n<=92682.
```

Le target `n=50000`, donc `m=25000`, est largement dans ce domaine. La phrase
`Theta(n^2)` est donc une description légitime de la rampe pertinente, mais pas
un théorème sans hypothèse sur une famille u16 où `H` serait fixé tandis que
`m` croîtrait au-delà du profil.

### 2.1 Sphère vide compatible q4

La sphère explicite passant par `A_i,B_j` a pour centre

```text
c_ij = (i, j, (H^2+i^2-j^2)/(2H)).
```

Pour les autres points :

```text
Pow(A_k) = (k-i)^2,
Pow(B_l) = (l-j)^2.
```

Elle est donc vide, shell des deux endpoints excepté.

Pour être compatible avec le disque de centres q4 associé à une arête maximale,
son rayon doit aussi satisfaire la borne de Jung

```text
R^2 <= 3 D^2/8.
```

En écrivant `t=c_ij-(A_i+B_j)/2`, cette condition devient exactement

```text
H^2(i^2+j^2) + 2(i^2-j^2)^2 <= H^4.
```

Elle tient uniformément pour toutes les paires du carré dès que

```text
m <= floor(H/sqrt(2)).
```

Avec `H=65535`, elle couvre `m<=46340`, donc à nouveau toute la cible `50k` et
toutes les mesures actuelles. La note contre-auditée a donc raison sur le régime
qu'elle vise ; il lui manquait seulement le domaine qui transforme une image
géométrique convaincante en énoncé rejouable.

Cette sphère montre qu'aucun certificateur **de profondeur universelle** ne peut
rendre `DEAD_DEPTH` sur ces paires : il existe une boule admissible vide. Un
gateway de positivité peut néanmoins les tuer à l'étage carrier, ce qui ne
contredit pas le plancher pair-level.

## 3. Absence de carriers : preuve complète

Tout triangle non dégénéré du nuage emploie deux points d'une même droite et un
point de l'autre. Pour `i<k`,

```text
(A_k-A_i) dot (B_j-A_i) = -(k-i)i < 0.
```

Le triangle `A_i A_k B_j` est obtus en `A_i`. Le cas des deux points sur la
seconde droite est symétrique. Les triples entièrement sur une droite sont de
rang affine un.

Ainsi :

```text
C4_carrier  = 0,
M4_apex     = 0,
W4_positive = 0,
H4_rank     = 0.
```

Le lemme du porteur aigu complète l'implication : un q4 bien centré possédant
`ab` pour arête maximale aurait au moins une face incidente `abx` strictement
aiguë. Il n'en existe aucune.

La note de l'autre auditeur reçoit donc correctement `two_lines` comme une
réfutation de la **matérialisation des propositions par paires**, pas comme une
borne inférieure quadratique sur la sortie q4.

## 4. Ce que borne réellement `2*r4`

Pour un seed exact `(a,b,x)`, si `p` sites sont intérieurs de façon permanente,
seuls les `r4-p` premiers groupes entrants et les `r4-p` derniers groupes
sortants peuvent engendrer un centre de profondeur `<r4`. En l'absence de fate
`OVERFLOW` non consommé :

```text
R4_bundle(seed) <= 2(r4-p) <= 2r4.
```

Donc, si `C4_carrier` désigne le nombre de seeds logiques exacts :

```text
R4_bundle <= 2*r4*C4_carrier.
```

Je reçois cette borne. Je recommande en revanche de supprimer le symbole
`M4_seed`, non défini dans la taxonomie proposée, et d'écrire explicitement
`C4_carrier` ou `M4_seed_exact` selon l'unité choisie.

### 4.1 Ce que cette borne ne dit pas

Un groupe de root peut contenir un nombre arbitraire de vrais `PointId` :
co-sphéricité, quantification et plateaux ne disparaissent pas parce que le
conteneur a un joli nom. Ainsi

```text
R4_bundle = O(C4_carrier)
```

n'implique ni

```text
T4_site = O(C4_carrier),
M4_apex = O(C4_carrier),
output4 = O(C4_carrier),
bytes = O(C4_carrier).
```

Sous `RelevantGP`, la multiplicité d'un root est bornée et le transfert vers les
`SupportKey` est recevable. Hors de ce domaine, le groupe égal doit être
range-reporté complètement, puis rendu comme :

```text
EXACT,
UNSUPPORTED_DEGENERACY,
PENDING_CONTINUATION,
RESOURCE_EXHAUSTED.
```

Il ne peut être tronqué à `2*r4` IDs. La gate doit donc publier simultanément :

```text
R4_bundle,
T4_site,
max_bundle_ids,
M4_apex,
continuations,
pending,
bytes,
HWM.
```

### 4.2 Le coût de sélection n'est pas la taille de la sortie

Le terme `sum_e binom(m_e,2)` disparaît seulement si les extrêmes shallow sont
sélectionnés directement. Un scan plat de tous les témoins pour chaque seed
coûte encore

```text
sum_seed |WitnessRegion(seed)|,
```

qui peut être énorme même lorsque `R4_bundle<=16*C4_carrier`.

La route reçue doit donc employer le best-first Morton/BVH de
`Q4SeedAxisTopR4`, avec :

- bornes rationnelles exactes sur les roots ;
- égalités range-reportées en seconde passe ;
- compteurs de nœuds visités et de feuilles touchées ;
- `OVERFLOW` conservé comme continuation ou refus ;
- aucune promesse `O(k log n)` avant mesure.

Le bound sur les groupes est une borne de **sortie candidate**, pas une borne de
travail de l'index.

## 5. Le maillon bloc manquant est désormais explicite

La note contre-auditée demande un verdict `NONE_ACUTE` avant expansion de la
masse pair-level. C'est exactement le rôle proposé dans
[`NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md`](NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md).

Pour trois AABB `A,B,C`, ce gateway calcule exactement les extrema de

```text
Phi(a,b,x)=(a-x) dot (b-x)
```

avec `24` produits pour le maximum et `12` produits pour le minimum, puis les
combine à des bornes corrélées de maximalité de `ab`. Il rend :

```text
NONE_ACUTE,
ALL_STRICT_OWNER,
MIXED.
```

Sur `two_lines`, les blocs d'indices ordonnés rendent uniformément
`Phi_hi<=0` ou `Delta_owner_hi<0`. La masse `m^2` peut donc mourir avant toute
liste de `PairId`. Les seuls blocs coupant une frontière d'ordre sont scindés.

Ce gateway fournit le pont constructif qui manquait à la bonne analyse de
l'autre auditeur. Il reste un candidat, pas une brique reçue : il lui faut
prototype, oracle exhaustif, mutants, pentes physiques et HWM.

## 6. Trois gates complémentaires

### 6.1 `two_lines_q4_stage_separation`

Dans le domaine analytique :

```text
V4_pair_walive = n^2/4+8n-72,
C4_carrier = 0,
M4_apex = 0,
W4_positive = 0,
H4_rank = 0,
pending = 0.
```

Et physiquement :

```text
aucun tableau de taille V4_pair_walive,
aucun PairId développé pour les paires croisées,
AcuteCarrierBlocks émis = 0,
R4_bundle = 0,
T4_site_apres_gateway = 0.
```

La rampe doit publier le nombre de blocs `MIXED` et sa pente : la preuve de
sortie nulle ne prouve pas à elle seule que le scheduler ne développe pas une
frontière quadratique pour le découvrir.

### 6.2 `regular_tetra_q4_positive`

Prendre quatre `PointId` distincts :

```text
p0=(0,0,0),
p1=(2,2,0),
p2=(2,0,2),
p3=(0,2,2).
```

Toutes les arêtes ont longueur carrée `8`. Le circumcentre est

```text
c=(1,1,1),
R^2=3,
lambda_i=1/4.
```

La `BallForm` primitive est

```text
(1, -2, -2, -2, 0).
```

Le support est affinement indépendant, strictement bien centré, vide à
l'intérieur et porte exactement ses quatre IDs sur le shell. La gate exige :

```text
un owner edge, choisi par le plus petit EdgeKey,
un primary carrier, choisi par le plus petit PointId incident restant,
un M4_apex exact,
un W4_positive,
un H4_rank,
une BallKey,
un SupportKey,
I_B=0,
U_B={p0,p1,p2,p3},
exact-once sous les 24 permutations.
```

Cette fixture empêche une optimisation conçue pour tuer `two_lines` de tuer
également toute la lane q4, réussite de performance assez facile à obtenir et
rarement commercialisable.

### 6.3 `cube8_shell_plateau`

Prendre les huit sommets de `{0,2}^3`. Ils appartiennent tous à la sphère

```text
c=(1,1,1), R^2=3.
```

La fixture force un groupe de root égal contenant plusieurs IDs. Elle doit
montrer simultanément :

```text
R4_bundle petit,
max_bundle_ids > 1,
T4_site > R4_bundle,
aucune troncature,
statut de dégénérescence ou disposition explicitement typé.
```

Elle tue la confusion `nombre de groupes = nombre d'apex` sans exiger que la
politique dégénérée finale soit déjà reçue.

## 7. Priorité commune proposée à Claude

1. Renommer immédiatement les compteurs `Vq_pair_walive` dans le probe et les
   reçus.
2. Fermer d'abord les portes de scission et de doublons déjà demandées ; elles
   rendent le tape pair-level fiable.
3. Prototyper `AcuteBox24` en autorité autonome avec oracle exhaustif.
4. Brancher seulement un **shadow factorisé** sur `two_lines` et le tétraèdre
   régulier : ne pas développer les paires.
5. Raccorder `Q4SeedAxisTopR4` avec `R4_bundle/T4_site/pending`, puis mesurer
   les visites BVH et les octets.
6. N'autoriser une rampe `50k` qu'après exact-once, `pending=0`, budget et HWM.

Le contre-audit conclut donc positivement : l'autre auditeur a identifié le bon
changement d'arité et corrigé une ambiguïté sémantique importante. Les réserves
ci-dessus ne changent pas la route ; elles empêchent seulement la borne sur les
groupes et la contre-famille finie d'être promues, par enthousiasme administratif,
en preuve de coût de bout en bout.
