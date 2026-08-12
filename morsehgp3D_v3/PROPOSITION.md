# MorseHGP3D v3 — proposition d'architecture courante

Date : 12 août 2026 UTC.

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
seulement sur l'ensemble `E` des lignes qui peuvent porter un carrier est une
génération superset exacte : au centre d'un vrai support, ses deux lignes sont
incidentes et le nombre de lignes de `E` strictement positives est au plus le
census global privé des `c` témoins permanents, donc au plus `k`. Retirer les
formes non-carriers ne peut qu'abaisser cette profondeur. Le rang de cet
sous-arrangement n'est toutefois jamais publié comme `p`; le census final
rejoue tous les sites nécessaires à `I_B` et `U_B`. Si un site écarté par
`U_z<theta` était shell d'un centre, les `smax-2` bornes qui définissent
`theta>0` donneraient déjà autant d'intérieurs stricts et le support serait
hors budget ; le shell d'une sortie acceptée est donc conservé. En revanche,
les identités des `always_inside` doivent encore être transportées pour
recevoir `I_B`, pas seulement leur cardinal.

Cette structure mono-ancre ne doit pas être confondue avec le plein arrangement
relevé de la section suivante. Si `m=|E|`, le nombre de centres géométriques
distincts de profondeur au plus `k` est inférieur à `e(k+1)m` pour `k>=1`, et
au plus `m` pour `k=0`. La preuve échantillonne chaque ligne avec probabilité
`1/(k+1)` et injecte tout sommet retenu dans un sommet de l'intersection convexe
des demi-plans négatifs échantillonnés. Cette borne n'affirme ni que la somme
des `m` sur toutes les ancres est linéaire, ni qu'une concurrence ne porte pas
quadratiquement beaucoup de `SupportKey`.

Une ordonnance exacte concrète choisit un chart entier du plan médiateur et
une cisaille unimodulaire sans ligne verticale, puis sépare les formes `P`
positives au-dessus de leur ligne et `N` positives au-dessous. Hors shell :

$$p_E(x,y)=\#\left\lbrace i\in P:l_i(x)<y\right\rbrace+\#\left\lbrace i\in N:l_i(x)>y\right\rbrace.$$

Construire les `k+1` niveaux inférieurs `0..k` de `P` et supérieurs de `N`
suffit. Les candidats sont les sommets `P-P` dont le rang opposé ferme le
budget, les sommets `N-N` symétriques et les overlays des **segments actifs**
des niveaux `r,s` avec `r+s<=k`. Les droites porteuses entières ne sont jamais
croisées deux à deux. Une construction conservatrice vise
`O(m log m+m*k^2+V+H)`, où `V` compte les centres shallow uniques et `H` les
supports réellement développés aux concurrences. Tout candidat valide ensuite
Jung, indépendance affine, positivité, les six distances, owner, census et clé.

Les parallèles distinctes ne créent aucun événement. Les droites confondues
sont des bundles avec identifiants et orientations ; les intersections
multiples sont groupées par centre rationnel et traitées atomiquement, toutes
les lignes incidentes étant exclues du rang strict. Une perturbation
séquentielle n'est pas exacte. Une grande cosphère est quotientée par une
branche de plateau reçue, développée selon le contrat, ou refusée explicitement.

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
  `-> q3/q4 : front Jung coalescé + lentille aiguë factorisée
       -> center-cover persistant + enveloppe top-(smax-2), ledger fail-open
       -> q3 intrinsèque + niveaux q4 P/P, N/N, P/N ou shallow cutting certifiée
       -> owner génératif exact-once ou RLE SupportKey -> une géométrie/owner
       -> census I/U complet + identités always-inside, lemme shell--theta
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
4. Conserver les probes q4 mass-only comme falsificateurs, jamais comme sources.
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
