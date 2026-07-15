# Avancement de la Phase 2A — prédicats exacts CPU

> [!IMPORTANT]
> Ce document est un point d'avancement, pas une revue de fermeture. La Phase 2A et la porte G1 restent ouvertes; aucun résultat MorseHGP3D public n'est promu vers `exact` par les lots décrits ici.

## Contexte opérationnel

- Phase : 2A — laboratoire de prédicats exacts CPU.
- Porte d'entrée : satisfaite par la clôture de la Phase 1.
- Backend : `reference_cpu`.
- Profils : couche numérique commune, priorité d'intégration `hgp_reduced`; aucune preuve supplémentaire pour `full_pi0`.
- Mode : `certified`.
- Rôle du code : certifier des décisions locales et produire des constructions ou témoins exacts de replay; il ne propose pas de cellules, ne réduit pas Gamma et ne construit pas encore de hiérarchie.
- Statut de sortie : Phase 2A `in_progress`, porte de sortie non satisfaite, G1 ouverte, Phase 2B bloquée.

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
| ordre exact et lots canoniques 2A.7 | présent lot | comparaison instrumentée des `ExactLevel`, égalité par produit croisé, supports minimaux et provenances canoniques, lots exacts, replay v6 et oracle `Fraction` indépendant |

La forme `H_RQ` utilise la convention coût de `R` moins coût de `Q`; son signe négatif signifie que `R` est moins coûteux au témoin. `ExactAffineForm3` conserve ses quatre coefficients rationnels et leur échelle exacte; sa clé primitive ne sert qu'à classifier le plan orienté ou la forme constante. Les API riches matérialisent encore leurs témoins multiprécision lorsque le signe est filtré. Le champ `certification_stage` identifie donc l'autorité du signe, pas le coût total d'un replay diagnostique.

## Matrice de couverture actuelle

| famille de décision exigée par 2A | exact multiprécision | filtre FP64 | expansion | replay indépendant | état |
|---|---:|---:|---:|---:|---|
| comparaison de distances à un témoin explicite | oui | oui | non | oui | partiel terminé |
| signe de `H_RQ` | oui | non | non | oui | référence exacte terminée |
| orientation 3D | oui | oui | non | oui | partiel terminé |
| orientation 2D dans un plan support | oui | non | non | oui | référence exacte terminée |
| intersection de trois plans | oui | non | non | oui | référence exacte terminée |
| appartenance d'un quatrième plan | oui | non | non | oui | référence exacte terminée |
| centres circonscrits d'un à quatre points | oui | non | non | oui | référence exacte terminée |
| coordonnées homogènes du centre et rayon carré | oui | non | non | oui | référence exacte terminée |
| signes barycentriques et `relint` | oui | non | non | oui | référence exacte terminée |
| intérieur, frontière ou extérieur d'une sphère | oui | non | non | oui | référence exacte terminée |
| comparaison de rayons de miniball | oui | non | non | oui | référence exacte terminée |
| égalité de niveaux issus de supports distincts | oui | non | non | oui | référence exacte terminée |

## Invariants du filtre FP64

Le filtre n'émet qu'un signe strict. Une borne qui contient zéro retourne `uncertain`; seul le fallback exact peut certifier zéro. Les additions, soustractions, produits et carrés sont calculés en binary64 strict puis élargis par `nextafter` vers les deux infinis. Toute borne non finie, tout overflow ou toute incohérence invalide l'intervalle.

La certification flottante exige simultanément :

- IEEE binary64 avec 53 bits de significande;
- `FE_TONEAREST`, y compris les bits de contrôle d'arrondi MXCSR sur x86;
- contraction FMA et réassociation interdites par la cible CMake;
- sous-normaux préservés;
- FTZ et DAZ désactivés;
- sauvegarde puis restauration de l'environnement et des drapeaux flottants de l'appelant.

`PredicateFilterPolicy::multiprecision_only` désactive le filtre sans état global. Le différentiel rejoue exactement le même corpus avec et sans filtre, compare signe et témoins scientifiques, et ignore uniquement l'étage et ses compteurs attendus.

## Régression numérique permanente

La première sonde DAZ était insuffisante et a laissé apparaître une fausse certification d'orientation. La fixture `morsehgp3d/tests/fixtures/predicates/orientation_daz_regression.json` encode le quadruplet minimal dont le déterminant exact est $2^{-75}>0$. Sous DAZ, l'ancien filtre produisait un signe négatif.

La correction inspecte MXCSR sur x86 et les bits d'une opération sous-normale sur les autres cibles. Sur x86 avec SSE2, les tests activent séparément DAZ, FTZ et les deux, exigent un filtre `uncertain`, un fallback `cpu_multiprecision`, le signe positif exact et la restitution du mode MXCSR initial. Les autres cibles exercent la sonde générique sans prétendre modifier un registre matériel non portable. Cette contradiction est aussi inscrite dans le registre des preuves et ne peut plus être supprimée comme simple cas de performance.

## Qualification du lot courant

- GCC 13, build strict : CTest 17/17.
- Clang 18, build Release strict : CTest 17/17.
- GCC 13 avec ASan et UBSan : CTest 17/17.
- Corpus court déterministe : 2 048 cas, hash `276619686350b9c4f900d856b657ce084466cfdea2c4e8711d5efc7e690a1d15`.
- Signes du corpus court : 1 002 négatifs, 892 positifs et 154 zéros.
- Distances du corpus court : 58 signes `fp64_filtered` et 966 fallbacks `cpu_multiprecision`.
- Orientations du corpus court : 55 signes `fp64_filtered` et 457 fallbacks `cpu_multiprecision`.
- Total filtrable du corpus court : 113 signes `fp64_filtered` et 1 423 fallbacks `cpu_multiprecision`.
- Les 512 cas `H_RQ` supplémentaires restent, par construction, `cpu_multiprecision`.
- Corpus additionnel local : 10 000 distances et 5 000 orientations, filtre activé puis désactivé, 1 246 certifications FP64, 13 754 fallbacks et zéro différence.
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
- Package CMake installé, wrapper installé rejoué sur une intersection rationnelle v3, un centre non dyadique v4, une réduction de frontière v5, une sphère à dénominateurs non triviaux v5, un niveau nul v5, deux niveaux v6 séparés par un produit croisé `-1` et une provenance réduite v6. Le consommateur externe exerce filtres, multiprécision, noyau affine, centre tétraédrique, réduction de support, côté de sphère, ordre exact et lots canoniques : 1/1.
- Contrats : 21 définitions, 21 exemples de schéma et 5 fixtures validés; 58 tests contractuels réussis.
- Oracle exhaustif indépendant : 91 tests réussis; campagne CI bornée sur trois dimensions affines, trois cas audités et zéro échec.
- Documentation : 25 documents actifs validés; 5 références locales et 9 modules d'oracle indépendant validés.
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

### 2A.8 — cascade adaptative complète

1. Étendre les filtres FP64 aux nouveaux signes seulement après disponibilité de leur référence exacte.
2. Ajouter les expansions adaptatives pour les cas que leurs bornes certifient.
3. Conserver `cpu_multiprecision` comme dernier étage portable et comme chemin désactivable indépendant.
4. Exiger exactement un compteur de certification par décision, même après plusieurs tentatives incertaines.
5. Falsifier chaque étage sous GCC et Clang, plusieurs optimisations, modes d'arrondi, sous-normaux et sanitizers.

### 2A.9 — campagne de fermeture

1. Ajouter un batch `decision-only` sans sérialisation systématique de témoins big-int.
2. Ajouter un checkpoint local fermé pour les runs triés, puis vérifier la reprise à chaque coupure et la fusion d'un niveau égal traversant plusieurs chunks; les permutations stateless de 2A.7 ne sont pas présentées comme cette preuve transactionnelle.
3. Geler générateur, graines, domaines d'exposants et hash du corpus.
4. Atteindre au moins dix millions de signes pseudo-aléatoires contre la référence, comme exigé par la feuille de route.
5. Couvrir séparément cas bien conditionnés, annulations, sous-normaux, grands offsets, quasi-coplanarité, quasi-cosphéricité et égalités exactes.
6. Exécuter les métamorphismes de permutation, réflexion d'axes, translations dyadiques représentables et échelles exactes.
7. Minimiser automatiquement toute différence; toute contradiction devient une fixture permanente et met à jour le registre des preuves avant reprise.
8. Publier compteurs d'étages, zéros, fallbacks, inconnues restantes, versions de compilateurs et hashes.

### 2A.10 — revue de porte

1. Vérifier que toutes les familles de prédicats de 2A possèdent API, témoins, replay, tests positifs et négatifs.
2. Vérifier zéro signe erroné et `remaining_unknown=0` sur la campagne certifiée.
3. Vérifier le package installé et un consommateur externe.
4. Rejouer contrats, oracle indépendant, documentation, références et registre opérationnel.
5. Évaluer explicitement G1; un benchmark ou un accord moyen ne peut pas la fermer.
6. Mettre `docs/implementation_status.toml` à jour dans le même commit seulement si la porte de sortie est réellement satisfaite.

## Prochaine sous-porte

Le prochain lot est 2A.8, cascade adaptative complète, toujours sur `reference_cpu`, couche commune aux profils et mode `certified`. Il doit étendre les filtres FP64 aux nouveaux signes dont la référence exacte est désormais disponible, ajouter les expansions de signe puis conserver le fallback multiprécision désactivable comme autorité terminale. Chaque décision doit rester attribuée à un unique étage, avec résultat canonique inchangé lorsque les étages rapides sont désactivés. La Phase 2B ne s'ouvre pas; aucune commande CUDA ou GCP n'est autorisée par ce point d'avancement.

## GCP

GCP non utilisé. Aucune VM n'a été créée, démarrée, arrêtée ou modifiée.
