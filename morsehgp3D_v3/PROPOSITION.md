# MorseHGP3D v3 — proposition d'architecture courante

Date : 13 août 2026 UTC.

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

Le `warm_e2e` officiel de la section 14.4 du plan de tests porte sur une famille
volumique favorable dont le certificat reste sparse. Il emploie
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
7. Le domaine exact est fermé avant la source : positions distinctes pour les
   preuves Yao-1, puis porte régulière certifiée, quotient de plateau reçu ou
   refus explicite `unsupported_degeneracy`. `RelevantGP` seul ne certifie pas
   toutes les incidences silencieuses. Une coquille ou un support dégénéré
   n'est jamais tronqué pour rentrer dans `smax`.

## 3. Faits qui imposent le changement de source

La session mass-only a été exécutée sur une VM de type G4 avec 48 threads CPU;
aucun kernel ni chronométrage CUDA n'a eu lieu et aucun tuple n'a été formé.
Sa provenance est
[`NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md`](audits/NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md),
avec les sorties brutes
[`cell_50k_raw.txt`](receipts/g4_massonly_20260811/cell_50k_raw.txt) et
[`mask_scale_raw.txt`](receipts/g4_massonly_20260811/mask_scale_raw.txt).
Après le prune d'axe, l'ordonnance par cellules conserve :

| lane | minimum | maximum |
| --- | ---: | ---: |
| q2 | 465 371 500 | 2 862 879 000 |
| q3 | 14 667 530 000 | 131 762 100 000 |
| q4 | 330 437 400 000 | 9 968 861 000 000 |

Les seuls top-t, dilations et comptes prennent 0,174--29,153 s sur 48 threads.
Ce reçu refuse le port combinadique/materialisant mesuré sur les maillages
cellulaires des familles terrain et scanline, aux pas six et dix; faute de borne
minimale d'octets ou d'opérations par tuple, il ne
prouve pas qu'aucune ordonnance implicite ne pourrait couvrir les mêmes
univers. q2 n'est pas « admise » : elle est seulement moins massive que q3 et
q4.

L'énumération combinadique/materialisante q4 par triples n'est pas une baseline
industrielle. La masse R3 publiée appartient à la lane q3 et ne peut pas lui être
imputée. En revanche, la masse q4 suffit à réfuter cette ordonnance : même le
schéma canonique des trois plus petits identifiants impose, au pas six, plus de
2,74 milliards, 10,63 milliards et 1,02 milliard de triples selon la famille,
avant range-report, census et fold. Ce reçu de masse ne juge pas une requête
implicite ponctuelle de premier croisement; il refuse le catalogue de triples
comme route 50 k.

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

### 4.1 Réparation du K-graphe de Gabriel

La proposition 6 et le théorème 5 du manuscrit sont faux pour le K-graphe de
Gabriel brut. Une coface non-Gabriel peut être une continuation H0 sans delta
de points tout en rattachant une facette simultanée qu'une coface Gabriel
réutilise plus tard. E5 grave ce défaut à l'ordre deux.

Sous support minimal unique et essentiel, intérieurs stricts et absence
d'égalité extérieure, la réparation exacte est `G_k^+`. Pour une coface
non-Gabriel `Q`, de support `U`, poser `I=Q\U`, choisir `u_0` dans `U` et
ajouter au niveau `beta(Q)` une arête de `Q\{u_0}` vers `Q\{x}` pour chaque
`x` dans `I`. Toutes les facettes obtenues en supprimant un élément du support
sont déjà connectées strictement avant ce niveau. L'étoile précédente induit
donc exactement la même équivalence que la coface Gamma complète. Après
contraction atomique de chaque lot, `G_k^+` et Gamma ont les mêmes composantes
de facettes aux coupes ouvertes et fermées. Un MSF de `G_k^+`, avec les poids
de naissance des sommets conservés séparément, fournit le théorème 5 corrigé.

Ce correctif est une autorité exhaustive : il doit encore connaître toutes
les cofaces. Le chemin produit ne le matérialise pas. Il part des cofaces
directes terminales, déduplique leurs facettes du cœur et classe les intrus
stricts `J_F` de chaque facette. Les cas zéro et un sont fermés par les
cofaces directes, avec tous les co-minimiseurs exacts requis dans le cas zéro;
à partir de deux intrus, une seule gateway canonique vers un carrier de niveau
strictement inférieur remplace l'effet H0 des co-minimiseurs silencieux. Le
carrier doit être résolu même s'il est extérieur au cœur.

Une table terminale complète de Source S retire les requêtes négatives de
gateway. Pour une facette `F`, écrire `r=|F intersection int(B_F)|`,
`j=|(X minus F) intersection int(B_F)|` et `q` pour le support minimal de sa
miniboule. Si sa clé est absente de la table des boules `p+q<=11`, alors
`r+j+q>=12`; or `r+q<=|F|<=10`, donc `j>=2`. Si la clé est présente, son census
donne `J_F` exactement. Seule la branche absente lance une recherche garantie
positive, arrêtée après deux intrus. Cette dichotomie dépend entièrement de la
complétude de Source S.

En dimension trois, seules les suppressions des points du support d'un
événement direct donnent des bras stricts; il y en a au plus quatre avant
déduplication. Cette borne locale ne borne ni le nombre d'événements directs,
ni le coût des census, ni le SLO. La composition sparse exige encore une
source directe complète, une porte régulière couvrant les cofaces omises ou
leur inertie de haut rang, tous les co-minimiseurs nécessaires, un resolver
terminal et des lots atomiques. Sa sortie est un MSF de carriers ou un fold
équivalent pour `normalized_horizontal_h0`, jamais un K-MST de Gabriel brut ni
le payload Gamma. L'énoncé complet est dans
[`AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md`](audits/AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md).

Les supports multiples ne se réparent pas en choisissant un pivot dans leur
union : sa suppression peut conserver le même niveau. Dans la fenêtre utile,
ils exigent un quotient de plateau certifié ou un refus; au-dessus de la
fenêtre, l'inertie du bloc saturé peut suffire. Le comparateur `Sphere` actuel
emploie correctement six limbs, soit 384 bits, pour une borne de produit croisé
inférieure à `2^326`. Une nouvelle clé de Gram réduite ramènerait cette borne
u16 sous 256 bits, mais exige son propre constructeur, pgcd, codec et ses propres
portes; elle ne permet pas de raccourcir le layout actuel. Le fold traite chaque
égalité atomiquement sur les racines pré-lot gelées. Les preuves de largeur et
le contrat minimal du quotient normalisé sont dans
[`AUDIT_REPONSES_ROUTE_SPARSE_GABRIEL_20260812.md`](audits/AUDIT_REPONSES_ROUTE_SPARSE_GABRIEL_20260812.md).

La réalisation sparse candidate est détaillée dans
[`NOTE_IMPLEMENTATION_SPARSE_COMPLETE_GABRIEL_GATEWAYS_20260812.md`](audits/NOTE_IMPLEMENTATION_SPARSE_COMPLETE_GABRIEL_GATEWAYS_20260812.md) :
source directe terminale, facettes du cœur, trois branches de première
incidence, resolver strict, MSF de carriers puis reconstruction atomique. Sa
complétude reste conditionnelle à la porte de rétraction; elle n'est pas reçue
par le probe de dimensionnement courant.

## 5. Objet streamé

Le chemin produit ne transporte pas un `CriticalSphere(rank<=32)` global. Il
stream des records à coquille variable, réunis par clé exacte :

```text
BallActivation
  BallKey canonique multiprécision
  niveau exact et owner de source
  support propre positif et provenance rejouable
  intérieurs stricts I et shell fermé E, ou preuve suffisante de tombstone
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

Une `BallKey` ne contracte pas seule une coquille dégénérée. Pour
`I_B=X cap interior(B)`, `U_B=X cap boundary(B)` et un support minimal `S`
inclus dans `U_B`, les cofaces directes portées par la boule sont les
`I_B union A` tels que `A` est inclus dans `U_B` et contient un support positif
du centre. Le record `(S,B)` ne représente que `A=S`. Dans la branche régulière
`U_B=S`, il donne l'unique événement direct minimal; sinon il faut un quotient
de plateau reçu ou un refus de rang pertinent.

Le cas terminal `k=n` reste obligatoire dans le certificat de totalité même en
l'absence de coface de rang `n+1`. Dans `full_pi0`, la facette `X` naît comme
composante isolée au niveau prescrit. Pour `n>1`, `hgp_reduced` conserve son
carrier latent mais publie une forêt horizontale vide : il ne faut pas inventer
un lot de coface ni un nœud public.

## 6. Source par arité

### 6.1 Ordre un : EMST exact

L'ordre un est exactement le single linkage, au niveau
`distance_squared/4`. Sur le profil initial à positions 3D deux à deux distinctes, la route
candidate extrait, pour chaque point, le plus proche voisin exact selon la clé
canonique dans chacune des 48 chambres Yao.
Le diamètre angulaire de chaque chambre est strictement inférieur à 60 degrés :
l'union non orientée de ces arêtes contient l'EMST canonique du graphe complet
et possède au plus `48n` candidats dirigés.

Équivalemment, tout EMST est contenu dans le Gabriel fermé de rang deux, donc
dans le graphe diamétral `I_B=empty`. Cela prouve que q2 profond est inutile à
`k=1`, mais ne recommande pas de matérialiser Gabriel : son degré 3D n'est pas
borné et sa taille peut être quadratique. Yao-1 est la compression sparse
retenue.

Le vecteur nul n'appartient pas à ce raisonnement directionnel. Le contrat
courant refuse les positions dupliquées; une future politique
`duplicate_policy=aggregate` forme un site canonique pondéré par position, puis
applique Yao-1 aux sites distincts. Elle ne conserve donc pas plusieurs
`PointId` colocalisés dans le graphe. Une sémantique hors contrat qui voudrait
conserver chaque occurrence devrait publier au niveau zéro l'étoile canonique
depuis le plus petit `PointId`, contracter chaque classe, puis appliquer Yao-1
aux représentants minimaux. Sa borne serait `(n-m)+48m<=48n` pour `m` classes,
et son ledger directionnel porterait `48m` slots.

Le parcours peut être mutualisé avec les banques q2, mais son reçu est plus
fort : une chambre publie un candidat seulement après fermeture de toutes les
bornes plus petites et de tous les ex æquo canoniques, ou publie `empty` après
épuisement de tous les nœuds compatibles. Un budget interrompu ou une banque
q2 sous-pleine ne certifie jamais le vide. Après déduplication, Kruskal ou
Borůvka s'exécute sur le graphe sparse; les `n-1` arêtes finales sont triées
par `(distance_squared,min_PointId,max_PointId)` et chaque lot d'égalité est rejoué
atomiquement.

Le Borůvka point--LBVH reste un diagnostic exact borné, pas la route
industrielle : il répète les requêtes géométriques à chaque ronde. Le prior art,
sa preuve et le rejet de son prototype CPU à 50 k sont dans
[`AUDIT_REEMPLOI_EMST_YAO48_LIGNE_ENREGISTREE_20260811.md`](audits/AUDIT_REEMPLOI_EMST_YAO48_LIGNE_ENREGISTREE_20260811.md).

Le read-off q2 du snapshot `e6f1ef3` est un second diagnostic, pas un
remplacement de Yao-1. Il collecte les paires à intérieur diamétral ouvert vide,
qui forment un sur-graphe du Gabriel fermé contenant l'EMST, puis compare les
poids à Prim. Il exécute auparavant toute la source q2/q3/q4 et ne publie que
`d^2=4 beta`, sans endpoints ni multifusions. Son vert reçoit un multiensemble
de poids borné, jamais un producteur H0 ou un coût 50 k.

La réception compare, après chaque coupe stricte et fermée, les partitions
canoniques de `PointId` à l'oracle EMST CPU. Elle couvre les ex æquo, plusieurs
EMST valides et une mutation qui reconnecte les mauvaises composantes aux
bons niveaux. Cette lane évite entièrement le catalogue Morse à `k=1`.

### 6.2 Supports q2 : cascade Yao, affine, dual et census terminal

La boule d'un support q2 est la boule diamétrale de sa paire `(x,y)`. Un point
`z` est strictement intérieur si et seulement si
`(z-x) dot (z-y)<0`. Une activation q2 non tombstonée a au plus `K-1=9`
points strictement intérieurs.

Le profil produit initial prévalant suppose des positions 3D deux à deux distinctes. Une
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

1. des tuiles d'ancres remplissent 48 banques directionnelles sur le LBVH
   Morton résident : `K=10` pour les dix plus proches certifiés, ou
   `K+1=11` pour un réservoir arbitraire dont une cible ponctuelle doit être
   exclue avant d'engager dix témoins;
2. les trois inégalités entières de Yao48, toutes strictes pour la lane H0,
   certifient dix témoins strictement intérieurs; une coupe cône--boîte exacte
   traite collectivement un masque de chambres, toute sous-plénitude ou égalité
   restant fail-open;
3. dix témoins ponctuels d'une banque chaude appliquent directement leur forme
   affine sur la boîte cible; ce certificat exact traite les cas que
   l'enveloppe Yao laisse non résolus;
4. un parcours dual-tree persistant cherche une antichaîne de nœuds témoins
   disjoints dont le minorant `L_p(Q,W)` est strictement positif et la masse au
   moins dix; aucun rescan racine ni matrice cible--témoin n'est admis;
5. le classifieur terminal résiduel parcourt le LBVH avec des bornes exactes sur
   `(z-x) dot (z-y)` et s'arrête à dix intérieurs seulement pour une tombstone;
6. toute paire conservée finit le census et publie la liste fermée complète
   `C(x,y)`, sa profondeur stricte, sa coquille, son rang, son niveau et sa
   `BallKey`, dans une seule passe multi-ordre.

Une primitive exacte GPU-friendly peut simplifier le remplissage Yao. Pour une
orientation signée et permutée, écrire les coordonnées de `d=q-p` dans la
chambre comme `r_1>=r_2>=r_3>=0` et poser

$$T(d)=(r_1-r_2,r_2-r_3,r_3).$$

La transformée est unimodulaire et linéaire dans cette orientation : `q` est
dans la chambre de `p` si et seulement si `T(q)>=T(p)` coordonnée par
coordonnée. En coordonnées `t=T(d)`, la distance est

$$Q(t)=(t_1+t_2+t_3)^2+(t_2+t_3)^2+t_3^2,$$

croissante sur l'orthant positif. Un AABB transformé incompatible est rejeté si
un upper est inférieur à `T(p)`; sinon son minorant exact est
`Q(max(lo-T(p),0))`. Un LBVH Morton en coordonnées transformées peut donc faire
les top-10 exacts par dominance, avec tie-break `(d^2,PointId)`. Les 48
orientations se streament sans matrice paire--chambre. Cette réduction est une
candidate à mesurer, pas une borne sublinéaire; elle exige des fixtures de
frontière de chambre et de doublons inter-orientations.

Le remplissage des banques ne balaie pas aveuglément tout le LBVH jusqu'à ce
que les 48 chambres soient pleines : les chambres vides de bord rendraient ce
schéma quadratique. Les dix plus proches ne sont pas requis. Une antichaîne de
nœuds disjoints, entièrement certifiés dans une chambre et de masse totale au
moins onze, fournit un réservoir arbitraire cible-indépendant; chaque reçu en
engage dix après exclusion de la cible. Une antichaîne déjà certifiée disjointe
de sa cible ou de sa boîte peut se limiter à une masse dix. Le maximum de
distance AABB donne un `D` sûr. Les nœuds ne sont raffinés que si réduire `D`
promet une masse cible utile. Une banque qui reste sous-pleine ne coupe rien.
Les survivantes sont classifiées par lots avec une frontière partagée;
relancer la racine pour chaque paire n'est pas une architecture admise.

Une banque certifiée des `K=10` plus proches de l'ancre dans la chambre n'a pas
besoin d'un onzième candidat. Si la cible `q` appartient à ce top-10, tout
témoin strict `w` vérifie `||w-p||<||q-p||`; il ne peut donc pas en exister dix
dans cette chambre. Remplacer `q` par le onzième, encore plus loin, ne sauve
aucun prune. Passer ce mode exact à onze ne fait qu'augmenter le remplissage.

Un réservoir arbitraire, notamment issu d'une antichaîne, peut en revanche
contenir `q` sans contenir les dix autres témoins disponibles. Il conserve
alors `K+1=11` candidats, exclut la cible et recalcule `D` sur les dix engagés.
La table factorisée de onze candidats reste immuable. Chaque reçu Yao porte un
masque de onze bits dont exactement dix sont levés; chaque entrée d'un reçu
radial porte de même `(bank_index,engagement_mask)`. Le juge exige exactement
dix bits levés, recalcule `D`, l'identité des témoins et les inégalités
strictes. Un état `engaged` mutable partagé par plusieurs cibles est interdit.

Pour une boîte de cibles, un masque d'engagement commun pris dans onze
candidats n'est recevable que si la boîte contient au plus un identifiant de la
banque. Si elle en contient deux ou davantage, aucun choix uniforme de dix ne
les exclut tous : il faut scinder la boîte, employer une banque certifiée
disjointe ou échouer ouvert. Cette précondition vaut pour les reçus Yao,
radiaux et affines.

Pour dix témoins ponctuels immuables `w_j`, définir
`h_j(q)=(q-p) dot (w_j-p)-||w_j-p||^2`. La fonction est affine en `q`; son
minimum exact sur une boîte choisit par axe l'extrémité déterminée par le signe
de `w_j-p`. Dix minima strictement positifs certifient toute la boîte. Pour la
même banque engagée, ce test domine la coupe Yao, mais sa couverture globale
n'est pas prouvée. Les dix identifiants sont distincts, différents de l'ancre
et disjoints de toute la plage cible. Le reçu porte version, masque et dix
minima; un échec passe au dual-tree sans classer la boîte.

Pour une boîte de cibles `Q`, une antichaîne de nœuds témoins `W` peut remplacer
la chambre. La fonction à minorer est :

$$A(p;q,w)=(q-p)\mathbin{\cdot}(w-p)-\left\Vert w-p\right\Vert^{2}.$$

Pour l'axe `i`, soient `E_i^Q` et `E_i^W` les deux extrémités des intervalles
de `Q-p` et `W-p`. La borne AABB exacte est :

$$L_p(Q,W)=\sum_{i=1}^{3}\min_{\alpha\in E_i^Q,\,\beta\in E_i^W}\left(\alpha\beta-\beta^{2}\right).$$

Les plages témoins créditées sont deux à deux disjointes, hors de la plage
cible `Q` et hors de l'ancre `p`; leurs masses exactes totalisent au moins dix.
Si chaque crédit vérifie `L_p(Q,W)>0`, tous ses vrais `PointId` sont strictement
intérieurs pour toute cible de `Q`. Toute égalité descend. Cette voie dual-tree
utilise toutes les directions et ne doit pas être appelée P1a, qui reste un
falsificateur q4-only.

Au split `Q=Q_L union Q_R`, chaque enfant hérite crédits et frontière ambiguë.
Cette frontière doit notamment avoir conservé les domaines qui chevauchaient
`Q` : les points du sibling, exclus comme témoins chez le parent, deviennent
alors admissibles chez l'enfant et sont reclassifiés. Une insertion séparée du
sibling n'est utile que si l'implémentation l'a réellement retiré et qu'elle
déduplique les plages; l'injecter alors qu'il est déjà représenté créerait un
double crédit. Une machine qui repart de la racine reste exacte mais retrouve
le coût proscrit. Les états témoins sont immuables et partagés structurellement.

Une feuille témoin partielle conserve trois masques disjoints : accepté,
rejeté et ambigu. Seul l'ambigu est raffiné; retirer une feuille entière après
un crédit partiel perd des prunes descendants. Le majorant de rejet est le
maximum **entier** exact de `u*v-v^2` : pour chaque extrémité `u` de la boîte
cible, évaluer les deux bords témoins et les deux entiers bornés voisins de
`u/2`, puis prendre le maximum et sommer les trois axes. L'arrondi continu
`ceil(u^2/4)` est sûr mais peut perdre un rejet lorsque `u` est impair.

La frontière emploie partage structurel ou rollback par watermark. Aucun
suffixe mort n'est copié après le dixième crédit. Un microtile de cibles contre
un nœud témoin produit trois bitmasks accepté/rejeté/ambigu et ne splitte le
témoin qu'une fois pour toutes les lanes actives.

Si une boîte cible peut rencontrer un ensemble conservateur de chambres `S`,
elle est encore prunable lorsque toutes les banques de `S` sont pleines et que
`dist^2(p,box)>3*max_{c in S} D_c`. En effet, l'échec d'au moins une des trois
coupes Yao implique `||q-p||^2<=3D_c`. Cette enveloppe multi-chambre reste
strictement fail-open aux égalités.

Le filtre suivant ne se contente pas de cette enveloppe radiale. Pour chaque
chambre du masque, il intersecte exactement la boîte avec le cône fermé
`x>=y>=z>=0` après transformation signée/permutée et obtient en entiers les
minima de `x`, `x+y` et `x+y+z`. Il exige les trois inégalités strictes contre
la banque de cette chambre; une intersection vide retire seulement le bit
correspondant. Le cône fermé est un sur-ensemble conservateur des frontières
semi-ouvertes, et toute égalité descend.

Les banques ponctuelles de la tuile active restent résidentes dans l'enveloppe
`O(B*48*K)` pour le top-nearest ou `O(B*48*(K+1))` pour un réservoir
arbitraire. Une table globale de onze identifiants u32 pour 50 000 ancres et 48
chambres occuperait déjà 105 600 000 octets, hors masques et provenance; elle
n'est pas l'architecture par défaut, sans être mathématiquement interdite si
une mesure future en justifie le trafic. L'owner Morton d'une
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
une banque factorisée `(ancre, chambre, version)` et son masque d'engagement au
lieu de recopier ses dix ou onze candidats. Les compteurs couvrent construction du LBVH, visites et pops de
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

Le successeur prioritaire est un état produit adaptatif `(Q,A,F)`. `A` est une
antichaîne immuable de témoins acceptés et `F` une antichaîne lossless de
domaines ambigus. Les monotonicités de `L/U` autorisent à scinder librement la
cible ou le témoin; un look-ahead choisit seulement l'ordre qui résout le plus
de masse. Les feuilles partielles conservent trois masques
accepté/rejeté/ambigu, et les siblings partagent `F` ou emploient un rollback.
Si l'ancrage reste rouge, la même preuve se lève en triple-tree `(P,Q,W)` avec
partition canonique des paires. Cette ordonnance et ses gates sont fixées dans
[`AUDIT_DEBLOCAGE_Q2_ET_LOCALITE_20260812.md`](audits/AUDIT_DEBLOCAGE_Q2_ET_LOCALITE_20260812.md).

L'inversion en une ancre donne séparément une borne exacte de localité. Une
couverture stricte de toute la sphère ne peut toutefois certifier ni les ancres
du bord convexe, ni aucune ancre d'un nuage coplanaire; elle n'est donc pas une
source générale. La variante candidate maintient plutôt le dixième seuil de
calotte par cellule angulaire et range-report seulement les cibles sous ce
seuil. Une cellule sous-pleine retombe vers le dual-tree. Ce lemme local ne
donne aucune borne de travail : une requête cône--LBVH peut visiter tout l'arbre
et le census fermé peut rapporter une coquille linéaire. Le statut des
prototypes, raccords et portes appartient exclusivement à l'audit live.

La cascade est admise comme architecture seulement si ses compteurs couvrent
la recherche, le classifieur, le census et la mémoire : sorties compactes et
prunes nombreux ne suffisent pas. Toute ordonnance doit franchir la rampe
`12 500/25 000/50 000` avant un port device. Les mesures de snapshots, les
défauts du code courant et les conditions de réception détaillées restent dans
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md) et
[`AUDIT_ROUTE_50K_1S_DF9DC77_20260812.md`](audits/AUDIT_ROUTE_50K_1S_DF9DC77_20260812.md).

`smax` ne borne pas non plus une coquille fermée arbitraire : une activation
admise vérifie le rang fermé sous `RelevantGP`, tandis qu'une coquille plus
grande doit être refusée explicitement, jamais tronquée. Il ne transforme pas
ce pire cas en graphe de degré borné. Dans l'espace
euclidien, pour tout `m`, un point `p` et `m` points distincts `q_i` sur une
sphère centrée en `p` vérifient, pour `j!=i`,

$$\Phi_{p,q_i}(q_j)=R^{2}\left(1-\cos\theta_{ij}\right)>0.$$

Les `m` paires `p q_i` ont donc toutes le rang fermé deux. En ajoutant `h`
points communs strictement intérieurs sur l'axe d'une petite calotte de
directions, la même construction donne un degré arbitraire dans le bucket
exact `closed_rank=h+2`; ceci vaut notamment au rang exact 11. Le kissing
number 12 est inapplicable et `smax=11` borne seulement le contenu d'une
activation admise sous `RelevantGP`, jamais le degré de `p`. Sur la grille u16
finie, les seules bornes universelles
immédiates disponibles ici sont les caps triviaux `n-1` et `2^48-1`; l'absence
d'une meilleure borne finie n'est pas prouvée. Deux constructions documentées
à treize voisins réfutent le cap 12.
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
d'extrémités est scindé, le domaine du frère devient admissible comme banque de
témoins pour l'enfant. Il doit avoir été conservé parmi les domaines ambigus du
parent, puis être reclassifié par `L4/U4`; s'il avait été retiré, il faut le
réintroduire exactement une fois. Une frontière qui perd ces points omettrait
de nouveaux témoins, tandis qu'une seconde insertion non dédupliquée pourrait
les compter deux fois.

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
raffinement. Toute frontière persistante reste lossless, sans cap, conserve les
domaines qui chevauchent le bloc parent et les reclassifie lorsqu'ils deviennent
disjoints après un split.

Une route par ancre évite le rescan d'un arbre de témoins pour chaque paire.
Pour une ancre `p`, un témoin `w`, une cible `q`, poser `s=w-p`, `d=q-p` et
`A=d dot s-||s||^2`. Le témoin est universel sous les tests :

$$\text{q3: }A>0\ \text{ et }\ 3A^2>\lVert d\mathbin{\times}s\rVert^2,\qquad\text{q4: }A>0\ \text{ et }\ 2A^2>\lVert d\mathbin{\times}s\rVert^2.$$

Pour `p,w` fixes, écrire plus naturellement `t=q-w`. Alors
`A=t dot s` et `d cross s=t cross s`. En décomposant `t=alpha*u+v`, où
`u=s/||s||` et `v` est orthogonal à `u`, le domaine cible q3/q4 devient
respectivement `alpha>0, ||v||<sqrt(3)*alpha` et
`alpha>0, ||v||<sqrt(2)*alpha`. Ce sont les intérieurs de cônes de Lorentz
convexes d'apex `w`, d'axe `w-p` et de demi-angles exacts `60` degrés et
`arctan(sqrt(2))`. Par conséquent, un nœud AABB de cibles est **entièrement**
couvert si et seulement si ses huit coins satisfont strictement le prédicat
entier ponctuel. Cette porte `iff` est plus serrée que comparer séparément
`A_min` et un maximum de produit vectoriel. Toute égalité, `w=p` ou boîte
indécise descend.

Le test par coin peut éviter le produit vectoriel : avec
`E2=||w-p||^2`, `X2=||q-w||^2` et `H=(q-w) dot (w-p)`, q3 exige
`H>0 && 4*H^2>E2*X2`, tandis que q4 exige
`H>0 && 3*H^2>E2*X2`. `E2` est précalculé par entrée de banque ; les carrés et
produits sont promus avant multiplication vers au moins `u128`.

Une porte `NONE` fail-open évite de descendre tout l'extérieur du cône. Sur la
boîte cible, calculer exactement `Hmax`, puis un minorant `Rlb` de
`||(q-w) cross (w-p)||^2` : chaque composante du produit vectoriel est une
forme linéaire dont l'intervalle est exact, et `Rlb` somme les distances
carrées de zéro à ces trois intervalles. Le nœud est `NONE-q3` si
`Hmax<=0 || 3*max(Hmax,0)^2<=Rlb`, et `NONE-q4` en remplaçant `3` par `2`.
Ce rejet peut être incomplet à cause des corrélations entre composantes, jamais
faux. Dans le seul diagnostic borné, `UNKNOWN` peut rejoindre au cap un
résiduel muni de son état de reprise, sans émission de `PairId`. Le profil
produit qui prétend au SLO n'accepte aucun budget configuré, même non atteint :
il poursuit exactement ou échoue sur une ressource réelle. Un compteur de
candidats qui omet la masse résiduelle ne ferme aucune identité.

Une banque de neuf ou huit `PointId` distincts dont les cônes couvrent le nœud
certifie celui-ci sans partager ni univers ni sort avec q2. Aucun nombre fixe de
banques directionnelles n'est affirmé complet : ce `Jung--Yao target range` est
un certificat fail-open à mesurer.

Une spécialisation concrète construit par requête k-NN exacte une banque bornée
`Z_a` des voisins les plus proches de chaque endpoint, puis classe des nœuds
partenaires entiers par les huit coins de ces cônes. Elle ne balaie jamais `Z_a` par
`PairId`. Huit témoins ferment q4 et neuf ferment q3 à `smax=11`;
un prune simultané des trois lanes exige en plus dix témoins q2. Un échec de
la banque conserve le nœud ou le divise sous ce budget et finit en bloc sur le
chemin complet. Le
demi-angle mesuré depuis l'endpoint n'est qu'asymptotique et garde une condition
radiale ; le cône exact ci-dessus est mesuré depuis le témoin. Seuls ses
prédicats entiers sans racine font autorité. Le ledger sépare visites k-NN,
tests témoin--nœud, crédits hérités, masse de paires fermée et résiduelle. Un
coût `M*C(n,2)` est interdit.

Le contre-audit du premier producteur ponctuel confirme le lemme mais refuse
son ordonnance industrielle. Sur une rampe mono-ELF
`n=500/1 000/2 000/4 000`, banques 48 et 96, aucune des séries
`uniform` ou `eight_clusters` ne ferme deux pentes de travail
`<=1,35`; à banque 96, les troisièmes pentes restent
`1,591/1,600` pour les visites cible,
`1,524/1,625` pour les tests témoin--nœud et
`1,419/1,602` pour les candidats. Le
chemin fait une requête k-NN et une DFS cible pour chacun des `n` endpoints :
une baisse de la seule masse candidate, même avec banque 256, ne reçoit pas son
coût. La primitive reste oracle/classifieur terminal jusqu'au lift collectif.

Avant toute `site_list`, une variante plus directe classe un nœud témoin AABB
contre le spindle complet d'une ancre ponctuelle. Comme `W_3(a,b)` et
`W_4(a,b)` sont convexes, une boîte fermée est incluse dans `W_q` si et
seulement si ses huit coins satisfont strictement le prédicat correspondant.
Un nœud `ALL-W4` crédite sa masse aux deux lanes ; un nœud `ALL-W3` crédite q3
mais doit encore être descendu tant que q4 est vivante. Sa frame marque alors
la plage déjà créditée q3, afin qu'un enfant `ALL-W4` ne crédite que q4. À
`smax=11`, huit crédits tuent q4 et neuf tuent q3. Les nœuds crédités forment une antichaîne par lane,
les endpoints sont exclus et toute égalité descend. Si les deux lanes meurent,
la machine ne construit ni `site_list`, ni `kept`, ni `lens`. Cette route vise
les témoins proches des endpoints que la seule boule médiane ignore ; elle doit
être reçue contre un scan ponctuel et profilée sur `eight_clusters`.

Cette DFS ponctuelle est un oracle, pas encore la route 50 k. Le producteur
relève ensuite le certificat sur `A_endpoint times B_partner times C_witness`
avant tout `PairId` : une même antichaîne témoin doit être `ALL-W3/W4` pour
toutes les paires du bloc sous bornes dirigées, et `UNKNOWN` subdivise. Une DFS
ou une liste par partenaire réintroduirait le front quadratique sous un autre
nom. Le ledger porte masse de blocs/paires, visites, pentes et cap absolu.

Un lift de bloc entièrement entier rend cette route falsifiable. Pour
`a in A`, `b in B`, `z in C`, poser `H=(b-z) dot (z-a)` et
`R=||(b-a) cross (z-a)||^2`. Le minimum `Hmin` est atteint sur les coins : `H`
est affine en `a,b`, concave en `z` et séparable par coordonnée, si bien que
trois groupes de huit évaluations scalaires suffisent. Le maximum `Rmax` est
également atteint parmi les `8^3` triples de coins, par convexité séparée de la
norme carrée d'une application affine. Le bloc est donc `ALL-W3` sous
`Hmin>0 && 3*Hmin^2>Rmax`, et `ALL-W4` sous
`Hmin>0 && 2*Hmin^2>Rmax`. Ces tests sont des certificats suffisants calculés
exactement, pas des décisions `iff` : les deux extrema peuvent provenir de
triples différents et un échec reste `UNKNOWN`.

Un fallback plus serré est en revanche un `iff ALL`. À `a,z`
fixes, la fibre admissible en `b` est le cône cible convexe ; à
`b,z` fixes, la symétrie donne le même cône en `a` ; à
`a,b` fixes, la fibre en `z` est le spindle convexe. Le
prédicat est donc convexe séparément dans les trois variables. Si les
`8^3` triples de coins sont strictement admis, interpoler
successivement `C`, `B` puis `A` admet tout le
produit ; la réciproque est immédiate. L'échec de ces 512 tests signifie
« pas ALL », jamais `NONE`.

Le self-join `A times B` est canonique et partage une frontière `C` persistante.
Scinder `A/B` partitionne la masse de paires ; scinder `C` partitionne seulement
la recherche des témoins et ne recrédite jamais cette masse. Les reçus de plages
sont séparés par lane, hérités sans retour à la racine, et toute intersection
de `C` avec les identifiants de `A/B` force la descente jusqu'à exclusion des
endpoints. La broad phase emploie `Hmin` puis des bornes d'intervalles de `R` ;
le test direct des `512` triples reste un fallback `ALL` exact
et compté, jamais un coût silencieux par état. Sous u16 et
`n<=50 000`, `H` tient dans `i64`, mais `R` et les carrés
comparés exigent une promotion avant multiplication vers au moins `u128`.
L'orientation du self-join est géométrique et canonique ; l'union dédupliquée
des banques des deux endpoints ordonne la recherche sans dépendre du
`PointId`. La lane q2 garde sa cascade Yao/affine/dual séparée et ne
retarde pas les morts q3/q4.

Sur CUDA, cette promotion doit être un prédicat explicite à deux limbs, pas un
`__int128` hôte annoté : multiplication 64 fois 64, mot haut, petit coefficient
3 ou 4, puis comparaison lexicographique. Une porte CPU/device aux frontières
reçoit les 68--70 bits. Le ledger du lift publie séparément masse fermée,
terminale et résiduelle par lane, tests `NONE`, triples de coins, allocations,
octets copiés et high-water de frontière ; aucun résultat G4 ne précède deux
pentes vertes et ces caps absolus.

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

Sous un Poisson homogène tridimensionnel sans bord, la taille du front possède
néanmoins une baseline exacte. Les spindles q4, q3 et la boule diamétrale q2
sont imbriqués; un seul `PairId` porte donc un masque de lanes. Après
coalescence des événements « moins de 8/9/10 témoins », l'intensité attendue est
`141,183365 rho |Omega|`, soit environ `7,06` millions de paires à 50 000
points. Cette valeur ne borne ni les visites nécessaires à la produire, ni les
tiers q3, ni les apex q4. Les gates `W_front` et `W_extend` restent séparées;
le dual-tree actuel sert de baseline réfutée, pas de producteur reçu. Le calcul
et la cascade exacte par boule de milieu sont dans
[`AUDIT_VERROU_MATHEMATIQUE_FRONT_JUNG_H0_GPU_20260812.md`](audits/AUDIT_VERROU_MATHEMATIQUE_FRONT_JUNG_H0_GPU_20260812.md).

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

Fixer l'ancre et compter `c` sites strictement intérieurs sur tout le disque.
Pour q4, poser `k=smax-4-c`, soit `k=7-c` à `smax=11`. Construire l'arrangement
seulement sur le multiensemble `E` des contraintes orientées de la lentille
fermée `||z-a||^2<=D^2,||z-b||^2<=D^2` dont la ligne coupe le disque est une
génération superset exacte : au centre d'un vrai support, ses deux lignes sont
incidentes et le nombre de lignes de `E` strictement positives est au plus le
census global privé des `c` témoins permanents, donc au plus `k`. Retirer les
formes non-carriers ne peut qu'abaisser cette profondeur. Le rang de cet
sous-arrangement n'est toutefois jamais publié comme `p`; le census final
rejoue tous les sites nécessaires à `I_B` et `U_B`. Plus précisément, poser
`d=smax-2` et `theta` égal à la d-ième plus grande borne inférieure. Sur un
domaine vivant, moins de `d` bornes sont strictement positives, donc
`theta<=0` et `U_z<theta` implique déjà `U_z<0`. Si au moins `d` bornes sont
positives, q3 et q4 sont mortes. Le filtre global `theta` est donc redondant
sur tout domaine vivant ; le top-k sert seulement de certificat de mort. Les
identités des `always_inside` doivent être transportées pour recevoir `I_B`,
pas seulement leur cardinal, tandis que `U_z<0` est le seul rejet global utile.

La lentille **aiguë** n'est qu'un certificat collectif d'existence : tout q4
possède au moins un carrier aigu adjacent à l'ancre. Elle ne filtre jamais les
deux lignes de `E`. Le bit `acute(z)` reste attaché à chaque ligne et un couple
n'est admissible que si `acute(x) ou acute(y)`. La fixture à une seule face
positive est une porte permanente de cette distinction.

Cette structure mono-ancre ne doit pas être confondue avec le plein arrangement
relevé de la section suivante. Si `m=|E|`, le nombre de centres géométriques
distincts de profondeur au plus `k` est inférieur à `e(k+1)m` pour `k>=1`, et
au plus `m` pour `k=0`. La preuve échantillonne chaque ligne avec probabilité
`1/(k+1)` et injecte tout sommet retenu dans un sommet de l'intersection convexe
des demi-plans négatifs échantillonnés. Cette borne n'affirme ni que la somme
des `m` sur toutes les ancres est linéaire, ni qu'une concurrence ne porte pas
quadratiquement beaucoup de `SupportKey`.

Le même argument borne la masse d'incidences orientées centre--contrainte :
`I_<=0<=2m` et `I_<=k<2e(k+1)m` pour `k>=1`. Une contrainte échantillonnée est
incidente à au plus deux sommets de l'intersection convexe négative. Le ledger
porte donc `shell_incidence_mass` séparément ; le vrai résiduel potentiellement
quadratique commence aux couples cross-bundle `J_pos`; l'aval paie ensuite le
payload des `H_out` sorties acceptées.

Une ordonnance exacte concrète choisit un chart entier du plan médiateur et
une cisaille unimodulaire sans ligne verticale, puis sépare les formes `P`
positives au-dessus de leur ligne et `N` positives au-dessous. Hors shell :

$$p_E(x,y)=\#\left\lbrace i\in P:l_i(x)<y\right\rbrace+\#\left\lbrace i\in N:l_i(x)>y\right\rbrace.$$

Construire les `k+1` niveaux inférieurs `0..k` de `P` et supérieurs de `N`
suffit. Les candidats sont les sommets `P-P` dont le rang opposé ferme le
budget, les sommets `N-N` symétriques et les overlays des **segments actifs**
des niveaux `r,s` avec `r+s<=k`. Les droites porteuses entières ne sont jamais
croisées deux à deux. Une construction conservatrice vise
`O(m*k^2+m*alpha(m)*log m+V+J_pos)` jusqu'aux candidats géométriques, où `V`
compte les centres shallow uniques et `J_pos` les couples cross-bundle qui
passent le reporting de positivité. Owner et census ajoutent un terme séparé
`W_census`, qui doit être fourni par les listes de conflits/identités plutôt
que par un rescan du nuage. Le payload aval ajoute le coût de ses `H_out`
sorties acceptées, avec `H_out<=J_pos`. Cette borne reste une cible conditionnelle :
la référence des niveaux ordinaires ne reçoit pas encore leur variante
pondérée. Tout candidat valide ensuite
Jung, indépendance affine, positivité, les six distances, owner, census et clé.

Les parallèles distinctes ne créent aucun événement. Les droites confondues
sont des bundles avec identifiants et orientations ; les intersections
multiples sont groupées par centre rationnel et traitées atomiquement, toutes
les lignes incidentes étant exclues du rang strict. Une perturbation
séquentielle n'est pas exacte. Une grande cosphère est quotientée par une
branche de plateau reçue, développée selon le contrat, ou refusée explicitement.

La positivité d'un lot concurrent admet un terminal sortie-sensible plus fort
que toutes les paires. Fixer son centre `c_v`, poser `d=b-a`, `m_ab=(a+b)/2`,
`u=c_v-m_ab` et `n=d cross u`. Si `u=0`, aucun q4 propre positif ne peut avoir
cette ancre. Pour tout carrier incident `z`, définir
`t_z=d dot (z-c_v)`, `r_z=u dot (z-c_v)` et `s_z=n dot (z-c_v)`. Soit `h_v`
le nombre de `PointId` incidents hors endpoints. Une paire `x,y`
porte un tétraèdre propre positif avant le test de diamètre si et seulement si
`s_x*s_y<0` et :

$$D^2R-2\left\lVert u\right\rVert^2T>0,\qquad D^2R+2\left\lVert u\right\rVert^2T>0.$$

où `R=|s_y|r_x+|s_x|r_y` et `T=|s_y|t_x+|s_x|t_y`. En posant
`S_z^-=(D^2r_z-2||u||^2t_z)/|s_z|` et
`S_z^+=(D^2r_z+2||u||^2t_z)/|s_z|`, la condition devient
`S_x^-+S_y^->0` et `S_x^++S_y^+>0` entre côtés opposés et bundles distincts.
Un reporting de dominance 2D exact produit ces couples en
`O(h_v log h_v+J_pos)`, puis la sixième distance,
l'owner et le census les décident. Si un côté est vide, tout le centre est
rejeté en `O(h_v)`. `J_pos` peut encore être quadratique alors que la distance
rejette tout ; il reste donc un compteur et une obligation de cap, pas une
borne générale de sortie.

Pour un layout device, une shallow cutting certifiée emploie des cellules
half-open, un `base_inside` exact et des listes de conflits complètes. Sous un
cap terminal `tau`, elle doit fermer
`sum_C C(|X_C|,2)<=((tau-1)/2)*sum_C |X_C|`; toute cellule lourde est raffinée,
routée vers le moteur de niveaux ou refusée, jamais tronquée. La preuve, la
borne, les dégénérescences et les gates sont détaillées dans
[`AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md`](audits/AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md).

### 6.6 Catalogue de supports et vrai arrangement

Cette section traite l'arrangement **global relevé** et ses transits. Sa famille
quadratique interdit de matérialiser cet objet ; elle ne contredit pas la borne
de la section 6.5 sur les centres shallow distincts d'une ancre déjà admise.
Le coût global de la route mono-ancre reste néanmoins conditionné par la masse
des ancres, `sum |E_ab|`, les censuses et les plateaux.

Le catalogue abstrait des supports propres positifs `S`, de tailles deux à
quatre, satisfaisant `|I_B|+|S|<=11` est une source **générative** de toutes les
cofaces de cardinalité au plus onze. Il n'est pas une bijection et ne signifie
pas que le rang fermé `|I_B|+|U_B|` est au plus onze. Chaque `BallRecord` groupe
par `BallKey` le census global `I_B/U_B` et tous les supports minimaux; les
statuts `relevant_by_min_support` et `accepted_closed_rank` restent séparés.

Un arrangement de bissecteurs décrit les centres de sphères incidentes. Les
supports q2/q3 sont les projections auto-centrées sur ses faces et arêtes; les
supports q4 sont certaines intersections auto-centrées. Le sous-graphe des
seules sorties n'est pas une autorité de parcours. Un sweep du plein arrangement
conserve les états non auto-centrés comme transits et choisit l'intersection
consécutive avec tous les ex æquo. Une ordonnance comprimée peut au contraire
sauter les sorties intermédiaires et viser directement le prochain contact
entrant; son état de chambre, sa reachability et ses lots doivent alors être
prouvés séparément. Une minimisation globale du rayon sans cet invariant, un
remplacement par un point intérieur ou un germe par ancre ne remplace pas ces
obligations.

Cette voie ne possède aucune complexité sortie-sensible générale. Une requête
LBVH d'intérieur vide peut visiter `Theta(n)` nœuds et le shell complet peut
avoir `Theta(n)` labels. Son admission exige donc les sommes séparées de
visites boule/pivot, les états de transit, les tailles de shell, les octets et
les deux pentes de la rampe, pas le seul nombre d'objets émis.

La séparation peut être quadratique. Pour les deux droites u16
`A_i=(1+i,0,0)` et `B_j=(0,1+j,1)`, l'arrangement relevé contient exactement
`55m^2-440m+715` sommets q4 jusqu'au niveau neuf pour `n=2m`, tandis que Source S
q2--q4 contient exactement `20m-55` supports, tous q2. À `n=50 000`, les deux
volumes valent `34 364 000 715` et `499 945`. Le plein arrangement n'est donc ni
la sortie ni un minorant que toute route exacte doit payer. La preuve et le
blueprint support-first sont dans
[`AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md`](audits/AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md).

La sortie elle-même est linéaire mais volumineuse sur le modèle favorable.
L'équation (7) de
[`Poisson--Delaunay Mosaics of Order k`](https://doi.org/10.1007/s00454-018-0049-2)
et les constantes 3D de
[`Expected Sizes of Poisson--Delaunay Mosaics`](https://doi.org/10.1017/apr.2017.20)
donnent, pour un Poisson homogène continu et les supports positifs de Source S
jusqu'à `smax=11`, une espérance
`(175+495*pi^2/16) rho |Omega|`, soit environ `480,340886` supports par point.
L'analogie bulk à 50 000 points vaut environ **24,017 millions de supports**.
Elle impose un ledger de débit et une fusion device vers le fold, pas un
catalogue hôte. Ce n'est ni une identité pour une boîte u16 finie, ni un
minorant sur tout algorithme H0, ni une borne sur le travail de découverte.

#### 6.6.1 Minimum auto-centré sur le flat relevé

Poser `ell_x(c)=2<x,c>-||x||^2`. Pour un support affinement indépendant `U`,
son flat relevé est défini par `lambda=ell_u(c)` pour tout `u in U`, et la
fonction rayon y vaut `Phi(c,lambda)=||c||^2-lambda`. Sa restriction possède un
minimum unique : le circumcentre intrinsèque `c_U`, de valeur `beta_U`. Le
support est positif exactement lorsque `c_U in relint conv(U)`; à ce point,
`ell_y(c_U)>lambda_U` équivaut à `y` strictement intérieur.

Cette caractérisation corrige une fausse bijection. Une face de première
génération d'un niveau shallow n'est pas nécessairement une source q2/q3 : son
point courant peut ne pas minimiser `Phi` sur le plan ou la droite d'égalité.
Par exemple, `U={(-1,0,0),(1,0,0)}` et `y=(0,2,0)` réalisent une sphère shallow
centrée en `y`, tandis que la miniboule critique de `U` est centrée en zéro et
n'a pas `y` pour intérieur. Pour q4 affine-3, le lieu d'égalité est un point :
le sommet shallow donne alors le centre, mais positivité, owner et rang restent
à vérifier. Les mosaïques d'ordre supérieur restent donc des oracles; une
shallow cutting à listes de conflits complètes est seulement une recherche q4.

#### 6.6.2 Sentinelle top-`(smax-q+1)` hors support

Pour un support validé `U` d'arité `q`, poser `t=12-q`. La primitive reçoit les
`t` vrais plus proches `PointId` de `X minus U`, ex aequo arbitraires, avec le
certificat que la distance maximale retournée `delta` ne dépasse aucune
distance omise. Si moins de `t` points restent hors de `U`, elle scanne tout
`X minus U`.

- `delta>beta` implique que tous les intérieurs et contacts hors `U` sont dans
  les retours. Le fast path direct exige que l'extra-shell `H=E minus U` soit
  vide, donc `E=U`;
- `delta<beta` fournit `12-q` intérieurs, donc `p+q>=12`, et rejette la fenêtre
  `smax=11`;
- `delta=beta` contient tous les intérieurs et au moins un contact hors `U`.
  Comme `p<=t-1`, on a automatiquement `p+q<=11`; la boule pertinente rejoint
  le range-report, le quotient de plateau ou un refus fermé.

Cette profondeur est minimale parmi les sentinelles fixes une fois `U` connu.
Top-`(11-q)` ne distingue pas les mêmes premiers retours d'une version qui
ajoute le dernier intérieur faisant passer à `p+q=12`, ni d'une version qui
ajoute un contact hors support. Le top-12 global reste sûr, mais sa minimalité
est rétractée. L'exclusion se fait par identité, jamais par coordonnées. Avec
`c=C/D`, comparer exactement `||D*x-C||^2` et ne pruner le LBVH que sur une
borne entière certifiée.

Si le producteur livre déjà un census certifié, la sentinelle devient un oracle
différentiel ou un fallback. En particulier, l'enveloppe top-9 q3/q4 connaît au
centre les `always_inside`, toutes les fonctions strictement positives et
toutes celles égales à zéro; ses omises portent une preuve négative. Elle rend
donc directement `(I,E)`. Son neuvième ordre statistique est pris dans
`X minus {a,b}` et tous les ex aequo du cutoff restent actifs.

Cette espérance ne donne pas de borne déterministe sur la sortie exhaustive.
Dans le modèle continu, ou lorsque la précision croît avec `m`, quatre petites
calottes autour des directions d'un tétraèdre régulier, toutes sur une même
sphère, fournissent `Theta(m^4)` supports q4 positifs ayant la même
`GeometricBallKey`. Le domaine u16 fixé est fini : ce motif y impose une gate
de plateau et une fixture finie, pas un claim asymptotique. Un RLE par boule mutualise le census, jamais les
`SupportKey` exigés par Gamma. Un SLO universel exige donc un quotient de
plateau explicitement autorisé pour H0 ou une hypothèse d'entrée qui exclut
cette sortie; `smax` seul ne la borne pas.

La décision d'exploration est tenue par tranche, sans promotion implicite :

| tranche | candidat examiné | comparateur ou voie suspendue |
| --- | --- | --- |
| `k=1` | Yao-1 exact puis EMST sparse | Borůvka point--LBVH borné |
| q2 profond | lane cellules `D_9` | Yao--banque affine--dual et self-join comme diagnostics/falsificateurs |
| q3/q4 | cellules de centres, arités et budgets indépendants | exhaustif borné et fronts historiques comme falsificateurs |
| quotient H0 | fusion vers activations, gateways et token Johnson | provenance exhaustive conservée tant que Gamma/verticales ne sont pas reconstructibles |

Le théorème de propriétaire donne des plafonds de couverture par arité : neuf
pour q2, huit pour q3 et sept pour q4. Une route scindée qui produit q2 sans
arrangement ne doit pas conserver le plafond neuf pour q3/q4; une lane q4
séparée s'arrête à sept. Ce sont des plafonds de complétude, pas une obligation
d'énumérer tous les sommets qui les respectent.

Chaque feuille produit d'abord une occurrence compacte
`(cloud_epoch,SupportKey)` après les seuls filtres sûrs qui ne demandent pas la
géométrie complète; `CellId` reste un diagnostic facultatif. Un premier
radix/RLE par `SupportKey` calcule centre et positivité une seule fois, descend
ce centre dans la partition half-open et retrouve sa feuille owner. La table
terminale transitoire conserve pool, seuils et buckets : le run rejoue
`U subset D_{11-q}(C_owner)` et les filtres de la lane. Zéro owner ou membre
manquant rejette un tuple arbitraire; la complétude garantit le rejeu pour tout
support pertinent. Plusieurs owners signalent un invariant rompu. Si cette
table n'est pas conservée ou si les arités n'ont pas une partition commune,
chaque occurrence transporte à la place son `CensusContext` et le RLE choisit
le contexte owner certifié.

Au profil contractuel `n=50 000`, les lanes séparées encodent exactement q2
dans un `u32`, q3 dans les 48 bits utiles d'un `u64` et q4 dans un `u64`
seulement avec un `DensePointIndex:u16`. Une bijection immuable, liée à
`cloud_epoch`, le relie aux `PointId` durables; la sortie canonique remappe puis
ordonne les vrais identifiants. Le reçu uniforme courant compte
`96 241 855 / 352 786 093 / 390 554 718` occurrences : les clés nues occupent
environ `6,33 Go`, ou `12,66 Go` en double buffer, sans `CellId`, table de
remap, workspace, listes ni sorties. Une point-location owner après RLE est
donc aussi une décision de layout. Le nombre de clés uniques et le high-water
complet demeurent des gates : un RLE retire la multiplicité spatiale, pas les
candidats intrinsèquement distincts qui échoueront ensuite.

Le collecteur CPU non commité `UniqueKeyReceipt-v1` ne reçoit pas ce layout :
il stocke q2/q3/q4 sur huit octets, empaquette les identifiants croissants dans
les bits de poids croissant et trie donc q4 par `(d,c,b,a)`. Il mesure un nombre
de clés, pas la contiguïté lexicographique des faces ni le trafic
`u32/u64/u64`. Son quota divisé entre workers peut alterner succès et refus sous
une commande identique et ignore les capacités, le tableau des longueurs de
run et les temporaires. Avant de devenir une autorité de dimensionnement, il
doit fermer les occurrences par arité, lier bijection/époque/digest, borner le
HWM global et recevoir un résultat indépendant du scheduling.

L'ordre lexicographique q4 groupe naturellement le préfixe de face `(a,b,c)`.
Un warp construit une fois la normale et l'axe circumcentrique de cette face;
chaque lane apex `d` intersecte cet axe avec le seul bissecteur `a/d`, puis
teste owner et barycentriques en arithmétique exacte. Cette factorisation ne
dépend jamais de l'admission q3 de la face. Les q3 rejettent d'abord les
triangles non strictement aigus par trois produits scalaires i64, avant leur
solve rationnel.

Pour la route par front de Jung, cette factorisation admet un owner génératif
exact-once. Q3 n'émet qu'avec la plus petite `PairId` parmi ses arêtes de
longueur maximale; q4 applique la même règle aux six arêtes et traite les deux
carriers comme un ensemble non ordonné. Le centre appartient ensuite à un seul
patch half-open. Sous complétude du front et de cette partition,
`occurrences=SupportKey_unique` avant plateaux; sans canonicalisation, les caps
sont trois en q3 et six en q4. Cette égalité reste une gate d'identités, pas une
hypothèse de dimensionnement.

Le candidat owner peut suivre deux ordonnances. L'ordonnance générale produit
`(cloud_epoch,GeometricBallKey,SupportKey,OwnerCellId)` sans census, puis un
second RLE conserve tous les supports d'une même boule. Elle choisit un support
canonique `U_star` d'arité minimale `q_min` et, si aucun census producteur n'est
reçu, interroge top-`(12-q_min)` dans `X minus U_star`. Employer `q_max` ou
exclure l'union des supports peut masquer une boule pertinente. Le fast path
`RelevantGP` emploie plutôt le census reçu ou top-`(12-q)` par `SupportKey` et
ne forme la clé de boule que pour sa side queue : si `delta>beta` et le shell global rendu est `E=U`,
aucun autre support minimal distinct ne peut porter la même boule, car il
serait un sous-ensemble propre de la base affinement indépendante `U`. Le
record est donc publiable sans tri de sphères. Toute extra-shell, y compris
connue avec `delta>beta`, et toute égalité route vers range-report, quotient de
plateau ou refus fermé. Une A/B décide si le second RLE global amortit mieux les
rares boules multi-supports.

Le backend choisi exécute soit le census producteur, soit la sentinelle hors
support, soit le census pool-relatif déjà prouvé.
Pour `H_run=smax-q_min`, un contexte avec `b_cert>=H_run` effectue seul ce
census terminal, ou le run appelle un census global : une première passe additionne en bloc
les nœuds strictement intérieurs et désactive chaque support dès son
`(12-q)`-ième témoin; si au moins un support reste pertinent, une seconde passe
matérialise `I_B/U_B` complet et attache `U_B` comme identité sémantique aval.
Avec la convention reçue
`power>0` intérieur et `power<0` extérieur, les extrema de puissance classent
`lower>0` intérieur, `upper<0` extérieur et seulement `lower=upper=0` shell en
bloc; toute ambiguïté descend. Le rejet d'un support de grande arité ne supprime
pas la boule si un support plus petit du même run survit. Cette réduction évite
les strict-count et census fermés répétés, sans revendiquer de borne
sublinéaire.

Le premier RLE admet deux réalisations exactes. Premièrement, un lot spatial
contient des feuilles terminales entières. Il peut couper un run global et paie
alors une géométrie par `(SupportKey,lot)`; seul le lot contenant la feuille
owner publie. Sous partition et epoch communs, tous les supports d'une même
boule ont le même centre, donc la même feuille owner et le même lot. Leur second
RLE/census reste local exact-once dès qu'un contexte certifie
`b_cert>=H_run`. Deuxièmement, un shard radix déterministe par `SupportKey`
réunit toutes les occurrences égales et ne paie qu'une géométrie par support, à
condition qu'aucune frontière ne coupe son run. Des `SupportKey` distinctes
d'une même boule peuvent toutefois appartenir à des shards différents. Le flux
positif owner subit donc une seconde redistribution streamée par
`(cloud_epoch,GeometricBallKey)`, portant `OwnerCellId` et tous les contextes;
un désaccord d'owner est un échec d'invariant. Si les producteurs n'emploient
pas une partition commune, ils routent vers un `BallOwner` canonique et un
contexte de census certifié, ou appellent le census global exact.

Sur un plateau, le token exact est un générateur saturé
`(BallKey,beta,S_B=X cap B)` qui représente implicitement le bloc de Johnson.
En effet, toute union de deux k-sous-ensembles adjacents possède `k+1` labels
dans `S_B`, donc une miniboule de niveau au plus `beta`; le graphe fermé induit
contient tout `J(|S_B|,k)` et est connexe. Il ne peut éviter l'énumération des
cofaces que si un lookup de containment ou un join exhaustif
`|S_B intersection S_C|>=k`, les racines pré-lot et l'atomicité sont reçus. Une
facette canonique arbitraire perd des multifusions et des interfaces futures.
Les contre-fixtures et les six réponses sont dans
[`AUDIT_REPONSES_SOURCE_FRONT_INVERSE_20260812.md`](audits/AUDIT_REPONSES_SOURCE_FRONT_INVERSE_20260812.md).

## 7. Cellules de centres : sujet CPU borné, source candidate transitoire

Le prototype CPU actuel est un sujet/référence différentielle branch-and-bound,
pas un oracle. Son juge interne partage encore lifts et puissances. Le juge
externe ajouté au snapshot `90c06b0` emploie au contraire Gauss rationnel,
barycentriques et distances multiprécision, mais reste un candidat borné : ses
quatre accords `n=32,smax=7` sont utiles, tandis que sa porte mutant passe sur
un refus code 2 sous `WILL_FAIL` au lieu d'un désaccord code 1. Un
successeur device ne devient une source produit qu'après preuve de complétude,
gate de travail et inclusion de tout son coût dans `warm_e2e`. Les CSR de
cellules de centres sont alors transitoires; aucun atlas de cellules
d'arrangement ou de centres ne persiste. La formulation exacte est la suivante.

Pour une cellule half-open `C`, les bornes utilisent sa fermeture. Noter
`l_C(x)` et `u_C(x)` les distances carrées minimale et maximale de `x` à cette
fermeture. Pour la lane `q`, poser `t_q=K+2-q`, prendre les `t_q` plus petites
valeurs `u_C`, poser `R_q(C)` égal à leur maximum et
`A_q(C)={x : l_C(x)<=R_q(C)}`.

Si une boule owner de `C` a `beta>R_q(C)`, les `t_q` témoins sont strictement
intérieurs et la boule est H0-inerte. Sinon `beta<=R_q(C)` et son saturé fermé
entier appartient à `A_q(C)`. L'égalité reste toujours dans la branche
conservée.

Le test rationnel explicite `beta<=R_q(C)` n'est pas requis après census local.
Si `p'` est le nombre d'intérieurs vu dans `A_q(C)`, alors
`p'+q<=smax` implique `beta<=R_q(C)` par contraposée : dans la branche opposée,
les `t_q` témoins seraient tous comptés. Le census est alors global et complet.
Il peut être effectué sur la liste plus large `A_2`, avec l'arité q pour la
porte, à condition que le support ait été généré dans `A_q`. Aucun shell partiel
d'une branche rejetée n'est publiable.

Une séparation stricte entre la fermeture de `C` et `conv(A_q(C))` exclut un
support de la branche conservée, car son centre devrait appartenir à
`conv(A_q(C))`. Elle ne signifie jamais « aucun support » : la branche haute
peut contenir de vraies sphères, omissibles seulement du quotient H0 après
théorème 4.2 et resolver.

Sous subdivision de domaines actifs emboîtés, les listes globales sont
imbriquées : un enfant ne peut qu'augmenter `l`, diminuer `u` et diminuer `R`,
donc son `A_q` est inclus dans celui du parent. La racine couvre `conv(X)`, les
enfants half-open ont un owner rationnel unique, et une branche ne termine que
par certificat, producteur exact alternatif ou fallback exhaustif.

Cette inclusion donne une machine plus forte qu'un simple oracle : la
statistique d'ordre et la liste d'un enfant se calculent exactement depuis la
seule liste parente lorsque les domaines actifs sont emboîtés. Le resserrement
par `tight`, suivi d'un enfant dyadique qui peut en déborder, exige un reçu plus
faible : le pool hérité conserve `I_B union U_B` de toute boule pertinente
encore possédable, mais ses seuils ne sont plus les seuils globaux. La même
contradiction par `p+1` témoins prouve cette conservation relative au pool;
`c_B in conv(U_B)` rend bbox et k-DOP sûrs. Domaine actif, cellule owner et
digest du pool sont trois identités distinctes.

Sur GPU, chaque niveau se réalise par réduction top-`t_q`, puis
`count/scan/fill` CSR d'identifiants, avec les listes q4 incluses dans q3,
elles-mêmes incluses dans q2. Les témoins top-`t_q` fixent seulement `R` : ils
ne remplacent jamais la liste entière et tous les ex æquo `l<=R` restent.

Une jauge dyadique `c_0` fixée une fois pour tout l'arbre retire le terme
quadratique commun. Poser

$$s_x(c)=\left\Vert x-c_0\right\Vert^2-2\langle x-c_0,c-c_0\rangle.$$

Puisque `||x-c||^2=s_x(c)+||c-c_0||^2`, les rangs et les preuves de témoins
emploient les extrema affines de `s_x`. Pour une boule de centre `c_B`, le
niveau correspondant est `theta_B=beta_B-||c_B-c_0||^2`; les extrema affines
se comparent à `theta_B`, jamais directement à `beta_B`. Le choix par signes
des coefficients vaut sur l'AABB; un k-DOP reste un prune séparé sauf à résoudre
ses extrema par sommets ou programme linéaire exact. Cette variante `L/U,theta`
est un layout exclusif; la formulation `l/u,beta` des paragraphes suivants est
l'autre layout et les deux conventions ne se mélangent pas. Le graphe d'ambiguïté relie deux sites
si zéro appartient à l'intervalle exact de leur différence de scores; tout
support q induit une q-clique. Ce graphe se calcule une fois par terminal et
sert aussi au choix du split. La jauge ne change jamais par enfant sans une
nouvelle preuve de nesting.

Une stratification plus forte emploie un budget `h` d'intérieurs. Poser
`R_h(C)` égal à la `(h+1)`-ième plus petite valeur de `u_C`, puis
`D_h(C)={x:l_C(x)<=R_h(C)}`. Une boule ayant exactement `p` intérieurs vérifie
`beta<=R_p(C)` et son census fermé est dans `D_p(C)`. Les listes `D_h`
croissent avec `h` et restent imbriquées sous subdivision. Poser
`tau_C(x)=min{h:x in D_h(C)}`. Pour un support proposé `U`, son entrée immuable
est `e0=max(tau_C(x):x in U)`; le curseur de promotion distinct commence à
`h=e0`. Après scan complet de `D_h`, le compte intérieur total `r_h` vérifie
`h<=r_h<=p`. Si `r_h>h`, la machine pose `h=r_h` et ne scanne que les nouveaux
buckets; si `r_h<=h`, le census est global et `r_h=p=h`. Un dépassement de
`smax-q` rejette sans publier de shell partiel, et tous les contacts nuls sont
accumulés durant les promotions.

L'exact-once exige une partition terminale commune à tous les budgets d'une
arité. Une cellule ne peut pas émettre pour `h=0` pendant qu'une autre lane de
budget subdivise le même domaine. Dans une feuille commune, chaque
q-sous-ensemble de `D_(h_max)` est formé une seule fois et étiqueté par son `e0`;
une génération par buckets impose `max tau(U)=e0`. Avec
`h_max=smax-q`, la somme sur les strates reste exactement
`C(|D_(h_max)|,q)` : cette technique retire les doublons de
budget et réduit le census, mais ne prouve aucune parcimonie de génération.

La génération q3 et la génération q4 sont indépendantes. Un q3 pertinent peut
n'avoir aucune arête q2 pertinente; un q4 pertinent peut n'avoir aucune face q3
pertinente. Une feuille commune petite énumère donc directement les triplets de
`D_8` et les quadruplets de `D_7`, puis décide positivité et owner. Les bitsets de
bissecteurs, Jung, boules équatoriales et pinceaux sont des filtres fail-open,
jamais une dépendance envers un support d'arité inférieure retenu. Une branche
dense appelle un producteur exact alternatif ou rend `resource_exhausted`.

Un q4 propre positif a son circumcentre strictement intérieur et possède au
moins deux faces aiguës. Une génération q4 peut donc partir de **toutes** les
faces aiguës géométriques, indépendamment de leur admission q3. Pour une telle
face, la droite rationnelle des centres équidistants doit rencontrer le domaine
actif de la cellule; ce test droite--cellule précède les apex. Le q4 choisit sa
plus petite face aiguë canonique. Le résultat géométrique est documenté dans
[`Crux Mathematicorum 38(8), problème 3653`](https://cms.math.ca/wp-content/uploads/crux-pdfs/CRUXv38n8.pdf).

Le filtre du prototype courant n'emploie pas cette spécialisation : la droite
des centres équidistants de **toute** face non colinéaire, aiguë ou obtuse,
contient le circumcentre q4. Tester l'intersection de la droite de la face
canonique avec la cellule est donc exact sans hypothèse d'acuité. Ajouter un
test `acute` à cette seule face serait incomplet; pour exploiter le théorème de
Crux, il faut énumérer toutes les faces aiguës puis choisir la plus petite face
aiguë canonique.

Le premier RLE chaud reçoit
`(cloud_epoch,SupportKey,CensusContext)` avant toute géométrie. Après calcul
unique du lift et choix du contexte owner, la variante BallKey-first produit
`(cloud_epoch,GeometricBallKey,SupportKey,CensusContext)` et son second RLE
emploie cette clé géométrique exacte de taille fixe. La variante
SupportKey-first emploie le census du producteur ou la sentinelle hors support,
puis ne construit cette clé que pour la side queue. Si la forme est
`D||y-a||^2+C dot (y-a)=0`, le 5-uplet homogène
`(D,C-2Da,D||a||^2-C dot a)`, normalisé par signe puis pgcd, est une clé
primitive de sphère disponible avant census. Dans BallKey-first, le contexte lie
cellule, digests de pool/domaine, arène ou backend, `e0` et budget certifié
`b_cert`. Ce RLE conserve tous les supports et contextes. Pour
`H_run=smax-q_min`, il choisit atomiquement un contexte avec
`b_cert>=H_run`, puis emploie son `e0` et ses buckets pour une seule promotion
et un seul census par boule; sans tel contexte, il appelle le census global.
Le `p` obtenu est commun à la boule, mais `p+q<=smax` se décide séparément pour
chaque support. Le shell trié `U_B` identifie sémantiquement la boule dans un
cloud/epoch fixé, mais il n'existe qu'après census et peut avoir `Theta(n)`
labels; il reste un certificat aval. Owner half-open, census complet et
agrégation ferment les doublons.

« Conserver tous les supports » est une exigence dépendante de la sortie, pas un
invariant du chemin H0 normalisé. Une grande cosphère peut porter un nombre
quartique de supports q4. Gamma peut exiger leur provenance; le quotient H0
reçoit un support positif canonique, `q_min` et le saturé fermé, puis exploite la
connectivité du graphe de Johnson sans énumérer tous les supports. Cette
compression ne prouve ni Gamma, ni les verticales. Le plan corrigé, les deux
contre-fixtures inter-arités, les égalités, reçus et compteurs sont dans
[`NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md`](audits/NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md).
Le contre-audit du prototype et les réponses à Claude sont dans
[`AUDIT_REPONSES_CELLULES_CENTRES_20260812.md`](audits/AUDIT_REPONSES_CELLULES_CENTRES_20260812.md).

Cette complétude ne borne pas le travail. Une cosphère massive peut conserver
`|A_q|=Theta(n)` à toute profondeur et provoquer `Theta(n^q)` vues pour une
seule boule. Subdiviser jusqu'à stabilité peut aussi reconstruire implicitement
les bisecteurs du diagramme de Voronoi d'ordre supérieur. La structure reste
donc transitoire, sans atlas ni adjacences persistantes/globales, et hors du chemin chaud tant
qu'aucune borne d'admission n'existe.

Une borne conditionnelle mesurable est toutefois disponible. Pour `c0` dans le
domaine actif `K`, `diam(K)<=delta`, et `rho` distance au `(H+1)`-ième voisin
de `c0` dans le pool, on a `R_(H,P)(K)<=(rho+delta)^2` puis
`D_(H,P)(K) subseteq B(c0,rho+2 delta)`. Ainsi, si
`delta<=alpha*rho` et si un census local certifie au plus
`Lambda*(H+1)` sites dans `B(c0,(1+2 alpha)rho)`, la liste terminale a au plus
ce cardinal. Cette propriété, mesurée sur `uniform` et `eight_clusters`, peut
autoriser un bitset warp `m<=64`; en son absence, la branche choisit CSR,
subdivision ou `resource_exhausted`. `max_depth` seul n'est jamais un
certificat sparse.

Le graphe bissecteur terminal fournit aussi une enveloppe q4 exacte sans lift.
Sur le cut q4 de taille `m_4`, si `T_4` et `Q_4` désignent ses triangles et K4,
alors `4Q_4<=(m_4-3)T_4`, par comptage des quatre faces de chaque K4. Cette
borne remplace tout coefficient empirique constant. Sur bitsets orientés,
sommer pour chaque triangle `i<j<k` le popcount de
`N+(i) intersection N+(j) intersection N+(k)` compte même `Q_4` exactement une
fois. La gate emploie séparément `E_2` sur `D_(smax-2)`, `T_3` sur
`D_(smax-3)` et `T_4/Q_4` sur `D_(smax-4)` — `D_9/D_8/D_7` au défaut
`smax=11` — avec sommes saturées et préflight du bitset ou de la CSR.
Si la statistique d'ordre commune n'existe pas dans le pool courant, la lane
emploie explicitement le pool entier comme superset fail-open et le receipt ne
doit pas le nommer `D_h` exact.
Le majorant intermédiaire `Q_4<=sum C(c_ij,2)`, où
`c_ij=popcount(N+(i) intersection N+(j))`, réutilise le sweep de triangles et
peut s'arrêter dès que le cap est dépassé.

Avant toute intersection, une CSR forward fournit une première enveloppe
exacte. Pour un ordre total quelconque, chaque clique a un unique plus petit
sommet, d'où `T_3<=sum_v C(d_3^+(v),2)` et
`Q_4<=sum_v C(d_4^+(v),3)`. Ces sommes sont saturées pendant le
`count/scan/fill`; une orientation de dégénérescence de valeur `d` garantit
`max d^+<=d` et donc les bornes globales `n*C(d,2)` et `n*C(d,3)`, sans être
requise pour la sûreté. L'ordre d'admission est : enveloppe de degrés,
triangles exacts avec Kruskal--Katona, puis K4 exacts seulement pour une petite
bande résiduelle préflightée. Un budget distinct couvre octets, contextes,
census, tris et launches.

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
  |-> q2 : lane cellules D_9 comparée à Yao--affine--dual suspendu
  `-> q3/q4 : top-(M+1) exact -> fenêtre locale + coupure premier omis
       |-> supports certifiés -> owner tardif -> RLE
       `-> domaine résiduel complet A×B×C ou cellules directionnelles exactes
       -> front Jung + lentille fermée factorisée, bit/certificat aigu
       -> center-cover persistant + cutting signée; top-k tue le patch
       -> q3 intrinsèque + niveaux q4 P/P, N/N, P/N ou shallow cutting certifiée
       -> owner génératif exact-once ou RLE SupportKey -> une géométrie/owner
       -> census I/U complet + identités always-inside et support explicite
       -> side queue H!=empty/plateau, ou second RLE BallKey A/B
       -> BallActivation/tombstones streamées + gate regular/plateau/high-rank
       -> facettes du cœur + gateway canonique de première incidence
       -> carriers stricts + resolver latent
       -> MSF de carriers ou fold direct recertifié
       -> reconstruction (k,beta) atomique + coverage
       -> verticales séparément reçues + payload officiel nommé
```

Aucun tableau global de tuples, paires, cellules, faces ou cofaces ne persiste.
Chaque kernel a un count exact, une arène dimensionnée ou un segment
reprenable, un fill et une identité de consommation. Les segments ne coupent
ni une `BallKey`, ni un lot exact, ni une unité de recertification. Une
insuffisance physique refuse atomiquement; aucun budget configurable ne publie
un préfixe.

Le prototype CPU de cellules et l'oracle exhaustif restent hors du chrono
produit. Ils recertifient des échantillons et des fixtures, puis comparent
digests, masses et décisions à la source device. Un successeur device par
cellules ne peut entrer dans le chrono qu'après la gate de travail; son coût de
listes, subdivision et génération y est alors intégralement inclus.

### 11.1 Sous-source certifiée par fenêtre

Pour `W_M(a)` formé des `M` autres sites exacts et
`delta_out(a)` égal à la distance du premier site omis, toute boule fermée de
rayon `R` portant `a` vérifie

$$4R^2<\delta_{\mathrm{out}}(a)^2\Longrightarrow X\cap B\subseteq\lbrace a\rbrace\cup W_M(a).$$

La conséquence est exacte seulement si le générateur local énumère toutes les
arités demandées et reconstruit `I_B/U_B` entier. Elle reçoit une sous-source,
pas la route globale : un support jamais proposé doit appartenir à un domaine
résiduel couvert avant les tuples. L'égalité est résiduelle. L'owner intervient
après découverte, car un endpoint non owner peut être le seul à satisfaire la
coupure.

Source S borne `|I_B|+|S|`, jamais `|I_B|+|U_B|`. Une coquille peut donc être
linéaire et `M` n'a aucune borne universelle. Une entrée régulière qui déborde
la fenêtre rejoint le résiduel ; elle ne devient pas une dégénérescence. La
preuve, les contre-fixtures et les réponses à Claude sont dans
[`AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md`](audits/AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md).

Avant d'ouvrir ce résiduel, la positivité donne un prune directionnel exact.
Pour tout support positif contenant `a`, le vecteur du centre depuis `a`
appartient au cône `cone(X-a)`. Une cellule qu'un demi-espace entier sépare de
ce cône est vide de supports positifs, même à rayon arbitraire. Toute cellule
qui l'intersecte reste fail-open : ce test ne remplace ni la coupure radiale,
ni l'oracle de minimum sortant, ni le fold.

Le résiduel ne doit pas commencer par une nouvelle boucle sur les paires. Dans
les 432 sous-cônes rationnels déjà reçus, `25r^2<=9D^2` certifie q4 et
`64r^2<=25D^2` certifie q3 pour une candidature d'arête maximale. Le top-8,
top-9 et top-10 par hauteur de cône, puis le report des seules cibles sous le
cutoff, se factorisent en requêtes de dominance. Les fermetures dirigées se
fusionnent par `OR`, les résiduels par `AND`. Des groupes coniques de trois
témoins et un cœur commun WSPD sont deux étages exacts supplémentaires ; leur
échec reste factorisé dans un relation-tree `A×B×C`. La preuve, les fixtures et
la gate commune sont dans
[`AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md`](audits/AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md).
Le premier prototype de fenêtre sert uniquement à recevoir ce lemme et son
census sur petit `n`. Avant toute réutilisation, son juge reconstruit son propre
top-M/premier omis, compare les ensembles `I_B/U_B` et la clé rationnelle de
boule, impose la positivité au fast path Jung et retire tout overflow signé des
mutants. Le contre-audit pincé est
[`AUDIT_WORKTREE_WINDOW_SOURCE_20260813.md`](audits/AUDIT_WORKTREE_WINDOW_SOURCE_20260813.md).
Le successeur `ffe5b69` raccorde ce probe et passe `21/21` portes : il reçoit les
`SupportKey` certifiables et mesure les supports jamais proposés, pas encore les
membres du census, la boule ni la provenance par ancre. Son audit de réception
est
[`AUDIT_SUCCESSEUR_WINDOW_SOURCE_FFE5B69_20260813.md`](audits/AUDIT_SUCCESSEUR_WINDOW_SOURCE_FFE5B69_20260813.md).

### 11.1.1 Ce que la dimension trois autorise sans raccourci combinatoire

La chambre directionnelle ne borne pas le degré q2 par une petite constante :
une fixture u16 porte treize partenaires propres de rang fermé deux dans une
chambre, avec normes distinctes et sans cinq sites cosphériques. La finitude de
la grille donne seulement un plafond combinatoire astronomique. Ni un cap `12`,
ni une exception de mesure nulle, ni un quotient de PointId par les 48
symétries ne peut donc servir d'invariant produit.

L'apport exploitable est ailleurs. Dans chacun des neuf triangles canoniques,
le pire seuil spindle uniforme d'une lane est atteint sur une des neuf paires
de rayons sommets. Si `x` et `y` sont les hauteurs cible et témoin, et si la
table de la cellule porte `B,P,C`, le certificat direct est :

$$xP-yB>0\quad\text{et}\quad c(xP-yB)^2>Cx^2,$$

avec `c=2` en q4 et `c=3` en q3. La preuve vient de la concavité séparée du
prédicat norme--angle sur le produit des deux triangles ; `P` n'est pas un
minorant indépendant des produits scalaires. Après top-`h` par
`(ancre,cellule,lane)`, ce certificat ferme un suffixe entier de hauteurs. La
prochaine ordonnance doit donc faire count--scan/range-report sur ces suffixes,
pas les recompter par une boucle sur `A times B`.

Les groupes coniques doivent subir la même factorisation. Le théorème vaut pour
un crédit `G` de taille quelconque : Carathéodory garantit seulement qu'un
sous-groupe de taille au plus trois existe pour une direction fixée, pas qu'il
faille l'énumérer. Pour une cellule `C=cone(r0,r1,r2)` à hauteur de section
`T`, poser `m_C(s)=min_j r_j dot s`. Le témoin `s` satisfait H2 uniformément
sur le suffixe de hauteur `x` dès :

$$x\,m_C(s)>T\left\lVert s\right\rVert^2.$$

Trier les événements d'activation exacts
`X_s=floor(T||s||^2/m_C(s))+1`. Dans le pool actif, couper les directions par
le plan positif de normale `w=r0+r1+r2` et construire leur enveloppe convexe
2D sans division. La cellule est contenue dans `cone(G)` exactement lorsque
les trois rayons normalisés appartiennent à cette enveloppe. Une triangulation
canonique extrait pour chacun un carrier de taille un à trois ; leur union
forme un crédit de taille au plus neuf. Retirer ses IDs et recommencer donne
`h<=10` crédits disjoints ou échoue fail-open. Cela remplace le catalogue
`C(m,3)` et le 3-set-packing par des enveloppes 2D et produit directement un
suffixe de cibles factorisé.

Pour une direction ponctuelle ou un raffinement d'une cellule, chaque candidat
peut aussi être projeté dans le plan transverse par :

$$V_s=\left\lVert d_0\right\rVert^2s-(d_0\mathbin{\cdot}s)d_0.$$

Un tri angulaire exact y trouve singleton, paire antipodale ou triangle
encerclant l'origine. Ce chemin sert d'oracle et d'ablation de rappel ; un
packing glouton raté ne prouve jamais l'absence d'un packing de taille `h`.

Un groupe plein rang ne doit ensuite pas rester attaché à une seule cible. Pour
un triple `G={s1,s2,s3}`, poser `Delta=det(s1,s2,s3)`, `r=|Delta|`,
`n1=sign(Delta)(s2 cross s3)` et cycliquement, `qi=||si||^2`, puis
`p=sum_i qi*ni`. Le groupe couvre **toute** sphère passant par l'ancre et la
cible `d` si et seulement si :

```text
ni dot d >= 0 pour i=1,2,3
F(d)=r*||d||^2-p dot d > 0.
```

La première ligne est Cramer ; la seconde est l'alternative exacte de Farkas.
Sur un `BNode`, les minima coniques sont affines aux extrémités et le minimum
entier de `F` est séparable, aux deux entiers voisins de `p_k/(2r)` clipés sur
chaque axe. Trois H2 individuelles strictes donnent encore un préfiltre sûr à
six formes `i64`, mais il est incomplet : pour `G={e1,e2,e3}` et
`d=(3,1,1)`, `F=6>0` alors que deux H2 sont à égalité. Le test exact demande
un entier signé d'environ 87 bits sous u16 ; le P0 cellulaire reste `i64` et
ce test devient l'ablation large de rappel.

Pour un groupe arbitraire, « six formes » est faux : l'enveloppe peut porter
jusqu'à environ `2|G|` facettes et strictes. L'alternative bornée prend, pour
chacun des huit coins du `BNode`, un carrier exact de taille au plus trois,
puis rejoue leur union de taille au plus 24. Les cas singleton/paire vivent sur
leurs strates exactes et ne sont jamais promus par une boîte volumique. Cette
ordonnance transforme le certificat de groupe en `ALL/OPEN` factorisé et évite
à la fois le catalogue global de triples et le bitset de `PairId`.

La gate publie événements d'activation, rebuilds/updates d'enveloppe, carriers
de rayons, tailles des crédits, conflits de `PointId`, formes produites, nœuds cibles
`ALL/NONE/MIXED`, masse fermée, blocs résiduels, bytes et HWM. Elle compare le
bitset développé seulement chez le juge borné. Le microprobe ponctuel du pin
`2270077` ne reçoit pas cette ordonnance ; le théorème, ses P0 et la construction
sont séparés dans
[`AUDIT_CONTRE_GROUPES_CONIQUES_2270077_20260813.md`](audits/AUDIT_CONTRE_GROUPES_CONIQUES_2270077_20260813.md) et
[`AUDIT_REPONSE_DOMINANCE_GROUPES_5DDF4A3_20260813.md`](audits/AUDIT_REPONSE_DOMINANCE_GROUPES_5DDF4A3_20260813.md).

Le premier code de crédit cellulaire ne reçoit pas encore cette proposition.
Son événement H2 est correct, mais le défaut `pool=16` interdit structurellement les
huit crédits q4 disjoints : une cellule 3D pleine exige au moins trois IDs par
crédit, donc au moins `24` au total. Il sélectionne en outre seize sites par
distance avant de trier leurs activations, fixe `smax` à `10/9/8`, ne rejoue pas
les carriers avec un juge indépendant et recompte `n(n-1)` cibles. Le premier
raccord rend `8/8` sans fermeture ; à `n=60`, `pool=16/32` paie `43,96 M/350,27 M` tests
coniques pour zéro fermeture q4. Pin et gates :
[`AUDIT_WORKTREE_CREDITS_CELLULAIRES_20260813.md`](audits/AUDIT_WORKTREE_CREDITS_CELLULAIRES_20260813.md).

Le pin logiciel `01a3a3f`, inchangé par le successeur documentaire `88eb36d`,
substitue une enveloppe projective à `C(m,3)` et confirme la
bonne direction algorithmique, mais pas encore l'implémentation : le selftest
aléatoire committé est vert, tandis qu'un exhaustif borné trouve encore faux
positifs et faux négatifs. La branche de rang deux accepte toute la droite
projective au lieu du segment positif. Dans `U00`, le pool
`{(3,1,0),(3,2,0)}` ne contient pas `(3,0,0)` dans son cône, bien que le chemin
live l'accepte. Le rang deux doit recevoir les signes des deux coefficients
coniques ; les directions projectives identiques restent des piles de
`PointId` de rang un.

Ce n'est pas qu'un défaut d'API : seize sites colinéaires de même direction
projective sont groupés deux par deux en huit faux crédits et ferment q4 pour
`d=(18,1,1)`, alors que l'offset de centre `t=(-37,0,666)` rend les seize
puissances strictement négatives. La fixture doit précéder toute nouvelle
mesure de fermeture.
La porte conserve la convention forte : une divergence de marge sans sphère
fautive est une ablation, jamais un mutant tué. Les fixtures exactes pour le
`+1`, le rayon unique, le partage d'IDs et la positivité sont dans
[`AUDIT_REPONSE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md`](audits/AUDIT_REPONSE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md),
qui répond à
[`NOTE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md`](audits/NOTE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md).

Le faux négatif du même delta vient d'un pivot Jarvis situé au milieu d'une
arête en cas d'ex aequo : la marche cycle, puis son cap fabrique un hull. La
route proposée est donc un ordre projectif rationnel total et deux chaînes
monotones ; tout cycle/non-retour reste `UNKNOWN`, jamais un polygone tronqué.
Avec `e` orthogonal à `w` et `f=w cross e`, trier exactement
`((e dot s)/(w dot s),(f dot s)/(w dot s))` évite aussi le tie-break live de
degré quatre, dont une fixture u16 fait déborder `i64` et inverse le signe.

Le successeur commis `090f752` implémente Andrew et le seuil dynamique.
Le build Release/CUDA OFF passe `37 752/37 752` accords, `471` couvertures et
trois contradictions exactes ; un checker indépendant ajoute `1 533` cas avec
`fp=fn=bad_carrier=bad_id=0`. Ce résultat reçoit le correctif local sur son
domaine borné, pas les anciennes masses ni l'ordonnance. La sortie doit encore
être transactionnelle (`union_size=0` et aucun `rank_counts` fusionné sur
`false`) et ses carriers doivent être rejoués. Sinon les planchers de rang
peuvent être servis par des tentatives sans crédit. La fixture singleton « un
seul rayon » tue le théorème mutant via
l'ancien oracle, pas l'injection nominale `cell_covered(rays_one)` ; le hull
plein `G={(6,1,0),(6,-1,0),(6,0,1)}`, `d=(9,2,1)`, `t=(-4,12,12)` exerce cette
route et donne les puissances `-5,-57,-6`.

L'étape positive suivante est elle aussi bornée : trier une fois, conserver les
directions dupliquées en piles `(X,PointId)`, extraire jusqu'à
`h=smax+1-q<=33` carriers rejouables, puis confier le suffixe à un
`CellSuffixReporter` ancre-feuille × LBVH cible. Il classe les AABB par extrema
des facettes half-open et de hauteur, émet les nœuds `ALL` maximaux et conserve
le front `MIXED`. Le live retrie actuellement par insertion à chaque préfixe et
reste cubique au pire ; le partage de l'ordre projectif vise `O(hm log m)`.
Une ancre représentative propose ensuite le même carrier sur un bloc et les
huit coins le recertifient.
Chaque ID classe le rectangle par le minorant exact
`L_z(A,B)=sum_i min_{a_i,b_i}(z_i-a_i)(b_i-z_i)`. Le front publie des
`RectKey/BankKey/CreditKey`, jamais des `PairId`; c'est lui, et non le probe
quadratique, qui passe aux rampes 12,5/25/50 k.

Gate de raccord positive : après translation par `o=(100,100,100)`, prendre
`A={o,o+(1,0,0)}`, `B={o+(100,20,10),o+(110,20,10)}` et les huit crédits
`G_lambda={o+lambda r0,o+lambda r1,o+lambda r2}`. Un unique `RectKey` doit
fermer exactement les quatre couples dirigés q4 à `smax=11`, rester résiduel à
`smax=12`, et développer multiplicité un/digest invariant chez le seul juge
borné.

Le groupe octaédrique partage les neuf tables et le kernel, mais la chambre
dépend de `x-a` : `canon(x-a)` ne se déduit pas de
`canon(x)-canon(a)`. Les 48 ordres relatifs ne disparaissent pas par un tri
absolu. Un lift de `n` points vers `(p,||p||^2)` est autorisé comme index de
requête linéaire, mais un AABB 4D perd la corrélation quadratique ; il ne remplace
le LBVH3D à extrema séparables qu'après une ablation visites/bytes/HWM.

Enfin, pour deux blocs séparés, `d>3S` ne fait que construire un cœur commun.
La fermeture q4/q3 exige encore d'y recevoir respectivement huit/neuf IDs
uniques strictement intérieurs. Tester d'abord deux fixtures identiques à cœur
vide puis occupé ; ne promouvoir une WSPD que si le range-count du cœur ferme
une masse observée. Détails, Q1--Q7 et gates :
[`AUDIT_REPONSES_CLAUDE_GEOMETRIE_3D_20260813.md`](audits/AUDIT_REPONSES_CLAUDE_GEOMETRIE_3D_20260813.md).
La troisième voie est désormais bornée par la note, son contre-audit et la
réponse :
[`NOTE_CLAUDE_GATE_TROIS_VOIES_20260813.md`](audits/NOTE_CLAUDE_GATE_TROIS_VOIES_20260813.md),
[`AUDIT_WORKTREE_COEUR_COMMUN_20260813.md`](audits/AUDIT_WORKTREE_COEUR_COMMUN_20260813.md) et
[`AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md`](audits/AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md).

Le premier probe dominance ne reçoit pas encore cette architecture. Il
énumère toutes les paires, conserve trois bitsets `C(n,2)` et mélange des
mesures directes/radiales. `smax` doit piloter dynamiquement
`h=smax+1-q` ou être refusé hors `11` ; le pin actuel ferme à tort sous
`smax=34`. La gate suivante construit réellement les index, distingue
`residual_pair_mass` de `residual_node_records`, publie bytes/HWM et exige deux
pentes au plus `1,35`. Voir
[`AUDIT_CONTRE_DOMINANCE_432_5DDF4A3_20260813.md`](audits/AUDIT_CONTRE_DOMINANCE_432_5DDF4A3_20260813.md).

Cette séparation masse/stockage est désormais nécessaire, pas seulement
prudente. Pour `n=2m<=50 000`, deux grilles u16 parallèles peuvent être placées
à `x=0` et `x=60000`, avec un décalage transverse tel que toutes les directions
croisées appartiennent à `U00`. Tout témoin du plan opposé a alors la même
hauteur que la cible, tout témoin du plan source est hors de sa chambre, et le
garde direct vaut `9 tau_d-11 tau_h<0`. Les deux orientations laissent donc les
trois lanes ouvertes sur exactement :

$$R_{\mathrm{pair}}=m^2=\frac{n^2}{4}.$$

Le crédit cellulaire uniforme échoue aussi : pour tout site croisé, son
événement strict vérifie `X_s>=60001` alors que la cible a hauteur `60000`.
Cette famille ferme le claim d'un résiduel `PairId` sparse pour dominance et
crédits cellulaires. Elle ne ferme pas le front factorisé, puisque la relation
croisée se décrit par le rectangle unique `A times B`. Les pentes s'appliquent
donc à `node_visits`, `front_records`, octets, copies, high-water et travail du
consommateur ; jamais au champ de masse d'un `RectKey`.

Le plancher est indépendant de ces deux certificats en précision croissante.
Le lemme 5.1 de Chazelle et al. construit `2m` points réels dont `m^2` boules
diamétrales croisées sont vides, donc `m^2` vraies arêtes Gabriel. Aucun
certificat sound exigeant au moins un intérieur universel ne peut les fermer.
Ce résultat interdit un théorème distribution-indépendant de catalogue Gabriel
littéral sparse, mais sa réalisation `50 k` u16 n'est pas reçue et il ne borne
pas le quotient H0 normalisé.

Deux planchers distincts deviennent des objets du ledger. `U_q` compte les
paires pour lesquelles le disque de Jung contient une sphère avec moins de
`h_q=smax+1-q` intérieurs ; il borne les routes dont l'autorité couvre tout ce
disque. `L_q` compte les `PairId` distincts qui sont owners d'au moins un
support propre, positif et pertinent ; il borne toute ordonnance qui doit
produire le catalogue de supports littéraux. Il ne borne pas un calcul direct
du quotient H0 sans catalogue, dont la complétude demande une preuve séparée.
On a `L_q<=U_q`, jamais l'égalité en général.

La séparation est déjà u16. Pour `A_i=(i,0,0)`, `B_j=(0,j,65535)` et
`1<=i,j<=m<=25000`, une sphère strictement vide de centre
`(i,j,(65535^2+i^2-j^2)/(2*65535))` appartient aux disques q3 et q4 de chaque
paire croisée. Ainsi `U_3,U_4>=m^2`. Pourtant aucun support q3/q4 n'est positif
et, à `smax=11`, Source S vaut exactement `20m-55`, soit `499 945` à
`50 k`. Une route exclusivement universelle est donc refusée, mais la
positivité et la source générative restent capables de sauter cette masse. Si
`L_q` lui-même est quadratique, la sortie littérale est dense : son contrat
devient output-sensitive ou retourne `resource_exhausted` atomiquement ;
changer de générateur ne rend pas ce catalogue sparse.

Coordonnées contractuelles, preuve, limite de précision et ordre des jalons :
[`AUDIT_REPONSE_CLAUDE_TUER_LA_VOIE_20260813.md`](audits/AUDIT_REPONSE_CLAUDE_TUER_LA_VOIE_20260813.md).

Le prochain jalon est un walking skeleton, pas une nouvelle rampe isolée. Il
fige `RectKey`, `SupportKey` et `BallKey`, construit le front factorisé, puis
raccorde dans le même jalon une tranche q4 régulière :
`RectTree -> SymmetricAnd -> terminal q4 -> RLE SupportKey ->
lift/positivité -> owner -> RLE BallKey -> census I_B/U_B -> lot gelé -> fold`.
Le premier RLE précède le lift ; le second précède l'unique census. Cette
sortie diagnostique reste `incomplete` relativement à
`BenchmarkOutputContract-v1`. Si un choix binaire est imposé, les rectangles
viennent d'abord, mais aucune admission ne précède le raccord jusqu'au fold.

Le reporter de suffixes doit en outre recevoir les 432 cellules et trois lanes
dans **une seule DFS par ancre**, sous la forme conceptuelle
`report(anchor,root,X[432][3],banks)`. Un appel par seuil réintroduirait jusqu'à
`1296` reprises de racine. Les AABB dont les huit coins ont le même `CellId`
sont homogènes par convexité de la cellule half-open ; les autres restent
`MIXED`. Le ledger impose `root_entries=n`, compte séparément la construction
des banques, émet les nœuds `ALL` maximaux et conserve les fronts
`BELOW_SUFFIX/NO_BANK/CELL_MIXED/HEIGHT_MIXED/RESOURCE_CAP` sans les aplatir.

La prochaine implémentation doit partir d'une partition canonique de rectangles
LBVH, non des ancres : `(N,N)` se décompose en `(L,L),(L,R),(R,R)`. Sur chaque
`A times B`, le cœur commun n'est qu'un fast path opportuniste, puis une cellule
de différences et des témoins communs à toutes les ancres de `A` peuvent fermer
le rectangle. Si `ell` est la hauteur, `zeta_h` le h-ième témoin absolu commun,
`alpha=min_A ell` et `beta=min_B ell`, le pire rapport relatif vaut :

$$r_{AB}=\frac{\beta-\alpha}{\zeta_h-\alpha}.$$

« Commun » signifie ici que, pour chaque témoin sélectionné `z` et tout
`a in A`, les extrema exacts des formes de facette placent `z-a` dans la même
cellule half-open ; les `PointId` sont distincts. Sous les gardes explicites
`zeta_h>max_A ell` et `beta>zeta_h`, ce rapport
croît avec `ell(a)` et `ell(b)` ; les minima `alpha,beta` sont donc bien le pire
cas lorsque le dénominateur est écrit avec les hauteurs absolues communes. Le
cutoff direct ferme alors tout le rectangle, et les témoins crédités précèdent
toutes les cibles. En cas d'échec, scinder `A/B`; ne créer `(a,CellId)` qu'à la
feuille du résiduel. Les deux orientations sont évaluées sur le même `RectId` :
fermeture par `OR`, résiduel par `AND`, sans join matériel de `PairId`.
L'orientation inverse conserve son propre état de cellule : sur une frontière
half-open, elle ne se déduit pas par un simple antipode de la cellule directe.

Les crédits coniques cellulaires viennent après cette dominance et, dans la
première version, seulement pour une ancre feuille. Le pool dépend de
`s=z-a` ; le réutiliser sur un bloc d'ancres sans extrema H2/coniques serait un
faux partage. Le cœur commun reste présent seulement si sa borne et un minorant
d'occupation viennent de la traversée déjà en cours. Les deux amas purs et leur
cœur vide interdisent de construire une WSPD ou un index dédié à ce seul fast
path.

Une extension reçue peut cependant garder le même rectangle sans descendre
toutes les ancres. Une ancre canonique propose au plus trois IDs par rayon ; le
même carrier est ensuite recertifié sur les huit coins de l'AABB d'ancres. Les
déterminants et numérateurs de Cramer sont affines en l'ancre, tandis que, pour
une hauteur cible minimale fixée, la marge H2 est concave ; les coins sont donc
une autorité exacte sur la boîte. L'échec scinde le bloc. La construction
complète par les 24 intersections coin--rayon reste un oracle ou tier de
secours, car elle peut consommer jusqu'à 72 IDs par crédit. Voir
[`AUDIT_WORKTREE_CREDITS_CELLULAIRES_20260813.md`](audits/AUDIT_WORKTREE_CREDITS_CELLULAIRES_20260813.md).

Sur un rectangle fixé, H2 possède en outre un minorant exact sur les AABB, qui
ne dépend plus du cutoff de hauteur :

$$L_z(A,B)=\sum_{i=0}^{2}\min_{a_i\in\left\lbrace A_i^-,A_i^+\right\rbrace,\ b_i\in\left\lbrace B_i^-,B_i^+\right\rbrace}\left(z_i-a_i\right)\left(b_i-z_i\right).$$

Cette formule vient de
`(b-a) dot(z-a)-||z-a||^2=(z-a) dot(b-z)`. Un crédit est H2-`ALL` si
`L_z(A,B)>0` pour chacun de ses IDs. Le minimum est exact sur les boîtes
continues, mais peut employer un coin absent des ensembles de PointId :
`L_z<=0` reste donc `MIXED/UNKNOWN`, jamais `NONE`. Le seuil cellulaire sert de
fast path comprimé, puis ce test bilinéaire conservateur traite son résiduel
avant de scinder.

La version nœud--nœud complète évite de tester les IDs un par un. Pour un
troisième AABB `C`, poser :

$$\Lambda(A,B,C)=\sum_{i=0}^{2}\min_{a_i\in\left\lbrace A_i^-,A_i^+\right\rbrace,\ b_i\in\left\lbrace B_i^-,B_i^+\right\rbrace,\ z_i\in\left\lbrace C_i^-,C_i^+\right\rbrace}(z_i-a_i)(b_i-z_i).$$

La fonction est séparable, affine en `a_i/b_i` et concave en `z_i`; ce test
est donc le minimum continu exact sur `A times B times C`. `Lambda>0` fait de
tout `CNode` un `DIAMETRAL_BOX_CREDIT` q2 pour le rectangle. Une antichaîne de
plages PointId disjointes, également disjointes des endpoints, peut sommer ses
comptes jusqu'à `h_2=smax-1`. Égalité, cap ou échec restent au front ; aucune
population partielle n'est publiée. Sous u16, vingt-quatre produits et deux
additions tiennent dans `i64`.

`Lambda>0` seul ne ferme pas q3/q4 : `a=(0,0,0)`, `b=(10,0,0)`,
`z=(1,2,0)` est intérieur diamétral mais échoue déjà le prédicat universel q3.
Une extension suffisante évite toutefois les `512` triples dans une partie des
cas. Si `E2max` et `X2max` majorent exactement `||z-a||^2` et `||b-z||^2` sur
les boîtes, `Lambda>0 && 4*Lambda^2>E2max*X2max` certifie `ALL-q3`, et le
coefficient `3` certifie `ALL-q4`. L'échec reste `MIXED`; les crédits coniques
et le classifieur exact large restent les étages suivants.

Le `RectId` ne dépend jamais du score de split. Dans un arbre pincé,
`NodeKey=(RelationTreeDigest,Epoch,NodeOrdinal)` ou une clé structurelle
injective équivalente ; `RectId` est la paire ordonnée de deux `NodeKey`.
Un FNV-64 peut authentifier un replay, mais ne décide jamais l'égalité de deux
nœuds ou rectangles. Seuls les enfants canoniques sont admissibles. Une
politique `Lambda-guided` peut choisir de scinder `A` ou `B`
selon la masse q2 prouvée par un lookahead cappé, puis tie-break fixe, à
condition de compter et réutiliser ce travail. Elle est comparée à la politique
canonique pure ; un timeout mural ne décide jamais la topologie du front.

La grille adverse `125 times 200` reçoit une gate fermée : quatre crédits
logiques ferment `624 990 000` paires q2, puis le résiduel exact compte `55`
supports q2 croisés, de profondeurs `0:1,1:2,...,9:10`, représentables par dix
rectangles. Cette fermeture ne dit rien des supports positifs q3/q4. Voir
[`AUDIT_REPONSE_CLAUDE_LZ_RECTANGLE_20260813.md`](audits/AUDIT_REPONSE_CLAUDE_LZ_RECTANGLE_20260813.md).

La masse portée par le front n'est pas nécessairement un compteur de travail.
Elle reste obligatoire pour l'identité
`closed_pair_mass+residual_pair_mass=C(n,2)`, le digest borné et le rapport de
compression `residual_pair_mass/residual_node_records`. Sa pente devient
diagnostique non bloquante seulement après réception d'une borne globale
`W_source<=c_I I_n+c_F F+c_K K`, où `F` compte les records et `K` toutes les
sorties matérialisées ; aucun coefficient ne dépend de la masse. La gate
`<=1,35` porte sur `rect_visits`, records, appels et produits des
classifieurs, `source_tasks/source_point_visits`, occurrences et clés uniques,
census/fold, octets et high-water. Une source dite « par point » ne peut être
`O(n)` par rectangle : aucun rescan racine, aucune expansion de `PairId`, owner
complet et raccord aval dans le même jalon. Tant que ce contrat transitif
manque, la masse reste un risque bloquant ; elle n'interdit toutefois plus la
tranche de recherche du walking skeleton.

La fusion `OR/AND` des orientations ne doit pas matérialiser le résiduel dense.
Les relations dirigées restent une partition canonique de rectangles ; leur
intersection avec la transposée est évaluée paresseusement sur les blocs LCA ou
WSPD par `ALL/NONE/MIXED`, avec owner, masse, digest et caps. Le ledger distingue
la masse sémantique `R_pair_mass`, potentiellement quadratique, du stockage
physique `R_node_records` soumis aux pentes. Sans ce `SymmetricAnd` factorisé, un
radix/RLE par `PairId` recréerait le catalogue global que la v3 doit éviter.

### 11.1.2 Front WSPD, borne supérieure et banque bornée de témoins

Le front `A×B` peut employer une WSPD comme **partition canonique des
relations**, jamais comme approximation d'une décision. Le tape P0 emploie une
séparation entière en norme L-infini, nommée `s_inf`. Pour `s_inf` fixé, un
fair-split tree canonique en dimension trois donne `O(s_inf^3 n)` records.
Cette borne suppose une politique reçue pour les positions dupliquées et les
nœuds de rayon nul, un tie-break terminal par `PointId` et le prédicat pincé.
La seule identité des masses ne prouve pas l'unicité : le juge borné développe
les records et exige une multiplicité un pour chaque `PairId`.

Cette norme ne porte aucune vérité géométrique : les certificats recalculent
leurs bornes euclidiennes. Une variante à boules euclidiennes égales est une
ablation distincte, avec `D2=sum((cA2-cB2)^2)`,
`R2=max(sum(widthA^2),sum(widthB^2))` et le test entier
`D2 >= (s_e+2)^2*R2`. Elle porte un autre `sep_metric` et un autre
`FrontDigest`. Pour le gap normalisé par le plus grand des deux rayons propres
et `s_inf>=2`, le seul minorant déduit de L-infini est
`(s_inf+2)/sqrt(3)-2`, pas
`s_inf/sqrt(3)`.

Le front WSPD borne seulement `front_records` et les nœuds internes de sa
construction à un facteur constant. Il ne borne ni la somme des populations
des blocs, ni un rescan témoin par record, ni la source du résiduel. Les gates
continuent donc de porter sur `rect_visits`, classifications uniques, pushes et
pops, `source_tasks`, sorties, octets et HWM, puis sur le consommateur complet.

Une implémentation Patricia/Karras ne reçoit pas automatiquement la preuve de
l'octree. Un préfixe utile de longueur `3*l+r`, avec `r=0/1/2`, conserve les
`r` bits partiels : sa cellule est un pavé aligné d'aspect au plus deux, pas le
cube obtenu en tronquant `r`. Les frères ont alors des intérieurs disjoints et
la fibre au-dessus d'une cellule octree est explicitement bornable. Le choix du
côté à scinder emploie cette cellule de préfixe. Une boîte serrée peut seulement
ajouter un arrêt `sep_cell || sep_tight` : son centre déplacé interdit de dire
que le prédicat centre/rayon devient monotone par simple inclusion.

Avec `n-1` graines, `T` terminaux et des expansions binaires, l'identité
comptable est `tests=2*T-(n-1)`. Elle remplace toute regex approchée, mais ne
prouve pas `T=O(n)`. La profondeur Patricia est bornée par les quarante-huit
bits Morton utiles, non par `2*log2(n)`. Une preuve de packing porte sur ces
cellules et cette politique exactes ; un degré maximal observé sur trois tailles
uniformes reste seulement un falsificateur.

Une rampe de familles conserve la densité et la géométrie du générateur : pour
chaque taille, elle emploie `cloud_family_default_coord(family,n)`, exige la
cardinalité exacte `m==n` et pince `coord`, graine, bbox, lignes occupées,
profondeurs et splits. Un `coord=65535` commun aux préfixes scanline n'est pas
une rampe homothétique et ne permet pas d'attribuer une inflexion au WSPD.

L'intervalle bidirectionnel de `H` est interprété sur le réseau entier u16. Son
minimum emploie les extrémités en `z`; son maximum emploie le sommet entier de
la parabole, écrêté à la boîte. La preuve vient de l'affinité séparée en `a` et
`b`, pas d'une convexité jointe. Le maximum continu peut différer du maximum
entier. Sous u16, `H` tient dans `i64`, tandis que les carrés q3/q4 exigent deux
limbes.

Un `NONE` propre aux lanes étroites évite des raffinements. Avec
`U=max(Hmax,0)` et `LE,LX` les minima exacts des distances carrées entre
`A,C` et `B,C` :

$$4U^2\leq LE\,LX\Longrightarrow NONE_{q3},\qquad 3U^2\leq LE\,LX\Longrightarrow NONE_{q4}.$$

Les nœuds `ALL`, `NONE` et `MIXED` doivent former une antichaîne qui partitionne
la racine témoin. `cred+pending` est alors un majorant du nombre de témoins
universels possible pour chaque paire. Sous endpoints distincts, être sous dix
prouve un support q2 de rang pertinent. Être sous neuf ou huit ne prouve pas un
support q3/q4 : cela conserve seulement une arête maximale candidate. La sortie
est `KEEP_ANCHOR/DELEGATED_TO_SOURCE`; un nuage collinéaire interdit de la
renommer `POSITIVE_SUPPORT`.

La première réduction de constante ne doit pas être une file dynamique par
rectangle. Les seuils `10/9/8` autorisent une banque partagée de taille fixe.
Pour un rectangle, le milieu des centres de `A,B` définit une requête
bichromatique top-`L`; chaque `PointId` proposé est ensuite recertifié
exactement sur tout `A×B`, avec un masque commun q2/q3/q4. Une banque
incomplète peut perdre une fermeture mais ne peut créer un faux crédit.

Un cœur commun fournit une gate plus forte. Si `A,B` sont contenus dans des
boules de rayons `r_A,r_B`, poser `S=r_A+r_B`, `d` la distance de leurs centres
et `m_0` leur milieu. Pour q2, `d>2S` donne un cœur ouvert de rayon
`(d-2S)/2` autour de `m_0`. Pour q3/q4, `d>3S` donne géométriquement un cœur
ouvert de rayon `(d-3S)/4`, sans hypothèse d'owner. Dix, neuf ou huit IDs
distincts dans le cœur ferment le certificat de la lane. q2 rend
`CLOSED_PAIR_SHARD`. En q3/q4, le rectangle ne peut pas authentifier l'owner
d'un support encore inconnu : il rend un `PRUNED_OWNER_SHARD` conditionnel,
qui supprime tous les supports dont l'arête maximale canonique appartient au
`PairId`-shard du rectangle. Les rayons et frontières sont décidés en carrés
entiers ; aucune hausse globale de `s_inf` n'est nécessaire.

Le résiduel seul reçoit une continuation persistante. Pour chaque
`RectKey/lane`, `Credit ⊔ None ⊔ Mixed` partitionne `C-root`; raffiner remplace
un nœud par ses enfants. Lors d'une scission `A/B`, `Credit` et `None`
s'héritent par monotonie seulement quand `None` est un vrai
`GEOMETRIC_NONE`; un échec de certificateur est `Mixed` et se reclassifie. Un quantum rend
`PENDING_CONTINUATION`; un arrêt de ce certificateur rend
`DELEGATED_RESIDUAL`; une ressource réellement épuisée rend
`RESOURCE_EXHAUSTED` atomiquement. Aucun cap ne change la vérité et aucun enfant
ne recommence depuis `C-root`.

### 11.1.3 Préfixe WSPD mesurable et raccord exact aux carriers

Le jalon de décision est `WspdFrontLowerBound-v1`. Il construit d'abord une
WSPD canonique entière à faible séparation, puis classifie une seule fois ses
terminaux avec un masque q2/q3/q4. Il applique le cœur central et les corridors
d'ordre, fait `count--scan--fill` de toutes les issues et rend un reçu après
synchronisation. Aucun `PairId` n'est développé. Toute ambiguïté devient
`CARRIER_FRONT/DELEGATED`, jamais une conclusion scientifique.

Le cœur central partagé emploie `D2=||b-a||^2` et
`V2=||2z-a-b||^2`. Sur trois AABB, `Dlo` minore `D2` et `Vhi` majore `V2`.
Sous `Dlo>0`, `Vhi<Dlo` certifie q2, `3Vhi<Dlo` certifie q3 et
`209Vhi<=56Dlo` certifie q4. La dernière fraction est strictement sous
`2-sqrt(3)`. Ces tests sont seulement `ALL` fail-open. Le cœur sphérique plus
simple de rayon `||b-a||/4` autour du milieu est lui aussi inclus dans le
spindle q4 : avec `A=||b-a||^2` et `B=||z-m||^2<=A/16`, la marge
`3H^2-E2X2` reste strictement positive. Un juge AABB qui rend `MIXED` ne réfute
pas ce lemme ; le juge indépendant énumère les paires ponctuelles et évalue les
trois prédicats directs.

Le corridor d'ordre emploie la transformée unimodulaire
`T3(x,y,z)=(x-y,y-z,z)`. Pour `L_i=max_A T3_i(a)` et
`U_i=min_B T3_i(b)`, tout `PointId` de la boîte transformée `L<=T3(z)<=U`, hors
endpoints, est témoin universel q2/q3 de `A×B`. Il est aussi q4 si
`L_2<U_2`; sinon q4 reste délégué ou passe par les deux sous-cônes exacts. Les
IDs de plusieurs transformées sont dédupliqués par `(RectId,PointId,lane)`.

Le front q3 ne cherche plus des témoins universels mais des carriers. Pour une
arête owner `ab`, `x` est un carrier aigu exactement si
`H=(x-a) dot(b-x)<0`, `||x-a||^2<=||b-a||^2` et
`||b-x||^2<=||b-a||^2`. Cette relation doit être vide sur tout le produit avant
d'annoncer `SOURCE_EMPTY`; un cap ou une recherche partielle reste
`CARRIER_FRONT`.

Pour q4, deux points de la lentille fermée sont appariés, mais un seul doit
être aigu : `acute(x) || acute(y)`. Le sujet vérifie ensuite
`||x-y||^2<=||b-a||^2`, rang affine trois, positivité stricte et owner. Le
moteur shallow conserve tous les points pour le census ; la lentille filtre les
porteurs, jamais les témoins intérieurs. Bundles, concurrences et shell restent
exactement rejoués au résiduel.

Le quantum de profondeur est une règle versionnée de scheduling. Son
épuisement transfère un état consommable par la source exacte dans le même
`warm_e2e`; il ne change aucune sortie. La porte publie `rect_eval_hwm`, tous
les records et masses délégués, octets/HWM et deux pentes de travail. Le plan,
les fixtures et l'ABI sont détaillés dans
[`AUDIT_DEBLOCAGE_WSPD_PREFIX_CARRIERS_20260813.md`](audits/AUDIT_DEBLOCAGE_WSPD_PREFIX_CARRIERS_20260813.md).

### 11.1.4 `RF-GPU-P0-A` : `Central-VWave`, banque seulement en ablation

Le premier jalon device recherche exactement les crédits du masque central,
sans top-`L`, fenêtre Morton ni heap par rectangle. Pour un terminal `R=A×B`,
poser `D=Dlo(A,B)` et, pour un PointId `z`,

$$S_R(z)=\sum_{i=1}^{3}\max\left(\left|2z_i-A_i^{lo}-B_i^{lo}\right|,\left|2z_i-A_i^{hi}-B_i^{hi}\right|\right)^2.$$

Ce score est exactement `Vhi(A,B,{z})`. Les bits centraux sont q2 si `S<D`,
q3 si `3*S<D`, et q4 si `D>0` et `209*S<=56*D`. Ils vérifient
`q4=>q3=>q2`, tiennent en `u64` sous u16 et saturent à `8/9/10` PointIds
distincts.

Pour un nœud témoin `C`, `Smax` est `rect_v_max(A,B,C)`. `Smin` est aussi
exact sur l'AABB entière : par axe, avec `u=Alo+Blo` et `v=Ahi+Bhi`, minimiser
`max(|2z-u|,|2z-v|)^2` aux entiers voisins de `(u+v)/4`, écrêtés à `C`, puis
sommer. Le classifieur de vague est :

```text
q2 : ALL si Smax<D;        CENTRAL_DEAD si Smin>=D
q3 : ALL si 3*Smax<D;      CENTRAL_DEAD si 3*Smin>=D
q4 : ALL si D>0 et 209*Smax<=56*D;
     CENTRAL_DEAD si D==0 ou 209*Smin>56*D
```

Une tâche porte `(RectOrdinal,CNodeOrdinal,open_mask)`. `ALL` crédite une
antichaîne disjointe et retire les bits saturés ; `CENTRAL_DEAD` retire seulement
les bits du certificat central, sans conclure `GEOMETRIC_NONE` ; `MIXED`
remplace le nœud par ses enfants. Un cap
sérialise la tâche comme `DELEGATED_RESIDUAL`. Aucune issue ne relance
silencieusement `C=root`. Un nœud `ALL` ne peut contenir un endpoint de `A/B`,
mais le replay conserve tout de même jusqu'à dix `proof_ids` et vérifie leur
identité, leur disjonction et chaque bit.

La porte de parcours développe, à petit `n`, tous les nœuds crédités et exige
la multiplicité `0/1` de chaque `(RectId,lane,PointId)`, le cardinal exact de
l'union et un plancher non vide séparé pour q2/q3/q4. Elle juge ensuite chaque
triplet par les prédicats géométriques ponctuels, pas seulement par un second
appel au masque central. Pour tout quantum, la comptabilité est
`tasks_created=tasks_consumed+tasks_pending`; consommer `tasks_pending` doit
donner le même digest final qu'une vague sans cap. Un compteur de tâches
abandonnées ne constitue jamais une continuation.

Une ordonnance locale `locate-and-climb` est une ablation licite seulement si
ses racines initiales partitionnent tout le `PointTree`. Pour une feuille
localisée `z_0`, l'ensemble exact est `{z_0}` plus chacun des sous-arbres frères
rencontrés jusqu'à la racine. Leur union vaut `X` avec multiplicité un. Si une
pile LIFO vise l'ordre proche d'abord, elle empile les frères grossiers avant
les frères proches, puis la feuille en dernier. À quantum illimité, cette
ordonnance doit produire le même ledger de crédits que la graine `C=root` ; une
baisse de fermeture signale du travail omis et interdit toute comparaison de
coût.

La fixture `A={(0,0,0)}`, `B={(20,0,0)}`, `z_0=(10,0,0)` rend cette porte
non vacante. Les ensembles axiaux `x=5,...,14`, `x=6,...,14` et
`x=7,...,14` contiennent exactement `10`, `9` et `8` crédits centraux q2/q3/q4.
Comme `D=400` et `S(x)=(2x-20)^2`, leur appartenance se rejoue sans oracle de
nœud. Toute ablation qui omet `z_0` reste à `h_q-1` et doit échouer.

Cette `Central-VWave` est complète pour les crédits du certificat central ;
elle ne prétend pas être complète pour tous les témoins géométriques. Son
compteur directeur est `J`, nombre de tâches `Rect×CNode` consommées. Le pire
cas peut rester `Theta(Fn)` : deux pentes de `J`, les octets, HWM, rounds et
spills sont bloquants avant tout claim GPU.

La fenêtre `(Morton48,PointId)` `W=32/L=16` reste une ablation propositionnelle
positive. Elle recadre ses bornes, rejette les endpoints, déduplique et
recertifie chaque ID ; fenêtre vide, cap et sous-seuil rendent toujours
`DELEGATED_RESIDUAL`. L'encodeur entrelace les bits aux positions
`3b/3b+1/3b+2` et est jugé contre une boucle 16 bits ; le masque 2D
`0x5555555555555555` est interdit. Sa vitesse ou son pourcentage fermé ne
remplace jamais le ledger de `Central-VWave`.

Une extension q2 calcule `Hmin_singleton` en douze produits `i64`. Sa stricte
positivité est exacte sur le produit des AABB entières et seulement suffisante
sur leurs populations corrélées. Elle vient en ablation après la vague
centrale ; q3/q4 restent au masque commun jusqu'à mesure justifiant les carrés
larges.

q2 rend `CLOSED_PAIR_SHARD`. q3/q4 rendent `PRUNED_OWNER_SHARD` pour les
supports dont l'arête maximale canonique appartient au shard ; une paire nue ne
porte pas le bit owner. Le résultat conserve règle/version d'owner,
`proof_ids`, `closed_mask`, `residual_mask`, masses et digests. L'oracle petit
`n` développe chaque record, rejoue les preuves et exige
`closed_mask ⊔ residual_mask = input_mask` ainsi que chaque PairId exactement
une fois.

Les buffers de vagues sont préalloués, résidents et traités par
`count--scan--fill`, avec compactage stable terminal. La porte publie `F`, `J`,
ALL/CENTRAL_DEAD/MIXED, splits, rounds, preuves, octets/HWM, kernels, syncs et masses
par fate. Elle compare `s=1`, `3/2` et `2` sur les trois lanes. `s=2` reste le
contrôle reçu ; `s=1` est un candidat de coût. `s=4` n'est pas promu par une
masse PairId, puisque la source ne la développe pas. Le choix final minimise
`WSPD + Central-VWave + source + shallow + census + fold` sur trente warms,
jamais un pourcentage isolé.

Les détails de preuve, la réponse à la fourche de coût et les fixtures sont
dans
[`AUDIT_REPONSE_FOURCHE_SOURCE_CENTRAL_VWAVE_DBA8961_20260813.md`](audits/AUDIT_REPONSE_FOURCHE_SOURCE_CENTRAL_VWAVE_DBA8961_20260813.md).

### 11.1.5 Wavefront commune `D,V,T` et relations de carriers

Cette wavefront reste un classifieur partagé et une source de tombstones ; elle
ne doit plus être consommée par un développement de `Acute×Lens`. Le prochain
falsificateur source est la fenêtre projective par ancre de la section 11.1.6.
Le join de relations demeure une ablation/fallback borné si cette fenêtre
échoue, jamais le chemin produit présumé.

Après réception du microkernel P0, le résiduel ne repart pas dans une seconde
source depuis `C=root`. Les `PRUNED_OWNER_SHARD` q3/q4 sont retirés avant cette
wavefront ; seuls les shards délégués y entrent. Pour `d=b-a`, `v=2z-a-b`, poser
`D=||d||^2`, `V=||v||^2` et `T=d dot v`. Les identités exactes sont
`4H=D-V`, `16E2X2=(D+V)^2-4T^2` et l'appartenance à la lentille faible est
équivalente à `V+2*abs(T)<=3D`. Ainsi le même classifieur alimente les témoins
centraux `D>V`, les carriers aigus `D<V` dans la lentille et tous les points de
lentille nécessaires à q4.

`DVT-CWave-v0` porte des masks indépendants witness/lentille/aigu et itère par
`count--scan--fill`. `ALL` émet ou crédite une antichaîne, un vrai `NONE`
retire le mask et `UNKNOWN` seul descend. Un quantum épuisé sérialise une
continuation exacte. Les preuves de P0 initialisent les crédits et sont
dédupliquées avant toute nouvelle saturation. La baseline de correction admet
`F` tâches `(RectId,C=root)`, mais aucun rectangle ne peut être reseedé après
un split ou un quantum.

La route industrielle factorise aussi ces requêtes. Un `QueryTree` canonique
ordonne les RectIds par `(A-path,B-path,RectOrdinal)` et joint ses nœuds au
`PointTree`. Une seule graine `(Qroot,Croot)` est lancée ; `ALL` émet un bloc
`(QSpan,CNode)`, `NONE` retire et `UNKNOWN` scinde `Q` ou `C` avec tie-break de
clé. À feuille `Q`, un warp traite un petit span de RectIds contre le même
`CNode`. Le coût honnête est `O(R+n+J+B+P+H_out)` ; `J` peut encore être
`Theta(Rn)` et aucune boîte agrégée trop lâche ne conclut autrement que
`UNKNOWN`. Les gates exigent `join_root_seeds=1`,
`scalar_rect_root_launches=0`, `tasks_created=tasks_consumed`, octets/HWM et
développement petit `n` de chaque couple `(PairId,PointId)` avec multiplicité
un.

La relation carrier se classe plus finement par marges couplées. Pour
`E=||x-a||^2` et `X=||b-x||^2`, poser `M0=E+X-D=-2H`, `M1=D-E` et `M2=D-X`.
Un carrier vérifie exactement `M0>0`, `M1>=0`, `M2>=0`. Pour un axe, avec
`near(u,I)` la distance carrée à l'intervalle et `far(u,I)` la plus grande
distance carrée à ses extrémités :

```text
M1min = somme min sur a in {Alo,Ahi} de near(a,B)-far(a,C)
M1max = somme max sur a in {Alo,Ahi} de far(a,B)-near(a,C)
M2min = somme min sur b in {Blo,Bhi} de near(b,A)-far(b,C)
M2max = somme max sur b in {Blo,Bhi} de far(b,A)-near(b,C)
M0min = -2*Hmax; M0max = -2*Hmin
```

`ALL` est exact sur l'AABB si `M0min>0`, `M1min>=0`, `M2min>=0`. `NONE` est
sûr si `M0max<=0` ou `M1max<0` ou `M2max<0`, mais reste incomplet lorsque des
contraintes incompatibles échouent sur des points différents. Ces marges
tiennent en `i64`. La source produit `P=AcuteLens` si q3 ou q4 reste ouvert et
`L=Lens` si q4 reste ouvert ; une feuille `MIXED` est
`POSSIBLE_OR_PRESENT`, jamais présence prouvée.

Pour q4, noter `L` la relation lentille et `P` la sous-relation aiguë. Les
couples admissibles sont factorisés par `x∈P,y∈L`, avec owner PointId sur
`P×P`; ils ne sont jamais construits par `C(n_lens,2)`. Les formes affines de
`L` alimentent le moteur mono-ancre shallow, avec un bit `acute` pour `P` ; il
n'émet que les sommets de profondeur au plus `7-credit4` incidents à une forme
aiguë. Le replay vérifie distance `xy`, rang, positivité, owner de support,
`BallKey`, census et shell. Concurrences, bundles et dégénérescences sont des
sorties comptées à `H_out`, pas du travail caché. Les classes historiques
`P/N` du shallow désignent l'orientation des demi-plans et restent distinctes
du bit d'acuité.

Sous split `A/B`, seuls une preuve `ALL` et un vrai `GEOMETRIC_NONE`
s'héritent. `Vbest` impossible, `CENTRAL_DEAD`, fenêtre vide/capée, endpoint
relatif et tout `UNKNOWN` sont reclassifiés : une restriction des boîtes peut
rendre le certificat possible. Le split adaptatif est capé en rondes et chaque
cap rend `DELEGATED_RESIDUAL`; la borne linéaire du WSPD de base ne s'étend pas
automatiquement à ses enfants ni au join source. La directive, les bornes AABB
et la fixture dead-parent/live-child sont dans
[`AUDIT_DIRECTIVE_DVT_CWAVE_4F4B463_20260813.md`](audits/AUDIT_DIRECTIVE_DVT_CWAVE_4F4B463_20260813.md).

### 11.1.6 Fenêtre projective, shallow local et census par `BallKey`

La source exacte évite la fourche « une fois par rectangle ou une fois par
paire ». Fixer une ancre `a`, une cible `b`, `d=b-a` et des vecteurs
`s_i=z_i-a`. Un crédit projectif `G` porte des coefficients positifs tels que
`d=sum_i lambda_i*s_i` et vérifie strictement
`d dot s_i>||s_i||^2` pour chaque membre. Pour tout centre `t=c-a` d'une sphère
passant par `a,b`, l'identité suivante est positive :

```text
sum_i lambda_i*(2*t dot s_i-||s_i||^2)
  = ||d||^2-sum_i lambda_i*||s_i||^2 > 0
```

Au moins un membre de `G` est donc intérieur à toute telle sphère. Avec
`h_q=smax+1-q` crédits dont les unions de `PointId` sont disjointes, toute
sphère passant par `a,b` possède au moins `h_q` intérieurs distincts ; aucun
support pertinent d'arité `q` ne peut contenir les deux points.

Le certificat ferme une **paire**, indépendamment du fait qu'elle devienne ou
non l'owner d'un support futur. L'orientation d'énumération est donc libre. La
choisir par `GenerationRank=(Morton48,PointId)`, et non par le seul `PointId`,
rend toutes les cibles de rang supérieur contiguës dans le `PointTree`. Pour une
paire non ordonnée, poser `a=argmin GenerationRank` et
`b=argmax GenerationRank`, puis :

```text
E_q(a) = { b : GenerationRank(b)>GenerationRank(a)
               et aucun suffixe projectif reçu ne ferme (a,b)
               avec h_q crédits disjoints et rejouables }.
```

Tout état `OPEN`, `MIXED`, `UNDERFULL`, capé, dégénéré ou sans continuation
reçue appartient à `E_q(a)`. Ce n'est ni l'ensemble des vrais co-sommets, ni un
owner attaché à une paire nue. L'invariant exact porte sur le support complet :
si son arête de longueur maximale canonique est l'ensemble `{u,v}`, alors son
endpoint de plus grand `GenerationRank` appartient à la fenêtre ancrée sur
l'autre endpoint. `PairKey=(min PointId,max PointId)` et le tie-break de l'owner
scientifique restent inchangés et non orientés. Sinon les crédits de la paire
imposeraient trop d'intérieurs à la sphère même de ce support. Ses autres
sommets sont générés par la lentille de cette arête. Ils n'ont pas à appartenir
à une fenêtre par owner minimal.

Le pipeline source candidat est donc :

```text
ProjectiveCreditBank propositionnelle
  -> CanonicalEdgeWindowReporter et EdgeWindow E_q(a) en OpenEdgeSpans
  -> EdgeActiveFormCounter dual-tree, M=sum m_ab
  -> formes de lentille et niveaux shallow locaux par arête ouverte
  -> ShallowEvent et bundle incident
  -> BallKey canonique/RLE
  -> census global une fois par BallKey
  -> SupportKey, positivité et owner maximal canonique
  -> fold streamé
```

La fenêtre retire seulement des **arêtes de génération** certifiées mortes.
Tous les `PointId`, y compris ceux absents d'un pool projectif ou d'une
lentille locale, restent dans le census global. Les niveaux shallow sont locaux
à une arête et éphémères ; aucune mosaïque de Delaunay d'ordre supérieur n'est
matérialisée.

Le `BallKey` est l'équation primitive entière normalisée de la sphère. Le
census compte d'abord les intérieurs jusqu'au seuil et tue l'événement avant
toute expansion. Seules les boules survivantes matérialisent leur petit
intérieur, puis leur shell. Une cosphère lourde reste un `PlateauRecord` ou un
refus atomique reçu, jamais une expansion précoce de toutes ses incidences.

Le premier jalon est `PWC0-A/CanonicalEdgeWindowReporter-q4-v0`, sans support ni
CUDA. Il travaille par endpoint feuille `a`, garde un pool transitoire borné par
tuile, traverse des tâches `(AnchorId,BNodeKey,chamber_mask)` et publie
`sum_a|E_4(a)|`, maximum, crédits/IDs disjoints, spans ouverts, tâches, octets,
HWM et `anchor_root_seeds=n`. Ses fates exclusifs sont
`CLOSED_EDGE_SPAN`, `OPEN_EDGE_SPAN` ou `PENDING_CONTINUATION` ; un span en
attente n'est pas aussi compté ouvert. L'oracle petit
`n` exige que l'arête maximale canonique de chaque vrai support q4 reste dans
un span ouvert, puis que le shallow retrouve exactement ce support.
Par lane, `input_span_mass=closed_mass+open_mass+pending_mass`; la fenêtre
finale n'est publiée qu'avec `pending_mass=0`.
Le reporter ferme des paires quelconques ; il ne connaît pas leur futur owner.
L'arête maximale canonique intervient seulement dans cette gate de complétude.

Le reporter reçoit huit crédits et les vrais `PointId`. Il commence par 48
chambres grossières indépendantes ; seule une chambre `OPEN/MIXED` est raffinée
dans ses neuf sous-cellules. Le mode 432 fixe est une ablation séparée. Une
fenêtre dense aux 48 chambres refuse donc cette résolution, pas encore la route
adaptative `48 -> 9`. q3 et q2 ne sont ajoutés avec masque partagé qu'après un
signal q4 positif.

Une banque de proposition bornée est fail-open : candidat manqué, pool
`UNDERFULL`, overflow ou cap agrandit `E_4`, jamais ne ferme un span. Une mesure
dense à `P=96` réfute seulement cette configuration. Le reçu publie au moins
`P=48/96/192` et `UNDERFULL`. La fermeture n'est exigée monotone que si ces
banques sont des préfixes emboîtés et conservent les crédits déjà commis ; un
greedy recalculé peut perdre du rappel. Un `NO-GO` global exige soit une
enveloppe industrielle explicitement figée avec layout et preflight, soit la
stabilisation de cette ablation. Un cap diagnostic `192` et un cap produit `96`
restent deux descripteurs distincts.

Seul l'univers des **cibles** est le suffixe de `GenerationRank`. La banque de
témoins reste libre de proposer tout `PointId` sauf l'ancre ; la limiter au
suffixe serait sûr mais détruirait du rappel. Dans une cellule 3D pleine, un
groupe couvrant les trois rayons consomme au moins trois IDs. Une fermeture q4
à huit groupes exige donc au moins vingt-quatre IDs actifs distincts dans la
cellule ; `active_pool<24` produit immédiatement `STRUCTURAL_UNDERFULL`. Le reçu
publie l'histogramme du pool actif par cellule, distinct du cap global `P`.
Pour rendre l'ablation `48/96/192` monotone, conserver chaque preuve huit-
disjointe déjà reçue et prendre le meilleur cutoff parmi les preuves
alternatives ; ne jamais repacker puis jeter l'ancien certificat.

Si `PWC0-A` reçoit une fenêtre sparse mais que ses `n` graines racine ou ses
tâches dominent, `PWC0-B` universalise les mêmes preuves sur un `ANode` et
traverse `ANode×BNode` depuis une graine unique. Comme les deux nœuds varient,
huit coins de l'`ANode` ne suffisent pas. Un vérificateur rectangulaire exact
rejoue les mêmes witness IDs et leur disjonction sur tout `A×B`; il emploie les
`8×8` couples de coins seulement pour les quantités prouvées multi-affines et
des extrema dédiés pour les autres. Toutes les différences `B-A` restent dans
la même cellule half-open ; cône, H2 et hauteur minimale sont uniformes. Les
rangs inférieurs, signes non uniformes ou bornes non reçues restent ouverts. Ce
partage exige une récursion root×root canonique et publie `rect_tasks`, splits
`A/B` et prédicats larges ; il est le jalon suivant, pas une précondition du
falsificateur feuille.

Deux pentes supérieures à `1,35` sur une métrique physique bloquante, une HWM
hors enveloppe ou une fenêtre quasi quadratique rendent la configuration
`NO-GO` avant le shallow. La masse logique seule n'est jamais un temps.

Le compteur du pin `32589ad` ne satisfait pas cette définition. Pour chaque
terminal q2 central ouvert `A×B`, il ajoute les deux orientations et vérifie
identiquement `sum_N=2*sum |A||B|`. C'est un degré résiduel central redondant
avec la masse, sans crédit projectif, owner, q3/q4 ni span ; sa pente n'entre
pas dans sa gate. Il ne choisit donc aucune séparation WSPD. Le contre-audit et
la boucle exacte du reporter sont dans
[`AUDIT_CONTRE_COMPTEUR_FENETRE_32589AD_20260813.md`](audits/AUDIT_CONTRE_COMPTEUR_FENETRE_32589AD_20260813.md).

La masse dirigée de `E_q` n'est qu'un ledger d'arêtes. Une incidence ouverte
`(a,b)` n'est pas une forme : chaque site actif `z` fournit sa propre
`LineForm`. Avant tout `PlaneTape`, un `EdgeActiveFormCounter-v0` doit mesurer
factoriellement :

```text
M = sum_(a,b in E_q) m_ab,
```

où `m_ab` est le nombre de copies de formes actives de l'arête. Même
`|E_q|=O(n)` avec `m_ab=Theta(n)` reste quadratique. Le compteur publie tâches
arête×site, tests de nœuds, blocs factorisés, hits ponctuels, `M`, maximum par
arête, octets/HWM et continuations. Il utilise un dual-tree exact
`(EdgeSpan,CNode)` avec verdicts AABB et ne peut revendiquer l'absence de
produit avant que cette factorisation soit reçue. Le `PlaneTape` n'est
matérialisé par `count--scan--fill` qu'après
les deux portes `E_q` et `M`. Cette expansion tardive est distincte d'un
fallback qui développerait la masse brute du WSPD.

Une banque par distance n'est pas un cutoff projectif. Dans la chambre de
rayons `(3,0,0),(3,1,0),(3,1,1)`, `s_near=(1,-2,0)` a norme carrée `5` mais
activation `16`, tandis que `s_far=(4,0,0)` a norme carrée `16` mais activation
`5`. Les `CreditKey` conservent donc les vrais `PointId`, les trois preuves de
rayons et un commit transactionnel ; une cellule incomplète reste ouverte.

Le lemme est ponctuel en `b`. Fermer un `BNode` exige une preuve uniforme : la
représentation conique et toutes les strictes valent pour chaque cible du span,
avec coefficients/certificats rejouables ou une réduction AABB reçue. Un crédit
obtenu pour un représentant ne s'étend jamais à sa cellule. La réplication d'un
groupe ne permet pas non plus de réutiliser ses PointIds deux fois dans le même
certificat de paire.

Pour un triple plein rang, cette universalisation ne demande pas de tester un
représentant. Les trois minima coniques et le minimum entier séparable de
`F(d)=|Delta|*||d||^2-p dot d` donnent un `ALL` nécessaire et suffisant sur la
boîte de différences du `BNode`. Le fast path à trois H2 strictes reste sûr
mais incomplet et tient en `i64`; le test `F` exige environ 87 bits sous u16.
Le reporter v0 commence néanmoins par les suffixes `i64` des 48 chambres : le
triple exact large est une ablation de rappel et les 432 sous-cellules une
ablation de résolution, jamais trois résidences obligatoires. Fixtures, ABI et
ordre sont pincés dans
[`AUDIT_DIRECTIVE_BNODE_PROJECTIF_ET_ARRET_CLIMB_75F16DB_20260813.md`](audits/AUDIT_DIRECTIVE_BNODE_PROJECTIF_ET_ARRET_CLIMB_75F16DB_20260813.md).

La construction de `ProjectiveCreditBank` reste elle-même à factoriser. Un cap
conserve les cellules non classées dans `E_q(a)` ; aucune recherche partielle ne
ferme un suffixe. Le premier reçu publie tâches de banque, produits,
répétitions `(CreditKey,PointId)`, continuations et digests, pas seulement la
taille finale des fenêtres.

Avant de remplacer la baseline, un petit adaptateur borné
`BallFormToBallEvent-v0` fournit l'autorité de comparaison qui lui manque : clé
primitive de sphère, RLE, un census `I_B/U_B` par `BallKey` et liste des
`SupportKey` incidents. Il ne répare aucune pente et ne doit pas être rampé ;
il permet seulement de comparer reporter, shallow et fold sur une identité
output-bearing commune.

La preuve, l'ABI, les contre-fixtures et l'ordre d'implémentation sont détaillés
dans
[`AUDIT_REPONSE_FOURCHE_SOURCE_AF08B0E_20260813.md`](audits/AUDIT_REPONSE_FOURCHE_SOURCE_AF08B0E_20260813.md).

### 11.2 Tuilage spatial et fold

À dix millions de points, les listes de voisins et les supports sont calculés
et streamés par tuiles, mais les tuiles ne sont pas des domaines topologiques
indépendants. L'index ou l'annuaire global certifie la coupure k-NN ; le
résiduel conserve les supports transfrontières ; les occurrences sont regroupées
globalement par `SupportKey/BallKey` ; les flux sont fusionnés par ordre et
niveau exact. Toutes les racines d'un lot égal sont gelées avant ses incidences,
puis un unique commit global ferme le lot. Un DSU des seuls `PointId` ne
remplace pas les handles de facettes et carriers aux ordres supérieurs.

Le résident minimal candidat comprend points/identités, index global, état du
fold, lot courant, fronts résiduels, deux buffers de travail, ancres verticales
et manifestes. Il exclut toutes les banques par point, tous les supports, Gamma
et toute mosaïque. Un ledger réel compte remaps, nœuds LBVH, workspaces de tri,
files spillées, sorties et mémoire hôte épinglée ; les `60 MB` de coordonnées à
`10^7` ne constituent pas une enveloppe mémoire.

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
- `SupportKey` avant le premier RLE, `GeometricBallKey` sur la side queue ou
  avant le second RLE global, `U_B` après census et preuve des doublons ;
- incidences fast/fallback, hits prévus/lus, resolver et ledger pré-DSU ;
- octets de chaque arène, workspace, pile, high-water et marge ;
- temps séparés build, source, census, resolver, fold, payload et
  `warm_e2e`.

Toutes les sommes et tailles sont calculées en entiers vérifiés avant cast
vers l'ABI device. Une erreur d'identité, un cas mathématique non supporté et
un refus de ressource sont trois statuts distincts.

## 13. Jalons

1. Installer d'abord le squelette ABI de `BenchmarkOutputContract-v1`, le
   payload et l'interface verticale, avec producteurs explicitement
   `incomplete`; taguer chaque chantier `slo_critical_path=yes/no`. Conserver
   ensuite générateur, self-joins, sidecar borné, cellules et ancres comme
   portes locales ou oracles. Fermer les identités persistantes et les juges
   indépendants sans promouvoir ces parcours exhaustifs.
2. Pour `k=1`, conserver le Borůvka point--LBVH comme diagnostic et produire
   le transcript Yao-1 exact mutualisé avec q2 : fermeture des ex æquo et du
   vide par chambre, au plus `48n` candidats, réduction sparse puis tri des
   arêtes finales par niveau.
3. Seulement si la comparaison q2 rouvre la voie suspendue, réemployer les
   motifs de lease, ledger et `count--scan` de la ligne
   enregistrée, sans copier ses layouts binary64 ni ses décisions de rang
   fermé. Fermer q2 par top-`K` exact ou réservoir arbitraire `K+1`, puis
   cascade cône--boîte, banque affine et dual-tree résiduel avec maximum entier
   exact, masques de feuilles et rollback. Réserver la classification terminale
   et le census fermé multi-ordre au résiduel; fermer reçu, mutants, télémétrie
   et deux pentes avant CUDA. Conserver le self-join comme oracle ou second
   prune selon les masses.
4. Évaluer d'abord la sous-source de fenêtre : top-`M+1` exact, coupure au
   premier omis, génération indépendante q2/q3/q4, census fermé, owner tardif
   et oracle par identités. Construire une vraie partition de domaine
   `certifie/residuel` qui couvre aussi les supports jamais proposés ; mesurer
   requêtes, propositions, positivité, census et tâches résiduelles. Conserver
   ensuite les probes q4 mass-only comme falsificateurs, jamais comme sources.
   Appliquer le cœur universel de Jung avant une wavefront témoin persistante
   munie des bornes dirigées `L/U`; fermer le juge, puis recevoir partition
   triangulaire implicite, 64 patches, seuil huit, microtuiles terminales et
   ledger complet. Cette tranche n'émet aucune ancre.
   Après le différentiel hôte à `n=32`, la même session G4 ferme la parité
   native et `n=32` sous Compute Sanitizer, puis va directement au profil 50 k.
   Une masse majoritairement terminale, un rescan par bloc ou une queue lourde
   arrêtent la route avant son extension à P1.
5. Sur les seuls blocs encore admis, mesurer d'abord le classifieur collectif
   `NONE/ALL/UNKNOWN` de lentille aiguë ; `NONE` ferme la masse avant `PairId`,
   `ALL` reste factorisé et `UNKNOWN` se subdivise. Recevoir séparément
   Jung--Yao, la borne AABB `g_min/Q_max`, Helly, la composition
   cœur--profondeur et la profondeur terminale. Mesurer le gain marginal de
   chacun contre son coût exact ; aucun rescan racine n'est admis comme route.
6. Garder q2/q3/q4 par lanes indépendantes et budgets `h` comme comparateur,
   avec partition terminale commune, `e0` immuable et promotion, sans dépendre
   des supports inférieurs retenus et sans supprimer le transcript Yao-1 de
   `k=1`. Pour la route front, recevoir l'arête maximale canonique, puis
   remplacer `C(nlens,2)` par les niveaux mono-ancre `P-P/N-N/P-N` sur leurs
   segments actifs ou par une shallow cutting certifiée. Le rang restreint
   génère seulement des centres ; un census complet décide `p,I_B,U_B`.
   Recevoir le patch half-open et `occurrences=SupportKey_unique`; son RLE
   devient vérificateur.
   Pour la baseline cellulaire, émettre les occurrences compactes, faire le RLE
   `SupportKey` avant le lift, chercher directement la feuille owner et rejouer
   son pool; transporter les contextes seulement si la partition terminale
   n'est pas directement adressable.
   Comparer lots spatiaux de feuilles atomiques et shards radix par clé. Dans le premier cas, la feuille owner commune rend le
   second RLE par clé primitive de sphère local. Dans le second, redistribuer les
   pending owner par `GeometricBallKey/OwnerCellId` avant ce RLE : un owner
   commun ne colocalise pas des shards `SupportKey` distincts. Comparer ce
   second RLE global au fast path par clé : employer d'abord le census reçu du
   producteur, sinon top-`(12-q)` hors `U`. Seuls `delta>beta` et `E=U`
   publient directement la branche régulière; toute extra-shell, toute égalité
   et toute demande Gamma route vers la clé de boule et le range-report. Pour
   le census pool, choisir un contexte
   `b_cert>=H_run` et faire un unique census par boule;
   `U_B` est un certificat aval. Gamma
   conserve les `SupportKey` requis; le H0 normalisé
   emploie le token Johnson et un support canonique. Graver les fixtures
   q3-sans-q2, q4-sans-q3, pool-relative, budgets indépendants et shell 30.
   La famille exacte à deux droites reste une gate adversariale : toute
   matérialisation quadratique du front est refusée alors que Source S reste
   linéaire.
7. Fermer le domaine dégénéré et le cas terminal `k=n`, puis recevoir
   `BallActivation`, census `I/E`, source directe, trois branches `J_F`,
   resolver strict, MSF/fold et reconstruction des verticales contre Gamma
   exhaustif à petit `n`. Une extra-shell pertinente passe par un quotient de
   plateau reçu ou échoue fermée; elle n'est jamais assimilée à un support
   minimal multiple.
8. Brancher deux harnesses nommés sur les ABI déjà installées : le diagnostic
   horizontal réduit et le `BenchmarkOutputContract-v1` officiel. Ils ne
   partagent aucun verdict SLO.
9. Appliquer la gate `12 500/25 000/50 000` aux autres routes de source,
   porter seulement les routes admises sur CUDA avec arènes préallouées et une
   synchronisation terminale, puis mesurer le payload officiel complet dans
   un même `warm_e2e`.

## 14. Conditions de GO

Le backend G4 devient candidat uniquement si :

- les sources q2/q3/q4 ont une preuve de complétude sans atlas d'ordre
  supérieur caché ;
- la sous-source locale et le résiduel forment une partition d'identités, et
  le résultat est invariant entre une, deux et plusieurs tuiles ;
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

## 15. Déblocage de la source au pin `f02d5ed`

Le producteur par arête courant reste une excellente baseline différentielle,
mais son ordonnance n'est pas le `LocalShallowBall` produit. Après fixation de
`(a,b)`, `kept(a,b)` contient les sites dont la marge varie sur le disque de
Jung ; q4 parcourt ensuite explicitement `C(n_lens,2)` et rappelle le census par
support. `hw_kept` est un maximum par paire. Il n'est pas comparable à l'ancien
`sum_N`, qui vaut exactement deux fois la masse q2 ouverte et ne porte aucun
crédit projectif.

Le déblocage est une chaîne de quatre objets, chacun falsifiable séparément.

### 15.1 `BallFormToBallEvent-v0` : premier objet consommable

Avant toute optimisation, enrichir la baseline bornée. Pour une boule
`c=a+Y/Delta`, former la clé primitive entière :

```text
A=Delta
B=-2*(Delta*a+Y)
C=Delta*||a||^2+2*a dot Y
```

Réduire les cinq coefficients par leur pgcd commun et imposer `A>0`. Faire un
RLE par `BallKey`, puis un unique census global par boule qui conserve les vrais
ensembles `I_B` et `U_B`, pas seulement leurs cardinalités. Joindre tous les
`SupportKey` incidents. La clé logique est
`(CloudEpoch,A,Bx,By,Bz,C)` : elle ne contient ni `q`, ni owner, ni `p`. Pour
chaque `PointId z`, le census évalue directement
`A*||z||^2+Bx*z.x+By*z.y+Bz*z.z+C`; les signes négatif, nul et positif donnent
respectivement `I_B`, `U_B` et l'extérieur. Chaque support incident doit être
inclus dans `U_B`, conserver son owner et appliquer séparément `p+q<=smax`.
Une même boule peut donc être admise dans une lane et refusée dans une autre.
`U_B=S` donne la branche régulière ; `U_B!=S` va vers un `PlateauRecord`
lossless, un quotient reçu ou un refus explicite. Ce pont reste lent et borné :
il fournit l'identité output-bearing à laquelle comparer les remplacements,
pas une route à ramper.

Sous u16, les coefficients non réduits et l'évaluation directe restent en
signé 128 bits : la borne la plus large du chemin q3 est inférieure à `2^106`.
Cette borne n'autorise pas des produits croisés supplémentaires non bornés.
Réutiliser `ExactCenterKey/same_exact_ball` ou un `BigInt<4>` reçu pour tout
autre comparateur. La version device/persistante encode chaque entier signé en
deux limbs `u64`; elle ne sérialise pas la représentation native de `__int128`
et ne compare jamais la clé par `memcmp`. Les coefficients primitifs encodent
centre et rayon ; le record ajoute schéma, `CloudDigest` et `Epoch`, tandis que
lane, niveau, owner et `p` restent des champs d'activation séparés.

La fixture permanente de cosphère, cocyclique dans `z=5`, prend les six points
`(10,5,5),(8,9,5),(2,9,5),(0,5,5),(2,1,5),(8,1,5)`. Les deux triangles alternés
sont aigus et portent la même boule de centre `(5,5,5)` et rayon `5`. La porte
exige donc plusieurs `SupportKey`, un seul `BallKey`, un seul census et les six
IDs de shell. En ajoutant le centre et `(6,5,5),(4,5,5)`, la clé de rayon `5`
vaut `(1,-10,-10,-10,50)`, avec trois IDs intérieurs et six sur la coquille ;
la boule concentrique de rayon `1` vaut `(1,-10,-10,-10,74)` et ne doit jamais
être fusionnée avec elle. La fixture owner q3
`x=0:(5,6,0),a=1:(0,0,0),b=2:(10,0,0)` attend la clé
`(6,-60,-11,0,0)` et l'arête owner `(1,2)`. Enfin, le tétraèdre u16
`(0,0,0),(65535,65535,0),(65535,0,65535),(0,65535,65535)` attend
`(1,-65535,-65535,-65535,0)` et tue tout intermédiaire rétréci en `i64`.

### 15.2 `CanonicalEdgeWindowReporter-q4-v0` : tuer les paires avant la source

Pour chaque premier endpoint `a`, définir `E_q(a)` comme les seconds endpoints
de `GenerationRank` supérieur non fermés par `h_q=smax+1-q` crédits projectifs
aux unions de `PointId` disjointes. Tout support vrai conserve nécessairement
son unique arête maximale canonique dans cette fenêtre, car un crédit projectif
tue toute sphère passant par ses deux endpoints, indépendamment du sens de
génération et de l'owner futur.

Le P0 feuille `PWC0-A` construit une banque transactionnelle de vrais
`PointId`, essaie les 48 chambres, puis raffine seulement chaque chambre ouverte
dans ses neuf sous-cellules. Il émet des `OPEN_EDGE_SPAN`, des
`CLOSED_EDGE_SPAN` ou une continuation fail-open comme fates exclusifs. Il publie
`sum_a|E_4(a)|`, le maximum, tâches, activations, tests Andrew, octets et HWM.
Le ledger `input=closed+open+pending` est exact et la fenêtre finale exige
`pending=0`.
Le sous-remplissage de la banque ouvre, jamais ne ferme. Comparer au moins
`P=48/96/192`; la monotonie exige des banques préfixes et la conservation des
crédits déjà commis. Si les `n` graines racine deviennent le verrou après une
fenêtre sparse, seulement alors universaliser les mêmes preuves sur
`ANode×BNode` dans `PWC0-B`.

Le raffinement `48 -> 9` porte sur les **spans de cibles** encore ouverts dans
la chambre half-open, pas sur un bit global de chambre. Pour une cible, le
verdict sûr est `coarse_certificate OR fine_certificate`. Les deux preuves sont
alternatives : on ne somme jamais des crédits issus de sous-cellules différentes
et aucune disjonction globale d'IDs n'est exigée entre chambres ou résolutions.
Seuls les huit groupes de la preuve effectivement consommée sont disjoints. Un
`BNode` traversant une frontière est scindé ou laissé ouvert ; une feuille est
classée dans son unique cellule half-open.

Une fenêtre d'arêtes sparse n'autorise pas encore les niveaux. Le jalon
`EdgeActiveFormCounter-v0` traverse un dual-tree `(EdgeSpan,CNode)` avec bornes
exactes de lentille/marge et mesure
`M=sum_(a,b in E_4)m_ab`, tests de nœuds, blocs factorisés, hits ponctuels,
tâches, maximum par arête, octets et HWM. Une pente rouge de `M` ou des tâches
arrête la route même si `|E_4|` est linéaire.

Deux certificateurs `ALL` suffisants peuvent être réunis par OR, sous masque de
lane et antichaîne sans double crédit. Cette sûreté ne donne aucune loi de coût.
Le juge du pin ne reçoit pas encore cet OR : il recompte uniquement les crédits
centraux et la commande avec `--fallback` rend `exit=1`, `2864` désaccords à
`n=1200`, alors que les cinq CTests sans fallback passent. Un replay par
`ProofSpan` et prédicat ponctuel indépendant est requis.
Si `c(a,b)` est le nombre de crédits reçus, la fenêtre au seuil `h` vaut
`{(a,b):c(a,b)<h}` et peut sauter presque instantanément de vide à quadratique.
Ne revendiquer ni `Theta(Kn)`, ni indépendance de `s` et `K` depuis deux seuils
du complément q2. `s=2` reste une ablation du tape jusqu'à mesure du vrai
reporter q4 et du coût composé.

### 15.3 Vrai `LocalShallowBall` sur les seules arêtes ouvertes

Pour les formes `F_i(x,y)=A_i*x+B_i*y+C_i` du plan médiateur, séparer les côtés
positifs au-dessus `P` et au-dessous `N`. À q4, avec
`k=7-always_inside`, streamer les `k+1` niveaux inférieurs de `P` et supérieurs
de `N`. Générer uniquement les sommets `P-P`, `N-N` et les intersections des
segments actifs `P-N` dont les rangs satisfont `r+s<=k`. Grouper atomiquement
les concurrences par centre rationnel avant rang strict, Jung, positivité,
owner et census.

Cette ordonnance remplace `C(n_lens,2)` pour les centres shallow distincts. À
`m_ab` copies de formes et profondeur stricte `k`, le nombre de centres est
inférieur à `e*(k+1)*m_ab` et les incidences à
`2e*(k+1)*m_ab`. Cette borne ne contrôle ni les couples de segments visités, ni
les `SupportKey` incidents à une concurrence lourde ; ces deux sorties restent
mesurées. Sous les hypothèses du constructeur reçu, la cible par arête est
`O(m_ab*k^2 + m_ab*alpha(m_ab)*log(m_ab) + |V_ab| + J_ab + H_ab)` ; les
opérations de segments et la HWM sont des compteurs bloquants séparés. Aucun
arrangement global n'est construit : les niveaux sont locaux à
une arête, éphémères et détruits après les `BallEvent`. Le probe CPU compare exactement
`(BallKey,SupportKey,I_B,U_B,owner)` à `BallFormToBallEvent-v0`. Les mutants
omettent séparément `P-P`, `N-N`, `P-N`, le niveau terminal, le batch des
concurrences et l'exclusion du shell du rang strict.

### 15.4 Fold streamé au lieu du catalogue de supports

Le payload officiel ne demande pas le retour hôte de tous les supports. Après
census régulier, former un `RegularDirectRecord` compact portant niveau exact,
`BallKey`, `Q=I_B union S` avec `|Q|<=11` et masque de support. Le census doit
conserver les IDs de `I_B`, y compris `always_inside`; `(p,extra)` ne suffit
pas. Si `r=|Q|`, ce record porte logiquement une naissance à `k=r` et une
coface directe à `k=r-1`, dans leurs fenêtres respectives. Il n'est pas
dupliqué : les runs sont triés par `(beta_exacte,event_key)`, puis le macro-lot
dérive les deux rôles sur un même snapshot pré-lot gelé.

L'émission par ancre n'a aucun watermark monotone : un événement à
`beta(Q)` peut révéler un bras ou une gateway de niveau strictement inférieur.
Il faut spooler les directs, RLE les `FacetKey`, résoudre les gateways, sceller
le manifeste de source, puis seulement merger les niveaux et committer le
fold. Le streaming supprime le catalogue résident ; il ne permet pas un commit
au fil de l'émission.

La conservation exacte des composantes par MSF est standard sur un graphe déjà
complet. Elle ne remplace ni naissances, facettes égales, `coverage_delta`,
gateways, verticales, ni complétude de source. La rétraction
`directes+gateways`, ses verticales et son quotient de plateau ne sont **pas
encore reçus**. `AnchorOutputFoldCounter-v0` doit comparer les dix
forêts, lots, verticales et coverage à Gamma exhaustif sur petit `n`, puis
comparer run matérialisé, streamé, permuté et tuilé. Aucun catalogue global de
supports, Gamma ou mosaïque d'ordre supérieur n'entre dans le chemin candidat.

La porte complète est détaillée dans
[`AUDIT_CORRECTION_FOLD_STREAM_REGULIER_4CE3618_20260813.md`](audits/AUDIT_CORRECTION_FOLD_STREAM_REGULIER_4CE3618_20260813.md).

### 15.5 Porte de coût

Le chemin actuel est déjà réfuté par `eight_clusters,n=500` et ne mérite aucune
rampe `50000`. Chaque nouvel objet passe d'abord fixtures/petit oracle, puis
`1500/3000/6000`. Seulement si ses compteurs physiques restent admis, exécuter
`12500/25000/50000` sur `uniform` et `eight_clusters`, avec deux pentes
consécutives sur tâches, centres, `BallKey`, census, octets et HWM. Le premier
kernel G4 vient après ce vert ; le p95 contractuel demande ensuite trente
répétitions chaudes à `50000` du même `BenchmarkOutputContract-v1` complet.

GCP non utilisé pour cette proposition.

## 16. Réponse au front de triplets aigus du pin `4ce3618`

La caractérisation q3 proposée est correcte : sous rang affine deux,
`ab` maximale faible et angle en `x` strictement aigu équivalent à un triangle
aigu dont `ab` est une arête maximale. Elle ne sauve pas le certificat par
hull.

Si `a0,b0` réalisent `Dmin`, alors pour tout `z` :

```text
max(||z-a0||,||z-b0||) >= Dmin/2.
```

Les deux endpoints appartiennent au hull des trois boîtes. La condition
`max_(c in hull)||z-c||^2<Dmin^2/4` est donc identiquement fausse. Son échec est
un `CERTIFICATE_MISS`, jamais un `NONE`. La fixture
`a=(0,0,0),b=(4,0,0),x=(2,3,0),z=(2,1,0)` porte même un vrai témoin intérieur
que ce test manque.

Ne pas implémenter ce prune. La borne d'empilement `O(s^6 n)` peut seulement
décrire un front grossier de cellules dyadiques canoniques de taille comparable,
sans raffinement non borné des `MIXED`. Elle ne vaut pas pour des boîtes serrées
arbitraires et ne borne ni la masse relationnelle ni la sortie. Une future
ablation `AcuteCarrierCellFront-v0` se place dans
`EdgeActiveFormCounter-v0`, emploie le classifieur carrier par marges et transmet
tout `MIXED`; elle ne précède pas `PWC0-A` et ne retarde pas le pont BallKey.

La démonstration et les réponses directes sont dans
[`AUDIT_REPONSE_TRIPLETS_AIGUS_4CE3618_20260813.md`](audits/AUDIT_REPONSE_TRIPLETS_AIGUS_4CE3618_20260813.md).
