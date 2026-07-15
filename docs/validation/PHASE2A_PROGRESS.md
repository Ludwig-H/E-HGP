# Avancement de la Phase 2A — prédicats exacts CPU

> [!IMPORTANT]
> Ce document est un point d'avancement, pas une revue de fermeture. La Phase 2A et la porte G1 restent ouvertes; aucun résultat MorseHGP3D public n'est promu vers `exact` par les lots décrits ici.

## Contexte opérationnel

- Phase : 2A — laboratoire de prédicats exacts CPU.
- Porte d'entrée : satisfaite par la clôture de la Phase 1.
- Backend : `reference_cpu`.
- Profils : couche numérique commune, priorité d'intégration `hgp_reduced`; aucune preuve supplémentaire pour `full_pi0`.
- Mode : `certified`.
- Rôle du code : certifier des décisions locales et produire des témoins de replay; il ne propose pas de cellules, ne réduit pas Gamma et ne construit pas encore de hiérarchie.
- Statut de sortie : Phase 2A `in_progress`, porte de sortie non satisfaite, G1 ouverte, Phase 2B bloquée.

## Lots intégrés

| lot | preuve | contenu effectivement livré |
|---|---|---|
| socle rationnel | `daf3ff9` | entiers multiprécision, rationnels canoniques, coordonnées binary64 exactes, `ExactRational3`, `ExactLevel`, packaging CMake |
| premiers signes exacts | `82bb676` | comparaison de distances carrées, orientation 3D, décisions, compteurs, replay par bits et différentiel `Fraction` |
| forme affine de puissance | `08e432c` | moments exacts de labels, domaine de cardinalité 1 à 10, signe de `H_RQ`, témoin rationnel, replay v2 et flux batch |
| intervalles FP64 conservateurs | commit du présent lot | filtres distance/orientation 3D, repli multiprécision, désactivation par appel, différentiel activé/désactivé et préservation du FENV |

La forme `H_RQ` utilise la convention coût de `R` moins coût de `Q`; son signe négatif signifie que `R` est moins coûteux au témoin. Les API riches matérialisent encore leurs témoins multiprécision lorsque le signe est filtré. Le champ `certification_stage` identifie donc l'autorité du signe, pas le coût total d'un replay diagnostique.

## Matrice de couverture actuelle

| famille de décision exigée par 2A | exact multiprécision | filtre FP64 | expansion | replay indépendant | état |
|---|---:|---:|---:|---:|---|
| comparaison de distances à un témoin explicite | oui | oui | non | oui | partiel terminé |
| signe de `H_RQ` | oui | non | non | oui | référence exacte terminée |
| orientation 3D | oui | oui | non | oui | partiel terminé |
| orientation 2D dans un plan support | non | non | non | non | prochain noyau affine |
| intersection de trois plans | non | non | non | non | prochain noyau affine |
| appartenance d'un quatrième plan | non | non | non | non | prochain noyau affine |
| centres circonscrits de deux, trois ou quatre points | non | non | non | non | en attente du noyau affine |
| coordonnées homogènes du centre et rayon carré | non | non | non | non | en attente des centres |
| signes barycentriques et `relint` | non | non | non | non | en attente des centres |
| intérieur, frontière ou extérieur d'une sphère | non | non | non | non | en attente des centres |
| comparaison de rayons de miniball | non | non | non | non | en attente des niveaux homogènes |
| égalité de niveaux issus de supports distincts | infrastructure `ExactLevel` seulement | non | non | partiel | prédicat à terminer |

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

- GCC 13, build strict : CTest 5/5.
- Clang 18, build Release strict : CTest 5/5.
- GCC 13 avec ASan, UBSan et détection de fuites : CTest 5/5.
- Corpus court déterministe : 2 048 cas, hash `276619686350b9c4f900d856b657ce084466cfdea2c4e8711d5efc7e690a1d15`.
- Signes du corpus court : 1 002 négatifs, 892 positifs et 154 zéros.
- Distances du corpus court : 58 signes `fp64_filtered` et 966 fallbacks `cpu_multiprecision`.
- Orientations du corpus court : 55 signes `fp64_filtered` et 457 fallbacks `cpu_multiprecision`.
- Total filtrable du corpus court : 113 signes `fp64_filtered` et 1 423 fallbacks `cpu_multiprecision`.
- Les 512 cas `H_RQ` supplémentaires restent, par construction, `cpu_multiprecision`.
- Corpus additionnel local : 10 000 distances et 5 000 orientations, filtre activé puis désactivé, 1 246 certifications FP64, 13 754 fallbacks et zéro différence.
- Package CMake installé, wrapper installé sur une fixture filtrée et consommateur externe exerçant les chemins filtré et multiprécision : 1/1.
- Contrats : 21 définitions, 21 exemples de schéma et 5 fixtures validés; 58 tests contractuels réussis.
- Oracle exhaustif indépendant : 91 tests réussis; campagne CI bornée sur trois dimensions affines, trois cas audités et zéro échec.
- Documentation : 25 documents actifs validés; 5 références locales et 9 modules d'oracle indépendant validés.
- Registre et sécurité : 20 phases validées, scope actif validé, workflow GCP en lecture seule validé et 11 tests de garde GCP réussis.
- Limite de portabilité : MSVC, Apple et ARM ne sont pas exécutés localement; aucune conclusion n'est extrapolée à ces cibles.

## Séquence détaillée restante avant la porte 2A

### 2A.4 — noyau affine exact

1. Figer la convention d'un plan orienté et sa clé géométrique non orientée.
2. Introduire `ExactPlane3` avec coefficients homogènes canoniques, normalisation par PGCD et rejet d'une normale nulle.
3. Construire un plan depuis trois points affinement indépendants et depuis une forme `H_RQ`.
4. Évaluer exactement un plan sur `CertifiedPoint3` et `ExactRational3`.
5. Définir l'orientation 2D par rapport à la normale orientée du plan support; refuser tout point hors du support avant de toucher les compteurs.
6. Classer l'intersection de trois plans en `unique`, `empty` ou `affine_family` par rang exact.
7. Représenter toute intersection unique par `ExactRational3` à dénominateur positif.
8. Tester l'incidence et le côté d'un quatrième plan au point d'intersection.
9. Ajouter replay fermé, oracle `Fraction`, cas parallèles, confondus, incompatibles, non dyadiques, à un ULP et métamorphismes de permutation ou changement d'échelle.

### 2A.5 — centres et niveaux homogènes

1. Construire le centre de deux points sans division flottante.
2. Construire le centre circonscrit de trois points dans leur enveloppe affine.
3. Construire le centre circonscrit de quatre points par le noyau affine.
4. Classer les supports affinement dépendants avant toute décision.
5. Produire ensemble le centre homogène et le rayon carré `ExactLevel`.
6. Vérifier permutation, translations dyadiques représentables, permutations signées des axes et homothéties par puissances de deux.
7. Ajouter des fixtures obtuses, coplanaires, quasi cosphériques et à un ULP.

### 2A.6 — minimalité, barycentriques et sphères

1. Calculer les signes barycentriques exacts d'un centre dans son support.
2. Réduire canoniquement tout support sur la frontière à son support minimal.
3. Décider `relint` sans epsilon.
4. Classer chaque point comme strictement intérieur, sur la frontière ou extérieur à une sphère candidate.
5. Relier ces décisions au domaine `RelevantGP` sans transformer une dégénérescence non couverte en résultat exact.

### 2A.7 — ordre total des miniballs

1. Comparer deux rayons carrés homogènes par produit croisé exact.
2. Décider l'égalité de niveaux provenant de supports distincts.
3. Figer le tie-break canonique indépendant de l'ordre d'entrée et des threads.
4. Rejouer les lots de niveaux égaux et les supports réduits.

### 2A.8 — cascade adaptative complète

1. Étendre les filtres FP64 aux nouveaux signes seulement après disponibilité de leur référence exacte.
2. Ajouter les expansions adaptatives pour les cas que leurs bornes certifient.
3. Conserver `cpu_multiprecision` comme dernier étage portable et comme chemin désactivable indépendant.
4. Exiger exactement un compteur de certification par décision, même après plusieurs tentatives incertaines.
5. Falsifier chaque étage sous GCC et Clang, plusieurs optimisations, modes d'arrondi, sous-normaux et sanitizers.

### 2A.9 — campagne de fermeture

1. Ajouter un batch `decision-only` sans sérialisation systématique de témoins big-int.
2. Geler générateur, graines, domaines d'exposants et hash du corpus.
3. Atteindre au moins dix millions de signes pseudo-aléatoires contre la référence, comme exigé par la feuille de route.
4. Couvrir séparément cas bien conditionnés, annulations, sous-normaux, grands offsets, quasi-coplanarité, quasi-cosphéricité et égalités exactes.
5. Exécuter les métamorphismes de permutation, réflexion d'axes, translations dyadiques représentables et échelles exactes.
6. Minimiser automatiquement toute différence; toute contradiction devient une fixture permanente et met à jour le registre des preuves avant reprise.
7. Publier compteurs d'étages, zéros, fallbacks, inconnues restantes, versions de compilateurs et hashes.

### 2A.10 — revue de porte

1. Vérifier que toutes les familles de prédicats de 2A possèdent API, témoins, replay, tests positifs et négatifs.
2. Vérifier zéro signe erroné et `remaining_unknown=0` sur la campagne certifiée.
3. Vérifier le package installé et un consommateur externe.
4. Rejouer contrats, oracle indépendant, documentation, références et registre opérationnel.
5. Évaluer explicitement G1; un benchmark ou un accord moyen ne peut pas la fermer.
6. Mettre `docs/implementation_status.toml` à jour dans le même commit seulement si la porte de sortie est réellement satisfaite.

## Prochaine sous-porte

Le prochain lot est 2A.4, noyau affine exact, toujours sur `reference_cpu`, couche commune aux profils et mode `certified`. Il doit compléter la surface scientifique de référence avant d'étendre les optimisations flottantes. La Phase 2B ne s'ouvre pas; aucune commande CUDA ou GCP n'est autorisée par ce point d'avancement.

## GCP

GCP non utilisé. Aucune VM n'a été créée, démarrée, arrêtée ou modifiée.
