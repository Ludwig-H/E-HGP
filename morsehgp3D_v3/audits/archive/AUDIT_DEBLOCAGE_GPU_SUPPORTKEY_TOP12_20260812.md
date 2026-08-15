# Audit de déblocage GPU — dédupliquer avant la géométrie, sentinelle hors support

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cet audit répond à la question industrielle suivante : si environ vingt-quatre
millions de supports utiles sont une charge acceptable pour une G4, quel travail
mathématique faut-il supprimer pour que le GPU voie une tâche utile par support,
et non des dizaines ou centaines d'occurrences de cellules par support ? Il
contre-audite aussi
[`NOTE_CLAUDE_PENTES_UNIFORM_VERTES_20260812.md`](NOTE_CLAUDE_PENTES_UNIFORM_VERTES_20260812.md).

## 1. Verdict

Le nombre attendu de supports n'est pas le verrou observé. Le point gelé
`uniform,n=50 000` publie `21 395 212` supports mais construit `839 582 666`
géométries, soit `39,242` géométries par support; `81,555 %` des propositions
finissent rejetées par l'owner. Sur `terrain,n=50 000`, les ratios valent
`127,688` géométries par support et `93,387 %` de rejets owner. L'ordonnance
CPU paie donc principalement **avant** de connaître la tâche utile.

La voie immédiate recommandée est :

```text
front/lane q -> owner génératif exact-once, sinon count/scan/fill SupportKey
                 -> radix shardé + RLE SupportKey de vérification
                 -> une géométrie exacte par clé
                 -> point-location directe de la feuille owner
                 -> positivité + rejeu du contexte owner
                 -> census producteur ou top-(12-q) hors U en fallback
                 -> fast path E=U ou side queue H!=empty/plateau
                 -> flux device vers activation/gateway/fold
```

Ce pipeline ne promet pas encore la seconde. Il change toutefois le bon ordre
de grandeur : la géométrie, l'owner et le census sont indexés par clés uniques,
pas par occurrences spatiales. Il ne garantit pas qu'une clé unique soit un
support final : le compteur `SupportKey_unique` à 50 000 manque encore et reste
la première gate quantitative.

Le second résultat est un théorème terminal nouveau et simple, corrigé par le
contre-audit : pour `smax=11` et un support connu d'arité `q`, les `12-q` vrais
plus proches voisins **hors support** certifient la branche. Le top-12 global
reste un certificateur sûr, jamais un générateur, mais n'est pas minimal.

Enfin, la piste séduisante « toute face de première génération d'un niveau
shallow est une source critique » est fausse pour q2 et q3. Les mosaïques
d'ordre supérieur restent des oracles; elles ne doivent pas être réintroduites
comme architecture produit. Une shallow cutting à conflits complets reste une
piste de recherche, prioritairement pour q4.

## 2. Le bon objet relevé : un minimum critique, pas toute face shallow

Pour un site `x`, poser le score affine relevé
`ell_x(c)=2<x,c>-||x||^2`. La distance vérifie
`||x-c||^2=||c||^2-ell_x(c)`. Pour un support affinement indépendant `U`, les
équations `lambda=ell_u(c)`, `u in U`, définissent son flat d'égalité relevé.
Sur ce flat, poser `Phi(c,lambda)=||c||^2-lambda`.

### Théorème du minimum auto-centré

La restriction de `Phi` au flat d'égalité de `U` possède un unique minimum :
le centre intrinsèque `c_U` de `U`, de valeur `beta_U`. Le support est positif
exactement lorsque `c_U` appartient à l'intérieur relatif de `conv(U)`. À ce
minimum, un site `y` est strictement intérieur à la boule exactement lorsque
`ell_y(c_U)>lambda_U`.

La preuve est directe. Le lieu des centres équidistants à `U` est l'espace
affine orthogonal à `aff(U)` passant par son circumcentre intrinsèque. La
distance carrée commune y est la somme de `beta_U` et de la distance carrée au
circumcentre; elle admet donc ce minimum unique. Le critère d'intérieur est
l'identité de distance ci-dessus. La positivité est le critère classique de
support minimal : tous les barycentriques du centre sont strictement positifs.

Ainsi une source de taille `q` et de profondeur `p` est un minimum auto-centré
de dimension de définition `q`, avec `p<=11-q`. Ce n'est pas une face shallow
arbitraire.

### Contre-exemple permanent q2/q3

Prendre `U={(-1,0,0),(1,0,0)}` et `y=(0,2,0)`. La sphère centrée en `y`, de
rayon carré cinq, passe par `U` et porte `y` strictement à l'intérieur; elle
réalise donc une tranche de première génération à profondeur un. Mais la
miniboule de `U` est centrée en zéro, de rayon carré un, et `y` est extérieur.
La tranche shallow ne donne ni le centre critique ni son census.

Pour q2, le lieu d'égalité est un plan; pour q3, une droite. Il faut encore
minimiser `Phi` sur ce lieu et vérifier que ce minimum appartient à la cellule
de signes considérée. Pour un q4 affine-3, le lieu est de dimension zéro : un
sommet de quatre hyperplans shallow donne directement le centre, sous réserve
de positivité et des égalités exactes. Cette différence justifie une
expérience de cutting q4 séparée, pas une bijection générale.

## 3. Théorème top-`(12-q)` hors support

Soit `U` un support proposé de cardinal `q`, avec centre exact `c`, rayon carré
`beta` et `p` points strictement intérieurs. Poser `t=12-q`. Une primitive
renvoie les `t` vrais plus proches `PointId` de `X minus U`, les ex aequo pouvant
être choisis arbitrairement, et certifie que leur distance maximale `delta` ne
dépasse aucune distance omise. Si moins de `t` identifiants restent, elle
scanne tout `X minus U`.

### Théorème

- Si `delta>beta`, tous les points hors `U` de la boule fermée sont retournés.
  Le census intérieur `I` et l'extra-shell `H=E minus U` sont globaux et
  complets. Le fast path publie seulement si `H` est vide, donc `E=U`.
- Si `delta<beta`, les `t` retours sont intérieurs. Alors `p>=12-q`, donc
  `p+q>=12`; le support est hors fenêtre.
- Si `delta=beta`, tous les intérieurs sont retournés et au moins un retour est
  un contact hors `U`. De plus `p<=t-1`, donc `p+q<=11`; la boule pertinente
  rejoint le range-report, le quotient de plateau ou un refus fermé.

Le choix arbitraire dans un tie ne fragilise pas la preuve : un intérieur omis
aurait une distance strictement inférieure à `delta`. L'exclusion se fait par
`PointId`, jamais par coordonnées; deux identifiants colocalisés restent deux
sites distincts.

La profondeur est minimale parmi les sentinelles fixes qui connaissent `U`.
Top-`(t-1)` ne distingue pas les mêmes premiers retours d'une boule régulière
avec `p=t-1`, de la même boule avec un `t`-ième intérieur, ni de la même boule
avec un contact extra-shell. Le top-12 global reste sûr, mais son ancien claim
de minimalité est faux.

### Portée exacte

Le théorème accepte ou rejette exactement la branche `RelevantGP`. Il ne
publie pas le cardinal complet d'un plateau lorsque `delta=beta`. Si le contrat
diagnostique exige tout le shell ou son cardinal exact, cette branche lance un
range-report d'égalité ou retourne `unsupported_degeneracy`; elle ne tronque
jamais le plateau.

Avec un centre rationnel `c=C/D`, toutes les comparaisons emploient
`||D*x-C||^2`. Un LBVH élague seulement sur une borne AABB entière certifiée;
toute ambiguïté ou tout overflow descend ou emploie le repli multiprécision.
La requête doit publier visites moyenne/p95/max, replis exacts et requêtes
lourdes. Le théorème ne garantit pas `O(log n)` au pire.

## 4. Premier RLE avant toute géométrie

Chaque lane émet seulement la clé triée de ses deux, trois ou quatre
`PointId`. À `n<=65 536`, quatre identifiants tiennent dans 64 bits; l'ABI
industrielle garde néanmoins des identifiants 32 bits et sépare les lanes, soit
au plus seize octets de clé q4. Un `CellId` diagnostic porte le record à vingt
octets; il n'est pas nécessaire à la décision finale.

### Lemme de rejeu owner sans transporter tous les contextes

Supposer la complétude déjà prouvée du producteur cellulaire et une partition
half-open commune à la lane. Après RLE, calculer une seule fois `c_U`, descendre
directement ce centre dans l'arbre et retrouver sa feuille owner `C`. Rejouer
alors `U subset D_{11-q}(C)`, les filtres de la lane et les seuils du pool
persisté.

Si `U` est pertinent, la preuve de complétude garantit son émission dans cette
feuille et le rejeu réussit. Si une clé large a été proposée seulement dans une
autre cellule, le rejeu owner la rejette. Une feuille absente, plusieurs
feuilles ou un membre manquant sont des échecs d'invariant. Le RLE n'a donc pas
besoin de copier tous les `CensusContext`; il a besoin de la table transitoire
des feuilles, pools et seuils permettant ce rejeu.

Ce lemme supprime la cause mesurée : le lift et la positivité ne sont plus
payés avant chaque rejet owner. Il ne réduit pas encore le nombre de clés brutes
émises. Le ledger obligatoire est donc :

- occurrences `SupportKey` avant RLE;
- clés uniques par arité;
- multiplicité p50/p95/max et nombre de lots traversés;
- octets `count/fill/radix/temp` et high-water;
- clés sans owner, à plusieurs owners et rejetées au rejeu;
- géométries physiques q2/q3/q4 après RLE.

### Lots et shards

Deux mises en mémoire sont exactes.

1. Un lot spatial contient des feuilles entières, fait un RLE local et paie une
   géométrie par `(SupportKey,lot)`. Seul le lot de la feuille owner publie.
2. Un shard déterministe par bits de `SupportKey` réunit toutes les occurrences
   égales et paie une géométrie globale par support. Il exige un
   `count/scan/fill` 64 bits, puis traite les shards sous une capacité annoncée.

Dans les deux cas, les survivants doivent ensuite être redistribués par
`OwnerCellId` et par clé géométrique exacte de sphère. Deux supports distincts
de la même boule peuvent appartenir à deux shards `SupportKey`. La clé chaude
est l'équation primitive centre/rayon, pas `U_B`, qui n'est connu qu'après le
census. Toute collision de hash est résolue par comparaison rationnelle exacte.

## 5. Fast path régulier et side queue par boule

Après géométrie, owner et positivité, deux ordonnances restent exactes. La
première forme une `GeometricBallKey`, fait un second RLE, puis lance une seule
requête par boule si son producteur n'a pas déjà livré le census. Elle choisit
un support canonique `U_star` d'arité minimale `q_min` et interroge
top-`(12-q_min)` dans `X minus U_star`. Choisir `q_max` peut rejeter une boule
encore pertinente par son support minimal; exclure l'union des supports peut
masquer un contact. La seconde ordonnance emploie le census producteur ou
top-`(12-q)` directement par `SupportKey_unique`. Elle évite un tri large dans
le chemin régulier.

Elle est exacte sous `RelevantGP`. Si `delta>beta`, la sentinelle donne le shell global
`E`. L'acceptation exige `E=U`. Un autre support minimal `V` de la même boule
serait inclus dans `E=U`; comme `U` est affinement indépendant et son centre est
dans `relint conv(U)`, aucun sous-ensemble propre de `U` ne peut porter ce même
centre. Donc `V=U`. Le record régulier peut être publié immédiatement. Si
`H=E minus U` n'est pas vide, y compris avec `delta>beta`, la boule rejoint la
side queue pour range-report, quotient de plateau ou refus fermé. Le cas
`delta=beta` prouve nécessairement un tel contact. Aucun hash ne décide cette
branche.

Le second RLE global reste utile si les boules multi-supports sont assez
nombreuses pour amortir son trafic, ou si le contrat exige leur catalogue
complet. Il n'est plus une obligation du fast path régulier. Les deux variantes
doivent être comparées sur les requêtes par arité, `BallKey` uniques, octets radix et
side-queue high-water.

Le census pool-relatif déjà prouvé reste un backend valide. La sentinelle hors
support offre une sortie de taille fixe, mais pas un nombre fixe de visites
LBVH; il faut comparer sur device `census producteur`, `scan du pool owner` et
`top-(12-q) LBVH`.
Le reçu CPU montre précisément que le census aval n'est pas le verrou actuel :
le théorème n'autorise donc pas à retarder le premier RLE.

## 6. Layout G4 pour vingt-quatre millions de supports

Le type [`g4-standard-48` documenté par Google](https://docs.cloud.google.com/compute/docs/accelerator-optimized-machines)
porte une RTX PRO 6000 Blackwell Server Edition et 96 Go de mémoire GDDR7. Cela
borne la capacité physique; le high-water complet du pipeline reste à recevoir.

Le reçu `uniform,n=50 000` permet un dimensionnement plus direct. Les
occurrences q2/q3/q4 avant le lift utile valent respectivement
`96 241 855 / 352 786 093 / 390 554 718`. Avec des lanes séparées, q2 se
stocke en `u32`, q3 dans les 48 bits utiles d'un `u64` et q4 dans un `u64`
uniquement si chaque identifiant chaud est un `DensePointIndex:u16`. Une
bijection immuable liée à `cloud_epoch` doit le relier aux `PointId` durables;
l'ordre canonique se décide après remap. Le stream brut complet occupe alors
environ `6,33 Go`, sans `CellId`, table de remap, listes, sorties ni workspace.
Un double buffer radix occupe `12,66 Go`, toujours hors workspace. Dans un modèle à digits de huit bits,
quatre, six et huit passes avec lecture plus écriture déplacent environ
`86,94 Go`. C'est un modèle de trafic, pas une mesure CUB; il montre néanmoins
que ces seules clés tiennent en mémoire sur la G4. Le débit des passes, le
nombre de clés uniques et le high-water de toutes les autres arènes restent des
verrous.

Pour `F=24 017 000` :

| arène | ordre de grandeur |
| --- | ---: |
| `SupportKey` 16 octets | `384 Mo` |
| clé + `CellId` 20 octets | `480 Mo` |
| intérieurs, pire favorable neuf `u32` | `865 Mo` |
| offsets CSR `u64`, `F+1` | `192 Mo` |
| double buffer de records 32 octets | `1,54 Go` |
| double buffer de records 48 octets | `2,31 Go` |
| niveau/clé exacte 32 à 48 octets | `0,77` à `1,15 Go` |

Un pic de trois à six gigaoctets pour les seuls records utiles, hors listes de
conflits et espace temporaire du radix, est plausible. Le double buffer des
occurrences brutes porte le plancher mesuré plus près de treize gigaoctets. Ces
calculs prouvent seulement que **24 millions de tâches utiles et le stream
compact observé tiennent**. Ils ne prouvent aucun débit. Ajouter un `CellId` à
chaque occurrence ferait inutilement exploser ce flux; la point-location owner
après RLE est précisément ce qui autorise les clés nues.

Le plan de kernels est :

1. points u16 SoA, LBVH et arbre terminal résidents;
2. front/enveloppe par arité avec owner génératif, ou wavefront cellulaire de
   référence et `count/scan/fill` de clés seulement;
3. vérification `occurrences=unique` ou radix/RLE global;
4. une lane pour q2 et sous-groupes/warps pour les déterminants q3/q4; après le
   tri q4, grouper le préfixe `(a,b,c)` et construire une seule fois son axe
   circumcentrique, puis résoudre chaque apex `d` par une intersection scalaire
   exacte avec le bissecteur `a/d`;
5. point-location owner et rejeu contigu du pool;
6. census reçu du producteur; file persistante top-10/top-9/top-8 seulement en
   fallback exact;
7. fast path `delta>beta` avec `E=U`, et radix `GeometricBallKey` pour toute
   extra-shell, tout plateau ou lorsque l'A/B reçoit le second RLE global;
8. flux immédiat des activations vers gateway/fold, sans catalogue hôte;
9. copie hôte du seul payload officiel.

Chaque phase possède compte, capacité, high-water, temps CUDA events et statut
terminal. Une capacité insuffisante rend `resource_exhausted`; elle ne publie
jamais un préfixe.

## 7. Piste de recherche : shallow cutting critique

La caractérisation du minimum suggère une source plus directe. Dans l'espace
relevé `(c,lambda)`, construire une shallow cutting dont chaque cellule porte :

- une profondeur de base exacte;
- la liste **complète** des hyperplans qui la coupent;
- un certificat de signe fixe pour chaque site omis;
- une couverture half-open exacte et un owner canonique.

Si la liste de conflits contient `m` sites sous un cap, énumérer séparément les
q2, q3 et q4 locaux, calculer leur minimum `Phi`, vérifier positivité, owner et
sentinelle hors support si le census n'est pas déjà reçu. Sinon,
subdiviser/rééchantillonner ou échouer sur ressource. Le hasard
peut modifier le travail, jamais la couverture.

Le travail à publier est
`W_cut+sum_C [C(m_C,2)+C(m_C,3)+C(m_C,4)]+W_cert`. Aucun théorème universel ne
le borne par la sortie. Les premiers niveaux en dimension relevée peuvent être
quadratiques, et la famille u16 à deux droites du dépôt sépare déjà des
milliards de transits d'une Source S linéaire.

Une RIC « sorties acceptées seulement » est incomplète : un q3 pertinent peut
n'avoir aucune paire q2 pertinente, et un q4 pertinent aucune face q3
pertinente. Les flats auxiliaires ou les listes de conflits complètes sont
indispensables. La cutting est donc une branche de recherche q4 prioritaire,
pas un remplacement reçu du premier RLE.

Une seconde branche de recherche est le
[`front canonique de Jung`](AUDIT_VERROU_MATHEMATIQUE_FRONT_JUNG_H0_GPU_20260812.md).
La couverture déterministe est correcte : toute ancre maximale d'un q3/q4
pertinent survit les seuils de témoins universels. Sous Poisson bulk, la
coalescence des lanes imbriquées prédit environ `141,183365 n` paires physiques,
soit `7,06` millions à 50 000 points. Cette petite sortie ne reçoit pas son
producteur : le dual-tree existant a des pentes de visites proches de `2,3`, et
l'extension naïve reste beaucoup plus grosse. Les gates séparées `W_front` et
`W_extend` décident cette branche. Elles ne retardent pas le port du stream
compact `SupportKey`, qui demeure la baseline device exacte.

## 8. Contre-audit de la note de Claude

Les conclusions suivantes sont conservées :

- le transcript gelé contient désormais neuf cas et un footer; `uniform` ferme
  deux pentes de compteurs sous `1,16`, tandis que `terrain` n'a qu'une pente de
  cellules rouge suivie d'une verte;
- cette campagne à trois familles, sans `eight_clusters`, sans digest
  d'identités et sous charge concurrente reste un diagnostic count-only, pas le
  reçu contractuel ni une mesure de latence;
- `uniform,n=50 000` confirme un régime très productif, `21,395` millions de
  supports et l'amplification d'environ `39,24` lifts par support;
- vingt à vingt-quatre millions de supports sont cohérents avec la baseline
  Poisson, sans être une identité pour la boîte u16.

Les corrections obligatoires sont :

1. Une colonne de l'adjugée de la covariance est une normale exacte lorsque la
   covariance est exactement de rang deux. En rang trois, ce n'est généralement
   pas le vecteur propre de plus petite valeur propre. Le code choisit une
   **direction adaptative issue de l'adjugée**; le prune reste sûr parce que le
   test de séparation final est entier pour toute direction.
2. La positivité donne des points de support de projections supérieure et
   inférieure **non strictes** à celle du centre. Une direction orthogonale à
   `aff(U)` donne des égalités. Le code `>=/<=` est correct; le commentaire
   strict est faux.
3. Le prune `rank_cell` est exact sous l'invariant `U subset mine`. Sa
   conclusion se propage parce que la cellule entière est certifiée sans
   support, pas parce qu'une monotonie séparée de `lambda` aurait été prouvée.
4. Le terminal sur stagnation est sémantiquement exact seulement si son
   énumération exhaustive termine. Il peut convertir une cellule dense en
   adjacency quadratique et en `C(m,4)` candidats. Il exige un hard-cap
   d'octets/travail, puis split ou `resource_exhausted`.
5. La saturation de `work` arrive actuellement après les additions signées.
   Pour une clique complète de taille `78 000`, le score dépasse `INT64_MAX` :
   le clamp ne répare pas l'overflow antérieur. Une valeur saturée ne doit pas
   non plus, à elle seule, autoriser un terminal de stagnation.
6. Le binaire gelé antérieur n'est pas une borne supérieure du temps du
   successeur. Les prunes ajoutent un coût et le stall change le parcours; seuls
   des compteurs sous flot comparable peuvent être monotones.
7. Les `wall_s` gelés ont subi la charge concurrente et ne prouvent ni coût par
   support ni facteur G4. L'extrapolation CPU vers environ 400 secondes reste un
   dimensionnement, pas une prédiction de latence.
8. Le mode `--no-normal-separation` calcule encore la covariance avant de sauter
   le test : il compare la sémantique, pas le coût complet de la normale.

Au pin initial de cet audit, le couple `HEAD=b3c8f75...`, source `a240c2f...`,
CMake `70de0e2...` rendait `38/38` CTests centre-cell en `110,79 s`; la sortie
brute observée en `/tmp` portait le SHA-256 `8af0202f...` mais n'était pas
archivée. Le successeur `90c06b0...` ajoute un juge rationnel et rend `48/48`,
mais sa porte mutant passe sur un refus code 2 et non sur un désaccord code 1;
voir
[`AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md`](AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md).
Ces verts bornés n'exercent ni overflow `78 000`, ni mutant `rank_cell`, ni cap
dur du stall, ni HWM du terminal dense. Le pin logiciel courant appartient à
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

## 9. Portes avant CUDA

### Mathématiques

- fixture permanente du faux first-generation q2 ci-dessus;
- q3 pertinent sans q2 pertinent et q4 pertinent sans q3 pertinent;
- top-10/top-9/top-8 hors support : `delta>beta`, `delta<beta`, `delta=beta`,
  ties arbitraires et moins de `12-q` sites hors support;
- mutants top-`(11-q)`, support compté dans le heap, exclusion par coordonnées
  et égalité acceptée directement;
- enveloppe top-9 dans `X minus {a,b}`, avec tous les ex aequo du neuvième
  niveau et mutant qui compte `a/b`;
- `rank_cell` : direction orthogonale, égalité `u=lambda`, cardinal frontière;
- plateau : range-report ou refus atomique, jamais shell tronqué.

### Identités

- comparaison exacte de tous les `(SupportKey,I_B,E_B)` à l'oracle borné;
- identité `census_from_envelope` contre sentinelle et oracle, ledger
  `envelope_certified + knn_fallback + plateau = supports`;
- digest eager contre `SupportKey-RLE` sur les quatre familles;
- owner d'arête maximale et patch half-open, avec
  `occurrences=SupportKey_unique` avant plateaux;
- zéro/multiple owner fail-close;
- même boule depuis supports et shards distincts;
- fermeture des ledgers occurrences, uniques, géométries, boules et supports.

### Travail et mémoire

- `occurrences/SupportKey_unique` et `SupportKey_unique/BallKey_unique`;
- octets et high-water de chaque arène, radix et LBVH;
- visites de sentinelle par arité moyenne/p95/max et exact fallbacks;
- aucun heap, `std::vector`, allocation ou copie hôte par support;
- trois tailles `12 500/25 000/50 000` sur `uniform` et `eight_clusters`;
- seulement après ces portes, session G4 gardée et `warm_e2e` officiel complet.

## 10. Décision proposée à Claude

1. Viser une émission exacte-once par support depuis le front canonique, sans
   réduire ni tronquer le catalogue mathématique requis.
2. Garder `SupportKey` comme premier objet compact et son RLE comme vérificateur
   tant que l'owner génératif n'est pas reçu.
3. Réutiliser le census de l'enveloppe lorsqu'il est certifié; comparer la
   sentinelle top-`(12-q)` hors support comme oracle/fallback au scan
   pool-relatif.
4. Rejouer l'owner par point-location pour la baseline cellulaire au lieu de
   transporter une copie de tous les contextes.
5. Graver les gates exact-once, census et `eight_clusters` avant CUDA; la source
   cellulaire reste le comparateur qui mesure le gain structurel.
6. Ne jamais appeler les faces shallow q2/q3 des sources sans le test du
   minimum `Phi`.
7. Ne lancer un benchmark G4 de latence qu'avec ce pipeline plat, ses capacités
   préflightées et le payload officiel branché.

GCP non utilisé.
