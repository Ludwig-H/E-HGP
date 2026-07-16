# Avancement de la Phase 2A — prédicats exacts CPU

> [!IMPORTANT]
> Ce document est le journal historique détaillé de l'implémentation. La [revue de fermeture](PHASE2A_GATE_REVIEW.md) a depuis fermé la Phase 2A avec `G1=go`; aucun résultat MorseHGP3D public n'est pour autant promu vers `exact`.

## Contexte opérationnel

- Phase documentée : 2A — laboratoire de prédicats exacts CPU, désormais terminée.
- Porte d'entrée : satisfaite par la clôture de la Phase 1.
- Backend : `reference_cpu`.
- Profils : couche numérique commune, priorité d'intégration `hgp_reduced`; aucune preuve supplémentaire pour `full_pi0`.
- Mode : `certified`.
- Rôle du code : certifier des décisions locales et produire des constructions ou témoins exacts de replay; il ne propose pas de cellules, ne réduit pas Gamma et ne construit pas encore de hiérarchie.
- Statut de sortie : Phase 2A `completed`, porte de sortie satisfaite et `G1=go`; Phase 3 `in_progress`, Phase 2B bloquée par la seule Phase 3.

## Lots intégrés

| lot | preuve | contenu effectivement livré |
|---|---|---|
| socle rationnel | `daf3ff9` | entiers multiprécision, rationnels canoniques, coordonnées binary64 exactes, `ExactRational3`, `ExactLevel`, packaging CMake |
| premiers signes exacts | `82bb676` | comparaison de distances carrées, orientation 3D, décisions, compteurs, replay par bits et différentiel `Fraction` |
| forme affine de puissance | `08e432c` | moments exacts de labels, domaine de cardinalité 1 à 10, signe de `H_RQ`, témoin rationnel, replay v2 et flux batch |
| intervalles FP64 conservateurs | `3d67d3c` | filtres distance/orientation 3D, repli multiprécision, désactivation par appel, différentiel activé/désactivé et préservation du FENV |
| noyau affine exact 2A.4 | `ed5bd7e` | `ExactPlane3`, forme `H_RQ` à échelle exacte, orientation 2D dans un support, rang et intersection de trois plans, incidence déterminantale d'un quatrième plan, replay v3 et oracle `Fraction` |
| centres et niveaux homogènes 2A.5 | `3f3fa2b` | centres exacts de supports de taille deux à quatre, dimension affine, `ExactRational3` et `ExactLevel` atomiques, replay v4 et oracle Gram/RREF indépendant |
| minimalité locale, barycentriques et sphères 2A.6 | `39f7902` | singleton exact, signes barycentriques de supports indépendants, réduction certifiée des supports de frontière, côté exact d'une sphère, replay v5 et oracle RREF indépendant |
| ordre exact et lots canoniques 2A.7 | `1718e08` | comparaison instrumentée des `ExactLevel`, égalité par produit croisé, supports minimaux et provenances canoniques, lots exacts, replay v6 et oracle `Fraction` indépendant |
| cascade adaptative 2A.8a | `c98a5a1` | expansions flottantes exactes fermées, cascades distance/orientation 3D/`H_RQ` binary64, provenance canonique des labels, politiques indépendantes, replay et différentiel `Fraction` forçant les trois étages |
| supports et niveaux adaptatifs 2A.8b | `6651600` | provenance dyadique positionnelle et réduite, polynômes homogènes Gram/Cramer, barycentriques, côté de sphère et ordre de niveaux adaptatifs, replay v7 et oracles `Fraction` |
| formes et plans adaptatifs 2A.8c | `ffabbdc` | provenance dyadique canonique de trois origines, polynômes homogènes affines, orientation 2D, côté de plan, rangs et intersections, incidence d'un quatrième plan, replay v8 et oracles `Fraction` |
| batch décisionnel 2A.9(1) | `287b8b1` | `--decision-only --batch` pour distance, orientation 3D et `H_RQ`; sortie minimale sans témoin bigint sérialisé, replays riches v1 à v8 inchangés et différentiel `Fraction` fermé |
| checkpoint des niveaux 2A.9(2) | `0bab2dd` | runs natifs bornés et checksummés, manifeste interne v1, reprise après chaque run et niveau entièrement fermé, merge global des égalités inter-chunks et résultat adressé par contenu |
| infrastructure de campagne 2A.9(3) | `92df6fc`, `eebd859`, `4c64d0c` | générateur indexé v2 de 10 800 000 bases strictement non nulles dans la strate bien conditionnée, oracle dyadique entier indépendant, cinq métamorphismes exacts, chunks natifs reprenables, racines ordonnées, provenance Git/build et minimisation durable |
| campagne longue 2A.9(3) | `phase2a_predicates_v1.result.json` | 1 228 chunks, 10 800 000 bases, 1 080 000 dérivés et 11 880 000 décisions natives; certificat SHA-256 `57627bdb68662234ea1d1716d06e8c8abbb4cc828b2353fdf763cde64df54ad8`, zéro mauvais signe et zéro inconnue |
| revue de porte 2A.10 | `PHASE2A_GATE_REVIEW.md` | matrice T1 complète, campagne certifiée, qualification GCC/Clang/sanitizers, package, consommateur, contrats, oracle et décision explicite `G1=go` |

La forme `H_RQ` utilise la convention coût de `R` moins coût de `Q`; son signe négatif signifie que `R` est moins coûteux au témoin. `ExactAffineForm3` conserve ses quatre coefficients rationnels et leur échelle exacte; sa clé primitive ne sert qu'à classifier le plan orienté ou la forme constante. Les API riches matérialisent encore leurs témoins multiprécision lorsque le signe est filtré. Le champ `certification_stage` identifie donc l'autorité du signe, pas le coût total d'un replay diagnostique.

## Matrice de couverture actuelle

| famille de décision exigée par 2A | exact multiprécision | filtre FP64 | expansion | replay indépendant | état |
|---|---:|---:|---:|---:|---|
| comparaison de distances à un témoin explicite | oui | oui | oui | oui | cascade 2A.8a terminée |
| signe de `H_RQ` | oui | oui si provenance binary64 | oui si provenance binary64 | oui | cascade 2A.8a conditionnelle à la provenance |
| orientation 3D | oui | oui | oui | oui | cascade 2A.8a terminée |
| orientation 2D dans un plan support | oui | oui si provenance du plan | oui si provenance du plan | oui | cascade 2A.8c conditionnelle à la provenance |
| côté d'un plan affine | oui | oui si provenance du plan | oui si provenance du plan | oui | cascade 2A.8c conditionnelle à la provenance |
| intersection de trois plans | oui | oui si trois provenances et rang plein | oui si trois provenances | oui | cascade 2A.8c, rangs exacts à l'expansion |
| appartenance d'un quatrième plan | oui | oui si quatre provenances | oui si quatre provenances | oui | cascade 2A.8c sous précondition d'intersection unique |
| centres circonscrits d'un à quatre points | oui | non | non | oui | référence exacte terminée |
| coordonnées homogènes du centre et rayon carré | oui | non | non | oui | référence exacte terminée |
| signes barycentriques et `relint` | oui | oui si support binary64 | oui si support binary64 | oui | cascade 2A.8b conditionnelle à la provenance |
| intérieur, frontière ou extérieur d'une sphère | oui | oui si support binary64 | oui si support binary64 | oui | cascade 2A.8b sur sphère circonscrite |
| comparaison de rayons de miniball | oui | oui si émissions géométriques | oui si émissions géométriques | oui | cascade 2A.8b après analyse et réduction |
| égalité de niveaux issus de supports distincts | oui | oui si supports binary64 | oui si supports binary64 | oui | cascade 2A.8b conditionnelle à la provenance |

## Invariants de la cascade adaptative

Le filtre d'intervalles n'émet qu'un signe strict. Une borne qui contient zéro retourne `uncertain`. L'étage d'expansion suivant représente exactement une somme finie de composantes binary64 et peut donc certifier un zéro exact. Les additions, soustractions, produits et carrés du premier étage sont calculés en binary64 strict puis élargis par `nextafter` vers les deux infinis. Toute borne non finie, tout overflow ou toute incohérence invalide l'intervalle.

La certification flottante exige simultanément :

- IEEE binary64 avec 53 bits de significande;
- `FE_TONEAREST`, y compris les bits de contrôle d'arrondi MXCSR sur x86;
- contraction FMA et réassociation interdites par la cible CMake;
- sous-normaux préservés;
- FTZ et DAZ désactivés;
- sauvegarde puis restauration de l'environnement et des drapeaux flottants de l'appelant.

Les expansions emploient les transformations sans erreur `TwoSum` et `TwoProduct` avec résidu FMA. Avant tout produit, l'exposant du bit exact le plus faible est calculé depuis les mots binary64; un bit sous `2^-1074` rend immédiatement la tentative incertaine. Toute exception `FE_INVALID`, `FE_DIVBYZERO`, `FE_OVERFLOW` ou `FE_UNDERFLOW`, toute composante non finie et tout échec de restauration invalident également la tentative. Cette règle ferme notamment `denorm_min * denorm_min`, que nulle somme de binary64 ne peut représenter. Les plafonds de 4 096 composantes et 16 384 couples de produit bornent le travail diagnostique; les atteindre invalide l'expansion et replie vers la multiprécision sans aucune troncature.

`PredicateFilterPolicy::allow_adaptive` active FP64, expansion puis multiprécision. `allow_fp64` conserve l'ancien chemin FP64 puis multiprécision et ses valeurs d'énumération historiques; `multiprecision_only` désactive les deux étages rapides sans état global. Le différentiel rejoue exactement le même corpus adaptatif en multiprécision seule et exige des signes et témoins scientifiques identiques.

Les supports binary64 sont triés par ordre total après canonicalisation du zéro signé uniquement pour évaluer les polynômes; les signes barycentriques sont ensuite remappés aux positions d'entrée. `CircumcenterSupportAnalysis` conserve le support source positionnel et `SupportLevelEmission` seulement le support réduit. Ces provenances et l'étage collectif sont exclus de l'égalité scientifique; une entrée rationnelle arbitraire laisse la provenance vide et rend les étages rapides inapplicables.

Les formes affines et plans conservent de même une recette binary64 vérifiée issue de quatre coefficients explicites, de trois points orientés ou de deux multisets définissant un bisecteur de puissance. Les zéros signés sont canonicalisés; points et multisets sont triés avec un multiplicateur d'orientation séparé. La recette reconstruite doit donner exactement les coefficients rationnels de l'objet. Elle est exclue de l'égalité scientifique, des clés géométriques et du record fermé `ExactPlane3`; relire un record exact ne fabrique donc aucune provenance flottante.

Les numérateurs barycentriques, la puissance signée de la sphère et le produit croisé des rayons sont évalués sous forme Gram/Cramer homogène. Aucun quotient, centre ou rayon flottant n'est matérialisé. Le déterminant de Gram doit être strictement positif au même étage. Le résultat riche reconstruit toujours le témoin rationnel canonique et refuse toute contradiction avec un signe rapide. Les surcharges tableau décrivent une sphère circonscrite; la sémantique miniball exige la chaîne analyse, réduction exacte, puis émission géométrique.

Pour trois plans, le filtre d'intervalles ne certifie que le déterminant des normales strictement non nul. L'expansion examine exactement les mineurs normaux et augmentés et certifie aussi les systèmes singuliers cohérents ou incohérents. L'incidence d'un quatrième plan combine séparément le signe du déterminant liant et celui du déterminant homogène, sans quotient ni produit d'expansions. Les API décisionnelles ne matérialisent le déterminant multiprécision qu'après échec des étages rapides; les API riches reconstruisent le témoin rationnel pour le diagnostic et ferment toute contradiction.

## Régression numérique permanente

La première sonde DAZ était insuffisante et a laissé apparaître une fausse certification d'orientation. La fixture `morsehgp3d/tests/fixtures/predicates/orientation_daz_regression.json` encode le quadruplet minimal dont le déterminant exact est $2^{-75}>0$. Sous DAZ, l'ancien filtre produisait un signe négatif.

La correction inspecte MXCSR sur x86 et les bits d'une opération sous-normale sur les autres cibles. Sur x86 avec SSE2, les tests activent séparément DAZ, FTZ et les deux, exigent un filtre `uncertain`, un fallback `cpu_multiprecision`, le signe positif exact et la restitution du mode MXCSR initial. Les autres cibles exercent la sonde générique sans prétendre modifier un registre matériel non portable. Cette contradiction est aussi inscrite dans le registre des preuves et ne peut plus être supprimée comme simple cas de performance.

## Qualification du lot courant

- GCC 13, build Release strict : CTest 24/24.
- Clang 18, build Release strict : CTest historique 22/22; différentiels décisionnel et checkpoint ajoutés 1/1 chacun.
- GCC 13 Debug avec ASan et UBSan : CTest historique 22/22; différentiels décisionnel et checkpoint ajoutés 1/1 chacun.
- Corpus court déterministe `mixed-binary64-splitmix64-v2` : 2 048 cas, hash `066f9ee577fc6c6d64ed930df4eaeca88e96895fa801a661980ba22fa98b4be3`.
- Signes du corpus court : 1 001 négatifs, 891 positifs et 156 zéros.
- Distances du corpus court : 58 signes `fp64_filtered`, 3 `expansion` et 963 fallbacks `cpu_multiprecision`.
- Orientations du corpus court : 55 signes `fp64_filtered`, 3 `expansion` et 454 fallbacks `cpu_multiprecision`.
- Les deux familles historiques totalisent 113 signes `fp64_filtered`, 6 `expansion` et 1 417 fallbacks `cpu_multiprecision`.
- Les 512 cas `H_RQ` donnent 13 signes `fp64_filtered`, 3 `expansion` et 496 fallbacks `cpu_multiprecision`; les témoins non représentables exactement en binary64 restent sur la surcharge rationnelle.
- Total du corpus court : 126 signes `fp64_filtered`, 9 `expansion`, 1 913 `cpu_multiprecision`, zéro différence scientifique avec le replay multiprécision seul et `remaining_unknown=0`.
- Tests directs de l'expansion : 4 096 tirages arithmétiques contre `ExactRational`, puis 2 048 distances, 2 048 orientations 3D et 1 024 signes `H_RQ`; annulations, signes stricts, underflow, overflow, quatre modes d'arrondi et FTZ/DAZ sont couverts.
- Corpus additionnel local v2 : 10 000 distances et 5 000 orientations, hash `3686e4d7872e879671d575ee0ad256afbef238097b41d257359db8e5034f58d7`, cascade activée puis multiprécision seule, 1 245 certifications FP64, 6 expansions, 13 749 fallbacks et zéro différence.
- Replay affine v3 : 17 fixtures, 5 familles, 4 classes de forme affine, 4 cas d'intersection, 3 signes d'incidence, 12 métamorphismes, 7 rejets d'entrée, 5 faux témoins natifs et 1 diagnostic natif inattendu rejetés.
- Corpus affine `affine-dyadic-splitmix64-v1` : 1 760 cas, graine `0x414646494e453356`, hash des commandes `1dc9bce8051ba64ddc67d7943122190be36bed884fb35a9a9a6f78e111418457`, hash des réponses exactes de l'oracle `47929b8b1fed34e3ee38a77da13bf3d994b66962fb3b35e773ce49e0a785a0a9`, sorties normale et `--multiprecision-only` byte-identiques.
- Répartition affine : 240 constructions de plan, 240 formes `H_RQ`, 336 orientations 2D, 608 intersections et 336 incidences de quatrième plan.
- Classes `H_RQ` du corpus affine : 96 plans propres, 48 constantes négatives, 48 constantes positives et 48 formes identiquement nulles; cardinalités de labels 1 à 4 couvertes.
- Intersections du corpus affine : 160 uniques, 160 vides et 288 familles affines; permutations, inversions d'orientation, translations dyadiques et changements d'échelle exacts couverts. Les fixtures v3 ajoutent le rang vide `(1,2)`, un support à un ULP, les sous-normaux minimaux et la valeur binary64 finie maximale.
- Replay centres v4 : 13 fixtures, tailles deux à quatre, dépendances de dimensions zéro à deux, 64 permutations exhaustives, 18 métamorphismes, 10 rejets d'entrée, 8 faux témoins natifs, 1 diagnostic natif inattendu et 3 sentinelles historiques v1 à v3 vérifiés. Les frontières à un ULP sont exercées depuis zéro sous-normal et autour des valeurs normales `1.0` et `2.0`.
- Corpus centres `center-dyadic-splitmix64-v1` : 1 728 cas, graine `0x43454e5445523356`, hash des commandes `7277882dcdf66ba798087c19284283600821d56a4e5b60c723558a73224ea0cf`, hash des réponses exactes de l'oracle `3dfdd2b9727b5db7b7eccd1110bbdd1367deb0abaaf713c9c1b7e09c720d214f`, sorties normale et `--multiprecision-only` byte-identiques.
- Répartition centres : 1 152 supports indépendants et 576 dépendants; 416 paires, 576 triangles et 736 tétraèdres; dimensions affines zéro à trois toutes couvertes.
- Replay supports et sphères v5 : 19 fixtures, dont 12 supports et 7 sphères, 4 statuts de support, 3 classes d'enveloppe, 4 tailles réduites, 3 côtés de sphère, 131 permutations exhaustives, 57 métamorphismes, 20 rejets d'entrée, 22 faux témoins natifs, 1 sortie non canonique, 1 diagnostic natif inattendu et 4 sentinelles historiques v1 à v4 vérifiés; centre et niveau à dénominateur non trivial, niveau nul et sorties normale et `--multiprecision-only` byte-identiques couverts.
- Corpus supports et sphères `support-sphere-dyadic-splitmix64-v2` : 2 128 cas, graine `0x535550504f525435`, hash des commandes `0d5f30c7067c253b6e3c2cc36a530fad9a3f83e526b7f35fe23543ad1508a68c`, hash des réponses exactes de l'oracle `7d10f2cb87398847abcfae505c1365072d48786212b25159d8154f92d65f88d9`, sorties normale et `--multiprecision-only` byte-identiques. Le corpus contient 137 multisets barycentriques distincts, dont 128 géométries dyadiques non obtenues par simple similarité, 48 centres rationnels distincts et 136 niveaux distincts.
- Répartition supports 2A.6 : 1 408 analyses, dont 512 `minimal`, 288 `boundary_reduced`, 320 `exterior_circumcenter` et 288 `affinely_dependent`; tailles un à quatre et dimensions affines zéro à trois couvertes. Les 720 côtés de sphère donnent 216 intérieurs stricts, 240 frontières et 264 extérieurs; les cas de rayon nul restent isolés dans leurs familles dédiées.
- Replay ordre v6 : 11 fixtures, 3 signes, supports de tailles un à quatre, 10 lots, 17 émissions uniques et 1 doublon; 5 symétries de comparaison, 159 permutations d'items, 6 permutations internes d'identifiants, 2 homothéties exactes, 34 rejets d'entrée, 10 commandes natives invalides, 30 faux natifs, 1 sortie non canonique, 1 diagnostic natif inattendu et 5 sentinelles historiques v1 à v5 vérifiés. Un niveau exact encodé comme chaîne de 5 000 chiffres décimaux est accepté par le wrapper et le natif, tandis qu'un entier numérique JSON de même taille est refusé avant conversion. `PointId=2^53-1` et le lot maximal de 64 émissions sont acceptés nativement; `PointId=2^53` est refusé. Au-delà de cinq items, les permutations du harnais sont bornées à huit ordres déterministes, tous exécutés de bout en bout sur un lot de 64 supports distincts; les sorties normale et `--multiprecision-only` sont byte-identiques.
- Corpus d'ordre `level-order-dyadic-splitmix64-v1` : 1 792 cas, graine `0x4c4556454c324137`, hash des commandes `59fc92204073609b622d86eea14f92efcb704e6bcba3daba7eb6c9dbf53a7e8b`, hash des réponses exactes de l'oracle `3cb912b811f649d8bf96be79047234b7f5a1a59e6ac846be3e1efc8bd512bbec`, sorties normale et `--multiprecision-only` byte-identiques.
- Répartition ordre 2A.7 : 1 536 comparaisons, dont 640 négatives, 640 positives et 256 égalités exactes; six familles de 256 cas couvrent produits croisés `-1`, `1`, égalités, ULP, exposants extrêmes et rationnels génériques. Les 256 cas de chaînes, issus de 32 chaînes de base rejouées sous huit ordonnancements, totalisent 5 120 émissions, 1 536 occurrences de niveaux de lots, 768 émissions dupliquées, des supports minimaux et sources de tailles un à quatre, jusqu'à trois supports par niveau et trois provenances par support.
- Replay adaptatif v7 : 30 fixtures, dont 6 barycentriques, 8 analyses, 8 côtés de sphère et 8 comparaisons de niveaux. Chaque famille couvre les signes stricts au filtre FP64, les signes négatif, nul et positif à l'expansion et les trois signes au fallback multiprécision; une analyse dépendante n'émet aucune décision. Le manifeste vaut `9c4a399b7cc81da3274407e38c84a53041a1e335a34f32d741deb13bcb3fb8fc`; 10 entrées fermées invalides, dont un objet JSON à clé dupliquée, 7 commandes natives invalides et un résultat natif scientifiquement falsifié sont rejetés, et 6 sentinelles conservent les identités v1 à v6.
- Corpus adaptatif supports/niveaux `support-adaptive-fraction-splitmix64-v1` : 4 050 cas, graine `0x3241384253555050`, hash des commandes `229c59e99a1d7d2404ec90cd8d30e430a0650f4972d8ce07f4dc5b2f0eb6019f` et hash des réponses de l'oracle `3f4094fe7768dcf14e3ad81d3e96ff529eb38cb0e8f1daceeb15a56c5b6d0ca1`. Les sorties adaptatives et multiprécision ont des projections scientifiques identiques; 3 589 cas terminent au filtre FP64, 447 à l'expansion, 11 en multiprécision et 3 analyses dépendantes sans signe.
- Couverture adaptative 2A.8b : 31 cas construits forcent toutes les classes disponibles, 528 décisions exactes nulles sont observées et 288 relations métamorphiques vérifient permutations de support et transformations orthogonales signées. Les quatre tailles de support sont couvertes, chaque décision terminale est comptée exactement une fois et `remaining_unknown=0` partout.
- Replay adaptatif affine v8 : 37 fixtures, dont 9 côtés de plan, 9 orientations 2D, 11 intersections et 8 incidences. Chaque famille couvre les signes stricts au filtre FP64, les signes négatif, nul et positif à l'expansion et les trois signes au fallback multiprécision avec provenance binary64; les intersections singulières couvrent rangs `(1,1)`, `(1,2)`, `(2,2)` et `(2,3)`. Le manifeste vaut `20649770c099e9c4fca6837af0ea86af0b8e7399d82c786e4ed532941727cf49`; 21 entrées fermées, 15 commandes natives et 6 sorties scientifiques falsifiées, dont des booléens substitués aux compteurs ou à la dimension, sont rejetées. Sept sentinelles conservent les identités v1 à v7.
- Corpus affine adaptatif `affine-adaptive-fraction-rref-splitmix64-v2` : 5 120 cas, graine `0x3241384341464649`, hash des commandes `63d22e4daf8417de218b98101b49fcedf501c0557eaff7a2b251a17c07bed5eb` et hash des réponses de l'oracle `03dfcacbd777b2debaebd802a86f356fa7e34948ba04ef43e3d748bbdc2fabdc`. Les sorties adaptatives et multiprécision ont des projections scientifiques identiques; 3 350 décisions terminent au filtre FP64, 484 à l'expansion et 1 286 en multiprécision, avec 645 zéros exacts et `remaining_unknown=0` partout.
- Couverture adaptative 2A.8c : 1 024 côtés de plan, 1 372 orientations, 1 363 intersections et 1 361 incidences; 13 cas à provenance binary64 complète forcent la chute expansion vers multiprécision. Les 1 363 intersections comprennent 1 258 points uniques, 53 familles et 52 systèmes vides, avec tous les couples de rang possibles pour trois plans propres. Un système mixte avec une source exacte sans provenance conserve une intersection de dénominateur trois et une incidence signée de même dénominateur sous autorité multiprécision. Les quatre origines, 20 sources obliques permanentes, un bisecteur de puissance à deux points, 48 groupes et 120 relations métamorphiques sont couverts.
- Audit affine additionnel : GCC et Clang stricts, modes d'arrondi, FTZ/DAZ et restauration du FENV passent; 20 000 systèmes aléatoires, 10 000 orientations et 10 000 configurations de bisecteurs de puissance ont été recoupés, complétés par 2 923 incidences et 7 927 classifications de rang rationnelles indépendantes. Aucune conversion de `BigInt` vers `double` n'est présente.
- Batch décisionnel 2A.9(1) : 14 cas ciblés couvrant les trois signes dans chacune des familles distance, orientation 3D et `H_RQ` passent contre l'oracle `Fraction`. La cascade adaptative termine 5 décisions au filtre FP64, 5 à l'expansion et 4 en multiprécision; le rejeu `multiprecision_only` termine les 14 en multiprécision avec les mêmes signes. Les trois sentinelles riches sont identiques entre invocation individuelle et batch, sept entrées ou usages invalides — dont une erreur sur la seconde ligne — échouent sans publier de préfixe, aucun champ témoin n'apparaît et `remaining_unknown=0` partout.
- Checkpoint des niveaux 2A.9(2) : corpus `level-checkpoint-fraction-v1` de 31 émissions, 26 triplets support–source uniques, 5 doublons et 9 niveaux. Le hash JSONL vaut `dfc0497be5151f77073afb5956c0d113c6f7ffcad667a65959f751444e8930f6` et le hash de l'oracle `Fraction` vaut `3ca27bc45f134bd7c295ccfa6327de0cf60948cbd324cf2495ddc7d7d16245dc`; l'oracle et le natif monolithique sont byte-identiques. Les tailles de chunk 31, 16, 11, 5 et 1 forcent exactement 1, 2, 3, 7 et 31 runs. Quatre-vingt-quinze invocations reprennent après chaque run, chaque niveau fermé et la finalisation; le niveau un traverse les frontières avec 11 émissions et 5 supports, tandis que deux niveaux voisins de produit croisé `-1` restent distincts. Quatorze rejets fermés couvrent schéma, version, configuration, entrée, runs, contradiction globale et cinq coupures injectées; bootstrap à zéro transaction, manifeste ancien autoritaire, résultat content-addressé et alias final sont vérifiés.
- Infrastructure de campagne 2A.9(3) : le smoke rejoue 900 bases et 90 dérivés avec deux découpages, couvre les 24 cellules prédicat–strate, les cardinalités `H_RQ` de 1 à 10, dix motifs de recouvrement et les dénominateurs 1, 2, 3, 5, 7 et 8. Six cents cas recoupent l'oracle dyadique avec `Fraction`; les cinq métamorphismes ont des compteurs non nuls. Le contre-audit du premier pré-calcul a révélé à l'indice 32 un `H_RQ` structurellement nul, car le générateur v1 autorisait `R=Q` sous l'étiquette `well_conditioned`; aucune racine n'a été publiée. Le générateur v2 impose désormais `R` distinct de `Q`, rejette par arithmétique entière toute nullité fortuite sous une borne gelée et conserve le cas dans `campaign_well_conditioned_zero_regression.json`. Le smoke vérifie 558 cas bien conditionnés stricts, 600 membres de la famille `60k+32`, un resampling réel pour chacun des trois prédicats, dix inversions métamorphiques non vacues, deux réflexions nulles classées comme signe conservé et un seul appel de l'oracle par signe. Sept rejets fermés, quatre failpoints transactionnels et une divergence native injectée vérifient respectivement provenance/corruption/verrou, coupures chunk–manifeste–certificat–alias et publication de l'original avant réduction. Il termine avec zéro mauvais signe et `remaining_unknown=0`; il ne constitue pas la campagne longue.
- Campagne longue 2A.9(3) : le commit `4c64d0c` gèle la racine des commandes `76544b0c85c2f6602e560bd007762609e1feb080f5fe13a71ec4d0d3f31f294b` et celle de l'oracle `62a4f14bc4d25971d67d7c321bdea9091d45f7a17dc37d79a292fd34cd078210`. Les 10 044 000 bases pseudo-aléatoires `well_conditioned` dépassent seules le seuil de dix millions; 756 000 bases adversariales et 1 080 000 dérivés restent comptés séparément. Les 11 880 000 décisions se répartissent en 9 192 000 au filtre FP64, 696 000 à l'expansion et 1 992 000 en multiprécision; les trois étages, les trois signes, les 24 cellules prédicat–strate et les cinq métamorphismes sont non nuls. Les 216 000 zéros proviennent de la strate d'égalités exactes. La campagne termine en 1 228 chunks avec `wrong_sign=0`, `remaining_unknown=0`, la racine native `729f06551fa4012e7ff3a97b871ba81faa9524c99c900c8616b7ed87c0de9079` et le certificat SHA-256 `57627bdb68662234ea1d1716d06e8c8abbb4cc828b2353fdf763cde64df54ad8`. Une reprise `--max-chunks 0` et une lecture indépendante des 1 228 artefacts confirment le résultat sans nouvel appel natif.
- Package CMake installé, binaire installé qualifié sur les 14 décisions 2A.9(1) et les 95 reprises 2A.9(2), et wrapper installé rejoué sur une intersection rationnelle v3, un centre non dyadique v4, une réduction de frontière v5, une sphère à dénominateurs non triviaux v5, un niveau nul v5, deux niveaux v6 séparés par un produit croisé `-1`, une provenance réduite v6, une égalité de niveaux v7 certifiée par expansion et les quatre familles adaptatives v8. Le consommateur externe exerce filtres, multiprécision, noyau affine, provenance de coefficients et de trois points, côté de plan, intersection certifiée, incidence adaptative, centre tétraédrique, réduction de support, côté de sphère adaptatif, ordre de niveaux avec provenance et lots canoniques : 1/1.
- Contrats : 21 définitions, 21 exemples de schéma et 5 fixtures validés; 58 tests contractuels réussis.
- Oracle exhaustif indépendant : 92 tests réussis; campagne CI bornée sur trois dimensions affines, trois cas audités et zéro échec.
- Documentation : 27 documents actifs validés; 5 références locales et 9 modules d'oracle indépendant validés.
- Registre et sécurité : 20 phases validées, scope actif validé, workflow GCP en lecture seule validé et 11 tests de garde GCP réussis.
- Limite de portabilité : MSVC, Apple et ARM ne sont pas exécutés localement; aucune conclusion n'est extrapolée à ces cibles.

## Séquence détaillée avant la porte 2A

### 2A.4 — noyau affine exact — terminé

1. Convention du plan orienté et clé géométrique non orientée figées.
2. `ExactPlane3` homogène primitif, normalisé par PGCD positif, avec rejet d'une normale nulle et record fermé `2.0.0` livré.
3. Construction depuis trois points rationnels ou dyadiques affinement indépendants et classification sûre de `H_RQ` livrées.
4. Évaluation exacte sur `CertifiedPoint3` et `ExactRational3` livrée.
5. Orientation 2D par normale orientée livrée; tout point hors support est rejeté avant les compteurs.
6. Intersection classée en `unique`, `empty` ou `affine_family` par rang normal et augmenté exacts; dimension affine explicite livrée.
7. Toute intersection unique est un `ExactRational3` canonique à dénominateur positif et est revérifiée sur les trois plans liants.
8. Côté rationnel riche et signe déterminantal direct d'un quatrième plan livrés, avec contradiction interne fermée.
9. Replay v3 fermé, oracle `Fraction` par élimination de Gauss, cas parallèles, confondus, incompatibles, non dyadiques, sous-normaux, à un ULP et métamorphismes livrés.

### 2A.5 — centres et niveaux homogènes — terminé

1. Centre d'une paire distincte construit comme milieu rationnel exact, sans division flottante.
2. Centre d'un triangle indépendant construit dans son plan affine par deux médiateurs exacts.
3. Centre d'un tétraèdre indépendant construit par trois médiateurs et le noyau affine exact.
4. Dimension affine zéro à trois calculée avant toute construction; tout support dépendant expose deux témoins `null`.
5. Centre homogène `ExactRational3` et rayon carré `ExactLevel` produits ensemble puis revérifiés sur toutes les distances du support.
6. Permutations exhaustives des triangles et tétraèdres, translations dyadiques représentables, permutations signées des axes et homothéties par puissances de deux vérifiées.
7. Fixtures obtuses, collinéaires, coplanaires cosphériques, sous-normales, maximales finies et à un ULP livrées. La classification d'un cinquième point quasi cosphérique reste explicitement le prédicat de sphère de 2A.6.

### 2A.6 — minimalité locale, barycentriques et sphères — terminé

1. Base singleton matérialisée : centre égal au point, dimension affine zéro et `ExactLevel` nul, y compris aux extrêmes binary64 finis.
2. Coordonnées et signes barycentriques exacts calculés pour tout support affinement indépendant de taille un à quatre; somme un et reconstruction affine sont revérifiées avant certification.
3. Tout centre sur la frontière relative est réduit aux indices de coefficients strictement positifs; le centre, le niveau et l'intérieur relatif du support réduit sont ensuite revérifiés exactement.
4. `relative_interior`, `relative_boundary` et `exterior` sont décidés sans epsilon, avec une décision `cpu_multiprecision` par coefficient.
5. Un point est classé `strictly_inside`, `boundary` ou `outside` par le signe exact de sa distance carrée moins le niveau de la sphère, avec témoin rationnel canonique.
6. Les supports dépendants et les centres circonscrits extérieurs n'inventent aucun support minimal. Le résultat reste une décision locale : il ne renseigne pas `relevant_gp_complete`, ne constitue pas une énumération de miniball et ne promeut aucun statut public.

### 2A.7 — ordre total des miniballs — terminé

1. `ExactLevel::operator<=>`, la décision seule et le résultat riche partagent le même produit croisé entier; le résultat riche expose son témoin et enregistre exactement une certification multiprécision explicite.
2. L'égalité est exclusivement le produit croisé nul; les niveaux voisins dont le produit croisé vaut `-1` ou `1` restent distincts même avec des entiers de plusieurs milliers de bits.
3. Les identifiants de support couvrent tout le domaine `PointId` JSON exact jusqu'à `2^53-1`, sont triés et validés uniques après application du masque positionnel de réduction 2A.6; tout doublon est refusé.
4. Le tie-break local est `(niveau exact, cardinalité du support minimal, identifiants croissants)`. Le centre demeure un témoin de l'analyse, pas une clé supplémentaire susceptible de masquer un support associé à deux niveaux contradictoires.
5. Les supports sources distincts réduits vers le même support minimal sont conservés comme provenances triées; les émissions identiques sont comptées séparément de toute multiplicité géométrique ou de Morse.
6. Les lots sont groupés uniquement par égalité exacte, restent invariants par permutation d'arrivée et préservent les supports minimaux distincts d'un même niveau.
7. L'API refuse les supports vides, trop grands, dupliqués ou hors domaine, les provenances qui n'incluent pas leur support minimal, les analyses extérieures ou dépendantes et un même support minimal associé à deux niveaux.
8. Le replay v6 et son oracle `Fraction` réexercent comparaison, groupement, ordre par cardinalité, réduction de provenance, duplication, quasi-égalité, gros entiers et compatibilité des identifiants v1 à v5.

### 2A.8a — première cascade adaptative — terminé

1. Une expansion flottante zéro-éliminée ordonnée du composant le moins significatif au plus significatif implémente somme, différence et produit exacts par `TwoSum` et résidu FMA.
2. Les produits dont le bit exact le plus faible serait sous `2^-1074`, les opérations non finies, les exceptions d'underflow ou overflow et les environnements non conformes retournent `uncertain`; aucun bit n'est tronqué silencieusement.
3. Distance, orientation 3D et `H_RQ` sur données binary64 suivent désormais FP64, expansion puis multiprécision. L'expansion peut certifier les zéros, contrairement au filtre d'intervalles.
4. `ExactLabelMoments` conserve une provenance binary64 triée par ordre total des coordonnées sans l'inclure dans l'égalité sémantique des moments; des provenances distinctes donnant les mêmes moments restent égales et le signe exact reste inchangé.
5. `allow_adaptive`, `allow_fp64` et `multiprecision_only` gardent trois chemins indépendants. Une décision enregistre exactement une certification terminale, jamais ses tentatives incertaines.
6. Le replay reconnaît sans arrondi un témoin rationnel exactement représentable en binary64; le corpus `Fraction` v2 force les signes négatif, nul et positif à l'étage expansion pour chacune des trois familles et rejoue les mêmes commandes en multiprécision seule.
7. À ce stade historique, la Phase 2A restait ouverte : ce sous-lot ne prétendait pas filtrer des plans ou niveaux rationnels arbitraires en convertissant leurs `BigInt` vers `double`.

### 2A.8b — provenance dyadique des supports et niveaux — terminé

1. `CircumcenterSupportAnalysis` conserve le support binary64 dans l'ordre positionnel et `SupportLevelEmission` seulement le support minimal après réduction; les entrées rationnelles et les émissions structurelles gardent une provenance vide.
2. Les supports sont canonicalisés par ordre total avec normalisation de zéro signé pour l'évaluation, puis les signes barycentriques sont remappés aux positions d'origine. Provenance et étage collectif restent exclus de l'égalité scientifique.
3. Les numérateurs barycentriques d'une requête et du centre circonscrit sont évalués par déterminants de Gram et règle de Cramer homogènes. Le déterminant doit être strictement positif au même étage et aucun quotient flottant n'est matérialisé.
4. Le côté de la sphère et l'ordre de deux niveaux utilisent des polynômes homogènes équivalents à la puissance signée et au produit croisé exact des rayons. Le résultat riche conserve le centre, le niveau, les coordonnées ou le produit croisé rationnels comme témoins diagnostiques.
5. Les surcharges tableau portent explicitement sur les sphères circonscrites de supports indépendants. La sémantique de miniball local passe par l'analyse, la réduction exacte puis l'émission géométrique; un centre extérieur ou un support dépendant n'est jamais promu.
6. `allow_adaptive`, `allow_fp64` et `multiprecision_only` restent indépendants. Les rationnels ou `BigInt` arbitraires restent sous autorité `cpu_multiprecision`; une provenance absente désactive les étages rapides sans approximation.
7. Le replay v7, 30 fixtures permanentes et le corpus de 4 050 cas forcent les classes de signe disponibles aux trois étages dans les quatre familles, comparent les témoins au mode multiprécision seul et enregistrent exactement une certification terminale par décision.
8. Aucune contradiction mathématique n'a été observée. À ce stade historique, la Phase 2A, 2A.8 dans son ensemble et G1 restaient ouvertes; ce sous-lot ne changeait aucun statut public.

### 2A.8c — provenance dyadique des formes et plans — terminé

1. `Binary64AffineProvenance` conserve une recette canonique vérifiée pour des coefficients binary64 explicites, un plan par trois points ou un bisecteur de puissance par deux multisets de même cardinalité. Les zéros signés, permutations et orientations opposées ont des clés stables.
2. `ExactAffineForm3` et `ExactPlane3` propagent la recette lorsqu'elle est complète et vérifient ses quatre coefficients rationnels. Provenance et étage restent exclus de l'identité scientifique, de la clé orientée, de la clé non orientée et du record fermé `2.0.0`.
3. L'orientation 2D et le côté d'un plan évaluent directement la recette et les points binary64. Incidence exacte au support et autres préconditions sont validées avant les compteurs; aucune conversion de `BigInt` vers `double` n'existe.
4. Le rang de trois plans est certifié par déterminant strict au filtre FP64, puis par déterminant et mineurs exacts à l'expansion. Les classes `(3,3)`, `(2,3)`, `(2,2)`, `(1,2)` et `(1,1)` sont couvertes; le témoin riche exact reste `unique`, `empty` ou `affine_family`.
5. L'incidence d'un quatrième plan exige exactement une intersection unique des trois premiers, puis combine le signe du déterminant normal et celui du déterminant homogène sans division. La permutation canonique des plans liants stabilise l'étage et le signe diagnostique; inversion d'un plan ne change pas l'objet intersection scientifique.
6. Les API historiques conservent leurs signatures de pointeurs de fonction. Les variantes avec politique rendent `allow_adaptive`, `allow_fp64` et `multiprecision_only` indépendants; les tests vérifient notamment que `allow_fp64` saute l'expansion et replie exactement sur les zéros d'orientation, de côté, de rang et d'incidence. Une décision terminale est comptée exactement une fois et les tentatives incertaines ne le sont jamais.
7. Le replay v8 et ses oracles `Fraction` fermés reconstruisent les recettes, évaluent le côté d'un plan, utilisent RREF pour les rangs et évaluent directement le quatrième plan. Les quatre familles forcent tous les signes disponibles aux trois étages, y compris un vrai repli expansion vers multiprécision avec provenance binary64, et rejettent les booléens à la place de rangs, dimensions ou compteurs entiers. Les systèmes mixtes sans provenance complète replient aussi vers un témoin rationnel non entier.
8. La qualification 2A.8 couvre GCC, Clang, options flottantes strictes, modes d'arrondi, FTZ/DAZ, sous-normaux, ASan, UBSan, package installé et consommateur externe. Aucune contradiction mathématique n'a été observée. À ce stade historique, la Phase 2A et G1 restaient ouvertes; ce sous-lot ne changeait aucun statut public.

### 2A.9 — campagne de fermeture

1. **Livré et qualifié par `287b8b1` :** batch `--decision-only --batch` limité à la comparaison de distances, à l'orientation 3D et au signe de `H_RQ`. Il émet seulement `certification_stage`, `counters`, `predicate` et `sign`, sans sérialiser les niveaux, déterminants ou témoins rationnels multiprécision; les replays riches v1 à v8 restent inchangés. L'invocation transactionnelle conserve sa sortie compacte en mémoire et reste donc limitée à un chunk borné.
2. **Livré et qualifié par `0bab2dd` :** coordinateur POSIX interne `level_run_checkpoint.py`, chunks de 1 à 64 émissions certifiés par le natif, runs et artefacts checksummés, verrou mono-écrivain, manifeste v1 fermé distinct du contrat public, checkpoint après chaque run puis chaque niveau exact entièrement fusionné, et résultat content-addressé avant l'alias final. Le merge est interdit avant gel de tous les runs; un même support porté à deux niveaux est rejeté globalement. Les cinq coupures injectées reprennent depuis l'ancien manifeste sans niveau partiel ni alias prématuré.
3. **Livré par `eebd859` et `4c64d0c` :** le générateur v2, la graine, la politique bornée de resampling, les strates, le découpage, les dénominateurs, le catalogue métamorphique et les deux racines attendues sont gelés. Un premier pré-calcul a été interrompu sans artefact après découverte du cas minimal v1 `R=Q`; sa fixture permanente et le registre des preuves ont été mis à jour avant reprise.
4. **Satisfait :** 10 044 000 bases pseudo-aléatoires bien conditionnées ont été décidées contre l'oracle indépendant, sans compter les dérivés ni les 756 000 bases adversariales.
5. **Satisfait :** chaque prédicat couvre séparément les huit strates; chacune des 24 cellules possède 72 000 décisions adversariales ou 3 456 000 décisions bien conditionnées après ajout des dérivés.
6. **Satisfait :** permutation signée propre, réflexion impropre, translation dyadique exactement représentable, homothétie par puissance de deux et symétrie propre au prédicat totalisent 1 080 000 dérivés vérifiés.
7. **Satisfait pour la campagne :** la contradiction de stratification du générateur v1 est une fixture permanente; aucune divergence native réelle n'a demandé de nouvelle fixture. L'injection du smoke conserve la preuve que l'original est publié avant réduction automatique.
8. **Satisfait :** le certificat versionné porte compteurs, racines, commit, hashes, plateforme, Python, cache CMake, compilateur, type de build et `flags.make` effectif. Il garde `repository_phase_exit_claimed=false`; seule la revue 2A.10 peut fermer la phase.

La qualification de 2A.9(1) rejoue un corpus court par le batch décisionnel adaptatif, le replay riche et le batch décisionnel `multiprecision_only`, puis les compare à l'oracle `Fraction`. Elle vérifie les trois signes, les trois étages disponibles, l'exactitude des compteurs, `remaining_unknown=0`, le schéma JSON fermé sans témoins riches, la compatibilité des options et l'échec fermé sur entrée incomplète. Ce corpus ciblé qualifie l'interface; il ne remplace ni le gel du générateur long, ni la campagne d'au moins dix millions de signes.

La qualification de 2A.9(2) distingue strictement les objets locaux des types publics v2 : seul `ExactLevel` v2 est imbriqué. Le manifeste interne porte `public_contract_claimed=false` et ne prétend être ni `CheckpointManifest` ni `EqualLevelBatch`. Le coordinateur revalide hashes, tranches d'entrée, catalogues, curseurs et résultats à chaque reprise. Sa complexité de validation et sa dépendance à `flock` et au `fsync` de répertoire en font une preuve transactionnelle locale Linux/POSIX, pas la preuve de streaming budgété attendue en Phase 15. Aucune contradiction mathématique n'a été observée.

### 2A.10 — revue de porte

1. Vérifier que toutes les familles de prédicats de 2A possèdent API, témoins, replay, tests positifs et négatifs.
2. Vérifier zéro signe erroné et `remaining_unknown=0` sur la campagne certifiée.
3. Vérifier le package installé et un consommateur externe.
4. Rejouer contrats, oracle indépendant, documentation, références et registre opérationnel.
5. Évaluer explicitement G1; un benchmark ou un accord moyen ne peut pas la fermer.
6. Mettre `docs/implementation_status.toml` à jour dans le même commit seulement si la porte de sortie est réellement satisfaite.

Ces six contrôles sont satisfaits par la revue de fermeture. La décision explicite est `G1=go`; le registre est modifié dans le même commit que cette revue.

## Prochaine sous-porte

La Phase 2A et G1 sont fermées. La Phase 3 est la phase active pour l'environnement CUDA G4 reproductible; sa fermeture est la prochaine porte avant la Phase 2B. La sous-phase 17A est parallèlement `ready` comme oracle CPU borné de recherche, sans activation d'un backend public. Ce journal n'autorise aucune création ou mise en route de VM GCP.

## GCP

GCP non utilisé. Aucune VM n'a été créée, démarrée, arrêtée ou modifiée.
