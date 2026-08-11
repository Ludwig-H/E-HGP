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
`q_cert` est une arité de support propre positif effectivement exhibée pour
renforcer l'inertie; l'absence d'un support plus grand n'est jamais déduite.

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

### 6.2 Supports q2 : paires diamétrales peu profondes

La boule d'un support q2 est la boule diamétrale de sa paire `(x,y)`. Un point
`z` est strictement intérieur si et seulement si
`(z-x) dot (z-y)<0`. Une activation q2 non tombstonée a au plus `K-1=9`
points strictement intérieurs.

La source candidate est un produit dual-tree canonique de deux LBVH :

1. chaque produit de nœuds représente un ensemble disjoint de paires non
   ordonnées ;
2. une borne entière sur boîtes cherche dix `PointId` distincts qui satisfont
   strictement le prédicat pour toutes les paires du produit ;
3. dix témoins certifient l'inertie H0 et suppriment le produit ;
4. sinon le produit se scinde; une feuille calcule exactement profondeur,
   coquille, clé et preuve.

La borne peut avoir des faux négatifs, jamais des faux positifs. Les contacts
restent aux feuilles. La complétude se prouve par la partition du self-produit
LBVH et par le fait qu'une paire non inerte ne peut rencontrer le certificat
de dix témoins.

Cette route ne matérialise ni matrice ni liste globale de paires. Son pire cas
reste quadratique. Avant CUDA, une sonde count-only doit donc publier
produits visités/scindés/prunés, feuilles, paires exactes, tombstones, coquilles,
octets, pile et high-water. La cible n'est admise que sur ces masses complètes.

Cette suppression vaut exclusivement pour la lane q2. Une paire dont la boule
diamétrale contient dix témoins peut rester le diamètre d'un support q3/q4 dont
le centre est décalé et dont la sphère exclut ces témoins. La fixture u16
explicite est consignée dans
[`AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md`](audits/AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md). Le résiduel q2 ne devient jamais, par
soustraction, la source d'ancres des arités supérieures.

### 6.3 Supports q3 : ancre de diamètre

Pour un support triangulaire propre positif, choisir canoniquement la plus
petite paire parmi ses côtés de longueur maximale `D`. Son circumcentre
appartient au plan médiateur de l'ancre et vérifie `h^2<=D^2/12`, où `h` est
la distance au milieu de l'ancre.

Pour une ancre reçue, chaque troisième point définit au plus un circumcentre
q3 dans ce disque. Le candidat est conservé seulement après vérification
exacte de l'arité positive, de l'ancre canonique, de la profondeur
`p<=K-2=8`, de l'owner et de la `BallKey`.

Cette réduction remplace les triples par `ancre x troisieme_point`, mais elle
n'est utile que si la source des ancres est elle-même sparse et complète.
Tester toutes les paires comme ancres n'est pas admis.

### 6.4 Supports q4 : sommets peu profonds dans un plan médiateur

Pour un support tétraédrique propre positif et une ancre diamètre `D`, le
théorème de Jung en dimension trois donne `h^2<=D^2/8`. Dans le disque
médiateur ainsi borné, chaque autre point induit une droite d'égalité de
puissance et un demi-plan où il est strictement intérieur.

Un q4 pertinent est une intersection de deux droites dont la profondeur
stricte est au plus `K-3=7`. La route candidate construit directement les
niveaux peu profonds de cet arrangement, au lieu de former toutes les paires
de droites. Elle valide ensuite indépendance affine, positivité, ancre
canonique, owner, census et clé.

Les parallèles, droites confondues, intersections multiples, points de
frontière et grandes coquilles sont des lots exacts, pas une hypothèse de
position générale. Le sweep doit les traiter ou refuser la route avant toute
sortie.

Le verrou de recherche est explicite : il manque encore un producteur sparse
et complet des ancres de diamètre, ainsi qu'une admission globale de la somme
des arrangements. Sans ces deux preuves, q3/q4 restent des propositions et non
un chemin produit.

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

Les sidecars `9483b1c` et `cbac109` ne sont pas reçus. Le second transmet bien
le type au fold et corrige plusieurs validations, mais son token vide reste
forgeable par `std::bit_cast`, son index manque `[r1,r2,r1]`, une entrée
`INT128_MIN` atteint un overflow signé et son digest est incomplet. La
réparation doit séparer deux usages :

- un oracle CPU borné peut rescanner tous les points et recalculer les
  miniboules afin de falsifier une source ;
- le chemin produit consomme les certificats streamés du producteur, avec
  count/fill, domaine de tâches complet et rejeu indépendant ciblé.

Le constructeur du reçu et toute capacité nécessaire doivent être privés,
non forgeables par copie binaire et possédés par le producteur terminal. La
preuve de complétude ne se réduit pas à `rank_bound>=point_count`. Les clés utilisent une fraction
multiprécision canonique; les digests utilisent une sérialisation champ par
champ et SHA-256; `q_min`, support, ordre et fermeture sont reconstruits ou
liés à une preuve rejouable.

## 11. Architecture G4 candidate

```text
points u16 + LBVH exact résidents
  |-> k=1 : Boruvka/EMST exact
  |-> q2 : self-produit dual-tree peu profond
  `-> q3/q4 : ancres diamètre + reporters médiateurs peu profonds
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

1. Corriger la forge fraîche, le doublon concentrique, l'entrée
   `INT128_MIN`, le support canonique et le digest complet; recevoir ensuite
   le sidecar seulement comme oracle borné.
2. Recevoir le prune de cellule avec sa portée exacte de branche, puis le
   conserver comme diagnostic et oracle adaptatif.
3. Corriger le différentiel compensable de la sonde q2 dual-tree, ajouter ses
   CTests et l'admettre ou la réfuter sur `terrain`, scanline simple,
   multiecho et dégénérescences. Son résiduel ne fournit jamais les ancres
   q3/q4.
4. Prouver une source sparse complète des ancres q3/q4; sans preuve, ne pas
   implémenter le sweep G4.
5. Recevoir `BallActivation`, tombstones et resolver contre Gamma exhaustif à
   petit `n`.
6. Porter les seules routes admises sur CUDA et mesurer source+fold+payload
   dans un même `warm_e2e`.
7. Spécifier séparément les verticales ou conserver explicitement le contrat
   horizontal réduit.

## 14. Conditions de GO

Le backend G4 devient candidat uniquement si :

- les sources q2/q3/q4 ont une preuve de complétude sans atlas d'ordre
  supérieur caché ;
- toutes les identités CPU/device sont vertes sur les mêmes entrées ;
- les familles normales et dégénérées sont admises séparément ;
- aucun cap, timeout ou buffer plein ne publie un préfixe ;
- le pic mémoire réel tient l'enveloppe avec marge ;
- `warm_e2e < 1 s` inclut index, source, certification, fold et payload ;
- le contrat de sortie et le statut des verticales sont nommés sans
  ambiguïté.

Jusque-là : `public_status=not_claimed`.

GCP non utilisé pour cette proposition.
