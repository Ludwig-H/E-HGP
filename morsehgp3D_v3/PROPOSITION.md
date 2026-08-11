# MorseHGP3D v3 — proposition d'architecture courante

Date : 11 août 2026 UTC.

> Cette proposition n'est ni une spécification ni une qualification produit.
> L'autorité existante reste
> [`SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md) et le
> registre [`STATUT_PREUVES_ET_HEURISTIQUES.md`](../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md).

## 1. Décision d'architecture

La route 50 k ne doit ni construire la mosaïque de Delaunay d'ordre supérieur,
ni matérialiser toutes les paires de générateurs, ni produire tous les
quadruplets d'une dilation locale.

La proposition vise un nouveau contrat explicitement versionné,
`hgp_reduced_normalized_h0_v3` : composantes horizontales exactes, niveaux
exacts, lots atomiques et union exacte des `PointId`, avec quotient certifié des
blocs H0 silencieux.

Elle ne remplace pas le transcript Gamma/v2 exhaustif. Les facettes, cofaces,
identifiants de lots silencieux et verticales ne peuvent être omis d'une sortie
qui prétend rester byte-identique au contrat v2.

## 2. Invariants non négociables

1. Tous les prédicats qui décident une émission, une omission, un owner ou un
   lot utilisent l'arithmétique exacte reçue.
2. Un préflight calcule une borne exacte ou majorante avant allocation; un
   dépassement refuse, il ne tronque jamais.
3. Chaque niveau exact est un lot transactionnel : snapshot strict gelé,
   calcul complet, puis commit fermé unique.
4. Une candidate GPU est jugée par une vérité CPU indépendante et par des
   identités de masse, pas seulement par la partition DSU finale.
5. Aucun cap, cache, filtre flottant ou partition spatiale ne modifie la vérité.
6. Aucun backend produit ne matérialise une structure globale de Delaunay
   d'ordre supérieur sous un autre nom.

## 3. Objet intermédiaire : `BallActivation`

Le type borné `CriticalSphere(rank<=32)` n'est pas adapté au profil 50 k. Le
chemin proposé stream un objet interne à coquille variable :

```text
BallActivation
  BallKey exacte, beta exact et cellule owner
  p = nombre de points strictement intérieurs
  shell complet et digest de census
  masque des arités de supports propres positifs certifiés
  q_min certifié pour la provenance Morse
  q_cert = plus grande arité positive effectivement certifiée
  fenêtre d'ordres H0 encore pertinente
  handles strict/fermé latents par ordre
  preuve d'émission ou tombstone d'inertie
```

`q_cert` n'affirme pas l'absence d'un support plus grand. Trouver un support
propre positif plus grand renforce le certificat; ne pas le trouver conserve
une activation de trop sans perdre l'exactitude. `q_min` et `q_cert` ne doivent
jamais partager un champ ni une sémantique implicite.

Les activations émises par plusieurs lanes sont réunies par `BallKey`. Un
certificat positif de haute arité peut tombstoner une activation conservatrice
de la même clé émise par une lane plus basse.

## 4. Pont de haut rang

Soit une boule fermée `B`, avec `p` points strictement intérieurs, coquille
complète `E` et support propre positif `U` de cardinal `q`. Le théorème 4.2 de
[`INCIDENCES_SILENCIEUSES_GAMMA.md`](../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md) donne :

$$1\leq k\leq p+q-2\Longrightarrow J_k(I\mathbin{\cup}E)\text{ est une continuation }H_0\text{ à racine antérieure unique et sans nouveau PointId.}$$

Ainsi, pour les ordres `1..K`, une preuve positive de `p+q>=K+2` rend la boule
inerte pour le quotient horizontal entier.

Le support doit être propre positif : points affinement indépendants et centre
dans l'intérieur relatif de leur enveloppe convexe. Une combinaison positive
redondante ne suffit pas. Le carré plan cosphérique est la fixture permanente
qui tue cette confusion.

Ce théorème autorise une tombstone H0. Il n'efface pas les données Gamma
facettées et ne prouve aucune verticale.

## 5. Source exacte par cellules de centres

### 5.1 Partition et ownership

Construire une partition half-open de l'espace des centres. Elle doit couvrir
toutes les cellules pouvant rencontrer `conv(X)`, y compris celles qui ne
contiennent aucun point du nuage. Le centre rationnel exact de la miniboule
choisit une cellule owner unique; les dernières faces de la boîte racine ont
une convention fermée canonique.

Les distances de banque et de dilation utilisent la fermeture de la cellule.
Pour une grille entière, une forme sûre est :

$$t_q=K+2-q,\qquad Q_{q,C}=1+\max_{w\in W_{q,C},\,z\in\mathrm{corners}(C)}\left\Vert w-z\right\Vert^2.$$

Puis :

$$A_{q,C}=\left\lbrace x:\mathrm{dist}^2(x,\overline{C})<Q_{q,C}\right\rbrace.$$

Une variante fermée sans `+1` est possible avec `<=`; les deux conventions ne
doivent jamais être mélangées.

Pour `K=10`, les tailles de banques sont `10/9/8` pour `q=2/3/4`. Un seul
top-10 exact par cellule fournit des préfixes imbriqués, donc
`A_{4,C} subset A_{3,C} subset A_{2,C}`.

### 5.2 Terminal banque avant census

Pour chaque support propre candidat dont le centre appartient à `C` :

- si les `t_q` témoins sont tous strictement intérieurs à sa sphère, alors
  `p+q>=K+2`; émettre un certificat d'inertie ou ignorer cette vue sous preuve ;
- sinon un témoin non intérieur prouve `beta<Q_{q,C}`; seulement alors le scan
  de `A_{q,C}` constitue un census global complet du support, de l'intérieur et
  de toute la coquille.

L'ordre est essentiel : scanner `A_C`, accepter, puis invoquer la banque serait
circulaire et pourrait manquer des intérieurs hors de `A_C`.

Les singletons suivent une lane séparée. Les supports `q=2/3/4` sont validés
comme miniboules propres avant ownership, puis les `BallKey` sont triées et
réduites entre lanes.

### 5.3 Top-t et préflight

Le top-t exact peut utiliser un LBVH avec branch-and-bound. Pour une cellule
fermée, le score d'un témoin est son maximum de distance carrée aux huit coins;
la borne d'un nœud AABB est séparable par axe. L'arrêt sur la valeur du t-ième
score est exact; si une sélection lexicographique des témoins est contractuelle,
les égalités doivent également être visitées ou bornées par le plus petit
`PointId`.

Avant toute énumération, accumuler en entier vérifié : nombre de cellules,
`sum |A_C|`, `sum C(|A_C|,q)`, borne de census, duplications avant owner,
BallKeys attendues et octets de chaque arène. La subdivision n'est acceptée que
si la somme exacte pondérée des enfants améliore la route et tient le budget.

## 6. Prune convexe des cellules hautes

Tout support vrai `U` possédé par `C` vérifie `U subset A_C` et son centre
appartient à `conv(U)`. La condition nécessaire est donc :

$$\overline{C}\cap\mathrm{conv}(A_C)\neq\varnothing.$$

Si les deux compacts sont strictement séparés, la cellule ne possède aucun
support propre et sa masse combinadique peut être supprimée exactement.

Pipeline de décision :

1. séparateurs d'axes depuis les min/max calculés pendant le fill ;
2. proposition d'un plan général par GJK ou LP flottant seulement pour les
   cellules lourdes ;
3. quantification du normal et revérification exacte de la séparation stricte
   sur tous les points de `A_C` et les huit coins fermés ;
4. sans certificat, conserver la cellule.

L'admission publie les masses non linéaires retirées, pas seulement le nombre
de cellules prunées. La partition anisotrope ne vient qu'après ce prune et
seulement sur les cellules lourdes survivantes.

## 7. Lane q4 si les quadruplets restent rouges

La baseline exacte évite les quadruplets directs sans supposer une source
shallow : pour chaque cellule survivante, énumérer tous les triples non
collinéaires de `A_{4,C}`. Les centres des sphères passant par un triple `T`
forment une droite `L_T`; la puissance de chaque point le long de cette droite
est affine.

Les huit témoins définissent un intervalle ouvert `J_omit`. Tout zéro qui y
porterait un support q4 propre positif serait H0-inerte; un zéro impropre n'est
pas une activation q4. Sur `J_keep`, complément d'au plus deux intervalles, un
reporter terminal doit rendre tous les zéros, y compris égalités et extrémités.

Pour chaque zéro : former le quatrième point, vérifier indépendance affine,
support propre positif, owner half-open, terminal banque, census complet,
`BallKey`, puis dédupliquer. Imposer comme `T` les trois plus petits `PointId`
du support retire les vues combinatoires dupliquées.

Scanner tous les points pour chaque triple est interdit : le travail vaut
exactement `4*R_4+3*R_3`. Le reporter doit être un vrai range reporter avec
compteurs de nœuds, feuilles, points et zéros.

Les extrémités de `L_T intersect C` sont rationnelles et ne sont pas des
`Sphere` issues de supports. La proposition introduit un `PencilInterval` avec
comparaisons multiprécision à largeur prouvée. Les cas de segment réduit,
centre sur plusieurs faces, puissance constante, zéro en extrémité et zéros ex
æquo sont des fixtures obligatoires.

## 8. Resolver des activations silencieuses

Omettre une boule H0-inerte sans conserver de locator serait faux : une face de
ce bloc peut devenir le carrier d'une fusion ultérieure.

Pour résoudre une `k`-face `F` avant un cutoff futur `a` :

1. calculer sa miniboule exacte `D`, son intérieur strict et un support propre
   positif de taille `q` ;
2. si `k>p+q-2`, retourner le handle de `D` fermé après son propre lot ;
3. si `k<=p`, remplacer `F` par les `k` intérieurs canoniques ;
4. sinon prendre tous les intérieurs et `k-p` points canoniques du support ;
5. recommencer, le niveau ayant strictement diminué.

La descente termine sur l'ensemble fini des niveaux. Le lookup temporel est
`closed@beta(D)` avec `beta(D)<a`, jamais `strict@beta(D)`, jamais `closed@a`
et jamais pendant le lot de `D` lui-même. Le cache stocke un handle
`BallActivation`, pas une racine DSU susceptible de changer.

## 9. Quotient local des coquilles

Pour une boule encore pertinente à l'ordre `k`, poser `t=k-p`. Les composantes
du graphe local strict des k-faces sont représentées par l'arrangement :

$$\Omega_{k,B}=\bigcup_{A\subset E,\,|A|=t}\left\lbrace \nu\in S^2:\langle x-c,\nu\rangle>0\text{ pour tout }x\in A\right\rbrace.$$

Une composante de `Omega` fournit une face stricte canonique; le resolver la
projette sur une racine globale gelée. Les collisions entre composantes locales
sont dédupliquées après ce lookup. Au niveau fermé, le bloc de Johnson est
connexe; le lot s'exprime comme un graphe biparti
`BallActivation--racine_stricte` et se ferme atomiquement.

Avant intégration, une gate exhaustive doit comparer `pi0(Omega)` au graphe
local strict sur les coquilles multiples. La taille de l'arrangement, la somme
des tailles de coquilles et leur carré appartiennent au préflight; aucune borne
linéaire n'est supposée.

## 10. Fold sparse

### 10.1 Fast principal

Pour un générateur de support principal `U`, les attaches
`S_u=(U sans u) union T` sont licites lorsque `q<=k+1`. Le certificat principal
donne directement `beta(M sans u)<beta(M)`; comme `S_u subset M sans u`, chaque
carrier est strict.

Le fast path dans un lot multiple exige : support principal certifié,
`q<=k+1`, `CarrierClosure`, handle unique par `BallKey` et lookup pré-lot de
chaque carrier. Lookup absent ou non strict sous prétention complète signifie
refus atomique. `q>k+1` reste au fallback dans les lots multiples.

### 10.2 Fallback préfixe

Le fallback indexe les membres sous un ordre global commun. Une requête de rang
`r` à l'ordre `k` utilise `r-k+1` membres, fusionne les postings, puis
recertifie `|M intersection N|>=k` sur les deux saturés réels. Le préflight
calcule exactement les hits depuis les degrés figés du snapshot.

La représentation device cible est une CSR compacte avec compteurs vérifiés,
streaming par slabs de candidats et ledger pré-DSU. `prefix-all` reste le juge
relatif de tout backend optimisé.

### 10.3 Lot atomique

Pour chaque niveau et ordre :

1. figer les racines strictes et les degrés de l'index ;
2. construire fast et fallback sans muter le DSU public ;
3. recertifier chaque incidence avant projection vers une racine ;
4. comparer masses, ledger et couverture au reçu ;
5. appliquer les composantes du graphe biparti en un commit ;
6. publier les handles fermés et marqueurs, y compris pour une continuation.

## 11. Architecture GPU candidate

```text
points u16 + LBVH résidents
  -> cellules count-only, top-10, dilations et prune convexe
  -> CSR des seules feuilles admises
  -> lanes q2/q3 streamées, q4 pencil si nécessaire
  -> tri/RLE BallKey, census et BallActivation
  -> CSR préfixe + incidences recertifiées
  -> composantes de lot / DSU device ou replay certifié
  -> payload horizontal normalisé
```

Aucun tableau global de tuples, de faces ou de paires ne persiste. Chaque kernel
possède un pass count, une arène bornée, un pass fill et une identité indépendante
entre les deux. Les lots trop gros sont chunkés sans couper une unité de
recertification ni publier un lot partiel.

## 12. Admission et reçus

Le manifeste minimal porte :

- digests des points, paramètres, source, ordre global et code ;
- cellules, listes `A_C`, masses par lane et masses après prune ;
- requêtes fast/fallback par catégorie, longueur CSR et hits prévus/lus ;
- candidats, faux positifs, incidences vraies et ledger avant DSU ;
- BallKeys, supports par arité, tombstones, intérieur et coquille ;
- coût du resolver et de `Omega`, tailles de lots et high-water de chaque arène ;
- timings séparés build, source, census, fold, payload et `warm_e2e`.

Toutes les sommes combinatoires et tous les produits d'octets sont calculés en
entiers vérifiés suffisamment larges avant cast vers l'ABI device. Une erreur
d'identité est distincte d'un refus de ressource.

## 13. Jalons

1. Recevoir le fast ex æquo courant et construire `ValidatedHybridSidecar`.
2. Mesurer le prune convexe count-only sur `terrain` et deux familles scanline.
3. Recevoir `BallActivation`, tombstones et census à coquille variable sur CPU.
4. Recevoir le resolver et `Omega` contre des oracles exhaustifs.
5. Implémenter et mesurer le range reporter q4 seulement si la lane reste rouge.
6. Porter les primitives reçues sur CUDA, d'abord sans fold public.
7. Qualifier source+fold horizontal, puis traiter les verticales selon le contrat
   finalement retenu.

Budget de conception, non mesuré : au plus 0,50 s pour source+census+`Omega`,
0,30 s pour joins+lots+fold et 0,20 s pour construction, transferts et payload.
Ces nombres sont des enveloppes d'admission, pas une prédiction de performance.

## 14. Conditions de GO

Le backend G4 ne devient candidat produit que si :

- toutes les gates CPU et device sont vertes sur les mêmes identités ;
- les profils générique, terrain, scanline simple, scanline multi-écho et
  fixtures dégénérées sont séparément admis ;
- aucun chemin hostile ne cappe ou n'omet une sortie ;
- le pic mémoire est mesuré et inférieur à la borne publiée ;
- le temps `warm_e2e` inclut réellement source, fold et payload ;
- le contrat de sortie est nommé sans ambiguïté et les verticales requises sont
  reçues.

Jusque-là : `public_status=not_claimed`.

GCP non utilisé pour cette proposition.
