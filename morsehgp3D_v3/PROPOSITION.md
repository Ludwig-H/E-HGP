# MorseHGP3D v3 — proposition d'architecture courante

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette proposition n'est ni une spécification ni une qualification produit.
L'autorité existante reste
[`SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md), avec le
registre [`STATUT_PREUVES_ET_HEURISTIQUES.md`](../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md).
Le verdict live est
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md).

## 1. Décision d'architecture

La route 50 k ne doit matérialiser ni mosaïque de Delaunay d'ordre supérieur,
ni catalogue Gamma global, ni tableau de toutes les paires, cellules, faces,
cofaces ou incidences. Un oracle exhaustif borné peut recertifier la route; il
ne devient jamais son implémentation par défaut.

Le candidat vise un contrat distinct,
`hgp_reduced_normalized_h0_v3` : composantes horizontales exactes, niveaux
exacts, lots atomiques et unions exactes des `PointId`, après quotient certifié
des blocs H0 inertes.

Il ne remplace pas Gamma/v2. Une boule H0-inerte peut encore porter des
facettes, des incidences silencieuses ou une application verticale. Tant que
ces informations ne sont pas reconstruites par une preuve séparée, la sortie
ne peut revendiquer ni le transcript v2 ni la hiérarchie verticale complète.

## 2. Invariants

1. Toute émission, omission, ownership et égalité de niveau est décidée par
   l'arithmétique exacte u16/rationnelle reçue.
2. Count, fill et consommation portent la même identité. Un dépassement
   refuse atomiquement; il ne tronque jamais.
   Un générateur demandé avec `n` publie exactement `n` points, ou refuse
   avant toute construction; tous ses consommateurs partagent ce contrat.
3. Chaque niveau exact utilise un snapshot strict gelé, puis un unique commit
   fermé. Une égalité n'est jamais séquentialisée arbitrairement.
4. Une source se prouve par son domaine complet et ses certificats rejouables,
   pas par un booléen, un rang maximal ou un digest de sa propre sortie.
5. Une proposition flottante peut ordonner le travail; seul un prédicat exact
   autorise un prune.
6. Aucun résultat plausible, accord moyen ou microbenchmark ne promeut le
   statut public.

## 3. Faits qui imposent le changement de source

La session G4 mass-only n'a formé aucun tuple. Après le prune d'axe, elle
conserve :

| lane | minimum | maximum |
| --- | ---: | ---: |
| q2 | 465 371 500 | 2 862 879 000 |
| q3 | 14 667 530 000 | 131 762 100 000 |
| q4 | 330 437 400 000 | 9 968 861 000 000 |

Les seuls top-t, dilations et comptes prennent 0,174--29,153 s sur 48 threads.
Ces nombres réfutent l'énumération combinadique de la grille uniforme. q2
n'est pas « admise » : elle est seulement moins massive que q3 et q4.

Le pinceau q4 par triples n'est pas une baseline industrielle. La masse R3
publiée appartient à la lane q3 et ne peut pas lui être imputée. En revanche,
la masse q4 suffit à le réfuter : même le schéma canonique des trois plus
petits identifiants impose, au pas 6, plus de 2,74 milliards, 10,63 milliards
et 1,02 milliard de triples selon la famille, avant range-report, census et
fold. Il reste un fallback exhaustif borné ou un oracle, jamais la route 50 k.

## 4. Pont de haut rang

Pour une boule fermée, soit `p` le nombre de points strictement intérieurs et
soit `q` la taille d'un support propre positif. Le théorème 4.2 de
[`INCIDENCES_SILENCIEUSES_GAMMA.md`](../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md) prouve :

$$1\leq k\leq p+q-2\Longrightarrow\text{le bloc est une continuation }H_0\text{ sans fusion ni nouveau PointId.}$$

Pour `K=10`, une preuve positive `p+q>=12` autorise une tombstone du quotient
horizontal demandé. Le support doit être affinement indépendant et le centre
doit appartenir à l'intérieur relatif de son enveloppe convexe. Un support
cosphérique redondant ne suffit pas.

La tombstone conserve une clé de boule et les témoins permettant son rejeu.
Elle suppose un resolver des carriers silencieux. Elle ne prouve ni l'absence
de support, ni l'absence d'incidence Gamma, ni une verticale.

## 5. Objet streamé

Le chemin produit ne transporte pas un `CriticalSphere(rank<=32)` global. Il
stream des records à coquille variable, réunis par clé exacte :

```text
BallActivation
  BallKey canonique multiprécision
  niveau exact et owner de source
  support propre positif et provenance rejouable
  p exact ou témoins stricts suffisants pour une tombstone
  q_min de provenance Morse, distinct de q_cert
  coquille fermée complète pour toute activation conservée
  fenêtre d'ordres H0 pertinente
  handles strict et fermé latents
  preuve d'émission, d'inertie ou de refus
```

`q_min` est la plus petite arité de provenance prouvée pour la boule.
`q_cert` est le maximum des arités de supports propres positifs effectivement
exhibées et rejouées pour cette boule afin de renforcer l'inertie; l'absence
d'un support plus grand n'est jamais déduite.

Une activation non inerte exige le census fermé complet avant fold. Une
tombstone peut éviter ce census uniquement si sa preuve H0 et son futur
resolver n'en ont pas besoin. Plusieurs lanes réunissent leurs records par la
même `BallKey`; les supports multiples ne créent pas plusieurs boules.

## 6. Source par arité

### 6.1 Ordre un : EMST exact

L'ordre un est exactement le single linkage, au niveau
`distance_squared/4`. La route candidate calcule un EMST par Boruvka exact sur
points u16, trie ses arêtes par niveau rationnel et rejoue chaque lot d'égalité
atomiquement.

La réception compare, après chaque coupe stricte et fermée, les partitions
canoniques de `PointId` à l'oracle EMST CPU. Elle couvre les ex æquo, plusieurs
EMST valides et une mutation qui reconnecte les mauvaises composantes aux
bons niveaux. Cette lane évite entièrement le catalogue Morse à `k=1`.

### 6.2 Supports q2 : Yao48, LBVH et census terminal

La boule d'un support q2 est la boule diamétrale de sa paire `(x,y)`. Un point
`z` est strictement intérieur si et seulement si
`(z-x) dot (z-y)<0`. Une activation q2 non tombstonée a au plus `K-1=9`
points strictement intérieurs.

La source candidate reprend l'architecture exacte de
[`CATALOGUE_PAIRES_DIAMETRALES_EXACT.md`](../docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md) :

1. des tuiles d'ancres remplissent 48 banques directionnelles de témoins
   distincts sur le LBVH Morton résident;
2. les trois inégalités entières de Yao48, toutes strictes pour la lane H0,
   certifient dix témoins strictement intérieurs et remplacent une région par
   un reçu de prune; une chambre sous-pleine ou une égalité reste fail-open;
3. le classifieur terminal parcourt le LBVH avec des bornes exactes sur
   `(z-x) dot (z-y)` et s'arrête à dix intérieurs seulement pour une tombstone;
4. toute paire conservée finit le census et publie la liste fermée complète
   `C(x,y)`, sa profondeur stricte, sa coquille, son rang, son niveau et sa
   `BallKey`, dans une seule passe multi-ordre.

Le ledger ferme simultanément
`candidate+certified_pruned+unresolved=C(n,2)`, la partition terminale
`below+exact+above`, la multiplicité canonique un et la liste fermée de chaque
record. Une frontière non vide ou un budget épuisé refuse la publication.

Le self-join AABB par témoins communs est une seconde preuve exacte : chaque
état représente un ensemble disjoint de paires et dix témoins universels
suppriment le bloc. Il reste un oracle indépendant, un falsificateur de masses
ou un second prune tant que ses compteurs ne battent pas Yao48/LBVH. Le juge
borné peut tenir un sort quadratique à petit `n`; aucun chemin produit ne
matérialise de matrice ou de liste globale de paires. Le pire cas reste
quadratique en sortie.

Pour trois AABB `W,X,Y`, le certificat exact combine le supremum `U4` et
l'infimum `L4`. Par axe, pour chaque couple d'extrémités `x,y`, poser
`t=clip(x+y,[2*w_min,2*w_max])`; le minimum de quatre fois
`(w-x)(w-y)` vaut `(t-2*x)(t-2*y)`. Le minimum sur les quatre couples puis la
somme des trois axes donne `L4`, tandis que les coins donnent `U4`. `U4<0`
accepte un nœud témoin, `L4>=0` l'écarte de la recherche d'intérieurs et le cas
intermédiaire descend. Les décisions sont monotones sous raffinement. Un
contact écarté de la recherche stricte reste obligatoirement rescanné pour le
census fermé terminal.

Sur le profil quantifié, une borne plus forte au même nombre de produits est
l'infimum exact sur la grille entière. Pour chaque couple `x,y`, prendre
`w0=clip(floor((x+y)/2),[w_min,w_max])` et évaluer
`4*(w0-x)*(w0-y)`; lorsque `x+y` est impair, l'autre entier voisin donne la
même valeur s'il appartient à l'intervalle. Le minimum sur les quatre couples,
puis la somme des axes, est au moins l'infimum continu et reste exact sur les
coordonnées u16. Son admission exige un différentiel exhaustif borné, notamment
sommes impaires et clips aux deux bords; aucune arithmétique flottante n'entre
dans la décision.

Une frontière persistante reste une antichaîne exacte sans cap. Lorsqu'un bloc
d'extrémités est scindé, le sous-arbre du frère libéré devient admissible comme
banque de témoins pour l'enfant et doit être réintroduit puis reclassifié par
`L4/U4`. Hériter seulement l'ancienne frontière omettrait ces nouveaux témoins.

Cette suppression vaut exclusivement pour q2. Une paire dont la boule
diamétrale contient dix témoins peut rester le diamètre d'un support q3/q4 dont
le centre est décalé et dont la sphère exclut ces témoins. Le résiduel q2 ne
devient jamais, par soustraction, la source d'ancres des arités supérieures.

### 6.3 Ancres q3/q4 : cœur universel de Jung

Tout support positif q3 ou q4 possède une arête de longueur maximale. Choisir
canoniquement la plus petite paire parmi les ex æquo. Pour `d=b-a`,
`D^2=d dot d`, `U=2w-a-b`, poser `g=D^2-||U||^2` et
`Q=D^2||U||^2-(U dot d)^2`.

La preuve reformulée dans
[`NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md`](audits/NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md)
redonne les filtres P0 déjà documentés pour `JungChordCsrTile` :

$$\text{q3: }g>0\ \text{ et }\ 3g^2>4Q,$$

$$\text{q4: }g>0\ \text{ et }\ g^2>2Q.$$

Ils prouvent que `w` est strictement intérieur à toute sphère admissible ancrée
par la paire. Neuf témoins distincts en q3 ou huit en q4 certifient
`p+q>=12`; le bloc d'ancres est H0-inerte jusqu'à `K=10`. Toute égalité, paire
non certifiée maximale, support non positif ou dégénérescence reste fail-open.
En particulier `D^2>0` est une garde préalable : l'égalité q4
`15||U||^2<=4D^2` ne constitue aucun certificat lorsque `D^2=U^2=0`.

Un self-join canonique implicite de toutes les paires peut donc émettre un
sur-ensemble complet des ancres non inertes sans construire un tableau de
paires et sans filtrer par q2. Le test de boule inscrit
`3||U||^2<D^2` en q3 ou `15||U||^2<=4D^2` en q4 fournit un premier prune AABB;
la comparaison large q4 exige toutefois `min D^2>0` sur tout le bloc. Sinon le
cas `D^2=U^2=0` fabriquerait un faux certificat malgré `g=0`, et la machine
doit descendre jusqu'au prédicat exact.

Le rejet `L4>=0` de la lane q2 s'applique aussi à la recherche de témoins du
cœur, car tout témoin Jung vérifie d'abord `g>0`, équivalent à l'intérieur
diamétral strict. Les 9/8 témoins déjà universels peuvent être hérités sous
raffinement. Toute frontière persistante reste lossless, sans cap, et
réintroduit les sous-arbres d'extrémités devenus disjoints après un split.

La profondeur fermée de demi-boule fournit un filtre terminal complémentaire.
Si `P={z:(z-a) dot (z-b)<0}` et `delta(a,b)` est le minimum du nombre de
projections de `P` dans un demi-plan fermé du plan médiateur, toute sphère de
coquille contenant `a,b` possède au moins `delta` intérieurs. Les seuils sont
`delta>=9` en q3 et `delta>=8` en q4, à condition que la `BallKey` porte un
support propre positif q3/q4 certifié contenant la paire. `q_min=2` ou un
`q_cert=2` seul n'autorise pas ces seuils; une même `BallKey` avec un
`q_cert=3/4` certifié peut en bénéficier. La lane q2 emploie séparément le total `|P|`; ses
survivants et ceux de q3/q4 ne sont pas emboîtés. Le schéma conditionnel de
center-cover par 64 patches reste une troisième preuve, non encore reçue comme
composant complet; sa banque dépend du patch plutôt que d'être universelle
pour toute la paire.

Le total diamétral q2 et la profondeur q3/q4 ont des résiduels incomparables.
Une seule machine peut partager l'arbre et la partition des paires, mais chaque
lane conserve son propre sort, son ledger et ses compteurs. Le cœur seul, la
profondeur seule et leur combinaison devront être mesurés séparément; seul le
cœur possède aujourd'hui un falsificateur.

Cette preuve donne la couverture, pas la parcimonie. Le nombre d'ancres peut
rester quadratique et une recherche naïve des témoins cubique. La source ne
devient candidate produit qu'après un ledger pair-à-pair borné et une admission
des masses à `12 500/25 000/50 000`. Chaque reçu de prune déduplique ses
`PointId` témoins avant d'appliquer le seuil. Le juge de couverture doit être
indépendant des primitives du sujet; c'est une exigence d'admission. Une
dépendance v2 commune peut servir de différentiel supplémentaire, jamais
d'autorité unique.

### 6.4 Supports q3 : un centre par troisième point

Pour un support triangulaire propre positif et son ancre reçue, le circumcentre
appartient au plan médiateur et vérifie `h^2<=D^2/12`. Chaque troisième point
définit au plus un centre dans ce disque.

Le candidat est conservé seulement après vérification exacte de l'indépendance,
de la positivité, de l'ancre canonique, de la profondeur `p<=K-2=8`, de
l'owner, du census fermé et de la `BallKey`. La réduction remplace les triples
globaux par `ancre x points rapportés`; aucun scan du nuage par ancre n'est
admis dans le chemin produit.

### 6.5 Supports q4 : niveaux peu profonds du plan médiateur

Pour un support tétraédrique propre positif et une ancre diamètre, Jung donne
`h^2<=D^2/8`. Dans ce disque, chaque autre point induit une droite d'égalité de
puissance et un demi-plan strictement intérieur.

Un q4 pertinent est une intersection de deux droites dont la profondeur
stricte est au plus `K-3=7`. La route construit directement les premiers
niveaux de l'arrangement, au lieu de former toutes les paires de droites. Elle
valide ensuite indépendance affine, positivité, diamètre et owner canoniques,
census et clé.

Les parallèles, droites confondues, intersections multiples, points de
frontière et grandes coquilles sont groupés en lots exacts. Le sweep doit les
traiter ou refuser la route avant toute sortie. La borne locale shallow ne
prouve ni que la somme des points rapportés par ancre est linéaire, ni que le
range-report l'est; ces masses restent des obligations d'admission.

## 7. Cellules de centres : oracle branch-and-bound

La grille existante reste utile pour falsifier les masses et recertifier les
petits cas. Sa formulation exacte est la suivante.

Pour une cellule half-open `C`, les bornes utilisent sa fermeture. Noter
`l_C(x)` et `u_C(x)` les distances carrées minimale et maximale de `x` à cette
fermeture. Pour la lane `q`, poser `t_q=K+2-q`, prendre les `t_q` plus petites
valeurs `u_C`, poser `R_q(C)` égal à leur maximum et
`A_q(C)={x : l_C(x)<=R_q(C)}`.

Si une boule owner de `C` a `beta>R_q(C)`, les `t_q` témoins sont strictement
intérieurs et la boule est H0-inerte. Sinon `beta<=R_q(C)` et son saturé fermé
entier appartient à `A_q(C)`. L'égalité reste toujours dans la branche
conservée.

Une séparation stricte entre la fermeture de `C` et `conv(A_q(C))` exclut un
support de la branche conservée, car son centre devrait appartenir à
`conv(A_q(C))`. Elle ne signifie jamais « aucun support » : la branche haute
peut contenir de vraies sphères, omissibles seulement du quotient H0 après
théorème 4.2 et resolver.

Sous subdivision, les listes sont imbriquées : un enfant ne peut qu'augmenter
`l`, diminuer `u` et diminuer `R`, donc son `A_q` est inclus dans celui du
parent. La racine couvre `conv(X)`, les enfants half-open ont un owner
rationnel unique, et une branche ne termine que par certificat, producteur
exact alternatif ou fallback exhaustif.

Cette complétude ne borne pas le travail. Une cosphère massive peut conserver
`|A_q|=Theta(n)` à toute profondeur et provoquer `Theta(n^q)` vues pour une
seule boule. Subdiviser jusqu'à stabilité peut aussi reconstruire implicitement
les bisecteurs du diagramme de Voronoi d'ordre supérieur. La structure reste
donc transitoire, sans atlas ni adjacences, et hors du chemin chaud tant
qu'aucune borne d'admission n'existe.

## 8. Resolver des blocs silencieux

Omettre un bloc H0-inerte sans locator est faux : une de ses facettes peut
devenir le carrier d'une fusion ultérieure. Pour résoudre une `k`-face `F`
avant un cutoff futur :

1. calculer sa miniboule exacte `D`, ses points strictement intérieurs et un
   support propre positif de taille `q` ;
2. si `k>p+q-2`, retourner le handle fermé de `D` ;
3. si `k<=p`, remplacer `F` par les `k` intérieurs canoniques ;
4. sinon prendre tous les intérieurs et `k-p` points canoniques du support ;
5. recommencer au niveau strictement plus petit.

Le lookup est `closed@beta(D)` après le lot propre de `D`, jamais le snapshot
strict du même lot. Le cache stocke un handle stable, pas une racine DSU
mutable. La terminaison, les ex æquo et les cycles apparents sont rejoués par
un oracle exhaustif borné avant intégration.

## 9. Fold sparse

Le fast principal multi-lot reçu s'applique seulement si `q<=k+1`, si le
support principal est certifié, si la source prouve la fermeture de chaque
carrier et si chaque lookup pré-lot est strictement antérieur. `q>k+1` reste
au fallback.

Le fallback préfixe emploie un ordre global commun, une longueur
`rank-k+1`, un préflight exact des postings puis la recertification
`|M intersection N|>=k` sur les saturés réels. `prefix-all` demeure le juge
relatif.

Le fold produit un graphe biparti `activation--racine_stricte` par lot :

1. geler racines, handles et degrés ;
2. construire toutes les incidences fast, fallback et résolues sans mutation ;
3. recertifier le ledger et la couverture ;
4. contracter chaque composante du lot en un commit atomique ;
5. publier handles fermés, tombstones et `coverage_delta`.

Une fermeture de source n'est pas un champ booléen. Le fold doit consommer un
stream ou sidecar validé jusqu'au bout; il ne revient jamais à un catalogue
brut après la gate.

## 10. Frontière de confiance

La frontière sépare durablement deux usages :

- un oracle CPU borné peut rescanner tous les points et recalculer les
  miniboules afin de falsifier une source. Le pipeline hybride exhaustif est
  explicitement limité à `n<=32` et ne devient jamais une source 50 k;
- le chemin produit consomme les certificats streamés du producteur, avec
  count/fill, domaine de tâches complet et rejeu indépendant ciblé.

Le constructeur du reçu et toute capacité sont privés, non forgeables par copie
binaire et possédés par le producteur terminal. Avant toute géométrie, la gate
valide le domaine u16 des points et de chaque représentation de sphère. Elle
reconstruit ou normalise un unique support canonique qui devient l'autorité des
certificats, carriers, digests et du fold.

La preuve de complétude ne se réduit ni à `rank_bound>=point_count` ni au digest
d'une table. Le reçu engage contrat et SHA du producteur, profil, schéma et
identité des tâches, counts prévus/remplis/consommés, statut terminal sans
censure, points, catalogue et paramètres. Les clés sont multiprécision et
canoniques; les digests emploient une sérialisation champ par champ, taggée,
versionnée et SHA-256. Le digest lie le reçu aux données; le ledger et les
certificats en prouvent la portée.

Les défauts précis d'une livraison n'appartiennent pas à cette proposition
durable; ils sont tenus dans
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md).

## 11. Architecture G4 candidate

```text
points u16 + LBVH exact résidents
  |-> k=1 : Boruvka/EMST exact
  |-> q2 : Yao48 strict + classification LBVH et census fermé
  `-> q3/q4 : cœur de Jung + profondeur fermée + reporters shallow
       -> BallActivation/tombstones streamées
       -> sort/RLE par BallKey exacte
       -> carriers stricts + resolver latent
       -> fast/fallback recertifiés par lot
       -> composantes et payload horizontal normalisé
```

Aucun tableau global de tuples, paires, cellules, faces ou cofaces ne persiste.
Chaque kernel a un count, une arène bornée, un fill et une identité de
consommation. Les slabs ne coupent ni une `BallKey`, ni un lot exact, ni une
unité de recertification.

Les cellules adaptatives et l'oracle exhaustif restent hors du chrono produit.
Ils recertifient des échantillons et des fixtures, puis comparent digests,
masses et décisions à la source device.

Pour le jalon secondaire d'une seconde, l'enveloppe provisoire est :

| tranche | enveloppe chaude |
| --- | ---: |
| transfert + LBVH | 40 ms |
| source + cover | 200 ms |
| cordes | 200 ms |
| shallow + décision exacte | 300 ms |
| reducer + payload | 200 ms |
| réserve | 60 ms |

Source, cover et cordes au-dessus de 400 ms chaud classent la route no-go.
Ces enveloppes sont des seuils de falsification d'architecture, pas une
qualification : seul le p95 du pipeline complet décide `warm_e2e`.

## 12. Admission et reçus

Le manifeste engage au minimum :

- commit, digests des points, paramètres et schéma de clé ;
- tâches source prévues, remplies, consommées, scindées et refusées ;
- produits dual-tree, ancres, arrangements, sommets shallow et faux positifs ;
- activations, tombstones, supports par arité, profondeurs et coquilles ;
- `BallKey` avant/après RLE et preuve des doublons ;
- incidences fast/fallback, hits prévus/lus, resolver et ledger pré-DSU ;
- octets de chaque arène, workspace, pile, high-water et marge ;
- temps séparés build, source, census, resolver, fold, payload et
  `warm_e2e`.

Toutes les sommes et tailles sont calculées en entiers vérifiés avant cast
vers l'ABI device. Une erreur d'identité, un cas mathématique non supporté et
un refus de ressource sont trois statuts distincts.

## 13. Jalons

1. Étendre la porte de cardinalité reçue dans q2 au générateur partagé et à
   tous ses consommateurs, imposer le code zéro à chaque CTest nominal, puis
   recevoir la borne inférieure q2 et l'héritage de témoins par différentiel
   baseline, mutants ciblés et égalité de tous les sorts et masses.
2. Implémenter et comparer la route q2 Yao48/LBVH avec census fermé; conserver
   le self-join comme oracle ou second prune selon les masses.
3. Lier le reçu sidecar à une identité producteur vérifiée, tuer les mauvaises
   métadonnées et cibler le self-test interne à la factory; recevoir ce chemin
   uniquement comme oracle permanent `n<=32`.
4. Recevoir le prune de cellule avec sa portée exacte de branche et le
   conserver comme diagnostic adaptatif, hors chemin chaud.
5. Corriger la garde de bloc q4 `min D^2>0`, rendre le rejeu de blocs non
   vacu, puis recevoir le self-join cœur q3/q4 avec certificats dédupliqués et
   juge indépendant. Ajouter `L4`, héritage et frontière lossless; abandonner
   cette route produit si sa gate d'exposant mord avant tout sweep G4.
6. Prototyper ensuite la profondeur fermée terminale et le center-cover
   séparément, avec positivité et owner canonique reçus dans le constructeur
   aval.
7. Recevoir `BallActivation`, census fermé, tombstones et resolver contre Gamma
   exhaustif à petit `n`.
8. Porter les seules routes admises sur CUDA et mesurer source, certification,
   fold et payload dans un même `warm_e2e`.
9. Spécifier séparément les verticales ou conserver explicitement le contrat
   horizontal réduit.

## 14. Conditions de GO

Le backend G4 devient candidat uniquement si :

- les sources q2/q3/q4 ont une preuve de complétude sans atlas d'ordre
  supérieur caché ;
- toutes les identités CPU/device sont vertes sur les mêmes entrées ;
- les familles normales et dégénérées sont admises séparément ;
- aucun cap, timeout ou buffer plein ne publie un préfixe ;
- le pic mémoire réel tient l'enveloppe avec marge ;
- le gate secondaire demandé établit un p95 `warm_e2e<1 s`, puis la porte
  produit principale établit un p95 `warm_e2e<100 ms`; tous deux incluent
  index, source, certification, census, resolver, fold et payload ;
- le contrat de sortie et le statut des verticales sont nommés sans
  ambiguïté.

Jusque-là : `public_status=not_claimed`.

GCP non utilisé pour cette proposition.
