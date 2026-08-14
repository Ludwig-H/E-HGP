# Déblocage q4 : corriger le shadow SOC64, puis passer par un porteur aigu factorisé

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

Le contrat `50000/G4` reste ouvert et n'est pas encore mesurable de bout en
bout. Le verrou q4 n'est plus le prédicat ponctuel : c'est la génération de la
relation utile sans développer une masse de paires ou de couples de carriers.

Quatre décisions en découlent.

1. `SOC64` et `CORNER512` sont mathématiquement sûrs comme certificats `ALL`.
   Le nouveau probe autonome reçoit leurs primitives bornées, mais pas encore
   l'ablation WSPD ni son coût transitif.
2. Le shadow WSPD observé pendant cette passe peut surcompter les mêmes
   `PointId` entre un ancêtre `SOC64-ALL` et ses descendants `central-ALL`.
   Ses compteurs `fermetures/masse_fermee` ne sont donc pas recevables avant un
   ledger contrefactuel disjoint.
3. Même un oracle universel parfait — LP, pelages inversés ou cages — peut
   laisser `n^2/4` paires q4 sur une famille u16 dont la vraie source q4 est
   vide. Ces certificats restent des fast paths et des diagnostics ; ils ne
   peuvent être la preuve de complétude de l'architecture principale.
4. Tout tétraèdre strictement bien centré possède, pour son arête maximale,
   au moins une face incidente aiguë. Cela ouvre une source exacte
   `RectKey -> AcuteCarrierBlock -> sweep 1D`, avant tout développement de
   `PairId` et sans arrangement global.

Ce dernier lemme lève un verrou de complétude, pas encore un verrou de coût.
Les nombres de blocs, faces et événements restent soumis à des portes de pente,
d'octets, de HWM et de temps.

## Snapshot concurrent audité

Le pin commis reste
`HEAD=35fcea884cb93eff24db1e7c5962f8be23d4cb04`. Pendant l'audit, Claude a
ajouté ou modifié concurremment le candidat SOC64 ; le worktree n'est donc pas
un snapshot documentaire propre. Au dernier relevé avant consolidation :

- `prototype/soc64_rect.hpp` :
  `b4750efee21affbf3160fb4db0f39b3498d4afd19093187db939450b317f3bc1` ;
- `prototype/soc64_probe.cpp` :
  `d442b59279f345d11337b86993b8b620774eb236815a8f77a227cbf8edc4944f` ;
- `prototype/wspd_wavefront_probe.cpp` :
  `1e90ac322b523a355d79a51100469a7fd9839b29619302197bafa8aa5c0b6336` ;
- `CMakeLists.txt` :
  `31471094023dd5fac172b9ce5afd3e756a20381c82b409663fdeeffa34c683dd`.

Ces empreintes ne promeuvent rien. L'auditeur n'a modifié aucun fichier sous
`prototype/`, `oracle/`, `tests/`, `receipts/` ou `CMakeLists.txt`.

GCP non utilisé par l'auditeur. Les chiffres G4 ci-dessous proviennent de reçus
existants dont la cible documentée est arrêtée.

## 1. Le SLO n'est pas encore qualifiable

Le squelette `BenchmarkOutputContract-v1` ne produit toujours ni source reçue,
ni fold Morse/HGP complet, ni dix forêts, ni verticales et payload officiel.
Le chronomètre actuel ne peut donc pas répondre à la question `warm_e2e`.

Les mesures disponibles suffisent néanmoins à localiser le coût :

- le chemin CPU dit « chaîne complète » prend `78,841 s` sur `uniform/50000`
  et produit `21 413 140` `SupportKey`, sans fermer le contrat officiel ;
- `eight_clusters/r4/50000` conserve `525 902 961` arêtes q4 sémantiques,
  `31 852 043` terminaux et `63 654 087` tests de front ;
- à seulement `n=500`, le chemin local q4 parcourt `191 538 784` couples de
  lentilles et `334 430 649` tests intérieurs pour `32 280` sorties q4.

Sources :
[`chaine_complete_g4_20260813`](../receipts/chaine_complete_g4_20260813/README.md),
[`rampe_raffinement_g4_20260813`](../receipts/rampe_raffinement_g4_20260813/README.md)
et
[`midball_amas_provenance_20260813`](../receipts/midball_amas_provenance_20260813/midball_eight_clusters_raw.txt).

Le chiffre `32,22 %`, obtenu en comparant `E4(50000)` corrigé à
`E4(25000)` non corrigé, n'est pas une gate valide lorsque SOC s'applique aux
deux tailles. Si `E'_n=(1-f_n)E_n`, la pente corrigée est
`p'=p+log2((1-f_(2n))/(1-f_n))`. Une fraction constante déplace seulement la
constante et laisse l'exposant inchangé. Il faut mesurer les mêmes ledgers à au
moins trois tailles et gater directement leurs pentes physiques et aval.

## 2. Contre-audit du candidat SOC64 live

### 2.1 Preuves reçues

Pour `e=z-a`, `t=b-z`, `H=e dot t`, `E=||e||^2` et `X=||t||^2`, les lanes sont

```text
q2 : H>0
q3 : H>0 et 4H^2>EX
q4 : H>0 et 3H^2>EX
```

À `t` fixé, puis à `e` fixé, le domaine strict est un cône circulaire convexe.
Si les `8*8` couples de coins de `(C-A)*(B-C)` passent, tout le produit relaxé
passe ; le vrai rectangle corrélé passe donc aussi. Un échec reste `UNKNOWN`.

Pour `a,b` fixés, prendre l'axe `ab`, écrire `z=(x,y)`, `r=||y||` et
`L=||b-a||`. Pour le coefficient `c` égal à quatre ou trois, le spindle strict
s'écrit sous la forme suivante :

$$x^2+r^2+\frac{L}{\sqrt{c-1}}r<\frac{L^2}{4}.$$

Le membre gauche est convexe. La convexité séparée en `a`, `b`, puis `z`
justifie donc les 512 coins de `CORNER512`. L'équivalence porte sur l'enveloppe
AABB continue ; l'échec d'un coin fictif ne prouve jamais `NONE` sur les seuls
points stockés.

Les différences u16 restent dans `[-65535,65535]`. `H`, `E` et `X` tiennent
sur 34 bits ; `EX` et `4H^2` restent sous 70 bits. `i128` suffit aux décisions
de ce fichier, sous réserve que les boîtes reçues appartiennent bien au profil.

### 2.2 Rejeux autonomes

Au snapshot des deux fichiers SOC indiqué plus haut :

```text
build cible mhgp3v_soc64_probe : code 0
CTest ^mhgp3v_soc64_ : 16/16 verts, 0,64 s
fixtures : accord=OUI, 9 gravées, 432 équivariances
selftest seed=1 : 20000 rectangles, 0 désaccord
```

Le mutant 64 bits, initialement non mordu par une fixture colinéaire, possède
maintenant une arithmétique modulaire définie et une fixture presque
orthogonale qui le tue. Ces résultats reçoivent la primitive bornée et ses
mutants ; ils ne reçoivent ni le ledger shadow, ni une baisse de `M4`, ni un
coût G4.

### 2.3 Double comptage trouvé, puis réparé dans le worktree live

La première version du shadow créditait toute la population d'un CNode lorsque
`SOC64` rendait `ALL`, puis laissait la traversée centrale descendre. Un
descendant `central-ALL` entrait aussi dans `cred[2]`; le masque `socm`
empêchait `SOC -> SOC`, pas `SOC -> central`. Le test
`cred[2]+soc_cred>=need[2]` comptait donc deux fois les mêmes IDs.

Claude a depuis appliqué la réparation conceptuelle demandée :

```text
ledger_base : traversée centrale inchangée
ledger_alt  : même traversée, mais SOC64-ALL crédite le nœud et coupe q4 sous lui
```

Le code live maintient `cred/mask` et `ccred/cmask`, coupe la lane combinée à
son premier `ALL`, puis comptabilise le flip seulement au ledger terminal. Sur
`eight_clusters/500/r4`, le terminal corrigé donne `15775` fermetures et
`21945` de masse, contre `20788` et `28081` pour la somme brute conservée comme
mutant témoin. Le build ciblé passe et les 27 CTests WSPD historiques restent
verts.

Cette réparation logique est recevable par inspection, mais aucune CTest ne
lance encore `--soc64-shadow`. La porte d'intégration doit comparer :

1. baseline centrale ;
2. vraie traversée `central OR SOC` qui s'arrête au premier `SOC64-ALL` ;
3. shadow à deux ledgers, jugé après expansion par vrais `PointId`.

Une fixture axiale place un parent `SOC64-ALL` au-dessus de descendants
`central-ALL`; une seconde place `fallback-ALL` sur le même nœud. Les
fermetures d'un terminal `OPEN` restent séparées de celles encore `PENDING`.
`masse_creditee` demeure un `AttemptStats` parent-plus-enfants et peut dépasser
`C(n,2)` ; seule la ligne terminale est une masse exclusive.

### 2.4 API et coût encore incomplets

Avec `floor>q2`, un retour sous le seuil n'est pas la lane `ALL` exacte ; il
signifie seulement « le seuil est impossible ». Contre-fixture u16 :

```text
A={(0,0,0)}, C={(1,1,1)}, B=[0,4]x{1}x{1}, floor=q4
```

Le centre a `e=(1,1,1)`, `t=(1,0,0)` et rend q3, mais le coin
`b=(0,1,1)` donne `H=-1`; la vraie lane minimale est `NONE`. Claude a gravé
cette fixture et l'API live rend désormais `UNKNOWN_BELOW_FLOOR` lorsque
l'énumération s'arrête sous le seuil. L'appel shadow ne lit que `retour>=q4`.

Autres écarts à l'ablation annoncée :

- le coût maximal implémenté est 65 prédicats pour `SOC64` et 513 pour
  `CORNER512`, prétest central compris ;
- `SocStats.wide` ajoute deux alors que le chemin forme trois ou quatre
  multiplications larges selon la lane ;
- le probe autonome jette les statistiques de chaque rectangle et ne publie
  ni temps ni HWM ;
- le shadow live visite chaque CNode q4 `MIXED`, sans le cap déterministe de
  4096 tâches annoncé ; `soc-judge-cap` borne les triples par verdict, pas le
  nombre de tâches, et un grand nombre de verdicts sautés reste code zéro.

Un rejeu local lecture seule sur `eight_clusters/2000` a soumis `3 809 028`
tâches et `18 871 452` prédicats de couples. La phase de vague est passée
d'environ `2,83 s` sans shadow à `4,94 s` avec shadow. Ce timing sur machine
partagée n'est pas un benchmark ; l'écart suffit à interdire une rampe 50k
aveugle. Échantillonner d'abord un ensemble scellé, déterministe et pondéré par
la masse `|A||B|`, puis publier gain massique, prédicats, opérations larges,
temps et HWM.

## 3. Contre-famille u16 : pourquoi LP/cages ne peuvent être la source

Pour `1<=i,j<=m<=25000` et `H=65535`, poser

$$A_i=(i,0,0),\qquad B_j=(0,j,H).$$

Pour chaque paire croisée, le centre suivant définit une sphère vide passant
par ses deux extrémités :

$$c_{ij}=\left(i,j,\frac{H^2+i^2-j^2}{2H}\right).$$

Pour tout autre point des deux droites, les puissances extérieures sont

$$\left\Vert A_k-c_{ij}\right\Vert^2-R_{ij}^2=(k-i)^2>0,\qquad \left\Vert B_l-c_{ij}\right\Vert^2-R_{ij}^2=(l-j)^2>0.$$

Cette sphère appartient même au disque de Jung q4 sur tout le carré u16. Ainsi
aucun certificat sound exigeant huit intérieurs dans toute sphère admissible
ne ferme les `m^2=n^2/4` paires croisées : ni LP global, ni pelages inversés,
ni cages universelles, même avec une ordonnance parfaite.

Pourtant chaque triangle non dégénéré emploie deux points d'une des droites.
Si `i<k`, l'angle en `A_i` vérifie

$$\left(A_k-A_i\right)\mathbin{\cdot}\left(B_j-A_i\right)=-(k-i)i<0.$$

Le calcul symétrique vaut sur la droite B. Aucun triangle n'est aigu, donc
aucun triangle q3 n'est positif et, par le lemme de la section suivante, aucun
tétraèdre q4 n'est positif. La vraie source q3/q4 est vide alors que le
résiduel universel est quadratique.

Cette contradiction devient une fixture permanente. À petit `m`, l'oracle
développe et vérifie tout. À grande taille, le chemin produit doit conserver
la relation dans quelques `RectKey`, obtenir zéro carrier aigu et zéro sweep,
sans allouer de tableau par `PairId`.

## 4. Lemme exact du porteur aigu

**Lemme.** Soit un tétraèdre non dégénéré dont le circumcentre `O` est
strictement intérieur. Pour toute arête maximale `ab`, au moins une des faces
incidentes `abx` ou `aby` est un triangle strictement aigu.

**Preuve.** Translater `O` en zéro, noter `R` le rayon et poser `n=a+b`. Pour
`z` égal à `x` ou `y`, l'angle opposé à `ab` est aigu si et seulement si

$$\left(a-z\right)\mathbin{\cdot}\left(b-z\right)>0\quad\Longleftrightarrow\quad n\mathbin{\cdot}z<\frac{\left\Vert n\right\Vert^2}{2}.$$

En effet, tous les sommets ont norme `R` et
`||n||^2/2=R^2+a dot b=n dot a=n dot b`. Si les deux angles en `x` et `y`
étaient droits ou obtus, les quatre sommets appartiendraient au demi-espace
fermé `n dot v>=||n||^2/2`. On a `n!=0` : sinon `O` serait le milieu de
`ab`, donc sur la frontière du tétraèdre. Ce demi-espace exclut alors
strictement `O` de l'enveloppe des quatre sommets, contradiction. L'un des
deux angles opposés est donc aigu. Comme `ab` est maximale, cet angle est le
plus grand de sa face ; les deux autres sont eux aussi strictement aigus. CQFD.

La stricte intériorité et la maximalité sont indispensables. Le carrier doit
être un triangle aigu géométrique même si sa propre boule q3 est hors de la
fenêtre de profondeur : filtrer par la sortie q3 reçue perdrait des q4.

## 5. Source candidate : `AcuteCarrierGateway-q4`

Pour `d=b-a`, `u=x-a`, poser

```text
D = d dot d
E = u dot u
F = d dot u
X = D+E-2F
```

Le triplet `(a,b,x)` est un porteur admissible pour l'owner `ab` lorsque
`D>=E`, `D>=X` et `E+X>D`, plus le tie-break exact d'`EdgeKey`. Une porte de
blocs applique ces inégalités par bornes sûres sur `A*B*C` : `NONE` élimine le
bloc entier, `ALL` émet un `AcuteCarrierBlock`, `MIXED` subdivise ou conserve
une continuation. À défaut d'une borne corrélée, le test simple
`max ||2x-a-b||^2 <= min ||b-a||^2` certifie que tout le bloc est non aigu ;
l'égalité peut être éliminée car le lemme promet une face strictement aiguë.

Une fois `(a,b,x)` fixé, poser

```text
G = D*E-F^2
n = d cross u
W = E*(D-F)*d + D*(E-F)*u
```

Tous les centres de sphères passant par la face vivent sur une seule droite :

$$c(\tau)=a+\frac{W+\tau n}{2G}.$$

Pour `s=z-a`, la puissance multipliée par `G` est affine en `tau` :

$$P_z(\tau)=G\left\Vert s\right\Vert^2-W\mathbin{\cdot}s-\tau n\mathbin{\cdot}s.$$

Un apex non coplanaire `y` crée donc l'événement rationnel

$$\tau_y=\frac{G\left\Vert y-a\right\Vert^2-W\mathbin{\cdot}(y-a)}{n\mathbin{\cdot}(y-a)}.$$

Le moteur q4 n'a plus à construire toutes les intersections de deux lignes
dans le plan médiateur : pour chaque porteur retenu, il trie ou sélectionne les
événements d'une sweep 1D, par lots de valeurs égales. Il rejoue ensuite
indépendance, positivité tétraédrique, owner, Jung, rang strict, shell,
`BallKey`, census et exact-once. Si les deux faces incidentes sont aiguës, le
plus petit `PointId` carrier admissible porte l'occurrence.

Les comparaisons de `tau` peuvent atteindre environ 155 bits sous u16 ; un
`i128` implicite n'est pas une autorité suffisante. Employer des limbs prouvés
ou BigInt dans le juge, normaliser le signe du dénominateur et traiter les lots
égaux atomiquement.

Cette voie évite l'arrangement global et tue exactement la contre-famille à
deux droites avant le lift. Elle ne garantit pas que le nombre de faces aiguës
ou de touches site-face soit sparse. Un scan de tous les sites par face
réintroduirait le coût interdit.

## 6. Fast path collectif supplémentaire : `OriginOnionDepth-h`

Le LP pairwise peut être remplacé, sur une banque bornée, par un certificat
collectif factorisable. Fixer l'ancre `a` et inverser chaque témoin :

$$p_z=\frac{z-a}{\left\Vert z-a\right\Vert^2}.$$

Poser `P_0=P` et construire successivement
`K_j=conv({0} union {p_z:z in P_j})`. À chaque couche, retirer un ID canonique
pour chaque sommet géométrique non nul de `K_j`, donnant `P_(j+1)`.

Si `p_b` appartient à l'intérieur relatif de `K_j` pour
`j=0,...,h-1`, toute sphère passant par `a,b` contient au moins `h` IDs
distincts. En effet, l'inversion écrit la frontière comme `y dot p=1` et son
intérieur comme `y dot p>1`, tandis que `y dot 0=0`. Un point intérieur relatif
à valeur un force un sommet de la couche à valeur strictement supérieure à un.
Les couches retirées sont disjointes.

Il suffit de vérifier que `K_0` et `K_(h-1)` ont le même rang affine et que
`p_b` est strictement dans `K_(h-1)`. Pour une facette rationnelle
`u dot p<=v`, l'origine donne `v>=0` et, avec `d=b-a`, le test entier devient

$$v\left\Vert d\right\Vert^2-u\mathbin{\cdot}d\geq1.$$

Son minimum sur une AABB est séparable. Les formes homogènes restent sous
environ 87 bits dans le profil u16 ; `i128` suffit au replay de facette. Rang
qui chute, égalité, banque capée, ID dupliqué ou construction incomplète donnent
`UNKNOWN`.

`h=8/9/10` ferme respectivement q4/q3/q2 sous `smax=11`. Ce certificat peut
créditer un BNode entier sans PairId et mérite une ablation après correction du
shadow. Il reste universel et ne ferme donc pas la contre-famille précédente ;
il est un OR de prune, pas l'architecture source.

## 7. Gates avant toute nouvelle session G4

### `SOC64-shadow-q4-v1`

- snapshot scellé de tâches `central-MIXED`, échantillonné déterministement et
  pondéré par `|A||B|` ;
- cap strict de 4096 tâches par famille pour le diagnostic initial ;
- ledgers baseline/alternatif disjoints et oracle de vrais IDs à petit `n` ;
- `predicats_et`, sorties anticipées, vraies opérations larges, masse nouvelle,
  `M4` évité, temps et HWM ;
- résiduels corrigés aux mêmes trois ou quatre tailles : une fraction de
  fermeture plate ne change pas la pente ; aucune rampe 50k si deux pentes
  physiques consécutives ou le coût composé restent rouges.

### `AcuteCarrierGateway-shadow-q4-v0`

- compteurs `RectKey`, blocs testés/`NONE`/`ALL`/`MIXED`, faces aiguës exactes,
  événements de sweep, touches site-face, `BallKey`, supports, octets et HWM ;
- oracle exhaustif : chaque q4 positif possède exactement un chemin
  owner-edge/carrier sous permutations, égalités et tuilage ;
- fixture « face aiguë géométrique hors fenêtre q3 » ;
- famille u16 à deux droites : zéro face aiguë, zéro sweep, aucune allocation
  par paire malgré `n^2/4` de masse universelle ;
- `uniform` et `eight_clusters` aux tailles `1500/3000/6000` : deux pentes
  physiques consécutives au plus `1,35`, sinon arrêt avant 50k.

Après ces portes seulement, brancher la sweep 1D par `count -> preflight ->
fill`, avec bundles égaux et continuations. Une réussite sur masse sémantique
sans baisse des événements, touches, octets ou HWM reste rouge.

## 8. Contre-audit de l'autre auditeur

Les preuves SOC64, CORNER512, LP `kappa<D`, base LP de trois IDs et récursion à
3280 appels q4 sont correctes. Les corrections « cages de quatre à six sites »,
`64/72` formes maximales et largeur du constructeur supérieure à `i128` sont
également justes.

Trois restrictions restent à conserver :

- `delta>=4h-3` est une borne suffisante pour une extraction gloutonne après
  réduction à une positive basis inclusion-minimale ; son optimalité globale
  n'est pas démontrée ;
- ce seuil construit des groupes positive-spanning, mais ne prouve pas que
  leurs fleurs créditent une cible `b` ;
- un échec du LP global ne sélectionne pas directement le moteur local : le
  certificat borné au vrai disque de Jung, via Helly et profondeur huit, est
  un diagnostic intermédiaire plus général.

La cascade `SOC64 -> LP -> cages` est donc recevable comme diagnostic de cause,
pas comme ordre d'architecture principal. La contre-famille u16 impose de faire
de la positivité et du porteur aigu une entrée du générateur q4.

## 9. Non-claims

Ni le lemme aigu, ni les pelages inversés, ni les 16 CTests SOC ne reçoivent une
source q4 industrielle. Ils définissent des obligations exactes et des portes
qui peuvent tuer rapidement une ordonnance. Le contrat reste ouvert jusqu'à
un `BenchmarkOutputContract-v1` complet, `pending=0`, mesuré au p95 sur la même
G4 et sur toutes les familles contractuelles.

GCP non utilisé.
