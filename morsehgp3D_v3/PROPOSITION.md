# MorseHGP3D v3 — proposition d'architecture courante

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette proposition n'est ni une spécification ni une qualification produit.
L'autorité reste
[`SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md) et le
registre
[`STATUT_PREUVES_ET_HEURISTIQUES.md`](../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md).

## 1. Décision

La route candidate est :

```text
k=1 : EMST exact distinct

k>=2 : self-join LBVH des paires
  -> center-cover fail-open par blocs
  -> ancres diamètre résiduelles
  -> cordes de Jung et niveaux peu profonds
  -> census terminal et BallActivation streamées
  -> tombstones H0, resolver, fold sparse par lots
```

Elle ne construit ni mosaïque de Delaunay d'ordre supérieur, ni Gamma global,
ni matrice paire--point, ni tableau global de tuples, de facettes ou de
cofaces. Les arrangements bidimensionnels sont des scratchs éphémères par
ancre ou par slab.

Cette route reste conditionnelle : la parcimonie globale de ses ancres et de
ses cordes n'est pas prouvée. Le prochain jalon est un falsificateur mass-only,
pas l'implémentation anticipée d'un produit.

## 2. Décisions négatives

Les chemins suivants ne sont plus proposés comme architectures produit :

- catalogue exhaustif `flat_catalogue`, borné en rang et déjà à 675 secondes
  pour n=6 250 sur une famille mesurée;
- cellules uniformes suivies de toutes les combinaisons locales;
- pinceaux q4 qui commencent par tous les triples locaux;
- scan de tout le nuage pour chaque ancre ou chaque triple;
- RNG, kNN fixe ou cascade de Jung d'un nombre fixé d'étages comme autorité de
  complétude;
- onion peeling des formes duales, réfuté par une fixture Morse bien centrée;
- MST de points comme substitut aux ordres `k>=2`;
- face-owner qui matérialise toutes les incidences;
- factory sidecar qui recertifie chaque générateur par un scan `O(G*n)` dans le
  chemin chaud.

La source par cellules et le plan séparateur restent utiles comme oracles,
falsificateurs de masse et sources bornées. Ils ne reprennent le statut de
candidat que si leurs préflights deviennent compatibles avec une enveloppe
mesurée.

## 3. Contrat de sortie

La proposition vise le contrat distinct
`hgp_reduced_normalized_h0_v3` : composantes horizontales exactes, niveaux
exacts, lots atomiques et union exacte des `PointId`, avec quotient certifié de
blocs H0 silencieux.

Elle ne promet pas le transcript Gamma exhaustif. Ce dernier exige aussi les
facettes, cofaces, incidences silencieuses, `coverage_delta`, identifiants de
lots et applications verticales. Une boule silencieuse pour H0 peut porter ces
données; une tombstone horizontale ne les supprime donc jamais d'un contrat
Gamma.

## 4. Invariants

1. Toute décision d'émission, d'omission, d'owner ou de niveau est exacte.
   Le flottant ou le GPU peut proposer; un certificat entier, rationnel ou
   multiprécision décide.
2. Une ambiguïté, une égalité non traitée ou un overflow conserve le travail ou
   déclenche le repli exact.
3. Tout préflight est calculé avant allocation. Une ressource insuffisante
   refuse ou produit un token de reprise; elle ne tronque rien.
4. Chaque niveau exact est transactionnel : snapshot strict, calcul complet,
   contrôles d'identité, puis commit fermé unique.
5. Count et fill ont la même partition, le même owner et la même masse.
6. La randomisation peut modifier le temps, jamais la sortie. Sa graine et son
   transcript sont scellés et rejouables.
7. Les oracles exhaustifs restent bornés et structurellement indépendants du
   chemin produit.

## 5. Pont H0 de haut rang

Pour une boule fermée `B`, soit `p` le nombre de points strictement intérieurs
et soit `q` la taille d'un support propre positif. Le théorème 4.2 de
[`INCIDENCES_SILENCIEUSES_GAMMA.md`](../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md)
donne :

$$1leq kleq p+q-2Longrightarrow B	ext{ est une continuation }H_0	ext{ sans fusion ni nouveau PointId.}$$

Pour `K=10`, une preuve positive `p+q>=12` rend la boule inerte pour tous les
ordres horizontaux demandés. Les nombres de témoins stricts qui suffisent à
écarter une lane pertinente sont donc `10/9/8` pour q2/q3/q4.

Le support doit être affinement indépendant et son centre dans l'intérieur
relatif de son enveloppe convexe. Un carré cosphérique redondant n'est pas un
support propre q4. `q_min` est la plus petite arité propre d'une source complète;
`q_cert` est la plus grande arité positive effectivement prouvée et sert à
renforcer l'inertie. Une absence de grand support n'est jamais inférée.

## 6. Lane `k=1`

`k=1` est exactement le single-linkage aux coupes stricte et fermée. La lane
produit doit construire un EMST exact puis rejouer ses lots de distances
égales. Le Prim `O(n^2)` actuel reste un oracle.

La candidate device est un Borůvka sur LBVH : proposition de plus proche arête
par composante, comparaison exacte des distances carrées u16, tie-break par
`PointId`, union atomique par lot égal et comparaison des partitions à l'oracle.
Ce composant doit avoir ses propres reçus; il ne passe pas par le fallback des
générateurs q3/q4.

## 7. Source d'ancres par self-join LBVH

### 7.1 Partition de toutes les paires

Pour un nœud binaire `N` de fils `L,R`, les paires internes se décomposent en
trois ensembles disjoints : paires internes à `L`, produit croisé `L x R`,
paires internes à `R`. Un produit croisé est ensuite divisé sur un seul côté
selon une règle déterministe. Chaque paire non ordonnée appartient ainsi à un
unique état ou microtuile.

Pour chaque lane, le ledger doit fermer :

$$P_{mathrm{prune}}+P_{mathrm{microtile}}=inom{n}{2}.$$

Après consommation terminale, aucun état `pending` ne subsiste. Les plages de
feuilles des témoins sont disjointes de celles des deux extrémités.

### 7.2 Prune q2 par blocs

Pour une paire `(x,y)`, un témoin `w` est strictement intérieur à sa boule
diamétrale exactement lorsque :

$$(w-x)mathbin{cdot}(w-y)<0.$$

Une borne d'intervalle strictement négative pour dix témoins distincts et pour
toutes les paires d'un bloc certifie l'inertie q2 de ce bloc. Zéro signifie
shell et ne compte pas. Un intervalle traversant zéro conserve le bloc.

### 7.3 Center-cover q3/q4

Un état de paires représente aussi un sur-ensemble extérieur des centres de
tous les supports dont l'une de ces paires est diamétrale. Ce domaine est
découpé en patches fermés. Un patch n'est retiré que si l'équation du plan
médiateur ou la borne de Jung y est impossible par intervalles exacts.

Dans chaque patch encore faisable, un parcours témoin du LBVH construit une
antichaîne de plages disjointes :

- une borne strictement positive sur la puissance compte tout le nœud comme
  intérieur;
- une borne négative ou nulle l'exclut des témoins stricts;
- sinon le parcours descend.

Neuf témoins q3 ou huit témoins q4 dans chaque patch faisable ferment la lane du
bloc. Les mêmes points peuvent servir dans des patches différents; ils ne
peuvent être comptés deux fois dans une même antichaîne.

Le contrat exact et ses bornes sont dans
[`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md).
Le RNG peut ordonner la file, jamais fermer un bloc.

### 7.4 Porte de parcimonie

Le mass-only publie au minimum : états de paires, patches faisables, visites
témoin, paires prunées, microtuiles, ancres résiduelles `a`, files p50/p95/p99
et max, ambiguïtés, profondeur de reprise, octets et temps chaud.

Le précédent prototype `center-cover`, qui bouclait trop largement, a dépassé
600 secondes à 50 k sans JSON. Il est rejeté. La nouvelle porte doit éviter
toute boucle ancre--nuage et toute insertion des `n-1` plans pour chacune des
`n` ancres.

## 8. Réduction exacte par ancre diamètre

### 8.1 Coordonnées du disque de Jung

Fixer une paire diamètre `e=pq`, poser `d=q-p`, `D^2=d dot d` et
`M=(p+q)/2`. Choisir deux vecteurs entiers indépendants `b1,b2` orthogonaux à
`d`, poser `B=[b1 b2]` et écrire le centre `c=M+Bt`.

Pour q4, Jung impose le disque elliptique exact :

$$J_e^{(4)}=leftlbrace t:t^{mathsf{T}}(B^{mathsf{T}}B)tleqrac{D^2}{8}ightbrace.$$

Chaque point `x` distinct de l'ancre définit la forme affine :

$$h_x(t)=2(Bt)mathbin{cdot}(x-M)-left(leftVert x-MightVert^2-rac{D^2}{4}ight).$$

L'identité exacte est `h_x(t)=r^2-dist2(x,c)`. Son signe classe intérieur
strict, shell et extérieur. Aucune base orthonormale ni racine carrée n'est
requise.

Un range-report LBVH fournit seulement les points susceptibles d'être
intérieurs ou sur le shell pour un centre de `J_e`. Il doit employer la borne
extérieure de Jung et rester fail-open à l'égalité; scanner les `n` points par
ancre est interdit.

### 8.2 q2 et q3

- q2 évalue directement la boule diamétrale, puis le census et l'owner.
- q3 traite chaque forme dont la droite rencontre le domaine q3. Le
  circumcentre du triangle est le point de cette droite qui minimise la forme
  quadratique du rayon. Bon centrage, profondeur, shell et diamètre sont
  vérifiés exactement.

Les domaines, constantes intérieures et seuils q3/q4 sont distincts. Une passe
q4 ne certifie pas q3.

### 8.3 q4 comme niveau peu profond

Sur `J_e`, classifier chaque point en intérieur constant, extérieur constant ou
corde active. Soit `c_e` le nombre d'intérieurs constants et `m_e` le nombre de
cordes. Une intersection de deux cordes a pour profondeur stricte le nombre de
demi-plans actifs qui la contiennent. Son rang fermé vaut :

$$mathrm{rang}_{mathrm{ferme}}=4+c_e+delta_e.$$

À `smax=11`, la profondeur utile maximale est `kappa_e=7-c_e`. Le nombre de
sommets utiles vérifie :

$$Z_eleq m_e(kappa_e+1)=m_e(8-c_e)leq8m_e.$$

Cette borne est le cœur de la proposition. Elle n'apporte aucun gain si le code
forme d'abord les `C(m_e,2)` intersections. Le constructeur doit bâtir
directement le sous-complexe des niveaux `0..kappa_e`, par une construction
incrémentale Las Vegas avec listes de conflits ou une méthode de même
complexité sortie-sensible.

La graine est scellée, mais le résultat est canonique. Les comparaisons d'ordre
le long d'une corde utilisent le signe du déterminant homogène `3x3`; sous les
bases équilibrées u16 reçues, ce chirotope tient en `i128`. Parallèles et
concurrences suivent des branches exactes, sans perturbation symbolique.

### 8.4 Dégénérescences et owner

Une concurrence de `t` cordes est un seul niveau avec shell multiple, pas
`C(t,2)` sommets. Une ligne inéligible comme carrier peut rester un témoin de
profondeur et ne doit pas être supprimée du comptage.

Une même boule peut avoir plusieurs supports positifs et plusieurs paires de
diamètre. L'ordre sûr est : census fermé, regroupement par `BallKey`, choix du
support positif canonique, puis paire de longueur maximale et plus petite paire
lexicographique en cas d'égalité. Un owner appliqué indépendamment aux supports
bruts peut dupliquer ou perdre une boule.

## 9. Pourquoi la route cellules--triples est rétrogradée

La séparation entre une cellule `C` et `conv(A_C)` exclut seulement les
supports de la branche `beta<Q` contenus dans `A_C`. Un support `beta>=Q` peut
subsister hors de cette liste; il est omissible pour le quotient H0 seulement
avec les témoins stricts, le théorème d'inertie et le resolver. Il n'existe
aucun verdict Gamma `no_support`.

Après le prune d'axe à 50 k, une cellule q4 survivante de taille `m_C` porte
`C(m_C,4)` quadruplets. Même si le pinceau visite seulement les trois plus
petits identifiants de chaque quadruplet, il doit visiter `C(m_C-1,3)` triples.
Ainsi :

$$P_{mathrm{triple}}geqrac{4R'_4}{m_{max}}.$$

Les trois familles au pas 6 imposent respectivement plus de `2,74e9`,
`1,063e10` et `1,020e9` triples. Le plan du barycentre certifié réduit encore
les masses sur de petits cas, mais laisse plus d'un milliard de quadruplets q4
à n=2 400 sur chacune des trois mesures locales. GJK, plans généraux et
anisotropie restent donc des sondes; aucun reporter q4 n'est codé avant
admission de son préflight.

## 10. Census terminal et `BallActivation`

Toute proposition survivante termine par :

1. indépendance affine et barycentriques strictement positives;
2. niveau et centre exacts, sans projection binary64;
3. census fermé complet par range-report exact, puis vérification des feuilles
   ambiguës;
4. shell complet, intérieur strict, rang et `BallKey` canonique;
5. RLE entre lanes et ancres;
6. reconstruction de `q_min`, preuves positives de `q_cert` et certificat
   principal;
7. émission ou tombstone.

Le relèvement en dimension quatre transforme le census point--sphère en requête
de demi-espace; l'index propose des nœuds, tandis que l'arithmétique exacte
rejoue toute frontière.

L'objet streamé est :

```text
BallActivation
  BallKey et niveau exacts
  p, shell complet et digest de census
  q_min et preuves de q_cert
  état principal et témoins de suppression
  fenêtre d'ordres pertinente ou tombstone H0
  handles stricts/fermés latents
  provenance de source et owner
```

La source produit un pass count, une arène bornée, un pass fill et une identité
de masse. Aucun catalogue global de saturés n'est requis.

## 11. Frontière de confiance

La capability de complétude provient de l'achèvement rejouable du self-join et
de ses ledgers, pas de `smax>=n`, d'un booléen ni d'un reçu public.

Le reçu de producteur est inconstructible hors du producteur terminal et lie :

- schéma, epoch, digest des points et paramètres;
- transcript de partition des paires et états de reprise;
- digests canoniques des activations et certificats;
- masses count/fill, owners et RLE;
- statut terminal sans censure.

Le fold consomme `const ValidatedHybridSidecar&` ou une capability équivalente;
il ne reprend jamais `Catalogue + bool`. Les clés et sérialisations sont
canoniques, multiprécision lorsque nécessaire, puis engagées par SHA-256.

Le juge CPU peut refaire un census `O(G*n)` à petit `n`. Cette revalidation
exhaustive ne se trouve pas dans le chemin produit.

## 12. Tombstones et resolver

Une boule H0-inerte doit conserver un locator : une face de son bloc peut être
le carrier d'une fusion ultérieure. Le resolver d'une face `F` avant un cutoff
`a` :

1. calcule sa miniboule `D`, son intérieur et un support propre positif;
2. retourne le handle fermé de `D` si la face est déjà au-delà de sa fenêtre
   d'inertie;
3. sinon remplace `F` par un carrier canonique de niveau strictement inférieur;
4. répète jusqu'au handle antérieur.

Le lookup est `closed@beta(D)` avec `beta(D)<a`. Le cache porte
`(BallKey,k)->handle`, jamais une racine DSU mutable.

Pour les grandes coquilles, le quotient local peut être construit par
l'arrangement unique des grands cercles et le seuil de demi-sphère `Omega`, au
lieu d'énumérer tous les sous-ensembles. Il reste un fallback de dégénérescence
dont le pire cas quadratique doit être préflighté et comparé au graphe local
exhaustif.

## 13. Fold sparse

Le fast principal est actif seulement si `q<=k+1`, si le support principal et
`CarrierClosure` sont certifiés, et si toutes les attaches se résolvent à un
niveau strictement inférieur dans le snapshot pré-lot. Un lookup égal ou absent
sous prétention complète refuse atomiquement.

Le fallback préfixe emploie un ordre global, des postings possédés par lots, un
préflight exact des hits et la recertification réelle des intersections. Il
reste la vérité relative à toute table fournie.

Pour chaque lot : figer les racines, construire fast et fallback, recertifier,
comparer masses et ledger, appliquer les composantes en un commit, puis publier
les handles fermés. La DSU ne peut pas masquer une incidence manquante dans les
contrôles pré-commit.

## 14. Architecture device

```text
points u16 + LBVH + ordre Morton résidents
  -> file dual-tree de blocs de paires
  -> patches center-cover et antichaînes témoins
  -> microtuiles d'ancres
  -> range-report de cordes par slabs
  -> niveaux shallow et décisions exactes filtrées
  -> RLE BallKey et BallActivation
  -> CSR préfixe, lots et DSU
  -> payload horizontal normalisé
```

Il n'existe aucune allocation, remise à zéro ou synchronisation globale par
ancre, patch ou niveau. Les files lourdes sont segmentées sans couper une unité
de certificat. Les sorties ambiguës sont compactées vers le CPU exact ou un
kernel multiprécision reçu; elles ne sont jamais décidées en flottant.

## 15. Gates

### P0 — sidecar

- forge fraîche sur table amputée refusée;
- clé de boule multiprécision extrême u16;
- supports redondant, dépendant et champ `support` incohérent refusés;
- sérialisation canonique et mutation de chaque champ;
- fold recevant réellement la capability;
- ASan/UBSan et campagne de permutations.

### P1 — prédicats source

- oracle rationnel à n=32;
- `prune + microtile = C(n,2)` par lane;
- seuils 10/9/8 décalés et égalité non stricte;
- antichaîne dupliquée, chevauchante ou recouvrant une extrémité;
- intervalles traversant zéro;
- reprise à chaque frontière count/fill.

### P2 — parcimonie 50 k

Publier `Q`, visites témoin, `a`, `M=sum_e m_e`, `sum_e c_e`, `sum_e Z_e`,
queues, octets et temps. NO-GO si :

- `source-cover + cordes > 400 ms` chaud sur G4;
- la majorité des paires atteint les microtuiles;
- une queue lourde sérialise le parcours;
- le travail contient `sum_e m_e^2`;
- un scan ancre--nuage ou une matrice paire--point apparaît.

Le seuil 400 ms est une porte d'exploration compatible avec le jalon sous la
seconde; ce n'est ni une prédiction ni le SLO principal de 100 ms.

### P3 — shallow et terminal

- égalité byte-à-byte au sweep dense local sur petites ancres;
- fixture diagonale absente de l'onion;
- parallèles, concurrences, Jung tangent, diamètre ex æquo et shell multiple;
- `Z_e<=m_e*(8-c_e)` et plancher de sorties non vide;
- census et `BallKey` comparés à un oracle multiprécision indépendant.

### P4 — quotient et fold

- tombstone, resolver et `Omega` comparés à Gamma exhaustif à petit `n`;
- coupes stricte et fermée après chaque lot;
- `q_min` distinct de `q_cert`;
- lookup strict/égal/manquant et cache de handle;
- ledger pré-DSU, couverture et records complets.

### P5 — qualification

Mesurer successivement count-only, source+census, fold et `warm_e2e` complet.
La qualification emploie la sortie réelle, toutes allocations et transferts,
des répétitions fraîches par famille, aucune limite configurée et un p95. La
cible principale reste `<100 ms`; `<1 s` est le jalon secondaire immédiat.

## 16. Reçus

Le manifeste porte au minimum : commit, compilateur, machine, digests des
binaires et entrées, paramètres, graine, compteurs de chaque gate, high-water
des arènes, temps par étage, état de reprise et statut censuré. Une mesure
CPU sur une machine G4 est étiquetée CPU; elle ne devient pas un benchmark GPU.

Les sommes combinatoires et tailles d'arènes sont calculées en entiers vérifiés
avant tout cast device. Une erreur d'identité est distincte d'un refus de
ressource.

## 17. Statut

La source cellules--tuples est refusée comme route produit sur les masses
actuelles. La source pair-block--shallow est mathématiquement fondée localement,
mais sa parcimonie globale, son constructeur exact et sa performance restent à
recevoir. Aucun résultat 50 k bout en bout n'existe.

Jusqu'à fermeture de P0--P5 : `public_status=not_claimed`.

GCP non utilisé pour cette proposition.
