# E-HGP — MorseHGP3D

E-HGP est désormais centré sur **MorseHGP3D**, le backend 3D destiné à reconstruire la hiérarchie des amas de forte densité K-NN pour tous les ordres $1\leq k\leq K_{\max}$, avec $K_{\max}=10$ comme première cible. Le dépôt fixe l'objet mathématique, conserve des oracles CPU exhaustifs bornés et développe maintenant la Phase 9 : un flux direct de supports critiques sensible à $H_0$, branch-and-bound sur LBVH et GPU-first. Les travaux cellulaires de Phase 8 restent des différentiels jusqu'à huit sites; ils ne sont plus le chemin produit.

L'objet continu est la tour de multicovertures

$$L_k(a)=\left\lbrace y\in\mathbb{R}^{3}:D_k(y)\leq a\right\rbrace,$$

où $D_k(y)$ est le carré de la distance au $k$-ième plus proche voisin. À chaque ordre, on suit les composantes connexes de $L_k(a)$ quand $a$ croît; entre deux ordres, on conserve les applications induites par $L_{k+1}(a)\subseteq L_k(a)$. La sortie est donc une **tour de forêts de fusion**, pas une partition plate.

> [!IMPORTANT]
> La voie exacte de référence bornée est : **catalogue exhaustif de sphères critiques, Gamma exhaustif, réduction hiérarchique par lots**. La voie produit est : **LBVH, branch-and-bound des supports deux à quatre, flux d'événements, journal Morse**. Le flot de Gabriel reste partiel tant que les incidences silencieuses ne sont pas restituées par une construction prouvée. Une grille, une mosaïque de Delaunay d'ordre supérieur, un atlas de cellules top-$m$ ou un rhomboid tiling ne font pas partie de l'algorithme cible.

## Décision mathématique

Une sphère critique de centre $c$, de rayon carré $a$ et de rang fermé $s$ intervient comme minimum à l'ordre $s$ si $1\leq s\leq10$, et comme événement d'indice un à l'ordre $s-1$ si $2\leq s\leq11$. Pour $K_{\max}=10$, seuls les rangs $s\leq11$ sont utiles. En dimension trois et en position générale, son support frontal contient au plus quatre observations.

Le verrou combinatoire est donc reformulé ainsi : énumérer exactement les sphères bien centrées de rang au plus onze sans énumérer les $\binom{n}{k}$ sous-ensembles et sans refermer tous les parents top-$m$. La primitive retenue parcourt directement les produits de deux, trois et quatre nœuds LBVH, élimine des régions par bornes certifiables et ne matérialise que les supports feuilles survivants. Le GPU propose et filtre en masse; le rejeu exact décide.

Le chemin hiérarchique exact de référence énumère chaque coface de Gamma : une coface de cardinal $k+1$ relie ses facettes de cardinal $k$ au niveau exact de sa miniball. Un hyper-Kruskal par lots reconstruit les composantes non triviales en conservant aussi les incidences silencieuses. Le sous-flot Gabriel est calculé séparément pour certifier une connectivité positive, mais le contre-exemple permanent à cinq points montre qu'il peut retarder une fusion Gamma. La reconstruction de toutes les composantes isolées et de leur généalogie reste un profil distinct, fondé sur le catalogue de Morse et des attaches certifiées.

## Deux profils de sortie honnêtes

| profil | objet promis | condition du statut `exact` |
|---|---|---|
| `hgp_reduced` | toutes les composantes à $k=1$, puis les K-polyèdres non triviaux pour $k\geq2$, comme ensembles d'observations éventuellement recouvrants | Gamma exhaustif sur `reference_cpu`, niveaux exacts, lots fermés, cartes verticales complètes et ancre EMST validée |
| `full_pi0` | toutes les composantes de $L_k(a)$, y compris les naissances isolées, avec morphismes verticaux | catalogue critique complet, attaches globales certifiées et lots fermés |

Le premier profil est défini par la réduction des composantes de Gamma exhaustif; le théorème du manuscrit qui l'identifiait au seul flot Gabriel est faux en général. Le second est la cible complète de MorseHGP3D; il ne recevra le statut `exact` qu'après validation de son théorème de reconstruction et de ses oracles d'attache. Un arrêt sur budget produit `conditional`, jamais un faux résultat exact.

## Ce qui n'est pas la régularisation recherchée

La DTM et les lissages entropiques restent utiles pour ordonner les candidats, initialiser l'ordre suivant ou construire un mode budgété. Ils ne réduisent pas, à eux seuls, l'atlas des ensembles top-$k$ et deviennent neutres au cas fondamental $k=1$. Ils ne définissent donc pas la hiérarchie exacte de MorseHGP3D.

La régularisation opérationnelle retenue est plutôt une **sparsification certifiée du calcul** : événements de rang borné, prunes rejouables, frontières explicites, traitement par lots et stockage en flux. Elle conserve exactement l'objet HGP lorsqu'elle termine.

## Objectifs GPU

La machine de référence est une VM GCP `g4-standard-48` avec une RTX PRO 6000 Blackwell Server Edition de 96 Go. Le cœur futur sera C++/CUDA, mono-GPU d'abord, avec :

- propositions géométriques en FP32;
- filtres FP64 et bornes d'erreur;
- expansions exactes sur GPU pour les cas ambigus;
- repli multiprécision CPU rare;
- données en structures de tableaux, tris radiaux, files compactes et traitement out-of-core des frontières, événements et attaches;
- plafond d'allocation à 80 % de la VRAM.

Le premier jalon CUDA de Phase 5 conserve cette séparation : `cuda_g4` ne propose que les cibles situées sous un cutoff de graine conservateur, en deux passes count/emit sans troncature; `reference_cpu` vérifie le sur-ensemble, recalcule les distances et résout les minima Borůvka. Sa compilation AOT `sm_120`, son replay matériel et `compute-sanitizer` ont été qualifiés sur la G4 gardée, sans promouvoir ce producteur de propositions en décision hiérarchique publique.

La cible principale $50\,000$ points, $K_{\max}=10$, avec un p95 strictement inférieur à 100 ms est un **objectif expérimental réfutable** en protocole `warm_e2e` sur nuages volumiques réguliers : runtime initialisé, mais validation, transfert, construction de l'index, calcul et matérialisation du résultat inclus. Un p95 strictement inférieur à une seconde sur le même payload est seulement l'objectif secondaire. Le nombre de paires peut être quadratique et leur classification naïve rendre le travail cubique; aucune garantie universelle de latence n'est annoncée. Pour dix millions de points ou davantage, le calcul devra diffuser frontières, événements et attaches vers des runs triés, reprendre après préemption et refuser proprement une certification devenue matériellement impossible. Toute baseline Voronoï plus large doit d'abord évaluer un adaptateur Geogram versionné ou une bibliothèque mature équivalente; cette voie reste un oracle de test et ne remplace ni le chemin GPU-first ni la recertification exacte.

## Documentation active

| document | rôle |
|---|---|
| [Spécification mathématique](docs/SPECIFICATION_MORSEHGP3D.md) | définition exacte de l'objet, profils de sortie et théorème cible |
| [Catalogue critique 3D](docs/math/CATALOGUE_CRITIQUE_3D.md) | caractérisation des événements, flux direct cible et oracle cellulaire borné |
| [Tour globale de boules saturées](docs/math/TOUR_BOULES_SATUREES.md) | audit, preuves exactes, limites de complexité et piste de référence alternative |
| [Définition HGP 3D](docs/math/DEFINITION_HGP_3D.md) | équivalence avec les K-polyèdres et réduction de Gabriel |
| [Attaches par miniball](docs/math/ATTACHES_DESCENTE_MINIBALL.md) | mécanisme facultatif pour `full_pi0` et contrôle croisé |
| [Preuves et heuristiques](docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md) | registre des garanties, hypothèses et verrous ouverts |
| [Architecture GPU G4](docs/GPU_G4_ARCHITECTURE.md) | données, noyaux, prédicats et stratégie mémoire |
| [Feuille de route](docs/ROADMAP_IMPLEMENTATION_MORSEHGP3D.md) | plan d'implémentation détaillé, artefacts et portes de validation |
| [État des phases](docs/implementation_status.toml) | registre opérationnel lisible par les futurs agents |
| [Contrats exécutables de phase 0](docs/contracts/README.md) | schéma v2 actif, archive v1, M.1, exemples et matrice de traçabilité |
| [Plan de tests](docs/TEST_PLAN_MORSEHGP3D.md) | oracles, familles de nuages, métriques et critères go/no-go |
| [Références](docs/references/README.md) | corpus relu, PDF locaux et licences |

L'[index documentaire](docs/README.md) donne l'ordre de lecture. Les décisions remplacées restent traçables dans l'[historique](docs/HISTORIQUE.md).

## Arborescence

```text
.
├── docs/                         # spécification scientifique active
│   ├── math/                    # définitions, propositions et statut des preuves
│   ├── contracts/               # contrats, exemples et traçabilité de phase 0
│   └── references/              # manuscrit et articles redistribuables
├── schemas/                     # contrat JSON canonique versionné
├── reference/                   # oracle CPU exhaustif indépendant de petite taille
├── morsehgp3d/                  # nouveau cœur C++20 certifié, indépendant de HGP-old
├── tests/oracle/                # fixtures, différentiels et campagnes de phase 1
├── HGP-old/                     # code historique lié au manuscrit, conservé tel quel
├── gcp-migration/               # VM G4, coupe-circuits et arrêt vérifié
├── tools/                       # contrôles documentaires et bibliographiques
├── .github/workflows/           # CI non facturable et inspection GCP en lecture seule
└── AGENTS.md                    # règles impératives pour les agents IA
```

## Vérifications locales

```bash
cmake -S morsehgp3d -B build/morsehgp3d -DMORSEHGP3D_BUILD_TESTS=ON
cmake --build build/morsehgp3d --parallel
ctest --test-dir build/morsehgp3d --output-on-failure
python tools/check_docs.py
python tools/check_contracts.py
python -m unittest discover -s tests/contracts -p 'test_*.py'
PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s tests/oracle -p 'test_*.py'
PYTHONDONTWRITEBYTECODE=1 python tools/run_oracle_campaign.py --ci
python tools/check_references.py
python tools/check_scope.py
python tools/check_implementation_status.py
python tools/check_gcp_workflows.py
python -m unittest discover -s tests/gcp -p 'test_*.py'
bash -n gcp-migration/*.sh
```

## Sécurité GCP

Toute session GPU doit être lancée par les scripts du dossier [`gcp-migration`](gcp-migration/), avec un coupe-circuit GCE borné et un arrêt invité déjà armé. Elle se termine obligatoirement par un arrêt dont l'état `TERMINATED` est vérifié. Les règles normatives figurent dans [`AGENTS.md`](AGENTS.md); aucun workflow GitHub ne doit créer ou démarrer une ressource facturable.

## Licences

La licence MIT à la racine couvre le code actif et la documentation produits par ce projet. Elle ne relicencie ni [`HGP-old/`](HGP-old/), qui conserve sa licence historique non commerciale, ni les PDF de [`docs/references/`](docs/references/), dont les droits et les conditions de redistribution sont indiqués fichier par fichier dans le manifeste bibliographique.

## État du projet

Les Phases 1, 2A, 2B, 3 et 4 sont fermées. L'[oracle CPU exhaustif](reference/README.md) de petite taille reste la vérité terrain différentielle indépendante, tandis que le [cœur C++20 certifié](morsehgp3d/README.md) fournit les dyadiques binary64, les prédicats filtrés, l'oracle spatial exact et le parcours LBVH qualifié sur G4. La Phase 9 est le chantier actif avec le backend `reference_cpu`, le profil `hgp_reduced` et le mode `certified` pour le flux direct de supports-paires. La Phase 5 reste une ancre $k=1$ ouverte à l'état `ready`; ses cinq premiers jalons CPU livrent l'EMST exact, le catalogue global des paires, la réduction rang-deux/Gabriel, un différentiel indépendant de 50 cas jusqu'à $n=14$, la forêt hiérarchique compacte à stockage linéaire et un producteur Borůvka exact vérifié sur le LBVH global. La boucle GPU historique à graine Morton et son témoin EMST local sont qualifiés sur G4. Sur hôte, le parcours self-dual partagé possède maintenant une pile certifiée $2H+1$, une borne de $n(n+1)-1$ visites et une chaîne qui offre chaque graine recertifiée aux deux composantes incidentes avant de décider directement les minima sous une enveloppe max reconstruite après chaque contraction, sans minima ponctuels; un contexte neuf la rejoue avant l'accord de l'ancre CPU. Sa qualification CUDA, son profil de travail et toute preuve sous-quadratique restent ouverts. Cette progression ne promeut aucun résultat MorseHGP3D public vers `exact`. Toute revendication future devra indiquer le profil, le statut de certification, $n$, $K_{\max}$, la famille de données, le matériel, le pic mémoire et le protocole de mesure.
