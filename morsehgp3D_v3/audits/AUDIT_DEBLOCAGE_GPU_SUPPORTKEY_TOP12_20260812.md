# Audit de déblocage GPU — dédupliquer avant la géométrie, certifier par top-12

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

Le nombre attendu de supports n'est pas le verrou observé. Le premier point
volumique gelé, `uniform,n=12 500`, publie `4 990 227` supports mais construit
`194 463 795` géométries, soit `38,969` géométries par support; `81,778 %` de
ces propositions finissent rejetées par l'owner. Sur `terrain,n=50 000`, les
ratios valent `127,688` géométries par support et `93,387 %` de rejets owner.
L'ordonnance CPU paie donc principalement **avant** de connaître la tâche utile.

La voie immédiate recommandée est :

```text
cellules/lane q -> count/scan/fill SupportKey
                 -> radix shardé + RLE SupportKey
                 -> une géométrie exacte par clé
                 -> point-location directe de la feuille owner
                 -> positivité + rejeu du contexte owner
                 -> GeometricBallKey + second RLE
                 -> une certification top-12 exacte par boule
                 -> flux device vers activation/gateway/fold
```

Ce pipeline ne promet pas encore la seconde. Il change toutefois le bon ordre
de grandeur : la géométrie, l'owner et le census sont indexés par clés uniques,
pas par occurrences spatiales.

Le second résultat est un théorème terminal nouveau et simple : pour
`smax=11`, douze vrais plus proches voisins certifient exactement une boule
candidate régulière, son intérieur et l'absence d'extra-shell. Le top-12 est un
**certificateur**, jamais un générateur.

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

## 3. Théorème top-12

Soit `U` un support proposé de cardinal `q`, avec centre exact `c`, rayon carré
`beta` et `p` points strictement intérieurs. Pour `n>=12`, une primitive renvoie
douze `PointId` distincts formant un vrai ensemble de douze plus proches
voisins `R`, les ex aequo pouvant être choisis arbitrairement, et certifie que
la distance maximale retournée ne dépasse aucune distance non retournée. Noter
`delta` cette distance maximale.

### Théorème

- Si `delta>beta`, toute la boule fermée est contenue dans `R`. Le census
  `I={x:d(x,c)^2<beta}` et le shell `E={x:d(x,c)^2=beta}` obtenus dans `R` sont
  globaux et complets.
- Si `delta<beta`, les douze points retournés sont strictement intérieurs. Le
  support est hors fenêtre `p+q<=11`.
- Si `delta=beta`, tous les points strictement intérieurs sont dans `R`, donc
  `p` est exact. Si `p+q<=11`, les douze points fermés de `R` ne peuvent pas
  tous appartenir à `I union U`, de cardinal au plus onze : une extra-shell est
  prouvée. La branche régulière échoue fermée sans range-report.
- Pour `n<12`, la primitive retourne tout le nuage.

Le choix arbitraire dans un tie ne fragilise pas la preuve. Dans le troisième
cas, un intérieur omis aurait une distance strictement inférieure à `delta`, en
contradiction avec le certificat des douze voisins. Certains membres de `U`
peuvent être omis au cutoff, mais cela renforce seulement l'existence d'un
contact hors de `I union U`.

Un top-11 ne suffit pas. Une boule régulière de rang fermé onze et la même
boule après ajout d'un douzième point cosphérique peuvent retourner exactement
les mêmes onze identifiants. Le top-12 est donc la sentinelle fixe minimale
pour `smax=11`.

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

## 5. Deuxième RLE et certification par boule

Après géométrie, owner et positivité, former une `GeometricBallKey` primitive
et faire un second RLE. Une requête top-12 est alors exécutée par boule, pas par
occurrence. Tous les supports du run reçoivent le même `I_B/E_B`; le budget
`p+q<=11` reste testé par support.

Sous `RelevantGP`, presque toute boule Poisson porte un seul support minimal :
ce second RLE ne promet donc pas une forte compression. Sa fonction est
l'exactitude, la mutualisation des rares multi-supports et la gestion atomique
des plateaux. Le premier RLE porte le gain principal mesuré.

Le census pool-relatif déjà prouvé reste un backend valide. Le top-12 offre un
contrat alternatif à travail fixe et indépendant de la longueur d'une liste de
cellule; il faudra comparer sur device `scan du pool owner` et `top-12 LBVH`.
Le reçu CPU montre précisément que le census aval n'est pas le verrou actuel :
le théorème n'autorise donc pas à retarder le premier RLE.

## 6. Layout G4 pour vingt-quatre millions de supports

Le type [`g4-standard-48` documenté par Google](https://docs.cloud.google.com/compute/docs/accelerator-optimized-machines)
porte une RTX PRO 6000 Blackwell Server Edition et 96 Go de mémoire GDDR7. La
capacité mémoire n'est donc pas le premier obstacle du catalogue transitoire, à
condition de rester device-only et SoA.

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

Un pic de trois à six gigaoctets, hors listes de conflits et espace temporaire
du radix, est plausible. Ce calcul prouve seulement que **24 millions de
tâches utiles tiennent**. Il ne prouve aucun débit. À multiplicité brute 39
transférée telle quelle, environ `936 millions` d'occurrences exigeraient déjà
`18,7 Go` à vingt octets avant double-buffer; cette amplification doit être
mesurée et, si nécessaire, réduite par RLE local puis sharding global.

Le plan de kernels est :

1. points u16 SoA, LBVH et arbre terminal résidents;
2. wavefront de cellules par arité, `count/scan/fill` de clés seulement;
3. RLE local facultatif, histogramme de shards puis radix/RLE global;
4. une lane pour q2 et sous-groupes/warps pour les déterminants q3/q4;
5. point-location owner et rejeu contigu du pool;
6. radix par `GeometricBallKey`;
7. file persistante de requêtes top-12, exact fallback séparé;
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
top-12. Sinon, subdiviser/rééchantillonner ou échouer sur ressource. Le hasard
peut modifier le travail, jamais la couverture.

Le travail à publier est
`W_cut+sum_C [C(m_C,2)+C(m_C,3)+C(m_C,4)]+W_top12`. Aucun théorème universel ne
le borne par la sortie. Les premiers niveaux en dimension relevée peuvent être
quadratiques, et la famille u16 à deux droites du dépôt sépare déjà des
milliards de transits d'une Source S linéaire.

Une RIC « sorties acceptées seulement » est incomplète : un q3 pertinent peut
n'avoir aucune paire q2 pertinente, et un q4 pertinent aucune face q3
pertinente. Les flats auxiliaires ou les listes de conflits complètes sont
indispensables. La cutting est donc une branche de recherche q4 prioritaire,
pas un remplacement reçu du premier RLE.

## 8. Contre-audit de la note de Claude

Les conclusions suivantes sont conservées :

- la rampe locale uniforme annonce des pentes de compteurs sous `1,35`, mais
  n'est pas un reçu tant que ses quatre points ne sont pas pincés;
- le reçu gelé ferme bien `terrain` aux trois tailles et sa seconde pente de
  cellules est verte; une seule pente rouge ne déclenche pas le NO-GO défini;
- `uniform,n=12 500` confirme un régime très productif et l'amplification
  d'environ 39 lifts par support;
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
- top-12 : `delta>beta`, `delta<beta`, `delta=beta`, ties arbitraires, `n<12`;
- top-11 mutant qui accepte le douzième contact;
- `rank_cell` : direction orthogonale, égalité `u=lambda`, cardinal frontière;
- plateau : range-report ou refus atomique, jamais shell tronqué.

### Identités

- comparaison exacte de tous les `(SupportKey,I_B,E_B)` à l'oracle borné;
- digest eager contre `SupportKey-RLE` sur les quatre familles;
- zéro/multiple owner fail-close;
- même boule depuis supports et shards distincts;
- fermeture des ledgers occurrences, uniques, géométries, boules et supports.

### Travail et mémoire

- `occurrences/SupportKey_unique` et `SupportKey_unique/BallKey_unique`;
- octets et high-water de chaque arène, radix et LBVH;
- visites top-12 moyenne/p95/max et exact fallbacks;
- aucun heap, `std::vector`, allocation ou copie hôte par support;
- trois tailles `12 500/25 000/50 000` sur `uniform` et `eight_clusters`;
- seulement après ces portes, session G4 gardée et `warm_e2e` officiel complet.

## 10. Décision proposée à Claude

1. Ne pas chercher à réduire les vingt-quatre millions de sorties régulières.
2. Faire de `SupportKey` le premier objet CUDA et déplacer tout lift après son
   RLE.
3. Rejouer l'owner par point-location au lieu de transporter une copie de tous
   les contextes.
4. Ajouter le top-12 comme certificateur terminal commun et comparer son coût
   au scan pool-relatif.
5. Garder la source cellulaire comme producteur de clés tant que la cutting
   critique n'est pas reçue.
6. Expérimenter la cutting d'abord sur q4; ne jamais appeler les faces shallow
   q2/q3 des sources sans le test du minimum `Phi`.
7. Ne lancer un benchmark G4 de latence qu'avec ce pipeline plat, ses capacités
   préflightées et le payload officiel branché.

GCP non utilisé.
