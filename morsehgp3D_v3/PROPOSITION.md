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

Il ne remplace pas le contrat Gamma de la ligne enregistrée. Une boule
H0-inerte peut encore porter des facettes, des incidences silencieuses ou une
application verticale. Tant que
ces informations ne sont pas reconstruites par une preuve séparée, la sortie
ne peut revendiquer ni le transcript Gamma enregistré ni la hiérarchie
verticale complète. `morsehgp3D_v2` reste un différentiel et une dépendance,
jamais l'autorité de cette décision.

Le `warm_e2e` officiel de la section 14.4 du plan de tests emploie
`BenchmarkOutputContract-v1`, qui matérialise dix forêts, applications
verticales, lots et certificat minimal. Le payload horizontal ci-dessus peut
avoir sa propre série diagnostique, mais son p95 ne ferme ni le seuil
secondaire d'une seconde ni le seuil principal de 100 ms du contrat officiel.

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
`distance_squared/4`. La route candidate extrait, pour chaque point, le plus
proche voisin exact selon la clé canonique dans chacune des 48 chambres Yao.
Le diamètre angulaire de chaque chambre est strictement inférieur à 60 degrés :
l'union non orientée de ces arêtes contient l'EMST canonique du graphe complet
et possède au plus `48n` candidats dirigés.

Le parcours peut être mutualisé avec les banques q2, mais son reçu est plus
fort : une chambre publie un candidat seulement après fermeture de toutes les
bornes plus petites et de tous les ex æquo canoniques, ou publie `empty` après
épuisement de tous les nœuds compatibles. Un budget interrompu ou une banque
q2 sous-pleine ne certifie jamais le vide. Après déduplication, Kruskal ou
Borůvka s'exécute sur le graphe sparse; les `n-1` arêtes finales sont triées
par `(distance_squared,PointId,PointId)` et chaque lot d'égalité est rejoué
atomiquement.

Le Borůvka point--LBVH reste un diagnostic exact borné, pas la route
industrielle : il répète les requêtes géométriques à chaque ronde. Le prior art,
sa preuve et le rejet de son prototype CPU à 50 k sont dans
[`AUDIT_REEMPLOI_EMST_YAO48_LIGNE_ENREGISTREE_20260811.md`](audits/AUDIT_REEMPLOI_EMST_YAO48_LIGNE_ENREGISTREE_20260811.md).

La réception compare, après chaque coupe stricte et fermée, les partitions
canoniques de `PointId` à l'oracle EMST CPU. Elle couvre les ex æquo, plusieurs
EMST valides et une mutation qui reconnecte les mauvaises composantes aux
bons niveaux. Cette lane évite entièrement le catalogue Morse à `k=1`.

### 6.2 Supports q2 : Yao48, LBVH et census terminal

La boule d'un support q2 est la boule diamétrale de sa paire `(x,y)`. Un point
`z` est strictement intérieur si et seulement si
`(z-x) dot (z-y)<0`. Une activation q2 non tombstonée a au plus `K-1=9`
points strictement intérieurs.

Le profil produit initial prévalant suppose des coordonnées distinctes. Une
paire de `PointId` colocalisés a un diamètre nul et ne constitue pas un support
propre positif q2 : le préflight la refuse avec l'entrée dupliquée, ou une
future extension l'agrège en site pondéré avant la construction. La capacité
du classifieur borné à compter de tels contacts est une robustesse de juge, pas
une permission de publier une activation dégénérée.

La source candidate reprend l'architecture exacte de
[`CATALOGUE_PAIRES_DIAMETRALES_EXACT.md`](../docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md).

La ligne enregistrée possède des composants CUDA séparés : une frontière
tuilée et reprenable, puis un classifieur `count--scan` sous un ancien contrat
de rang fermé. Ils constituent un prior art mécanique, pas une chaîne v3 : le
prune ancien admet une égalité radiale et le classifieur peut s'arrêter sur des
contacts, alors que v3 exige dix intérieurs stricts et le census fermé complet.
Leurs motifs structurels et transactionnels d'ownership, de tuiles, d'epochs,
de lease/reprise/backpressure, de ledger et de `count--scan` à offsets 64 bits
sont à réécrire puis à requalifier en u16. Leurs décisions sémantiques,
layouts, ABI et juges ne sont pas des autorités v3; leur périmètre et leurs
mesures sont inventoriés dans
[`AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md`](audits/AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md).

La cible v3 conserve les étapes suivantes :

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

Le remplissage des banques ne balaie pas aveuglément tout le LBVH jusqu'à ce
que les 48 chambres soient pleines : les chambres vides de bord rendraient ce
schéma quadratique. Les dix plus proches ne sont pas requis. Une antichaîne de
nœuds disjoints, entièrement certifiés dans une chambre et de masse totale dix,
fournit dix témoins; le maximum de distance AABB donne un `D` sûr. Les nœuds ne
sont raffinés que si réduire `D` promet une masse cible utile. Une banque qui
reste sous-pleine ne coupe rien. Les survivantes sont classifiées par lots avec
une frontière partagée; relancer la racine pour chaque paire n'est pas une
architecture admise.

Si une boîte cible peut rencontrer un ensemble conservateur de chambres `S`,
elle est encore prunable lorsque toutes les banques de `S` sont pleines et que
`dist^2(p,box)>3*max_{c in S} D_c`. En effet, l'échec d'au moins une des trois
coupes Yao implique `||q-p||^2<=3D_c`. Cette enveloppe multi-chambre reste
strictement fail-open aux égalités.

Les banques de la tuile active restent résidentes dans l'enveloppe
`O(B*48*K)`; aucune table globale `n*48*K` n'est admise. L'owner Morton d'une
paire l'émet toujours une seule fois. Il peut présenter un certificat Yao
centré sur l'autre extrémité seulement si la banque correspondante est déjà
disponible dans la même tuile ou dans un cache borné et authentifié. Cette
orientation inverse est facultative : son absence ne change ni la complétude
ni le ledger, et son coût doit être payé par un gain mesuré avant adoption.

Le ledger ferme simultanément
`candidate+certified_pruned+unresolved=C(n,2)`, la partition terminale
`below+exact+above`, la multiplicité canonique un et la liste fermée de chaque
record. Une frontière non vide refuse toute revendication d'exactitude. Le
chemin industriel sans budget la reprend avec backpressure jusqu'à fermeture;
il rend l'objet exact complet ou échoue sur une ressource physique réelle.

Les masses ferment également `pos(j)` ancre par ancre et engagent des
intervalles disjoints ou un digest canonique; une seule égalité globale ne peut
pas masquer une omission compensée par un doublon. Un reçu de région référence
une banque factorisée `(ancre, chambre, version)` au lieu de recopier ses dix
`PointId`. Les compteurs couvrent construction du LBVH, visites et pops de
banques, tas, parcours de prune, classification, tests ponctuels, piles,
records et octets réels. Un cap de probe peut abandonner un prune et retomber
fail-open; aucun `max_work` configurable ne refuse le chemin produit.

Le self-join AABB par témoins communs est une seconde preuve exacte : chaque
état représente un ensemble disjoint de paires et dix témoins universels
suppriment le bloc. Il reste un oracle indépendant, un falsificateur de masses
ou un second prune tant que ses compteurs ne battent pas Yao48/LBVH. Le juge
borné peut tenir un sort quadratique à petit `n`; aucun chemin produit ne
matérialise de matrice ou de liste globale de paires. Le pire cas reste
quadratique en sortie.

`smax` ne transforme pas ce pire cas en graphe de degré borné. Dans l'espace
euclidien, pour tout `m`, un point `p` et `m` points distincts `q_i` sur une
sphère centrée en `p` vérifient, pour `j!=i`,

$$\Phi_{p,q_i}(q_j)=R^{2}\left(1-\cos\theta_{ij}\right)>0.$$

Les `m` paires `p q_i` ont donc toutes le rang fermé deux. En ajoutant `h`
points communs strictement intérieurs sur l'axe d'une petite calotte de
directions, la même construction donne un degré arbitraire dans le bucket
exact `closed_rank=h+2`; ceci vaut notamment au rang exact 11. Le kissing
number 12 est inapplicable et `smax=11` borne le contenu d'un record, jamais le
degré de `p`. Sur la grille u16 finie, les caps triviaux sont `n-1` et
`2^48-1`; deux constructions documentées à treize voisins réfutent le cap 12.
Le statut de leur gate appartient à l'audit live. Un record q2 de rang `R<=11`
propose localement au plus neuf
triplets et 36 quadruplets de même miniboule; cela ne fournit aucune source
complète pour q3/q4. L'audit et les fixtures u16 permanentes demandées sont dans
[`AUDIT_DEGRE_GABRIEL_KISSING_SMAX11_20260811.md`](audits/AUDIT_DEGRE_GABRIEL_KISSING_SMAX11_20260811.md).

Une baseline probabiliste peut dimensionner un régime favorable sans devenir
un contrat. Sous le Palm d'un processus de Poisson homogène simple dans
`R^d`, le degré moyen cumulé jusqu'à `h` tiers diamétraux vaut
`2^d(h+1)`. En dimension trois et sous `smax=11`, il vaut donc 80, soit 8 par
bucket exact. Cette moyenne ne borne ni le maximum, ni la queue, ni les paires
inspectées, et n'est pas exacte pour une fenêtre tronquée, la grille u16 ou les
familles G4 du dépôt.

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
l'infimum exact sur le produit cartésien entier des trois AABB. Pour chaque couple `x,y`, prendre
`w0=clip(floor((x+y)/2),[w_min,w_max])` et évaluer
`4*(w0-x)*(w0-y)`; lorsque `x+y` est impair, l'autre entier voisin donne la
même valeur s'il appartient à l'intervalle. Le minimum sur les quatre couples,
puis la somme des axes, est au moins l'infimum continu et reste une borne sûre
pour les `PointId` réellement contenus dans les nœuds, dont le sous-ensemble
peut être plus clairsemé. Son admission exige un différentiel exhaustif borné,
notamment sommes impaires et clips aux deux bords; aucune arithmétique flottante
n'entre dans la décision.

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

Une route par ancre évite le rescan d'un arbre de témoins pour chaque paire.
Pour une ancre `p`, un témoin `w`, une cible `q`, poser `s=w-p`, `d=q-p` et
`A=d dot s-||s||^2`. Le témoin est universel sous les tests :

$$\text{q3: }A>0\ \text{ et }\ 3A^2>\lVert d\mathbin{\times}s\rVert^2,\qquad\text{q4: }A>0\ \text{ et }\ 2A^2>\lVert d\mathbin{\times}s\rVert^2.$$

Pour `p,w` fixes et un nœud AABB de cibles, `A_min` s'obtient exactement par
choix d'extrémités. La fonction convexe `||d cross s||^2` atteint son maximum
sur l'un des huit sommets. Les mêmes comparaisons avec ces deux bornes peuvent
donc créditer le témoin pour tout le nœud; une égalité ou une boîte indécise
descend. Une banque de neuf ou huit `PointId` distincts certifie le nœud sans
partager ni univers ni sort avec q2. Aucun nombre fixe de banques
directionnelles n'est affirmé complet : ce `Jung--Yao target range` est un
certificat fail-open à mesurer.

Pour un produit général de boîtes d'extrémités et de témoins, une borne plus
orientée utilise `g_min=D2_min-U2_max` et une majoration entière `Q_max` de
`||d cross U||^2` par intervalles. Le bloc est universel sous `g_min>0` puis
`3*g_min^2>4*Q_max` en q3 ou `g_min^2>2*Q_max` en q4. Toute incertitude et
toute égalité descendent. Cette borne doit être reçue contre les huit coins et
un juge ponctuel indépendant avant de remplacer le préfiltre norm-only.

La profondeur fermée de demi-boule fournit un filtre terminal complémentaire.
Si `P={z:(z-a) dot (z-b)<0}` et `delta(a,b)` est le minimum du nombre de
projections de `P` dans un demi-plan fermé du plan médiateur, toute sphère de
coquille contenant `a,b` possède au moins `delta` intérieurs. Les seuils sont
`delta>=9` en q3 et `delta>=8` en q4, à condition que la `BallKey` porte un
support propre positif q3/q4 certifié contenant la paire. `q_min=2` ou un
`q_cert=2` seul n'autorise pas ces seuils; une même `BallKey` avec un
`q_cert=3/4` certifié peut en bénéficier. La lane q2 emploie séparément le total
`|P|`; ses survivants et ceux de q3/q4 ne sont pas emboîtés.

Le center-cover par 64 patches intervient plus tôt : `P15-HOCUDA-P1` est le
candidat de complétude par blocs q3/q4, pas un filtre terminal interchangeable.
Il partitionne implicitement toutes les paires, couvre extérieurement leur
domaine de centres de Jung et ne supprime un bloc que si chaque patch non
certifié infaisable possède 9/8 `PointId` stricts certifiés. Un patch ou un range-query ambigu
force le partage du bloc. Sa première tranche `P15-HOCUDA-P1a` profile seulement
le prune q4 au seuil huit et n'émet aucune ancre. Elle ferme uniquement
`pruned_mass+microtile_mass=C(n,2)`; elle ne prouve pas la complétude de P1.
Les preuves ponctuelles ci-dessous ne peuvent remplacer cette fermeture
globale. La spécialisation P1a q4 en arithmétique u16 exacte, avec coins
rationnels évalués à l'échelle seize, range-query collective et juge bijectif,
est fixée dans
[`NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY_20260811.md`](audits/NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY_20260811.md).

Un certificat collectif borné peut précéder le sweep complet. Pour un point
`z`, poser `V_z=2z-a-b` et `g_z=D^2-||V_z||^2`. Tout centre de sphère passant
par `a,b` s'écrit `M+t` avec `t` orthogonal à `d`, et la marge intérieure de
`z` vaut

$$\frac{g_z}{4}+V_z\mathbin{\cdot}t.$$

Pour chaque point, le mauvais côté fermé est
`B_z={t:g_z+4 V_z dot t<=0}`. Un groupe couvre le disque de Jung exactement si
l'intersection du disque avec tous ses `B_z` est vide. Par Helly dans le plan,
toute couverture possède un sous-groupe de trois `PointId` au plus. Neuf
groupes disjoints certifient q3 et huit certifient q4, avec au plus 27 ou 24
identifiants. Un greedy est sûr mais incomplet; son échec conserve la paire.
L'égalité au bord reste mauvaise. Le solveur rationnel, les formules entières
et les limites sont dans
[`NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md`](audits/NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md).
Le test `0 in conv{Proj(V_z)}` avec tous les `g_z>0` demeure un cas particulier
qui couvre tout le plan. Les groupes et les témoins individuels ne peuvent
partager aucun `PointId` lorsqu'ils additionnent leurs crédits.

Une autre composition exacte réutilise les crédits singleton. Si `C` contient
`c` témoins Jung universels distincts et si la profondeur est calculée sur les
témoins diamétraux `P` privés de `C`, chaque sphère admissible contient au
moins `c+delta(P minus C)` points stricts. La soustraction de `C` et la
déduplication des `PointId` sont indispensables : additionner le cœur et une
profondeur calculée sur `P` compterait deux fois les mêmes témoins. Cette
composition peut gagner des cas où aucun certificat seul n'atteint 9/8, mais
elle ne supprime pas le coût de collecte par paire.

Le noyau produit commun de `delta` reçoit un rayon par `PointId` distinct et
la banque scalaire `always`. Des identifiants distincts peuvent avoir des
rayons confondus, mais un même identifiant ne peut jamais être réinjecté. Les
adaptateurs de projection peuvent employer `r=d cross V` ou une base entière
équivalente, mais aucun consommateur produit ne conserve une seconde copie du
tri et du sweep. Le noyau trie les rayons par
demi-tour et produit croisé, puis balaie l'arc semi-ouvert
`[theta_i,theta_i+pi)` à deux pointeurs. Rayons confondus inclus et antipodes
exclus donnent

$$\delta=\mathrm{always}+m-\mathrm{max\_open}$$

en `O(m log m)`. Lorsqu'un noyau partagé est présent, sa qualification et son
pincement appartiennent à l'audit live. Le juge reste une minimisation fermée
quadratique `n<=32`, avec une autre collecte et
une autre base, afin qu'un défaut du tri ou des frontières ne soit pas rejoué
par le même code. Chaque adaptateur engage aussi une borne d'amplitude : l'API
générique en `int64` ne suffit pas à prouver que ses produits `i128` ne
débordent pas.

Le total diamétral q2 et la profondeur q3/q4 ont des résiduels incomparables.
Une seule machine peut partager l'arbre et la partition des paires, mais chaque
lane conserve son propre sort, son ledger et ses compteurs. Le cœur seul, la
profondeur seule et leur combinaison doivent toujours être reçus et mesurés
séparément. Leur état d'implémentation appartient exclusivement à
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md).

Cette preuve donne la couverture, pas la parcimonie. Le nombre d'ancres peut
rester quadratique et une recherche naïve des témoins cubique. Hors du
falsificateur P1a, une source ne devient candidate produit qu'après fermeture
du ledger de l'univers implicite complet et admission des masses à
`12 500/25 000/50 000`. P1a suit son protocole distinct : différentiel à
`n=32`, puis profil direct à 50 k sans palier. Un cap appartient
seulement au falsificateur diagnostique et ne tronque jamais un résultat
produit. Chaque reçu de prune déduplique ses
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

L'hypothèse diamètre doit être vérifiée sur les six arêtes du support. Le fait
que deux carriers appartiennent séparément à la lentille de l'ancre ne borne
pas leur distance mutuelle. Toute intersection shallow doit donc vérifier que
la paire ancre reste maximale, ou retomber fail-open. La contre-fixture
permanente est dans
[`AUDIT_JUNG_ANCHOR_389A742.md`](audits/AUDIT_JUNG_ANCHOR_389A742.md); le
statut du prototype correspondant appartient à l'audit live.

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
d'une table. Le reçu engage contrat, version sémantique et provenance vérifiée
du producteur, profil, schéma et identité des tâches, counts
prévus/remplis/consommés, statut terminal sans censure, points, catalogue et
paramètres. La provenance distingue explicitement version, source/manifeste,
ELF et options de build; le hash d'un littéral ne les remplace pas. Les clés
sont multiprécision et canoniques; les digests emploient une sérialisation champ
par champ, taggée, versionnée et SHA-256. Le digest lie le reçu aux données; le
ledger et les certificats en prouvent la portée.

Les défauts précis d'une livraison n'appartiennent pas à cette proposition
durable; ils sont tenus dans
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md).

## 11. Architecture G4 candidate

```text
points u16 + LBVH exact résidents
  |-> k=1 : Yao-1 exact mutualisé -> EMST sparse
  |-> q2 : Yao48 strict + classification LBVH et census fermé
  `-> q3/q4 : center-cover de blocs complet et fail-open
       -> banques Jung--Yao + groupes de Helly + profondeur terminale
       -> range-report q3 + niveaux shallow q4
       -> BallActivation/tombstones streamées
       -> sort/RLE par BallKey exacte
       -> carriers stricts + resolver latent
       -> fast/fallback recertifiés par lot
       -> composantes, verticales et payload officiel nommé
```

Aucun tableau global de tuples, paires, cellules, faces ou cofaces ne persiste.
Chaque kernel a un count exact, une arène dimensionnée ou un segment
reprenable, un fill et une identité de consommation. Les segments ne coupent
ni une `BallKey`, ni un lot exact, ni une unité de recertification. Une
insuffisance physique refuse atomiquement; aucun budget configurable ne publie
un préfixe.

Les cellules adaptatives et l'oracle exhaustif restent hors du chrono produit.
Ils recertifient des échantillons et des fixtures, puis comparent digests,
masses et décisions à la source device.

Pour le seul diagnostic horizontal `warm_e2e_h0_v3_diagnostic`, l'enveloppe de
falsification provisoire est :

| tranche | enveloppe chaude |
| --- | ---: |
| transfert + LBVH | 40 ms |
| source + cover | 200 ms |
| cordes | 200 ms |
| shallow + décision exacte | 300 ms |
| reducer + payload | 200 ms |
| réserve | 60 ms |

Source, cover et cordes au-dessus de 400 ms chaud classent cette route no-go.
Ces enveloppes sont des seuils de falsification d'architecture, pas une
qualification. Elles ne ferment ni la seconde secondaire ni les 100 ms
principaux du contrat officiel. Celui-ci exige en plus les dix forêts, les
verticales, les lots, le certificat minimal et le retour hôte dans le même p95;
aucune enveloppe par tranche n'est revendiquée avant leur architecture.

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

1. Conserver générateur, self-joins, sidecar borné, cellules et ancres comme
   portes locales ou oracles. Fermer les identités persistantes et les juges
   indépendants encore ouverts sans promouvoir ces parcours exhaustifs.
2. Pour `k=1`, conserver le Borůvka point--LBVH comme diagnostic et produire
   le transcript Yao-1 exact mutualisé avec q2 : fermeture des ex æquo et du
   vide par chambre, au plus `48n` candidats, réduction sparse puis tri des
   arêtes finales par niveau.
3. Réemployer les motifs de lease, ledger et `count--scan` de la ligne
   enregistrée, sans copier ses layouts binary64 ni ses décisions de rang
   fermé. Fermer q2 par Yao48 strict fail-open, classification terminale et
   census fermé multi-ordre; conserver le self-join comme oracle ou second
   prune selon les masses.
4. Porter et requalifier `P15-HOCUDA-P1a` en mass-only q4 : partition triangulaire
   implicite, 64 patches, seuil huit, range-query collective, microtuiles
   terminales et ledger complet. Cette tranche n'émet aucune ancre.
   Après le différentiel hôte à `n=32`, la même session G4 ferme la parité
   native et `n=32` sous Compute Sanitizer, puis va directement au profil 50 k.
   Une masse majoritairement terminale, un rescan par bloc ou une queue lourde
   arrêtent la route avant son extension à P1.
5. Sur les seules ancres admises, recevoir séparément Jung--Yao, la borne AABB
   `g_min/Q_max`, Helly, la composition cœur--profondeur et la profondeur
   terminale. Mesurer le gain marginal de chacun contre son coût exact.
6. Construire les range-reports q3 et les niveaux shallow q4 sans développer
   tous les triples ou quadruples, puis recevoir owner, positivité et census.
7. Recevoir `BallActivation`, census fermé, tombstones, resolver, fold et
   reconstruction des verticales contre Gamma exhaustif à petit `n`.
8. Installer deux harnesses nommés : le diagnostic horizontal réduit et le
   `BenchmarkOutputContract-v1` officiel. Ils ne partagent aucun verdict SLO.
9. Appliquer la gate `12 500/25 000/50 000` aux autres routes de source,
   porter seulement les routes admises sur CUDA avec arènes préallouées et une
   synchronisation terminale, puis mesurer le payload officiel complet dans
   un même `warm_e2e`.

## 14. Conditions de GO

Le backend G4 devient candidat uniquement si :

- les sources q2/q3/q4 ont une preuve de complétude sans atlas d'ordre
  supérieur caché ;
- toutes les identités CPU/device sont vertes sur les mêmes entrées ;
- les familles normales et dégénérées sont admises séparément ;
- aucun cap, timeout ou buffer plein ne publie un préfixe ;
- le pic mémoire réel tient l'enveloppe avec marge ;
- le gate secondaire demandé établit un p95 `warm_e2e<1 s`, puis la porte
  produit principale établit un p95 `warm_e2e<100 ms`; pour le SLO officiel,
  tous deux utilisent exactement `BenchmarkOutputContract-v1`, dont dix
  forêts, verticales, lots et certificat minimal ;
- toute série horizontale réduite est nommée séparément et ne revendique aucun
  de ces deux seuils officiels.

Jusque-là : `public_status=not_claimed`.

GCP non utilisé pour cette proposition.
