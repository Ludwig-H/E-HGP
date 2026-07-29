# E-HGP — MorseHGP3D

MorseHGP3D calcule la hiérarchie 3D des amas discrets de forte densité K-NN sans matérialiser la mosaïque de Delaunay d'ordre supérieur. La priorité actuelle est étroite : énumérer exactement toutes les paires dont la boule diamétrale fermée contient au plus $K_{\max}+1$ points, avec leur rang et la liste complète des points contenus, sans inspecter inconditionnellement l'univers quadratique.

> [!IMPORTANT]
> État courant : Phase 15, porte d'entrée satisfaite, `backend=reference_cpu`, `profile=hgp_reduced`, `mode=budgeted`, `deployment_status=architecture_only`, `public_status=not_claimed`. Deux oracles CPU bornés, leurs différentiels, le contrat hôte/fake du futur catalogue, la spécification exécutable de `morton_yao48_pair_frontier` et le prédicat ponctuel exact CUDA sont intégrés. Aucun kernel du catalogue résident ni SLO n'est encore revendiqué.

## Voie privilégiée

| ordre | objet généré | élimination immédiate |
|---|---|---|
| paires | toutes les paires de rang fermé $2\leq R\leq K_{\max}+1$, payload fermé complet | parcours Morton/LBVH fusionné avec banques Yao48; seules les feuilles survivantes sont classifiées exactement |
| triangles | frontière indépendante des supports minimaux de taille trois | dépendance affine, triangles droits ou obtus déjà ramenés à un support de taille deux |
| tétraèdres | frontière indépendante des supports minimaux de taille quatre; les fermetures de supports inférieurs sont fusionnées séparément | dépendance affine et centre circonscrit hors de l'intérieur strict |
| hiérarchie | lots de niveaux exacts, incidences utiles et réduction Morse sparse | aucune cellule, coface ou mosaïque globale |

Les réductions vers un support inférieur sont exhaustives dans la fenêtre certifiée sous `RelevantGP`. Hors de ce domaine, une grande cosphère peut porter un petit simplexe Gabriel; elle déclenche `unsupported_degeneracy` au lieu d'être omise silencieusement.

Une seule passe paramétrée par $K_{\max}$ construit les buckets de rang fermé $R\leq K_{\max}+1$. Ici `requested_order=K` signifie donc « rang fermé au plus $K+1$ »; si une API demande littéralement « au plus $K_{\mathrm{total}}$ points dans la boule », le nombre de témoins requis pour exclure une paire est $K_{\mathrm{total}}-1$. Un simplexe Gabriel de cardinal $q$ alimente le niveau $q-1$; sous `RelevantGP`, son shell supplémentaire utile est vide, donc $q=R$ et ce routage se réduit à $k=R-1$.

Morton fournit l'index, la localité et l'ownership exact une fois; il ne sert jamais de certificat de proximité. Le parcours LBVH proche-en-premier remplit à la volée 48 banques Yao certifiées par ancre active. Dès qu'une banque contient le nombre requis de témoins distincts, les bornes directionnelles éliminent des feuilles ou régions lointaines avec une masse certifiée. Une chambre sous-remplie reste fail-open. L'échec d'un cutoff signifie seulement « candidat » : les survivants forment un sur-ensemble exhaustif et sont les seuls à atteindre la classification exacte.

Le chemin commun doit rester sur le GPU : construction Morton/LBVH, banques par chambre en mémoire $O(B\mathbin{\cdot}48\mathbin{\cdot}K_{\max})$ pour une tuile de $B$ ancres, prunes certifiés, classification des seuls survivants, `count/scan`, payload, tri, déduplication et découpage en chunks. Il ne matérialise ni ne visite par principe toutes les paires et ne possède aucun fallback dense. Son reçu ferme la comptabilité `candidate_pair_mass + certified_pruned_pair_mass + unresolved_pair_mass = n(n-1)/2`; seul un résidu nul autorise une sortie exhaustive. Le CPU reçoit seulement la canonicalisation finale et les replis numériques qui ne sont pas encore qualifiés sur GPU; aucun callback par paire et aucun transfert de candidats par vague ne sont admis.

## Faits mathématiques qui structurent le code

- Une paire exacte $(u,v)$ et son saturé $S=X\cap B_{uv}$ ferment tous les sous-ensembles contenant $u$ et $v$ : leur miniboule est $B_{uv}$. Un tel sous-ensemble est Gabriel exactement lorsqu'il contient tous les points strictement intérieurs; les points supplémentaires du shell sont optionnels.
- Cette fermeture ne suffit pas pour les triangles aigus. Une fixture rationnelle permanente donne un triangle aigu de rang trois dont les trois côtés ont rang quatre et qui n'est récupéré par aucun sous-graphe des paires de rang au plus trois.
- Un triangle non dégénéré droit ou obtus a un support minimal de taille deux. La frontière de taille trois ne garde donc que les triangles aigus.
- Un tétraèdre dont le centre circonscrit n'est pas strictement intérieur se réduit à un support de taille deux ou trois. La frontière indépendante de taille quatre ne garde que les tétraèdres bien centrés.
- L'acuité des quatre faces ne filtre pas cette frontière : un tétraèdre bien centré peut avoir des faces obtuses, et quatre faces aiguës n'impliquent pas un centre intérieur. Une fixture rationnelle permanente recertifie les deux directions.
- En dimension trois, tout miniball possède un support minimal de cardinal au plus quatre; cette cascade est exhaustive une fois chacune des trois frontières fermée.

Le nombre de résultats, et donc le pire cas de travail, peut lui-même être quadratique : à 50 000 points, il existe 1 249 975 000 paires non ordonnées. Le chemin produit doit être adaptatif et sous-quadratique sur les profils favorables qualifiés, mais ne revendique aucune borne universelle incompatible avec cette sortie. Le contrat de latence est sensible au profil et à la sortie; les caps explicites censurent tout cas défavorable par `budget_exhausted`, sans basculer vers un scan dense.

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
