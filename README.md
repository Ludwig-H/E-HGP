# E-HGP — MorseHGP3D

MorseHGP3D calcule la hiérarchie 3D des amas discrets de forte densité K-NN sans matérialiser la mosaïque de Delaunay d'ordre supérieur. La priorité actuelle est étroite : énumérer exactement toutes les paires dont la boule diamétrale fermée contient au plus $K_{\max}+1$ points, avec leur rang et la liste complète des points contenus.

> [!IMPORTANT]
> État courant : Phase 15, porte d'entrée satisfaite, `backend=reference_cpu`, `profile=hgp_reduced`, `mode=budgeted`, `deployment_status=architecture_only`, `public_status=not_claimed`. Deux oracles CPU bornés et leurs différentiels sont intégrés. Le pipeline GPU résident n'est pas encore implémenté et aucun SLO n'est revendiqué.

## Voie privilégiée

| ordre | objet généré | élimination immédiate |
|---|---|---|
| paires | toutes les paires de rang fermé $2\leq R\leq K_{\max}+1$, payload fermé complet | cutoff exact Yao48, rapports de régions Morton/LBVH, arrêt dès que le rang dépasse $K_{\max}+1$ |
| triangles | frontière indépendante des supports minimaux de taille trois | dépendance affine, triangles droits ou obtus déjà ramenés à un support de taille deux |
| tétraèdres | candidats issus des triangles, puis frontière indépendante des supports minimaux de taille quatre | dépendance affine et centre circonscrit hors de l'intérieur strict |
| hiérarchie | lots de niveaux exacts, incidences utiles et réduction Morse sparse | aucune cellule, coface ou mosaïque globale |

Une seule passe paramétrée par $K_{\max}$ alimente tous les ordres $1\leq k\leq K_{\max}$ : une paire de rang fermé $R$ est routée vers le niveau $k=R-1$. Morton ordonne les données et les parcours; il ne sert jamais de certificat de voisinage. Dans chaque chambre Yao48 suffisamment remplie, le $K_{\max}$-ième témoin fournit un cutoff directionnel démontré. Une chambre sous-remplie ne prune pas.

Le chemin commun doit rester sur le GPU : construction Morton/LBVH, top-$K_{\max}$ par chambre, rapports de régions, émission exacte une fois, trichotomie de rang, `count/scan`, payload, tri, déduplication et découpage en chunks. Le CPU reçoit seulement une file rare de cas multiprécision et le transcript terminal; aucun callback par paire et aucun transfert de candidats par vague ne sont admis.

## Faits mathématiques qui structurent le code

- Une paire exacte $(u,v)$ et son saturé $S=X\cap B_{uv}$ ferment tous les sous-ensembles contenant $u$ et $v$ : leur miniboule est $B_{uv}$. Le record fournit donc immédiatement les candidats intérieurs supportés par cette paire.
- Cette fermeture ne suffit pas pour les triangles aigus. Une fixture rationnelle permanente donne un triangle aigu de rang trois dont les trois côtés ont rang quatre et qui n'est récupéré par aucun sous-graphe des paires de rang au plus trois.
- Un triangle non dégénéré droit ou obtus a un support minimal de taille deux. La frontière de taille trois ne garde donc que les triangles aigus.
- Un tétraèdre dont le centre circonscrit n'est pas strictement intérieur se réduit à un support de taille deux ou trois. La frontière indépendante de taille quatre ne garde que les tétraèdres bien centrés.
- En dimension trois, tout miniball possède un support minimal de cardinal au plus quatre; cette cascade est exhaustive une fois chacune des trois frontières fermée.

Le nombre de résultats peut lui-même être quadratique : à 50 000 points, il existe 1 249 975 000 paires non ordonnées. Le contrat de latence est donc sensible à la sortie et impose des caps explicites; aucune implémentation exacte ne peut promettre 100 ms sur une famille dont le résultat à matérialiser est trop grand.

## Objectifs de performance

La cible principale est un p95 `warm_e2e` strictement inférieur à 100 ms pour 50 000 points et $K_{\max}=10$ sur des familles enregistrées : nouveau nuage, index, calcul, validation et matérialisation bornée inclus. Le passage à l'échelle est vérifié séquentiellement à 1 M, 10 M puis 30 M de points, avec flux reprenable et caps de mémoire/sortie.

Le protocole G4 ne démarre qu'après l'existence d'un vrai noyau résident : différentiel borné, Compute Sanitizer, falsificateur de croissance à 12 500 points, puis trente nuages frais à 50 000 points. Les benchmarks utilisent uniquement les VM G4 `SPOT` et les coupe-circuits du dépôt.

## Lire le dépôt

1. Les Parties I et II du [manuscrit](docs/references/MANUSCRIT_THESE_HAUSEUX.pdf), pages PDF 35 à 134, définissent les clusters discrets visés.
2. La [spécification](docs/SPECIFICATION_MORSEHGP3D.md) fixe l'objet public et les profils de sortie.
3. Le [catalogue exact des paires diamétrales](docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md) donne le contrat, le théorème Yao48, l'oracle borné et l'architecture GPU attendue.
4. La [frontière des supports trois et quatre](docs/math/FRONTIERE_DIRECTE_SUPPORTS_3_4.md) formalise les triangles aigus puis les tétraèdres bien centrés.
5. Le [registre des preuves](docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md), la [roadmap](docs/ROADMAP_IMPLEMENTATION_MORSEHGP3D.md), le [plan de tests](docs/TEST_PLAN_MORSEHGP3D.md) et l'[état des phases](docs/implementation_status.toml) portent l'autorité opérationnelle.
6. Les [replis maintenus](docs/research/README.md) sont peu nombreux et explicitement bornés; les [pistes abandonnées](docs/archive/abandoned/README.md) sont séparées de la navigation active.

L'[index documentaire](docs/README.md) donne un parcours plus détaillé.

## Arborescence

```text
.
├── morsehgp3d/          # cœur C++20/CUDA et références bornées isolées
├── reference/           # oracle Python exhaustif de petite taille
├── docs/math/           # noyau mathématique actif
├── docs/research/       # replis secondaires maintenus
├── docs/archive/        # pistes abandonnées et rapports scellés
├── docs/validation/     # revues actives et artefacts bruts des checkers
├── tests/               # différentiels, fixtures et non-régressions
├── gcp-migration/       # démarrage/arrêt G4 SPOT avec doubles coupe-circuits
├── HGP-old/             # code historique du manuscrit, jamais dépendance produit
└── tools/               # contrôles documentaires, scientifiques et opérationnels
```

## Vérifications locales

```bash
cmake -S morsehgp3d -B build/morsehgp3d -DMORSEHGP3D_BUILD_TESTS=ON
cmake --build build/morsehgp3d --parallel
ctest --test-dir build/morsehgp3d --output-on-failure
python tools/check_docs.py
python tools/check_implementation_status.py
PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s tests/oracle -p 'test_*.py'
```

## Sécurité GCP

Toute session GPU passe par [`gcp-migration/start_and_verify.sh`](gcp-migration/start_and_verify.sh), sur une cible G4 `SPOT` étiquetée `project=e-hgp`, avec `instanceTerminationAction=STOP`, durée bornée et arrêt invité armé. La session se termine toujours par [`stop_and_verify.sh`](gcp-migration/stop_and_verify.sh) sur la cible exacte. Les règles normatives sont dans [`AGENTS.md`](AGENTS.md).

## Licences

La licence MIT couvre le code actif et la documentation du projet. Elle ne relicencie ni [`HGP-old/`](HGP-old/), qui conserve sa licence historique non commerciale, ni les PDF de [`docs/references/`](docs/references/), dont les conditions sont documentées fichier par fichier.
