# Feuille de route d'implémentation de MorseHGP3D

> **Mission.** Construire un backend 3D hiérarchique, certifiable et GPU-first pour $1\leq k\leq10$, beaucoup plus léger que `HGP-old` : la hiérarchie utile doit être obtenue sans matérialiser, ni reconstituer sous forme de cellules top-$m$, la mosaïque de Delaunay d'ordre $K$. Deux régimes produit sont non négociables : passage complet d'environ 50 000 points à $K_{\max}\leq10$ avec un p95 `warm_e2e` strictement inférieur à 100 ms sur les familles favorables enregistrées, l'objectif strictement inférieur à une seconde n'étant que secondaire; puis streaming transactionnel, reprenable et honnêtement budgeté à dix millions de points ou davantage. Cette feuille de route est écrite comme un protocole exécutable par de futurs agents ChatGPT.

## Cap produit courant — un seul chemin

Le seul chemin produit actif est le pipeline sparse exact `exact_sparse_frontier`. Sa première porte transversale est une passe résidente unique à $K_{\max}$ qui énumère les paires de rang fermé utile et leur payload complet au moyen d'une frontière fusionnée : Morton fournit l'index et l'ownership, les banques Yao48 certifient les prunes de régions et le classifieur exact ne reçoit que les survivants. Elle ne matérialise ni ne visite inconditionnellement toutes les paires et n'a aucun fallback dense. Elle est suivie d'une frontière indépendante des triangles aigus, puis des tétraèdres bien centrés; un merge canonique forme les lots de niveau et le reducer sparse produit le journal Morse. À $k=1$, ce même flux exact de paires est réduit directement en temps de fusion : le chemin produit ne construit pas l'EMST, qui reste seulement un oracle hors ligne de comparaison. Aucune Delaunay ordinaire ou d'ordre supérieur n'entre dans le chemin industriel. Aucun sélecteur de « variante produit » n'est admis.

Les anciens surrogates, `two_edge`, `closed_star`, `square_clique`, `link_face_fan`, `one_edge`, Gamma exhaustif et le sidecar Geogram/PDEL sont gelés comme oracles bornés hors ligne. Geogram reste l'unique autorité Delaunay de ces diagnostics; aucune triangulation n'est recodée. La fenêtre Morton seule reste une heuristique historique, jamais une autorité de voisinage. Ils ne peuvent être ni une dépendance, ni un fallback, ni une correction, ni une option d'exécution du produit. Dès que leurs conclusions et fixtures permanentes sont scellées, un jalon de nettoyage conserve seulement les petits oracles, checkers et rapports nécessaires à la non-régression, puis retire leurs cibles, configurations et benchmarks du build produit. Les sections historiques ci-dessous documentent les décisions passées; elles ne rouvrent aucune architecture alternative.

## 1. Règles de conduite pour tout agent

Avant toute modification, l'agent doit lire :

1. [`AGENTS.md`](../AGENTS.md), notamment les règles Git et GCP;
2. les parties I et II du [manuscrit de thèse](references/MANUSCRIT_THESE_HAUSEUX.pdf), pages PDF 35 à 134;
3. la [spécification](SPECIFICATION_MORSEHGP3D.md);
4. le [registre des preuves](math/STATUT_PREUVES_ET_HEURISTIQUES.md);
5. le [registre d'implémentation](implementation_status.toml), qui désigne la phase réellement ouverte;
6. la phase courante de cette feuille de route;
7. les tests et artefacts produits par les phases antérieures.

À chaque intervention, il doit :

- annoncer la phase et le profil affecté;
- indiquer si son code propose, certifie ou réduit;
- ne travailler que sur les artefacts de la phase;
- ajouter les tests avant de changer un statut scientifique;
- conserver un chemin CPU indépendant pour les différentiels;
- refuser toute troncature silencieuse;
- exécuter les validations proportionnées au risque;
- publier les compteurs, versions et hypothèses;
- publier le nombre de labels, cellules, cofaces et incidences évités aussi explicitement que le nombre d'événements produits;
- refuser dans le chemin produit toute arène dimensionnée par $\binom{n}{k}$, toute fermeture de tous les parents top-$m$ et tout Gamma global;
- faire un commit cohérent sur la branche courante, sans créer de branche sans accord explicite;
- ne jamais laisser une VM GCP active au moment du compte rendu final.

Un agent ne doit pas commencer une phase dont la porte d'entrée n'est pas satisfaite. S'il découvre une contradiction mathématique, il suspend l'optimisation, ajoute un cas minimal à l'oracle et met à jour le registre des preuves.

`implementation_status.toml` est la source opérationnelle de vérité : toute ouverture ou fermeture de phase met à jour son état, la porte évaluée et les preuves associées dans le même commit. Une case `completed` sans commande, artefact ou commit de preuve est invalide.

## 2. Définition de terminé

Une phase est terminée uniquement si :

- tous ses artefacts existent et sont documentés;
- ses tests positifs et négatifs passent;
- sa porte de sortie est évaluée explicitement;
- les compteurs attendus sont exposés;
- les changements de statut sont justifiés par une preuve ou un différentiel exhaustif adapté;
- la documentation, les références et les scripts passent la CI;
- toute instance GCP ciblée par l'intervention est confirmée `TERMINATED`; les autres VM E-HGP éventuellement actives sont inventoriées et signalées, mais ne bloquent pas la clôture et ne sont jamais mutées sans autorisation explicite.

Une démonstration sur notebook, une partition visuellement plausible ou un benchmark sans certificat ne termine aucune phase.

## 3. Axes orthogonaux et ordre de livraison

Les cinq axes suivants ne doivent jamais être concaténés en un pseudo-statut :

| axe | valeurs initiales | question répondue |
|---|---|---|
| `backend` | `reference_cpu`, `cuda_g4` | quelle implémentation a calculé la sortie ? |
| `profile` | `hgp_reduced`, `full_pi0` | quel objet hiérarchique est promis ? |
| `mode` | `certified`, `budgeted` | exige-t-on une fermeture complète ou autorise-t-on un budget ? |
| `forest_semantics` | `exact`, `partial_refinement` ou absent | la sortie porte-t-elle la forêt exacte, un raffinement partiel, ou seulement des événements vérifiés ? |
| `public_status` | `exact`, `conditional`, `budget_exhausted`, `unsupported_degeneracy`, `numeric_failure` | quelle garantie ce run a-t-il effectivement obtenue ? |

Le backend de référence n'est pas un mode. Un mode `certified` peut échouer sans produire `exact`; un mode `budgeted` ne produit jamais `exact` tant que sa fermeture globale n'a pas été prouvée indépendamment. La combinaison `mode=budgeted, forest_semantics=partial_refinement` conserve les événements individuellement certifiés et un `PartialScope`, mais ne fait aucune assertion d'absence et retourne `public_status=conditional` ou `public_status=budget_exhausted`. Elle est incompatible avec `require_exact=true`. Une réponse qui ne contient que des événements vérifiés omet `forest_semantics` et fournit obligatoirement `PartialScope`.

Pour $n\geq1$, le programme fixe $K_{\mathrm{eff}}=\min(K_{\max},n)$. Les $n$ minima de support un règlent directement le cas de rayon nul. Dans le domaine générique, les événements non nuls nécessaires ont un rang au plus $s_{\max}=\min(K_{\mathrm{eff}}+1,n)$ et la profondeur intérieure maximale vaut

$$m_{\star}=s_{\max}-2=\min(K_{\mathrm{eff}}-1,n-2),$$

lorsque $n\geq2$; pour $n=1$, aucun raffinement n'est lancé. Ainsi $K_{\max}=1$ signifie minima plus sphères de rang deux et se contrôle entièrement par l'EMST; la profondeur neuf n'est atteinte que si $K_{\mathrm{eff}}=10$ et $n\geq11$.

Le développement suit cet ordre :

1. conserver Gamma exhaustif et l'ancienne réduction comme **oracles bornés** sur petits $n$; ils définissent et testent la vérité de référence mais ne prescrivent aucune structure produit;
2. produire directement, sur `reference_cpu` puis `cuda_g4`, le flux de supports de tailles deux à quatre dont le rang fermé est au plus $s_{\max}$, par LBVH, bornes exactes et files de travail bornées;
3. réduire ce flux en journal Morse minimal (`RootBirth`, `AtomicUnionBatch`, `GatewayAttach`, ancrages verticaux) et fermer l'obligation M.1 ainsi que les incidences silencieuses avant toute promotion `public_status=exact`;
4. construire les morphismes verticaux à partir du journal, sans matérialiser les cellules top-$m$ ni les cofaces de Gamma;
5. instrumenter la latence et la mémoire dès le premier flux direct utilisable : les essais courts à 12 500, 25 000 et 50 000 points guident l'architecture sans attendre la fermeture de toutes les preuves;
6. ouvrir ensuite en parallèle la piste topologique (`full_pi0` et dégénérescences) et la piste produit (latence, puis streaming), chacune au statut effectivement prouvé;
7. réunir seulement les jalons dont les portes propres sont fermées et conserver les sorties partielles sous un contrat `PartialScope` sans assertion d'absence;
8. garder la tour saturée et toute reconstruction de mosaïque comme oracles exacts bornés, jamais comme dépendances de la voie 50 k ou 10 M+.

Le profil réduit arrive avant le profil complet parce que les oracles exhaustifs en donnent une définition directement testable sur petits nuages. La voie produit ne doit toutefois jamais matérialiser leur univers combinatoire : Gamma, la mosaïque de Delaunay d'ordre $K$, les cellules top-$m$ et le K-graphe de Gabriel brut restent des instruments de comparaison. Le profil complet demeure un objectif distinct, avec un statut séparé tant que ses attaches ne sont pas toutes certifiées.

## 4. Arborescence logicielle cible

```text
morsehgp3d/
├── CMakeLists.txt
├── pyproject.toml
├── include/morsehgp3d/
│   ├── api.hpp
│   ├── schemas.hpp
│   ├── exact/
│   ├── spatial/
│   ├── power/
│   ├── catalog/
│   ├── hierarchy/
│   └── streaming/
├── src/
│   ├── cpu/
│   ├── cuda/
│   └── python/
├── tests/
│   ├── oracle/
│   ├── unit/
│   ├── property/
│   ├── degeneracies/
│   ├── differential/
│   ├── gpu/
│   └── fixtures/
├── benchmarks/
│   ├── generators/
│   ├── configs/
│   ├── runners/
│   └── schemas/
└── tools/
    ├── inspect_certificate.py
    ├── replay_predicate.py
    └── compare_forests.py
```

Les résultats générés, traces Nsight et grands nuages ne sont pas versionnés. Les configurations, graines, petits contre-exemples et schémas de résultats le sont.

## 5. Schémas à geler avant CUDA

Les schémas suivants doivent avoir une version et une sérialisation canonique :

- `InputSemantics`;
- `CertifiedPoint3`, `ExactRational3`, `ExactLevel` et `VertexWitness`;
- `CriticalEvent`;
- `GammaCoface`;
- `GabrielHyperedge`;
- `Attachment`;
- `EqualLevelBatch`;
- `MergeForest`;
- `VerticalMap`;
- `MorseHGP3DResult`;
- `RunCertificate`;
- `PartialScope`, `FragmentHint` et `CanonicalCellCertificate`;
- `BudgetPolicy` et `BudgetSnapshot`;
- `CheckpointManifest`;
- `BenchmarkRecord`.

Chaque champ possède une unité, une convention d'indexation, une règle de canonisation et une politique de compatibilité. Les rayons publics sont carrés. Les identifiants ne dépendent pas de l'ordre d'arrivée des threads.

## Phase 0 — Gel du contrat mathématique

### But

Transformer les documents actuels en énoncés directement testables et fermer les ambiguïtés avant le code.

### Travaux

- Formaliser `hgp_reduced` comme tour complète à $k=1$, puis comme tour des K-polyèdres non triviaux pour $k\geq2$.
- Figer `hgp-reduced-v2` : Gamma exhaustif est la seule base exacte, limitée à `reference_cpu`; Gabriel brut ne donne qu'une connectivité positive `partial_refinement`.
- Formaliser `full_pi0` comme tour de toutes les composantes des multicovertures.
- Figer l'énoncé candidat M.1 avec événements simultanés, ses hypothèses, sa conclusion et ses obligations de preuve, sans le déclarer démontré.
- Inscrire dans M.1 la multiplicité de Morse $\Delta=\binom{\lvert U\rvert-1}{\mu}$ et, pour $\mu=1$, les $\lvert U\rvert$ bras susceptibles de tuer au plus $\lvert U\rvert-1$ classes de $H_0$.
- Donner une preuve détaillée de l'équivalence sphère rang $k+1$–simplexe de Gabriel.
- Définir exactement les germes d'un événement d'indice un.
- Définir les morphismes verticaux sur les forêts comprimées.
- Figer la position générale minimale et la sémantique des doublons.
- Définir les statuts publics et les conditions nécessaires de `exact`.

### Artefacts

- note d'énoncé M.1 relue et registre explicite des obligations encore ouvertes;
- schémas JSON ou Cap'n Proto versionnés;
- cinq exemples dessinés à la main avec sorties attendues;
- matrice énoncé–test–champ du certificat.

### Tests

- validation et round-trip de chaque schéma;
- refus des champs inconnus critiques et des unités incohérentes;
- exemples $k=1$, naissance isolée, fusion binaire, multifusion et recouvrement $k=2$.

### Porte de sortie

Aucune phrase ne doit confondre profil réduit et profil complet, ni Gamma exhaustif et flot Gabriel brut. Chaque statut public est calculable à partir des champs du certificat. M.1 reste un contrat cible jusqu'à la preuve de la phase 12. Le contrat v1 reste archivé, le contrat v2 et ses tests négatifs sont actifs. Aucun code CUDA avant cette porte.

## Phase 1 — Oracle CPU exhaustif

### But

Construire une vérité terrain indépendante pour $n\leq12$, extensible à $n\leq14$ sur cas sélectionnés.

### Travaux

Pour chaque $1\leq k\leq\min(10,n)$ :

1. énumérer tous les sous-ensembles $F$ de cardinal $k$;
2. calculer leur miniball par tous les supports de taille au plus quatre;
3. énumérer toutes les cofaces $S$ de cardinal $k+1$;
4. construire $\Gamma_k$ complet avec poids exacts;
5. construire indépendamment le K-graphe de Gabriel;
6. trier les valeurs exactes et traiter les égalités par lots;
7. produire `full_pi0` et `hgp_reduced`;
8. calculer les unions d'observations;
9. construire les applications verticales à chaque intervalle;
10. sérialiser tous les événements et coupes.

L'arithmétique utilise les dyadiques exacts ou une multiprécision rationnelle. L'oracle n'importe aucun code de production hormis les schémas.

### Fixtures minimales

- configuration à six points du chapitre 6;
- deux, trois et quatre points bien centrés;
- triangle obtus et tétraèdre non bien centré;
- facette isolée absorbée plus tard;
- deux fusions de même niveau;
- multifusion en un centre;
- deux K-polyèdres recouvrants;
- paire de Gabriel absente d'une petite liste locale;
- points quasi coplanaires et quasi cosphériques à un ULP.

### Propriétés

- invariance sous permutation;
- invariance sous permutations signées des axes, qui préservent exactement les dyadiques IEEE;
- invariance sous les translations dyadiques dont chaque coordonnée transformée a été vérifiée exactement représentable;
- multiplication des niveaux par $2^{2q}$ sous homothétie exacte de facteur $2^q$, sans overflow ni underflow;
- monotonie des coupes en $a$;
- inclusion verticale;
- égalité entre $\pi_0(L_k)$ échantillonné symboliquement et $\Gamma_k$;
- égalité entre `hgp_reduced` et la réduction des composantes de Gamma exhaustif;
- inclusion positive de toute connexion Gabriel brute dans Gamma, sans exiger l'égalité contredite par la fixture permanente.

### Porte de sortie

Toutes les fixtures, y compris le contre-exemple Gabriel, et au moins $10\,000$ petits nuages aléatoires par dimension affine passent pour la cible Gamma. Toute différence produit automatiquement un fichier minimal reproductible. Le flot Gabriel doit passer sa garantie unilatérale et reproduire le désaccord attendu de la fixture, jamais masquer celle-ci.

## Phase 2A — Laboratoire de prédicats exacts CPU

### But

Décider exactement la combinatoire à partir des coordonnées IEEE et fournir la référence portable des filtres futurs.

### Prédicats

- comparaison de deux distances à un point explicite;
- signe de $H_{R,Q}$ à un point témoin;
- orientation 2D dans un plan support et orientation 3D;
- intersection de trois plans;
- appartenance d'un quatrième plan;
- centre circonscrit de deux, trois ou quatre points;
- construction rationnelle homogène du centre et du rayon carré : numérateur, dénominateur strictement positif et représentation canonique réduite ou lazy;
- signes barycentriques et `relint`;
- appartenance stricte, frontière ou extérieur d'une sphère;
- comparaison exacte de deux rayons de miniball;
- égalité de niveaux provenant de supports différents.

Les coordonnées d'entrée dyadiques ne rendent pas en général les centres ni les rayons dyadiques pour les supports de taille trois ou quatre. ExactLevel représente donc un quotient homogène exact. Toute comparaison $a/b$ contre $c/d$, avec $b,d>0$, décide le signe de $ad-bc$ par filtre, expansion de signe puis bigint; aucune division flottante ne décide le tri.

### Étages

1. calcul approché et borne d'erreur FP64;
2. expansion adaptative sur CPU;
3. fallback Boost.Multiprecision;
4. outil de replay par identifiants et bits d'entrée.

### Tests

- au moins $10^7$ signes pseudo-aléatoires contre la référence;
- égalités exactes construites;
- niveaux rationnels égaux issus de supports différents, et niveaux distincts séparés par un quotient arbitrairement proche;
- tri, déduplication, sérialisation canonique et reprise de ExactLevel;
- exposants extrêmes;
- sous-normaux, zéros signés et annulations;
- familles presque coplanaires et presque cosphériques;
- fuzzing différentiel avec réduction automatique.

### Porte de sortie

Zéro signe erroné. Un résultat indécis doit tomber au niveau suivant, jamais choisir une branche par défaut. Les taux de fallback sont publiés mais ne conditionnent pas la correction.

## Phase 2B — Portage des prédicats sur GPU

**Statut opérationnel au 18 juillet 2026 :** `completed`. La porte est fermée par les campagnes certifiées, la qualification du contexte résident et la preuve versionnée `warm_context_e2e`; cette fermeture ne qualifie ni G2 ni un statut public `exact`.

### Entrée

La phase 2A et l'environnement CUDA de la phase 3 sont fermés.

### Travaux

- porter les filtres FP64 et expansions nécessaires;
- conserver le fallback CPU asynchrone;
- instrumenter chaque étage;
- compiler les unités certifiantes sans fast math;
- rendre chaque cas GPU rejouable par l'outil CPU.

### Porte de sortie

Zéro différence CPU/GPU sur le corpus de phase 2A et sur au moins $10^7$ signes supplémentaires. Un `unknown` GPU est transmis au CPU.

## Phase 3 — Environnement reproductible G4

### But

Préparer un socle CUDA mesurable sans lancer encore l'algorithme complet.

### Travaux

- image Docker CUDA 12.9 pour `sm_120`;
- CMake presets CPU, CUDA release, CUDA audit et sanitizer;
- compilation ahead-of-time de tous les kernels mesurés;
- intégration CCCL/CUB, DLPack, NVTX et liaison Python;
- manifeste pilote, CUDA, compilateur, GPU, clocks et image;
- allocation asynchrone et compteur mémoire;
- harness de benchmark JSONL;
- trap de checkpoint et arrêt distant.

### Tests

- allocation jusqu'au plafond configuré puis libération;
- DLPack sans copie;
- kernel déterministe simple;
- erreurs CUDA converties en statut structuré;
- scénario simulé de fermeture GCP;
- test réel court uniquement après autorisation, avec arrêt final vérifié.

### Porte de sortie

Aucune compilation dans une mesure `warm` ou `resident`, aucune fuite, et manifeste complet attaché à chaque résultat. La VM est `TERMINATED` après le test.

## Phase 4 — Canonisation et oracle spatial

**Statut opérationnel au 18 juillet 2026 :** `completed`, backend de décision `reference_cpu`, profil `hgp_reduced`, mode `certified`. Les portes d'entrée et de sortie sont satisfaites; la revue de fermeture est [`PHASE4_GATE_REVIEW.md`](validation/PHASE4_GATE_REVIEW.md).

Le jalon de correction CUDA résident est qualifié au SHA `e5b32ac19c41bd0d7f0c5e6c47c4c2433488ea76` : le device parcourt réellement la topologie LBVH et propose une antichaîne de feuilles candidates ou de sous-arbres strictement extérieurs, puis le CPU reconstruit la couverture et recertifie rationnellement chaque rejet avant toute partition top-$k$ ou boule fermée. Le front parallèle est qualifié au SHA `c846ed7b253840ef6fe1f0f39f7f10c63af64b8e` sur 1 013 cas `Fraction` couvrant toutes les tailles de 1 à 1 000, avec `memcheck`, `racecheck` et audit AOT `sm_120` sans PTX. Cette preuve ferme la Phase 4, mais ne ferme pas la porte globale G2 du catalogue et ne promeut aucun statut public.

### But

Construire l'unique index global utilisé par l'énumération, les rangs et les descentes.

### Travaux

- validation des coordonnées;
- canonisation des identifiants et détection exacte des doublons;
- Morton codes et LBVH;
- 1-NN exact avec exclusion d'au plus $m_{\star}$ IDs;
- top-$k$ exact et shell complet jusqu'au rang $s_{\max}$;
- partition globale intérieur–shell–extérieur exacte; un rejet anticipé dès que le rang dépasse $s_{\max}$ reste une primitive interne explicitement incomplète et ne certifie jamais le shell global ni `RelevantGP`;
- bornes AABB dirigées vers l'extérieur;
- requêtes sur `CertifiedPoint3` sans matérialisation flottante non certifiée.

### Références

- force brute CPU;
- force brute GPU indépendante;
- cuVS brute force optionnel pour les voisins, sans lui déléguer le shell algébrique.

### Tests

- toutes les tailles $n\leq1\,000$ contre force brute;
- requêtes aléatoires, centres critiques et intersections de plans;
- exclusions, ties, doublons et distances extrêmes;
- permutation des points et ordre de parcours BVH;
- vérification que tout élagage possède une marge certifiée.

### Porte de sortie

Identifiants, ordre, shell et rang identiques à la référence. Aucun chemin exact ne dépend d'un `epsilon` ou d'une limite arbitraire de visites.

## Phase 5 — Ancre $k=1$ et EMST

Statut opérationnel : `ready`; proposition et rejeu indépendant `cuda_g4`, recertification, décision, contraction et réduction compacte locale `reference_cpu`, profil `hgp_reduced`, mode `certified`. La boucle hybride complète chunkée, son budget de confiance, le resserrement Morton borné avec cutoff exact monotone, leurs rejeux indépendants et le témoin EMST local sont qualifiés sur G4. Deux benchmarks séparés en mode `benchmark` distinguent le producteur Morton exhaustif, quadratique sur les familles uniformes et en amas, de la recherche external-1NN exacte sans payload candidat, mesurée jusqu'à 16 384 points avec des exposants empiriques entre 1,090 et 1,240 sur le dernier quadruplement. Ils ne revendiquent ni qualification, ni scalabilité, ni résultat scientifique. Une famille dyadique exacte montre que la frontière point--LBVH indépendante par source exige un travail $\Omega(n^2)$ dans le pire cas du modèle paramétrique. Un parcours self-dual partagé exact neutralise ce bloc sur hôte, ferme la partition des paires, borne sa pile par $2H+1$ et ses visites par $n(n+1)-1$, puis offre chaque graine recertifiée aux deux composantes incidentes avant de réduire directement les cutoffs. Après les enveloppes figée et sparse, le mode `exact_current_maximal_uniform_roots` partitionne les feuilles par racines uniformes maximales, les indexe par composante en CSR, lit tout nœud uniforme depuis le cutoff live et recalcule les maxima mixtes. Son enveloppe est pointwise au plus les enveloppes sparse, figée et dynamique par point; le parcours déterministe domine donc chacune sur visites, expansions, bornes AABB--AABB et distances exactes. Le nouveau mode hôte `exact_current_deduplicated_mixed_ancestors` conserve cette enveloppe mais traite une seule fois l'union des ancêtres mixtes : pour $r$ racines et une union $A$, il compte exactement $\lvert A\rvert+r-1$ découvertes, $r-1$ doublons et au plus $\lvert A\rvert$ recomputations. Cette borne est linéaire par baisse stricte, sans dominance temporelle ni borne globale sous-quadratique. La chaîne Borůvka locale conserve explicitement `frozen_initial` et son `proof_basis` v4. Le schéma hôte v5 ajoute désormais la voie courante dédupliquée au v4 et se projette exactement sur v4, v3 puis v1; sa fixture exerce un doublon positif sans réduction stricte des recomputations et ne change pas le pipeline G4. La réduction explicite ferme une `K1CompactForest` locale après rejeu frais du témoin historique; la qualification CUDA du parcours partagé, une amélioration sous-quadratique, les ordres supérieurs, les applications verticales et le statut public global restent ouverts.

### But

Valider la première hiérarchie avant tout ordre supérieur.

Le premier parcours self-dual partagé est maintenant certifié sur hôte : il partitionne exactement les $\binom{n}{2}$ paires non ordonnées, neutralise le bloc adversarial de `morton_overlap` et ferme les bornes $F\leq2H+1$ et $V\leq n(n+1)-1$. La ronde directe offre d'abord chaque graine externe aux composantes de ses deux extrémités, réduit ces offres en un incumbent exact faisable par composante et relâche les deux composantes de chaque paire externe sans tableau ponctuel. L'enveloppe exactement courante lit les tags uniformes depuis l'incumbent de composante et ne cache que les maxima mixtes; elle ferme ainsi la dominance des quatre compteurs de parcours jusque contre la variante dynamique par point. Sa variante dédupliquée marque l'union des ancêtres mixtes d'une composante puis la traite une fois en ordre bottom-up; elle borne linéairement chaque maintenance sans borner le nombre total de baisses. La voie persistante v4 demeure figée, reconstruit tags, slots et enveloppe après chaque contraction, puis exige un rejeu direct neuf et l'accord de l'ancre CPU. Les listes CSR et structures de déduplication du mode courant sont transitoires et comptées; aucune dominance de temps ou du travail total n'est établie. Une amélioration sous-quadratique et la qualification matérielle restent ouvertes.

### Travaux

- implémenter ou adapter un EMST GPU exact dans sa combinatoire;
- injecter canoniquement les $n$ événements de support un, rayon nul et rang un;
- grouper les arêtes de longueurs égales;
- produire $T_1$ aux niveaux $\left\Vert u-v\right\Vert^2/4$;
- implémenter en parallèle le catalogue des sphères de rang deux;
- produire le graphe de Gabriel puis sa réduction;
- comparer les deux forêts nœud par nœud.

### Tests

- oracle complet jusqu'à $n=14$;
- nuages colinéaires, coplanaires et 3D;
- grilles et égalités de longueurs;
- paire locale manquante;
- famille `morton_overlap` aux tailles 66, 258 et 1026, avec collision Morton centrale et bornes quadratiques exactes;
- même famille sur le parcours self-dual, avec fermeture des $\binom{n}{2}$ paires et plafonds de travail séparés;
- fixture 3D à cinq singletons discriminant l'enveloppe max et la relaxation bidirectionnelle de la ronde directe par composante;
- sur cette même ronde et les mêmes graines, comparaison `sparse_witness_path_monotone`--`frozen_initial` des minima, majorants live, maxima internes, domination pointwise et quatre compteurs de parcours, sans seuil empirique;
- fixture `uniform`, $n=12$, $W=2$, graine deux, comparant l'enveloppe exactement courante aux voies dynamique, figée et sparse sur les mêmes partitions, avec couverture CSR, mises à jour de racines et remontées mixtes fermées;
- composante fragmentée possédant plusieurs racines uniformes dont les chemins partagent un long tronc mixte, avec égalités exactes entre découvertes, ancêtres distincts et doublons, parcours bottom-up complet, recomputations au plus égales aux ancêtres distincts et mêmes minima et contractions que la baseline courante;
- fixture 3D entière à quatre points et deux composantes forçant une diminution stricte du cutoff de $2134$ à $14$ par l'offre cible d'une graine recertifiée;
- régression dyadique verrouillant le remapping des labels d'ordre d'entrée vers les `PointId` canoniques avant toute construction Morton;
- jusqu'à plusieurs millions de points pour l'EMST seul.

### Porte de sortie

Les coupes, niveaux et multifurcations des deux voies sont identiques après canonisation. Cette porte bloque toute revendication sur $k>1$.

### Progression actuelle

Les cinq premiers jalons CPU `reference_cpu` sont livrés : graphe complet euclidien exact, EMST, minima singleton, lots égaux figés, catalogue global de toutes les boules diamétrales, séparation exacte entre événements rang-deux, dégénérescences extra-shell et paires bloquées, réduction rang-deux indépendante, forêt hiérarchique compacte construite depuis un EMST certifié, puis producteur Borůvka exact sur le LBVH global. Le certificat compare aux niveaux réunis les coupes strictes et fermées du graphe complet, de l'EMST, du graphe rang-deux, de son arbre témoin et du graphe Gabriel diagnostique; il exige aussi les mêmes multifusions et poids exacts. Un dump canonique est confronté sur 50 cas à trois oracles Python indépendants ou séparément écrits, pour toutes les tailles jusqu'à $n=14$ et les dimensions affines un à trois. La forêt compacte remplace les couvertures persistantes potentiellement quadratiques par des feuilles implicites et des enfants CSR, sans changer les niveaux d'événements, coupes ni multifusions; ses cinq arènes principales sont bornées par $6(n-1)$ enregistrements.

Le producteur Borůvka fige les labels de composantes à chaque ronde et ordonne les arêtes par la clé totale $\kappa(e)=(d^2(e),u,v)$, avec $u<v$. Le minimum unique de chaque coupe appartient à l'arbre de Kruskal canonique; les arêtes acceptées forment donc une sous-forêt de cet arbre. Chaque composante non terminale est incidente à une arête choisie, ce qui donne $c_{r+1}\leq\left\lfloor\frac{c_r}{2}\right\rfloor$. Le parcours LBVH rejette un sous-arbre par sa borne AABB exacte seulement sous inégalité stricte; une égalité descend afin de préserver le tie-break sur les extrémités. Un vérificateur séparé rejoue minima, contractions, poids et borne de rondes sans faire confiance au drapeau de résultat. Ce jalon est le socle CPU de la ronde GPU de proposition; il ne produit aucun `public_status=exact`. La [note de progression](validation/PHASE5_PROGRESS.md) en fixe la preuve, les invariants et la portée de chaque chemin. La Phase 5 reste ouverte jusqu'à l'adoption d'un autre parcours exact scalable ou d'une portée explicitement bornée, et jusqu'à l'intégration de cette ancre locale dans les ordres et morphismes de la tour complète.

La première ronde GPU de proposition est maintenant implémentée sous la sémantique `gpu_stackless_lbvh_fixed_seed_candidate_superset`. Pour chaque point $q$, une cible sortante déterministe fixe exactement un rayon $R_q$; le GPU doit retourner sans troncature un sur-ensemble de toutes les cibles externes à distance au plus $R_q$. Le LBVH résident utilise des cordes left-first, des tags uniformes ou mixtes sur les composantes figées et une borne AABB binary64 dirigée vers le bas; il ne prune que si cette borne est strictement supérieure au majorant binary64 du rayon, tandis qu'une borne invalide ou une égalité descend. Deux noyaux count/emit et un préfixe contrôlé donnent une capacité logique exacte, sans append global atomique.

Le GPU reste exclusivement producteur de candidats. Le CPU rejoue exactement l'ensemble exigé par la graine, invalide toute omission, réévalue les distances et résout les minima de points puis de composantes selon $\kappa$. `build_gpu_proposed_cpu_exact_k1_boruvka` enchaîne désormais ces propositions dans un unique contexte producteur résident, applique après chaque décision une contraction CPU canonique et retourne les $n-1$ arêtes d'un témoin EMST local. Les audits de proposition, minima exacts et contractions sont des payloads et statuts distincts; les candidats complets restent éphémères. Un vérificateur crée un second contexte GPU, rejoue la chaîne, la compare à l'ancre Borůvka CPU et recalcule contractions et poids sans faire confiance aux champs reçus.

Cette boucle complète passe en Release strict sur le lanceur hôte, y compris le singleton, une chaîne $8\to4\to2\to1$, l'égalité de la forêt compacte induite et la falsification indépendante des trois couches. Elle ne construit elle-même aucune hiérarchie et ne peut produire `public_status=exact`. Le replay multi-ronde monolithique est qualifié au SHA `c199651d86e861eb755357986d036889839578d4` en AOT CUDA 12.9 `sm_120` sans PTX : singleton, chaîne $8\to4\to2\to1$ de poids exacts $8127$ et $8127/4$, carré à ex æquo, rejeu GPU indépendant, `memcheck` et `racecheck` ferment l'artefact `phase5-k1-boruvka-c199651d86e861eb755357986d036889839578d4.json` de SHA-256 `b10e3bb8c94d6e8fa0f70223d5faa99d94a5144701a87c087587753c912a7215`, publié seulement après l'arrêt ciblé certifié. Le chemin chunké complet conserve des sources atomiques, reçoit sa politique de budget séparément du résultat non fiable, distingue le volume logique des capacités physiques et ne publie aucune décision avant le dernier chunk. Au SHA `6d944132d2f7d261a934a1864788c2fb7a81831f`, son replay v2 réel ferme AOT `sm_120` sans PTX, `memcheck`, `racecheck` et un second rejeu GPU chunké; la chaîne utilise seize chunks pour 86 candidats logiques avec un pic simultané de 224 octets. L'artefact `phase5-k1-boruvka-6d944132d2f7d261a934a1864788c2fb7a81831f.json` de SHA-256 `c247c1de8dc1a0d6d4aad31fada79c1bc3ca09019146e427cb35fc6ab41d68a4` a été publié seulement après l'arrêt ciblé certifié. Le resserrement Morton inspecte au plus $2Wn$ voisins par ronde, recertifie exactement au plus une proposition distincte par source et conserve un cutoff externe inférieur ou égal au fallback. Au SHA `7c4933b678cbc6d9860e33596522ab971c0c5df5`, le replay v3 réel ferme sur G4 AOT `sm_120` sans PTX, `memcheck`, `racecheck`, les certificats de graines et leur rejeu indépendant; sur la chaîne avec $W=1$, il réduit le volume logique de 86 à 41 et les chunks de seize à neuf sans modifier décisions exactes, contractions, EMST ni poids. L'artefact `phase5-k1-boruvka-7c4933b678cbc6d9860e33596522ab971c0c5df5.json` de SHA-256 `b0ef2101bb37bacffaffffc4051ea5219aa8b79fda0a5fe9510d58467ebd7a01` a été publié après arrêt ciblé certifié.

Le profil final au SHA propre `4cbdb2bb7f0fb9decc9ede1c9a313727cb8b93ed` couvre `uniform`, `clusters` et `lattice` pour $n\in\left\lbrace64,256,1024\right\rbrace$ et $W\in\left\lbrace1,4,16\right\rbrace$. Son artefact `phase5-k1-boruvka-work-profile-4cbdb2bb7f0fb9decc9ede1c9a313727cb8b93ed.json`, de SHA-256 `e39e1355c3b4381858e6bfd1272d7a7048aa1078648a0e8ec8fcc0f372c07e54`, est lié à l'environnement Phase 3 de SHA-256 `e15597be190786f9cd27107d647d98a943675f4ef5781f617b1ebfd9a21101a7`. La session gardée du projet `devpod-gpu-exploration` a démarré la cible `europe-west4-ai1a/ehgp-blackwell-spot-ai1a` à `2026-07-19T02:18:17.482-07:00`, vérifié le coupe-circuit invité de 45 minutes, puis arrêté cette cible précise et l'a relue `TERMINATED` le `2026-07-19T09:25:18Z`. Entre 256 et 1024 points, le travail exact des voies Morton croît sur `uniform` et `clusters` par des facteurs de $16.12$ à $20.49$, soit des exposants empiriques de $2.005$ à $2.178$; $W=16$ procure des gains de constante importants mais ne rend pas ces familles scalables. `resolve_round_exact_external_1nn` réalise désormais la décision suivante sans payload candidat : une graine Morton externe recertifiée initialise le cutoff, le CPU explore le LBVH par borne exacte point--AABB, élague seulement sous inégalité stricte, descend sur égalité et réduit un minimum exact par source au minimum de composante. `build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka` enchaîne ces décisions, contracte séparément, relâche les minima ponctuels et exige un rejeu frais ainsi que l'accord tour par tour de l'ancre CPU avant de certifier le témoin EMST local. Le singleton et la chaîne $8\to4\to2\to1$ sont fermés sur hôte strict. Le benchmark G4 propre au SHA `a81d8e50e4655a2f1b6acad74bbffddbc98ff0ba` étend les trois familles jusqu'à 16 384 points avec $W=16$. Entre 4096 et 16 384, son proxy exact est multiplié par $5.582$, $5.118$ et $4.531$, soit les exposants empiriques $1.240$, $1.178$ et $1.090$; le pire parcours atteint 235 nœuds et la frontière 93, sans record candidat. L'artefact de SHA-256 `8c66a5027ff446a6df43faa87ff7da5360e4b9479e662cbb3637dde925d54a10` reste `benchmark_only` et ne prouve aucune borne asymptotique. La famille dyadique `morton_overlap` force cependant au premier tour singleton au moins $(n-2)^2/8$ expansions et $3(n-2)^2/8$ visites et bornes pour la frontière indépendante par source actuelle; les témoins binary64 $n=66,258,1026$ passent sous GCC et Clang stricts. Ces mesures favorables ne peuvent donc pas différer une stratégie partagée, batchée ou dual-tree si la cible reste scalable. L'adaptateur séparé recertifie désormais cette source avant construction, compare ses cinq arènes à la réduction du témoin et à celle d'une nouvelle ancre Borůvka, puis publie seulement `compact_k1_forest_certified/local_k1_compact_forest_only`. Le champ `hierarchy_reduction_status=not_performed` du résultat EMST source reste inchangé et aucun statut public n'est créé.

Le resolver alternatif `resolve_round_exact_external_1nn_dual_tree` ferme désormais sur `reference_cpu` la couverture des $\binom{n}{2}$ paires, les minima exacts par point et par composante, les égalités de $\kappa$ et trois partitions successives de la chaîne. La décomposition est exécutée par une pile DFS localement near-first et l'audit certifie les bornes générales $2H+1$ sur la pile et $n(n+1)-1$ sur les visites. `resolve_round_exact_component_minima_dual_tree` ajoute aux modes figé et sparse les modes `exact_current_maximal_uniform_roots` et `exact_current_deduplicated_mixed_ancestors`. Les racines uniformes maximales partitionnent les feuilles et sont indexées par composante dans deux tableaux CSR; l'accesseur lit un nœud uniforme directement dans l'incumbent live de son tag, tandis que les nœuds mixtes conservent le maximum exact de leurs enfants. La baseline courante rafraîchit séparément les chemins de toutes les racines CSR; la variante dédupliquée marque leur union, certifie exactement $\lvert A\rvert+r-1$ découvertes et $r-1$ doublons pour une baisse à $r$ racines, puis traite chaque nœud sale une fois en ordre bottom-up. Elle effectue au plus $\lvert A\rvert$ recomputations et conserve la même enveloppe exacte. `build_gpu_seeded_cpu_exact_dual_tree_k1_boruvka` reste explicitement `frozen_initial` sous son contrat v4 et exige les champs current et dédupliqués nuls dans producteur et rejeu frais. La borne linéaire par baisse ne borne ni leur nombre total, ni le temps, et aucune borne globale sous-quadratique n'est ajoutée.

Le work-profile external-1NN conserve le mode par défaut v1 et `--compare-resolvers` v3 byte-contractuellement inchangés. `--compare-current-envelope` émet la v4, qui ajoute `direct_current` aux trois voies v3 et doit se projeter exactement sur v3 puis v1. Sur chaque partition et chaque audit Morton communs, les quatre minima et contractions doivent coïncider avant publication des compteurs. La fixture `uniform`, $n=12$, $W=2$, graine deux, contracte $12\to4\to1$ et totalise `(visites,expansions,distances)` égaux à `(141,61,15)` en courant, `(147,64,22)` en figé, `(147,64,19)` en sparse et `(153,67,27)` en dynamique. Les rondes courantes ferment respectivement 12 puis sept racines, trois mises à jour de racines chacune, sept recomputations mixtes chacune et six puis cinq mises à jour mixtes, pour trois puis deux baisses strictes. Ces comptes protègent seulement le contrat hôte v4. `--compare-deduplicated-current-envelope` émet la v5 en ajoutant `direct_deduplicated_current`; son retrait restitue v4, puis v3 et v1. Sur la même fixture, ses tuples `(distincts,doublons,découvertes,recomputations,mises à jour,max distinct par baisse)` valent `(10,0,10,7,6,4)` puis `(7,1,8,7,5,4)`, avec les mêmes 141 visites, 61 expansions et 15 distances que current. La collision positive de la seconde ronde valide la déduplication structurelle, mais les sept recomputations de chaque ronde ne montrent aucun gain strict. Aucune accélération temporelle ou borne sous-quadratique n'est revendiquée, et le pipeline G4 reste v1.

## Phase 6 — Miniballs et descentes

Statut historique à la livraison du jalon 6.23 : `ready`; backend préparatoire `reference_cpu`, profil d'autorité `full_pi0` projeté vers `hgp_reduced`, mode `certified`, portée `bounded_n14_k10_single_order_morse_minimum_saddle_partition_sweep_compared_to_exhaustive_gamma_at_every_activation_level_only`. À cette date la phase 5 était l'unique phase `in_progress`; le registre courant place les Phases 5 et 6 à `ready`, la Phase 9 à `complete` et la Phase 10 à `in_progress`. Les jalons préparatoires 6.1 à 6.6 certifient la miniball locale, son shell global, la famille top-$k$, l'arc canonique strict, son segment analytique, une chaîne issue d'une seule facette, puis le germe initial exact d'un bras critique d'indice un et son raccord. Le jalon 6.7 ferme tous les bras d'un même événement en classes d'identité de facettes terminales; 6.8 construit indépendamment la coupe exhaustive strictement ouverte de Gamma; 6.9 raccorde une famille complète à ses composantes strictement antérieures; 6.10 ferme la transition exhaustive et simultanée de $\Gamma_k^{<a}$ vers $\Gamma_k^{\leq a}$; 6.11 superpose les seules provenances événementielles fournies et complètes aux groupes qui les contiennent; 6.12 construit le catalogue critique exhaustif borné et ses lots H0; 6.13 projette chaque transition 6.10 vers la sémantique locale `hgp_reduced`; 6.14 déroule tous les niveaux exacts d'un ordre fixé dans un journal compact; 6.15 rejoue une coupe ouverte ou fermée depuis un préfixe entier de ce journal. Le jalon 6.16 reconstruit fraîchement 6.12 et 6.14 depuis le même nuage puis raccorde exhaustivement chaque rôle H0 à son slot Gamma fermé par niveau et `closed_point_ids`; 6.17 reconstruit ensuite toutes les familles de bras de toutes les selles d'un ordre et certifie leur composante stricte `full_pi0` par double lookup des facettes initiale et terminale, en gardant les annotations `hgp_reduced` séparées. Le jalon 6.18 réconcilie ces deux sources fraîchement vérifiées dans un unique journal mono-ordre typé. Le jalon 6.19 recertifie ce journal externe, rejoue son histoire une fois sur des snapshots atomiques et raccorde chaque cible non triviale à l'unique racine réduite locale pré-lot de même famille complète; une cible singleton est au contraire certifiée absente de toutes les racines et explicitement omise. Le kind réduit n'est contrôlé qu'après cette décision. Le jalon 6.20 compose ensuite chaque bras avec cette liaison sans nouvelle géométrie et sans dédupliquer les bras partageant une cible. Le jalon 6.21 reconstruit le catalogue puis une famille 6.7 par selle et conserve un chemin strict compact, exact et rejouable pour chaque candidat 6.20, avec sa cible `full_pi0` externe et sa disposition réduite séparée. Le jalon 6.22 calcule les vrais `event_id` v2 par projection scientifique complète et SHA-256 séparé par domaine, puis agrège canoniquement exactement un tuple durable `(event_id, order, removed_shell_id)` par chemin 6.21. Le jalon 6.23 rompt volontairement cette dépendance décisionnelle à Gamma : il construit d'abord une généalogie candidate depuis les seuls minima du catalogue et les terminaux complets 6.7, contracte ensemble toutes les selles d'un même niveau, puis utilise Gamma uniquement comme oracle postérieur à tous ses niveaux d'activation. L'objet reste un falsificateur interne : aucun `attachment_id`, `Attachment`, `target_node_id`, lot public, fermeture de H5, O3 ou M.1, attache verticale, DAG global ou forêt multi-ordre n'est publié, et aucun `public_status` n'est modifié.

### But

Implémenter l'oracle d'attache indépendant.

### Travaux

- énumération des 385 supports au plus par warp;
- centre et rayon filtrés;
- support minimal canonique;
- calcul du successeur top-$k$;
- preuve machine de la décroissance;
- émission des segments rejouables;
- fermeture des labels découverts;
- pointer-jumping déterministe;
- détection des plateaux et arrêt explicite.

### Tests

- toutes les facettes de l'oracle petit;
- comparaisons à Welzl ou CGAL optionnel;
- absence de cycle;
- segment sous-niveau;
- racine égale à la composante exhaustive;
- selles avec bras distincts, déjà reliés ou redondants;
- statistiques de longueur par famille.

### Porte de sortie

Chaque attache non dégénérée coïncide avec l'oracle. Toute hypothèse violée produit un statut précis. La descente n'est pas encore utilisée pour déclarer le catalogue complet.

### Progression actuelle

`build_exact_facet_miniball` exploite le fait qu'en dimension trois toute boule englobante minimale possède un support affinement indépendant de cardinal au plus quatre dont le centre appartient à l'intérieur relatif. Pour $k\leq10$, l'énumération exhaustive contient exactement $\sum_{j=1}^{4}\binom{k}{j}\leq385$ supports. Chaque support bien centré est classifié exactement contre toute la facette; le plus petit rayon est choisi, puis les ex æquo sont départagés par cardinalité et `PointId`. Le résultat conserve le nombre de supports optimaux et la partition intérieur--shell afin qu'un support canonique ne soit jamais présenté comme support essentiel unique.

`build_exact_facet_descent_preconditions` réutilise ce certificat local, appelle les partitions exactes `closed_ball` et top-$k$ sans exclusion au même centre, puis vérifie leurs identités croisées. L'activité signifie $F\in\mathcal{N}_k(c_F)$ et non l'égalité de $F$ avec `canonical_choice_ids`; elle équivaut à l'absence de point de $X\setminus F$ strictement intérieur. Le domaine régulier exige en plus `boundary_point_ids==support_point_ids`, un unique support optimal et l'absence de point de $X\setminus F$ sur la frontière. Si la facette régulière est inactive, ces faits ferment les hypothèses du théorème universel $\beta(G)<\beta(F)$ pour tout $G\in\mathcal{N}_k(c_F)$; le logiciel certifie les préconditions, sans désigner le représentant de la partition comme successeur ni revendiquer un arc de descente.

`verify_exact_facet_descent_preconditions` rejoue la miniball et les deux partitions globales, puis falsifie séparément identité, compteurs, décisions et portée. Les fixtures ciblées distinguent : une paire diamétrale avec intrus strict dont le cutoff top-2 reste égal à $\beta(F)$ mais qui est `strict_descent_admissible`; la même géométrie avec un point extérieur sur le shell; une facette à frontière non essentielle où un choix top-3 descend et un autre forme un plateau; une paire déjà active; et une famille top-2 où la facette est active malgré un représentant canonique différent. La [note de progression](validation/PHASE6_PROGRESS.md) sépare la preuve mathématique conditionnelle de la validation logicielle hôte.

Le contrat préparatoire 6.3 prend le résultat 6.2 fraîchement rejoué. Pour une source `strict_descent_admissible`, il fixe $G$ égal à `canonical_choice_ids`, exige son appartenance à la famille top-$k$ et $G\neq F$, recalcule la miniball exacte de $G$, puis ferme $\beta(G)\leq D_k(c_F)\leq\beta(F)$ et $\beta(G)<\beta(F)$ sous `exact_descent_preconditions_canonical_top_k_member_fresh_miniball_strict_level_v1`. Les sources `already_active_at_own_center` ou `unsupported_degeneracy` retournent respectivement `no_arc_already_active_at_own_center` ou `no_arc_unsupported_degeneracy`, sans payload d'arc. Une violation de la stricte inégalité après gate régulier est une contradiction fail-closed, pas un plateau accepté. Les tests hôtes stricts GCC et Clang ferment les chaînes exactes $1/4<1=1$ et $1=1<4$, dont la seconde possède le même centre source--cible, puis l'absence totale d'arc pour les cas actif et non pris en charge; ils mutent séparément préconditions, optionnels, choix canonique, miniball cible, booléens, compteurs, décision, portée et identité du nuage. Le segment sous-niveau, le DAG, le pointer-jumping, les attaches, le différentiel indépendant, CUDA et G4 restent ouverts.

Le contrat préparatoire 6.4 rejoue 6.3 et n'émet un témoin que pour `strict_descent_arc_certified`. Avec $R=\beta(F)$, il recalcule $a=g_G(c_F)=D_k(c_F)\leq R$, lit $b=\beta(G)<R$ et calcule $\delta=\left\Vert c_G-c_F\right\Vert^2\geq0$. L'identité quadratique par point puis le maximum donnent $g_G(\gamma(t))\leq q(t)=(1-t)a+tb-t(1-t)\delta$; la décomposition de $q(t)-R$ ferme le segment entier dans le sous-niveau fermé et seulement $\gamma((0,1])$ dans le sous-niveau strict lorsque la source est au niveau. Le cas $\delta=0$ est valide et impose centres égaux ainsi que $a=b$. Les branches 6.3 sans arc retournent une décision sans témoin. Les tests hôtes stricts GCC et Clang valident `(a,b,delta)=(1,1/4,1/4)` avec $q(1/2)=9/16$ comme diagnostic, `(1,1,0)` avec centres égaux, les deux absences de témoin et les mutations de chaque champ. La portée `canonical_strict_arc_half_open_sublevel_segment_only` sous `exact_squared_distance_chord_identity_max_envelope_half_open_segment_v1` ne couvre ni concaténation, DAG, pointer-jumping, germe, attache, forêt, différentiel indépendant, CUDA ou G4.

Le contrat préparatoire 6.5 relance 6.4 à chaque nœud et exige que la facette, le centre rationnel et le niveau de miniball de la cible précédente coïncident exactement avec la source suivante. La baisse $\beta(F_{i+1})<\beta(F_i)$ à cardinal fixé exclut tout cycle et borne l'orbite stricte à $\binom{n}{k}-1$ segments; `finite_strict_facet_orbit_theorem_certified` certifie ce théorème, pas l'exhaustivité du budget effectif. Pour $R_0=\beta(F_0)$, le premier segment privé de sa source est strict sous $R_0$, et chaque segment fermé ultérieur est sous $\beta(F_i)<R_0$; la polyligne privée de son premier point est donc strictement sous $R_0$. Les budgets explicites zéro, un et exact conservent un `stopping_probe` complet et distinguent préfixe budgétaire, dégénérescence et terminaison active. La fixture à six points ferme les niveaux $52>85/4>325/16$; une seconde fixture ferme $58>49/4>1/4$ avec le niveau atomique $41/4<49/4$ à la couture intermédiaire, afin d'interdire toute égalité indue entre le niveau de miniball cousu et le niveau atomique du segment suivant. La portée `single_source_canonical_strict_descent_chain_only` exclut encore germe initial, fermeture multi-source, DAG, pointer-jumping, plateau, attache, forêt, statut public, différentiel indépendant, CUDA et G4.

Le contrat 6.6 reçoit le shell critique complet $U$, un point retiré $u\in U$ et la partition globale $S=I\cup U$, puis fixe $k=\lvert S\rvert-1$ et $F_u=S\setminus\left\lbrace u\right\rbrace$. Il accepte seulement $2\leq\lvert U\rvert\leq4$, un support positif minimal égal au shell global complet, un rang fermé au plus onze et donc une facette de bras d'ordre au plus dix. La miniball fraîche de $F_u$ doit avoir un niveau strictement inférieur au niveau critique et un centre distinct. Pour $d=c_{F_u}-c$, le coefficient directionnel du point retiré doit être strictement positif; chaque point extérieur avec clairance $A_p>0$ et coefficient $B_p=2(c-p)\mathbin{\cdot}d<0$ fournit la borne conservative $A_p/(-2B_p)$. Le minimum exact de ces bornes et de un certifie un préfixe $t\in(0,\tau]$ strictement sous le niveau critique. Le raccord composite exige ensuite l'identité exacte de $F_u$, de son centre et de son niveau avec le premier nœud 6.5; le budget 6.5 compte uniquement les segments engagés après ce germe initial. `committed_composite_path_segment_count` compte le segment initial engagé et les segments de chaîne engagés, jamais le `stopping_probe` certifié mais non engagé. Les sources non minimales, les shells globaux incomplets et les rangs fermés supérieurs à onze échouent fermés sans bras. Les tests ajoutent notamment le support critique positif à quatre points avec $(a,b,\delta,B_u)=(3,8/3,1/3,2)$ et rejettent tout budget supérieur à 4096 même lorsque la source est non prise en charge. Les builds Release stricts GCC et Clang et le CTest `morsehgp3d.hierarchy_miniball` passent; le statut logiciel est `validated_host_software`. La portée demeure mono-bras et exclut fermeture de labels, racine terminale, attache, DAG, pointer-jumping, plateau, forêt, différentiel indépendant, CUDA, G4 et `public_status`.

Le contrat préparatoire 6.7 canonise $U$, énumère exactement ses $\lvert U\rvert$ points retirés et appelle 6.6 indépendamment sous un budget commun. Il exige que toutes les copies reconstruisent la même source critique et conserve, pour chaque bras, le point retiré, la descente complète et son éventuelle facette terminale régulière active. Les labels terminaux sont les listes canoniques de `PointId`; leur égalité exacte forme une classe d'identité ordonnée et chaque classe conserve la provenance de tous les bras qui l'atteignent. Une égalité de label certifie seulement la même facette terminale. Une facette terminale active n'est pas une racine globale, des labels distincts peuvent encore appartenir à la même composante de Gamma, et la classe obtenue n'est ni Gamma, ni une attache.

`complete_terminal_label_partition_certified` n'est vrai que si chaque bras termine régulièrement et activement. Une source non prise en charge ne produit aucune famille terminale; un arrêt sur dégénérescence, sur budget ou sur les deux laisse au plus des classes observées partielles et retourne une décision incomplète distincte. Le rejeu frais reconstruit chaque 6.6, les terminaux, les classes, leur provenance, les compteurs, la décision et la portée `all_index_one_critical_arms_independent_canonical_strict_chains_terminal_labels_only`. Les critères hôtes sont les builds GCC et Clang Release stricts et le CTest ciblé `morsehgp3d.hierarchy_miniball`; ils ne créent aucun statut public. Le jalon 6.8 ferme indépendamment le Gamma exhaustif à la coupe ouverte $\beta<a$; 6.9 effectue maintenant le raccord borné sans promouvoir les composantes obtenues en attaches.

Le contrat préparatoire 6.8 introduit les types `ExactStrictGamma*` sur `reference_cpu`. Il accepte $n\leq14$, $1\leq k\leq10$ avec $k<n$ et une à quatre facettes sources distinctes de cardinal $k$. La coupe est exclusivement ouverte : facettes et cofaces sont actives sous la comparaison rationnelle exacte $\beta<a$. Une égalité $\beta=a$ reste inactive; aucune variante fermée $\beta\leq a$ et aucun epsilon ne peuvent être substitués au prédicat strict.

Le préflight est atomique. Il calcule $\binom{n}{k}$ facettes, $\binom{n}{k+1}$ cofaces et $k\binom{n}{k+1}$ tentatives d'union, puis refuse toute la coupe si un seul des trois budgets est insuffisant. Aucun catalogue partiel, aucune composante et aucune classification source ne sort alors de `no_cut_preflight_budget_insufficient`. Lorsque le budget couvre les trois bornes, chaque facette taille au plus dix reçoit une miniball exacte mise en cache. Les cofaces taille au plus dix utilisent la même primitive directement. Une coface taille onze, possible seulement pour $k=10$, utilise $\beta(Q)=\max_{q\in Q}\beta(Q\setminus\lbrace q\rbrace)$ sur les onze niveaux taille dix déjà en cache : un support minimal 3D contient au plus quatre points, donc une suppression qui l'évite atteint le niveau de $Q$, et aucune suppression ne peut le dépasser. Le certificat choisit le premier maximiseur lexicographique et rejoue la couverture du point omis.

Le DSU exhaustif matérialise toutes les facettes actives, y compris les facettes isolées requises par `full_pi0`, puis chaque coface active réunit ses $k+1$ facettes par exactement $k$ tentatives. Les composantes sont canonisées comme familles de labels de facettes. Chaque source est classifiée active avec son indice de composante, ou inactive sans indice; la présence d'une source inactive produit `complete_with_inactive_sources`, pas un calcul incomplet. Le rejeu frais recalcule préflight, miniballs, cache taille dix, témoins taille onze, activations strictes, DSU, composantes, classifications, compteurs et décision.

La base de preuve `exact_bounded_exhaustive_strict_gamma_full_pi0_source_component_classification_v1` et la portée `bounded_exhaustive_strict_gamma_full_pi0_source_components_only` ne couvrent que cette coupe bornée et ses classifications. Les fixtures hôtes obligatoires couvrent un cas binaire, un raccord passant par une facette-pont $P$, une classification ternaire partiellement inactive, le contre-exemple Gabriel à cinq points et le bord $n=11$, $k=10$. Les builds GCC et Clang Release stricts et le CTest ciblé `morsehgp3d.hierarchy_gamma` valident le certificat; le différentiel indépendant reste un test séparé. Gabriel n'est jamais utilisé comme substitut de Gamma. Aucun indice de composante n'est promu en racine ou attache, aucune structure de DAG ou de forêt n'est construite, aucun `public_status` n'est émis et aucune scalabilité n'est revendiquée.

Le contrat 6.9 introduit `ExactCriticalArmGamma*` sur le même backend. Il appelle d'abord 6.7 et n'engage 6.8 que lorsque chaque bras possède une terminaison régulière active. L'ordre $k$ et le niveau critique $a$ sont dérivés de la source partagée fraîchement rejouée; le demandeur fournit seulement le nuage, le shell critique et deux budgets fiables indépendants. Une unique source Gamma est construite pour chaque classe de label terminal, puis chaque classe et chaque bras sont projetés vers l'indice de composante strictement antérieure correspondant. Le résultat conserve séparément l'identité des labels, la provenance des points retirés et leur regroupement Gamma.

La preuve ouverte repose sur la compacité : tout chemin fini de $\Gamma_k^{<a}$ possède un maximum $b<a$ et relève du théorème 2 fermé à $b$; tout arc compact de $\left\lbrace D_k<a\right\rbrace$ possède également un maximum strict $b<a$ et relève du même théorème. La fixture de raccord à cinq points, de shell canonique $[0,2,3]$ et de niveau $169/36$, ferme trois terminaux $[0,1]$, $[0,3]$, $[2,4]$ classés dans les composantes $(0,1,0)$. Elle distingue ainsi l'égalité de label de la connectivité Gamma : les premier et troisième labels restent distincts mais se regroupent dans la même composante.

Une famille 6.7 incomplète retourne une décision sans calcul Gamma. Le budget Gamma minimal de cette fixture est $(10,10,20)$; le budget $(9,10,20)$ échoue au préflight tout-ou-rien, sans catalogue, classification ou projection partielle. Le rejeu 6.9 reconstruit les deux sous-certificats, l'ordre, le niveau, les projections, les regroupements, les faits, les compteurs, la décision et la portée depuis les entrées externes. Une seconde fixture canonique à cinq points, d'ordre trois et de niveau $25925/338$, ferme trois bras en deux classes : les retraits un et trois partagent le terminal $[0,1,2]$ et sa provenance $[1,3]$, tandis que le retrait deux atteint $[0,1,3]$. Avec le budget Gamma minimal $(10,5,15)$, une seule source représente la classe commune, `same_terminal_label_arm_coalescence_count` vaut un et les trois bras se projettent dans les composantes $(0,1,0)$. La base `exact_complete_critical_arm_family_strict_path_bounded_exhaustive_open_gamma_component_classification_v1` et la portée `bounded_complete_critical_arm_family_to_exhaustive_strict_gamma_components_only` restent événement-locales et bornées. Les événements de même niveau, les plateaux, la racine, la fusion, l'attache publique, le DAG, la forêt, CUDA, G4, la scalabilité et tout `public_status` restent ouverts.

Le contrat 6.10 introduit `ExactGammaTransition*` sur `reference_cpu`. Sous le même préflight borné que 6.8, il rejoue le catalogue strict, sépare exhaustivement les facettes et cofaces satisfaisant $\beta=a$, puis applique toutes les incidences égales sur un état pré-lot figé avant de canoniser les composantes de la coupe fermée. Chaque facette d'une coface égale porte exactement un jeton : l'indice de sa composante stricte si $\beta<a$, ou son label de facette nouvellement active si $\beta=a$.

Après contraction des sous-chemins stricts, les cofaces égales forment un hypergraphe sur ces jetons. Ses composantes sont exactement les composantes de $\Gamma_k^{\leq a}$ touchées au niveau $a$; les composantes strictes non touchées conservent leur indice de projection sans produire de groupe. Chaque groupe conserve ses composantes strictes, ses facettes nouvelles, ses cofaces égales et la catégorie diagnostique $q=0$, $q=1$ ou $q\geq2$. Cette catégorie compte des composantes Gamma strictes, pas des racines publiques.

Le préflight insuffisant reste atomique dans chacune de ses trois dimensions. Le rejeu frais reconstruit la coupe 6.8 embarquée, les catalogues égaux, les témoins taille onze, toutes les incidences, le DSU fermé, la projection et les groupes. Les fixtures couvrent batch vide, groupes $q=0$, $q=1$, $q=2$, deux cofaces simultanées donnant un unique $q=5$, trois groupes déconnectés au même niveau, le bord $n=11$, $k=10$, les plafonds combinatoires $n=14$ et les falsifications de toutes les couches. Un différentiel indépendant reconstruit en Python `Fraction` les deux coupes, leurs catalogues, incidences, projections et groupes sur six cas exacts, sans réutiliser la miniball ni le DSU C++. La base `exact_bounded_exhaustive_gamma_strict_to_closed_equal_level_simultaneous_transition_v1` et la portée `bounded_exhaustive_gamma_equal_level_transition_only` ne ferment ni superposition avec 6.9, ni racine persistante, ni naissance ou fusion réduite, ni attache publique, ni M.1, ni DAG, ni forêt, ni CUDA, G4, scalabilité ou `public_status`.

Le contrat 6.11 introduit `ExactCriticalEventGammaOverlay*` sur `reference_cpu`. Il canonise une à huit requêtes de shells critiques distincts, préflighte avant géométrie leur nombre et au plus 32 bras fournis, puis rejoue séparément 6.9 pour chaque requête. Seules les familles complètes peuvent continuer. L'ordre et le niveau exact sont dérivés de chaque résultat; ils ne sont publiés comme couple commun qu'après accord de toute la liste. Un budget Gamma insuffisant conserve les classifications 6.9 mais ne produit ni transition 6.10, ni projection partielle.

Pour chaque partition critique $S=I\cup U$, les suppressions par $u\in U$ sont réconciliées avec les facettes initiales et les composantes strictes déjà certifiées par 6.9. Les suppressions par $i\in I$ contiennent le support critique $U$ et ont exactement le niveau $a$; elles sont réconciliées avec les facettes nouvellement actives 6.10. La coface $S$ et ses $k+1$ incidences appartiennent à un unique groupe 6.10. Plusieurs événements peuvent donc documenter un même groupe simultané, tandis qu'un événement redondant peut documenter un groupe $q=1$.

La structure de sortie conserve tous les groupes 6.10. Les indices de provenance référencent l'ordre canonique des requêtes; ils sont triés, uniques et chaque événement apparaît exactement une fois dans son groupe. Chaque groupe expose en plus ses cofaces égales sans provenance événementielle fournie : `has_supplied_event_provenance` est existentiel et ne signifie jamais que la liste fournie est exhaustive. Les fixtures ferment le groupe simultané $q=5$, le cas $q=1$, une suppression intérieure, un groupe partiellement documenté, un groupe sans événement fourni, deux groupes tous deux documentés et le bord $k=10$, $n=11$ avec deux suppressions de shell et neuf suppressions intérieures. La base `exact_supplied_complete_critical_arm_gamma_event_cofaces_reconciled_with_exhaustive_equal_level_gamma_transition_v1` et la portée `bounded_supplied_equal_order_level_complete_critical_events_to_exhaustive_gamma_transition_groups_only` excluent encore racines, naissances, fusions publiques, couverture M.1, attache verticale, DAG, pointer-jumping, forêt, CUDA, G4, scalabilité et `public_status`.

Le contrat 6.12 introduit `ExactCriticalCatalog*` sur `reference_cpu`. Pour $1\leq n\leq14$ et $1\leq K_{\max}\leq10$, son préflight réserve avant toute géométrie les $\sum_{j=1}^{\min(4,n)}\binom{n}{j}$ supports canoniques et le pire cas de $n$ classifications ponctuelles par support; les plafonds exacts sont 1470 et 20580. Chaque support est classé comme dépendant, réduit sur sa frontière, extérieur, minimal avec shell supplémentaire pertinent ou hors fenêtre, minimal au-dessus du rang utile, ou événement accepté. Seuls les supports minimaux déclenchent une partition globale fermée.

Une égalité de shell est pertinente lorsque $\lvert I\rvert+\lvert U\rvert\leq s_{\max}$, et non lorsque le rang observé tronqué paraît admissible. Les dégénérescences agrègent canoniquement tous leurs supports, y compris de tailles différentes, puis réécrivent leurs références après tri. Les événements génériques exigent shell global égal au support et rang fermé au plus $s_{\max}$; ils sont triés par niveau, rang, intérieur, shell, support et centre. Les lots H0 groupent ensuite, pour chaque $(k,a)$, les événements de naissance de rang $k$ et de selle de rang $k+1$. Le bord colinéaire de rang onze fournit ainsi une selle d'ordre dix au niveau 25 sans naissance d'ordre onze.

Les fixtures courtes couvrent deux points, triangles aigu, obtus et rectangle, deux événements miroirs au même niveau, la ligne de onze points, le préflight maximal $n=14$, les huit issues exhaustives, l'ordre canonique et les falsifications de chaque couche. Deux contradictions permanentes protègent la logique de généricité : un shell observé de rang trois reste pertinent lorsque son support a rang de pertinence deux; une égalité horizontale hors fenêtre peut coexister avec deux égalités latérales pertinentes, tandis que le déplacement exact du point intérieur à $1/2$ isole réellement la branche hors fenêtre. Une sphère à cinq points vérifie enfin que des supports minimaux de tailles deux et trois restent associés au même enregistrement après canonisation. Le rejeu frais reconstruit tout le catalogue depuis les seules entrées externes. La base `exhaustive_exact_supports_up_to_four_global_closed_ball_critical_catalog_h0_batches_v1` et la portée `bounded_n14_k10_exhaustive_supports_up_to_four_critical_catalog_h0_batches_only` excluent raccord Gamma, racines persistantes, naissance ou fusion publique, attache verticale, M.1, DAG, forêt, CUDA, G4, scalabilité et `public_status`.

Le contrat 6.13 introduit `ExactReducedGammaBatch*` sur `reference_cpu`. Il appelle directement 6.10 pour un nuage canonique, un niveau exact et $2\leq k<n\leq14$ avec $k\leq10$, sous les mêmes trois budgets Gamma. Il ne prend ni la superposition fournie 6.11, ni le catalogue critique 6.12 comme autorité d'incidence : les cofaces non critiques ou non-Gabriel restent indispensables lorsque leurs incidences silencieuses modifient une composante future. L'ordre un, dont le profil réduit coïncide avec `full_pi0`, et l'ordre terminal $k=n$, réduit vide pour $n>1$, relèvent d'autres contrats.

La classification stricte utilise l'équivalence suivante pour $k\geq2$ : une composante de $\Gamma_k^{<a}$ est non triviale si et seulement si elle est incidente à une coface stricte. Elle porte alors exactement une racine réduite antérieure locale; une composante à facette unique reste omise. Un groupe contenant au moins une coface égale compte uniquement ces racines non triviales : zéro produit une naissance, une une continuation et au moins deux une multifusion. Les composantes strictes isolées absorbées sont conservées comme témoins, mais ne deviennent jamais parents. Une facette égale sans coface est différée sans racine ni delta.

Pour chaque groupe non différé, `coverage_delta` soustrait de la composante fermée l'union des couvertures de ses parents, d'abord comme ensemble de facettes puis comme ensemble de points. Le drapeau `fully_redundant` est vrai exactement lorsque les deux différences sont vides; un delta ponctuel vide n'autorise donc pas à omettre une facette nouvellement acquise, et un delta entièrement redondant ne supprime jamais le groupe topologique, notamment une multifusion. Le rejeu frais reconstruit la transition 6.10, toutes les classifications, les groupes, les différences d'ensembles, les faits, les compteurs, la décision et la portée.

Les fixtures courtes ferment une facette isolée différée, le triangle $q_{\Gamma}=2$ et le miroir $q_{\Gamma}=5$ qui restent tous deux des naissances réduites parce que leurs composantes strictes sont isolées, puis le nuage E5. Sur E5, les niveaux $25/16$ et $1105/242$ sont deux naissances, $13/2$ une multifusion binaire et $17/2$ une continuation; les deux derniers lots ajoutent une facette sans ajouter de point. La frontière positive $k=10$, $n=11$ sur la ligne $0,\ldots,10$ au niveau $25$ reste une naissance réduite : les deux facettes strictes isolées sont absorbées avec les neuf facettes égales. La fixture $A=(0,0)$, $B=(4,0)$, $C=(1,3)$, $z=(1,1)$ à $a=5$ ferme une continuation dont l'unique racine stricte couvre déjà les six facettes : les différences de facettes et de points sont vides et `fully_redundant=true`, mais le groupe reste présent. Un niveau sans égalité n'émet aucun groupe. Les trois budgets insuffisants échouent atomiquement, $k=1$ et $k=n$ sont rejetés, et les mutations de la transition, des classifications, groupes, deltas, faits, compteurs, décision, portée, budget et entrées externes échouent au rejeu. La base `exact_bounded_exhaustive_gamma_strict_nontrivial_component_reduction_and_equal_level_batch_semantics_v1` et la portée `bounded_exhaustive_gamma_single_equal_level_hgp_reduced_semantics_orders_two_to_ten_with_k_less_than_n_only` excluent identifiants persistants, raccord catalogue--Gamma, attache verticale, M.1, DAG, pointer-jumping, plateaux, forêt, CUDA, G4, scalabilité et `public_status`.

Le contrat 6.14 introduit `ExactPersistentReducedGammaOrderHistory*` sur `reference_cpu`. Pour $2\leq k<n\leq14$, $k\leq10$, une coupe haute exacte au niveau $2D^2$, où $D^2$ est le diamètre carré du nuage, contient strictement toutes les facettes et cofaces. Leurs niveaux de miniball triés et dédupliqués forment le sweep exhaustif. Le cas $k=n\leq10$ est fermé séparément par un historique complet vide, sans lancer la géométrie; $k=1$ reste sous l'autorité de l'EMST de phase 5.

Le préflight calcule $F=\binom{n}{k}$, $C=\binom{n}{k+1}$, $U=kC$ et la borne $L\leq F+C$ avant toute géométrie. Il réserve les travaux logiques de la coupe haute et des $L$ lots, le journal compact, les nœuds, les références d'enfants, les références de racines antérieures, les activations et les deltas. Les plafonds de portée sont $F,C\leq3432$, $U\leq21021$, $L\leq6435$, au plus 3432 nœuds, 3431 références topologiques de chaque famille, 6435 groupes, 3432 facettes de delta et 24024 références ponctuelles. Les lots 6.13 complets sont construits et vérifiés un par un, puis détruits; leur accumulation quadratique n'appartient pas au payload persistant.

Chaque lot fige la bijection entre racines locales et composantes strictes non triviales, résout tous les groupes avant mutation, puis reconstruit l'état de chaque racine comme union des états antérieurs et du delta exact. Les facettes et points reconstruits doivent coïncider avec la composante fermée 6.13 transitoire. Les naissances et multifusions créent des nœuds denses, les anciennes racines d'une multifusion en sont les enfants, les continuations conservent leur identifiant et les facettes isolées restent différées. Les facettes nouvellement actives et cofaces égales sont journalisées exactement une fois; chaque groupe non différé conserve son delta, y compris vide. L'application de racines, journal, compteurs et couverture globale est atomique après validation du lot entier.

Les régressions couvrent la branche terminale $n=k=2$, un triangle, deux triangles simultanés, une multifusion ternaire symétrique à trois enfants au niveau $13/4$, la continuation silencieuse à cinq points, le delta entièrement redondant à quatre points et E5 sur ses douze niveaux. E5 ferme treize groupes, six différés, deux naissances, quatre continuations, une multifusion et trois nœuds; le delta vide du niveau $85/9$ reste présent. Les budgets juste insuffisants, les maxima complémentaires $k=6$ et $k=7$, un nuage jumeau et les mutations de chaque couche doivent échouer fermés. La base `exact_bounded_exhaustive_gamma_all_exact_levels_persistent_reduced_root_genealogy_v1` et la portée `bounded_n14_k10_single_order_persistent_hgp_reduced_gamma_history_including_empty_terminal_only` ne revendiquent ni persistance durable, ni raccord catalogue--Gamma, ni identifiant SHA public, ni `full_pi0`, ni attache verticale, ni M.1, ni DAG global, ni pointer-jumping, ni plateau, ni CUDA, ni G4, ni scalabilité, ni `public_status`.

Le contrat 6.15 introduit `ExactReducedGammaCut*` sur `reference_cpu`. Il reçoit uniquement un historique 6.14 en mémoire, un niveau exact et la frontière `strict_open` ou `closed`. Après validation de la croissance stricte du catalogue de niveaux, la première frontière emploie `lower_bound` et la seconde `upper_bound`; le curseur est donc dérivé de la requête, jamais fourni par le résultat observé. Tous les groupes d'un niveau sélectionné sont rejoués ensemble, y compris les groupes différés et les deltas entièrement redondants qui peuvent avancer le curseur sans modifier les racines.

Un audit global de forme précède la sélection et expose séparément ses comptes de niveaux, métadonnées, nœuds, groupes, labels, références et dry-replay scalaire; son scratch statiquement borné reste hors du budget propre à la coupe. Le préflight du préfixe calcule ensuite sans payload de facettes ses tailles exactes et les bornes sûres de sortie. Avec $Q_p$ groupes non différés, $A_p$ racines finales, $N_p$ nœuds, $R_p$ références de racines antérieures et $H_p$ références d'enfants, il exige $R_p=Q_p-A_p$ et $H_p=N_p-A_p$. Les références de facettes de sortie valent exactement les facettes de delta cumulées; les références ponctuelles sont préflightées par $\min(nA_p,kD_{F,p})$. Le travail de reconstruction compte séparément les facettes relues dans les racines antérieures et les deltas : il est borné par $(F+C)F\leq22084920$, et son scan de `PointId` par 154594440. Les incidences nouvellement-active--delta--racine et les $k+1$ suppressions de chaque coface ajoutent deux capacités bornées par 27456 facettes et 192192 références ponctuelles. Les mutations d'un lot sont préparées sur snapshot puis déplacées atomiquement sans cloner la table ou les états; plages d'identifiants créés, préfixe de nœuds et représentants minimaux restent déterministes.

La preuve est relative au journal fourni. Le builder 6.15 ne reçoit pas le nuage et n'appelle ni le vérificateur 6.14, ni Gamma; son gate n'accepte que les déclarations et la structure bornée du journal. Un journal cohérent forgé reste donc indétectable, ce que les faits, décisions et vérifications qualifient explicitement de `journal_relative`. Le modèle d'entrée couvre l'historique retourné par le builder 6.14 et séparément accepté par son vérificateur, pas une désérialisation hostile dont les `BigInt` auraient une taille arbitraire. Le cas terminal demeure vide pour tout seuil. Le différentiel Python compare indépendamment `hgp_reduced` au Gamma direct aux niveaux critiques, entre niveaux et aux deux extrêmes. La base `exact_certified_persistent_reduced_gamma_journal_prefix_cut_replay_v1` et la portée `bounded_n14_k10_single_order_strict_or_closed_hgp_reduced_cut_from_certified_6_14_journal_only` n'ajoutent aucun raccord catalogue--Gamma, identifiant durable ou public, `full_pi0`, attache verticale, M.1, DAG global, forêt multi-ordre, CUDA, G4, scalabilité ou `public_status`.

Le contrat 6.16 introduit `ExactCriticalCatalogReducedGammaOverlay*` sur `reference_cpu` pour $2\leq k<n\leq14$, $k\leq10$. Il reçoit le nuage et un budget composite, préflighte la couche de raccord avant toute géométrie, puis construit et vérifie fraîchement un catalogue 6.12 avec $K_{\max}=k$ et une histoire 6.14 au même ordre. Une extra-shell pertinente arrête le raccord sans histoire ni payload; les échecs de budgets subordonnés restent distincts. Le terminal est exclu parce que son histoire réduite vide ne possède aucun slot pour sa naissance de rang $n$.

Pour chaque événement, la positivité barycentrique de son support prouve que la miniball de son étiquette fermée `closed_point_ids` a exactement son niveau. Un rang $k$ rejoint donc l'unique facette nouvellement active correspondante, et un rang $k+1$ l'unique coface égale. Les slots de l'histoire sont tous indexés avant la lecture du catalogue; l'overlay conserve chaque groupe et chaque slot, avec ou sans provenance. Sous la porte générique, les naissances du catalogue sont exactement les facettes différées. Les selles visent des groupes non différés, mais le choix naissance réduite, continuation ou multifusion reste exclusivement celui de Gamma et de l'histoire; plusieurs selles de niveau égal peuvent rejoindre simultanément un même groupe.

Le préflight ferme $F=\binom{n}{k}$, $C=\binom{n}{k+1}$, $L=F+C\leq6435$, le scan historique $kF+(k+1)C\leq48048$, au plus 1456 projections et références de groupes, et un scan fermé conservateur de 16016 `PointId`. Le résultat stocke les deux sources fraîchement certifiées, une projection par rôle, exactement $F+C$ slots et un overlay par groupe; les résidus restent des slots Gamma sans provenance H0 acceptée. Les identités $P_b+R_f=F$ et $P_s+R_c=C$ sont vérifiées sans requalifier les résidus. Le rejeu reconstruit chaque couche depuis le nuage, l'ordre et les budgets, sans lire les indices observés. La base `exact_critical_closed_label_h0_references_reconciled_with_exhaustive_persistent_reduced_gamma_equality_slots_v1` et la portée `bounded_n14_k10_single_order_freshly_verified_critical_catalog_h0_provenance_to_exhaustive_persistent_reduced_gamma_history_groups_only` n'ajoutent aucun identifiant durable ou public, attache verticale, M.1, DAG global, forêt multi-ordre, CUDA, G4, scalabilité ou `public_status`.

Le contrat 6.17 introduit `ExactCriticalCatalogArmGammaOverlay*` sur `reference_cpu` pour $3\leq n\leq14$, $2\leq k<n$ et $k\leq10$. Il reconstruit et vérifie fraîchement 6.12 avec $K_{\max}=k$, sélectionne exhaustivement chaque référence `saddle_order=k`, puis reconstruit une famille 6.7 par selle avec un budget de chaîne commun. La miniball, le support positif, la partition fermée, l'ordre et le niveau de chaque famille doivent coïncider exactement avec l'événement catalogué; une extra-shell pertinente bloque avant toute famille et toute coupe Gamma.

Le préflight de raccord précède toute géométrie subordonnée. Pour $E=\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}$, $A=4E$, $F=\binom{n}{k}$ et une limite de chaîne $L\leq4096$, ses sept capacités couvrent respectivement $E\leq1456$ selles, $A\leq5824$ bras, $E\leq1456$ lots de selles, $A\leq5824$ composantes-cibles, $EF\leq4996992$ références de facettes, $k(EF+A)=kE(F+4)\leq35025536$ références de `PointId` incluant les représentants canoniques, et $AL\leq23855104$ segments engagés. Une capacité juste insuffisante ne lance ni catalogue, ni famille, ni Gamma. Un échec subordonné conserve seulement son diagnostic certifié : une famille incomplète interdit tout lot Gamma et toute cible; l'insuffisance d'un budget 6.13 interdit tout payload de composantes ou d'incidences, sans overlay partiel.

Lorsque toutes les familles sont complètes, le builder construit et vérifie exactement un résultat 6.13 transitoire par lot H0 contenant au moins une selle. Pour chaque bras $u\in U$ d'une selle $S=I\cup U$, il recherche séparément la facette initiale $F_u=S\setminus\lbrace u\rbrace$ et la facette terminale $T_u$ dans la coupe stricte pré-lot. Les deux recherches doivent désigner la même unique composante exhaustive `full_pi0`, et cette composante doit appartenir à l'unique groupe non différé qui contient la coface fermée $S$. L'appartenance commune au seul groupe ne remplace jamais ce double lookup.

La cible conservée est le témoin complet de composante stricte. Son `reduced_component_kind`, soit `prior_nontrivial_reduced_root` soit `omitted_isolated_facet`, et le `reduced_gamma_group_kind`, soit naissance, continuation ou multifusion, restent des annotations séparées qui ne choisissent pas la cible. Le payload garde les familles 6.7 et leurs chemins, un enregistrement compact par lot, les seules composantes effectivement ciblées dédupliquées par couple lot--composante et exactement une incidence par triple `(catalog_event_index, order, removed_shell_point_id)`; les résultats 6.13 complets ne persistent pas.

Le vérificateur 6.17 reconstruit catalogue, familles, lots transitoires, double lookup, cibles, annotations, faits, compteurs et décision depuis le nuage, l'ordre et le budget composite; aucun indice, terminal, groupe ou statut observé et aucune composante observée ne pilote le rejeu. La base `exact_exhaustive_critical_catalog_index_one_arm_families_reconciled_with_strict_gamma_full_pi0_components_and_separate_reduced_annotations_v1` et la portée `bounded_n14_k10_single_order_fresh_catalog_all_index_one_saddle_arm_families_to_exhaustive_strict_gamma_full_pi0_components_with_reduced_annotations_only` ferment une couture mono-ordre bornée. Elles ne créent aucun `Attachment` public, identifiant durable ou public, certificat M.1, transaction simultanée de forêt, flèche verticale, forêt multi-ordre, traitement de plateau, chemin CUDA/G4, résultat de scalabilité ou `public_status`.

Le contrat 6.18 introduit `ExactCriticalCatalogTypedGammaJournal*` sur `reference_cpu` pour $3\leq n\leq14$, $2\leq k<n$ et $k\leq10$. Il exige avant toute géométrie l'égalité exacte des deux budgets de catalogue 6.12 et de la couture Gamma commune à 6.16 et 6.17, puis construit et vérifie fraîchement l'overlay de provenance 6.16. L'overlay de bras 6.17 ne démarre qu'après ce premier succès complet. Leurs catalogues doivent être identiques; les deux wrappers, les catalogues, les familles 6.7, leurs chaînes et les lots 6.13 sont transitoires. La sortie complète conserve exactement une histoire 6.14, déplacée depuis 6.16, et les seules arènes typées de raccord. Toute décision diagnostique conserve uniquement les décisions subordonnées et des scalaires, sans histoire ni enregistrement du journal.

Avec $F=\binom{n}{k}$, $C=\binom{n}{k+1}$, $E=\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}$ et $A=4E$, les neuf capacités propres, dans l'ordre de l'API, sont $F+C\leq6435$ entrées de labels, $E\leq1456$ selles, $A\leq5824$ classes terminales, $A\leq5824$ bras, $A\leq5824$ cibles strictes, $(k+1)A\leq64064$ références ponctuelles de classes, $2A\leq11648$ références de selles, $EF\leq4996992$ références de facettes cibles et $k(EF+A)\leq35025536$ références ponctuelles cibles représentants canoniques compris. Ces bornes conservatives sont validées avant les producteurs subordonnés; elles ne préjugent ni du nombre effectif de selles ni de la déduplication des cibles.

Chaque slot de label 6.16 devient exactement une entrée portant `catalog_birth`, `catalog_saddle`, `residual_newly_active_facet` ou `residual_equal_level_coface`. Les naissances du catalogue restent des facettes différées sans selle ni bras; les selles sont en bijection avec des cofaces égales non différées et avec les enregistrements de selles 6.18. La jointure selle--famille utilise les indices catalogue et H0, puis défend l'identité par ordre, niveau exact et `closed_point_ids`; elle ne confond jamais l'indice de lot H0, l'indice de batch historique et le lot compact 6.17. Chaque famille, classe et bras source est consommé exactement une fois. Un bras pointe vers une classe terminale puis vers une cible stricte; tous les bras d'une classe partagent cette cible, tandis que des classes ou selles distinctes peuvent partager une cible dédupliquée. La facette initiale est redérivée de l'étiquette fermée et du point de shell retiré.

Le témoin complet `ExactStrictGammaComponentWitness` reste l'unique autorité de cible `full_pi0`. `ExactReducedGammaStrictComponentKind` et `ExactReducedGammaBatchGroupKind` sont recopiés comme deux annotations `hgp_reduced` distinctes du rôle H0; ils ne choisissent jamais la cible et ne créent aucun lien vers un `root_node_id`. Les groupes et niveaux simultanés de l'histoire sont conservés sans séquentialisation. Le vérificateur frais reconstruit les deux sources, l'unique histoire et chaque enregistrement depuis le nuage, l'ordre et le budget fiable, sans laisser un champ observé piloter le rejeu. La base `exact_fresh_catalog_h0_provenance_and_strict_full_pi0_arm_targets_reconciled_through_one_typed_single_order_reduced_gamma_journal_v1` et la portée `bounded_n14_k10_single_order_exhaustive_gamma_groups_typed_catalog_h0_roles_and_strict_full_pi0_arm_targets_with_separate_hgp_reduced_effect_annotations_only` n'ajoutent aucun `Attachment` public, identifiant durable ou public, certificat M.1, lien cible--racine, flèche verticale, transaction de forêt `full_pi0`, forêt multi-ordre, DAG global, pointer-jumping, quotient de plateau, chemin CUDA/G4, résultat de scalabilité ou `public_status`.

Le contrat 6.19 introduit `ExactCriticalCatalogTypedGammaRootOverlay*` sur `reference_cpu` pour $3\leq n\leq14$, $2\leq k<n$ et $k\leq10$. Il reçoit un journal 6.18 externe avec le nuage, l'ordre et un budget propre qui embarque exactement le budget de cette source. Les dix capacités de la nouvelle couche sont validées avant le vérificateur 6.18 et avant toute allocation propre à cette couche proportionnelle au journal. Une source rejetée ou incomplète ne produit aucune liaison; la branche complète conserve uniquement une liaison dense par cible et ne recopie ni le journal ni l'histoire.

Pour chaque lot portant au moins une cible, le sweep fige l'état actif 6.14 puis indexe toutes les facettes de toutes ses racines. Une cible de plusieurs facettes doit posséder un unique candidat et sa famille canonique complète doit être exactement égale à celle de ce candidat; le `root_node_id` doit en outre figurer dans les racines antérieures du groupe historique de la cible. Une cible singleton doit être absente de tout l'index et reçoit la disposition `omitted_isolated_singleton`. L'annotation `ExactReducedGammaStrictComponentKind` n'est comparée qu'après cette classification indépendante. L'index est détruit avant la préparation des groupes, tous les groupes du niveau sont résolus sur le snapshot inchangé, puis leurs mutations sont committées simultanément. Les naissances du lot courant ne peuvent ainsi jamais servir de racines pré-lot.

Avec $F=\binom{n}{k}$, $C=\binom{n}{k+1}$, $L=F+C$, $E=\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}$, $A=4E$ et $R=\left\lfloor\frac{F}{k+1}\right\rfloor$, les dix capacités sont $A\leq5824$ liaisons, $2R\leq858$ états de racines actifs ou préparés, $2F\leq6864$ références de facettes de ces états, $2kF\leq48048$ identifiants contenus dans ces facettes, $LF\leq22084920$ et $kLF\leq154594440$ unités logiques de rejeu, $EF\leq4996992$ et $kEF\leq34978944$ unités logiques de comparaison de cibles, puis $EF\leq4996992$ et $kEF\leq34978944$ unités logiques d'indexation de snapshots. Ces unités portent sur les références sémantiques, non sur chaque opération interne des conteneurs CPU. La borne $R$ vient des $k+1$ facettes minimales de toute composante non triviale; les bornes $EF$ viennent d'au plus $E$ lots ciblés et de composantes strictes disjointes dans chacun. La réservation des mutations est bornée par $R$, celle de leurs familles par $F$, et la comparaison finale ne recopie pas l'état.

Le vérificateur recertifie d'abord la source externe depuis les entrées fiables, puis reconstruit le sweep et chaque liaison sans laisser les dispositions, identifiants ou compteurs observés piloter le rejeu. La base `exact_fresh_typed_full_pi0_target_families_reconciled_with_frozen_pre_batch_local_reduced_gamma_roots_v1` et la portée `bounded_n14_k10_single_order_full_pi0_target_families_to_frozen_pre_batch_local_hgp_reduced_roots_with_explicit_isolated_singletons_only` ferment uniquement le raccord local cible--racine pré-lot. Les identifiants restent locaux à la source recertifiée; aucune surjectivité des racines vers les cibles, aucun `Attachment`, identifiant durable ou public, certificat M.1, morphisme vertical, transaction de forêt `full_pi0`, DAG global, pointer-jumping, quotient de plateau, forêt multi-ordre, chemin CUDA/G4, résultat de scalabilité ou `public_status` n'en découle.

Le contrat 6.20 introduit `ExactCriticalCatalogTypedGammaArmRootComposition*` sur `reference_cpu` dans le même domaine borné. Il reçoit le journal 6.18 et l'overlay 6.19 comme deux sources externes non possédées, plus un budget qui embarque exactement le budget 6.19 et une seule capacité propre de candidats. Toute la hiérarchie des plafonds imbriqués est validée avant le préflight et avant le vérificateur 6.19; les deux coutures de budgets doivent être exactes. Une source rejetée ou incomplète produit un diagnostic sans candidat.

Si $\tau$ associe à chaque bras sa cible 6.18 et $\rho$ associe à chaque cible sa liaison 6.19, la nouvelle arène matérialise $\rho\circ\tau$ exactement une fois par bras. Elle vérifie les références denses du bras, son appartenance à la selle, la chaîne par la classe terminale, l'indice de cible et de liaison, puis l'égalité des coordonnées de lot et de groupe. La cible `full_pi0` reste externe et autoritative; la disposition et l'éventuelle racine locale `hgp_reduced` sont copiées sans reclassification. Deux bras partageant une cible conservent deux candidats distincts.

Avec $E=\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}$ et $A=4E$, la capacité propre est $A\leq5824$. Elle est statiquement cohérente avec les plafonds d'arène des bras 6.18 et des liaisons 6.19. L'arène et les compteurs sont préparés séparément puis committés ensemble; le vérificateur recalcule toute la composition après rejeu frais de la source. La base `exact_fresh_typed_critical_arm_target_indices_composed_with_recertified_target_root_bindings_v1` et la portée `bounded_n14_k10_single_order_event_local_typed_critical_arms_to_strict_full_pi0_targets_and_frozen_pre_batch_local_hgp_reduced_root_or_explicit_omitted_singleton_candidates_only` ne publient aucun chemin, `Attachment`, identifiant durable ou public, certificat M.1, morphisme vertical, transaction de forêt `full_pi0`, DAG global, pointer-jumping, quotient de plateau, forêt multi-ordre, chemin CUDA/G4, résultat de scalabilité ou `public_status`.

Le contrat 6.21 introduit `ExactCriticalCatalogTypedGammaArmRootPathOverlay*` sur `reference_cpu`. Le journal 6.18, l'overlay 6.19 et la composition 6.20 restent externes. Le budget embarque exactement celui de 6.20, valide récursivement ses plafonds, impose trois coutures exactes puis borne cinq arènes propres avant toute géométrie. Une composition rejetée ou incomplète produit un diagnostic vide.

La branche complète recertifie 6.20, reconstruit 6.12 une fois et 6.7 une fois par selle. Elle joint chaque candidat au bras frais par événement et point retiré, exige la même classe terminale, redérive la facette initiale, vérifie les coutures et la stricte sous-niveauté, puis contrôle l'appartenance des facettes initiale et terminale à la cible externe `full_pi0`. Le record compact garde le germe analytique, ses contraintes, les nœuds exacts et les témoins des segments engagés; les partitions globales et miniballs exhaustives restent transitoires. Les chemins ne sont jamais dédupliqués par cible ou racine.

Avec $B\leq4096$ segments 6.5 par bras, les capacités sont $A\leq5824$, $AB\leq23855104$, $A(B+1)\leq23860928$, $kA(B+1)\leq238609280$ et $A(n-k-1)\leq64064$. La base `exact_fresh_event_local_typed_critical_arm_strict_descent_paths_replayed_and_linked_to_full_pi0_targets_with_separate_local_reduced_dispositions_v1` et la portée `bounded_n14_k10_single_order_event_local_typed_critical_arms_with_replayable_strict_descent_paths_linked_to_external_full_pi0_targets_and_separate_frozen_pre_batch_local_hgp_reduced_root_or_omitted_singleton_dispositions_only` restent internes : la racine réduite locale n'est pas l'extrémité géométrique du chemin et aucun `Attachment`, identifiant durable ou public, H5, O3, M.1, morphisme vertical, transaction de forêt `full_pi0`, DAG global, pointer-jumping, quotient de plateau, forêt multi-ordre, chemin CUDA/G4, résultat de scalabilité ou `public_status` n'en découle.

Le contrat 6.22 introduit la primitive réutilisable `morsehgp3d::contract::CanonicalId` et `ExactCriticalCatalogTypedGammaDurableArmKeyCatalog*`. Le budget embarque exactement celui de 6.21, valide récursivement ses plafonds et exige les quatre coutures journal--racine--composition--chemins avant quatre bornes conservatives propres. La branche complète recertifie 6.21, reconstruit un catalogue frais et calcule chaque `CriticalEvent.event_id` v2 depuis l'intérieur, le shell, le support minimal, le centre homogène exact et le niveau carré exact. Le JSON canonique ne contient aucun `schema_version`; SHA-256 utilise le domaine `MorseHGP3D/v2/CriticalEvent/`, et toute égalité de digest est défendue par la comparaison de la projection complète.

Pour chaque événement trié par `event_id`, les tuples `(event_id, order, removed_shell_id)` sont triés, uniques et doivent retirer exactement une fois chaque point du shell frais. Ils sont en bijection avec les chemins 6.21 et les références événement--bras forment une agrégation mono-ordre complète. Avec $E=\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}$ et $A=4E$, les plafonds sont $1456$ événements, $5824$ tuples, $5824$ références d'agrégation et $21840$ références de `PointId`. La base `v2_domain_separated_sha256_critical_event_keys_with_full_projection_collision_checks_and_exhaustive_single_order_arm_identity_tuple_catalog_v1` et la portée `bounded_n14_k10_single_order_v2_critical_event_ids_and_canonical_arm_identity_tuples_from_recertified_internal_replayable_paths_only` ne calculent volontairement aucun `attachment_id` et ne publient ni `Attachment`, ni `target_node_id`, ni `EqualLevelBatch`. H5, O3, M.1, l'indépendance des choix admissibles, les plateaux, la verticalité, la forêt globale et tout `public_status` restent ouverts.

Le contrat 6.23 introduit `ExactMorseGammaPartitionSweep*` sur `reference_cpu` pour $3\leq n\leq14$, $2\leq k<n$ et $k\leq10$. Il reconstruit 6.12 avec $K_{\max}=k$ et n'emploie d'abord que ses minima de rang fermé $k$, ses selles de rang fermé $k+1$ et une famille 6.7 fraîche et complète par selle. Une facette terminale régulière active ferme son propre rang à $k$; son label, son centre rationnel et son niveau exact doivent donc retrouver un unique événement de naissance du catalogue. La stricte décroissance du chemin impose que cette naissance précède strictement la selle. Un terminal absent, ambigu, non régulier ou non strict arrête la branche Morse sans consulter Gamma.

À chaque niveau Morse exact, les racines antérieures sont figées avant toute résolution. Chaque bras vise la racine courante de sa naissance terminale, puis toutes les selles du niveau forment un seul hypergraphe sur ces racines pré-lot. Les composantes de cet hypergraphe sont déterminées avant mutation et comparées à celles obtenues en inversant l'ordre des selles. Une composante à une racine est une continuation; une composante à au moins deux racines crée une unique multifusion dont tous les enfants proviennent du snapshot strict. Les naissances et les groupes résolus sont ensuite committés ensemble. Ni une composante, ni une cible, ni une union Gamma ne participe à cette décision.

Avec $F=\binom{n}{k}$, $C=\binom{n}{k+1}$, $E=\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}$ et $A=4E$, le préflight conservateur borne séparément $E\leq1456$ naissances, selles, lots et groupes, $A\leq5824$ bras et références de racines, $2E-1\leq2911$ nœuds, $2E-2\leq2910$ enfants, $3E\leq4368$ références de lots et $F+C\leq6435$ checkpoints. Ces plafonds sont des capacités de l'oracle borné, pas des revendications de complexité scalable. Les familles 6.7 et leurs chemins restent transitoires; la sortie conserve seulement la généalogie locale nécessaire au sweep.

Gamma ne démarre qu'après la fermeture complète de cette généalogie. Une histoire 6.14 fraîche fournit tous les niveaux d'activation de facettes et de cofaces, y compris ceux qui ne portent aucun événement Morse. À chacun de ces niveaux, une transition 6.10 fraîche sert exclusivement d'oracle postérieur : la projection des facettes de naissance actives doit donner une bijection entre racines Morse et composantes Gamma, séparément à la coupe stricte et à la coupe fermée. Un niveau résiduel sans lot Morse peut ajouter une incidence silencieuse $q=1$, mais il ne peut ni fusionner deux racines Morse ni créer une composante Gamma dépourvue de naissance cataloguée. Toute discordance produit un témoin minimal indiquant niveau, coupe, naissance, racine ou composante fautive; elle ne répare jamais la généalogie en recopiant la décision Gamma.

La base est `exact_catalog_minima_and_strict_arm_terminal_hyperkruskal_partition_sweep_with_posterior_exhaustive_gamma_oracle_v1` et la portée `bounded_n14_k10_single_order_morse_minimum_saddle_partition_sweep_compared_to_exhaustive_gamma_at_every_activation_level_only`. Même un accord sur toutes les fixtures bornées reste un falsificateur logiciel, pas une preuve de O1, O3, O4 ou M.1 et pas une transaction publique `full_pi0`. Une contradiction devient une fixture minimale permanente et met à jour le registre des preuves avant toute optimisation. Historiquement, l'absence de contradiction a arrêté l'empilement des wrappers locaux et ouvert la Phase 7 pour l'oracle cellulaire de Phase 8. Le pivot du 21 juillet 2026 a séparé la Phase 9 en voie directe LBVH sans catalogue de puissance; les identifiants publics, H5, M.1, la verticalité et la forêt multi-ordre demeurent des chantiers ultérieurs distincts.

## Phase 7 — Audit de la primitive de puissance

### Jalon 7.1 — source figée et contrat de falsification borné

L'[audit dédié](validation/PHASE7_PRIMITIVE_AUDIT.md) fige Paragram au commit `cadf96c854d27c8234d5b64749b8998e3d1af7f8`, son gitlink cuBQL au commit `d18c5fa1a5c98665d13484841ae65774da7751e8` et la licence Apache-2.0. L'adaptateur direct est rejeté : la surface amont retourne seulement adjacency, offsets et statuts, tandis que le parcours BVH à pile 64 peut évincer silencieusement une entrée et que ni sommets, ni plans liants, ni incidences ne sont exposés. Un fork minimal reste candidat sans être choisi; une primitive interne demeure le repli.

Avant tout benchmark long, `build_exact_bounded_power_cell_reference` implémente un oracle `reference_cpu` borné à $n\leq8$. Il adopte $\delta_i(y)=\left\Vert y-p_i\right\Vert^2-w_i$, la cellule $\Phi_{ij}(y)=\delta_i(y)-\delta_j(y)\leq0$ dans une boîte dyadique explicite et au plus treize plans par cellule. Ses plafonds conservateurs sont 286 triplets de plans, 286 sommets rationnels et 3718 incidences sommet--plan. Les tests exacts couvrent notamment le cube, les poids signés, les sites confondus, une cellule vide par demi-espaces propres incompatibles et les dimensions deux, un et zéro. Cet oracle distingue la proposition flottante de la décision exacte locale et ne publie aucun statut MorseHGP3D.

### Jalon 7.2 — parcours BVH fail-closed et quarantaine CSR

Une série atomique reproductible part du commit Paragram figé et produit l'arbre candidat `591ed6a3257ecd0be8544ba129e08c081ed4eb80`. Elle remplace l'éviction silencieuse des piles de parcours ordinaires et pondérés par le statut explicite `traversal_stack_overflow=6`, sans renuméroter les statuts historiques, et conserve les branches de marge nulle qui ne sont pas strictement exclues. Une cellule en erreur n'écrit aucune adjacency brute et le compactage ne conserve que les arêtes dont les deux extrémités ont le statut `success`; la symétrisation ne peut donc pas réintroduire une cellule fautive.

Le manifeste de série fixe le patch, sa taille, son SHA-256, ses chemins et chaque arbre Git. Le checker hors réseau valide d'abord la provenance 7.1, applique toute la série dans un worktree temporaire, rejoue ces arbres et contrôle les invariants structurels ainsi qu'une fixture hôte de quarantaine. Ce jalon ne compile pas encore CUDA, ne fournit ni boîte explicite de l'appelant, ni géométrie de cellule et ne change aucun statut public. Le jalon 7.3 mesure d'abord l'ampleur de ces deux coutures restantes : si elles cessent d'être locales, la voie saine est la primitive interne plutôt que l'accumulation de patchs amont.

### Jalon 7.3 — boîte fermée binary32 explicite

Le second patch de la série impose `bounds` comme argument keyword-only des deux API Python et comme tensor CPU `float32` de forme `(2,3)` à la frontière native. Les six valeurs doivent être finies et chaque borne basse strictement inférieure à la borne haute. Les validations Python et C++ précèdent toute activité GPU; le binding natif vérifie aussi les devices, types et formes des autres tenseurs avant leur contiguïté, puis un `CUDAGuard` fixe et restaure le device des points.

Les six plans initiaux utilisent bit pour bit les coordonnées de l'appelant. La boîte dérivée de la racine BVH, le padding `1.0f`, `CUBE_EPSILON` et le rayon initial fixe `1e10` disparaissent; chaque rayon initial vient des bornes de la cellule après `CCInit`. La série à deux patchs aboutit à l'arbre candidat `732b1c2f9f42aa452a3282483ec9a8d947000497`, et 18 tests CPU valident l'interface en moins de trois secondes.

Ce résultat reste une proposition : l'inclusion des sites dans la boîte n'est pas vérifiée, les extrêmes finis ne sont qualifiés qu'à l'entrée, les rayons binary32 ne sont pas encore des majorants à arrondi dirigé, le stream PyTorch courant et les fautes CUDA asynchrones restent ouverts, et aucune géométrie n'est exportée. Le jalon suivant chiffre donc l'export des plans, sommets et incidences avant tout nouveau patch profond. Si cet export ou la correction runtime traverse cuBQL, la feuille de route choisit la primitive interne.

### Jalon 7.4 — fermeture sémantique bornée et choix de la primitive interne

Pour une cellule propriétaire $i$, soit $K$ la table complète des concurrents et $J\subseteq K$ une amorce authentifiée, par exemple les identifiants d'une ligne CSR symétrisée dont le statut amont vaut zéro. On reconstruit exactement $H_i(J)=\Omega\cap\bigcap_{j\in J}\left\lbrace y:\Phi_{ij}(y)\leq0\right\rbrace$. Puisque la cellule complète $C_i$ emploie toutes les contraintes de $K$, on a $C_i\subseteq H_i(J)$. Si $H_i(J)$ est vide, $C_i$ l'est aussi. Sinon, la compacité de la boîte implique qu'une contrainte affine omise est satisfaite sur tout $H_i(J)$ si et seulement si elle l'est sur tous ses sommets exacts.

`certify_exact_bounded_power_cell_subset_closure` implémente ce contrôle sur `reference_cpu` pour $n\leq8$. Les identifiants de $J$ sont triés, uniques et authentifiés contre $K$ avant toute géométrie; chaque forme est recalculée depuis les sites et poids dyadiques. Une valeur strictement positive produit `violating_halfspace`; une valeur nulle d'une contrainte propre produit `missing_active_incidence`, car égalité active, facette et adjacency sont des notions distinctes; une constante positive produit `competitor_dominates`, tandis qu'un tie coïncident est seulement compté. Le résultat retourne simultanément tous les identifiants à ajouter et conserve le premier témoin canonique. Toute contrainte propre laissée de côté est strictement négative sur $H_i(J)$, donc le reste après l'unique reconstruction : un seul lot de réparation suffit pour fermer la géométrie et ses incidences propres actives.

Avec $f=\lvert K\rvert\leq7$, $c=\lvert J\rvert$ et $T_c=\binom{6+c}{3}$, le scan omis est borné conservativement par $(f-c)T_c\leq360$. Les budgets du polyèdre candidat restent ceux de 7.1 et une insuffisance ne publie aucune géométrie. Les tests courts couvrent violation, égalité avec une face de boîte, réparation simultanée, cellule de dimension basse, constantes coïncidentes, cellule vide, permutations, identifiants falsifiés et les deux préflights juste insuffisants.

L'audit d'architecture ferme en parallèle la bifurcation 7.3. L'export Paragram serait local mais recopierait un scratch de chunk non durable dont chaque sommet ne garde que trois plans; il ne fournirait toujours pas les incidences dégénérées complètes. Surtout, le stream courant, les erreurs et la propriété des allocations traversent `pwr_bvh.cu`, les appels CUDA bruts et `cuBQL::gpuBuilder`; les guesses ne valident pas encore leurs valeurs et Paragram ne clippe pas les parents génériques requis par les phases 8–9. Ce seuil déclenche le repli prévu : la série à deux patchs est gelée comme comparateur et source optionnelle d'identifiants, tandis qu'une primitive H-polytope interne est sélectionnée pour le produit. La porte de sortie de la phase 7 reste fermée jusqu'à son implémentation CUDA et sa qualification courte.

### Jalon 7.5 — matérialisation atomique de la réparation

`repair_exact_bounded_power_cell_subset_closure` compose le décideur 7.4 sans en modifier le contrat. Avant la première construction, il valide les deux budgets imbriqués et réserve le coût d'une cellule finale utilisant tous les concurrents dès que $K\setminus J$ est non vide. Une insuffisance retourne donc les exigences et l'amorce canonique, mais aucune cellule initiale ou réparée. Si 7.4 décide l'amorce complète ou vide, sa première cellule est finale. Sinon tous les identifiants requis sont ajoutés simultanément, puis la cellule est reconstruite exactement une fois; aucun second scan n'est effectué.

Avec $T_k=\binom{6+k}{3}$, une branche réparée vérifie $c\leq6$ et $f\leq7$. Elle effectue au plus deux constructions, $T_c+T_f\leq220+286=506$ triplets et sommets conservatifs, et $(6+c)T_c+(6+f)T_f\leq2640+3718=6358$ incidences conservatives. Le scan unique reste borné par 360 évaluations. Lorsqu'elles sont omises, les constantes `owner_dominates` et les ties coïncidents restent seulement audités; un `competitor_dominates` omis est ajouté et rend la reconstruction vide. Un tel identifiant déjà présent dans $J$ prouve au contraire l'amorce vide sans seconde construction. Le résultat distingue amorce déjà complète, amorce déjà vide, réparation non vide, réparation vide et budget insuffisant. Les identifiants fermés demeurent une sur-amorce sémantique sûre, jamais une revendication d'adjacency exacte.

Ce jalon ferme la composition hôte bornée, pas la Phase 7 : la primitive H-polytope générique, son chemin CUDA, le faux lanceur, NVCC, G4 et la fermeture d'un diagramme global restent à livrer. Aucun `public_status` n'est modifié.

### Jalon 7.6 — oracle H-polytope générique borné

Le backend `reference_cpu` expose `build_exact_bounded_h_polytope_reference` pour une boîte dyadique explicite et une liste arbitraire de formes affines orientées $h_a\leq0$. Chaque contrainte porte un identifiant composite canonique — domaine et deux mots — ainsi que le rôle `parent_constraint` ou `new_clip`. Deux identifiants distincts restent deux incidences sémantiques même s'ils définissent le même plan. Les six faces artificielles de la boîte conservent des kinds séparés. Les constantes négatives, positives et identiquement nulles deviennent respectivement `redundant_strict`, `infeasible` et `identically_active`; seules les formes propres créent un plan frontière.

Après validation des caps, de la boîte, des enums et de l'unicité des identifiants, le cœur trie les contraintes, énumère tous les triplets de plans propres, teste exactement leur faisabilité, déduplique les sommets rationnels, reconstruit toutes les incidences et calcule la dimension affine de zéro à trois. Le budget distingue triplets, évaluations de faisabilité, sommets uniques et tests d'incidence finale. Une insuffisance ne publie aucune contrainte classifiée, frontière ou géométrie. `power_cell_reference` devient un adaptateur de ce cœur sans changer ses API, budgets, audits ni résultats 7.1–7.5.

Le cap hôte couvre un morceau restreint complet sur le domaine oracle $n\leq14$. Pour un parent d'intérieur $I$, le parent fournit au plus $m(14-m)$ contraintes croisées et le diagramme restreint ajoute au plus $14-m-1$ clips, donc $(m+1)(14-m)-1\leq55$ contraintes sémantiques. Avec la boîte, $B\leq61$, d'où $\binom{61}{3}=35990$ triplets et sommets conservatifs, puis $61\times35990=2195390$ tests conservatifs de faisabilité ou d'incidence. Ces plafonds volontairement non serrés dimensionnent un falsificateur; ils ne deviennent jamais un plafond silencieux du produit.

Le certificat porte seulement sur l'intersection de la H-représentation fournie. Il ne prouve ni que le parent est complet, ni `canonical_children_complete`, ni `RelevantGP`, ni un diagramme global. La Phase 7 reste `ready` et la Phase 8 reste bloquée.

### Jalon 7.7 — transcript batché de triplets proposé

Le backend hôte `fake_gpu` fige la frontière du futur kernel sans lui déléguer de géométrie. Un contexte lié à une boîte commune reçoit un CSR exact — identifiants de cellules, offsets et demi-espaces avec IDs composites et rôles — puis canonise les cellules par identifiant. Tous les budgets 7.6 et toute l'arithmétique des offsets sont préflightés avant géométrie ou lancement. La capacité physique porte des records de proposition et ne crée aucun plafond global arbitraire du nombre de cellules; une cellule qui ne tient pas entière bascule vers le cœur exact avec une ligne GPU vide.

Pour les $B\leq61$ frontières propres d'une cellule, l'ordre local reste les six faces de boîte puis les contraintes triées par ID. Le transcript réussi contient exactement $T(B)=\binom{B}{3}$ slots, un par triplet $i<j<k$, avec epoch courant. Un slot vaut `unknown_requires_cpu_exact`, `proposed_strict_reject` avec une frontière témoin, ou `proposed_survivor` avec trois intervalles binary64 finis et un masque `could_be_active`. Ce masque est seulement un sur-ensemble : toute incidence exacte doit y figurer, mais un faux positif reste une proposition licite. Un déterminant flottant contenant zéro ne prouve jamais la singularité et impose `unknown_requires_cpu_exact`.

L'hôte reconstruit dans tous les cas la décision locale depuis les formes exactes avec 7.6. Une constante strictement positive, qui prouve la vacuité sans engendrer de plan, force avant lancement un fallback d'intervalle à ligne vide. Pour les autres cellules, l'hôte rejoue chaque ordinal : un rejet exige une intersection exacte unique et une violation strictement positive de sa frontière témoin; un survivant exige une intersection exacte unique et faisable contenue dans les intervalles; toute incidence exacte doit appartenir au masque proposé. Taille, offsets, ordinals, avance d'exactement une epoch, sentinelles et queue de capacité sont vérifiés avant publication. Triplet manquant, incidence exacte omise, bit hors domaine, non-fini, faux rejet ou faute asynchrone invalident toute la transaction et empoisonnent le contexte. Les fallbacks de capacité, d'intervalle ou de projection restent au contraire des résultats exacts CPU sans payload GPU pour la cellule.

Ce jalon n'ouvre aucune TU `.cu`, ne compile pas NVCC et ne qualifie pas G4. Le transcript reste `proposal_only`; seul le résultat `reference_cpu` porte `complete_nonempty` ou `complete_empty`, localement et sans `public_status`. La Phase 7 reste `ready` et la Phase 8 reste bloquée.

### Jalon 7.8 — proposition CUDA AOT conservative

La TU interne `phase7_h_polytope_proposal.cu` matérialise l'ABI 7.7 sans changer sa sémantique. Le contexte possède un stream non bloquant et des buffers persistants; avant chaque kernel, toute la capacité physique des records est remise à zéro. Le plan hôte affecte seulement des lignes complètes dans l'ordre canonique, puis chaque thread écrit directement le slot de son ordinal combinadique, sans compteur atomique d'émission. Les cellules hors projection, à constante positive ou hors capacité gardent une ligne vide.

Les coefficients binary64 sont des enclosures dirigées des rationnels. Addition, soustraction et multiplication réutilisent la primitive d'intervalles 2B; la division évalue les quatre couples d'extrémités avec arrondis inférieur et supérieur. Le déterminant de Cramer contenant zéro, une opération non finie ou une faisabilité indécidable donnent `unknown_requires_cpu_exact`. Seule une borne inférieure strictement positive propose un rejet; un survivant exige une borne supérieure non positive pour chaque frontière. Le masque conserve tous les intervalles contenant zéro et les trois plans générateurs, puis l'hôte applique encore le rejeu exact 7.7.

Le target AOT et un exécutable analytique court sont câblés dans les presets CUDA release et audit. Le checker statique interdit fast math, FMA, tolérances, stream par défaut, émission atomique et verdict exact GPU; il vérifie aussi CUDA 12.9, `sm_120`, les arrondis dirigés, la queue nulle et l'ordre synchronisation–epoch. Une première compilation `cuda-release` gardée sur G4 a atteint NVCC 12.9.86 puis échoué fermée avant tout exécutable : les quatre `std::array` du POD interne appelaient un `operator[]` hôte depuis le device. L'ABI de transfert emploie désormais des tableaux natifs `std::uint64_t[N]`, avec tailles et offsets vérifiés, et le checker rejette toute régression vers `std::array`.

Le workflow gardé possède maintenant un compagnon 7.8 dédié. Sur un même SHA propre, il construit release et audit, exécute le binaire analytique release, exige uniquement un ELF `sm_120` sans PTX, puis lance `memcheck` et `racecheck`. L'assembleur distingue explicitement `proposal_only_exhaustive_plane_triple_transcript` de `reference_cpu_exact_all_constraints`, ferme les comptes, epochs, empreintes et journaux, et garde l'artefact provisoire jusqu'à la certification ciblée `TERMINATED`. La qualification réelle du SHA `39670649e1af1b999c5be7d580650a2792a09008` ferme cette route : les deux builds CUDA 12.9 passent, l'ELF ne contient que `sm_120` et aucune PTX, l'exécution analytique produit 55 records sur quatre cellules puis les recertifie exactement sur CPU, les deux epochs sont déterministes, et `memcheck` comme `racecheck` passent. L'artefact `phase7-h-polytope-39670649e1af1b999c5be7d580650a2792a09008.json` porte le SHA-256 `7894bc6bd7dbce3bddb1f5405d345d8f8ffe321d0be2949908355dfe130cabca` et la cible exacte a été relue `TERMINATED`. Cette preuve qualifie le backend de proposition, pas un diagramme global : la Phase 7 est `completed`, la Phase 8 devient `ready` et aucun statut public n'est promu.

### But

Choisir la primitive GPU sans lui déléguer la certification.

### Travaux

- pinner le commit de Paragram indiqué dans le corpus bibliographique;
- reproduire ses benchmarks de cellules ordinaires et pondérées;
- tester les conventions de poids par cas analytiques;
- compiler avec et sans fast math;
- mesurer les statuts d'overflow et le chunking;
- exposer ou reconstruire sommets et plans liants;
- comparer avec une primitive minimale maison;
- documenter la licence et les modifications.

### Jeux

- cube avec sites symétriques;
- cellules non bornées coupées par boîte;
- poids positifs et négatifs;
- sites presque confondus;
- cellules à grand nombre de faces;
- jusqu'à plusieurs millions de cellules ordinaires pour le débit.

### Décision de sortie

Choisir entre : adaptateur amont, fork minimal épinglé ou primitive interne. Le choix repose sur les sommets accessibles, la maîtrise de l'overflow, le coût sans fast math et la facilité de rejouer les cellules. Aucun statut exact à cette phase.

## Phase 8 — Raffinement ancré $0\to1$

### Porte d'entrée

Satisfaite par les Phases 1, 4 et 7 fermées. La Phase 7 a sélectionné puis qualifié la primitive H-polytope interne comme backend de proposition; la Phase 4 fournit l'oracle spatial exact et la Phase 1 l'oracle exhaustif de comparaison. La Phase 8 démarre en `ready` avec `backend=reference_cpu`, `profile=generic_core` et `mode=certified`; le chemin `cuda_g4` reste une accélération de proposition recertifiée et ne décide jamais seul la fermeture d'une cellule.

### But

Fermer exactement le diagramme ordinaire dans la boîte $\Omega$ et éprouver le lemme de séparation aux sommets.

### Jalon 8.1 — boîte dyadique strictement paddée

Le backend `reference_cpu` expose désormais `build_exact_point_cloud_aabb`, `build_strictly_padded_dyadic_aabb` et leur vérificateur frais. Le premier scan calcule exactement les six extrema binary64 du nuage canonique, conserve le plus petit `PointId` parmi les témoins ex æquo et publie les comptes fermés $3n$ évaluations de coordonnées et $6(n-1)$ comparaisons d'extrema. Le LBVH consomme cette même primitive puis exige que ses témoins racine reconstruits par l'arbre coïncident; il n'existe donc plus deux scans globaux concurrents.

Pour chaque axe $d$, si $a_d=\min_i x_{i,d}$ et $b_d=\max_i x_{i,d}$, le constructeur prend les voisins binary64 finis immédiats $\ell_d=\mathrm{pred64}(a_d)$ et $u_d=\mathrm{succ64}(b_d)$. Ces voisins sont calculés uniquement sur les mots IEEE, sans opération flottante, `nextafter`, epsilon, mode d'arrondi, FTZ ni DAZ. Les six marges exactes $a_d-\ell_d$ et $u_d-b_d$ sont enregistrées comme rationnels strictement positifs dans le schéma `morsehgp3d.phase8.strictly_padded_dyadic_aabb.v1`.

Si un minimum vaut le plus grand binary64 fini négatif ou si un maximum vaut le plus grand binary64 fini positif, le voisin extérieur serait infini. Le résultat devient alors `unsupported_finite_binary64_range`, rapporte simultanément tous les axes concernés et ne publie ni certificat ni boîte partielle. Aucun clamp n'est permis.

Le vérificateur reconstruit les extrema, témoins, voisins, marges et comptes depuis le nuage, puis rescane toutes les inégalités strictes. Comme chaque site vérifie $\ell_d<x_{i,d}<u_d$ et que l'intérieur d'une boîte est convexe, il certifie $X\subset\mathrm{int}(\Omega)$ puis $\mathrm{conv}(X)\subset\mathrm{int}(\Omega)$ sans hypothèse de dimension affine. Les singletons, axes constants et nuages colinéaires ou coplanaires sont donc couverts. Une boîte réussie initialise aussi le H-polytope de base à huit sommets et six faces artificielles distinctes.

Les tests ciblés GCC et Clang stricts couvrent zéro signé, sous-normaux, frontière de binade, extrema ex æquo, permutation, mots voisins des deux limites finies, échec simultané sur deux faces, mutations du certificat, mauvais nuage, entrée déplacée, accord LBVH et base $C_0$. Ce jalon ne ferme encore aucune cellule ordinaire, ne construit aucun `CatalogCertificate` et ne change aucun statut public. GCP n'est pas utilisé.

### Jalon 8.2 — fermeture monotone d'une cellule ordinaire bornée

Le backend `reference_cpu` expose désormais `build_exact_bounded_ordinary_cell_closure` et son vérificateur frais pour une cellule de Voronoï ordinaire unique sur un nuage canonique de un à huit sites. La cellule candidate $H_i(J)$ est reconstruite depuis la boîte 8.1 et les concurrents authentiques de l'amorce; une amorce vide reçoit le plus petit `PointId` extérieur lorsque celui-ci existe, avec provenance séparée de l'amorce demandée et de l'amorce effective.

À chaque ronde, le 1-NN exact global est interrogé à tous les sommets rationnels, sans exclusion. Le transcript conserve le shell complet $S(z)$ et jamais seulement son représentant canonique. Le lot gelé $A_t=(\bigcup_{z} S(z))\setminus(J_t\cup\left\lbrace i\right\rbrace)$ est trié, dédupliqué et ajouté simultanément après le scan entier. L'absence du propriétaire dans le shell signale une violation stricte; sa présence avec un concurrent omis signale une égalité active manquante. Toute ronde non terminale agrandit donc strictement $J_t$ et la boucle termine après un nombre fini de sites.

Une file vide certifie simultanément l'absence de violation et la réconciliation de toutes les égalités actives. En effet, la différence de deux distances carrées est affine : si une contrainte omise avait un maximum positif ou nul sur le polytope final, ce maximum serait atteint à un sommet et son concurrent apparaîtrait dans un shell requis. Le propriétaire est strictement dans la boîte et satisfait strictement chaque bisecteur d'un site canonique distinct; la cellule ordinaire est donc non vide et tridimensionnelle. Le vérificateur reconstruit tout le transcript, contrôle la croissance monotone, recalcule les shells, puis compare sommets et incidences actives à une cellule oracle bâtie avec tous les concurrents; les plans strictement redondants de la sur-amorce ne sont pas confondus avec des adjacences.

Le préflight borné `bounded_n8_single_ordinary_cell_only` réserve le pire chemin après germe : au plus 7 constructions, 966 triplets et sommets conservatifs, 10822 incidences conservatives, 966 requêtes, 7728 distances exactes et entrées de shell, 7 tests de faisabilité stricte et 6 ajouts. Toute capacité juste insuffisante renvoie `insufficient_budget` sans cellule ni ronde; une capacité supérieure au plafond de confiance est rejetée. Les fixtures courtes ferment singleton, germe de repli, violation issue d'une mauvaise amorce, trois révélations successives, concurrent strictement redondant omis, égalité tangentielle, shells de face/arête/sommet, permutation, budgets et mutations sous GCC et Clang stricts.

Ce jalon est local : il ne construit pas toutes les cellules, ne réconcilie pas encore les incidences réciproques entre propriétaires, n'extrait aucun événement et ne ferme ni la Phase 8 ni `closed_parent_orders[1]`. GCP n'est pas utilisé.

### Jalon 8.3 — diagramme ordinaire borné et strates réciproques

Le contrat `morsehgp3d.phase8.exact_bounded_ordinary_diagram_closure.v1` applique transactionnellement 8.2 à chaque propriétaire d'un nuage canonique de un à huit sites, avec amorce vide et germe extérieur déterministe. Un manifeste des mots binary64 canoniques lie aussi les décisions `insufficient_budget` au nuage exact, même lorsque deux nuages partagent la même boîte. Aucun payload de cellule, sommet ou contact n'est publié si un seul plafond global manque.

Les sommets finaux sont fusionnés par position rationnelle. Pour tout sommet global $v$, les occurrences de cellules doivent être en bijection exacte avec son shell co-1-NN complet $N(v)$; cette identité réconcilie tous les propriétaires sans déduire une adjacency d'un plan redondant. Pour chaque sous-ensemble $Q$ d'au moins deux sites, le contact commun est $K_Q=\bigcap_{i\in Q}C_i=\left\lbrace y\in\Omega:Q\subseteq N(y)\right\rbrace$, et ses sommets sont exactement les sommets globaux dont le shell contient $Q$.

Le barycentre rationnel positif de ces sommets appartient à l'intérieur relatif de $K_Q$. Son shell frais $S_Q$ est aussi l'intersection des shells aux sommets. Le contact devient une strate canonique seulement lorsque $Q=S_Q$; sinon il reste `noncanonical_quotient_contact`. Cette règle transforme le carré cocirculaire en une seule arête de shell quatre et le cube cosphérique en un seul sommet de shell huit, sans faces diagonales ni triangulation arbitraire. Une strate canonique non portée par la boîte vérifie $\dim(K_Q)+\dim_{\mathrm{aff}}(S_Q)=3$ : rang un pour une face, deux pour une arête et trois pour un sommet. Le ET des masques artificiels distingue les contacts entièrement portés par $\partial\Omega$, qui restent `box_supported_contact` et ne deviennent jamais événements.

Au plafond $n=8$, le préflight couvre 8 cellules, 56 constructions, 7728 triplets, sommets conservatifs et requêtes locales, 86576 incidences conservatives, 61824 distances et entrées de shell, 48 lots non terminaux, 48 ajouts au total et une taille de lot au plus 6. La projection finale réserve 2288 occurrences, 247 contacts, 565136 références contact–sommet, puis 247 requêtes de barycentre et 1976 distances. Les 21 plafonds sont testés juste en dessous et juste au-dessus de leur confiance.

Les fixtures courtes ferment diagrammes singleton, collinéaire sparse, paire, triangle, tétraèdre, carré cocirculaire et cube cosphérique; elles séparent aussi une arête naturelle d'un shell trois entièrement porté par une face de boîte. GCC et Clang stricts passent. Ce différentiel utilise encore les mêmes primitives rationnelles internes que le producteur : le jalon certifie `bounded_n8` et les incidences réciproques, mais ne ferme ni la Phase 8, ni `closed_parent_orders[1]`, ni un `CatalogCertificate` ou un statut public. GCP n'est pas utilisé.

### Jalon 8.4 — oracle différentiel par atlas affine

La dette d'indépendance de 8.3 est fermée par un oracle `reference_python` fondé uniquement sur les mots binary64 canoniques, `fractions.Fraction` et les six faces de $\Omega$. Le module de référence n'importe aucun code de production. Le dump C++ appelle 8.3, exige son rejeu certifié, puis expose seulement une projection sémantique; il n'entre jamais dans le calcul de l'oracle.

Pour chaque sous-ensemble non vide $Q$, l'oracle choisit $q_0=\min Q$, pose $\Delta_{q_0p}(z)=\left\Vert z-x_{q_0}\right\Vert^2-\left\Vert z-x_p\right\Vert^2$ et construit directement $K_Q=\left\lbrace z\in\Omega:\Delta_{q_0q}(z)=0\ \forall q\in Q,\ \Delta_{q_0p}(z)\leq0\ \forall p\notin Q\right\rbrace$. Une RREF rationnelle indépendante produit soit une incohérence, soit une paramétrisation $z=z_0+Bt$ de dimension $d\leq3$. L'oracle tire les inégalités dans cet espace, énumère tous les $d$-uplets de frontières indépendantes, résout exactement, rejoue toutes les formes et déduplique les sommets.

La boîte rend chaque $K_Q$ compact. S'il est non vide, il possède un sommet, et tout sommet possède $d$ normales actives indépendantes; l'énumération est donc complète sans boucle convergente. Les singletons reconstruisent les cellules et leurs occurrences. Les sous-ensembles de cardinal au moins deux reconstruisent les contacts directement, sans employer l'identité sommets-shells de 8.3; cette identité devient au contraire un invariant différentiel à vérifier. Le shell frais au barycentre, le rang affine, la dimension et le masque commun donnent ensuite carrier, quotient, strate naturelle ou support artificiel.

Au plafond $n=8$, le préflight indépendant couvre 255 sous-ensembles, 769 lignes d'égalité, 2546 inégalités, 13349 systèmes candidats, 142982 rejeux de formes et 108768 évaluations de distances conservatives. Il réserve aussi au plus 2288 références de cellule et sommets globaux, 11061 références contact–sommet, 18304 entrées de shell globales, 247 contacts et témoins, 1016 identifiants de requête et 1976 identifiants de carrier. Une insuffisance ne publie aucun objet géométrique et une capacité au-dessus de la confiance est refusée.

Le différentiel compare par valeurs rationnelles, indépendamment de l'ordre et des indices : sommets de chaque cellule, sommets globaux, shells, propriétaires, masques, contacts, carriers, rangs, dimensions, témoins et kinds. La matrice courte couvre chaque cardinal de un à huit, zéros signés et sous-normaux, paire oblique, collinéarité, support boîte, carré exact puis perturbé d'un ULP, tétraèdre avec et sans site central, courbe des moments, shell sept, cube cosphérique, cube perturbé et permutations. Toute contradiction doit devenir une fixture permanente avant de poursuivre.

Ce jalon contrôle la projection topologique exacte de 8.3 dans `bounded_n8`; il ne recertifie pas ses rondes, amorces, compteurs procéduraux ou budgets internes. Il ne ferme ni extraction Morse, ni `RelevantGP`, ni catalogue, ni hiérarchie, ni `closed_parent_orders[1]`, ni statut public. GCP n'est pas utilisé.

### Jalon 8.5 — supports naturels de profondeur zéro

Le contrat `morsehgp3d.phase8.exact_bounded_depth_zero_natural_supports.v1` consomme un diagramme 8.3 complet et fraîchement vérifié, puis énumère les sous-supports de deux à quatre sites de chaque carrier `natural_face`, `natural_edge` ou `natural_vertex`. Les contacts `noncanonical_quotient_contact` et `box_supported_contact` ne sont jamais des sources d'émission. Le producteur ne dépend pas du catalogue critique exhaustif : ce dernier reste exclusivement un oracle de test borné.

La complétude utile vient du centre lui-même. Si un support minimal $U$, avec $2\leq\lvert U\rvert\leq4$, a un intérieur strict vide et un shell global complet $S$, son centre appartient à $\mathrm{conv}(U)$, donc à l'intérieur de la boîte, ainsi qu'à $K_S$. Le contact de carrier $S$ est donc non vide, canonique et non porté par une face commune de boîte; c'est une strate naturelle, et l'énumération de tous ses sous-supports retrouve nécessairement $U$. Cette preuve couvre tous les supports critiques de profondeur zéro dans la portée bornée, pas tous les supports rejetés possibles et pas les profondeurs positives.

Chaque proposition dédupliquée recalcule exactement son centre circonscrit, son niveau et ses barycentriques avant une nouvelle partition globale de boule fermée. La priorité est fixe : dépendance affine, centre sur frontière ou extérieur; puis, pour un support minimal, intérieur strict non vide différé; ensuite extra-shell $S\neq U$ diagnostiqué selon le rang de pertinence $\lvert I\rvert+\lvert U\rvert$; enfin support au-dessus de la fenêtre ou support accepté. À profondeur zéro, $I=\varnothing$ avant le test extra-shell, donc la pertinence dépend de $\lvert U\rvert$, jamais du rang fermé observé $\lvert S\rvert$. Un extra-shell pertinent reste bloquant même lorsque son shell complet dépasse $s_{\max}$.

Un support accepté vérifie $S=U$ et $\lvert U\rvert\leq s_{\max}$, puis une réciprocité exacte avec son contact naturel : taille deux, `natural_face`, dimension deux et rang affine un; taille trois, `natural_edge`, dimension un et rang deux; taille quatre, `natural_vertex`, dimension zéro et rang trois. Les diagnostics extra-shell pertinents sont agrégés par centre, niveau, shell et carrier exacts. Le résultat distingue candidats proposés, supports acceptés et diagnostics; il ne transforme jamais le barycentre d'un contact en centre critique.

Le préflight transactionnel porte six capacités. À $n=8$, leurs plafonds de confiance sont 247 contacts source, 4704 propositions brutes, 13440 références brutes de `PointId`, 154 supports uniques, 504 références uniques et 1232 classifications point–boule. Une insuffisance précède tout rejeu du diagramme et publie seulement identité, boîte, exigences et budget; une source incomplète ou non certifiée arrête ensuite l'extraction avant toute proposition. Le vérificateur reconstruit chaque couche depuis le nuage, la boîte, la source, $K_{\max}$ et le budget fiables.

La suite courte passe sous GCC et Clang stricts. Elle couvre le singleton, le tétraèdre régulier et ses trois strates, un pentagone cocirculaire de shell cinq avec quotients explicites, la frontière exacte d'un triangle à un ULP, un contact porté par la boîte, les six budgets au plafond $n=8$ sans construire le diagramme du cube, les mutations hostiles et l'invariance par permutation. Le catalogue exhaustif 6.12 confirme uniquement côté test, dans les deux sens, les supports acceptés et les diagnostics de profondeur zéro.

Ce jalon ne produit aucun singleton de rayon nul dans le catalogue H0, lot H0, événement public, `CatalogCertificate`, `closed_parent_orders[1]` ou `public_status`. L'absence de diagnostic à profondeur zéro ne prouve pas `RelevantGP` global. La Phase 8 reste `ready`; le jalon 8.6 devra ajouter séparément les singletons H0, construire le catalogue H0 d'ordre un et fermer l'ordre parent seulement après réconciliation de toutes les files. GCP n'est pas utilisé.

### Décision de réutilisation et de performance

L'atlas `Fraction` de 8.4 est désormais gelé comme oracle de preuve `n<=8`; il ne doit pas devenir une bibliothèque de Voronoï générale ni entrer dans le chemin utilisateur. Avant toute nouvelle baseline de Voronoï, extension de domaine de test ou primitive CPU spécialisée, l'agent doit d'abord évaluer un adaptateur épinglé vers Geogram ou une bibliothèque mature équivalente, avec version, licence, options numériques et projection sémantique documentées. Une telle bibliothèque reste une baseline ou une source de propositions tant que ses sorties ne sont pas recertifiées par les contrats exacts du dépôt.

Le chemin produit reste GPU-first : génération et filtrage massifs sur `cuda_g4`, décisions combinatoires ambiguës rejouées exactement sur l'hôte, transferts et matérialisation strictement budgetés. Les choix des Phases 8 à 13 doivent préserver les deux cibles de produit : p95 `warm_e2e` strictement inférieur à 100 ms pour le passage complet d'environ 50 000 points et $K_{\max}\leq10$ sur les familles favorables préenregistrées, avec un seuil secondaire strictement inférieur à une seconde, puis streaming transactionnel pour dix millions de points ou davantage avec sortie exacte lorsque le certificat reste sparse et arrêt budgétaire honnête sinon. Aucun élargissement d'un oracle de test CPU ne doit retarder ou remplacer cette voie.

### Travaux

- définir $\Omega$ comme une AABB dyadique **strictement paddée** autour de l'AABB exacte de $X$; chaque face doit être strictement extérieure à $\mathrm{conv}(X)$;
- enregistrer le padding et refuser avant calcul toute construction qui ne reste pas finie dans le type de proposition, plutôt que rabattre une coordonnée sur la boîte;
- documenter la preuve $c\in\mathrm{conv}(U)\subseteq\mathrm{conv}(X)\subseteq\Omega$ pour tout centre critique;
- proposer un sous-ensemble initial de sites;
- clipper les cellules;
- interroger le 1-NN global hors label vide à chaque sommet;
- ajouter violateurs et co-minimiseurs;
- répéter jusqu'à file vide;
- marquer les faces artificielles;
- interdire l'émission d'un événement dont le support d'incidence utilise une face artificielle;
- réconcilier faces, arêtes et sommets;
- extraire supports de rang faible;
- produire `CatalogCertificate` ordre un.

### Tests

- cellule par cellule contre CGAL ou oracle demi-espaces;
- insertion volontairement mauvaise d'un seul site initial;
- site absent de l'amorce, révélé comme gagnant à un sommet exact provisoire; le cas « violateur uniquement intérieur sans sommet violateur » est impossible pour une différence affine et ne constitue pas une fixture valide;
- ties sur face, arête et sommet;
- ordres de colonnes aléatoires;
- aucun événement issu de $\partial\Omega$.
- centres critiques placés sur chaque face, arête et sommet de l'AABB non paddée, qui doivent rester intérieurs à $\Omega$ et être retrouvés;

### Porte de sortie

Toutes les cellules et incidences sont identiques à l'oracle sur petits cas. `closed_parent_orders[1]` n'est vrai que file vide et sans overflow.

## Phase 9 — Flux direct de supports $H_0$ sans mosaïque

### Porte d'entrée

Satisfaite par les Phases 1, 2A, 4 et 7. L'oracle exhaustif, les prédicats exacts, le LBVH et le contrat GPU proposition--recertification existent déjà. La fermeture globale de la Phase 8 n'est pas une dépendance : ses jalons bornés restent un différentiel de profondeur zéro.

### But

Énumérer directement les sphères critiques bien centrées de supports deux, trois et quatre et de rang fermé au plus $s_{\max}$, sans construire les cellules top-$m$, la mosaïque de Delaunay d'ordre $K$, les $\binom{n}{k}$ facettes ou Gamma. Un seul flux partagé alimente tous les ordres $1\leq k\leq K_{\mathrm{eff}}$.

Les oracles exhaustifs de supports et de Gamma restent mathématiquement valides jusqu'à $n\leq14$. La voie cellulaire D.1--D.4, qui ferme les parents top-$m$ et reconstruit l'essentiel de la combinatoire de la mosaïque sous une forme streamée, est séparément gelée à $n\leq8$ et ne constitue plus le chemin produit.

### 9.0 — image LBVH résidente puis construction device

Le premier noyau supports-deux peut réutiliser l'index CPU certifié et charger son snapshot sur le GPU, comme les Phases 4–5. Ce choix accélère l'apprentissage du nouveau parcours sans prétendre fermer le SLO : le dépôt ne possède pas encore de constructeur Morton/LBVH CUDA, seulement des consommateurs CUDA d'un arbre construit récursivement sur CPU.

Avant la qualification finale à 50 000 points et obligatoirement avant le chemin 10 M+, ajouter Morton SoA, radix sort stable device, topologie LBVH parallèle et réduction bottom-up des AABB, avec manifeste déterministe et vérification contre l'index de référence. Ajouter aussi `DeviceScan`, les deux frontières persistantes et leur capture CUDA Graph. Le temps `warm_e2e` inclut cette construction; mesurer seulement un noyau sur snapshot résident reste diagnostique.

### 9.1 — paires : branch-and-bound exact sur LBVH

Pour une paire $(u,v)$ et un point $x$, l'appartenance stricte à la boule diamétrale équivaut à $\phi(x,u,v)<0$, avec $\phi(x,u,v)=(x-u)\mathbin{\cdot}(x-v)$. Pour trois boîtes dyadiques $A,B,C$, le maximum exact de $\phi$ est la somme, axe par axe, des maxima aux huit choix d'extrémités : la fonction est convexe en $x$ et affine séparément en $u$ et $v$. Si ce maximum est strictement négatif, tous les points de $C$ sont intérieurs à toutes les boules des paires de $A\times B$.

Un parcours self-dual de paires de nœuds LBVH compte ainsi une antichaîne canonique de sous-arbres intérieurs garantis. Leurs plages Morton sont deux à deux disjointes et disjointes des deux plages supports; le compteur porte sur la cardinalité de leur union, jamais sur une somme de nœuds éventuellement chevauchants. Dès que ce compte atteint $s_{\max}-1$, toutes les paires du produit ont plus de $s_{\max}-2$ points intérieurs et le produit entier est éliminé. Sinon le plus grand nœud support est scindé; aux feuilles, la paire est recertifiée par centre, niveau, shell et rang globaux. Le nombre de paires terminales reste au plus quadratique, mais leur parcours témoin ou une classification globale naïve peut porter le travail brut jusqu'à $\Theta(n^3)$; aucune borne sous-cubique n'est revendiquée sans amortissement supplémentaire.

Le jalon hôte `9.1-RCPU` livre désormais cette partition self-duale, la borne $\phi$ exacte, les prunes strictes, les budgets transactionnels et un rejeu frais. Sa requête terminale n'appelle pas la partition globale existante : elle compte les extérieurs par sous-arbres, conserve au plus $s_{\max}-2\leq9$ intérieurs et remplace tout shell supplémentaire potentiellement massif par son cardinal exact et un seul témoin canonique. Son ancienne projection `remaining_frontier` reste un reçu non réinjectable. Le jalon `9.3a-RCPU` fournit désormais l'API séparée de checkpoint et de chunk; ce lot ne produit toujours ni forêt, ni statut public, ni complétude pour les supports trois et quatre; voir `docs/validation/PHASE9_PROGRESS.md`.

Le backend `cuda_g4` évalue des intervalles dirigés et produit une proposition de prune ou de descente; le backend `reference_cpu` recertifie tout prune exact utilisé par une sortie certifiée. Les deux passes count/emit utilisent une frontière bornée et aucun append global non borné.

Le premier incrément CUDA `9.1-CUDA-P1` reçoit un batch borné de triplets canoniques $(A,B,C)$ fourni par le pilote CPU et un snapshot AABB/plages du LBVH validé puis résident. Un thread traite un triplet par la formule exacte aux extrémités, mais chaque opération binary64 est arrondie vers l'extérieur; le transcript GPU ne propose `strict_interior` que si sa borne supérieure finie est strictement négative. L'hôte exige une permutation complète des requêtes, des identités inchangées, une epoch fraîche et une queue sentinelle intacte, puis recalcule rationnellement $M(A,B,C)$ avant de produire l'unique reçu de nœud utilisable. Une proposition n'est donc jamais une décision scientifique :

$$\overline{M}_{\mathrm{GPU}}\geq M(A,B,C),\qquad \overline{M}_{\mathrm{GPU}}<0,\qquad M_{\mathrm{CPU}}=M(A,B,C)\leq\overline{M}_{\mathrm{GPU}}<0.$$

P1 ne parcourt pas encore l'arbre témoin, n'agrège aucune antichaîne et ne publie aucun prune global de produit. Le jalon P2 doit intégrer ces reçus au seuil de rang, puis ajouter count/`DeviceScan`/emit, deux frontières device et CUDA Graph. La construction Morton/LBVH device, les supports trois--quatre CUDA, le SLO et la voie 10 M+ restent aux Phases 14--15.

### 9.2 — triangles et tétraèdres

Généraliser la même frontière aux triplets et quadruplets. Les filtres GPU utilisent les déterminants de dépendance affine, de barycentriques et d'in-sphère sur boîtes. Un prune n'est certifiant que si une borne exacte démontre soit l'impossibilité du bon centrage, soit au moins $s_{\max}-\lvert U\rvert+1$ points strictement intérieurs pour tous les supports du produit. Toute boîte ambiguë est scindée jusqu'aux feuilles; aucune liste $L$-NN fixe ne possède un pouvoir d'exclusion.

Le premier incrément hôte `9.2a-RCPU` emploie une frontière de groupes $(N_i,r_i)$ : la multiplicité d'un nœud est répartie entre ses deux enfants par tous les entiers admissibles, ce qui partitionne exactement les sous-ensembles non ordonnés sans matérialiser leurs permutations. Les cardinalités de preuve sont des `BigInt`, car $\binom{10\,000\,000}{4}$ dépasse 64 bits. Les bornes de produit évaluent exactement par intervalles rationnels les déterminants de Gram et de Cramer, les numérateurs barycentriques et le polynôme homogène de puissance. La spécialisation triangle majore chaque angle par huit triples d'extrémités unidimensionnels; les coins d'une boîte requête resserrent le majorant de puissance par convexité. En revanche, les coins des boîtes supports n'ont aucun pouvoir de décision universelle et une fixture permanente l'interdit.

Aux feuilles, dépendance, bon centrage, centre, niveau et boule fermée globale sont décidés avec les primitives exactes existantes. La requête LBVH garde au plus $s_{\max}-\lvert U\rvert$ intérieurs, compte extérieurs et shell sans les matérialiser et conserve un seul témoin extra-shell. Les prunes portent le produit, son cardinal exact, l'analyse d'intervalle et une antichaîne de reçus de rang rejouables. Après réservation des racines initiales complètes, une limite atteinte conserve une frontière résiduelle et retourne `budget_exhausted`; aucune complétude n'est déduite d'un run interrompu. Le jalon 9.2a reste historiquement l'API monolithique; le pire cas demeure $O(n^3)$ ou $O(n^4)$ en temps et la propriété acquise est l'absence d'arène combinatoire résidente, pas une borne de débit.

Le jalon `9.2b-RCPU` ajoute un checkpoint supérieur compact et une session en mémoire ancrée aux racines canoniques. Le curseur conserve le produit actif, son étape, la pile DFS de rang, les seuls reçus de nœuds stricts, leur cardinal exact et l'audit cumulatif; toutes les analyses rationnelles sont recalculées. `prepare_next` exige la réinjection exacte du checkpoint fiable sans avancer la session, puis `commit_prepared` rejoue la transition et déplace le checkpoint attendu. Une vérification locale de checksum et de cardinalités n'est jamais une preuve de provenance : une frontière peut dupliquer des produits de même cardinalité tout en gardant la bonne somme. Seules la session ancrée et le rejeu depuis les racines certifient donc la filiation. La chaîne de sortie engage une projection minimale déterministe des trois types de records; les analyses de prune dérivées restent couvertes séparément par le rejeu complet.

Ce jalon garde un seul checkpoint et aucun historique de chunks. Il ne contient encore ni codec supérieur hostile et borné, ni publication durable, ni récupération de session après perte de processus, ni filtre CUDA, réduction en forêt, qualification 50 k ou voie 10 M+. Un futur wire devra employer ses propres caps binary64, borner avant allocation les reçus stricts par neuf et la pile DFS par la profondeur du LBVH, et reconstruire une autorité ancrée plutôt que faire confiance à un checkpoint décodé.

### 9.3 — flux, reprise et certificat

Le flux durable persistera seulement des `CriticalEvent`, diagnostics de dégénérescence, compteurs et frontières de reprise. Chaque chunk est lié au nuage canonique, au LBVH, à $K_{\mathrm{eff}}$, aux règles de parcours et au digest exact du checkpoint source. Le checksum non secret prouve seulement l'intégrité locale; la filiation scientifique reconstruit le checkpoint initial depuis les autorités puis rejoue chaque transition dans l'ordre. La cible durable publiera uniquement entre deux unités de frontière et rejouera le dernier chunk non publié. `complete=true` exige une frontière vide, tous les prunes recertifiés, tous les candidats feuilles classifiés, tous les shells rang-pertinents finis et aucun overflow ou signe inconnu.

Le sous-jalon `9.3a-RCPU` ferme le contrat de préparation en mémoire : SHA-256 incrémental sans matérialiser les coordonnées sérialisées, manifeste complet, produit actif à trois états, curseur témoin avec reçus stricts et expansion différée, budgets relatifs au chunk, ordre inter-types des records, chaîne de digests par record, intégrité locale du checkpoint, transition relative rejouée fraîchement et run terminal ancré au checkpoint initial reconstruit. Les sept motifs d'arrêt reprennent vers le même audit terminal que l'exécution résidente. Les falsifications auto-rehashées de frontière, reçus, identités obligatoires de l'audit et filiation sont rejetées, et des vecteurs hexadécimaux dorés figent le schéma v1. Le candidat retourné reste toutefois une paire de valeurs mutables en mémoire : toute mutation échoue au rejeu, mais codec borné, validation des longueurs avant allocation, temporaire, synchronisation et renommage atomique sur stockage durable restent ouverts. La Phase 9 demeure `in_progress`.

Le sous-jalon `9.3b-RCPU` ferme le coût de vérification préalable au codec. Un contexte d'autorité immuable calcule le manifeste une fois, puis tous les chunks et leurs rejeux utilisent cette empreinte mise en cache. La frontière de $F$ produits est validée par un balayage de rectangles orientés en $O(F\log F)$ et $O(F)$ mémoire; les événements n'en conservent que l'index compact. Les $M$ plages de reçus et de curseur sont validées par tri en $O(M\log M)$, tandis que chaque reçu strict garde sa recertification rationnelle exacte. `verify_next` ancre son état au checkpoint initial, compare une seule transition attendue, ne revalide pas son checkpoint source privé déjà certifié, avance avec le checkpoint fraîchement recalculé et se verrouille sans avancer au premier échec ou à la première exception. Il possède une copie du cache d'autorité, rejette les autorités temporaires et ne conserve aucun chunk antérieur; le vérificateur de run historique devient un adaptateur de cette induction. Les compteurs instrumentés ferment un hash de chaque point, feuille et nœud exactement une fois, au plus deux rectangles et quatre événements de sweep par produit, au plus deux tests de voisinage par rectangle, une insertion d'intervalle par entrée témoin et une recertification géométrique par reçu. Le schéma et les digests v1 restent inchangés. Codec hostile, stockage durable, perte de processus ou de VM et SLO restent ouverts; la Phase 9 demeure `in_progress`.

Le sous-jalon `9.3c-RCPU` ferme la perte de processus sur un stockage Unix local disposant de `flock`, `fdatasync`, renommage atomique et `fsync` de répertoire. Un codec binaire v1 canonique, big-endian et indépendant de l'ABI encode dans un même bundle le chunk et son checkpoint successeur; au décodage hostile, ses limites externes finies bornent les octets, frontières, entrées auxiliaires, records, références `PointId` et textes exacts avant `reserve` ou construction de `BigInt`. Le SHA-256 wire ne certifie que l'intégrité du transport : même recalculé après mutation, il ne remplace jamais le rejeu géométrique ancré. Le sink fixe un budget de chunk externe immuable, prépare la transition sans avancer le checkpoint, écrit un temporaire dans le répertoire dédié, le synchronise, le relit, le renomme vers un fichier final immuable, synchronise le répertoire, puis seulement committe en mémoire par une opération sans exception. La reprise acquiert un verrou mono-écrivain, reconstruit l'état initial et rejoue les fichiers consécutifs un par un; elle ne garde aucun historique, nettoie seulement le temporaire non publié de la séquence suivante et rejette toute corruption présente, trou suivi d'un final ou filiation invalide. Des sous-processus terminés réellement aux quatre frontières d'écriture montrent l'ancien état avant renommage et le nouvel état après renommage, sans double transition. Ce résultat ne couvre ni coupure électrique, ni perte de VM ou Hyperdisk, ni Windows ou filesystem réseau, ni suppression ou rollback d'un suffixe final autrement valide sans `HEAD` externe. L'encodeur reçoit des objets C++ fiables et matérialise encore simultanément un payload et un wire bornés, tandis que chaque fichier répète le checkpoint. Il ne ferme donc ni la voie 10 M+, ni le SLO 50 000 points, ni les supports trois–quatre; la Phase 9 demeure `in_progress`.

Le sous-jalon `9.3d-RCPU` sépare l'autorité transactionnelle locale de la fraîcheur externe. `create_new` publie obligatoirement un `HEAD_0`, tandis que `open_existing` exige un `HEAD` présent et ne déduit jamais l'état du plus long préfixe de fichiers plausible. Le `HEAD` v1 fixe de 142 octets lie un digest du contrat de run — checkpoint initial, manifeste scientifique, budget et versions — au nombre de transitions committées, à leur taille wire cumulée et au digest du checkpoint fiable. Un chunk est publié sans remplacement par `linkat`, puis le remplacement atomique du seul `HEAD` devient le point de linéarisation locale; le checkpoint mémoire avance seulement après la seconde synchronisation du répertoire, suivant ce remplacement. Tout final exactement à la séquence désignée par `HEAD` est un orphelin non committé : la reprise doit le décoder et le rejouer exactement avant de le supprimer, sans jamais avancer. Relativement au `HEAD` observé, un chunk committé manquant, un trou, une corruption ou une fausse filiation échoue fermé. En revanche, un ancien snapshot cohérent de `HEAD` et de tous ses chunks est localement indistinguable d'un run honnêtement arrêté à ce préfixe. Une ancre externe $(j,D_j)$ rejette un rang local inférieur à $j$ ou une divergence au rang $j$, sous l'hypothèse d'absence de collision SHA-256; elle n'interdit pas un rollback vers un préfixe valide plus récent que cette ancre. L'anti-rollback d'un commit n'est donc acquitté qu'après remplacement atomique de l'ancre monotone externe par le compteur et le digest de ce commit; `open_existing` la recertifie au passage de l'induction. L'encodeur de chunk conserve le wire v1 mais écrit directement en un seul `ByteWriter`, sans second vecteur payload. Le décodeur depuis fichier et la conversion décimale exacte ne sont pas encore streamés : 10 M+, le SLO 50 000 points, les supports trois–quatre et le statut public restent ouverts; la Phase 9 demeure `in_progress`.

Le sous-jalon `9.3e-RCPU` supprime le vecteur wire du chemin durable sans créer un second codec. Pour un chunk immuable entre les passes, le visiteur canonique du payload est partagé entre comptage, vecteur et descripteur; le chemin fd mesure d'abord le payload, puis écrit et SHA-hache l'enveloppe finale et le payload intentionnels par blocs positionnels de 64 Kio. Après `fdatasync`, une relecture bornée exige la même taille et le même checksum avant publication. Le fd authentifié est ensuite relié aux noms temporaire et final par des contrôles d'inode avant tout avancement; le même contrôle lie le fd de `.HEAD.tmp` au `HEAD` renommé. Les substitutions de nom et corruptions en place aux fenêtres instrumentées sont réauthentifiées et échouent fermées. La reprise vérifie une première fois enveloppe, EOF et checksum avant allocations sémantiques, resserre le cap à la taille inventoriée, parse ensuite un seul chunk depuis le fd et rehache les octets consommés. L'encodeur refuse fichiers non réguliers, non seekables, non vides, mauvais modes, `O_APPEND` et `O_DIRECT`; vérificateur et décodeur acceptent un fichier non vide en `O_RDONLY` ou `O_RDWR`. Tous refusent offsets hors `off_t`, I/O sans progrès et changements de type, identité ou taille. Le golden wire v1 reste inchangé et un unique scratch physique de 64 Kio est emprunté successivement par vérification et parsing. Un préfiltre sur la longueur binaire des rationnels précède toute conversion décimale; pour un milieu et un rayon carré de paire issus directement de binary64 finis, la preuve 2099/1076 et 4200/2151 bits borne les textes à 958 et 1914 octets, donc la limite par défaut 2048 est sûre. Cette borne ne se transfère pas encore aux supports trois–quatre. Le chunk décodé, ses `BigInt`, la frontière géométrique et les temporaires du champ exact courant restent des mémoires réelles; le jalon ne ferme donc ni 10 M+, ni le SLO, ni le statut public, et la Phase 9 demeure `in_progress`.

### Invariants structurels

- aucune allocation de taille $\binom{n}{k}$ ou $\binom{n}{k+1}$;
- aucune arène de cellules ou de parents top-$m$;
- aucun Gamma global dans la cible produit;
- mémoire résidente bornée par points, LBVH, frontières/chunks, fallback exact et sortie;
- les nombres de produits prunés, supports feuilles, événements, octets et pics de frontière sont obligatoires;
- une frontière non vide retourne `budget_exhausted` ou `conditional`, jamais `exact`.

### Tests et portes courtes

- égalité complète avec le catalogue exhaustif pour $n\leq14$, dont permutations et contre-exemples permanents;
- chaque prune est falsifié par mutation indépendante de sa borne, de ses plages Morton et de son compte intérieur;
- les certificats qui répètent un nœud témoin, mélangent ancêtre et descendant ou recouvrent une plage support sont rejetés;
- paires longues absentes de listes $L$-NN mais conservées par le parcours;
- exécution chunkée incrémentale de même valeur canonique, même ordre typé, même audit et mêmes digests que l'exécution résidente, avec un seul manifeste, aucun historique de chunks retenu et des compteurs de validation quasi linéaires; identité octet par octet du codec v1, limites hostiles avant allocation et rejeu ancré après chaque crash de processus aux frontières de publication;
- smoke de croissance à 12 500, 25 000 et 50 000 points sur deux familles volumiques, sans campagne longue : deux exposants consécutifs supérieurs à 1,35 sur la frontière, les tests de boîtes ou les feuilles déclenchent un no-go architectural;
- démonstration de reprise et d'absence d'OOM sur un flux synthétique dépassant la mémoire de sortie.

### Porte de sortie

Le catalogue direct complet égale l'oracle $n\leq14$ et aucun objet interdit n'est alloué. Le prototype 50 000 points publie ses compteurs et son temps dès le support deux; la phase ne prétend pas que les supports trois et quatre auront le même régime avant mesure. La complétude des trois tailles ferme seulement la base interne du flux; une taille inachevée reste explicitement `budgeted`. Elle ne modifie pas le `public_status` du contrat v2 actuel, dont la source exacte reste `gamma_exhaustive_reference`; son remplacement exige une migration contractuelle versionnée après le journal, la tour verticale et la preuve M.1.

Évaluation finale du 22 juillet 2026 : la porte de sortie interne est satisfaite. Les arités deux à quatre terminales, fraîchement rejouées, coïncident avec l'oracle borné pour $n=1,\ldots,14$ et pour la permutation inversée à $n=14$; le checker statique ne trouve ni mosaïque, ni Gamma, ni arène combinatoire. Le smoke 12 500--50 000 points reste explicitement `budget_exhausted`, sans SLO. Le commit propre `976c1c6723760e9d1632f139e3cad238a40b1cb8` qualifie réellement `9.1-CUDA-P1` sur G4 : une proposition stricte, une descente, epochs 1 puis 2, ELF AOT `sm_120` sans PTX, recertification CPU exacte, `memcheck` et `racecheck` passés. P1 reste un batch borné sans parcours escape, antichaîne, prune global, LBVH device, frontières ou `DeviceScan`; les arités trois--quatre GPU, le SLO 50 k, 10 M+, la forêt, M.1 et tout statut public restent ouverts. La Phase 9 est `completed` et la Phase 10 est ouverte en `partial_refinement`; voir `docs/validation/PHASE9_GATE_REVIEW.md`.

## Phase 10 — Journal Morse et réduction directe

### Porte d'entrée

Satisfaite par la fermeture de la Phase 9 et par le jalon local de Phase 5 `compact_k1_forest_certified/local_k1_compact_forest_only`. Cette dépendance de jalon ne ferme pas la Phase 5 globale : sa voie scalable générale reste `ready`. Le recul mathématique du 23 juillet 2026 retire la complétude des gateways silencieux de la porte produit, mais ne permet pas d'invoquer les Théorèmes 4--5 et la Proposition 6 pour remplacer Gamma par le graphe Gabriel brut : E5 réfute cette implication. La chaîne candidate conserve le catalogue Gabriel, ses bras stricts, leurs descentes vers les carriers complets et le quotient atomique des racines réduites optionnelles. Sa fidélité globale à Gamma est une obligation horizontale distincte; M.1 reste explicitement différée à la Phase 12 après construction de la tour réduite.

### But

Consommer le flux de Phase 9 pour construire directement une généalogie $H_0$ compacte. Gamma exhaustif reste un oracle $n\leq14$ et n'entre jamais dans le binaire ni dans la mémoire du chemin produit.

### Travaux

- injecter les minima de rang $k$ et le catalogue complet des simplexes de Gabriel de rang $k+1$, y compris les selles tardives, depuis le flux partagé;
- construire les au plus quatre bras de chaque simplexe et les attacher aux carriers du snapshot strict pré-lot;
- trier les niveaux par filtre puis comparateur exact;
- résoudre chaque niveau comme un hypergraphe quotient sur les sommets typés $R(r)$ et $L(h)$ des carriers, sans retirer les carriers latents avant la fermeture transitive, puis compter seulement les racines réduites non nulles;
- garder les minima d'ordre au moins deux latents jusqu'à leur première composante non triviale;
- persister seulement les minima, les liaisons bras--carrier, les lots atomiques, les enfants et les racines finales par ordre;
- traverser à la demande les facettes silencieuses lorsqu'une descente stricte les rencontre, sans les cataloguer ni en faire des événements $H_0$;
- réutiliser le sweep Morse 6.23 comme falsificateur interne, puis retirer son historique Gamma du chemin produit;
- produire les forêts horizontales de chaque ordre et les runs externes nécessaires au streaming.

La décision candidate, l'induction conditionnelle et l'obligation de fidélité des carriers sont consignées dans [REDUCTION_MORSE_H0_PHASE10.md](math/REDUCTION_MORSE_H0_PHASE10.md). Les jalons 10.6--10.15 de première incidence et de gateways restent disponibles comme audits renforcés facultatifs. Ils certifient notamment les premières promotions des minima latents mais ne découvrent pas les selles tardives et ne remplacent donc jamais le catalogue direct. Ils ne pilotent plus le chemin produit, ne sont plus une porte de fermeture et ne sont pas réimplémentés sous une autre forme.

### État du déploiement au 23 juillet 2026

Les incréments 10.1 et 10.2 sont livrés en `partial_refinement`. Le premier projette les naissances et selles dans des lots exacts avec une borne $3n+5E$. Le second applique le théorème du support positif minimal : pour chaque selle $S=I\cup U$, il conserve exactement les graines $(S,u)$, $u\in U$, dont les facettes $F_u=S\setminus\lbrace u\rbrace$ vérifient $\beta(F_u)<\beta(S)$. Les facettes sont reconstruites dans un scratch de dix identifiants et ne sont pas stockées; l'ajout vaut au plus $5E$, soit $3n+10E$ pour les deux étages. Les rejeux 10.1 et 10.2 sont streaming, sans seconde arène persistante ni tri global, et les reconstructions sont liées aux digests d'autorité.

L'incrément 10.3 ferme exactement la tranche $k=1$. Une selle d'ordre un est nécessairement de rang fermé deux, sans intérieur, et ses deux bras sont les singletons opposés. Le lemme de descente de la boule diamétrale fermée remplace toute paire non directe par deux paires strictement plus courtes; les paires directes engendrent donc la même filtration de composantes que le graphe complet et la forêt EMST. À chaque niveau exact, `ExactDirectK1ForestJournalResult` fige les racines antérieures, résout toutes les paires dans un quotient local, puis seulement applique les unions. Une composante $q\geq2$ crée une multifusion unique; $q=1$ persiste comme continuation sans nœud. Les naissances sont implicites, le stockage ajouté est au plus $2n+5J_1-2$, le scratch vaut $O(n+B_{\max})$ et le target isolé n'appelle ni archive historique, ni géométrie globale. Les trois rejeux sont streaming et le dernier ne construit pas une seconde sortie persistante.

L'incrément 10.4 complète exactement les $k+1$ suppressions locales de chaque selle directe. Les suppressions d'un point du support réutilisent les bras stricts 10.2; pour $x\in I$, l'inclusion $U\subseteq S\setminus\lbrace x\rbrace\subseteq S$ prouve par monotonie $\beta(S\setminus\lbrace x\rbrace)=\beta(S)$. Le journal ajoute seulement une famille et une graine factorisée par suppression intérieure, soit $J+P\leq10E$ entrées et au plus $3n+20E$ avec 10.1--10.2. La frontière collinéaire $K=10$, $n=11$ certifie deux bras stricts et neuf facettes égales. Le noyau plat séparé de fermeture sur racines externes préserve aussi les groupes $q=1$, mais ne représente pas les facettes latentes et ne classe aucune action HGP.

L'incrément 10.5a livre le premier locator positif sparse autonome. Une clé canonique conserve son cardinal et jusqu'à dix `PointId` dans une arène plate; l'empreinte masquée n'est qu'un accélérateur et chaque candidat est comparé par clé complète. Les requêtes voient strictement l'état pré-appel, les unions explicites sont appliquées dans un DSU candidat avant la compatibilité des doublons, puis le lot est engagé tout-ou-rien. Les clés conservent leurs handles stables sans réécriture; un miss retourne exclusivement `unresolved` et un conflit exact post-unions rejette aussi les unions sans rapport. Le noyau vérifie seulement la forme de jetons liés à une autorité affirmée par l'appelant : il ne rejoue ni cette autorité, ni la géométrie. Son vérificateur structurel recalcule empreintes et index, couvre l'arène, rejoue dans l'ordre committé le premier slot libre de chaque insertion, rejoue le DSU et contrôle les agrégats, sans transformer les assertions historiques en preuves fraîches. Un budget de vérification séparé borne les populations avant allocation, les visites cumulées sous collisions, la chronologie physique et le rejeu DSU; un épuisement échoue fermé et reste distinct d'une structure falsifiée.

Le stamp du locator engage aussi l'historique sémantique committé par une chaîne SHA-256 séparée par domaine. Deux historiques distincts ne sont distingués par ce digest que sous l'hypothèse d'absence de collision SHA-256; le digest tout-zéro reste une valeur valide et n'est pas une sentinelle. Une `ExactDirectSparsePositiveFacetLocatorStateView` emprunte ses spans au stockage sous-jacent : tout appel à `apply_batch` l'invalide contractuellement et impose de reprendre la vue. Son rejeu structurel exige que ce stockage reste vivant et immuable pendant toute la vérification, sous exclusion externe de toute mutation concurrente.

L'état durable 10.5a est $O(M+KL+H+U+T)$ pour une capacité de table $M$, $L$ clés, $H$ handles, $U$ unions et $T$ lots. Depuis l'incrément industriel 14B, le scratch d'un lot est $O(Q+KB+U_b)$ pour $U_b$ demandes d'union : un journal sans compression conserve seulement les $W_b\leq\min(U_b,H-1)$ racines réorientées pour $H>0$, sans copie ni scan systématique des $H$ parents. Il n'existe aucun terme en $\binom{n}{k}$, aucune facette absente, cellule, coface, incidence Gamma ou mosaïque d'ordre supérieur n'est construite. La table reste préallouée à $2M+1$, et le sondage comme la profondeur DSU ont encore des pires cas linéaires : ni le SLO 50 k sous la seconde, ni le profil 10 M+ ne sont qualifiés.

L'incrément 10.5b livre et valide sur l'hôte une tentative exacte d'un seul pas. La sonde locator `const` conserve la clé complète et son vérificateur public rejoue locator, clé, témoin et budget; elle distingue exactement hit, miss complet et épuisement sans mutation. Chaque miniball locale énumère au plus 385 supports, puis son vérificateur frais répète cette énumération; avec au plus deux facettes, une tentative effectue donc au plus quatre passes et 1540 examens de supports candidats. Le top-k LBVH contrôle séparément sept plafonds avant opération : visites, expansions internes, bornes AABB exactes, distances exactes, frontière, meilleurs voisins et shell. Un épuisement ne publie aucune partition scientifique. Les six CTests GCC Release ciblés passent en 4,16 secondes, y compris les deux fixtures d'égalité, les sept frontières exactes et moins-un, les deux plafonds de chaque sonde et les contrôles statiques/symboliques.

Pour une source admise au niveau fermé $a$, 10.5b accepte l'égalité $\beta(F)=a$ et ne publie un témoin que si $\beta(G)<\beta(F)\leq a$. Si $d=d_k(c_F)$, le théorème exact établit la stricte sous-niveau pour le segment source-ouvert $t\in(0,1]$; le segment fermé entier est strictement sous $a$ si et seulement si $d<a$. La source ou la cible ne résout la composante que sur un hit déjà présent dans le même locator pré-appel. Un miss reste `unresolved`; un plafond atteint reste `budget_exhausted`; aucun cas ne crée insertion, singleton ou attache. Voir [DESCENTE_FACETTE_SPARSE_PHASE10.md](math/DESCENTE_FACETTE_SPARSE_PHASE10.md).

L'incrément 10.5c livre la fermeture bornée multi-source de ces pas sous la forme d'une forêt fonctionnelle plate. Les $R$ références de graines sont triées, dédupliquées par clé complète, puis les $S$ graines distinctes sont préinternées avant la première opération géométrique; un cap de sommets inférieur à $S$ échoue donc au préflight. Chaque sommet est évalué par 10.5b au plus une fois, chaque cible d'une arête stricte publiée est internée même lorsqu'elle est déjà positive, et deux chemins convergents partagent exactement leur suffixe. Un même `LocatorSnapshotStamp` est capturé avant la construction, vérifié après chaque pas et contrôlé à la fin; sa chaîne SHA-256 séparée par domaine engage les deltas sémantiques committés et, sous l'hypothèse d'absence de collision, son vérificateur la rejoue depuis les clés, unions, témoins et compteurs durables. Ces stamps sont des gardes séquentielles, pas des verrous. Le `const` ne fournit aucune synchronisation : l'appelant doit geler le locator et exclure tout `apply_batch` concurrent depuis la capture du build jusqu'à la fin du vérificateur frais. La preuve et la portée relative au locator gelé sont consignées dans [FERMETURE_DESCENTE_FACETTE_SPARSE_PHASE10.md](math/FERMETURE_DESCENTE_FACETTE_SPARSE_PHASE10.md).

Si $V$ est le nombre de sommets internés, $T$ celui des terminaux, $E$ celui des arêtes strictes, $B$ celui des sources évaluées et $Q$ celui des requêtes top-$k$, la décroissance exacte interdit les cycles et ferme les identités suivantes :

$$E=V-T<V,\qquad E\leq Q\leq B\leq V\quad(V>0).$$

Un cache transitoire indexé par clé complète conserve une unique miniball fraîche à chaque couture. Il réutilise aussi la miniball d'un successeur non strict, même si ce successeur ne devient pas un sommet scientifique; les compteurs de construction et de réemploi sont recroisés avec les compteurs agrégés de 10.5b. Les partitions top-$k$, shells et frontières ne sont jamais persistés. Les budgets de graines, sommets, appels, table de mémoïsation et travail local sont contrôlés avant publication : si une cible ne peut pas être engagée, le témoin reste diagnostique et aucune demi-arête n'est publiée. Le résultat persistant contient $R$ projections, $V$ nœuds, $E$ arêtes et au plus $KV$ identifiants, d'où un stockage logique $O(R+KV)$ pour $K\leq10$, sans Gamma, cofaces, cellules ni mosaïque de Delaunay d'ordre supérieur.

Le jalon `10.6-RCPU` livre l'oracle sparse de première incidence pour une unique facette canonique fournie, de cardinal au plus dix, et toutes ses cofaces obtenues par ajout d'un point. La miniball source est construite puis rejouée, soit deux passes de 385 supports et au plus 770 examens. Un point dans sa boule fermée conserve le niveau source; pour un point strictement extérieur, tout support positif minimal de la coface contient ce point, de sorte que l'énumération se réduit à $1+10+45+120=176$ supports au bord $K=10$. Le minorant rationnel d'un nœud LBVH est le maximum de la borne radiale issue du support pondéré et des bornes de paires avec chacun des points de la facette; il consomme donc exactement $K+1$ distances AABB au bord. Seule une borne strictement supérieure à l'incumbent autorise un prune; l'égalité force la descente, et tous les co-minimiseurs du niveau minimal sont publiés atomiquement ou aucun ne l'est sur épuisement.

Cette portée locale ne choisit pas les facettes-portes à interroger et ne publie ni token latent, ni `GatewayAttach`, ni racine, ni union, ni mutation de forêt. Elle ne construit aucun catalogue global de facettes ou de cofaces, aucune incidence ou composante Gamma, aucune cellule top-$m$ et aucune mosaïque de Delaunay d'ordre supérieur. Les neuf CTests GCC Release ciblés passent en 2,15 secondes lors du dernier rejeu : le test fonctionnel inclut autorités invalides, intérieur, frontière, extérieur, fixture silencieuse `AC`, différentiels bornés $n=6$ et $n=14$, deux ordres, neuf budgets et $K=10$; quatre gardes statiques et symboliques recertifient les dépendances et l'isolation du nouveau target. Son pire cas visite encore linéairement le LBVH et sa sortie de co-minimiseurs peut être linéaire; ni le SLO 50 k sous la seconde, ni le profil 10 M+ ne sont qualifiés.

Le jalon `10.7-RCPU` est validé sur l'hôte avec `backend=reference_cpu`, `profile=hgp_reduced`, `mode=certified` et sémantique `partial_refinement`. Sa porte locale est satisfaite par les rejeux validés de 10.2, 10.4 et 10.6 sous des autorités concordantes. Pour chaque selle directe $S_e$ et chaque $v\in S_e$, il reconstruit l'occurrence $F_{e,v}=S_e\setminus\lbrace v\rbrace$, conserve sa provenance, puis déduplique par clé complète. Si $J$ est le nombre de selles, $R$ le nombre d'occurrences et $D$ celui des clés distinctes, le contrat impose exactement $R=\sum_e\lvert S_e\rvert\leq11J$ et $D\leq R$.

Chaque clé distincte appelle 10.6 exactement une fois. Le résultat complet conserve un `facet_token` par clé $F$, avec $\lambda(F)$, puis un `gateway_candidate` factorisé par paire $(F,x)$ et des lots canoniques `(cardinal, niveau exact)`. Comme $S_e=F_{e,v}\cup\lbrace v\rbrace$, chaque provenance vérifie $\lambda(F_{e,v})\leq a_e=\beta(S_e)$ et reçoit une disposition stricte antérieure ou égale au lot direct; une valeur supérieure ou `complete_no_coface` est contradictoire. Les cofaces logiques de onze points restent factorisées : aucune clé persistante de largeur onze n'est créée. Toute autorité divergente, toute requête 10.6 incomplète ou tout budget global épuisé supprime atomiquement les cinq arènes scientifiques.

Les preuves de l'énumération directe, des bornes, de $\lambda(F_{e,v})\leq a_e$ et de la complétude des candidats relativement à ces seules clés sont `proved_here`; l'implémentation est `validated_host_software`. Le dernier rejeu GCC Release ferme 11 CTests en 3,84 secondes : différentiel explicite borné, résultat vide, provenance partagée, cas strict et égal, fixture `AC`, frontière $K=10$, deux ordres LBVH, huit plafonds globaux et neuf plafonds 10.6 exacts puis moins-un, falsifications, garde statique et audit symbolique. La sortie logique reste $O(R+KD+C)$ pour $C=\sum_F\lvert M(F)\rvert$, avec une seule scratch 10.6, sans univers $\binom{n}{k}$, catalogue de cofaces, Gamma, cellule, mosaïque d'ordre supérieur, locator, quotient, racine, union, forêt ou `GatewayAttach`. Le pire cas $O(Dn)$, la possibilité $C=\Theta(Dn)$, les rationnels sans budget de limbs et l'absence de reprise interdisent toute qualification 50 k sous la seconde ou 10 M+. Voir [PREMIERES_INCIDENCES_FACETTES_DIRECTES_PHASE10.md](math/PREMIERES_INCIDENCES_FACETTES_DIRECTES_PHASE10.md).

Le jalon `10.8-ORACLE` est validé sous `backend=reference_cpu_oracle`, `profile=hgp_reduced` et `mode=bounded_differential`; sa porte locale est satisfaite et son suivi est `validated_oracle_software`. À toutes les coupes exactes ouvertes et fermées, il compare séparément la suffisance de l'alphabet $V_k=B_k\cup\mathcal{F}_{\mathrm{dir}}$ puis, après projection sur ce même $V_k$, celle des seules selles directes complétées par les cofaces minimisantes 10.7 et toutes leurs suppressions transitoires. La couverture se calcule sur les composantes complètes de référence et de relais, tandis que la généalogie cumulative porte sur toute la suite des coupes. Les 15 tests dédiés couvrent E5, son ablation, les falsifications typées, huit cas $k=2$, quatre cas $k=3$ et la frontière $n=11$, $k=10$; aucun des treize cas non ablatés ne contredit les deux hypothèses. Gamma, ses catalogues et ses chemins restent exclusivement dans `reference/` et `tests/oracle/`; ce succès signifie `open_bounded_evidence_only`, jamais preuve générale. Voir [DIFFERENTIEL_GATEWAYS_DIRECTS_PHASE10.md](math/DIFFERENTIEL_GATEWAYS_DIRECTS_PHASE10.md).

Le jalon `10.9-RCPU` valide sur l'hôte la localisation produit bornée des candidats 10.7 avec `backend=reference_cpu`, `profile=hgp_reduced`, `mode=certified`, sémantique `partial_refinement` et suivi `validated_host_software`. Il reconstruit transitoirement les au plus onze suppressions de chaque paire factorisée $(F,x)$, déduplique globalement leurs clés complètes de largeur au plus dix et sonde chaque clé distincte exactement une fois dans un locator positif 10.5a gelé. Les deux seules arènes persistantes sont les projections occurrence--token et les tokens positifs relatifs ou latents non résolus; tout budget ou épuisement de sonde échoue atomiquement sans convertir un miss en isolation. Les caps agrégés bornent directement chaque sonde par leur reliquat et les compteurs sont engagés ensemble.

Le résultat conserve le lot source de chaque projection mais ne prétend pas que le snapshot courant du locator correspond à l'état pré-lot historique. `locator_snapshot_batch_level_alignment_claimed` reste faux : 10.9 fournit une localisation spatiale relative, pas encore l'autorité temporelle nécessaire à une mutation de quotient. Il ne crée ni singleton, union, racine, forêt ou `GatewayAttach`, ne matérialise aucune clé de coface de onze points et ne dépend d'aucun symbole Gamma. Les douze CTests GCC Release de la chaîne sparse passent en 18,36 secondes; les deux contrôles dédiés Clang 18 passent en 4,03 secondes, puis installation/export et consumer externe passent aussi. Voir [LOCALISATION_CANDIDATS_GATEWAYS_PHASE10.md](math/LOCALISATION_CANDIDATS_GATEWAYS_PHASE10.md).

Le jalon `10.10-RCPU` ouvre l'historique interne du locator sans en copier un snapshot par lot. Avec `backend=reference_cpu`, `profile=hgp_reduced`, `mode=certified` et sémantique `partial_refinement`, il reçoit des requêtes triées par `committed_batch_prefix_count` et initialise une seule DSU identitaire. Une passe de préflight lit les $B$ records requis et certifie leurs sommes; le curseur monotone relit ensuite chacun de ces records une fois pour appliquer sa transition et consomme chaque union une seule fois, soit exactement $2B$ scans de records de lots. Pour le préfixe $p$, les sommes durables déterminent exactement $I_p$ liaisons et $U_p$ unions actives. La table finale fixe est sondée comme une table historique : un slot inoccupé ou dont l'indice de liaison est au moins $I_p$ est un vide logique. Le vérificateur 10.5a recertifie séparément que les destinations physiques sont bien celles obtenues en rejouant les insertions dans l'ordre committé.

Le coût propre du sweep est $O(H+B+U+V+J+R+K(Q+C))$, où $C\leq V$ est le nombre de comparaisons de clés complètes, avec scratch $O(H)$ et sortie $O(Q)$. Les budgets couvrent séparément scans, unions, sauts DSU, slots, points de clés et sortie, et tout arrêt supprime atomiquement les résolutions. La recertification structurelle possède son propre budget de tailles, scratch et parcours variables; elle précède la reconstruction du sweep et n'appartient pas au chemin SLO. Ce chemin ne copie pas `component_parents`, n'appelle pas `apply_batch` ou la sonde courante, et ne construit ni Gamma, catalogue de cofaces, quotient, forêt ou attache.

10.10 certifie l'horloge de commits 10.5a, pas celle des lots sources 10.7. Le locator ne persiste aucun `source_batch_index`, niveau exact ou digest de lot amont; `source_batch_alignment_claimed` reste donc faux pour ce jalon pris isolément. 10.11-CLOCK ajoute désormais le certificat explicite entre ces deux horloges avant toute mutation pré-lot du quotient, mais sa conclusion reste conditionnelle au rejeu séparé de l'autorité externe. La recertification qui inclut PSTAMP passe quatorze CTests GCC Release en 9,03 secondes, quatre contrôles Clang 18 en 0,30 seconde, deux tests fonctionnels GCC ASan/UBSan en 0,06 seconde, puis l'installation, l'export et le consumer externe 1/1 en 0,01 seconde. Voir [SONDE_PREFIXE_LOCATOR_POSITIF_PHASE10.md](math/SONDE_PREFIXE_LOCATOR_POSITIF_PHASE10.md).

Le sous-jalon `10.11-PSTAMP` ajoute `ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult` au même target que 10.5a. Une suite non décroissante de $Q$ préfixes de commits, répétitions comprises, produit exactement $Q$ `LocatorSnapshotStamp`. Le stamp de rang $p$ contient l'autorité du locator, le nombre de commits, les cumuls de liaisons uniques, d'unions demandées et de demandes de liaison, puis le digest après exactement $p$ transitions. Le digest initial et la transition séparée par domaine sont ceux de 10.5a : le commit, le vérificateur structurel et PSTAMP ne maintiennent aucune seconde transcription canonique. Un commit vide modifie donc le stamp historique.

L'appel est strictement en lecture seule sur un locator maintenu gelé par l'appelant. Une première passe somme les besoins jusqu'au plus grand préfixe $B$; une seconde rejoue la transition partagée. Il y a exactement $2B$ scans de records, $M$ scans de slots si $I_B>0$ et zéro sinon, puis exactement $U_B$ records d'union, $I_B$ liaisons et $P_B$ identifiants de points rejoués. Le coût propre est $O(Q+2B+M\mathbf{1}_{I_B>0}+U_B+I_B+P_B)$, le scratch $O(I_B)$ et la sortie $O(Q)$. Les huit caps couvrent requêtes, records, slots, indices scratch, records d'union, liaisons, points et octets scratch; toute insuffisance ou histoire mal formée vide atomiquement la sortie. Aucun DSU, parent de composante, snapshot complet par lot, Gamma, cellule, coface globale, quotient, forêt ou attache n'est construit.

PSTAMP reconstruit seulement l'identité de l'état interne du locator; il ne reçoit ni lot 10.7, ni `source_batch_index`, ni niveau exact, ni digest amont. Il ne revendique donc aucune fonction source-vers-locator, aucune simultanéité et aucun `source_batch_alignment_claimed`. 10.11-CLOCK apporte maintenant le fait commun émis à la frontière par l'orchestrateur; deux digests internes valides ne suffisent toujours pas sans l'ancre externe et son rejeu. L'incrément PSTAMP est `validated_host_software` sous `backend=reference_cpu`, `profile=hgp_reduced`, `mode=certified`, sémantique `partial_refinement` et `public_status=not_claimed` : la chaîne sparse GCC Release passe 14/14 CTests en 9,03 secondes, les quatre contrôles locator et 10.10 sous Clang 18 passent en 0,30 seconde, les deux tests fonctionnels GCC ASan/UBSan en 0,06 seconde, puis installation, export et consumer externe passent 1/1 en 0,01 seconde. Les deux CTests PSTAMP dédiés passent en outre en 0,22 seconde. Voir [RACCORD_HORLOGES_SPARSE_PHASE10.md](math/RACCORD_HORLOGES_SPARSE_PHASE10.md).

Le sous-jalon `10.11-CLOCK` est validé avec `backend=reference_cpu`, `profile=hgp_reduced`, `mode=certified`, sémantique `partial_refinement` et `public_status=not_claimed`. Il introduit d'abord une identité scientifique indépendante pour le journal 10.7. L'encodage canonique engage schéma, ordre LBVH, compteurs sources, quatre digests amont et chacun des champs des cinq arènes scientifiques, audits 10.6 compris. Pour $R$ projections, $D$ tokens, $C$ candidats, $S$ lots, $I$ références de tokens et $E$ octets décimaux d'`ExactLevel`, la taille du payload hors domaine est exactement $194+75R+313D+66C+48S+8I+E$. Les caps de populations sont vérifiés avant les arènes, la taille binaire de chaque entier exact avant sa conversion décimale, puis le cumul décimal et le payload avant le hash.

Le certificat conserve cette identité, le stamp final du locator et une frontière dense par lot source. La frontière $s$ désigne le préfixe strict pré-lot $p_s$ du locator et son stamp historique, avec $0\leq p_s\leq T$. La suite $(p_s)$ peut être non monotone et contenir des répétitions. Le payload du certificat hors domaine vaut exactement $136+92S$ octets et engage aussi l'identifiant et le token non nuls de l'autorité externe; l'ancre séparée porte ces mêmes champs et le digest attendu. Le digest tout-zéro reste une donnée distinguée de son booléen de présence.

Le vérificateur préflighte les frontières, les deux scans et les scratches, puis rejoue fraîchement 10.7, exécute le vérificateur structurel borné complet du locator, recalcule les deux digests et trie les couples `(préfixe, source)` en consommant le budget avant chaque comparaison. Il appelle PSTAMP exactement une fois sur les préfixes triés avant de remapper les stamps vers les lots sources. Le stamp final doit égaler l'état gelé à l'entrée et à la sortie. Les budgets propres de l'identité, du digest, de la structure 10.5a et de PSTAMP restent imbriqués mais séparés. La conclusion conserve `external_clock_authority_replayed=false` et `conditional_on_caller_clock_authority_replay=true` : elle certifie conditionnellement la table source-vers-préfixe, pas le rejeu de l'orchestrateur, la composition durable de ses tokens, ni une transaction du quotient.

La recertification courte passe 16/16 CTests GCC Release en 7,69 secondes et les deux CTests CLOCK dédiés sous Clang 18 en 0,45 seconde. Elle couvre chaque champ des cinq arènes, les dix-huit compteurs 10.6, les tailles canoniques, tous les caps exacts puis moins-un, un `ExactLevel` hostile de 131 072 bits arrêté avant conversion décimale, les préfixes non monotones ou répétés, les stamps, autorités, tokens, digests et rehashs étrangers, le digest nul explicitement présent et l'immutabilité complète des deux entrées. Aucun benchmark long, sanitizer, GPU ou GCP n'a été exécuté.

Le sous-jalon `10.12-AUTH` ajoute l'autorité productrice qui manquait à CLOCK. L'ouverture engage l'identité scientifique 10.7 et le stamp initial du locator avant toute capture. Le journal est préalloué, mono-writer, non copiable et move-invalidant; chaque capture lit elle-même le stamp vivant, impose un indice source unique, des préfixes chronologiques non décroissants et l'égalité du stamp complet à préfixe égal. La chaîne SHA-256 séparée par domaines engage l'ouverture et chaque record. Le scellement exige la permutation complète des $S$ lots, remappe la chronologie vers l'ordre source et produit exactement un certificat CLOCK.

Le vérificateur AUTH rejoue fraîchement l'ouverture, la chaîne, la couverture, la monotonie et le scellement, puis compose le vérificateur CLOCK complet. Son seul succès supplémentaire est `external_clock_authority_replayed=true` avec la condition externe de CLOCK levée dans cette session mémoire. Il ne prouve ni verrouillage effectif hors du protocole, ni discipline scientifique pré-lot sans l'hypothèse d'orchestration, ni durabilité après crash. Son coût propre et son stockage sont $O(S)$, sans snapshot du locator par lot, copie des parents DSU, Gamma, cellule, coface globale, mosaïque de Delaunay d'ordre supérieur, quotient, forêt ou attache.

Le sous-jalon `10.13-TRES` consomme cette autorité sans muter le locator. Chaque projection 10.9 devient le couple exact `(préfixe pré-lot du lot source, token localisé)`; un heapsort borné déduplique ces couples et un seul sweep 10.10 restitue leurs dispositions historiques. Deux arènes plates conservent les $P$ références et les $Q\leq P$ résolutions sans recopier les clés. Un hit historique garde le témoin final mais peut avoir une racine historique différente; un hit final peut être latent avant son lot. Le fresh verifier rejoue localisation, AUTH et sweep, tout en conservant comme prémisses l'autorité scientifique externe des témoins, le gel synchronisé et la discipline pré-lot. Coût propre $O(S+P\log P+KP+KQ)$, stockage scientifique $O(P+Q)$; aucun snapshot ou DSU par lot, Gamma, catalogue global, cellule, mosaïque, quotient, forêt ou attache.

Le sous-jalon `10.14-QPROP` ferme ensuite, séparément dans chaque lot source, l'hypergraphe des seuls candidats fournis. Une projection historiquement positive devient le sommet typé `R(racine historique)`; une projection latente devient `L(indice de résolution temporelle)`. Les espaces de noms restent disjoints et les facettes latentes partagées raccordent correctement plusieurs hyperarêtes candidates avant la projection sur les seules racines connues. Les candidats répétés restent des hyperarêtes idempotentes : aucune déduplication de cofaces, clé persistante de onze points ou catalogue global n'est requis.

La sortie read-only possède cinq arènes plates pour les lots, les liaisons candidat--composante, les composantes, leurs racines et leurs indices de résolutions latentes. Une composante sans racine reste `latent_only_unresolved`; une ou plusieurs racines donnent seulement une classe de racines connues, jamais une naissance, une continuation ou une multifusion. Le fresh verifier rejoue d'abord 10.13 puis reconstruit toute la clôture typée. Avec $C$ candidats et $P\leq11C$ projections, le coût propre est $O(P\log P+(P+C)\alpha(C))$; scratch et sortie sont linéaires. Les trois heapsorts partagent un plafond de comparaisons et le DSU candidat possède un plafond distinct de sauts de parents. Aucun snapshot DSU par lot, Gamma, cellule, coface globale, mosaïque, union du locator, forêt ou `GatewayAttach` n'est construit.

Le sous-jalon `10.15-LIFE` réduit ensuite le pic de durée de vie des seules grandes arènes transitoires de 10.13, sans changer son schéma, ses budgets, ses sorties ni QPROP. Le builder construit désormais simultanément la table projection--résolution et les $Q$ résolutions compactes, libère explicitement le scratch trié de $P$ triplets, puis lance le sweep historique. Il ne rescane donc plus ce scratch après le sweep. La borne $P\leq11C$ et toute la sémantique scientifique restent inchangées.

Sur l'ABI LP64 courante, les tailles logiques sont 24 octets par triplet trié, 16 par référence de projection, 104 par requête de préfixe, 48 par résolution du sweep, 72 par résolution temporelle et 8 par parent DSU. En notant $H$ le nombre de handles du sweep, le pic ancien vaut $\max(40P+224Q,\ 40P+152Q+8H)$ octets et le nouveau $\max(40P+176Q,\ 16P+224Q+8H)$ octets. Ces formules excluent les en-têtes de vecteurs, l'arrondi des capacités et les métadonnées d'allocateur. Pour $P=Q=H=10\,000\,000$, le payload logique descend de 2,64 à 2,48 Go décimaux, soit 160 MB gagnés.

Cette réduction concerne le builder produit. Le fresh verifier conserve volontairement les requêtes et le résultat du sweep dans `BuildArtifacts`, puis reconstruit le sweep pour sa preuve imbriquée; il ne bénéficie donc pas de la borne de pic acquise ici. Comme `warm_e2e` devra inclure la validation fraîche, ces artefacts devront être streamés avant toute qualification SLO. 10.15 ne crée aucune structure globale, ne modifie aucun résultat scientifique et ne qualifie ni 50 k sous la seconde ni 10 M+.

Les incréments 10.6--10.15 restent `partial_refinement`, sans statut public, mais leur ancienne tentative de compléter un sous-univers historique de gateways est désormais non normative. La chaîne candidate reprend directement 10.1, 10.2, 10.5a et 10.5c : pour chaque lot, elle localise au plus quatre bras stricts par simplexe de Gabriel dans le snapshot gelé, exige un terminal positif issu d'un minimum strictement antérieur, ferme l'hypergraphe des carriers, puis compte les seules racines réduites. À l'ordre au moins deux, un minimum isolé n'est pas une naissance réduite; $q_R=0$ crée une naissance, $q_R=1$ continue et $q_R\geq2$ fusionne. Tous les carriers du groupe sont unis avant l'insertion des minima courants. Les facettes intérieures égales et les niveaux non Gabriel ne sont ni des bras, ni des parents de forêt, mais leur ancienne information d'attache doit être récupérée par la descente; la surjectivité de cette récupération reste à prouver. M.1 demeure une obligation de la Phase 12 et aucune qualification SLO 50 k ou 10 M+ ne découle de cette réduction.

### Tests

- journal direct contre Gamma exhaustif seulement pour $n\leq14$;
- fixture `gabriel-point-set-counterexample-5-points-v1`, avec désaccord exact attendu et sérialisé;
- oracle local 10.6 contre toutes les cofaces à un point sur petits nuages, avec co-minimiseurs égaux, deux ordres LBVH, neuf budgets exacts et moins-un, frontière $K=10$ à 770 examens source et 176 supports extérieurs;
- journal 10.7 contre les premières incidences exhaustives des seules facettes de selles directes pour $n\leq14$, avec déduplication de provenances, relations stricte et égale, lots `(cardinal, niveau)`, publication atomique, fixture `AC` vers $D,E$ au niveau $33/2$ et absence de clé persistante de onze points;
- localisation 10.9 sur E5 avec `AC` partagé entre `ACD` et `ACE`, hits et misses latents sous collisions forcées, puis frontière $K=10$ où 121 occurrences se dédupliquent en onze clés de cardinal dix; budgets exacts et moins-un, stamp périmé, falsifications et isolation symbolique;
- PSTAMP 10.11 sur les préfixes zéro à six du locator à collisions forcées de 10.10, avec répétitions, commit vide, doublon compatible, préfixe final égal au stamp vivant, huit caps exacts puis moins-un, entrées décroissantes ou hors histoire, mutation hostile et immutabilité complète du locator;
- CLOCK 10.11 sur l'identité de tous les champs et des cinq arènes 10.7, les vecteurs canoniques et les formules $194+75R+313D+66C+48S+8I+E$ et $136+92S$, chaque cap exact puis moins-un, un `ExactLevel` hostile arrêté par son cap binaire, des préfixes non monotones ou répétés, lots omis ou dupliqués, commit vide, suffixe corrompu après le plus grand préfixe, stamp final périmé, schémas, autorités, tokens, digests et rehashs hostiles, digest nul avec présence explicite, source ou locator étranger, gel et immutabilité complets; aucune campagne ne peut reclasser la conclusion conditionnelle en rejeu d'autorité, décision de quotient, statut public ou qualification SLO;
- AUTH 10.12 sur une capture source chronologiquement non monotone mais une horloge locator non décroissante, deux captures au même stamp, un commit vide, le rejet atomique d'un lot dupliqué et d'un préfixe décroissant, le scellement unique, l'interdiction de capture après scellement, les caps exacts puis moins-un, l'immutabilité des autorités et les faits explicites `in_memory_replay_only=true`, `crash_durable=false`;
- composantes avant et après chaque lot et attache silencieuse réutilisée plus tard;
- vraie fixture E5 tridimensionnelle : `AC` atteint la composante du minimum `DE`, puis `ABC` reste une continuation $q_R=1$ au niveau $83886/3563$;
- multifusions et niveaux égaux;
- recouvrements de points;
- EMST pour $k=1$.

### Porte de sortie

La porte logicielle locale est satisfaite lorsque le flux direct rejoué fournit tous les minima et tous les simplexes de Gabriel de sa portée, que chaque bras strict est certifié vers un carrier strictement antérieur sous un snapshot commun, que les minima isolés d'ordre au moins deux restent sans nœud, que chaque lot égal applique $q_R=0,1,\geq2$ avant mutation et que le journal compact conserve minima, liaisons, naissances réduites, continuations, multifusions, enfants et racines finales. Toute terminaison non positive, autorité divergente ou capacité insuffisante doit échouer sans action scientifique.

Le sweep 6.23 et Gamma exhaustif restent des falsificateurs bornés : toute contradiction devient une fixture permanente, mais leur accord ne promeut jamais le journal. La Phase 10 est fermée administrativement comme implémentation candidate conditionnelle afin d'ouvrir Phase 14 en `architecture_only`; la fidélité globale des carriers reste `proof_obligation`. Cette fermeture ne démontre ni `full_pi0`, ni M.1, ni la tour verticale, et ne change aucun `public_status` v2.

## Phase 11 — Tour verticale

### But

Transformer dix forêts en une hiérarchie ordre–échelle cohérente.

### Travaux

- construire les cibles exactes de référence par inclusion directe dans Gamma;
- conserver `locate_reduced_root(k,Q,a)` comme candidat par remplacement intrus–support strictement descendant;
- choisir canoniquement l'intrus et le support, enregistrer chaque facette partagée et vérifier $\beta(Q')<\beta(Q)$;
- traiter par pointer-jumping les chaînes indépendantes jusqu'à un simplexe de Gabriel sans promouvoir leur racine brute en cible exacte;
- créer l'ancre d'un événement rang $s$ entre sa naissance dans $T_s$ et l'état post-lot de $T_{s-1}$;
- définir le comportement des nœuds réduits qui n'existent pas encore comme composantes non triviales;
- propager les images le long des forêts;
- représenter les applications comme tableaux compacts;
- vérifier tous les carrés de naturalité;
- exposer une requête de suivi d'une composante en $k$ et $a$.

### Journal direct compact 11A

La branche exacte de référence reste celle de Phase 1 : Gamma exhaustif fournit les cibles uniques et vérifie les carrés entre toutes les coupes ouvertes et fermées consécutives. 11A ne duplique pas cet oracle dans le produit. Il construit une couture `reference_cpu / hgp_reduced / certified`, `architecture_only`, depuis le journal horizontal direct conditionnel.

L'unité de requête est chaque `strict_arm_key` distincte de chaque groupe atomique, y compris les continuations $q_R=1$. Après tri et déduplication locale, la requête conserve seulement le plus petit `arm_root_binding_index` représentant la clé; aucun `PointId` n'est recopié. Une proposition externe peut être résolue, non résolue ou absente. Le journal normalise toute graine valide vers la racine active de l'ordre inférieur dans l'état fermé du même niveau exact; une cible future, de mauvais ordre ou contradictoire ferme atomiquement la construction.

Les groupes $q_R=0$ ancrent leur nœud lorsque tous les labels sont résolus et concordants. Les continuations propagent le dernier checkpoint et peuvent créer un checkpoint tardif sans rétro-certifier le passé. Les groupes $q_R\geq2$ comparent les images propagées de tous leurs enfants et de tous leurs labels avant d'ancrer le parent. Les familles vides restent explicites. Chaque famille compte aussi les naissances sources isolées d'ordre supérieur omises du profil réduit; une cible absente reste non résolue et n'est jamais reclassée comme isolée. Les compteurs séparent labels manquants, non résolus et résolus, contrôles élémentaires vérifiés et non vérifiables, et contradictions.

Cette structure ne sérialise aucun `VerticalMap` v2 : ses identifiants de nœuds sont locaux et l'autorité des graines n'est pas rejouée. Même un journal conditionnellement complet garde `all_naturality_squares_replayed=false`, `vertical_maps_complete=false` et `public_status=not_claimed`. Il ferme une architecture conditionnelle de Phase 11, pas la porte G4 produit, qui reste `no-go` tant qu'une flèche nécessaire est absente ou non recertifiée. Voir [TOUR_VERTICALE_DIRECTE_PHASE11.md](math/TOUR_VERTICALE_DIRECTE_PHASE11.md).

### Tests

- oracle vertical exhaustif;
- événement partagé naissance–selle;
- lot égal dans deux ordres;
- composante qui devient non triviale plus tard;
- coface source non Gabriel dont aucune facette n'est initialement dans le DSU cible;
- deux labels sources adjacents qui doivent localiser la même racine;
- recouvrement de points;
- composition de plusieurs ordres.

### Porte de sortie

Chaque image Gamma de référence est unique et tous ses carrés commutent. Toute flèche issue de Gabriel brut est absente, partielle ou vérifiée indépendamment contre Gamma; le périmètre réduit est explicitement marqué lorsque la source ou la cible isolée n'appartient pas au profil.

À partir de cette porte, deux pistes sont indépendantes : les phases 12–13 ferment la cible topologique `full_pi0`, tandis que les phases 14–16 optimisent et diffusent `hgp_reduced`. Une piste ne peut revendiquer les garanties de l'autre et aucune n'attend artificiellement sa fermeture.

## Phase 12 — `full_pi0` sous position générale

### But

Ajouter les naissances isolées et leurs généalogies sans matérialiser $\Gamma_k$.

### Travaux

- créer un minimum pour chaque événement de rang $k$;
- énumérer les facettes actives $S\setminus\lbrace u\rbrace$, $u\in U$, de chaque selle;
- construire les $\lvert U\rvert$ bras, enregistrer $\Delta=\lvert U\rvert-1$ à l'indice un et compter les classes antérieures distinctes effectivement incidentes;
- calculer ou retrouver leur racine de descente;
- certifier le segment initial du germe;
- résoudre globalement les attaches dans l'état strictement antérieur;
- contracter le lot d'indice un;
- marquer les selles qui créent ou ne créent pas une fusion $H_0$;
- intégrer les composantes isolées aux morphismes verticaux;
- produire des chemins de replay.

### Preuve à compléter

Écrire la preuve formelle du contrat M.1, y compris :

- absence de changement de $H_0$ hors indices zéro et un;
- couverture de tous les germes par les facettes actives;
- correction du chemin miniball;
- gestion simultanée de plusieurs centres au même niveau;
- cohérence avec les inclusions verticales.

### Tests

- égalité avec $\Gamma_k$ à chaque intervalle critique;
- facettes isolées persistantes puis absorbées;
- selle sans fusion, fusion binaire et multifusion;
- supports $\lvert U\rvert=2,3,4$ donnant respectivement deux, trois et quatre bras, avec jusqu'à une, deux et trois classes $H_0$ tuées;
- bras partiellement déjà connectés puis tous déjà connectés, dernier cas sans fusion $H_0$ et potentielle création de $H_1$ hors sortie;
- multifusion canonique jamais binarisée par l'ordonnancement GPU;
- mêmes minima atteints par chemins différents;
- suppression volontaire d'un événement ou d'une attache doit faire perdre `exact`;
- permutation complète du planning GPU.

### Porte de sortie

Preuve relue, oracle exhaustif sans différence et zéro attache non certifiée. Alors seulement le profil `full_pi0` peut publier `exact` sous position générale.

## Phase 13 — Dégénérescences et multiplicités

### But

Étendre progressivement le domaine exact aux données réelles sans jitter.

### Sous-phase 13A — Doublons

- agréger les coordonnées identiques;
- stocker les multiplicités;
- adapter top-$k$, rang, labels et coupes;
- définir la restitution des occurrences;
- comparer à l'oracle multiensemble.

### Sous-phase 13B — Supports dimensionnels

- supports colinéaires ou coplanaires canoniques;
- barycentriques exactes dans l'enveloppe affine;
- réduction automatique d'un support non minimal.

### Sous-phase 13C — Cosphères

- construire l'arrangement directionnel sur $S^2$ autour du centre;
- identifier les germes où au moins $k-\lvert I\rvert$ points frontière restent sous le niveau;
- prouver l'isotopie locale;
- produire l'hyperévénement dégénéré;
- traiter ses plateaux d'attache par quotient multivalué.

### Repli partiel explicite

Tant qu'une sous-phase dégénérée n'est pas prouvée, `mode=certified` retourne `public_status=unsupported_degeneracy`, omet `forest_semantics`, conserve seulement les événements vérifiés et les loci non résolus dans `PartialScope`; `require_exact=true` lève une erreur. La combinaison `mode=budgeted, forest_semantics=partial_refinement` peut en plus produire une forêt partielle sur les profondeurs et ordres fermés; elle retourne `public_status=conditional` ou `public_status=budget_exhausted` et ne permet aucune assertion d'absence. Aucun jitter, aucune perturbation symbolique non contractée et aucune suppression silencieuse ne sont autorisés.

### Porte de sortie

Chaque sous-phase possède son théorème, son oracle et ses tests. Une sous-phase inachevée reste explicitement `unsupported_degeneracy`; elle ne bloque pas le profil générique.

## Phase 14 — Latence 50 000 points

### But

Atteindre ou réfuter proprement la cible principale de moins de 100 ms pour le passage complet du nuage en protocole `warm_e2e`; moins d'une seconde est seulement l'objectif secondaire.

### Préconditions

- pour la **qualification finale**, profil `hgp_reduced` certifié par le flux direct et le journal des Phases 9--11;
- pour l'**architecture industrielle non promotionnelle**, Phase 10 fermée sur sa réduction Morse horizontale conditionnelle; la Phase 11 et M.1 restent obligatoires avant toute qualification finale;
- prédicats et flux direct corrects dans leur portée annoncée;
- aucun JIT;
- runtime et allocateur initialisés, mais nuage encore en mémoire hôte;
- instrumentation NVTX complète.

### Socle industriel 14A

Le même moteur scientifique sert les deux régimes. Pour 50 k, le plan résident exige que tous les lots, le locator, les descentes et la forêt tiennent dans une seule enveloppe. Pour 10 M+, le plan streaming coupe seulement entre des lots exacts complets et pose une frontière de run/checkpoint après chaque chunk. Le planificateur compte les minima, simplexes de Gabriel, bras, références de clés et bornes conservatives de nœuds et enfants depuis 10.1--10.2, puis applique un modèle mémoire explicite et une réserve de descentes fournie par l'appelant.

Ce socle est `architecture_only` et `public_status=not_claimed` : accepter un plan ne mesure ni latence, ni débit, et ne qualifie aucun volume. La décision complète, les structures évitées et les goulots prioritaires sont dans [ARCHITECTURE_INDUSTRIELLE_MORSE_H0.md](math/ARCHITECTURE_INDUSTRIELLE_MORSE_H0.md).

### Transaction sparse 14B

Le locator remplace la copie de ses $H$ parents par un journal de rollback réservé à $U_b$ handles et utilisé par les seules $W_b$ unions effectives. Les lookups précèdent toujours la première écriture, les doublons voient le DSU post-unions, un succès conserve $W_b$ écritures et un rejet en restaure exactement $W_b$ en ordre inverse. Les diagnostics de travail restent hors du digest durable et une garde statique interdit le retour du clone dense. Le scratch de lot devient $O(Q+KB+U_b)$; aucun handle non touché n'est parcouru.

Cet incrément ferme le premier goulot commun aux profils 50 k et 10 M+ sans prétendre qualifier l'un ou l'autre. L'incrément suivant structure les descentes par lanes exactes avant leur exécution GPU, puis viendront les durées de vie des arènes et l'instrumentation.

### Lanes de descente 14C

Le planificateur `ExactDirectSparseFacetDescentBatchPlanResult` rejoue 14A sous un plafond explicite de chunks, contrôlé avant toute rétention supplémentaire et avec effacement du préfixe transitoire en cas de dépassement, puis divise chaque lot exact en au plus trois lanes selon le cardinal deux, trois ou quatre du support positif. Chaque lane reste dans son chunk et son lot, référence seulement les intervalles candidats de 10.2 et reçoit une tuile bornée de graines. Toutes les lanes d'un lot doivent utiliser le même snapshot locator, une seule sélection stable, une fermeture 10.5c commune avec une mémoïsation commune, puis une jointure par `arm_seed_index` avant le quotient.

Pour une facette de cardinal $k\leq10$, le nombre de supports locaux examinés par passe est $N_k=\sum_{j=1}^{\min(4,k)}\binom{k}{j}\leq385$. Quatre passes fraîches donnent une borne initiale autonome de $4rN_k$ pour une famille de support $r$. Le cas frontière $K=10$ avec support deux, neuf intérieurs et deux bras vaut 3080 examens. Cette borne ne couvre pas les successeurs de la fermeture partagée, la difficulté LBVH ou rationnelle, les octets combinés avec 14A ou un temps GPU.

Le prédicat interne reste un contrôle de forme; le résultat exige un rejeu frais avant exécution ou persistance. 14C est `architecture_only`, ne matérialise ni clés de lanes, ni facettes absentes, ni Gamma, cofaces globales, cellules ou mosaïque de Delaunay d'ordre supérieur, et ne qualifie ni 50 k sous la seconde, ni 10 M+.

### Exécuteur ancré 14D

`ExactDirectSparseFacetDescentAnchoredBatchExecutor` reconstruit et compare 14C une seule fois à l'ouverture de la session, conserve ce plan frais comme autorité et avance ensuite dans l'ordre canonique des lots. Pour chaque lot, il sélectionne les familles et les bras en une passe stable, reconstruit les seules clés réellement demandées, déduplique les $D\leq A$ clés complètes des $A$ bras, puis appelle une unique fermeture 10.5c sous un snapshot locator gelé. Les terminaux positifs sont immédiatement projetés vers un tableau compact clé--carrier--témoin et chaque `arm_seed_index` rejoint l'indice de sa clé résolue.

Le graphe, les arêtes, les projections de graines, les miniballs et la mémoïsation de 10.5c restent dans la portée locale de l'appel et sont détruits avant publication du delta. Le payload persistant d'un lot est donc $O(KD+A)$; il ne contient aucun indice de nœud transitoire. Un rejeu frais du seul lot courant compare exactement le delta avant d'avancer le curseur d'exécution. Cet avancement n'est ni le commit du quotient, ni une mutation du locator, ni une publication de hiérarchie.

14D est commun au profil résident et au profil streaming, mais ne suffit pas à les qualifier : le premier doit encore recevoir la voie de propositions GPU et le protocole `warm_e2e`; le second doit évacuer les deltas vers des runs et checkpoints durables au lieu de les accumuler. Aucune facette absente, Gamma, coface globale, cellule ou mosaïque de Delaunay d'ordre supérieur n'est construite, et aucune revendication 50 k, 10 M+ ou `public_status=exact` n'est faite.

### Couture canonique de fermeture 14E

L'exécuteur 14D possède déjà les $D$ clés initiales distinctes dans l'ordre canonique. La nouvelle entrée `build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys` consomme directement cette vue, vérifie sans allocation la validité, le cardinal commun et l'ordre strict des clés, puis attribue implicitement `seed_index=i`. L'API générale 10.5c reste disponible pour les graines désordonnées ou dupliquées et produit exactement le même résultat scientifique.

Sur ABI LP64, la voie précédente conservait pendant la fermeture un tableau 14D de $D$ clés, puis $D$ records de graines de 96 octets, une copie triée de ces records et un second tableau de clés de 88 octets. La couture directe supprime les trois dernières populations, soit $280D$ octets au pic, trois allocations, un tri d'identités et un tri-déduplication déjà effectués. Ce chiffre ABI-spécifique doit être remesuré ailleurs; la borne logique du delta reste $O(KD+A)$ et aucun graphe ne survit au lot.

La primitive top-$K$ bornée accepte en outre une vue facultative d'incumbents. Chaque `PointId` est contrôlé puis sa distance est recalculée exactement sous le cap existant; les incumbents initialisent seulement la heap et sa borne supérieure. La traversée LBVH reste complète, l'élagage reste strict et une borne égale descend toujours pour fermer toute la coquille. Cette primitive ne constitue pas encore un transcript de lot ni une voie GPU qualifiée : elle est la couture CPU exacte sur laquelle 14F branchera les propositions.

14E ne change aucune décision 10.5c, ne matérialise ni facette supplémentaire, ni coface globale, ni Gamma, ni cellule, ni mosaïque de Delaunay d'ordre supérieur, et ne qualifie ni temps ni volume.

### Transcript borné et pool exact 14F

Le contrat `direct_sparse_facet_top_k_proposal_transcript` reçoit un ensemble sparse de records triés strictement par clé complète. Chaque record porte de zéro à $K$ `PointId` distincts et aucune distance, aucun cutoff ni verdict géométrique. Cinq caps bornent avant copie le nombre de records, les références des clés, les candidats, les octets physiques et les entrées logiques. Toute forme invalide ou tout cap insuffisant laisse le payload vide; le domaine du nuage et les exclusions restent volontairement non certifiés jusqu'au point d'usage exact.

Pour une facette source $F$ de cardinal $K$ et une proposition $P$ d'au plus $K$ identifiants, la nouvelle primitive spatiale évalue exactement l'union dédupliquée $U=F\cup P$. Elle préflighte les $\lvert U\rvert\leq2K$ distances avant la première évaluation, conserve seulement les $K$ meilleurs couples distance--identifiant dans la heap, mais reconstruit la coquille avec tous les éléments de $U$ au cutoff. Comme $F\subseteq U$, le cutoff initial de $U$ n'est jamais supérieur à celui fourni par $F$; une proposition adversariale peut coûter jusqu'à $K$ distances exactes supplémentaires, mais elle ne peut ni dégrader cette borne géométrique, ni changer la partition finale. L'élagage demeure strict et toute égalité descend.

L'enveloppe 10.5c de 14F revalide atomiquement le lot, le niveau exact, le stamp locator vivant, les clés initiales, l'inclusion des clés de records et le domaine de chaque candidat avant de créer un `ClosureBuilder`. Un rejet ne construit aucune fermeture. Sur succès, chaque requête top-$K$ utilise $F$ comme baseline; un record non vide ajoute $P$, tandis qu'un record vide, une clé initiale absente ou toute clé d'abord atteinte comme successeur dynamique utilise $P=\varnothing$, même si cette clé possède aussi un record de seed. La fermeture scientifique demeure le résultat 10.5c habituel; un audit séparé compte les hits, fallbacks, tailles de pools, distances, visites, bornes AABB, élagages, scans complets et causes d'épuisement sans conserver transcript, partition ou coquille.

Ce premier raccord 14F reste hôte et n'est pas encore branché à la préparation/validation du curseur 14D. Il ne contient ni producteur CUDA, ni epoch GPU, ni digest de buffer, ni classe de difficulté utilisée par l'ordonnanceur et ne qualifie ni `warm_e2e`, ni 50 k, ni 10 M+. Le transcript sparse vaut $O(KR)$ pour $R\leq D$ records proposés et n'introduit aucune facette absente, coface globale, incidence, cellule, Gamma ou mosaïque de Delaunay d'ordre supérieur.

### Préparation synchrone 14G

`prepare_next_with_top_k_proposal_transcript` raccorde le transcript 14F au curseur ancré 14D sans modifier `commit_prepared`. L'exécuteur joint d'abord le plan, le chunk, le lot et les lanes, contrôle les caps de sélection, reconstruit les bras puis les $D$ clés canoniques distinctes; il ne consomme le transcript qu'après ces préflights. Un cap insuffisant garde donc la priorité et ne publie ni audit propositionnel, ni delta.

La consommation reste synchrone sous le snapshot locator gelé. Un transcript périmé ou invalide donne une enveloppe de rejet atomique avec zéro fermeture, zéro delta et un curseur inchangé. Le raccourci scientifique du lot vide reste sans fermeture dans le delta 14D, mais un transcript explicitement fourni y est tout de même revalidé et son audit séparé enregistre une construction de fermeture vide. Sur succès, l'enveloppe conserve seulement le delta scientifique 14D inchangé et l'audit scalaire 14F; elle ne possède aucun record, candidat, graphe, partition ou coquille.

Le transcript peut être détruit dès le retour de la préparation. Le commit historique reçoit uniquement le delta, reconstruit le lot courant par la voie exacte non amorcée et exige l'égalité complète avant d'avancer. Les propositions vides, utiles ou adversariales peuvent donc modifier le travail observé, jamais l'autorité du commit. Une préparation complète ne certifie toutefois pas que ce rejeu sans proposition tient sous les mêmes caps : un budget situé entre les travaux amorcé et non amorcé peut encore bloquer l'avancement sans compromettre la sûreté. Fermer cette dette de vivacité sans donner d'autorité à l'audit est le prochain verrou transactionnel.

Le delta logique vaut $O(KD+A)$, le scratch de sélection formé par `SelectedArm` et les clés distinctes vaut $O(KA+KD)$, et le transcript fourni par l'appelant vaut $O(KR)$ avec $R\leq D$. Ces objets peuvent coexister avec l'unique fermeture 10.5c transitoire; 14G ne qualifie donc aucun pic mémoire global.

14G reste hôte, `architecture_only` et `public_status=not_claimed`. Il n'apporte encore ni producteur CUDA, ni epoch ou digest de buffer, ni scheduler de difficulté, ni arène réutilisable, ni protocole `warm_e2e`, ni runs/checkpoints durables, et ne qualifie ni 50 k sous la seconde, ni 10 M+.

### Ticket scellé de commit 14H

`prepare_next_sealed_with_top_k_proposal_transcript` transforme uniquement une préparation 14G complète en capacité privée, non copiable et à usage unique. Le ticket possède le delta exact compact, une identité de session partagée non réutilisable, l'epoch courant, le curseur source complet, son successeur déjà validé et le stamp locator observé. Son constructeur est privé, ses vues scientifiques sont constantes et son déplacement neutralise explicitement la source; aucun résultat public 14G modifié ne peut fabriquer cette autorité.

`commit_prepared_ticket` consomme le ticket même en cas de rejet. Il exige le même sceau de session, le même epoch, les cinq composantes du curseur source et le même stamp locator vivant, puis affecte le curseur successeur prévalidé. Il ne relance ni top-$K$, ni 10.5b, ni 10.5c, ne consulte ni transcript ni audit pour décider, ne recalcule aucun digest et ne parcourt pas le delta. Le commit est donc constant dans la taille du payload préparé et ne peut plus épuiser le budget géométrique qui a déjà produit le ticket. Le chemin historique à rejeu frais reste disponible et invalide tout ticket spéculatif antérieur en avançant lui aussi l'epoch.

L'autorité de 14H est une provenance exacte locale au processus, pas un nouveau vérificateur indépendant. Une mutation du locator, un autre commit, un ticket étranger, déplacé ou réutilisé ferme l'opération sans avancement. L'audit opérationnel peut être extrait, modifié ou détruit avant le commit; s'il reste attaché, il est seulement déplacé vers le résultat. La session ne retient ni ticket, ni delta, ni record, ni candidat, ni fermeture. Les tickets simultanément conservés restent en revanche de la mémoire appartenant à l'appelant et devront être plafonnés par le futur ordonnanceur.

14H ferme la dette de vivacité préparation--commit sous un même cap, mais avance seulement le curseur architectural 14D. Il ne mute ni locator, ni quotient, ni hiérarchie, ne survit pas à une perte de processus et ne construit toujours aucune facette absente, coface ou incidence globale, cellule, Gamma ou mosaïque de Delaunay d'ordre supérieur. Son statut reste `reference_cpu / hgp_reduced / certified`, déploiement `architecture_only`, `public_status=not_claimed`, sans qualification du producteur CUDA, du SLO 50 k ou du régime 10 M+.

### Producteur GPU borné par fenêtres Morton 14I

14I implémente un contexte `cuda_g4 / hgp_reduced / proposal_only` au-dessus du snapshot Morton immuable du nuage. La capacité du snapshot device, allouée paresseusement au premier lot GPU supporté, contient trois mots binary64 de coordonnées par `PointId` et un `PointId` par position Morton, soit exactement $32n$ octets. L'hôte conserve ces mêmes $32n$ octets ainsi qu'un inverse de $n$ entrées `size_t`; sur la cible G4 64 bits, ce staging vaut donc $40n$ octets en plus du nuage et du LBVH. Les positions utiles sont injectées dans chaque record de requête, sans seconde table inverse de $8n$ octets sur le device.

Pour chaque facette de cardinal $k\leq10$, un kernel inspecte au plus $W$ voisins à gauche et $W$ à droite de chacun de ses sommets. Les fenêtres sont bornées séparément : un point couvert plusieurs fois peut être relu, mais le nombre d'occurrences reste au plus $2kW$ par clé. Après exclusion de la source et déduplication, le kernel sélectionne au plus $k$ candidats selon leur distance carrée flottante au projeté binary64 du centre exact fourni, puis les émet dans l'ordre canonique croissant des `PointId`. Ni cette sélection, ni l'epoch, ni le digest du buffer ne certifient le rappel, un cutoff ou une descente. La projection actuelle effectue jusqu'à environ 63 comparaisons rationnelles par coordonnée, soit environ 189 par requête sans cache; ce coût hôte reste une dette explicite avant tout SLO 50 k.

La construction du centre exact reste hors kernel. À $k\leq10$, elle peut examiner jusqu'à $\sum_{j=1}^{4}\binom{k}{j}\leq385$ supports par clé, puis 10.5c peut payer de nouveau ce travail lors de sa recertification. Pour une capacité physique $C$ et $D\leq C$ requêtes, le device réserve $208C+144C=352C$ octets pour les records d'entrée et de sortie; chaque appel copie $208D$ octets d'entrée, initialise puis recopie vers l'hôte toute la capacité de sortie de $144C$ octets, et le transcript utile vaut $O(kD)$. Le coût additionnel réel est donc $O(n+C+kD)$, et non $O(kD)$ sans l'hypothèse $C=O(D)$; le découpage doit choisir un $C$ borné et proche de la taille des chunks. Aucun tableau $D\times n$, catalogue global de facettes ou cofaces, incidence globale, Gamma, cellule ou mosaïque de Delaunay d'ordre supérieur n'est construit.

La frontière scientifique ne change pas. La sortie GPU alimente seulement le transcript 14F; le CPU exact revalide clés, namespace, exclusions et `PointId`, recalcule les distances au centre exact, utilise toujours $F$ comme baseline et achève la traversée LBVH avec prune strict et descente à égalité. Si la préparation est scellée par 14H, son commit ignore transcript, digest et audit. Une fenêtre Morton vide, pauvre ou adversariale doit donc changer au plus le travail exact, jamais le résultat.

Ce jalon est implémenté et son smoke ciblé passe sur l'hôte puis sur une vraie G4. Le premier SHA `136a4c3c72fb97087d9555bca270b25cca5b8d83` a mesuré 672 octets de pile locale par thread; le SHA optimisé `3aeb62019252c785d94cfb91de331bb74b6572e2` conserve la requête, les sources et le record dans les buffers globaux et ramène cette pile à 160 octets, soit une réduction de 76 %, avec toujours 62 registres et zéro spill. NVCC 12.9.86 produit uniquement un cubin `sm_120`; les deux CTests, le digest, les candidats $k=2$ et $k=10$, les partitions CPU exactes et le memcheck restent identiques. La projection rationnelle et le trafic en $C$ restent à traiter avant tout SLO. Le smoke ne revendique aucune garantie de rappel, accélération mesurée, p95 `warm_e2e`, latence sous la seconde à 50 k points, capacité 10 M+ ou statut public; la phase, ses gates, `architecture_only` et `public_status=not_claimed` restent inchangés.

### Trafic actif 14J

14J garde les buffers device persistants de $208C+144C=352C$ octets pour éviter les allocations par lot, tout en supprimant le trafic proportionnel à la capacité inutilisée. Pour $D\leq C$ requêtes supportées, l'appel copie exactement $208D$ octets d'entrée, initialise exactement $144D$ octets de sortie et rapatrie exactement $144D$ octets dans un vecteur de $D$ records. L'audit conserve séparément la capacité physique et ces trois volumes actifs.

La queue physique $[D,C)$ n'est pas une autorité : elle n'est ni initialisée, ni copiée, ni lue, et toute partie qui devient active lors d'un appel ultérieur est réinitialisée avant le kernel. La queue des candidats inutilisés de chaque record actif reste sentinellée; sa corruption invalide le lot. Ce contrat ferme la dette de trafic en $C$ sans changer le digest des records actifs, les propositions flottantes, la recertification CPU 14F ou le commit scellé 14H.

La mémoire de capacité reste $O(C)$ et le snapshot reste $O(n)$, mais le trafic propre à un appel devient $O(D)$. 14J ne construit ni facette ou coface absente, ni incidence globale, Gamma, cellule ou mosaïque de Delaunay d'ordre supérieur. Il reste `cuda_g4 / hgp_reduced / proposal_only`, déploiement `architecture_only`, avec décision scientifique `reference_cpu / hgp_reduced / certified` et `public_status=not_claimed`. Le raccord complet à l'exécuteur, la projection rationnelle, le protocole `warm_e2e`, le SLO 50 k et la capacité 10 M+ restent ouverts.

Le rejeu court du SHA `8c76feb4c28dd1360d71075b1d3e15c7af0a3c95` sur une G4 `SPOT` réelle passe. Les deux CTests ciblés terminent en 0,28 seconde, la transition $C=6$ et $D=4$, $D=1$, $D=5$ ferme les extents réinitialisés, ptxas conserve 62 registres, 160 octets de pile et zéro spill, `cuobjdump` trouve un seul cubin `sm_120` sans PTX, et memcheck annonce zéro erreur et zéro fuite. Cette recertification ne constitue ni un benchmark de débit, ni un protocole `warm_e2e`.

### Projecteur entier direct 14K

14K remplace le projecteur par recherche binaire rationnelle par une quantification entière exacte, sans changer le mot binary64 choisi. Pour $x=N/Q$, le contrôle de plage compare $A=\lvert N\rvert$ à $Q(2^{53}-1)2^{971}$. Le régime subnormal divise $A2^{1074}$ par $Q$. Le régime normal détermine $e=\lfloor\log_2(A/Q)\rfloor$ à partir des bits de poids fort et d'au plus une comparaison décalée, puis divise sur la grille $2^{e-52}$.

Le quotient et le reste ferment directement l'arrondi historique vers la borne numérique inférieure en cas d'égalité : incrément strict au-delà du demi-pas pour une coordonnée positive, incrément dès le demi-pas pour une coordonnée négative. Cette asymétrie conserve notamment les midpoints, le demi-minimum subnormal et les changements de binade bit à bit. La normalisation du significand ne publie ni infini, ni zéro négatif.

Comme les trois numérateurs partagent $Q$, son bit de poids fort et le seuil maximal sont préparés une seule fois. Chaque axe non nul supporté exécute une `divide_qr`; une requête en exécute donc au plus trois, contre environ 189 comparaisons rationnelles et trois constructions de `ExactRational` auparavant. L'audit compte exactement trois axes par requête et les partitionne entre zéro, hors plage et une division; une sous-déclaration isolée est rejetée. Aucun cache ou état persistant n'est ajouté.

Le différentiel court conserve l'ancien encadrement uniquement comme oracle de test. Il couvre zéro, $1/3$, minimum subnormal, demi-minimum subnormal, minimum normal, midpoints adjacents positifs et négatifs, changements de binade, maximum fini, hors plage et 64 rationnels déterministes. Le chemin produit ne dépend plus de cet encadrement. La sélection GPU reste une proposition sans rappel garanti; 14F et 14H conservent seuls l'autorité déjà décrite.

Le rejeu court du SHA `5e7e8449d7f4de2875ad0d9db8674d7664a30e4d` sur une G4 `SPOT` réelle passe 2/2 en 0,29 seconde. La partition de six axes vaut une division, cinq zéros et zéro hors plage; le digest `18249493464636075901`, l'unique cubin `sm_120` sans PTX, les 62 registres, les 160 octets de pile, l'absence de spill et le memcheck nul sont conservés. Ce rejeu n'est ni une mesure de débit, ni une qualification 50 k ou 10 M+.

### Couture intégrée générique 14L

`ExactDirectSparseFacetDescentAnchoredBatchExecutor::run_next` ferme la couture synchrone entre 14D, un producteur borné externe, 14F, 14G et 14H. L'exécuteur termine d'abord tous les préflights CPU du lot, sélectionne les $A$ bras et canonise leurs $D$ clés distinctes. Il construit ensuite exactement une miniball locale par clé pour fournir au producteur une requête compacte : clé source, centre exact, rayon carré exact, nombre de supports examinés et fait de certification. Aucune arène de miniball ou structure globale n'est retenue.

Sous une capacité $C>0$ et un plafond de chunks, les $Q=D$ requêtes sont partitionnées une seule fois en tranches d'au plus $C$. Le callback de préparation retourne uniquement des records `proposal_only` et son trafic opérationnel. Pour $G\leq Q$ requêtes effectivement supportées, l'exécuteur exige exactement $208G$ octets H2D, $144G$ octets d'initialisation device et $144G$ octets D2H; il rejette toute discordance avant le scellement. Un callback unique agrège ensuite au plus $R\leq D$ records canoniques et scelle le transcript 14F sous son budget complet.

La préparation exacte 14G consomme immédiatement ce transcript. Si elle est complète, `run_next` forge en privé un unique ticket 14H, l'engage sans rejeu géométrique et ne rend jamais la capacité à l'appelant. Le succès avance exactement une fois et retourne zéro ticket vivant; tout rejet antérieur à l'émission conserve les cinq composantes du curseur. L'audit sépare les compteurs $A,D,Q,C,G,R$, le nombre de chunks, les centres, miniballs et supports exacts, les trois trafics actifs et les durées de chaque étape. Le CTest court ferme $A=12$, $D=Q=G=4$, $C=2$, deux chunks et $R=1$, soit 832 octets H2D, 576 octets d'initialisation device et 576 octets D2H.

Cette couture est `external_bounded_proposal_plus_reference_cpu / hgp_reduced / proposal_only_then_certified`, `architecture_only`, avec `public_status=not_claimed`. Elle n'instancie encore aucun adaptateur vers le vrai contexte CUDA 14J/14K. Les centres exacts préparés pour le producteur ne sont pas transmis à 10.5c, qui reconstruit sa miniball au point d'usage. Aucun protocole `warm_e2e`, SLO 50 k, run 10 M+, résultat public, facette ou coface globale, incidence, Gamma, cellule ou mosaïque de Delaunay d'ordre supérieur n'est ajouté.

### Constructeur Morton/LBVH device 14M

14M implémente la construction spatiale linéaire qui manquait au chemin `warm_e2e`. Le GPU propose les $3n$ bins Morton avec arrondis dirigés; seul un axe ambigu revient au quotient dyadique exact CPU. Un tri CUB stable des codes, initialisé par les `PointId` croissants, produit exactement l'ordre `(Morton, PointId)`. La topologie emploie le même `find_split` que le CPU, y compris la division au milieu de toute plage de collisions, et place directement la racine d'une plage de $m$ feuilles à $B+2m-2$ dans le postordre. Une réduction par niveaux inverses calcule les témoins AABB avec départage au plus petit `PointId`.

Le snapshot de feuilles et nœuds ne devient jamais une autorité par transfert. L'import CPU recertifie en un passage la permutation, les $3n$ inégalités dyadiques, chaque split, le postordre, les AABB, la racine et les compteurs. Pour une capacité $C\leq\mathrm{INT\_MAX}$, la capacité device exacte vaut $308C-56+T_{\mathrm{CUB}}(C)$ octets et le snapshot actif $176n-80$ octets. La profondeur est au plus $63+\lceil\log_2 C\rceil$ et le coût conservatif de topologie est $O(n\log n)$.

La voie commune de canonicalisation réduit en parallèle son record de tri de 56 à 32 octets et évite les constructions rationnelles pour les extrema et pour la majorité des bins dyadiques; le repli `BigInt` demeure exact. Ces changements ne matérialisent que des tableaux linéaires. Ils ne créent aucune facette, coface, incidence, cellule, Gamma ou mosaïque de Delaunay d'ordre supérieur.

Le faux launcher hôte, l'import certifié et la source CUDA sont validés. Au SHA `20b6d60e62941a096cb81dc1005e7f5ed5017533`, NVCC 12.9.86 produit un unique cubin AOT `sm_120` sans PTX et le memcheck court ferme zéro erreur et zéro fuite. Le `component_smoke` certifie 50 000 points avec une médiane chaude de construction de 17 084 679 ns sur trois répétitions, puis 10 000 001 points en 6 324 126 601 ns avec 3 082 232 059 octets device et un pic RSS hôte de 5 850 509 312 octets. Cet artefact qualifie seulement le constructeur spatial, jamais le p95 `warm_e2e`, les dix ordres, la hiérarchie matérialisée, le streaming produit ou la porte de Phase 14.

### Lease device compacte 14N

La porte d'entrée 14M étant satisfaite, 14N ouvre sous `cuda_g4_plus_reference_cpu / hgp_reduced / device_morton_lbvh_lease / architecture_only`. `MortonLbvhBuildContext::release_device_lease` accepte uniquement le dernier résultat 14M complet et certifié provenant de cette instance, avec la même epoch et les mêmes extents. Un résultat étranger ou périmé et une seconde extraction sont refusés avant transfert. La lease est mobile, non copiable, neutralise sa source et possède ses ressources au-delà de la durée de vie du contexte.

Pour une capacité $C$, la lease retient exactement les $3C$ mots binary64 canoniques de coordonnées et les $C$ `PointId` du buffer actif trié Morton, soit $24C+8C=32C$ octets device. Elle libère bins, deux buffers de codes Morton, buffer d'identifiants inactif, feuilles, nœuds, frontières, indices de niveaux, contrôles et workspace CUB. À partir de la capacité 14M $308C-56+T_{\mathrm{CUB}}(C)$, le relâchement audité vaut donc $276C-56+T_{\mathrm{CUB}}(C)$ octets. La lease ne conserve aucun snapshot hôte; l'index CPU déjà recertifié reste l'unique LBVH hôte et demeure valide indépendamment d'elle.

Ce transfert de durée de vie ne donne aucune autorité scientifique au device. Le jalon 14N isolé ne fournit pas encore de consommateur; 14O décrit ci-dessous son adoption directe par 14I. Aucune facette, coface, incidence, cellule, Gamma ou mosaïque de Delaunay d'ordre supérieur n'est construite. Le faux launcher et le CTest hôte court valident l'unicité du transfert, les rejets et les comptes $24C$, $8C$ et $32C$.

Le `component_smoke` 14N est qualifié sur G4 `SPOT` au SHA `2a03f4ad55b2e369891de2081f67a5108a4de8ad`. NVCC 12.9.86 produit un unique cubin AOT `sm_120` sans PTX; le cas 4 096 passe avec memcheck à zéro erreur et zéro fuite. À 50 000 points et trois répétitions, la construction a une médiane de 17 569 191 ns et un maximum de 135 829 920 ns; l'extraction prend 238 770 ns, retient 1 600 000 octets device, en libère 13 819 911 et conserve zéro octet de snapshot hôte. Ces mesures qualifient uniquement le cycle constructeur--lease du composant. Elles ne sont ni le protocole p95 `warm_e2e`, ni le calcul des dix ordres, ni le pipeline complet 10 M+; aucun SLO, résultat exact public ou porte de Phase 14 n'est revendiqué.

### Adoption directe de la lease par 14I — 14O

La porte 14N étant satisfaite, 14O implémente dans le source une couture `cuda_g4_plus_reference_cpu / hgp_reduced / device_morton_lbvh_lease_adoption / architecture_only`, avec `public_status=not_claimed`. Un nouveau constructeur de `DirectSparseFacetTopKProposalContext` exige l'index 14M certifié, le même nuage canonique, une lease 14N prête de capacité exactement égale à $n$ et une capacité de requêtes non nulle. Il vérifie l'identité immuable du namespace `PointId`, l'epoch source, les extents $3n$ et $n$, la permutation Morton de l'index et le caractère non ambigu hôte-faux ou CUDA-résident.

Toutes les validations faillibles et l'unique allocation linéaire hôte précèdent le transfert. La voie adoptée garde seulement l'inverse Morton de $n$ entrées `size_t`, soit $8n$ octets sur l'ABI G4 LP64, puis consomme le propriétaire mobile de la lease et le retient pendant toute la vie du contexte. Les pointeurs device vers les $3n$ mots de coordonnées et les $n$ identifiants triés restent opaques et dominés par ce propriétaire. Aucun second snapshot device de $32n$ octets n'est alloué et le trafic H2D du snapshot vaut zéro à chaque lot.

Le schéma d'audit propositionnel passe à v4. Il distingue les comptes hôte de coordonnées, ordre Morton et inverse, l'epoch de la lease, le trafic H2D du snapshot et les faits d'adoption et de propriété. La voie historique reste disponible : elle conserve ses $3n$ mots de coordonnées, ses $n$ identifiants Morton et son inverse, soit $40n$ octets hôte sur LP64, puis copie $32n$ octets au premier lot GPU supporté et zéro aux suivants. Les deux voies gardent les mêmes requêtes, propositions, digest opérationnel et recertification CPU exacte 14F; une proposition GPU ne devient jamais une décision scientifique.

14O atteint `validated_real_G4_component_adoption_architecture_only`. La validation hôte reste acquise : sous GCC Release strict, les deux cibles proposition et Morton compilent et leurs deux CTests passent 2/2 en 0,05 seconde; après fermeture du défaut d'audit P2, le CTest proposition repasse seul 1/1 en 0,01 seconde. Les fixtures couvrent adoption, parité des transcripts et digests, rejets atomiques, durée de vie hors builder et index, deux lots adoptés sans snapshot H2D et le contrat legacy premier lot à $32n$ puis zéro. Les falsifications permanentes refusent séparément l'absence de ce premier transfert et sa répétition au second lot.

La qualification G4 réelle est enregistrée au SHA main `f44d77e7401b16fc0818fa151348d6588b7e9618`. Le build propre de la cible exacte réussit avec GCC 13.3 et NVCC 12.9.86; `cuobjdump` trouve exactement les deux cubins ELF `phase14_facet_top_k_proposal.sm_120.cubin` et `phase14_morton_lbvh_build.sm_120.cubin`, sans PTX. Le schéma outil `morsehgp3d.phase14k.facet_top_k_cuda_qualification.v4` confirme à $n=5$ le trafic legacy $160/0$ octets, l'adoption $8n=40$ octets hôte avec propriétaire retenu et zéro snapshot H2D aux epochs un et deux, la lease $32n=160$ octets, et la parité transcript/digest. Le digest vaut `18249493464636075901`. Le cas $K=10$ adopté propose dix candidats après 230 inspections, avec partition exacte validée et zéro snapshot H2D. Le rejeu memcheck rapporte zéro erreur et zéro octet fui.

L'artefact [phase14o_g4_adoption_f44d77e.json](validation/phase14o_g4_adoption_f44d77e.json) conserve la cible `SPOT`, les deux coupe-circuits, l'arrêt final `TERMINATED`, zéro autre VM active et la révocation de la clé avec suppression locale. Aucune mesure de temps ou RSS 14O n'a été prise. 14O ne raccorde pas le contexte aux callbacks 14L, ne matérialise pas la hiérarchie complète et ne qualifie ni le p95 `warm_e2e` 50 k, ni le SLO, ni 10 M+, ni un statut public exact. La porte de Phase 14 reste ouverte.

### Raccord intégré lease--callbacks--ticket--reducer — 14P

14P est validé hôte sous `backend=cuda_g4_plus_reference_cpu`, `profile=hgp_reduced`, `mode=proposal_only_then_certified`, `deployment_status=architecture_only` et `public_status=not_claimed`. `DirectSparseFacetTopKIntegratedAdapter` emprunte le contexte 14O, le nuage et une politique bornée. Il refuse dès sa construction un contexte legacy qui n'a pas adopté la lease 14N et interdit un nuage temporaire. Pour chaque chunk 14L, il transforme uniquement les centres exacts déjà préparés en requêtes propositionnelles actives, appelle le producteur 14I adopté, puis remet ses records au scelleur CPU 14F inchangé. Un lot vide suivant un lot non vide réinitialise transactionnellement l'audit; un seal invalide ne le corrompt pas et peut être retenté. L'adapter ne possède aucun état scientifique et n'avance aucun curseur.

`prepare_next_integrated` expose ensuite exactement un ticket 14H mobile, lié au sceau, à l'epoch, aux cinq composantes du curseur et au stamp locator strictement pré-lot. La préparation laisse ces cinq composantes inchangées. Le wrapper historique `run_next` réemploie la même primitive et committe immédiatement le ticket; il ne maintient donc pas un second chemin scientifique. La voie industrielle remet directement le ticket à 15D, qui effectue le fold du reducer avant l'unique avance sans rejeu.

La fixture composée emploie les lanceurs GPU hôte simulés, mais traverse les vrais objets lease 14N, contexte adopté 14O, adapter 14P, préparation 14L, ticket 14H et reducer 15D. Sur le tétraèdre, les deux chunks propositionnels couvrent quatre clés, gardent zéro transfert H2D du snapshot, conservent le propriétaire device et donnent une forêt récursivement identique aux voies résidente, projetée et vivante. Le témoin `3i+1` reste l'autorité du reducer; toute autre valeur est rejetée avant mutation.

Cette validation ferme le raccord logiciel manquant, pas sa qualification CUDA réelle. Les callbacks empruntent l'adapter et doivent rester synchrones. Les centres exacts sont encore reconstruits par 10.5c après avoir été bâtis pour la proposition, aucune mesure `warm_e2e` ou RSS n'est prise et aucun pipeline complet à 10 M+ n'est exécuté. Aucune facette, coface, incidence, cellule, structure Gamma ou mosaïque de Delaunay d'ordre supérieur n'est matérialisée. La prochaine optimisation interactive vise d'abord le goulot Phase 9 mesuré, puis la capsule exacte de miniball si son coût résiduel le justifie; la prochaine porte massive après 15H reste l'externalisation segmentée.

### Pruneur borné de rang pair — 14Q

La porte 14P étant satisfaite, 14Q implémente sous `cuda_g4_plus_reference_cpu / hgp_reduced / proposal_only_then_certified / architecture_only`, avec `public_status=not_claimed`, le premier accélérateur direct du `rank_search` Phase 9. Le flux CPU groupe au plus $P$ produits canoniques de sa frontière et interroge un callback synchrone. Un produit absent, inachevé sous le budget d'epochs ou arrêté par une capacité revient bit à bit au parcours CPU historique; une proposition fausse ou mal formée ferme l'appel avant mutation.

Le premier diagnostic réel G4 de P2 a rempli son rôle de falsification rapide. Sur `uniform_latin` à 12 500 points, le CPU historique prend 1 395,5 ms, le premier assisté 1 804,5 ms et le résident 1 654,3 ms. Les 43,5 millions d'items et 2 352 epochs ne produisent que 45 propositions pour 1 741 produits demandés, dont 26 consommées. P2 est donc plus lent sur ce cas et son parcours répété à faible rendement est le goulot prioritaire; aucune répétition longue ni échelle supérieure n'est justifiée pour cette version.

P3 introduit le culling par paire-ancre : CUDA propose avec une borne inférieure dirigée et le CPU recertifie le signe dyadique exact; seul un minimum certifié positif ou nul classe la boîte `anchor_noninterior` sans descente. P4 conserve alors une coupe terminale unifiée. Pour une capacité $C$, les terminaux device de 16 octets remplacent les reçus de 48 octets, les supports restent implicites et le workspace fixe hors snapshot et P1, scan inclus, devient $40P+80W+16C+8+T_{\mathrm{scan}}$ octets. Le snapshot LBVH reste exactement $80(2n-1)$ octets; capacités, pics, epochs, kernels, scans, octets H2D/D2H, classes terminales, digest et causes de fallback restent audités.

Le GPU ne décide ni prune ni keep. Le CPU authentifie et reclassifie chaque terminal. Il accepte un prune seulement après le préfixe canonique strict minimal atteignant $s_{\max}-1$. Il accepte le keep conservatif seulement lorsque les deux supports implicites et l'antichaîne terminale pavent exactement la racine et que le seuil strict n'est pas atteint; le produit est alors développé, sans conclure sur chaque paire individuellement. Les comptes prune, keep et fallback doivent fermer exactement le lot.

L'accélération reste une transition éphémère séparée du checkpoint, du bundle et du vérificateur durable v1. Son vérificateur frais rejoue exactement une fois chaque prune et keep consommé, refuse doublons et suppléments non consommés et reconstruit le chunk candidat. Aucun catalogue global de paires, facettes ou cofaces, aucune incidence, cellule, structure Gamma ou mosaïque de Delaunay d'ordre supérieur n'est matérialisé; le coût additionnel reste $O(n+P+W+C)$.

Le commit `d1e6d54` spécialise les prédicats de signe en dyadiques exacts non normalisés. Un unique smoke local budgeté a donné, pour `uniform_latin`, 626,180 ms à 12 500 points, 875,731 ms à 25 000 et 752,328 ms à 50 000; pour `eight_clusters`, 299,284 ms, 301,732 ms et 316,609 ms. Tous les chunks ont le statut `budget_exhausted`. Ces mono-mesures ne sont ni le pipeline complet, ni `warm_e2e`, ni un p95, ni un SLO.

P5 au SHA `a012af982e1e75ec5f9ba9c5a17d16178b795f90` ferme la copie de la capacité terminale entière : les $16C$ octets restent réservés sur device, tandis que l'hôte alloue et rapatrie uniquement les $16T$ octets actifs. Le test CUDA réel ciblé passe sur RTX PRO 6000 Blackwell et CUDA 12.9. L'unique diagnostic `uniform_latin` à 12 500 points, $K=10$, `work=20000`, $P=1$, $W=32768$, $C=16384$ et $E=64$ réussit le composant et son vérificateur frais sans cap ni fallback GPU. Il compte 223 callbacks, 3 404 epochs, 46 145 items visités et 22 381 terminaux recertifiés exactement; 23 prune et 200 keep sont proposés, 22 et 199 consommés, et deux produits se replient sur le CPU historique.

Le D2H terminal physique et actif vaut 358 096 octets, contre 58 458 112 octets physiques avant P5, soit une réduction de 99,39 %. Cette correction n'est toutefois pas la correction de latence principale : le CPU historique mesure 325,031 ms, le premier assisté 808,867 ms et le résident 646,858 ms. Le résident antérieur à 665,206 ms provient d'une exécution distincte; son écart d'environ 2,76 % est seulement diagnostique. Les synchronisations et lancements répétés des 3 404 epochs dominent désormais.

P5a implémente donc localement, pour $P=1$, un parcours LBVH stackless left-first sans boucle hôte par epoch. Une corde de 4 octets par nœud encode le prochain nœud après rejet ou terminal; un kernel borné produit la coupe. Sur snapshots résidents, une première synchronisation lit le contrôle de 40 octets, puis une seconde a lieu seulement lorsqu'un préfixe terminal non vide doit être copié à raison de 16 octets par terminal actif. Le workspace hors snapshots vaut exactement $32P+16C+40$ octets; il ne contient ni frontières ni scan. Si la borne de visites $Q=\min(N,WE)$ ou de terminaux est atteinte, le préfixe est authentifié pour l'audit mais la proposition entière est abandonnée et le produit suit le parcours CPU historique. Tous les terminaux restent propositionnels : l'authentification, les signes dyadiques, la couverture et les décisions prune ou keep restent recertifiés exactement par le CPU.

P5a est compilé et exercé sur la cible G4 réelle au SHA `cd4ab8b0a5cddf24fdb060073232186b5ba716b4`. Le test CUDA ciblé passe avec CUDA/NVCC 12.9.86; l'AOT contient exactement un cubin `sm_120` et aucun PTX, puis `compute-sanitizer` termine le cas `uniform_latin` à 4 096 points avec zéro erreur. Sur l'unique gate `uniform_latin` à 12 500 points, le CPU historique prend 320,620 ms, le premier assisté 1 094,620 ms et le résident 934,077 ms. Le composant et son vérificateur frais sont complets sans cap ni fallback GPU. Les 222 callbacks lancent 222 kernels stackless, zéro kernel de comptage et zéro scan, visitent 41 243 nœuds, effectuent 444 synchronisations résidentes et recertifient exactement 20 176 terminaux. Les 22 prune et 200 keep proposés donnent 22 et 199 consommés; un produit suit le fallback CPU historique. Le D2H physique et actif vaut 322 816 octets, la corde ajoute 99 996 octets au premier snapshot puis zéro résident, et le snapshot LBVH vaut 1 999 920 octets puis zéro résident.

P5b optimise alors le côté exact sans changer son autorité. Les coordonnées binary64 sont alignées dans une enveloppe prouvée de 124 bits puis les signes maximum et ancre sont calculés sur des entiers exacts de 256 bits; tout cas hors enveloppe revient obligatoirement au chemin BigInt avant décision. Au SHA exact `159edc0c39e4e56e02ba4bc3b1f5cdd5889320bf`, le même gate court mesure 298,696 ms sur le CPU historique, 1 035,830 ms au premier passage assisté, 846,446 ms au résident et 277,460 ms pour le vérificateur frais. Les gains indicatifs par rapport à l'exécution P5a distincte valent respectivement 6,84 % sur le CPU historique et 9,38 % sur le résident, mais le résident reste 2,83 fois plus lent que le CPU.

Les comptes P5b restent identiques à P5a : 222 callbacks et kernels, 41 243 visites, 444 synchronisations résidentes, 20 176 terminaux recertifiés exactement, 22 prune et 200 keep proposés, 22 et 199 consommés, un fallback CPU historique, aucun cap ni fallback GPU. Un profil RelWithDebInfo `gprof`, non assimilable à un SLO, attribue environ 36,36 % au PGCD et 29,09 % à la division entière non signée, contre 5,45 % au signe maximum borné et 3,64 % au signe ancre borné. La cible suivante est donc mathématique : utiliser directement l'identité exacte $\phi$ pour la classification boule--AABB, supprimer les normalisations rationnelles intermédiaires des décisions de signe et réserver la construction de `ExactLevel` à la sortie.

P5c réalise ce pivot sans changer l'autorité scientifique. Pour la boule de diamètre $[u,v]$, l'identité exacte $\left\Vert x-\frac{u+v}{2}\right\Vert^2-\frac{\left\Vert u-v\right\Vert^2}{4}=(x-u)\cdot(x-v)$ remplace la construction rationnelle du centre et du niveau dans la classification fermée. Une boîte est extérieure en bloc seulement si son minimum exact de $\phi$ est strictement positif, intérieure en bloc seulement si son maximum exact est strictement négatif et toute égalité descend jusqu'aux feuilles. `ExactLevel` n'est plus construit qu'après survie du rejet de rang, pour la sortie. Le champ historique `exact_point_distance_evaluation_count` compte désormais sur ce chemin les classifications exactes de feuilles non résolues par les deux tests stricts, fournies par les extrema de $\phi$ et nécessairement à égalité sur une boîte dégénérée; son nom reste inchangé pour préserver le schéma.

Au SHA exact `a5780312906230e92065e008ee7ac3d362a09bdc`, le même gate G4 mesure 28,093 ms sur le CPU historique, 796,881 ms au premier passage assisté, 607,506 ms au résident et 31,925 ms pour le vérificateur frais. Par rapport à l'exécution P5b distincte, les baisses diagnostiques valent 90,59 % sur le CPU historique et 28,23 % sur le résident, sans prétention statistique. Les comptes restent fermés et identiques : 222 callbacks et kernels, 41 243 visites, 444 synchronisations, 20 176 recertifications exactes, 22 prune et 200 keep proposés, 22 et 199 consommés, un fallback CPU historique, aucun cap ni fallback GPU. La fermeture exacte est acquise mais le résident reste 21,62 fois plus lent que le CPU; le gate de vitesse assisté échoue donc encore.

P5d parallélise ensuite sur device les produits d'un même callback stackless. Les capacités globales de visites et de terminaux sont partagées en segments déterministes entre les produits actifs; le workspace hors snapshots devient $72P+16C$ octets et reste borné. Le premier build au SHA `f31834aeb43282a1479b994a2f10356e855437b3` échoue avant mesure parce que `std::numeric_limits` est appelé depuis le code device. Le correctif strictement technique au SHA exact `3500feacbc37706054d54b99254175f435362cdf` remplace cette garde par une expression compatible device; CUDA/NVCC 12.9.86 compile alors la source ciblée.

Sur le gate unique `uniform_latin` à 12 500 points, $K=10$, `work=20000`, $P=64$, $W=32768$, $C=262144$ et $E=64$, P5d réussit exactement le composant et le vérificateur frais, sans cap ni fallback GPU. Le CPU historique mesure 28,289 ms, le premier assisté 4 070,298 ms, le résident 3 889,082 ms et le vérificateur frais 32,569 ms. Les lots ramènent kernels et synchronisations de 222 et 444 à 128 et 256, mais 127 remplacements de cache sur 128 callbacks demandent 1 518 produits, au plus 16 par lot, visitent 707 423 nœuds et font rapatrier puis recertifier exactement 351 246 terminaux, soit 5 619 936 octets actifs. Les 37 prune et 1 481 keep proposés ne donnent que 22 prune et 199 keep consommés; un produit suit le fallback CPU historique. La couture séquentielle invalide ainsi presque tout le travail spéculatif avant consommation. Le résident vaut 137,477 fois le CPU exact : le gate assisté échoue plus nettement malgré la baisse des lancements.

P6a spécialise ensuite les décisions booléennes Gram--Cramer du flux supérieur. Après alignement exact des extrémités binary64 sur un exposant dyadique commun, la voie bornée n'est utilisée que si chaque coordonnée alignée tient dans 124 bits de magnitude. Directions, entrées de Gram, déterminants, numérateurs barycentriques et puissances homogènes tiennent alors respectivement dans au plus 125, 252, 759, 762 et 1013 bits; `int1024` conserve donc exactement signes stricts, zéros et comparaisons non strictes. Toute entrée hors enveloppe revient au DAG rationnel arbitraire avant décision. Les certificats émis restent construits par l'analyse rationnelle riche : cette spécialisation ne transforme aucune proposition flottante en décision scientifique.

P6b supprime le second parcours géométrique supérieur de la façade terminale. Une session interne part des seules racines canoniques, n'accepte aucun chunk ni checkpoint externe et exécute des chunks de budget fixe. Chaque chunk committé est capturé une fois sans rejeu frais; événements et diagnostics restent segmentés, tandis que les payloads des certificats de prune sont détruits après engagement de leurs comptes et de leur ordre dans la chaîne authentifiée. La capacité de segments croît géométriquement avant le calcul exact, sans réservation proportionnelle au cap de chunks. Une autorité move-only terminale, liée par tokens stables au nuage et au LBVH d'origine, est consommée par la façade v2; la voie paire reste fraîchement rejouée. Ce sceau est strictement local au processus : il ne revendique ni reprise durable, ni réduction hiérarchique, ni statut public.

Le runner produit v2 raccorde désormais, dans un même processus CPU, flux paire, session supérieure scellée, façade terminale, journaux 14C, tickets 14H, reducer 15D et forêt matérialisée. Deux mono-mesures locales seulement ont été prises : `uniform_latin`, $n=5$, $K=4$ termine en 81,086 ms, dont 56,098 ms pour le flux supérieur et 21,596 ms pour le reducer; $n=8$, $K=7$ termine en 1 163,335 ms, dont 401,609 ms et 754,801 ms. Elles valident la couture et montrent que la croissance supérieure puis aval reste dominante; elles ne valent ni gate interactif, ni p95, ni mesure à $K=10$, ni qualification 50 k.

P6 ne matérialise pas les $\binom{n}{3}+\binom{n}{4}$ supports : le flux garde une frontière multiplicitaire, un checkpoint borné, des segments de sorties utiles et des certificats transitoires. Il ne construit aucune facette, coface, incidence, cellule, structure Gamma ou mosaïque de Delaunay d'ordre supérieur. Son pire cas peut néanmoins visiter ou classifier l'univers cubique et quartique implicite; le résultat à huit points interdit donc toute extrapolation vers 50 000 ou 10 M. Le prochain pivot doit réduire mathématiquement ce travail produit, et non multiplier les réglages ou les campagnes longues.

P7a ajoute au flux pair une seconde preuve de rang qui n'exige plus un même ensemble de témoins sur tout le produit. Pour des boîtes supports $A,B$, le centre doublé de chaque paire appartient à $A+B$. Pour chaque cellule $C$, le plancher exact $L(C)=\max(d^2(A,B),d^2(C,2A),d^2(C,2B))$ minore le carré de toute corde compatible. Une antichaîne disjointe des supports est acceptée seulement sous la borne division-free stricte $\max\left\Vert2x-s\right\Vert^2<L(C)$. Chaque cellule peut employer des témoins différents, mais doit en certifier au moins $s_{\max}-1$ strictement intérieurs à toutes ses boules. L'inégalité stricte est indispensable : un grand shell fermé ne permet pas d'éliminer un diagnostic extra-shell dont le nombre d'intérieurs reste dans la fenêtre. Toute égalité est donc inconclusive. La range query exacte rejette un nœud $Q$ lorsque $\min_{y\in2Q}\max_{s\in C}\left\Vert y-s\right\Vert^2\geq L(C)$ et une cellule est subdivisée sans parcours LBVH lorsque son cœur continu vérifie $\sum_i((C_i^+-C_i^-)/2)^2\geq L(C)$. L'implémentation visite au plus sept cellules et 256 nœuds LBVH, emploie au plus 64 entrées auxiliaires et revient sans décision à l'expansion historique si une capacité ou la preuve manque. Un crédit déterministe persistant borne en outre son travail cumulé à deux enveloppes atomiques plus un seizième du travail pair historique; les tentatives stériles ne peuvent donc plus monopoliser le flux. Elle ne construit ni les centres individuels, ni les paires d'un produit, ni une cellule de Delaunay.

P7b rend ensuite la classification fermée terminale physiquement budgetable. La pile DFS LBVH, son nombre local de visites, les identifiants intérieurs utiles, les comptes shell et extérieur, le témoin extra-shell canonique et le masque des deux supports deviennent le seul curseur actif du produit feuille. Chaque dépilage d'un nœud paie exactement une unité de travail avant mutation; le nombre de points classés en bloc reste un compte logique et non un faux coût $n$ par requête. Un checkpoint interrompu conserve donc $O(H+K)$ entrées pour cette requête, où $H$ est la profondeur LBVH, au lieu d'exiger une opération atomique de taille $n$. Le schéma v2 engage ce curseur et le travail physique; sa validation rejoue exactement les visites actives depuis la racine et reconstruit pile, catégories et témoins en $O(V_{\mathrm{actif}}+K)$ sans rescanner le préfixe logique, puis le rejeu frais ancré reste l'autorité scientifique.

Ces deux changements allègent le chemin produit sans créer de catalogue global de paires, facettes, cofaces, incidences, cellules ou mosaïque d'ordre supérieur. Ils ne bornent pas encore le nombre de produits pair visités, et ne touchent pas encore le pire cas implicite des supports trois et quatre. Le petit gate $K=10$ doit donc mesurer immédiatement le nombre de produits, les prunes center-cover et les visites fermées avant toute extension; si le supérieur domine encore, la même idée de couverture de centres doit être portée aux groupes multiplicité trois--quatre avant un test plus grand.

Le gate final $n=12\,500$, $K=10$ confirme que la classification feuille est désormais physiquement progressive et que le throttle ramène le center-cover à 1 660 unités sur 20 000, mais il ne ferme que 149 paires et ne produit aucun prune center-cover. Le prochain incrément ne doit donc ni augmenter simplement le budget, ni lancer 50 000 : il doit renforcer la réduction des produits pair, puis porter une preuve de rang groupée aux supports trois--quatre. Une nouvelle taille n'est autorisée qu'après fermeture scientifique du petit nuage.

P8a ajoute un préflight exact au seul certificat universel de rang. Pour deux intervalles $A=[a_0,a_1]$ et $B=[b_0,b_1]$, le minimum de $g(x)=\max_{a\in A,b\in B}(x-a)(x-b)$ vaut $-(b_0-a_1)^2/4$ lorsque $a_1\leq b_0$, symétriquement lorsque $b_1\leq a_0$, et $(a_1-a_0)(b_1-b_0)(b_1-a_0)(a_1-b_0)/((a_1-a_0)+(b_1-b_0))^2$ en recouvrement strict. Le minimax tridimensionnel est la somme de ces trois minima séparables. Un signe positif ou nul prouve donc que le relâché AABB n'a aucun cœur strict commun : le parcours global qui cherche les mêmes témoins pour tout le produit est omis, mais la couverture P7a à témoins variables est encore tentée et aucune paire n'est prunée par ce préflight.

La voie chaude aligne exactement les douze extrémités binary64 sous la garde existante de 124 bits et accumule le signe dans `int1024`; le numérateur commun occupe strictement moins de 1006 bits. Toute plage d'exposants plus large revient avant décision à `BigInt`. Une fixture permanente montre en outre que le cœur du relâché peut être vide alors qu'un point est strictement intérieur à toutes les boules du produit discret corrélé : P8a doit donc rester un saut de travail fail-open, jamais un certificat d'absence scientifique. Le gate court répété à $n=12\,500$, $K=10$ et 20 000 unités visite 319 produits, résout 165 paires, exécute 137 requêtes fermées et 8 249 visites de nœuds, avec 134 tentatives P7a et zéro prune P7a. Cette baisse d'environ 10 % des paires encore présentes par rapport au préfixe P7 n'est pas structurelle; le pipeline reste incomplet et 50 000 demeure fermé.

P8b remplace ensuite l'univers de produits par un composant candidat ancré, sans encore migrer le checkpoint pair. Pour une ancre $p$, un témoin $x$ et $r=x-p$, l'identité $x\in B(p,q)^\circ\Longleftrightarrow r\cdot(q-p)>\left\Vert r\right\Vert^2$ transforme chaque témoin en demi-espace. Sur un nœud AABB $Q$, le minimum du membre gauche est affine et se prend à une extrémité par axe. Si $m=s_{\max}-1$ `PointId` distincts vérifient strictement cette borne, toute feuille de $Q$ a un rang fermé au moins $m+2=s_{\max}+1$ et le sous-arbre est omis. Ainsi $K=10$ exige dix témoins; neuf ne suffisent pas. L'égalité descend toujours.

La banque contient au plus 64 points. Le top-$L$ exact actuel est seulement un moyen borné de proposer cette banque : son échec, une banque de moins de $m$ points, le cap de prédicats ou le cap de reçus désactive des prunes et force la descente. Aucun rappel top-$L$ n'est supposé. Les succès stricts d'un parent sont hérités par ses enfants dans un masque de 64 bits. Un filtre d'intervalles binary64 sous environnement certifié décide les marges séparées de zéro; toute marge ambiguë revient au prédicat dyadique exact P5c. Les paires sont orientées par `PointId`, et les feuilles survivantes sont matérialisées seulement dans le résultat borné de l'ancre courante. Il n'existe donc aucune arène globale de paires, cellule, coface, incidence, Gamma ou mosaïque de Delaunay d'ordre supérieur.

Le diagnostic unique `uniform_latin`, $n=12\,500$, $K=10$, $L=64$, sur 64 ancres échantillonnées ferme toutes les traversées. Il laisse 2 266 candidates, soit 35,406 par ancre en moyenne et 49 au maximum; l'extrapolation diagnostique vaut environ 442 578 paires au lieu de 78 118 750. Les 38 292 nœuds déclenchent 1 869 289 prédicats, dont seulement 4 429 fallbacks exacts. Le temps final observé de 1 174,782 ms pour les 64 ancres sur l'hôte partagé reste beaucoup trop lent et n'est ni un `warm_e2e` ni un p95. Une banque 32 laisse déjà 97,625 candidates par ancre; elle est abandonnée. P8b valide donc la réduction combinatoire, pas encore le SLO : l'étape suivante raccorde le classificateur fermé sparse, puis déplace ou vectorise les propositions avant un seul gate complet à quelques dizaines de milliers.

P8c raccorde chaque candidate à une partition fermée exacte du LBVH. Les deux supports appartiennent toujours à la coque; le classificateur conserve donc au plus $s_{\max}-2$ intérieurs et retourne `above_rank` dès qu'un intérieur supplémentaire est certifié. Pour $K=10$ et $s_{\max}=11$, neuf intérieurs ferment encore un événement de rang onze, tandis que le dixième prouve un rang minimal douze. Une sortie pertinente exige néanmoins la fermeture de toute la coque : deux points de coque donnent un événement régulier; une coque plus grande donne seulement le diagnostic extra-shell avec son plus petit témoin canonique. Les extérieurs ne sont jamais matérialisés et centre/niveau ne sont construits qu'après une partition complète pertinente.

Chaque nœud est classé par les signes exacts du minimum et du maximum de $\phi(q)=(q-a)\cdot(q-b)$. Un filtre binary64 outward emploie la forme centrée $\phi(q)=\left\Vert q-m\right\Vert^2-\left\Vert d\right\Vert^2$, avec $m=(a+b)/2$ et $d=(a-b)/2$ préparés une fois; seules des bornes strictement séparées de zéro décident, sinon le signe dyadique exact est appelé. Le diagnostic intégré sur les mêmes 64 ancres traite les 2 266 candidates sans épuisement : 2 200 événements, 66 rejets de rang et aucun extra-shell. Les 228 123 visites se partagent exactement en 101 753 décisions d'intervalle extérieures, 8 684 intérieures et 117 686 fallbacks exacts.

Ce raccord reste un diagnostic CPU non reprenable : un épuisement recommencerait le DFS de la paire depuis la racine. Il ne doit donc pas devenir un second chemin industriel parallèle à P7b. Le prochain incrément mutualise le curseur fermé reprenable, évite les réservations proportionnelles à $n$ et porte la proposition ancrée en lot résident avant un gate complet. Les 2 005,811 ms observées, dont 1 310,256 ms de banque et 577,469 ms de classification sur l'hôte partagé, ne sont ni comparables au diagnostic précédent, ni un p95, ni une extrapolation de qualification.

P8d retire ensuite le top-$L$ exact de la définition scientifique de la banque. La politique `bounded_morton_window` inspecte un nombre borné de feuilles de part et d'autre du rang Morton de l'ancre, classe ces seules propositions par distance binary64 puis `PointId` et conserve au plus 64 témoins. Ce classement n'a aucune autorité : toute banque de points distincts hors ancre est sûre parce que chaque omission est encore décidée par dix demi-espaces stricts; une banque trop petite désactive simplement les prunes. Le travail de proposition devient $O(W\log L)$ par ancre, avec $L\leq64$, sans scan global ni top-$L$ exact.

Deux diagnostics courts délimitent cette heuristique. Avec 512 inspections, les 64 ancres laissent 17 881 candidates, dont 15 681 sont ensuite rejetées : cette fenêtre est écartée du profil rapide. Avec 4 096 inspections, elles en laissent 7 565, dont 5 365 rejetées; les 2 200 événements utiles restent identiques sur l'échantillon, mais le sur-ensemble est encore 3,34 fois plus grand que celui de la banque exacte. Le temps observé de 1 546,015 ms n'est pas un résultat de qualification. P8d fournit donc une proposition naturellement parallélisable pour le premier kernel, pas une politique finale démontrée.

P8e prépare ce premier kernel sans dupliquer l'index spatial. Une extraction sœur move-only conserve sur device les trois coordonnées canoniques, les `PointId` en ordre Morton et les $2C-1$ nœuds de 80 octets déjà importés et certifiés par le CPU, soit exactement $192C-80$ octets pour une capacité $C$. Elle détruit les codes, buffers de tri, frontières et autres scratch du constructeur; elle ne retient aucun second snapshot hôte. L'extraction compacte 14N et l'extraction de traversée partagent la même capacité à usage unique, donc aucune des deux ne peut être rejouée après l'autre.

Le postordre strict évite une corde linéaire supplémentaire. En parcourant les indices décroissants, un nœud non pruné passe de $i$ à $i-1$. Un sous-arbre de $m$ feuilles occupe exactement $2m-1$ nœuds contigus et se saute par $i-(2m-1)$, ou termine lorsque cette largeur vaut $i+1$. Le curseur propositionnel contient donc un seul indice, le digest de la requête et l'epoch du snapshot. Le contexte fixe à la construction ses capacités de requêtes et de records de 16 octets, sépare le plafond actif de la capacité physique et exige une compaction déterministe par ordre de requête.

Le contrat hôte n'accorde encore aucune autorité scientifique aux records. Un masque nul propose seulement une feuille candidate; un masque non nul contient exactement $s_{\max}-1$ positions de la banque fournie. Identité du nuage, ordre des ancres, unicité des témoins, curseurs, epochs, domaines, cardinalités, ordre strictement décroissant, partition des segments et digest sont vérifiés; une sortie hostile empoisonne le contexte. Les témoins conservent leur ordre amont par distance puis `PointId`, utile aux premières vagues du warp, mais cet ordre ne devient jamais une hypothèse de rappel.

P8e réel ajoute trois kernels de comptage, préfixe/clamp et rejeu-écriture, puis un recertificateur CPU qui rejoue exactement chaque segment complet. Le diagnostic G4 au SHA `626e1b36f9f87bff4d7303908792ffd27c943b5b` ferme 4 096 points, un memcheck, 12 500 et 50 000 points. Pour 64 requêtes à 50 k, proposition et rejeu valent respectivement 535,046 et 204,483 ms; les 59 559 records compacts occupent seulement 1,42 % de la capacité physique copiée. C'est un échec du gate de vitesse échantillonné, non un SLO de produit : aucune passe toutes ancres, aucun support supérieur et aucune hiérarchie complète ne sont exécutés.

Le SHA `ba8dd15eea88c59fa7287c21c2d2500979bf40d0` sélectionne les dix bits canoniques en dix étapes au plus, évite la seconde vague lorsqu'elle ne peut plus modifier ce choix et ramène le transcript actif de 67,1 à 2,1 Mo. Les sorties recertifiées restent identiques, mais la médiane 50 k ne baisse qu'à 454,115 ms pour la proposition et 656,117 ms pour le composant. Le trafic de transcript n'est donc pas le terme dominant; les directions outward encore reconstruites pour chaque témoin et chaque nœud deviennent la cible suivante.

P8f est qualifié comme diagnostic de composant sous `cuda_g4_plus_reference_cpu / hgp_reduced / sampled_cuda_proposal_exact_cpu_recertification_component`. Un quatrième kernel prépare une seule fois, pour chaque couple requête--témoin de la tuile bornée, les trois intervalles outward de direction, les mots binary64 du témoin et le masque d'axes actifs. Comptage et rejeu réemploient ensuite ces 80 octets. L'égalité des mots détermine les mêmes axes inactifs, les opérations dépendant du nœud gardent leur ordre et une direction invalide ou contenant zéro reste fail-open; le rejeu CPU exact demeure la seule autorité.

L'arène supplémentaire vaut exactement $80\times64Q_{\max}=5120Q_{\max}$ octets, avec la garde exécutoire $Q_{\max}\leq4096$, et reste proportionnelle à la capacité fixe de la tuile de requêtes. Elle n'ajoute aucun tableau indexé par les $n$ points, aucun transfert H2D ou D2H et aucune structure globale interdite. À 4 096 points et 16 requêtes, le SHA `95314a18ecd2d8ff2d7377417f7943d20c0c2daf` retrouve le digest recertifié `8808628165750322254`, 6 848 visites, 457 candidates, 2 548 prunes et 3 005 records; proposition, recertification et composant prennent respectivement 7,672030, 17,015130 et 24,687310 ms.

À 50 000 points et 64 requêtes, les trois répétitions retrouvent toutes le digest `8188146790829181083`, 147 376 visites, 28 866 candidates, 30 693 prunes et 59 559 records. Les temps bruts de proposition sont 361,262217, 360,130686 et 360,229307 ms; ceux de recertification 201,816259, 201,742890 et 201,754859 ms; ceux du composant 563,078746, 561,873646 et 561,984346 ms. Leurs médianes valent donc 360,229307, 201,754859 et 561,984346 ms. Par rapport à `ba8dd15`, les accélérations valent 1,260627 pour la proposition et 1,167500 pour le composant. La porte interne de composant strictement inférieure à 100 ms échoue nettement et la recertification sérielle la dépasse à elle seule, bien qu'il ne s'agisse déjà que d'un échantillon de 64 ancres et non du `warm_e2e` produit.

Cette qualification arrête les micro-optimisations du prédicat ancré et ne justifie pas le raccord immédiat des banques device. L'artefact [phase14q_p8f_g4_95314a1.json](validation/phase14q_p8f_g4_95314a1.json) conserve les sorties brutes et les deux cubins AOT `sm_120`. La session G4 `SPOT`, démarrée à `2026-07-25T14:58:31.800-07:00`, a été arrêtée à `2026-07-25T15:08:45.175-07:00` et certifiée `TERMINATED`; aucune autre VM labellisée n'était active et la clé de session a été révoquée. Aucun SLO, passage toutes ancres ou statut public n'est promu.

P8g ouvre le premier certificat groupé exact sous `reference_cpu / hgp_reduced / grouped_exact_certificate`. P8o en conserve le reçu commun mais remplace la relaxation de la boîte des ancres par leur ensemble discret réel. Pour un groupe $P$ d'au plus 32 ancres, un pool propositionnel d'au plus 64 témoins et la boîte certifiée $Q$ d'un nœud LBVH, il calcule exactement $M_x(P,Q)=\max_{p\in P,q\in Q}(x-p)\cdot(x-q)$. Un signe strictement négatif vaut simultanément pour toute paire réelle du groupe et du nœud. Avec $s_{\max}-1$ témoins communs distincts, les deux supports donnent au moins $s_{\max}+1$ points fermés; le sous-arbre est donc hors fenêtre pour toutes les ancres. Le pool n'a aucune obligation de rappel, mais les coins hybrides qui ne sont aucune ancre ne bloquent plus un prune.

Le reçu est non agrégat, non constructible par défaut et son payload scientifique devient privé après le calcul. Son rejeu lie l'identité process-local du nuage et du LBVH, le nœud, sa plage, sa boîte, le rang fermé maximal et la liste canonique des ancres. Un slot témoin coûte entre une et $G$ évaluations exactes ancre--nœud, avec court-circuit au premier signe non négatif; un nœud coûte au plus $GW\leq2048$ signes. Les budgets échouent atomiquement sans publier de témoins partiels. La primitive ne construit ni tableau $64n$, ni matrice $G\times W$, ni catalogue de paires ou de cellules. La preuve, la contre-fixture de corrélation et les limites sont consignées dans [CERTIFICAT_GROUPE_PAIRES_ANCREES_PHASE14.md](math/CERTIFICAT_GROUPE_PAIRES_ANCREES_PHASE14.md).

P8h implémente localement le mode `reference_cpu / hgp_reduced / prepared_grouped_exact_traversal`. Un contexte move-only prépare les boîtes ponctuelles des ancres réelles et des au plus 64 témoins une seule fois, puis parcourt un sous-arbre par une DFS droite d'abord, à pile fixe et avec au plus un nœud actif reprenable. P8o ajoute l'offset de l'ancre active : un arrêt reprend au milieu d'un slot sans refacturer les signes déjà stricts ni publier d'autorité partielle. Chaque succès commun du parent reste valable chez un enfant de boîte $Q'\subseteq Q$; le masque correspondant reste privé et seul le certificat complet peut autoriser un prune. Un appel émet au plus un prune certifié, une feuille non résolue, un fallback de sous-arbre, un arrêt budgétaire typé ou la complétion.

Les fixtures permanentes exigent la même partition terminale que les oracles frais. La première conserve 19 slots et ferme maintenant l'identité physique $27=19+2\times4$ : 19 signes exécutés et quatre succès hérités qui auraient coûté les deux ancres. Elle interrompt aussi un slot après sa première ancre. La fixture non préfixe à une ancre conserve $8=7+1$ et la sortie canonique `q1` puis `q0`. La contre-fixture hybride rend l'ancien maximum AABB inconclusif, certifie le maximum discret en deux signes et échoue sans autorité sous cap un. La reprise segmentée, le fallback structurel, la révocation après déplacement et le rejet d'une autorité étrangère restent validés par les CTests courts; aucun benchmark n'est déclenché ici.

P8i ferme la porte de l'ordonnanceur sous `reference_cpu / hgp_reduced / bounded_morton_group_schedule`. Le contexte move-only partitionne à la volée l'ordre des feuilles LBVH en intervalles Morton contigus d'au plus 32 ancres. Les `PointId` de l'intervalle sont triés pour P8h; le pool propositionnel contient au plus 64 feuilles hors groupe, prises dans un halo Morton croissant gauche puis droite et retriées par `PointId`. Ce halo n'a aucune promesse de distance ou de rappel : une omission ne peut que réduire les prunes exacts. Un seul groupe, son pool et son parcours P8h sont vivants; aucun vecteur de groupes, tableau par ancre ou catalogue de paires n'est construit.

Les frontières `group_complete` matérialisent la fermeture successive des plages d'ancres, qui forment exactement une partition de $[0,n)$. Chaque `PointId` devient donc ancre une fois; en aval, l'orientation canonique $p<q$ de chaque feuille non résolue ou plage de fallback attribue chaque paire non ordonnée à son unique petite extrémité. Un prune reste consommable seulement par le certificat P8g embarqué et son `certifies(...)`; une feuille ou un fallback reste une source de candidates P8c, jamais une décision Morse. Pour contenir le trafic hôte, un arrêt budgétaire ne copie aucun tableau fixe, une feuille ou un fallback copie seulement les ancres, et le pool n'est snapshoté que pour un prune ou une frontière de groupe. La segmentation du budget P8h ne change ni groupes, ni terminaux, ni candidates, ni travail exact hors compteurs d'appels et d'épuisements.

Le différentiel court permanent utilise d'abord 20 points collinéaires, cinq groupes de quatre et un pool de douze : le flux monolithique et la reprise `(1,1)` sont identiques, chaque prune P8h est recertifié par un P8g frais sur les mêmes ancres, pool, nœud et témoins, et le travail exact est conservé. Une fixture tridimensionnelle de 24 points, dont l'ordre Morton diffère de l'ordre `PointId`, utilise des groupes de cinq et ferme un dernier groupe de quatre. Après orientation et classification P8c, ses événements et diagnostics sont identiques à ceux du chemin P8b--P8c ancré exact. Avec seulement deux témoins pour un rang maximal quatre, les cinq groupes retombent sur leur sous-arbre complet, émettent exactement les 276 paires et conservent la même sortie scientifique. Le CTest passe sous GCC Release en 0,04 seconde et Clang 18 Release en 0,05 seconde; l'installation/export et le consumer externe passent 1/1 en 0,01 seconde. Aucun benchmark, CUDA ou GCP n'est lancé.

P8j ferme maintenant la porte d'expansion sous `reference_cpu / hgp_reduced / bounded_morton_oriented_candidate_cursor`. Le contexte move-only possède P8i, une seule plage terminale et deux offsets scalaires. Il parcourt feuilles puis ancres, facture chaque orientation, avance l'offset avant émission et ne publie qu'une paire canonique $p<q$ par appel. Une plage pending se reprend sans nouvelle avance P8i; les budgets d'avances, d'orientations, de visites et de prédicats restent séparés, les deux derniers étant agrégés sur les appels P8i internes. Un arrêt ne copie aucune autorité imbriquée. Les prunes conservent au contraire toute la chaîne P8i--P8h--P8g et une frontière conserve la provenance du groupe fermé.

Le différentiel permanent compare un budget ample à `(1,1,1,1)` : mêmes candidates ordonnées, prunes, groupes, visites et prédicats exacts, puis mêmes événements et diagnostics après classification immédiate P8c. Dans le fallback intégral de 24 points, les 576 orientations se décomposent exactement en 276 candidates et 300 orientations inverses ou auto-couples; le petit ensemble global du test sert seulement d'oracle de doublons. Les budgets nuls, déplacements et autorités étrangères sont atomiques. GCC Release passe en 0,18 seconde, Clang 18 Release en 0,08 seconde, puis installation/export et consumer externe passent 1/1 en 0,01 seconde. Aucun benchmark, sanitizer, CUDA, test massif ou GCP n'est lancé; la portée reste `architecture_only`.

P8k ferme le classifieur local sous `reference_cpu / hgp_reduced / resumable_exact_anchored_pair_closed_ball_classifier`. Il s'agit d'une réécriture reprenable de P8c recertifiée par P7b, non d'une mutualisation du noyau durable P7b. Un contexte move-only conserve une seule paire, une frontière DFS fixe bornée par la profondeur certifiée, au plus neuf intérieurs pour $K\leq10$, les compteurs exacts de coque et d'extérieur et les ancres préparées nécessaires au filtre centré. Chaque visite est facturée avant dépilement; un arrêt reprend sans revisiter le nœud précédent. Toute incertitude du filtre outward retombe sur les bornes AABB exactes.

Les étapes `record_ready`, `above_rank`, `budget_exhausted` et `complete` séparent la progression de la construction du record. Le centre et le niveau ne sont construits par `take_result` qu'après la fermeture scientifique et le préflight exact de P8l. Une paire hors rang ne les construit jamais. Le contexte, et non l'agrégat historique forgeable, porte l'autorité process-local et ne laisse consommer le record qu'une fois. Un terminal déjà certifié reste rejouable après migration de thread ou changement du mode d'arrondi; seule une classification active exige de retrouver l'environnement FP figé au démarrage.

Le différentiel court compare toutes les paires et les rangs fermés 2 à 6 entre passage ample, reprises d'une visite et force brute, puis compare pour chaque rang les records au flux P7b complet. Les cas $K=10$ à neuf et dix intérieurs, la cosphère, les budgets nuls, le déplacement, l'autorité étrangère et les transitions FP sont permanents. L'adaptateur P8j--P8k termine toujours le contexte actif avant de demander la candidate suivante et conserve l'identité $576=276+300$. GCC Release passe les deux CTests ciblés en 0,15 seconde au total, Clang 18 Release en 0,10 seconde, puis installation/export et consumer passent 1/1 en 0,01 seconde. Ce jalon P8k isolé n'est ni un raccord runner, ni un résultat de performance.

P8l assemble désormais la session de production locale sous `reference_cpu / hgp_reduced / bounded_sparse_anchored_pair_terminal_session`. La session move-only possède P8j, au plus une paire pending et un seul contexte P8k; un record prêt est préflighté avec son type et son nombre exact de références `PointId`, réservé, puis consommé une fois avant toute nouvelle candidate. Huit capacités totales immuables bornent les avances P8i/P8j, orientations, visites et prédicats groupés, candidates admises, visites P8k, records et références. Un cap total atteint reste un terminal incomplet typé et ne peut jamais sceller une autorité.

Chaque prune P8j est immédiatement recertifié par le certificat P8g authentique, puis sa masse dirigée vaut le nombre d'ancres multiplié par la longueur de la plage de feuilles. Le scellement non forgeable exige les identités $P+O=n^2$, $O=C+S$, $C=A=T$ et $T=H+R$, la partition Morton complète, tous les recroisements P8i--P8j, l'égalité des audits P8k et la conservation de chaque record. L'autorité possède les tokens process-local sans pointeur vers les sources, expose seulement les records constants et permet une remise move-only `release_records() &&` qui la révoque. Seuls les $R$ records réellement pertinents sont retenus sous caps explicites : aucune candidate, paire rejetée, cellule, coface ou incidence globale n'est matérialisée.

Le CTest P8l passe sous GCC Release en 0,052 seconde et Clang 18 Release en 0,040 seconde. Il ferme le fallback tridimensionnel $576=276+300$, une vraie masse de prune sur 20 points, le passage ample contre la segmentation d'une visite, l'identité des records avec P7b, les budgets de sortie atomiques, les caps totaux, le singleton et la remise exactement une fois. L'installation/export et le consumer externe passent 1/1 en 0,01 seconde avec une session et une autorité réelles. Ces contrôles sont une validation logicielle courte, ni un benchmark 50 k, ni une preuve de débit 10 M+.

P8m raccorde cette autorité à la façade terminale sous `reference_cpu / hgp_reduced / sparse_pair_terminal_facade`. Le certificat passe au schéma v3 et choisit exclusivement une source paire P7b fraîche ou une autorité P8l scellée. Dans la nouvelle voie, il recroise le token process-local, le rang fermé dérivé de la requête, les identités $P+O=n^2$, $C+S=O$ et les partitions classification--sortie, puis consomme une fois les records P8l et l'autorité supérieure P6b existante. Aucun `ExactPairSupportStreamResult` n'est construit, vérifié ou synthétisé.

Le digest sémantique P7b n'est pas réutilisé : P8m calcule un digest de sortie paire canonique, séparé par domaine et stable entre segmentations, puis un digest sémantique P8l lié aux digests cloud/LBVH, aux exigences et à cette sortie. Les caps et compteurs de travail restent une provenance d'exécution et ne changent pas l'identité scientifique. Sur le tétraèdre, le chemin P8l plus P6b reproduit exactement les 11 événements historiques, dont six supports de taille deux et un univers $\binom{4}{2}=6$; une segmentation à une unité garde les deux digests. Les tokens étrangers à coordonnées égales, les autorités déjà libérées et les mutations de digest, d'index, d'ordre H0 ou de diagnostic échouent sans payload. Avec le CTest P8l, les deux tests passent 2/2 sous GCC Release en 0,14 seconde et sous Clang Release en 0,16 seconde; le contrat statique Phase 9 v9 passe aussi. Aucun benchmark, CUDA ou GCP n'est lancé.

P8n remplace maintenant la source paire par défaut du runner sous `reference_cpu / hgp_reduced / sparse_pair_product_runner`. La précondition $n>K$ fixe le rang fermé maximal à $K+1$. Le budget `support_work` devient explicitement un cap pour chacun des six axes physiques P8l; les sorties sont bornées par $R$ records et $R(K+2)$ références, tandis que P6b conserve sans changement son facteur $K+4$. Le runner reprend après un épuisement local, s'arrête en code 2 sur une capacité totale typée, recroise toutes les partitions avant scellement, puis déplace les deux autorités dans P8m. Il ne construit plus aucun résultat ou budget P7b.

Le rapport passe au schéma v3, sépare budget d'avance, capacités totales et audit P8l, publie explicitement `p7b_replay_performed=false` et retire les anciens compteurs P7b au lieu de les renommer. Sur `uniform_latin`, $n=5$, $K=4$, la projection scientifique historique reste exactement composée de 13 événements terminaux, 26 batches, 30 rôles, 13 familles de selles, 29 bras et 12 nœuds de forêt avec les mêmes décisions aval. Un cap de sortie d'un record ferme `total_output_record_capacity` avant P6b, et 50 001 points restent rejetés avant génération. Le CTest unique passe sous GCC et Clang Release en 0,13 seconde; ce différentiel agrégé s'appuie sur l'égalité exacte des événements déjà couverte par P8m.

P8o répond au premier diagnostic produit, sans campagne. Sur l'unique exécution `uniform_latin`, $n=50\,000$, $K=10$, P8n s'arrête en 221,773 ms au cap `total_classification_node_visit_capacity`. La voie paire ne prend encore que 31,235 ms, mais elle ouvre 452 candidates, en classe 451 `above_rank` et ne produit aucun prune après 43 visites groupées. L'univers dirigé vaut 2 500 000 000 : relever le cap de 20 000 visites P8k aurait seulement caché l'absence de certificat collectif.

L'analyse du premier groupe montre que le halo de 64 témoins n'est pas vide d'information; c'est la boîte des 32 ancres qui introduit des coins hybrides. Le resserrement exact sur les ancres réelles garde déjà 27 à 29 témoins communs sur les feuilles observées et 12 sur le sous-arbre Morton de 6 249 feuilles dans le diagnostic binary64 indépendant; seule la recertification dyadique du code peut autoriser les prunes. P8o implémente ce maximum discret, facture chaque signe ancre--témoin, reprend à l'offset d'ancre et conserve le masque commun unique. Il n'ajoute ni masque par ancre, ni second ordonnanceur.

La contre-fixture permanente, les budgets physiques, la reprise et les quatre coutures P8g, P8i, P8l et runner passent 4/4 sous GCC Release en 0,72 seconde et sous Clang Release en 0,35 seconde. Le runner distingue désormais slots témoins, réemplois hérités, signes physiques et découvertes strictes. P8o reste `architecture_only`, `public_status=not_claimed`; ces tests ne mesurent ni nouveau temps 50 k, ni CUDA, ni 10 M+.

L'unique diagnostic borné post-P8o sur `uniform_latin`, $n=50\,000$, $K=10$, s'arrête en 259,065 ms au cap `total_grouped_traversal_exact_predicate_capacity`. La voie paire prend 54,900 ms : 107 visites groupées ouvrent 6 576 slots, réemploient 756 succès hérités, dépensent 20 000 signes exacts et découvrent 138 témoins stricts. Les 38 prunes authentifiés couvrent 330 368 paires dirigées; 352 candidates, toutes `above_rank`, coûtent encore 12 808 visites P8k. Ce résultat ne ferme même pas le premier groupe : les 50 avances se décomposent en 38 prunes, 11 feuilles de 32 candidates et un arrêt, donc seulement $330\,368/32+11=10\,335$ feuilles requêtes sur 50 000. Multiplier simplement le budget masquerait ainsi une intersection commune trop étroite au lieu d'établir un chemin produit.

P8p installe le mode `reference_cpu / hgp_reduced / common_first_singleton_fallback`. Le parcours commun P8o reste la première ligne. Un nœud contenant une ancre réelle est seulement descendu, sans signe témoin : pour cette ancre diagonale, $\phi(x,p,p)=\left\Vert x-p\right\Vert^2\geq0$, donc aucun témoin ne peut certifier tout le nœud. Le premier sous-arbre hors diagonale encore inconclusif devient une frontière interne, jamais une autorité. P8i parcourt alors cette même frontière séquentiellement pour chaque ancre du groupe avec P8h/P8o singleton et un halo Morton frais d'au plus 64 témoins autour de sa propre feuille. Les autres ancres du groupe peuvent être témoins; un seul parcours et un seul pool singleton supplémentaires sont vivants.

La couverture repose sur la partition disjointe $A\times L(Q)=\bigsqcup_{p\in A}(\lbrace p\rbrace\times L(Q))$. En amont, les prunes communs et les frontières sont disjoints; dans chaque frontière, chaque singleton P8h partitionne le sous-arbre en prunes certifiés et feuilles non résolues. P8j snapshotte donc l'unique ancre et sa plage terminale au lieu de réémettre le groupe entier, et P8l conserve sa formule de masse sans double compte. Les passages amples et segmentés ferment la même partition, le même travail exact et le même oracle paire sur les fixtures permanentes. La recertification finale des quatre CTests ciblés passe 4/4 sous GCC Release en 0,54 seconde et sous Clang Release en 0,49 seconde. Cette fermeture logicielle ne promet ni rappel du halo, ni SLO 50 k, ni débit 10 M+.

L'unique diagnostic borné post-P8p sur `uniform_latin`, $n=50\,000$, $K=10$, s'arrête en 174,871 ms au cap `total_grouped_traversal_exact_predicate_capacity`. Il dépense 20 000 signes exacts en 360 visites groupées et ne prépare qu'un seul fallback singleton avant l'arrêt; le pipeline reste donc incomplet. Ce résultat localise le terme dominant dans la répétition des signes du premier singleton, sans constituer un SLO, une extrapolation de débit ou une autorité scientifique.

P8q remplace la partition immédiate en singletons par `reference_cpu / hgp_reduced / common_first_recursive_anchor_subrange_partition`. Pour chaque frontière commune $A\times L(Q)$, l'intervalle Morton contigu des ancres est partagé en deux moitiés exactes. Chaque sous-intervalle $B$ tente d'abord au seul nœud $Q$ un certificat P8o commun avec un halo frais orienté vers le côté Morton de $Q$. Un succès ferme $B\times L(Q)$; un échec sans autorité partage de nouveau $B$; seule une feuille de cette partition d'ancres lance le parcours P8h singleton complet. L'identité $B=B_0\sqcup B_1$, appliquée par induction jusqu'aux singletons, préserve la couverture et ne donne aucune force scientifique aux événements de partage.

Le halo orienté réserve d'abord jusqu'aux trois quarts de ses 64 slots au côté faisant face à $Q$, complète depuis l'autre côté, puis revient au côté préféré; si $Q$ chevauche l'intervalle d'ancres, la sélection symétrique P8p est conservée. Cette politique est une proposition heuristique : seul le signe dyadique exact et le reçu P8g autorisent un prune. L'état garde une pile fixe d'au plus $G$ sous-intervalles, un seul contexte de sonde ou singleton et un seul pool supplémentaire; il ne construit ni arbre dual global, ni banque de halos, ni matrice $G\times W$, ni catalogue de paires, facettes, cofaces, incidences, cellules, Gamma ou mosaïque de Delaunay d'ordre supérieur. Les sondes internes infructueuses ajoutent au plus $O(GW\lceil\log_2 G\rceil)$ signes par frontière équilibrée avant le pire cas singleton inchangé $O(GW\lvert T_Q\rvert)$; cette amélioration reste donc adaptative, pas une nouvelle borne sous-quadratique.

Les quatre contrôles ciblés certificat, ordonnanceur, session et runner passent 4/4 sous GCC Release en 0,33 seconde et 4/4 sous Clang Release en 0,29 seconde. Ils recertifient l'arbre de sous-intervalles, chaque prune, les identités d'audit, la stabilité roomy--segmentée et la couture runner. Aucun benchmark long, test massif, CUDA ou GCP n'est inclus.

Trois raffinements sont enregistrés comme dettes non bloquantes : exclure explicitement des pools les témoins appartenant à $Q$, hériter entre sondes les relations strictes déjà certifiées, et proposer des témoins depuis une cible continue de milieu ou de projection avant recertification exacte. Ils ne retardent pas le passage à la voie massive dès qu'une exécution complète à chaud sur 50 000 points et $K=10$ passe sous 0,5 seconde.

P8r ferme 14R comme incrément borné `architecture_only`, sous `cuda_g4_plus_reference_cpu / hgp_reduced / dynamic_bounded_witness_subtree_core_plus_massive_sparse_pair_prefix_smoke`, avec `public_status=not_claimed`. Pour chaque nœud requête du fallback, il cherche au plus 64 nœuds témoins LBVH, conserve seulement des reçus disjoints stricts pour toutes les ancres réelles et rejoue les `PointId` sélectionnés par P8g avant tout prune. L'ordre par milieu d'intervalles est une proposition; échec, cap ou insuffisance restaure le halo et retombe fail-open. L'état fixe n'ajoute ni arbre dual global, ni arène de paires, ni cellule, coface, incidence, Gamma ou mosaïque de Delaunay d'ordre supérieur.

Les cinq CTests GCC Release ciblés passent 5/5 en 0,43 seconde. L'unique gate final `uniform_latin`, 50 000 points et $K=10$, s'arrête toutefois avec le code 2 à `total_grouped_traversal_exact_predicate_capacity` après 177,470 ms au total, dont 18,158 ms pour la voie paire. Ses 120 prunes authentifiées ferment 1 509 560 paires dirigées et aucune candidate n'est ouverte, mais le premier groupe reste actif : le pipeline incomplet ne satisfait donc pas le gate inférieur à 0,5 seconde.

Le smoke massif passe localement sur 257 points, puis sur la G4 `SPOT` gardée avec `cuda14m`, au SHA exact `a9fbcb3eab6f4472c4a5169dd8179b92051183e7`, pour 10 000 001 points et $K=10$. Le total de 23 831,153 ms se décompose en génération 14 997,149 ms, canonicalisation 2 418,723 ms, spatial 6 349,896 ms et préfixe P8l 0,052 ms; le pic RSS vaut 5 851 394 048 octets et la capacité device 3 082 232 059 octets. Les 64 kernels et l'unique soumission de bibliothèque établissent la capacité du constructeur spatial suivie d'un préfixe borné, pas celle du pipeline produit : zéro record est retenu et tous les claims massifs ou publics restent faux. La génération GCE exacte est certifiée `TERMINATED`, sans autre VM active.

14S implémente ensuite `reference_cpu / hgp_reduced / bounded_morton_triangular_block_pair_schedule`, avec `public_status=not_claimed`. La récurrence $T(N)=T(L)\sqcup C(L,R)\sqcup T(R)$ partitionne exactement les paires non ordonnées; un produit croisé ne partage que sa plage d'ancres jusqu'à 32 éléments puis appelle P8r. Chaque reçu process-local recertifie P8g sur deux plages Morton disjointes et transporte les masses exactes $\lvert A\rvert\lvert Q\rvert$ et $2\lvert A\rvert\lvert Q\rvert$. La pile est bornée par $3D+1$ et aucune structure duale ou globale interdite n'est matérialisée.

La fixture $n=8$ d'ordre Morton `[1,0,2,6,3,4,5,7]` ferme exactement 28 paires avec des budgets ample et unitaire; la fixture colinéaire recertifie un prune symétrique positif de masses 2 et 4 et rejette trois liaisons invalides. La compilation stricte passe et les six CTests ciblés passent 6/6 en 0,68 seconde sous GCC Release. Le gate unique 50 k/$K=10$ s'arrête néanmoins au cap exact après 199,733 ms, dont 28,427 ms pour la voie paire : 41 blocs croisés sont ouverts, aucun bloc entier n'est certifié, puis 25 blocs singleton ouvrent 96 paires. Il n'existe ni autorité terminale ni sortie; le temps inférieur à 0,5 seconde n'est pas un SLO. L'ordre diagonal-first est désormais la dette mesurée : prioriser les blocs de grande masse, puis joindre ce flux au sink/checkpoint borné, sans micro-réglage du halo. GCP n'est pas utilisé pour 14S.

### Jalon 14T — priorité aux produits croisés et partage symétrique optionnel

14T conserve exactement la partition triangulaire de 14S. Le mode `cross-first` ne modifie que l'ordre de dépilement : $C(L,R)$ est visité avant les deux diagonales suspendues, qui restent présentes et seront toutes consommées. Sur un produit croisé inconclusif $A\times Q$, le partage symétrique optionnel choisit le plus grand côté LBVH non feuille et choisit $Q$ en cas d'égalité; les identités $A\times Q=(A_L\times Q)\sqcup(A_R\times Q)$ et $A\times Q=(A\times Q_L)\sqcup(A\times Q_R)$ préservent la couverture sans omission ni duplication. L'option historique qui ne partage que $A$ reste disponible.

Le chemin produit courant active `cross-first`, garde le partage symétrique en opt-in et désactive P8r pour revenir au parcours P8g ancre d'abord. Ces trois choix sont opérationnels : aucun n'autorise un prune. Seul un signe exact ou filtré par intervalle certifié, puis le reçu P8g recertifié au point d'usage, porte la décision; toute inconclusion reste fail-open. La fermeture 14T est structurelle : les tests de l'ordonnanceur couvrent les 28 paires et un partage de requête, tandis que le chemin produit conserve volontairement le partage ancre seulement. Elle ne revendique donc pas une validation produit du mode symétrique opt-in. 14T reste `reference_cpu / hgp_reduced / architecture_only`, avec `public_status=not_claimed`.

### Jalon 14U — ordre flottant propositionnel et filtre d'intervalle exact

14U est validé comme incrément hôte `architecture_only`. Il conserve le pool canonique trié par `PointId` et la signification de chacun de ses bits, mais propose un ordre de visite calculé en flottant pour rencontrer plus tôt les témoins prometteurs. Pour un témoin $x$, un bloc requête $Q=\prod_i[l_i,u_i]$ et une ancre $a$, il emploie le score $s_Q(x)=\max_{a\in A}\sum_i(x_i-a_i)(x_i-q_i^\star)$, où $q_i^\star=l_i$ si $x_i-a_i\geq0$ et $q_i^\star=u_i$ sinon. Cette formule est exactement le maximum séparable de l'expression P8g pour $x$ et $a$ fixés, mais son évaluation `long double`, ses valeurs non finies et son tri restent strictement propositionnels : aucun rappel, optimum de priorité ou gain asymptotique n'est revendiqué.

Le filtre associé évalue une enveloppe binary64 dirigée vers l'extérieur de la même expression. Un intervalle strictement négatif ou strictement positif décide le signe; un environnement non supporté, un intervalle invalide ou contenant zéro retombe sur le prédicat dyadique exact existant. Si l'environnement n'est pas supporté dès la construction, l'ordre redevient canonique et tous les signes utilisent le fallback exact; s'il perd en cours de reprise un contrat initialement certifié, l'exécution échoue fermée avant tout calcul propositionnel. Le garde restaure le mode d'arrondi et les exceptions de l'appelant. P8r et 14U sont désormais mutuellement exclusifs, et l'identité `signes logiques = filtrés négatifs + filtrés positifs + fallbacks exacts` appartient aux invariants terminaux de P8l. Le nom historique `exact_predicates` demeure un alias de compatibilité; le runner publie aussi `grouped_logical_signs` et le verdict de partition.

Les tests courts ciblés passent après reconstruction, y compris les modes FENV non supportés, la reprise terminale, le rejet des propositions concurrentes, les budgets segmentés et le différentiel du runner. Sur la petite fixture produit, les 78 signes logiques se partagent en 7 filtrés négatifs, 37 positifs et 34 fallbacks exacts; 22 préparations évaluent 86 scores finis. L'unique gate `uniform_latin`, 50 000 points et $K=10$, s'arrête avec le code 2 au cap de 20 000 signes logiques : 797 négatifs, 15 451 positifs, 3 752 fallbacks exacts, 491 préparations, 31 424 scores, 26 blocs croisés dont trois certifiés, 14 partages d'ancre et zéro record. Il n'existe ni autorité terminale, ni résultat matérialisé, ni SLO satisfait; aucun second gate 50 k n'est lancé.

Ni le partage symétrique, ni `cross-first`, ni l'ordre flottant ne donnent une borne déterministe $O(nK^c)$ au pipeline actuel. Lorsque les certificats collectifs échouent, une famille d'entrées laisse $\Omega((n-K)^2)$ interactions de paires terminales à examiner; changer leur ordre ne supprime pas ce travail. Pour le profil massif, la suite doit donc émettre des chunks bornés, engager un checkpoint recertifiable et reprendre depuis le dernier `HEAD`, sans conserver le vecteur global des records. Le prochain pivot scientifique vise des minima de Morse certifiés et leur réduction hiérarchique, pas le catalogue Gabriel brut : Gabriel peut proposer des arêtes, mais ne remplace ni le certificat de minimum pertinent pour $K$, ni la décision Morse exacte, ni la preuve de complétude du résultat réduit.

### Jalon 14V — chunks P8l bornés, publication atomique et reprise

14V livre le premier raccord fonctionnel `reference_cpu / hgp_reduced / durable_bounded_sparse_anchored_pair_chunk_run`, avec `deployment_status=architecture_only` et `public_status=not_claimed`. P8l peut déplacer son suffixe de records non scellé tout en conservant les indices et caps cumulatifs; le premier drain, même vide, révoque définitivement la voie `seal()` résidente. Chaque chunk avance P8l un nombre fixe d'appels, borne records, références, entiers exacts et payload, puis publie sa transition avec l'`AtomicLinearRunStore` de Phase 15.

Le payload n'est jamais décodé pour créer une autorité. À la publication comme à la reprise, un contexte frais repart du nuage et du LBVH authentifiés, rejoue P8l jusqu'aux bornes de la transition, réencode le chunk et exige l'égalité octet par octet ainsi que le digest de checkpoint. Le test court publie un préfixe, détruit le store et le contexte, rouvre depuis `HEAD`, termine la fixture puis retrouve exactement les records et l'audit de la session P8l résidente. Deux caches de rejeu au plus sont conservés; ni catalogue global de paires, ni facette, coface, incidence, cellule Gamma ou mosaïque de Delaunay d'ordre supérieur ne sont construits.

<!-- TODO 14V perfectionné : ajouter la matrice complète d'injection de fautes/orphelins au niveau applicatif, une ressource-gate mesurée, un checkpoint de curseur P8l évitant le rejeu linéaire du préfixe, puis brancher directement les consommateurs supports 3--4. Ces travaux ne bloquent pas le MVP fonctionnel, mais restent requis avant une qualification produit 10 M+. -->

14V supprime le verrou de rétention globale des records de paires; il ne supprime pas l'obstruction de calcul quadratique et ne valide donc encore ni le SLO complet 50 k, ni un pipeline scientifique 10 M+. Aucun nouveau gate long ni GCP n'est lancé pour cet incrément.

### Jalon 14W — run H0 pair-only borné et recertifié

14W est validé sur hôte sous `reference_cpu / hgp_reduced / bounded_pair_only_event_candidates`, avec `deployment_status=architecture_only` et `public_status=not_claimed`. Le MVP consomme la projection acceptée d'un chunk P8l seulement après son rejeu scientifique frais, puis dérive au plus les rôles H0 portés par ses événements de support paire. Records source, candidats, diagnostics exacts et références de `PointId` possèdent des caps séparés; le payload reste borné par 14V. Un cap insuffisant provoque un refus atomique sans run projeté.

La clé scientifique est la clé canonique déjà ordonnée par `event_less`; ni la frontière de chunk, ni l'ordre de découverte ne participent à l'identité. Chaque candidat ou diagnostic conserve son `source_output_record_index`, locator global du seul flux P8l. Seuls les indices finaux `event_index` et `event_projection_index` de Phase 10 restent différés; aucune absence de binding n'est reclassée en racine, naissance ou composante isolée. Le run ne fusionne pas les supports trois--quatre, ne construit pas de reducer et ne revendique ni catalogue critique complet, ni attaches complètes, ni autorité H0.

<!-- TODO 14W perfectionné / 14X : fusionner extérieurement et canoniquement les flux pair et higher-support avant toute autorité, puis construire l'oracle global naissances--selles de type Morse--Borůvka. Sa promotion exigera la complétude certifiée des coupes, la fermeture de toutes les incidences silencieuses utiles et M.1, niveaux égaux compris. -->

### Jalon 14X — fusion H0 sparse multi-support paginée

La porte 14W ouvre un MVP `reference_cpu / hgp_reduced / bounded_sparse_direct_h0_candidate_merge`, toujours `architecture_only` et `public_status=not_claimed`. Un run pair 14W ou un segment higher-support déjà porté par son autorité terminale est projeté vers le même candidat arité deux à quatre. La provenance paire garde le locator P8l global; la provenance supérieure d'un candidat garde le couple `(chunk_sequence, local_event_index)`, tandis qu'un diagnostic emploie son `local_diagnostic_index`. Le champ C++ commun `local_kind_index` encode l'un ou l'autre selon le variant; les certificats de prune détruits interdisent de forger un offset global de record.

Chaque run est trié par la clé exacte de la façade terminale. Une session non copiable et non déplaçable prend possession des runs, les valide une seule fois, puis émet un merge k-way à fan-in fixe en pages de taille fixe. Son état privé conserve une position par run et la position source du dernier candidat, sans recopier sa liste d'intérieurs; un doublon, y compris à une frontière de page, empoisonne la session sans committer la page fautive. Pour $N$ candidats, $D$ diagnostics, un fan-in $F$ et une page $M$, la validation initiale coûte $O(N+D)$, chaque page $O(F+Mlog F)$ et le scratch de merge $O(F+M)$. Elle ne matérialise ni univers de supports, ni facette, coface, incidence Gamma, cellule ou mosaïque de Delaunay d'ordre supérieur. Les diagnostics restent séparés et aucun index final, lot Morse, reducer, racine ou verdict H0 complet n'est produit.

<!-- TODO 14X perfectionné : ajouter d'abord un lecteur séquentiel certifié borné des runs `AtomicLinearRunStore`, puis une passe durable à fan-in fixe et enfin le spool multi-passe; drainer le producteur higher-support sans retenir tous ses segments et recertifier les reprises/crashs. Le futur 14Y devra ensuite prouver l'oracle global naissances--selles, la complétude des coupes, les incidences silencieuses et M.1; ces obligations ne sont pas contournées par le merge mécanique. -->

### Jalon 14Y — sparsifieur de partitions Morse--Borůvka relatif

La porte combinatoire bornée est satisfaite par le sweep 6.23 : il fournit, pour $n\leq14$ et $K\leq10$, des naissances, des selles, leurs niveaux exacts et tous leurs terminaux déjà raccordés. Cette porte autorise uniquement le noyau relatif `reference_cpu / hgp_reduced / certified_relative_morse_boruvka`, en `architecture_only` et `public_status=not_claimed`. Elle ne raccorde pas 14X à 14Y et ne certifie aucune complétude d'entrée sur le chemin produit.

Pour $B$ naissances, $S$ selles et $A$ références de terminaux avec $A\leq4S$, l'entrée est une arène CSR plate. Chaque selle est développée en une étoile canonique de liens portant son niveau et sa provenance; cette expansion engendre exactement la même relation d'équivalence à chaque seuil. Chaque ronde photographie les racines et balaie exactement tous les liens sortants. Pour chaque racine, le transcript conserve seulement le nombre exact de liens co-minimisants et l'unique lien choisi selon la clé canonique; ce lien référence sa selle et donc le niveau minimal sans recopier le rationnel exact, et les identifiants des autres co-minimiseurs ne sont pas persistés. Seuls les liens choisis sont contractés simultanément sur la photographie gelée. Aucun choix d'une racine ne voit donc la contraction d'une autre racine du même lot.

Les selles de provenance des liens sélectionnés sont ensuite triées par niveau exact puis identifiant dense. Une passe de quotient canonique ne retient une selle que si elle relie au moins deux composantes courantes. Si $C$ est le nombre final de composantes de l'hypergraphe fourni, cette base contient au plus $B-C$ selles et préserve, relativement à cette entrée, toutes ses partitions de seuil strictes et fermées. Elle est une base de transport de partitions, pas encore un `MergeForest`, une hiérarchie publique ou une preuve de multiplicité des selles.

Si $L\leq A$ est le nombre de liens étoilés, le travail des rondes est $O((B+L)\lceil\log_2 B\rceil)$ après validation et expansion linéaires, le scratch actif reste $O(B+L+S)$ et le transcript plat $O(B+L+S)$. Un lien non choisi peut rester co-minimisant à la ronde suivante : persister tous ses identifiants coûterait jusqu'à $O(L\log B)$ et est donc explicitement interdit. Le vérificateur frais rescane l'autorité pour recertifier les niveaux référencés, comptes et choix. Ces bornes n'incluent pas la bit-complexité des comparaisons rationnelles exactes. Le noyau ne matérialise ni univers de supports, ni facette ou coface globale, ni incidence Gamma, ni cellule, ni mosaïque de Delaunay d'ordre supérieur.

La validation courte adapte uniquement dans le test les quatre autorités bornées 6.23 `q2`, miroir simultané, terminal partagé et continuation E5. Elle ajoute la contradiction permanente `relative_boruvka_p0_whole_hyperedge_contraction` : cinq naissances de niveau zéro et les selles $s_0=(1,\lbrace1,2\rbrace)$, $s_1=(1,\lbrace3,4\rbrace)$, $s_2=(2,\lbrace2,3\rbrace)$ et $s_3=(3,\lbrace0,2,3\rbrace)$. Contracter toute $s_3$ parce que la racine zéro la choisit omet $s_2$ et casse la partition fermée au seuil deux; l'expansion en liens conserve $s_2$. Le CTest strict GCC Release compare la connectivité paire à paire du catalogue complet et de la base retenue à chaque niveau distinct, en coupe stricte puis fermée, et passe 1/1 en 15,19 secondes, temps total identique. Cet oracle exhaustif demeure borné et n'entre pas dans l'architecture produit.

<!-- TODO 14Y perfectionné : fournir au noyau un catalogue produit complet et scalable avec les incidences silencieuses utiles. La complétude des entrées, M.1, le raccord durable de 14X, la construction de la hiérarchie et les objectifs complets 50 k et 10 M+ restent des portes séparées. -->

### Jalon 14Z — lecteur séquentiel recertifié des runs H0 paire

Les portes 14V et 14X sont satisfaites pour un raccord de transport `reference_cpu / hgp_reduced / certified_bounded_atomic_pair_run_reader`, toujours `architecture_only` et `public_status=not_claimed`. 14Z ne change ni le wire ni le protocole de publication : il rouvre un `AtomicLinearRunStore` pair finalisé avec une ancre externe obligatoire, exige que cette ancre soit exactement le `HEAD` final relu, puis reçoit les transitions dans leur ordre certifié. Chaque transition est recertifiée par un rejeu P8l frais avant d'être projetée successivement par 14W et 14X vers un unique run commun pair.

Le callback reçoit une référence empruntée valable seulement pendant l'appel. Un seul run commun est vivant côté lecteur et aucun historique n'est retenu; copier ce run appartient à l'enveloppe mémoire du consommateur. Les effets de tous les callbacks restent provisoires jusqu'au retour réussi de la relecture complète : une exception, une corruption tardive, une ancre divergente ou un dernier chunk non terminal impose de jeter tout état dérivé. Le lecteur exige un préfixe non vide, des séquences et bornes d'avances contiguës, exactement une projection `session_complete` en dernière position, puis refuse tout arrêt par capacité totale ou limite d'avances.

Pour $T$ transitions bornées, de populations locales $C_i$, $D_i$ et $R_i$, le travail de projection après rejeu vaut $\sum_{i=1}^{T}O(C_i\log C_i+D_i)$ et le pic additionnel est $O(B_{\mathrm{wire}}+C_{\max}+D_{\max}+R_{\max})$, en plus des deux caches P8l déjà déclarés. Il est indépendant du nombre de transitions retenues et ne construit ni catalogue global de supports, ni facette, coface, incidence Gamma, cellule ou mosaïque de Delaunay d'ordre supérieur. Les runs sont triés individuellement; leur concaténation n'est pas une fusion globale.

Le CTest GCC Release ciblé passe 1/1 en 0,03 seconde et la régression store--14V--14W--14X--14Z passe 5/5 en 0,13 seconde. Elle couvre la réouverture après exception du callback, l'ancre finale contre une ancre ancienne, le refus d'un préfixe non terminal, les quatre caps cumulatifs juste insuffisants, l'identité champ par champ des runs et les compteurs de résidence nuls ou unitaires. Aucun benchmark, CUDA ou GCP n'est lancé. 14Z ne réduit pas le nombre de décisions de paires, ne draine pas encore higher-support, ne publie pas de passe à fan-in durable et ne ferme donc ni le SLO complet 50 k, ni le chemin produit 10 M+.

<!-- TODO 14Z perfectionné : la publication et la recertification des chunks higher-support sont fermées par 14AB; lier maintenant les voies paire et supérieure à une source commune, puis définir une passe durable à fan-in fixe qui conserve aussi le sidecar des diagnostics avant le spool multi-passe et 14Y. -->

### Jalon 14AA — drain borné des supports supérieurs vers les runs H0

Le gate court 14AA est satisfait pour `reference_cpu / hgp_reduced / bounded_unsealed_higher_support_h0_run`, toujours en `architecture_only` et avec `public_status=not_claimed`. `ExactHigherSupportTerminalSession` sépare désormais le nombre cumulatif de chunks du nombre de segments résidents. `drain_next_unsealed_segment` exécute au plus le prochain chunk scientifique et renvoie un jeton move-only opaque, ou les états explicites `terminal` et `maximum_chunk_count_reached` sans mutation. Un jeton prêt lie le segment au manifeste copié depuis la même session; tant qu'il n'a pas été projeté avec succès, une lease partagée interdit mécaniquement tout chunk suivant. Le premier drain réussi, même vide, révoque définitivement `run_to_terminal` et `seal`; aucun préfixe libéré ne peut donc être requalifié en autorité résidente.

Les compteurs libérés et résidents conservent l'identité avec l'audit cumulatif, les digests de checkpoint et de chaîne relient chaque segment à son prédécesseur et à son successeur, et les nombres exacts de certificats de prune et de reçus de rang détruits restent comptés sans recréer leurs payloads. La projection consommatrice 14X valide d'abord le manifeste, le segment, chaque payload et le compte de références, alloue les slots finaux, trie seulement une permutation d'indices et rejette les doublons avant le premier déplacement. Le commit ne contient ensuite que des déplacements `noexcept` vérifiés statiquement; une exception de limite, d'allocation ou de comparaison laisse donc le même jeton intact et rejouable. Le run commun passe au schéma v2 et conserve un contrat supérieur $O(1)$ comprenant le manifeste, la position dense du chunk, les quatre digests de liaison, les statuts et les comptes détruits. Le champ `local_kind_index` encode `local_event_index` pour un candidat et `local_diagnostic_index` pour un diagnostic; aucun offset global de record n'est inventé.

Le segment drainé et le run projeté ne revendiquent ni terminalité durable, ni fraîcheur externe, ni réduction hiérarchique, ni autorité H0 ou publique. Une fusion refuse désormais deux runs higher issus de manifestes distincts, même pour le même $n$ et le même $K$; elle ne certifie pas encore la continuité de leur chaîne, et la liaison d'autorité commune entre la voie pair et la voie higher reste à construire avant 14Y. Soient $B$ l'empreinte physique du checkpoint borné, $Q_i$ l'empreinte physique totale du payload transitoire du chunk $i$ — candidats, diagnostics, références, certificats de prune, reçus de rang et rationnels exacts inclus — et $C_i$ son nombre de candidats. Le pic local du producteur et d'un handoff vaut $O(B+Q_i+C_i)$, le dernier terme étant la permutation d'indices, et reste indépendant du nombre de chunks déjà libérés. Cette borne exclut explicitement les runs que l'appelant choisit de conserver; le merge 14X actuel reçoit encore leur vecteur complet. Aucun univers de supports, facette, coface, incidence Gamma, cellule ou mosaïque de Delaunay d'ordre supérieur n'est matérialisé.

Les deux CTests courts propres à 14AA passent 2/2 en 0,94 seconde sous GCC Release et 0,34 seconde sous Clang Release. Avec le lecteur 14Z aval, la régression passe 3/3 en 1,01 et 0,41 seconde respectivement. Chaque run consommé est comparé au run issu de l'autorité résidente. Le tétraèdre couvre segments vides, payloads événementiels, rejet précommit puis reprise du même jeton, lease anti-saut, abandon fail-closed et coupe-circuit de chunks; une coque de cinq sites couvre le diagnostic higher déplacé; une fixture de neuf points couvre des certificats de prune et reçus de rang réellement détruits; deux nuages distincts de même taille couvrent le refus de mélange de manifestes; un run muté au schéma v1 est rejeté. Les compteurs, digests, contrats de provenance, locators, rôles locaux et diagnostics sont conservés, la résidence du producteur retombe à zéro après chaque projection et la voie de scellement reste révoquée. Sur le nouveau chemin de drain, cet incrément évite la rétention cumulative des segments dans le producteur higher; `run_to_terminal` conserve volontairement sa voie résidente, et le test comme le merge accumulent encore les runs projetés en aval. Il ne réduit pas l'univers implicite combinatoire et ne ferme ni 50 k, ni 10 M+.

### Jalon 14AB — run durable recertifié des chunks H0 higher-support

La porte 14AA est satisfaite pour `reference_cpu / hgp_reduced / durable_recertified_higher_h0_chunk_run`, toujours en `architecture_only` et avec `public_status=not_claimed`. Le contexte lie dans un digest applicatif le manifeste supérieur immuable complet, le budget scientifique de chunk, les limites de projection H0 et les limites wire. Chaque transition de l'`AtomicLinearRunStore` porte exactement un segment scientifique, y compris lorsqu'il est vide : sa séquence, son index de chunk et son début de batch valent le même curseur dense, et sa fin de batch vaut le curseur suivant. Le checkpoint source, le checkpoint successeur, le budget fixe et la chaîne du store ancrent ainsi un unique préfixe reprenable.

Le payload big-endian canonique conserve le manifeste sémantique, les quatre digests scientifiques de liaison, les statuts, les comptes détruits, les événements, les diagnostics, leurs références et leurs rationnels exacts décimaux. Il ne sérialise ni frontière supérieure, ni checkpoint complet, ni certificat de prune ou reçu de rang détruit. Surtout, aucun décodeur scientifique ne lui accorde d'autorité : à la publication, à la reprise et au nettoyage d'un orphelin, un vérificateur repart des racines canoniques du même nuage et du même LBVH, rejoue jusqu'au curseur demandé, réencode le segment attendu et compare octet par octet payload, digests source--successeur et empreinte de budget. Seul le jeton fraîchement rejoué est ensuite consommé vers un run commun H0 14X. Une mutation wire ne peut donc pas devenir un objet exact simplement parce qu'elle est syntaxiquement plausible.

Le producteur garde au plus un jeton de segment en attente jusqu'à `durably_published`. Un refus atomique avant `HEAD` permet de réemployer ce même jeton; une issue indéterminée interdit toute nouvelle publication sur le store courant et impose une réouverture certifiée. Chaque store créé ou rouvert par l'adaptateur reçoit en outre une capacité process-local propre au contexte 14AB; `publish_next_chunk` refuse un store installé par un autre contexte. Cette capacité n'est ni sérialisée, ni dérivable du contrat durable et ne remplace aucun digest scientifique. Elle identifie toutefois le contexte, pas le namespace durable précis : après une issue indéterminée, l'API actuelle ne prouve pas qu'un autre store déjà rouvert par le même contexte désigne le répertoire incertain. Jusqu'à l'ajout d'une identité stable de run, l'exploitation doit donc imposer un contexte par cible durable et rouvrir ou nettoyer explicitement la cible incertaine avant de l'abandonner. À la réouverture, le préfixe committé est recertifié dans l'ordre, un callback synchrone reçoit seulement la projection fraîche courante, puis la production peut reprendre au successeur. Une ancre externe, lorsqu'elle est fournie, est comparée par le store; 14AB autorise néanmoins la reprise d'un préfixe non terminal et ne remplace pas le futur lecteur final qui devra exiger une ancre égale au `HEAD` courant et un dernier chunk terminal.

Toutes les limites scientifiques, de chunks, de texte décimal exact individuel, de somme de textes exacts et de payload doivent être finies; le payload doit pouvoir contenir ses 278 octets fixes et le span maximal du store vaut exactement un. Le cap individuel borne la conversion et l'encodage décimaux, pas la mémoire de l'objet `BigInt` source déjà présent dans le segment. Le préflight par longueur binaire est sûr mais conservateur : pour un entier gigantesque il peut refuser avant conversion une valeur dont le texte exact aurait encore tenu. Le contexte possède deux caches de session persistants bornés, producteur et vérificateur, et le nettoyage d'un orphelin peut leur adjoindre un scratch éphémère. Il ne retient aucun historique de transitions ou de runs communs. Si $B$ borne un checkpoint supérieur, $Q$ tous les objets scientifiques et exacts maximaux d'un chunk, copie transitoire de magnitude incluse, $W$ son wire et $C$ sa projection H0, l'architecture process-local reste $O(B+Q+W+C)$ avec un nombre constant de caches, indépendamment du nombre de chunks déjà committés; producteur, vérificateur, projection et buffers peuvent néanmoins coexister. Cette borne par caps n'est pas une mesure RSS et exclut le nuage, le LBVH, les fichiers durables proportionnels au préfixe et toute accumulation choisie par le visitor. La reprise froide rejoue linéairement ce préfixe, et le producteur puis le vérificateur recalculent actuellement chaque segment au lieu de partager une autorité mutable. Aucun univers global de supports, facette, coface, incidence Gamma, cellule ou mosaïque de Delaunay d'ordre supérieur n'est matérialisé.

Les bibliothèques strictes GCC et Clang compilent. La matrice courte higher stream--store atomique--merge 14X--14AB--lecteur 14Z passe 5/5 en 0,83 seconde sous GCC Release et 0,64 seconde sous Clang Release; le CTest propre à 14AB prend respectivement 0,08 et 0,05 seconde. Elle couvre aussi le plancher fixe 278, le store étranger, le cap d'un chunk, le retry pré-`HEAD`, l'issue indéterminée, le nettoyage recertifié et les mutations de budget ou payload. Aucun benchmark long, CUDA ou GCP n'est lancé. Ce jalon n'établit ni autorité source commune pair--higher, ni fusion durable à fan-in fixe, ni sidecar diagnostic multi-passe, ni incidences silencieuses, M.1 ou hiérarchie publique. Il ne mesure ni le chemin rapide complet à 50 k, ni un pipeline produit à 10 M+.

Au 27 juillet 2026, la ligne d'incréments de Phase 14 est gelée administrativement après 14AB : aucun 14AC n'est ouvert et tout nouveau développement est enregistré dans la Phase 15 courante. Ce gel n'est pas une fermeture scientifique. La Phase 14 reste `ready`, sa porte de sortie reste fausse et le protocole `warm_e2e` 50 k n'est ni atteint ni réfuté; aucun SLO n'est donc revendiqué.

### Diagnostic transversal 50 k/$K=10$ — censure du chemin résident de référence

Une tentative unique au SHA `d250d71756e4fb8a0f28e1a2d9f1d1b274b2af95` a demandé le pipeline résident complet sous `reference_cpu / hgp_reduced / complete_resident_diagnostic`, sur `uniform_latin` et $n=50\,000$. L'artefact [phase14_complete_resident_50k_k10_g4_d250d71.json](validation/phase14_complete_resident_50k_k10_g4_d250d71.json), de SHA-256 `67c8a45398c73dcfcd02b7e48d5acb6ec1d153775a9c9eae3ed191e79a8a6316`, désactive les caps totaux configurables de la session paire, traite les budgets de support comme des quanta reprenables et porte la limite effective de chunks supérieurs au maximum représentable. Les caps statiques aval restent présents mais ne sont pas exercés. Le coupe-circuit opérationnel censure la tentative à droite dans `sparse_pair_session` après 300 000,014 ms, dont 299 915,926 ms dans la voie paire; supports trois--quatre et aval complet restent à zéro.

La G4 `SPOT` `ehgp-blackwell-spot-ai1a`, génération `2026-07-27T06:15:04.767-07:00` dans `devpod-gpu-exploration / europe-west4-ai1a`, fournissait 48 vCPU, mais ce runner de référence est monothread CPU et le GPU est resté inutilisé. La cible exacte a été arrêtée et certifiée `TERMINATED`, sans autre VM labellisée active. Les 482 212 953 signes logiques, 324 482 913 visites de classification, 55 155 610 paires dirigées prunées et 3 772 050 candidates classées avant la censure localisent le verrou dans le certificat et la classification paire, avant tout support supérieur ou réduction.

Cette mesure impose un pivot de travail, pas une nouvelle hausse de caps ni une répétition du même benchmark. Le prochain changement de performance doit réduire mathématiquement les produits paire ouverts, partager les certificats bloc--bloc et batcher les signes exacts là où ils restent nécessaires, sans construire de catalogue global de paires, facettes, cofaces, incidences, cellules Gamma ou mosaïque de Delaunay d'ordre supérieur. Une nouvelle tentative 50 k n'est justifiée qu'après un changement structurel de ce terme dominant. La présente borne inférieure du temps de complétion, strictement supérieure à 300 secondes, vaut seulement pour le chemin CPU monothread courant; aucun résultat scientifique, hiérarchie, p95 `warm_e2e`, SLO, qualification ou statut public n'est produit.

### Diagnostics surrogate GPU à 50 k et 10 M+

Le prototype `cuda_heuristic_knn / hgp_reduced_surrogate / morton_window_knn` est mesuré au SHA `42ca9c6f8ca91fb730aa9913728ba5676822d6b8`, puis rejoué au SHA `99d3779c8e606e5ec0d89acd744ce8778b70e9e2`, sans ouvrir 14AC. Sur la G4 `SPOT` `devpod-gpu-exploration / europe-west4-ai1a / ehgp-blackwell-spot-ai1a`, génération `2026-07-27T07:02:42.816-07:00`, CUDA 12.9.86, le pilote 580.159.03 et la RTX PRO 6000 Blackwell à 97 887 Mio passent le memcheck 257 points, $W=32$, sans erreur. Le garde GCE vaut 3 600 secondes, l'arrêt invité 45 minutes et la cible finale est certifiée `TERMINATED`, sans autre VM active. [phase14_surrogate_g4_session_99d3779.json](validation/phase14_surrogate_g4_session_99d3779.json), SHA-256 `c8fa9381cf2bfc052296cd7f5de8abe8a470320dca10b87dcfa005b8f430d905`, lie l'environnement, le cycle de vie et les deux artefacts.

À $n=50\,000$, $K=10$, $W=16\,384$ et seed `0x14a750c0ffee`, [phase14_surrogate_50k_k10_g4_99d3779.json](validation/phase14_surrogate_50k_k10_g4_99d3779.json), SHA-256 `39b640c192c4b0754eda0a4efb1fd1a325528f04d76a3c19332466f2cc2cbfff`, donne 337,462300 ms à froid, 57,490338 ms au replay et une reconstruction diagnostique entrée--résultat de 81,326677 ms. Les 48 shards et dix réductions construisent les dix hiérarchies surrogate, mais `diagnostic_estimate_not_protocol` interdit d'en déduire le p95. Face à la référence binary64 exhaustive $W=49\,999$, le rappel du préfixe $K=10$ vaut 98,5808 % et 483 708 voisins sur 500 000 gardent leur rang; pourtant les dix digests diffèrent et seulement deux niveaux racine sur dix coïncident. Le surrogate est donc une proposition de débit, pas une approximation scientifiquement certifiée.

Le SHA `9d148f89f6b358b1c300d3832f18cc5150d1f355` conserve ce périmètre et ajoute une mesure de qualité de la hiérarchie surrogate complète dans [phase14_hierarchy_quality_50k_k10_g4_9d148f8.json](validation/phase14_hierarchy_quality_50k_k10_g4_9d148f8.json), SHA-256 `df4317b421682ecb94097ed5c14b1f1bd3ca266170b8361f52f837e30cdbf6bc`. Les 499 990 morts finies triées coïncident bit à bit dans 1 329 cas, soit 0,2658053 %, avec un écart $L^1$ relatif de 1,2422279 % et un écart $L^\infty$ maximal relatif de 25,6662 %; ces deux écarts comparent des mesures de comptage triées, sans appariement à la diagonale et sans revendiquer une distance de persistance. Sur 65 536 couples canoniques par ordre, soit 655 360 valeurs cophenétiques, 74,5656 % coïncident bit à bit, l'erreur absolue moyenne normalisée par la racine de référence vaut 0,569968 %, la corrélation de Pearson vaut 0,951918 et l'écart maximal atteint 76,2674 %.

Aux neuf quantiles de référence par ordre, les 5 898 240 décisions de co-appartenance atteignent 94,7395 % d'exactitude et 89,5281 % de Jaccard pour la classe « même composante ». Les 304 173 faux splits contre 6 105 faux merges représentent 98,0324 % d'erreurs de sur-segmentation. L'ordre deux est le pire avec 88,6500 % d'exactitude et 77,5646 % de Jaccard; l'ordre dix atteint 96,8550 %, 93,7132 %, 83,3725 % d'égalité cophenétique, 0,563872 % d'erreur moyenne normalisée et 0,904709 de corrélation. Le run froid vaut 316,942 ms; la reconstruction rejouée vaut 81,433 ms mais reste `diagnostic_estimate_not_protocol`. Les 340,881 ms de qualité CPU et les 68,375 ms du kernel de référence sont hors chemin produit. La référence exhaustive ne concerne que les voisins binary64 du même modèle `rank-k + chaîne Morton`; son rappel global reste 98,5808 %, et l'oracle CPU exhaustif borné à 4 096 requêtes mesure 98,366699 %. Rien ici ne compare au Morse HGP exact.

À $n=10\,000\,001$, $K=10$ et $W=256$, [phase14_surrogate_10000001_k10_g4_42ca9c6.json](validation/phase14_surrogate_10000001_k10_g4_42ca9c6.json), SHA-256 `803b02b5754ad3f20fd510657b0501b1830aa796d9289286a4fafa6c46ab760f`, termine à froid en 28,837100723 secondes, dont 15,885791079 secondes de génération, 5,628448021 secondes de LBVH, 115,700988 ms de kernel et 4,720249279 secondes de réduction. Il garde 1 920 000 192 octets device et produit 100 000 000 merges sur dix ordres connexes avec 48 shards et dix réductions. Sans référence, ce passage démontre seulement qu'une représentation H0 surrogate sans matrice globale tient à cette échelle; `heuristic`, `not_certified`, `architecture_only` et `not_claimed` restent normatifs.

Le commit `7faecca0107c990ac2a0b0bfed1a15f22dc1d153` livre la première fondation exacte de ce pivot sous `reference_cpu / hgp_reduced / exact_block_rank_prune_receipt`. Pour un seul produit local $A\times B$, le reçu process-local authentifie deux plages LBVH supports disjointes et une antichaîne canonique de plages témoins, disjointe des supports. À $K=10$, `maximum_closed_rank=11` exige dix feuilles distinctes; chaque nœud retenu satisfait exactement $\max\phi<0$. La proposition est bornée à 64 nœuds, un reçu en retient au plus dix et tout cap, chevauchement, masse insuffisante ou signe nul--positif échoue ouvert. Les masses $\lvert A\rvert\lvert B\rvert$ et $2\lvert A\rvert\lvert B\rvert$ sont locales au bloc et ne deviennent sommables qu'après preuve de la partition du schedule. Le composant ne construit ni paire individuelle, ni facette, coface, incidence, Gamma, cellule ou mosaïque de Delaunay d'ordre supérieur, et ne revendique aucune hiérarchie complète.

Le gate bloc--bloc envisagé est désormais différé au profit du parcours complet AABB ci-dessous. Son contrat reste valide : $W=4\,096$, environ 10,28 ms de kernel et 95,42 % de rappel sur l'échantillon court ne formaient qu'une proposition, et seul un reçu `recertified` après preuve de partition globale aurait pu autoriser un prune. Aucun résultat de ce composant ne change la Phase 14 `ready`, la Phase 15 `in_progress`, leurs portes, `architecture_only` ou `public_status=not_claimed`.

### Pivot retenu : top-$K$ binary64 par LBVH AABB stackless

La proposition à six ordres Morton est abandonnée : les permutations d'axes conservent les mêmes cellules octree et ne corrigent pas structurellement les directions manquantes. Les commits `e64bf83` et `19430a7` adoptent `cuda_binary64_lbvh_top_k / hgp_reduced_surrogate / stackless_aabb_branch_and_bound`. Une fenêtre Morton courte initialise seulement l'incumbent; le postordre certifié visite ensuite tout sous-arbre non prouvé inutile. La borne AABB est calculée par arrondis dirigés vers le bas, le prune exige une inégalité stricte, et égalité, invalidité ou non-finitude descendent. Le saut de $2m-1$ nœuds pour un sous-arbre de $m$ feuilles n'est effectué qu'après cette décision. Le parcours n'a aucun cap de nœuds ou de candidats et départage les voisins par `(distance, PointId)`.

Le différentiel G4 exhaustif $n=50\,000$, $K=10$, $W=256$ contre $W=49\,999$ donne 500 000 voisins sur 500 000 au même rang, dix digests sur dix, 499 990 morts sur 499 990, 655 360 niveaux cophenétiques sur 655 360 et 5 898 240 décisions de coappartenance sur 5 898 240. [phase14_binary64_lbvh_50k_k10_g4_19430a7.json](validation/phase14_binary64_lbvh_50k_k10_g4_19430a7.json), SHA-256 `b40786305b6b91db2747fbf22db8ad272324ca043c54e36cfee668f79d830834`, mesure 2,376064 ms de kernel, 29,691378 ms pour un rejeu avec index résident et 294,391209 ms à froid. Le réglage $W=32$ conserve les dix sorties hiérarchiques et ramène le kernel à 1,578431 ms et le rejeu à 28,353303 ms; il est consigné dans [phase14_binary64_lbvh_50k_k10_w32_tuning_g4_19430a7.json](validation/phase14_binary64_lbvh_50k_k10_w32_tuning_g4_19430a7.json), SHA-256 `23f958434fa069748bcea0b0b2c393953af97be36b352743847603d586069bac`.

À $n=10\,000\,001$, $K=10$ et $W=32$, [phase14_binary64_lbvh_10000001_k10_g4_19430a7.json](validation/phase14_binary64_lbvh_10000001_k10_g4_19430a7.json), SHA-256 `06e75e3f82210ea2473757c953f5220d87fd76574882f4cd9cbc0f8687573cf2`, produit 100 000 010 voisins et 100 000 000 merges sans échec en 30,433016536 s à froid; le kernel vaut 162,060348 ms, le rejeu 6,786548803 s et la capacité device 3 920 000 312 octets. Le parcours p99 visite 229 nœuds et le maximum 399, sans requête plein arbre. Les 48 vCPU servent les shards et les dix réductions indépendantes, tandis que le GPU porte le top-$K$ et le LBVH.

Ces résultats ferment le défaut de rappel observé dans le seul modèle binary64 `rank-k + chaîne Morton` et démontrent la capacité surrogate au-dessus de 10 M; ils ne ferment ni la recette arithmétique comme théorème formel, ni la fidélité Morse, ni le p95 `warm_e2e`. Le froid 50 k reste dominé par 74,247135 ms de génération synthétique et 184,281508 ms de construction/import LBVH. Le prochain gate de latence doit donc garder un processus et un contexte CUDA chauds, mais fournir un nouveau nuage et rebâtir son LBVH à chaque répétition. Le serveur publiera p50/p95 de l'entrée canonique jusqu'aux dix hiérarchies, séparément du cas index déjà résident; réutiliser le LBVH précédent ne peut pas qualifier ce gate.

La mémoire device vaut $192n-80$ octets persistants et, pour $K=10$, $200n$ octets de sorties et audits. Aucune matrice de distances, paire globale, facette, coface, incidence, Gamma ou mosaïque de Delaunay d'ordre supérieur n'est construite. Le pire cas de parcours demeure $O(n^2)$; seules les visites observées sont faibles. Le résumé [phase14_binary64_lbvh_g4_session_19430a7.json](validation/phase14_binary64_lbvh_g4_session_19430a7.json), SHA-256 `9a7a6f97733a671e6bc7995e04729c29757dcfe56bec7a7d27619be8577dd21c`, certifie la G4 `SPOT`, les deux coupe-circuits et l'arrêt final `TERMINATED`. Phase 14 reste `ready`, Phase 15 `in_progress`, `deployment_status=architecture_only` et `public_status=not_claimed`.

### Diagnostic transversal Geogram/CUDA — oracle hors ligne et pivot produit

#### Verdict historique des lanes $k=1$ et $k=2$ sur le LBVH commun

Le diagnostic le plus récent au SHA `509a9c6f0e41fb4ae37975b2e8f10f87bc0101f6` mesure directement le rang des arêtes incidentes à chaque triangle `gabriel_binary64` accepté par PDEL. Il effectue un parcours LBVH complet par source, jamais par arc; la fenêtre Morton initialise seulement les incumbents. Le résultat conserve deux octets par arc dirigé du CSR PDEL, aucune table $n\times M$ et aucun triangle supplémentaire. Les rangs sont exacts pour la clé binary64 déclarée jusqu'à $M$; `M+1` signifie seulement un rang strictement supérieur.

Sur l'unique famille issue de la graine canonique, les 417 839, 8 665 509, 87 631 258 et 263 693 761 triangles acceptés à 50 000, 1 000 001, 10 000 001 et 30 000 001 points possèdent tous une racine témoin PDEL mesurable, y compris 66 367, 1 388 480, 14 089 614 et 42 444 874 triangles évalués avec seulement deux paires PDEL. Les maxima \`directed / symmetric union / mutual\` valent respectivement \`77 / 77 / 84\`, \`111 / 107 / 111\`, \`120 / 117 / 132\` et \`142 / 137 / 146\`. La politique $M=\lceil4k\ln n\rceil$ couvre \`symmetric_union_star\` sur les quatre tailles, mais manque 2 triangles dirigés et 4 mutuels à 30 M. La politique $M=\lceil5k\ln n\rceil$ couvre les trois variantes avec marge sur tous les runs observés. Cette valeur règle une graine de proposition sur cette famille seulement; elle n'est ni une autorité, ni une loi universelle.

À 50 000 points, $M=128$ et $M=256$ donnent exactement les mêmes seuils de triangles; le noyau de rang vaut respectivement 28,391 ms et 66,686 ms. À $M=128$, le composant tient donc sous 100 ms, mais le pipeline produit $K=10$ complet n'est pas encore qualifié. À 1 M, 10 M et 30 M avec $M=256$, le noyau vaut 1,361 s, 15,948 s et 52,637 s; Geogram et le reste du sidecar portent les temps froids à 14,796 s, 156,870 s et 481,542 s. PDEL reste donc un oracle massif hors ligne, jamais une dépendance produit. Le rapport historique, les contrôles différentiels, le memcheck et les artefacts sont dans [l'archive Phase 15](archive/abandoned/phase15/PHASE15_GABRIEL_NEIGHBOR_RANK_G4.md).

Cette direction historique n'ajoute pas une nouvelle variante. Elle partage l'autorité Morton/LBVH, les buffers bornés et le merge, mais garde des algorithmes et des suites de certification distincts : Borůvka dual-tree ferme l'EMST à $k=1$; `pair`, `higher` puis `extra_shell` ferment le catalogue bas ordre à $k=2$. Le préfixe $M=\lceil5k\ln n\rceil$ propose seulement des incumbents ou des témoins; seules les décisions exactes et `frontier_empty=true` autorisent le commit. Aucun callback par produit, transfert massif de terminaux, Delaunay, table $n\times M$ ou catalogue global de triangles n'est admis. Le gate de ces lanes reste un différentiel exact sur petits nuages, puis un falsificateur 12 500 avant 50 000/$K=10$; il demeure distinct du nouveau gate prioritaire du catalogue de paires et le SLO reste strictement ouvert tant que le vrai `warm_e2e` ne passe pas sous 100 ms.

#### Contexte minimal — erreurs du surrogate et oracle PDEL

Au SHA `c56f8022f3ee7e27a296dcb04337047c2e548fab`, Geogram PDEL scelle les deux fixtures et confirme les retards du surrogate brut. À $k=1$, $(2,3,7)$ est une source Gabriel binary64 de niveau $149/4$, ses trois arêtes appartiennent au 1-squelette PDEL et sa première connexion brute arrive au niveau adapté $225/4$. À $k=2$, le même verdict vaut pour $(0,1,3)$, de $345/4$ à $457/4$. Ces deux candidats deviennent donc des erreurs permanentes confirmées. L'overlay glouton canonique les reconnecte sur leur plateau source et les checkers avec records rejouent indépendamment les sources, l'arbre brut, l'overlay et l'arbre corrigé.

Le critère `gabriel_fusion_deadline_v1` reste unilatéral : pour chaque triangle `gabriel_binary64` accepté de niveau $a$, il demande seulement que ses sommets soient déjà dans une même composante à la coupe fermée $a$. `connected_before` et `connected_at` passent; `late` et `never` échouent; `unsupported` échoue fermé. Il ne compare ni catalogue de triangles, ni arêtes, ni triangulation Delaunay; une fusion plus précoce est un succès. Les cinq variantes directes historiques ne possèdent qu'un certificat d'inclusion de l'événement source au niveau source, sans rejeu de leur premier niveau, et restent gelées hors produit.

Le voisinage du surrogate massif ne provient pas d'une fenêtre Morton tronquée. `Binary64LbvhTopKContext` emploie $W=32$ uniquement pour initialiser les incumbents, puis parcourt le LBVH complet en postordre stackless; la borne AABB binary64 dirigée vers le bas n'élague que sur inégalité stricte. Les quatre campagnes publient `complete_query_coverage=true`, `no_candidate_truncation=true`, une permutation Morton validée et zéro requête échouée jusqu'à 30 000 001 points. À 50 000/$K=10$, la requête top-$K$ vaut 7,011725 ms, dont 1,885504 ms de kernel; à 30 000 001/$K=2$, elle vaut 1,457085570 s, avec 1,224517932 s de launcher et 264,118347 ms de kernel. Cette complétude vaut pour la recette binary64 du sidecar et ne lui confère aucune exactitude Morse.

| Nuage | Sources acceptées / ambiguës | `late` brut par ordre | Postcondition overlay |
|---|---:|---|---|
| fixtures PDEL scellées | 12 / 4 et 19 / 0 | erreur cible $k=1$: 1; erreur cible $k=2$: 1 | erreurs cibles `connected_at`; aucun `late` ou `never` accepté |
| 50 000, $K=10$ | 417 839 / 0 | 122 188; 39 441; 30 574; 44 198; 66 251; 89 868; 115 032; 139 540; 163 310; 186 231 | zéro `late`, zéro `never` aux dix ordres |
| 1 000 001, $K=2$ | 8 665 509 / 1 | 2 281 319; 732 699 | zéro `late`, zéro `never` accepté; 1 `unsupported` |
| 10 000 001, $K=2$ | 87 631 258 / 12 | 23 325 610; 7 500 654 | zéro `late`, zéro `never` accepté; 12 `unsupported` |
| 30 000 001, $K=2$ | 263 693 761 / 84 | 74 289 378; 22 560 254 | zéro `late`, zéro `never` accepté; 84 `unsupported` |

Le run 30 000 001 ferme le gate de capacité du sidecar en 466,205 s à froid, avec un pic RSS de 32 517 076 Kio. Aux tailles massives, les arbres brut et corrigé sont construits et vérifiés transitoirement puis engagés par comptes et SHA-256; leurs records restent absents, donc le checker valide le schéma et les engagements mais ne prétend pas les rejouer indépendamment. Les sorties brutes, contrôles et mesures sont `docs/validation/phase15_gabriel_fusion_*.json`, `docs/validation/phase15_gabriel_fusion_*_check.json` et `docs/validation/phase15_gabriel_fusion_*_time.txt`.

Cette validation native complète demeure un oracle conditionnel binary64 hors ligne. PDEL, les wedges et l'overlay ne sont ni une dépendance, ni un fallback, ni une voie de correction du produit. Le sidecar conserve une arène globale de propositions Gabriel pour son tri, ne recertifie ni la topologie SoS, ni la vacuité Gabriel exacte, ni Gamma$_2$, ni la hiérarchie Morse, et garde `public_status=not_claimed`. Les falsifications historiques des restrictions Delaunay restent résumées dans [PHASE15_DELAUNAY_GAMMA2_FALSIFICATION.md](validation/PHASE15_DELAUNAY_GAMMA2_FALSIFICATION.md).

#### Chemin produit unique exact sans Delaunay

Les étages de rang partagent l'infrastructure LBVH, le merge de plateaux et le reducer sparse de `exact_sparse_frontier`; aucune sélection dynamique entre anciennes variantes n'est autorisée. EMST, Borůvka et Delaunay restent hors du binaire industriel et de son chrono; ils peuvent seulement fournir des comparaisons hors ligne.

Pour $k=1$, la route produit sélectionne les événements de paire de rang fermé deux, les ordonne par niveau exact puis les réduit directement en forêt de fusion. Le garde-fou compare hors ligne ses coupes strictes et fermées, ses multifusions et ses temps de fusion à un oracle EMST indépendant, sans appeler ni matérialiser cet EMST dans la route testée. Aucun graphe de Delaunay ordinaire ou d'ordre supérieur et aucun catalogue Gabriel ne sont construits.

Le SHA `c4631d76706f9b4dd150ec059c4a23bb0f7f807a` ajoute une ancre de conception $k=1$ distincte du chemin massif : `ExactYao48Emst` choisit exactement, pour chaque point, l'arête minimale dans chacun des 48 cônes obtenus par huit signes et six ordres des valeurs absolues des coordonnées. Le diamètre angulaire strictement inférieur à $60$ degrés et le départage canonique impliquent que ces arêtes contiennent l'EMST canonique du graphe complet. L'implémentation actuelle balaie toutefois toutes les paires et refuse $n>4\,096$ : elle reste un oracle borné hors ligne. Elle n'est ni un accélérateur à porter dans le produit, ni un second backend produit, ni un catalogue Gabriel, ni un squelette $k=2$.

`ExactLbvhYao48Emst` réalise maintenant ce remplacement du balayage par une recherche CPU exacte sur le LBVH commun. Une seule traversée stackless est exécutée par source; $W$ ne fait qu'amorcer les incumbents. Un masque exact de fermeture des 48 cônes filtre chaque AABB, le prune exige une borne strictement supérieure à tous les incumbents encore concernés et l'égalité descend. La feuille conserve le partage semi-ouvert canonique, puis au plus $48n$ candidats dirigés alimentent la déduplication et Kruskal. Les sorties $W=1$, $W=32$ et amorce exhaustive coïncident avec l'oracle Yao quadratique et avec le Borůvka LBVH exact sur le domaine borné; Release et AddressSanitizer passent.

Ce prototype ferme la sémantique de recherche, pas le gate de débit. Sur `uniform_latin`, $n=1\,000$ et $W=32$, la recherche prend 31,129001876 s pour 443 836 visites, 175 819 prunes stricts, 22 141 candidats dirigés, 14 272 arêtes uniques et 2 730 408 octets de records fixes. Le run 50 000 est arrêté proprement après 60 s sans résultat. La voie CPU est donc rejetée pour le produit et le SLO; le contrat est conservé uniquement comme oracle de comparaison hors ligne. Il reste strictement $k=1$ et ne fournit ni triangle Gabriel, ni squelette $k=2$, ni Delaunay.

La priorité scientifique suivante est désormais le catalogue exact multi-ordre des paires dont la boule diamétrale fermée contient au plus $K_{\max}+1$ points, avec la liste complète de ces points. La convention `requested_order=K` cible le rang fermé au plus $K+1$ et emploie $K$ témoins pour prouver l'exclusion; une demande « au plus $K_{\mathrm{total}}$ points au total » emploie $K_{\mathrm{total}}-1$ témoins. Ce jalon reste en Phase 15 sous `reference_cpu / hgp_reduced / budgeted / architecture_only / not_claimed`; il n'ouvre ni phase ni statut public. Son contrat, ses preuves et ses gates sont centralisés dans [CATALOGUE_PAIRES_DIAMETRALES_EXACT.md](math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md).

La nouvelle coupe Yao48 est adaptative et exhaustive comme filtre unilatéral. Pour chaque ancre et chaque chambre semi-ouverte, elle retient $K$ témoins distincts quelconques et une borne certifiée $D$ de leurs distances carrées. Dans les coordonnées canoniques $x\geq y\geq z\geq0$, une cible est rejetée seulement si $x^2\geq D$, $(x+y)^2\geq2D$ et $(x+y+z)^2\geq3D$ : ces trois inégalités prouvent que les $K$ témoins, distincts des deux supports, appartiennent à la boule diamétrale fermée. Une chambre de moins de $K$ points ne rejette rien. Les vrais plus proches resserrent $D$ mais ne sont pas requis pour la correction. L'échec du cutoff conserve seulement un survivant; il ne caractérise pas son rang.

Deux oracles bornés rendent maintenant ce contrat exécutable. `ExactYao48RankedPairCandidateResult` recalcule toutes les paires pour vérifier que chaque paire de rang au plus $K+1$ survit à la coupe. `ExactRankedDiametralPairCatalogReferenceResult` envoie chaque survivant au classifieur LBVH exact, ferme la trichotomie `below/exact/above` et émet uniquement les records de rang $K+1$ avec intérieur et shell complets. Ils sont plafonnés à 512 points et restent des oracles de falsification, jamais un chemin quadratique rebaptisé produit.

Le port GPU partage le LBVH Morton résident. Sa première couverture tuilée réelle traite des ancres en mémoire $O(B\mathbin{\cdot}48\mathbin{\cdot}K_{\max})$ : le même parcours applique l'ownership Morton, remplit les banques Yao48, certifie des régions prunées avec leur masse et conserve seulement les feuilles survivantes sur device. Le rejeu G4 run4 qualifie ce composant borné pour chaque seuil de rang fermé 2 à 11 sur 128 points, avec patch source, digest du binaire, sorties brutes et memcheck/racecheck conservés. Le cap de 2 048 visites est un quantum reprenable dans le même processus, sous un checkpoint device intra-appel. Le classifieur exact résident draine maintenant les survivants des rangs 2--3, construit par `count + scan` les listes intérieur/shell, trie le catalogue et conserve l'autorité du nuage; le wrapper terminal exige `candidate + certified_pruned + unresolved = n(n-1)/2`, `unresolved=0`, zéro fallback et une frontière vide. Le [reçu G4 `51102a0`](validation/phase15_ranked_pair_classifier_g4_51102a0.json) qualifie nativement ce seul composant support deux contre le différentiel indépendant toutes-paires/tous-témoins, avec invariance des tuiles 1 et 17, memcheck, racecheck et cycle de vie terminal ciblé; il ne qualifie aucun support trois, Gamma2, $k=2$ ou hiérarchie.

Le compositeur `MortonYao48TiledPairFrontierContext` fixe désormais l'ordonnancement de référence sous `reference_cpu / hgp_reduced / linear_budgeted_morton_yao48_anchor_tiles`. Une tuile se termine seulement entre deux ancres et les quatre caps globaux restent des multiples immuables de $n$; leur épuisement censure définitivement le run avec résidu explicite. Les reprises tuilée et monolithique donnent le même transcript sur le domaine borné, mais ce jalon ferme seulement une spécification CPU de couverture. Il n'exécute ni CUDA, ni rang diamétral exact, ni compactage scientifique et ne qualifie aucun catalogue.

Le protocole massif enregistré exige 30 runs frais à 50 000 points, puis 9 à 1 M, 6 à 10 M et 3 à 30 M, dans cet ordre strict. Il refuse tout binaire de composant ou proxy et toute présence d'EMST, Borůvka, Delaunay, matrice globale ou fallback dense dans le chemin chronométré. L'EMST à $k=1$ et la connexion, dans la source $\Gamma_2$, des trois facettes-paires de tout triplet $T$ ayant au moins deux arêtes de la triangulation de Delaunay ordinaire, au plus tard au niveau $\beta(T)$, restent deux préflights hors ligne appliqués avant condensation. Les six portes sont reliées à des artefacts canoniques typés et au digest du binaire; les échéances GCE et invité sont revérifiées avant chacun des 48 runs sur la totalité du budget restant. La couverture CUDA résidente et le consommateur exact support deux des rangs 2--3 ont franchi leurs gates natifs bornés, mais la persistance universelle des niveaux, les supports trois et quatre, les incidences, la couverture, la verticalité et la réduction complète restent ouverts. Le profil historique run4 à 50 000 points s'est censuré à `node_visit_capacity` après 6 263 ancres; run5, hors campagne produit, s'est censuré à `candidate_capacity` sur 10 M et 30 M. Ces profils restent `component_only / profile_only` et ne permettent pas de sauter le gate produit 50 000. La campagne 1 M, 10 M et 30 M demeure donc bloquée jusqu'au vrai pipeline et à sa vue aval `min_cluster_size=20`; les mesures de composant ou du proxy de graine n'ouvrent aucune porte.

Le premier port CUDA conservé est volontairement plus étroit : `MortonYao48RadialSubtreeFilterContext` reçoit un nœud du LBVH certifié et 48 banques déjà pleines, propose avec arrondis dirigés la coupe radiale `minDist2_down >= max(3*D_up)`, puis rejoue exactement sur CPU chaque proposition avant de publier `certified_prune`. Il ne parcourt aucune feuille et n'émet aucune paire. Le prototype antérieur qui visitait les feuilles de chaque tuile a été supprimé, car son coût restait $\Theta(n^2)$ même sur les entrées favorables. Ce composant demeure `architecture_only / not_claimed` : son reçu porte seulement la largeur locale authentifiée, non une masse additive.

La qualification gardée du SHA `8e23ff995d355649952e76d21bc4f1e425697034` ferme la porte d'exécution native de ce composant sur G4 : builds CUDA release et audit `sm_120`, marqueur du binaire radial exactement unique et arrêt ciblé `TERMINATED`. Le sanitizer de l'environnement passe, mais il n'est pas dédié au binaire radial; cette qualification ne ferme donc ni le compositeur tuilé, ni la partition globale, ni le gate de croissance. Le résumé durable est [phase15_morton_yao48_radial_subtree_filter_g4_8e23ff9.json](validation/phase15_morton_yao48_radial_subtree_filter_g4_8e23ff9.json).

`MortonYao48DeviceTiledPairFrontierContext` réalise maintenant ce premier compositeur avec une warp par ancre et un ownership sur le préfixe Morton strict. Une coupe exige $R-1$ témoins distincts retenus hors du sous-arbre et une borne dirigée non négative de $(w-a)\mathbin{\cdot}(x-w)$ sur toute sa boîte; tout cône ambigu descend jusqu'à une candidate non banquée. Les candidates, reçus, banques et checkpoints restent sur device, tandis que 168 octets de contrôle par ancre, plus un `uint64` de huit octets et une synchronisation par subdivision, sont rapatriés. Les saturations des segments candidat et prune publient des chunks reprenables après engagement du sous-arbre courant. La capability détachée conserve les vues source certifiées pour le classifieur suivant et une contre-pression impose une seule arène en vol : à $B=4096$ et $R=11$, son plafond v3 vaut exactement 1 263 829 000 octets, et l'ancienne arène est libérée avant la suivante. La mémoire device totale reste toutefois $O(n)$ pour les coordonnées, l'ordre Morton et le LBVH, plus cette arène transitoire $O(B\mathbin{\cdot}R)$. Le checkpoint reste process-local et ne rend pas le calcul reprenable après redémarrage.

Ce jalon ferme l'architecture de couverture, son contrat host/fake et sa qualification native bornée run4; il ne qualifie pas à lui seul le catalogue scientifique. Run4 G4 conserve la base et son patch source reproductible, le digest du binaire et les sorties brutes. Après succès sur 128 points et Compute Sanitizer, son profil 50 000/$R=11$ exécute deux tuiles puis censure à `node_visit_capacity` après 6 263 ancres : 675 200 paires candidates et 18 940 516 paires prunées sont engagées, 1 230 359 284 restent non résolues. Run5 exécute ensuite deux profils directs `affine_uniform_binary64` : 10 M termine 4 897 ancres avant `candidate_capacity`, avec 493 663 candidates, 11 499 090 paires prunées et 49 999 983 007 247 non résolues; 30 M termine 7 308 ancres, avec 807 304 candidates, 25 899 782 paires prunées et 449 999 958 292 914 non résolues. Tous deux ferment la masse mais gardent `coverage_complete=false`, `component_only=true`, `profile_only=true`, `process_restart_resumable=false` et tous les claims scientifiques ou de scalabilité à faux. Leur SHA Git n'est pas disponible localement et aucun patch source, build log, résumé d'environnement, manifest final du harnais ou reçu GCP run5 n'est archivé : ces sorties brutes ne sont pas une qualification. Le schéma v3 remplace désormais ces deux saturations par des chunks device reprenables; le classifieur exact multi-ordre, `count + scan`, le payload fermé et la lease terminale ont depuis franchi leur gate natif séparé sur 257 points. Une censure ne déclenche ni scan dense, ni Delaunay, ni EMST/Borůvka. La persistance des niveaux larges, la frontière support trois, les incidences, le différentiel borné du chemin complet, le falsificateur de croissance et un résultat produit complet à 50 000 précèdent la campagne massive séquentielle. Aucune complexité moyenne ou sous-quadratique générale n'est revendiquée.

Pour $K_{\max}$, ce port n'exécute qu'une passe : les banques de $K_{\max}$ témoins conservent tous les rangs jusqu'à $K_{\max}+1$, puis le classifieur retourne le rang exact et route le record vers $k=R-1$. LBVH, banques, rapport de masse, classification filtrée, scans, payloads, tri et déduplication restent sur GPU. Une file compacte hôte n'accepte que les dépassements rares des expansions ou entiers GPU de taille fixe; aucun callback par paire ou transfert D2H de candidates ou de payload par vague n'est autorisé. Le contrôle fixe par ancre du compositeur actuel reste explicitement permis et mesuré.

Chaque record de paire ferme exactement la branche de support minimal deux : si $\left\lbrace u,v\right\rbrace\subseteq Q\subseteq C(u,v)$, alors $Q$ a la même miniboule, le même niveau et le même saturé. Il donne donc $K-1$ triplets candidats et $\binom{K-1}{2}$ quadruplets candidats portés par cette paire sans nouvelle requête globale; un filtre exact d'indépendance affine élimine les candidats géométriquement dégénérés. Pour les buckets effectivement émis, les triangles non dégénérés obtus ou rectangles sont exhaustifs dans cette branche; les cosphères et rangs hors fenêtre conservent leurs diagnostics propres.

Cette fermeture ne couvre pas les triangles aigus. La fixture `hartigan_triangle_all_side_ranks_above_k.json` recertifie toutes les paires de rang au plus trois et montre que même l'union de toutes leurs sous-arêtes omet les trois côtés du triangle critique. Après le catalogue de paires, la roadmap ouvre donc une frontière exacte indépendante des supports aigus de taille trois; les supports bien centrés de taille quatre viennent seulement ensuite. Les préfixes fixes globaux, Yao48 et Morton restent propositionnels; seules les coupes adaptatives certifiées des sections précédentes portent l'exhaustivité.

La cascade GPU écarte les supports inférieurs avant le rang : trois signes diamétraux strictement positifs sont évalués directement sur chaque triplet proposé indépendamment et sont nécessaires pour qu'un triplet non dégénéré entre dans la frontière de support trois. Ils ne constituent jamais une jointure avec le catalogue des paires; tout triangle droit ou obtus est néanmoins déjà porté par une paire. Après émission des triangles aigus et de leur saturé, leurs points fermés ferment directement les quadruplets de support trois. En parallèle, la frontière indépendante de support quatre partitionne tous les quadruplets affinement indépendants et n'accepte que ceux dont les poids barycentriques du centre circonscrit sont tous strictement positifs; sinon leur miniboule est déjà portée par une paire ou un triangle.

Cette indépendance interdit un filtre par faces déjà acceptées. `tetrahedron_face_filter_counterexamples.json` recertifie un tétraèdre bien centré avec deux faces obtuses et un tétraèdre à quatre faces aiguës dont le centre est extérieur. La frontière support quatre applique donc directement ses quatre signes barycentriques; une clique de triangles aigus n'est ni une condition nécessaire ni une preuve suffisante.

Un générateur saturé n'est pas un cluster discret. Après recertification, ses cofaces alimentent $\Gamma_k$ par leurs facettes de suppression; seules les composantes connexes sur les identités de facettes constituent la hiérarchie Hartigan, avec recouvrements de points autorisés dès $k\geq2$. La validation exige simultanément les niveaux $\beta$ exacts et l'histoire exacte des composantes, jamais une simple partition de points.

L'EMST fournit en outre un diagnostic unilatéral mais limité autour des triplets. Si une boule de rayon $R$ contient trois sommets $a,b,c$, chaque distance entre deux de ces sommets est au plus $2R$. Le chemin EMST entre une paire a un bottleneck au plus égal à la distance de cette paire; chacune de ses arêtes apparaît donc dans Gamma$_1$ sous coupe fermée au niveau $d^2/4\leq R^2$. `point_component_clique_lift_v1` transporte seulement ce témoin de connexion de points vers le sidecar PDEL. Il ne porte pas sur les composantes de facettes de Gamma$_2$ et ne peut décider aucune naissance ou fusion Hartigan à $k=2$ : il ne produit ni triangle, ni incidence silencieuse, ni catalogue exact et n'interdit pas une fusion trop précoce.

Pour $k=2$, le chemin régulier sans extra-shell extrait le catalogue Gabriel exact des autorités déjà séparées : le flux `pair` fournit les événements de support deux, `closed_rank=3`, avec exactement un intérieur strict; le flux `higher` fournit les événements de support trois, `closed_rank=3`, sans intérieur strict. Les triangles droits, les égalités de coque et les cosphères ne sont pas couverts par cette seule projection : leurs diagnostics `extra_shell` doivent être développés depuis une autorité exacte de shell, sans catalogue Delaunay, puis fusionnés au même flux. Tant que cette extension ne ferme pas tous les diagnostics pertinents, le gate produit $k=2$ reste incomplet. La fusion canonique, les niveaux exacts, la fermeture des lots et le certificat terminal `frontier_empty` doivent finalement établir qu'aucun événement admissible, régulier ou dégénéré, ne reste hors flux. Toute ambiguïté, frontière résiduelle, source non recertifiée ou rupture de chaîne échoue fermée.

Le même SHA fournit `ExactLowOrderGabrielSkeleton`, raccord local et proportionnel à la sortie des deux flux existants. Il conserve les arêtes de rang fermé deux avec leur niveau exact, les triangles de rang fermé trois avec leur provenance `pair`, `higher` ou commune, trie et déduplique seulement à niveau identique et rejette toute contradiction de niveau. Ses reçus lient localement les payloads mais ne rejouent pas les autorités sources; un consommateur doit donc recertifier fraîchement les deux flux. Tout diagnostic `extra_shell` fait échouer la projection : ce jalon ferme la couture régulière de rang au plus trois, pas encore la complétude $k=1$ dégénérée ni celle de $k=2$.

La politique prudente observée est $M=\lceil5k\ln n\rceil$ sur une seule famille de graine canonique; elle reste adaptative et strictement propositionnelle. Elle peut améliorer le temps moyen et réduire les frontières, mais ne prouve jamais une borne universelle, une complétude ou une décision scientifique. Une augmentation de $M$ ne peut ni transformer un préfixe en certificat, ni autoriser un résultat lorsque `frontier_empty=false`.

Le dernier gate G4 au SHA `17f7b04` rejette explicitement la répétition de la voie historique `stackless_product_batch` à callbacks et terminaux. Sur le même nuage `uniform_latin` de 12 500 points et les mêmes budgets, le CPU exact prend 18,800618 ms à $K=2$ et 18,527669 ms à $K=10$, tandis que le premier passage GPU prend respectivement 5 001,966230 ms et 3 602,352073 ms; les trois rejeux résidents restent autour de 3 657 ms à $K=2$ et 2 257 ms à $K=10$. Le rang plus faible ne sauve donc pas cette ordonnance : il augmente les callbacks, visites et terminaux malgré deux témoins requis au lieu de dix. Tous les caps, frontières et rejeux exacts ferment, et le memcheck $n=4\,096$, $K=2$ ne trouve aucune erreur; la conclusion porte sur le coût, pas sur la correction du composant.

La règle d'arrêt interdit par conséquent le run 50 k et la variante $Q=31$ sur ce chemin inchangé. Ce résultat ne réfute ni le LBVH commun ni le squelette de rang au plus trois : il interdit de promouvoir en produit le flux historique répété. Le prochain prototype GPU doit aplatir la frontière partagée et émettre directement les événements bas ordre utiles, sans relancer une recherche stackless et sans rapatrier une arène de terminaux par callback. Les artefacts sont `phase15_pair_rank_n12500_k2_q3_g4_17f7b04.json`, `phase15_pair_rank_n12500_k10_q3_g4_17f7b04.json`, `phase15_pair_rank_n4096_k2_memcheck_g4_17f7b04.json` et `phase15_pair_rank_k2_vs_k10_g4_17f7b04.json`.

Une première couture de cette frontière plate a été poussée jusqu'au replay exact de la partition de $\binom{n}{2}$, puis rejetée et entièrement retirée avant commit et benchmark. Elle atteignait environ 1 484 lignes de production avant les tests, déléguait encore au contexte P1 et recopiait donc sur hôte $80(2n-1)$ octets de nœuds, soit environ 4,8 Gio à 30 M, en plus d'une table de feuilles. Elle conservait aussi des références brutes vers l'index et le nuage sans contrat de durée de vie assez fort et n'authentifiait pas toute la lease. Ses éléments sûrs — arène device Phase 14 partagée, split diagonal en trois enfants, split croisé en deux, `anchor_phi_lower_bound`, caps fail-closed, reçus stricts puis rejeu exact — restent des exigences, pas du code produit accepté.

Le falsificateur compact `prune-only` du SHA `c047e2f` a ensuite isolé ce pouvoir de prune sans fallback exact par produit. Seuls les reçus stricts des produits explicitement `pruned` sont rapatriés et rejoués; tout non-prune est subdivisé, et la partition finale ferme exactement $\binom{n}{2}$. Les runs $n=14$, $K=1,2,10$ et $n=257$, $K=1,2,10$ sont conclusifs sans résidu. À $n=3\,125$, $K=2$, il prune 4 863 364 des 4 881 250 paires, soit 99,634 %, mais traite 281 131 produits, visite 110 675 038 nœuds, effectue 47 synchronisations et prend 3 228,989 ms à chaud. Compute Sanitizer à 257/$K=2$ donne zéro erreur et zéro fuite.

Ce résultat rejette la subdivision hôte par vagues avant les profils 6 250, 12 500 et 50 000 : la qualité du prune est excellente, mais l'ordonnance et les traversées répétées sont déjà incompatibles avec le SLO. Le code expérimental est retiré après conservation des artefacts et du [rapport archivé](archive/abandoned/phase15/PHASE15_PRUNE_ONLY_FRONTIER_G4.md). Un nouvel essai n'est admissible que s'il emprunte la même autorité hôte et la même arène device certifiées, conserve les vagues sur GPU et ne produit qu'un transcript final borné; il ne doit ni dupliquer le snapshot, ni réintroduire une API de diagnostic dans le chemin produit.

Cette route ne matérialise ni Delaunay ordinaire, ni triangulation ou mosaïque de Delaunay d'ordre supérieur, ni matrice globale de distances, ni catalogue global de facettes, cofaces, incidences, cellules ou Gamma. Le gate G4 borné de la couverture CUDA tuilée est fermé avec base plus patch source, digest du binaire et sorties brutes conservés; viennent ensuite la reprise canonique du résidu, la trichotomie de rang, `count + scan` et le payload fermé des seuls survivants, puis la cascade supports deux à quatre et sa réduction comparée à Hartigan pour $k=1,\ldots,10$. Le reçu doit fermer les masses candidate, prunée et non résolue sans fallback dense. À $k=1$, le reducer consomme directement le flux exact de paires; l'EMST/Borůvka reste un oracle comparatif hors ligne et ne fournit aucun objet au produit. L'intégration de `pair`, `higher` et `extra_shell` au reducer sparse vient après le catalogue de paires et la frontière indépendante des triangles aigus. Le vrai `warm_e2e` $n=50\,000$, $K=10$ ne sera qualifié sous 100 ms que sur les familles enregistrées et sous caps de sortie; le pire cas quadratique est censuré par `budget_exhausted`. Le sidecar PDEL froid, qui vaut 1,925 s à 50 000 points, reste exclu de ce chrono produit.

### Historique 50 k de `d69539a` — surrogate point-MST rejeté

> [!CAUTION]
> La latence ci-dessous qualifie seulement un surrogate sur les `PointId`. Chaque ordre contient exactement $n-1$ fusions et ne porte ni facettes de $\Gamma_k$, ni cofaces, ni incidences silencieuses, ni `coverage_log`, ni morphismes verticaux. Elle ne satisfait donc pas le contrat MorseHGP3D, même lorsque ses gardes unilatéraux passent.

La première tentative pleinement attestée, au SHA `083c2de`, satisfaisait le seuil de latence mais a été rejetée par le sidecar scientifique : sur `balanced`, $k=1$ comptait 37 échéances EMST tardives et $k=2$ comptait 169 triangles tardifs. Cette contradiction est conservée dans la fixture permanente `tests/fixtures/regressions/phase15_balanced_50k_projection_top1_emst_deadlines.json`. Le SHA `d69539a` remplace l'unique extrême projeté de chaque côté d'une paire de composantes par les huit meilleurs supports projetés de chaque côté, puis retient le minimum canonique des au plus 64 distances croisées; la fixture vérifie que ce `top8_projection_cross_min` retrouve l'arête d'échange EMST manquée par la variante top-1.

La campagne G4 finale contient trente exécutions intégrales à 50 000 points, dix par famille `affine`, `jittered` et `balanced`; aucune n'est un smoke. Le chrono `warm_e2e` va des coordonnées brutes déjà en mémoire hôte aux dix arbres matérialisés et canoniques, canonicalisation incluse et génération synthétique exclue. Les p95 par famille sont respectivement `90.238639 ms`, `88.803800 ms` et `96.045749 ms`. L'agrégat donne un p50 de `88.803800 ms`, un p95 de `95.791070 ms`, puis un p99 et un maximum de `96.045749 ms`; les 30 mesures sur 30 sont strictement inférieures à 100 ms.

Les gardes scientifiques finaux sont exécutés hors chrono sur une Delaunay ordinaire SciPy/Qhull qui réussit directement avec les options par défaut, sans translation et sans `QJ`, pour les trois familles. À $k=1$, chaque arbre reproduit l'identité canonique des 49 999 arêtes de son EMST, y compris `balanced`. À $k=2$, tous les triangles uniques possédant au moins deux arêtes de la Delaunay ordinaire sont connectés strictement avant leur échéance de miniboule : 4 402 503 cas `affine`, 4 517 840 `jittered` et 4 183 217 `balanced`, avec zéro connexion au niveau, zéro retard, zéro source jamais connectée et zéro source non supportée.

La provenance lie les reçus au commit complet `d69539a18adc1e5815bd354f70e773a4a8a1d0f6` et au binaire de 1 883 544 octets dont le SHA-256 est `51bb7d8aa5565ea6c39eebfe77e7a34232e92bcb5c9553807cbaf84b079f726d`. Chaque rapport du runner contient cette auto-attestation; le checker rehache le fichier fourni, la compare à l'attestation et au digest attendu, puis le rehache après le rejeu scientifique. Les exports, les bits des nuages, les niveaux racines et les digests des dix hiérarchies sont également liés et revérifiés. La voie chronométrée ne construit ni Delaunay, ni EMST, ni matrice globale de paires, ni cellule, coface, incidence globale ou mosaïque de Delaunay d'ordre supérieur; Delaunay et EMST restent des oracles hors ligne.

Ce résultat demeure `architecture_only` avec `public_status=not_claimed`. L'enveloppe inférieure empirique de 64 ULP appliquée aux niveaux de core pour $k\geq2$ peut avancer une fusion, et le choix top-8 n'est qu'une observation sur les fixtures enregistrées : aucun des deux n'est une preuve Morse. La recherche des six centroïdes voisins reste en $O(C^2)$, sans cap explicite sur le nombre $C$ de composantes, et la construction échoue si ce graphe reste déconnecté. L'ancienne porte qui proposait de borner cette couture puis de rejouer les mêmes gardes est annulée : même borné, `d69539a` ne produit pas l'univers de facettes de Gamma et ne prouve ni MorseHGP3D, ni M.1, ni une portée publique.

### Durcissement borné du runner v5 — historique rejeté

Le runner v5 avait été enregistré sur hôte comme `implemented_host_pending_full_50k_replay`, sans ouvrir la Phase 16. Cette qualification est maintenant remplacée par `rejected_point_mst_surrogate`. Le mode historique `warm_fresh_cloud_lbvh_top10_capped_component_bridges_parallel_h0` refuse plus de 256 composantes, plus de 1 536 paires, plus de `16*n` projections de membres ou plus de 98 304 distances croisées. Ces comptes exacts sont calculés avant les workers et publiés avec leur budget et leur raison d'arrêt. Le chemin $O(C^2)$ subsiste sous le cap constant; il n'est ni présenté comme subquadratique, ni autorisé à poursuivre au-delà du cap.

Tous les workers `std::jthread` sont maintenant joints avant lecture et insertion des ponts. Les arbres demandés sont matérialisés et digérés, l'export devient un préfixe strict et `--export-max-order 2` suffit aux oracles $k=1$ et $k=2$; les vecteurs de merges sont ensuite libérés tout en conservant comptes, digests et niveaux racines. Le runner continue d'éviter toute Delaunay, EMST/Borůvka, matrice globale de paires, facette, coface, incidence, cellule ou mosaïque de Delaunay d'ordre supérieur. Ces objets restent exclusivement des oracles postérieurs.

Le runner v5 est désormais `rejected_point_mst_surrogate`. Borner sa couture, joindre ses threads et libérer ses dix vecteurs ne change pas son univers scientifique : il garde 50 000 sommets points et exactement 49 999 fusions par ordre, alors que les ordres supérieurs vivent sur les facettes de $\Gamma_k$. Aucun replay G4 de ce binaire n'est une porte de la campagne MorseHGP3D et aucun de ses résultats ne peut déverrouiller `1M / 10M / 30M` ou la Phase 16.

### Rectification de la porte Phase 15 à 50 k

La porte reste dans la Phase 15, sans ouverture ni fermeture administrative. Le contexte annoncé demeure `backend=reference_cpu`, `profile=hgp_reduced`, `mode=budgeted`, `deployment_status=architecture_only` et `public_status=not_claimed` tant que le nouveau chemin n'est pas raccordé et qualifié. La porte de la Phase 16 reste fermée.

Le payload chronométré doit maintenant être la vraie hiérarchie source pour $1\leq k\leq10$ : identités des facettes, cofaces ou leur flux sparse complet certifié, incidences silencieuses, lots exacts, `coverage_log`, forêts horizontales et applications verticales. Ces objets peuvent être streamés et encodés sans arène globale, mais ils ne peuvent être remplacés par dix arbres sur les points. Le nombre de fusions par ordre est une sortie observée, jamais la constante $n-1$.

L'option de sortie enregistrée est `min_cluster_size=20`, `relation=at_least`. Une composante devient visible lorsque l'union des `PointId` de ses facettes contient au moins vingt points distincts, après résolution du lot complet de niveau exact. Les composantes invisibles restent dans la source et continuent à recevoir cofaces, incidences et couverture; la condensation intervient seulement après réduction. Puisque $20>K_{\max}=10$, une composante visible ne peut pas être une facette isolée : les vues visibles `full_pi0` et `hgp_reduced` coïncident sur une même source Gamma exacte. La cible verticale d'une composante visible est visible elle aussi, mais cette propriété ne remplace pas la construction et la certification des morphismes sources.

La transformation aval bornée est maintenant implémentée sous `reference_cpu`. Elle recertifie fraîchement la source $k=1$ par l'EMST exhaustif pour $n\leq64$ et la source `hgp_reduced` par `ExactPersistentReducedGammaOrderHistory` pour $n\leq14$, puis applique `at_least/20` après les lots exacts. Les tests couvrent l'union distincte sous recouvrements, la croissance cachée, la fusion de composantes cachées qui atteint le seuil, les multifusions visibles, l'égalité au seuil, les permutations de lot et un passage réel à vingt points. Ce jalon est seulement un oracle horizontal de condensation : il ne fournit ni source Morse-HGP 50 k, ni verticalité, ni vue `full_pi0`, ni `MorseHGP3DResult`, ni statut public. Le `deployment_status` de la vraie campagne distingue donc désormais le contrat et cette condensation aval implémentés de la source industrielle, toujours manquante.

Les gardes scientifiques sont relancés avant condensation. À $k=1$, la forêt source doit retrouver l'EMST canonique avec ses lots de niveaux. À $k=2$, tout triplet $T$ ayant au moins deux arêtes de la triangulation de Delaunay ordinaire de l'oracle apporte trois facettes-paires distinctes; les trois doivent appartenir à une même composante source de $\Gamma_2$ au plus tard au niveau $\beta(T)$. Tester seulement la connexion de ses trois sommets points est interdit. Ces gardes ne prouvent pas à eux seuls la complétude des cofaces, des incidences ou de la verticalité.

Le protocole reste non-smoke et de bout en bout : deux warmups puis dix nuages frais mesurés pour chacune des trois familles, avec p95 `warm_e2e` strictement inférieur à 100 ms. Le chrono va des coordonnées brutes en mémoire à la source complète certifiée et à sa vue `at_least/20` matérialisée. Le contrat 100 ms n'est actuellement pas démontré. S'il échoue, la roadmap exige de publier les temps, la phase dominante, les cardinalités par ordre et les pics de mémoire, sans revenir au surrogate et sans revendiquer `exact`.

### Optimisations autorisées

- fusion de kernels sans fusionner proposition et certification;
- CUDA Graphs;
- classes d'unités de frontière par complexité;
- double buffering;
- culling directionnel;
- cache de violateurs;
- radix sort sur bits discriminants avec vérification complète;
- chevauchement GPU et fallbacks CPU;
- réduction des copies et sérialisation différée.

### Optimisations interdites

- taille fixe de voisinage sans fermeture;
- epsilon d'égalité;
- limite de faces tronquée;
- suppression d'un fallback;
- résultat conditionnel étiqueté exact;
- exclusion des cas lents du p95 sans règle préenregistrée.

### Protocole

- au moins 30 répétitions après warm-up;
- graines et configurations gelées;
- p50, p95, p99;
- temps `cold_e2e`, `warm_e2e` et `resident_core` séparés;
- H2D/D2H séparés;
- compteurs complets;
- comparaison bit à bit avec la version non optimisée.

### Porte de sortie

Objectif principal : $n=50\,000$, $K_{\max}=10$, p95 `warm_e2e` strictement inférieur à 100 ms sur famille volumique préenregistrée, portée entièrement certifiée et pic inférieur à 80 % de VRAM. L'objectif secondaire est un p95 strictement inférieur à une seconde sur exactement le même protocole et le même payload; l'atteindre ne ferme pas la porte principale. Cette mesure inclut validation, transfert, LBVH, calcul et matérialisation du résultat; `resident_core` reste diagnostique. Elle ne peut porter le statut public `exact` qu'après la migration contractuelle versionnée prévue à la sortie des Phases 9--11 et de M.1; un benchmark ne réalise jamais cette promotion. Si la cible échoue, publier la phase dominante et la courbe de croissance; ne pas modifier le contrat.

## Phase 15 — Streaming à dix millions et davantage

### But

Traiter un flux d'événements, d'attaches et de frontières plus grand que les arènes résidentes sans reconstruire la mosaïque et sans changer le résultat dans la portée certifiée.

### Tranche 15A — budget et store atomique implémentés

15A est implémenté sous `backend=reference_cpu`, `profile=hgp_reduced`, `mode=budgeted`, `deployment_status=architecture_only` et `public_status=not_claimed`. Son unité durable est un chunk 14A complet : tous ses lots ordre--niveau et leurs deltas compacts sont recertifiés et publiés ensemble, ou aucun ne l'est. Les chunks de callback 14L restent des découpages de transport éphémères et ne sont jamais des checkpoints.

Le snapshot interne conserve séparément les budgets device, hôte, scratch et sortie en octets, puis le budget temps en nanosecondes monotones, chacun avec limite, consommation, réserve et reliquat. La réserve temporelle exacte n'a pas de projection fidèle dans le `BudgetSnapshot` public v2, qui exprime le temps en secondes et ne possède aucun champ réservé; 15A interdit donc arrondi, omission de la réserve et sérialisation publique v2 avant migration explicite.

`ExactDirectMorseBudgetTracker` ferme l'arithmétique entière des cinq axes pour les demandes qui lui sont fournies et `AtomicLinearRunStore` implémente le protocole Unix local. Un callback de recertification sans voie par défaut reconstruit 14A et rejoue l'unité complète avant toute écriture. Le store garde l'ancien état, recertifie l'image canonique, écrit et synchronise un temporaire, relit ses octets, crée le final immuable par hard-link avec contrôles d'inode, synchronise le répertoire, puis publie et synchronise le manifeste par renommage. Un fichier final non référencé reste non committé. Cette tranche ne sérialise ni ticket 14H, ni locator, ni DSU, ni forêt et ne revendique ni reprise scientifique en place, ni 10 M+, ni SLO, ni statut public. Le contrat détaillé est consigné dans la [note de progression](validation/PHASE15_PROGRESS.md).

Les deux targets compilent en GCC Release strict et leurs CTests ciblés passent 2/2 en 0,02 seconde. Cette validation est uniquement logicielle et locale; elle ne mesure aucun volume produit.

### Tranche 15B — vrais chunks 14A recertifiés et reprenables

15B raccorde le store à `ExactDirectMorseChunkRunContext` sous `reference_cpu / hgp_reduced / budgeted / architecture_only`. Le contexte reconstruit et compare 14C une seule fois, indexe en $O(B)$ un curseur immuable par lot, puis rejoue directement tout lot demandé par le même cœur 14D. Il ne repart jamais du lot zéro, n'avance aucun curseur vivant et ne sérialise aucun ticket 14H. Un chunk durable contient exactement toutes les entrées de son intervalle 14A, y compris les lots vides, avec les compteurs 14A et les projections scientifiques compactes 14D.

Chaque lot engage le stamp du locator strictement pré-lot. Une vue non possédante sur un résolveur externe peut reconstruire ce préfixe à la demande et le libérer après l'appel; le delta est accepté seulement si son stamp égale le stamp attendu et reste stable pendant le rejeu. Cette frontière préserve l'atomicité des niveaux égaux et empêche le cœur de garder un locator historique par lot. La résidence et les caches du résolveur externe restent explicitement hors audit.

Le contexte possède un tracker de budget séquentiel dont la politique effective entre dans le digest. Une occupation sessionnelle device, hôte, scratch et sortie reste au contraire locale au redémarrage et charge les quatre axes sans modifier le contrat durable. Les enveloppes de ressources par lot sont maximisées pour les octets et sommées pour le temps; un gate frais précède tout résolveur ou rejeu, puis un gate final vérifie le temps monotone réellement écoulé. Pour le store, ce gate obligatoire et stateful est distinct du recertificateur dont la décision est pure et scientifiquement idempotente sous autorités immuables; les compteurs d'observation et caches non autoritatifs peuvent évoluer. Les factories 15B lient les deux callbacks du même contexte et préflightent le `HEAD` initial avant toute mutation du répertoire. Le gate reste actif pendant la recertification et les écritures réversibles, puis se ferme juste avant le remplacement de `HEAD`; un refus opérationnel ne peut ni accepter un payload, ni être confondu avec une preuve. Publication et reprise réévaluent leurs ressources avec la session courante : le snapshot persistant reste une provenance canonique et n'est jamais une autorité de récupération. Le cap wire est préflighté cumulativement avant la rétention de chaque segment scientifique.

Le CTest court sur un vrai tétraèdre à $K=1$ publie le lot vide du premier chunk, abandonne le second avant `HEAD`, détruit le contexte, refuse d'abord une reprise dont l'occupation hôte sessionnelle laisse un octet, rouvre ensuite le préfixe avec une reconstruction 14C unique, puis publie quatre clés résolues et douze joins. Un `HEAD` initial trop grand est refusé avant création du verrou ou d'un fichier; une erreur typée distingue le refus de ressource à la reprise. Un chunk à deux lots accepte son cap wire exact et refuse ce cap moins un octet avant de retenir le second segment. Il passe 1/1 en 0,03 seconde sous GCC 13 Release strict; mutation, octet terminal et stamp de locator substitué sont refusés sans avance. Le slack d'allocateur, les propositions basses retenues par un appelant, les copies du store et la synchronisation finale post-`HEAD` restent hors de la borne applicative. La tranche ne réduit pas encore les deltas vers locator, quotient et forêt, ne prouve pas l'identité résident--streaming et ne qualifie ni un million, ni 10 M+, ni le SLO 50 k.

### Tranche 15C — fold incrémental et reconstruction hiérarchique

15C ajoute `ExactDirectMorseForestReducer` sous `reference_cpu / hgp_reduced / budgeted / architecture_only`. Le store remet au visitor de reprise une projection typée construite directement depuis le résultat 14D frais déjà comparé au wire. Le pointeur est emprunté, le visitor s'exécute avant le gate final et les tableaux de clés et joins sont transmis au reducer par spans; il n'y a ni second décodage, ni second rejeu, ni rétention d'un historique de chunks. Une exception exige de jeter l'état dérivé partiel. Les allocations de recertification épuisées sont classées comme refus opérationnel et non comme rejet scientifique.

Le reducer garde uniquement locator sparse, DSU union-rank à identité canonique minimale, arènes compactes de sortie et compteurs par ordre. À chaque lot, il authentifie la clé terminale par un probe allocation-free du locator strict, puis gèle tous les carriers $R$ et les selles $L$, construit le quotient transitif complet sur $R\sqcup L$ et applique $q_R=0,1,\geq2$ avant toute naissance courante. Le payload, les unions et bindings sont entièrement préparés; après commit du locator, le DSU et les sorties avancent sans allocation. Le budget calculable des racines finales est vérifié dès le constructeur. Aucune facette ou coface globale, incidence, cellule, Gamma ou mosaïque de Delaunay d'ordre supérieur n'est construite.

La validation courte emploie les vrais stamps successifs du tétraèdre, rouvre deux chunks, replie chacun exactement une fois et obtient une forêt récursivement identique au builder résident. Les découpages un et deux lots donnent la même sortie. La fixture permanente Gabriel conserve la descente `AC` vers le terminal `DE`; une substitution de terminal échoue par probe sans mutation. Les quatre CTests ciblés passent en environ trois secondes, dont l'executor 14D domine. Cette tranche ferme le noyau de réduction résident--streaming et sa reconstruction depuis le préfixe durable, mais pas la transaction vivante 14H--locator--reducer, le jalon un million, 10 M+, le SLO 50 k, M.1 ou un statut public.

### Tranche 15D — commit vivant composite en mémoire

15D ajoute `ExactDirectMorseForestReducer::fold_prepared_ticket` sous `reference_cpu / hgp_reduced / budgeted / architecture_only`. L'entrée est la capability 14H mobile et à usage unique déjà préparée par l'executor, avec provenance exacte du delta, sceau de session, epoch, curseur source et successeur complets et stamp du locator strictement pré-lot. Le reducer exige que l'executor référence exactement son instance de locator, et non un état seulement égal en valeur, puis vérifie que leurs curseurs de lots coïncident.

Tant que le curseur 14H est gelé, le reducer effectue la projection, toutes les allocations, probes, décisions de budget, construction du quotient, préparation du payload et tentative atomique de fold. Un refus du reducer consomme le ticket mais laisse inchangés locator, DSU, forêt et curseur 14H. Après le commit du locator et de l'état scientifique, les seules opérations restantes sont des moves `noexcept`, des affectations scalaires et des incréments préflightés; le curseur 14H rejoint alors le même successeur sans rejeu géométrique indépendant. Une contradiction dans cette zone irréversible est fail-stop et ne peut pas être reclassée en rejet.

Les tickets déplacés, consommés, étrangers ou devenus stale sont rejetés atomiquement. Un locator distinct, même porteur du même stamp, et un désalignement entre curseurs reducer et executor le sont aussi. Le chemin conserve seulement le delta compact et les états sparse de 15C; il ne construit aucune facette ou coface globale, incidence, cellule, Gamma ou mosaïque de Delaunay d'ordre supérieur.

Cette transaction est exclusivement vivante et en mémoire. Elle ne lie pas encore le commit composite au remplacement durable de `HEAD`, ne checkpointte pas locator, DSU ou forêt et ne ferme ni la reprise après crash de cette progression, ni le jalon un million, ni 10 M+, ni le SLO 50 k, M.1, la Phase 15 ou l'entrée de la Phase 16.

### Tranche 15E — publication durable et commit vivant sérialisés

15E ajoute `ExactDirectMorseDurableLiveCommitCoordinator` sous `reference_cpu / hgp_reduced / budgeted / architecture_only`. Pour cette première couture de correction, le plan effectif doit contenir exactement un lot complet par chunk durable. La préparation recertifie par rejeu 14D frais que le delta du ticket 14H est exactement celui du chunk; elle n'en sérialise ni la capability, ni le locator, ni le DSU, ni la forêt.

Le store écrit, synchronise et relit le run et le candidat `HEAD`, puis appelle un participant process-local avant le remplacement de `HEAD`. Ce participant distingue obligatoirement `rejected_atomically`, `committed` et `indeterminate`. Il exécute le commit 15D pendant que l'ancien `HEAD` reste autoritatif. Un rejet atomique nettoie les fichiers pré-`HEAD` et autorise une nouvelle préparation. Après un commit vivant, toute erreur de gate, de renommage, de synchronisation ou toute issue indéterminée empoisonne le coordinateur et impose de jeter store, executor et reducer puis de les reconstruire depuis le seul `HEAD` recertifié; aucun retry local n'est permis.

Le théorème est une absence de divergence observable ou réutilisable sous sérialisation exclusive, et non une atomicité physique entre RAM et filesystem qu'Unix ne fournit pas. Sur succès acquitté, les trois autorités satisfont `HEAD.next_batch_index == reducer.next_source_batch_index() == executor.next_source_batch_index()`. Après réouverture, le reducer est reconstruit par 15C, puis un nouvel executor vérifie le compteur du locator et dérive tous ses curseurs chunk, lane, famille et bras depuis le préfixe certifié; un nouveau sceau de session est créé et aucun ancien ticket n'est repris.

La fixture courte committe le premier lot du tétraèdre, ferme le processus logique, rouvre et recertifie `HEAD`, reconstruit le reducer, reprend 14H au préfixe un, committe le second lot et retrouve exactement la forêt résidente. Une suppression de `.HEAD.tmp` provoque ensuite un échec de renommage après le commit vivant : store et coordinateur deviennent inutilisables jusqu'à réouverture. Cette tranche ferme la couture de correction mono-lot, mais un `fsync` par lot n'est pas une architecture de débit pour 10 M+. Le bootstrap singleton implicite et bulk, l'externalisation des autorités et les sorties segmentées restent requis avant le jalon un million, 10 M+, le SLO 50 k, la sortie de Phase 15 ou l'entrée de Phase 16.

### Tranche 15F — bootstrap singleton bulk sans staging linéaire

15F spécialise uniquement le premier lot certifié `(ordre=1, niveau carré=0)` sous `reference_cpu / hgp_reduced / budgeted / architecture_only`. Le nuage canonique refuse les doublons et une boule portée par au moins deux points distincts a un rayon strictement positif; le journal fraîchement vérifié doit en outre présenter exactement les `n` rôles singleton canoniques, sans selle, bras, clé résolue ni travail de fermeture. Si cette forme complète n'est pas satisfaite, le reducer conserve le chemin général.

`ExactDirectSparsePositiveFacetLocator::apply_canonical_singleton_identity_batch` reçoit seulement `n`. Il préflighte l'état vide, les handles, `PointId`, témoins `3i+1`, budgets, capacités, digest et allocations, puis remplit directement la table persistante et l'arène plate avec la clé `{i}` liée au composant `i`. Aucun tableau d'entrée `FacetBinding`, aucun `PendingBinding` et aucune table scratch de `2n+1` slots n'est formé. Le suffixe après la première mutation est `noexcept` ou fail-stop; les rejets antérieurs gardent le stamp initial.

Le reducer valide chaque rôle et projection, écrit directement dans ses arènes finales déjà réservées sous rollback, appelle ce commit bulk, puis active le préfixe DSU canonique sans allocation. Sur l'ABI GCC 64 bits courante, il supprime `160n+144n+120n` octets de staging reducer, puis `120n+(2n+1)8` octets dans le locator, soit `560n+8` octets transitoires : environ 5,6 Go à 10 M, 28 Go à 50 M et 56 Go à 100 M. La sortie persistante, le journal événementiel et les autres autorités restent inchangés; cette réduction de pic ne constitue donc pas encore l'externalisation.

Les tests courts comparent le locator bulk au lot ordinaire, recertifient sa structure, couvrent collisions forcées et rejets atomiques, puis attestent dans les voies projetée et vivante que les trois populations de staging valent zéro. La forêt finale reste identique au builder résident. Le probing sous un masque de fingerprint artificiellement nul demeure quadratique; ce cas de collision teste la correction et n'est pas un profil industriel. Le jalon un million, 10 M+, les sorties segmentées, le SLO 50 k, la sortie de Phase 15 et l'entrée de Phase 16 restent ouverts.

### Tranche 15G — journal événementiel singleton implicite

15G est validé hôte sous `backend=reference_cpu`, `profile=hgp_reduced`, `mode=implicit_singleton_event_journal`, `deployment_status=architecture_only` et `public_status=not_claimed`. Le schéma `ExactDirectMorseEventJournalResult` passe à la version 2. Il ne matérialise plus aucune projection ni aucun rôle singleton : `materialized_direct_event_projections` contient seulement les $E$ événements directs et `materialized_direct_role_records` seulement leurs $R$ rôles. Une vue non possédante à un pointeur génère à la demande le préfixe logique canonique; sa construction depuis un rvalue est interdite.

Le fait mathématique est fermé localement. Le journal refuse une source directe dont le niveau carré a un numérateur nul; comme `ExactLevel` est non négatif, tout événement direct a donc un niveau strictement positif. Les $n$ singletons d'ordre un et niveau zéro forment ainsi le premier lot strict. Trier uniquement les rôles directs puis préfixer le lot singleton reproduit exactement les indices logiques, offsets, comptes et clés de lots antérieurs. Les projections directes gardent les indices $n+j$, les rôles directs gardent les indices $n+r$, les familles de graines conservent leurs provenances et les digests de nuage, sémantiques et de reprise restent inchangés. Le schéma v2 distingue la représentation physique sans modifier l'identité scientifique ou le `HEAD`.

Sur l'ABI GCC 64 bits observée, une projection vaut 176 octets, un rôle 32, un batch 112 et un `RoleSeed` 96. L'ancien journal réservait $320n$ octets persistants et jusqu'à $416n$ octets au pic à cause notamment de `batches.reserve(role_seeds.size())`; $208n$ octets étaient construits persistants et $304n$ au pic. Le nouveau coût singleton a un coefficient nul en $n$ : il reste un batch de 112 octets et une vue temporaire de 8 octets. Pour $E$ événements directs et $R\leq2E$ rôles directs, le pic nominal devient $176E+128R+112(R+1)+O(1)$ octets.

Les trois CTests ciblés journal, graines de bras et reducer passent 3/3 en 0,08 seconde. Ils vérifient les comptes physiques $E,R$, les comptes logiques inchangés, la génération du préfixe, le rejeu streaming sans scan singleton, les provenances des graines, le bulk 15F et l'identité avec la forêt résidente. Cette preuve structurelle n'est ni une mesure RSS ni une qualification massive. L'archive des autorités, les sorties externalisées et segmentées, le jalon un million, le pipeline complet à 10 000 001 points, puis conditionnellement 30 000 000, 50 000 000 et 100 000 000 points, le SLO 50 k, la sortie de Phase 15 et l'entrée de Phase 16 restent ouverts.

### Tranche 15H — autorités singleton implicites jusqu'au locator et à la forêt

15H est validé hôte sous `backend=reference_cpu`, `profile=hgp_reduced`, `mode=implicit_singleton_locator_forest_authorities`, `deployment_status=architecture_only` et `public_status=not_claimed`. Après le bulk 15F, le locator ne matérialise plus aucun slot ni `PointId` de clé pour les $n$ singletons. Si $B$ est la capacité logique de bindings et $D=B-n$, sa table physique possède exactement $2D+1$ slots; un probe de `{i}` pour $i<n$ régénère le témoin `3i+1`, effectue zéro visite physique et suit les parents canoniques courants du locator. Les bindings directs ultérieurs conservent leurs indices logiques $n+j$ et occupent seuls la table et l'arène de clés. Le schéma logique des stamps et de leur digest reste volontairement v1; le résultat de probe, la vue de stockage et le sweep physique passent en v2.

Le journal de forêt passe au schéma v3. Les préfixes logiques de $n$ naissances et de $n$ nœuds d'ordre un sont générés à la demande par `ExactDirectMorseForestJournalView`; les arènes physiques retiennent uniquement le suffixe direct, sans modifier identifiants de nœuds, offsets, compteurs, budgets, digests ou résultat logique. Le reducer v2 réemploie en outre les parents canoniques du locator comme unique autorité d'union des carriers. Il garde seulement l'état des handles directs et une table d'override de racines réduites de capacité $2G+1$, où $G$ est le cap de groupes atomiques; aucun second DSU dense de singletons n'est construit. Les méthodes `certified_*` vérifient la cohérence interne de ces comptes; un vérificateur frais distinct les lie au nombre total de handles, aux $n$ singletons et au budget $G$ fournis par une autorité de confiance. La recherche finale ne balaie plus tous les handles : un représentant suivi par ordre donne au plus $K$ consultations.

Les probes, le sweep de préfixes, les vérificateurs structurels, les voies résidente et streaming et les consommateurs verticaux sont rejoués avec les mêmes résultats logiques. Les tests falsifient aussi les bornes implicites, les index physiques/logiques et les vues de suffixe. Cette tranche supprime des coefficients linéaires en $n$ des slots, clés, sorties d'ordre un et états de carriers, mais ne constitue pas une mesure RSS portable. Le nuage, le LBVH, les autorités directes, les sorties non singleton et le locator parent restent résidents.

15H ne construit toujours aucune population globale de facettes, cofaces, incidences, cellules, Gamma ou Delaunay d'ordre supérieur. Les chaînes de parents canonical-min restent non compressées et doivent recevoir une borne de hops certifiée. Le builder générique préflighte encore le cap logique historique $2B+1$, et le consommateur vertical conserve plusieurs arènes $O(n)$ malgré la suppression d'une réserve temporaire inutile de $n$ identifiants. 15H ne ferme donc ni l'archive unifiée des autorités, ni le sink segmenté des sorties, ni le jalon un million, ni le pipeline complet à 10 000 001 points avec interruption et reprise, ni le SLO 50 k, M.1, la sortie de Phase 15 ou l'entrée de Phase 16.

### Tranche 15I — sortie de forêt segmentée par lot committé

15I est validé hôte sous `backend=reference_cpu`, `profile=hgp_reduced`, `mode=segmented_committed_forest_batch_output`, `deployment_status=architecture_only` et `public_status=not_claimed`. Le mode de sortie est fixé au constructeur. Le chemin interactif conserve le journal résident complet; le chemin massif retient après un fold exactement un segment committé, refuse le lot suivant jusqu'à son acquittement, conserve ce segment bit à bit si le sink le rejette, puis rend ses capacités au reducer pour le lot suivant. Le premier segment décrit le lot singleton logique sans matérialiser ses $n$ naissances ni ses $n$ nœuds.

Les curseurs cumulatifs remplacent les tailles des vecteurs historiques pour tous les indices futurs. Chaque record garde donc ses offsets, identifiants de nœuds, références d'enfants, provenances et stamps locator dans le repère logique global du journal résident. Une chaîne SHA-256 structurelle lie le digest initial, le payload pré-commit, les curseurs et les champs post-commit. Elle détecte une mutation en mémoire, mais n'est ni un format wire, ni un checksum de fichier, ni une autorité scientifique. Les lots suivants consultent seulement le locator, ses parents canoniques, l'état compact des carriers, les overrides de racines et les compteurs; aucun segment historique ne pilote une décision.

Si $L$ borne le locator physique, $C$ l'état direct des carriers, $G$ les overrides de racines, $T_b$ le scratch du lot et $P_b$ son segment de sortie, la résidence propre au reducer segmenté est

$$\mathcal{O}\left(L+C+G+K+\max_{b}\left(T_b+P_b\right)\right).$$

Cette borne exclut les segments que l'appelant déciderait de conserver, le nuage, le LBVH, les autorités amont, le slack de l'allocateur et la taille des limbs exacts autrement que dans $P_b$; ce n'est pas une mesure RSS. Le chemin résident destiné aux nuages d'environ 50 000 points ne paie aucun sink, aucune sérialisation et aucune synchronisation supplémentaire. Le chemin segmenté évite de conserver les historiques complets de naissances, attaches de bras, selles, groupes atomiques, enfants, batches et nœuds. Il ne construit toujours aucune population globale de facettes, cofaces, incidences, cellules, Gamma ou mosaïque de Delaunay d'ordre supérieur.

Sur la fixture courte, la concaténation des segments reproduit exactement toutes les arènes physiques du journal résident, avec les mêmes batches, offsets et identifiants; le sceau terminal $O(K)$ retrouve les mêmes compteurs, racines finales, stamp locator et compte logique de sortie. Il serait cependant faux d'annoncer une reconstruction bit à bit du struct `ExactDirectMorseForestJournalResult` v3 complet : 15I ne transporte pas encore `requested_budget`, la configuration, l'ordre de parcours ni tous les drapeaux de tête. Il ne fournit pas davantage de codec, run header, spool durable, atomicité filesystem, reprise après crash, archive unifiée, checkpoint du locator ou des carriers, lecteur vertical segmenté, borne de profondeur des parents, mesure RSS, jalon un million, pipeline complet à 10 000 001 points, SLO 50 k, M.1 ou statut public exact.

La priorité d'externalisation annoncée par 15I est fermée par 15J ci-dessous. Le run header, le codec, le spool durable et le lecteur vertical segmenté restent distincts. Le SLO 50 k doit toujours être mesuré séparément sur le chemin résident chaud, sans lui imposer le protocole disque massif.

### Tranche 15J — autorité source unifiée par manifeste et fenêtre de lot

15J est validé hôte sous `backend=reference_cpu`, `profile=hgp_reduced`, `mode=bounded_unified_batch_authority_view`, `deployment_status=architecture_only` et `public_status=not_claimed`. `ExactDirectMorseForestSourceManifest` est un résumé scalaire certifié construit après rejeu frais des journaux événementiel et de graines. Il engage leurs versions et budgets, les comptes logiques globaux, les identités sémantiques pair et higher, une identité source commune séparée par domaine, les digests initial et final de la chaîne dense de lots et son propre digest SHA-256. Les digests pair et higher restent distincts et ne sont jamais comparés entre eux comme preuve d'une autorité commune.

`ExactDirectMorseForestSourceBatchProviderView` prête exactement le lot demandé pendant un callback synchrone. La fenêtre engage le manifeste, l'identité source, le digest de chaîne entrant, l'identité du lot et le digest successeur. Les indices demeurent globaux : les rôles, familles et graines sont des spans contigus, tandis que les projections et événements sont obtenus par lookup. Le lot singleton conserve ses $n$ rôles logiques implicites et un span physique vide. Dans cette voie, le cœur du reducer ne retient plus le nuage, la façade ou les journaux globaux; il conserve seulement le manifeste et la vue du provider, puis abandonne tous les spans et lookups avant le retour de `fold`. La sortie segmentée 15I, ses curseurs et sa chaîne restent inchangés.

Le manifeste scalaire est copié dans le reducer, donc un temporaire ne peut pas laisser de pointeur pendant. Les anciens faits `source_*_freshly_replayed` restent faux à l'ouverture; chaque fenêtre doit porter l'assertion d'autorité qu'elle vient d'être recertifiée, et le reducer ne publie ces faits qu'à la finalisation après avoir compté exactement tous les lots committés. Cette assertion reste relative au provider : elle ne dispense jamais son implémentation massive du rejeu frais exigé ci-dessous.

La chaîne SHA-256 fixe l'identité et l'ordre attendus, mais elle n'est pas une autorité scientifique précommit et ne rend pas fiable une fenêtre observée. Un provider massif devra reconstruire ou recertifier fraîchement chaque fenêtre depuis ses autorités immuables, comparer son identité complète, puis seulement la prêter; son cache décodé, son index et son éventuel `mmap` resteront dérivés et jetables. Si $W_b$ borne le payload scientifique du lot $b$ et $R_b$ le scratch de cette recertification, la couture persistante du reducer est scalaire et la résidence synchrone propre au provider visé vaut $\mathcal{O}\left(W_b+R_b\right)$, indépendamment des lots antérieurs. Cette expression compte les limbs exacts dans $W_b$ et n'autorise jamais à couper un lot ordre--niveau indivisible.

L'adapter résident de compatibilité ne copie aucun record scientifique et préserve le chemin interactif, mais il conserve encore la façade et les deux journaux, calcule les hashes de tous les lots et stocke $B+1$ préfixes de chaîne pour $B$ lots. Il n'est donc ni une borne massive, ni une preuve de latence chaude. Le caractère synchrone et la non-rétention du consumer sont un contrat de durée de vie, pas une capacité rendue impossible à copier par le type-erasure; un provider concret devra enfermer ce contrat et tester les fautes après callback. Le sceau final 15I ne transporte pas encore le digest du manifeste ni le digest terminal de la chaîne source, et l'exécuteur 14H intégré conserve toujours le nuage, la façade et les deux journaux globaux. Aucun provider massif concret, codec, archive, run header, footer durable, spool, atomicité filesystem, reprise après crash, checkpoint du locator ou des carriers, lecteur vertical segmenté, mesure RSS, jalon un million, pipeline complet à 10 000 001 points, SLO 50 k, M.1 ou statut public exact n'est acquis. 15J ne construit aucune facette, coface ou incidence globale, aucune cellule, aucun Gamma et aucune mosaïque de Delaunay d'ordre supérieur.

Conseil pour la suite : implémenter d'abord le wire canonique, un provider massif ne retenant qu'une fenêtre et un header--footer durable lié au manifeste et à la chaîne, puis fermer le gate complet à un million avant 10 000 001 points. Mesurer séparément le `warm_e2e` résident à 50 k et n'optimiser le hashing que s'il apparaît effectivement dans le profil.

### Tranche 15K — wire canonique borné comme témoin, jamais comme autorité

15K est validé hôte sous `backend=reference_cpu`, `profile=hgp_reduced`, `mode=canonical_bounded_source_batch_wire`, `deployment_status=architecture_only` et `public_status=not_claimed`. Chaque fenêtre fraîche 15J possède désormais un encodage canonique big-endian indépendant de l'ABI : enveloppe fixe de 64 octets, version et type fermés, longueur, digest SHA-256 du payload, identités de manifeste, source, lot et chaîne, indices globaux, batch, rôles, projections, événements complets, familles et graines. Les centres et niveaux exacts sont encodés par leur texte rationnel canonique sous caps individuel et cumulatif; les pointeurs, spans, callbacks et drapeaux de cache ou de résidence ne sont jamais sérialisés.

Le wire reste explicitement non fiable. `ExactDirectMorseForestCanonicalWireSourceProvider` emprunte une seule image observée, demande séparément au provider scientifique la fenêtre du même lot fraîchement recertifiée, mesure puis réémet canoniquement cette dernière et exige l'égalité octet par octet. Le reducer reçoit seulement la fenêtre fraîche; aucun record décodé du wire ne peut piloter une décision. Cette séparation est relative au contrat du provider frais — callbacks stables pendant la visite et absence de lecture du payload observé — et n'est pas imposée mécaniquement par les vues non possédantes. Une corruption, troncature, extension, incohérence de chaîne ou limite dépassée échoue avant le callback scientifique aval. Après un refus, le même lot et le même delta peuvent être rejoués sans avance du locator ou du curseur.

Si $C_b$ désigne le nombre d'octets du wire observé, $W_b$ la taille de la fenêtre scientifique, $R_b$ son scratch de recertification et $E_b$ le plus grand texte exact temporaire, le surcoût propre au comparateur 15K avant le callback aval est $\mathcal{O}\left(C_b+W_b+R_b+E_b\right)$ octets. Son coût courant est $T_{\mathrm{provider}}(b)+2\left(T_{\mathrm{preflight}}(F_b)+T_{\mathrm{cert}}(F_b)+T_{\mathrm{emit}}(F_b)\right)$ : chaque émission conserve volontairement ses propres gardes bornés et sa recertification. Cette borne exclut le backing store ou l'archive possédée par les providers ainsi que le scratch et le temps du consumer; l'appel complet ajoute donc explicitement leur résidence et $T_{\mathrm{consume}}(b)$. La comparaison ne construit pas un second wire attendu. Les conversions décimales sont préfiltrées arithmétiquement avant les appels canoniques de 15K et leur coût n'est pas annoncé linéaire dans $C_b$. L'API d'encodage destinée à produire l'image initiale possède naturellement son unique vecteur de $C_b$ octets. Un lot ordre--niveau demeure indivisible. Une optimisation future pourra fusionner ces validations seulement après avoir figé un jeton immuable entre les deux émissions.

Le test ciblé court couvre golden header--digest, déterminisme, padding non sémantique, singleton implicite, un lot direct avec événements et exacts, corruption de header ou payload, troncature, suffixe, trois classes de caps, rejet de `SIZE_MAX`, provider mono-fenêtre, retry atomique et identité finale avec la forêt résidente. Le CTest Release strict passe en 0,04 seconde sous GCC et 0,03 seconde sous Clang. 15K ne livre toutefois aucun codec de décodage scientifique, provider fichier, recertifier massif depuis les runs pair--higher, index d'archive, header--footer de run, `fsync`, publication atomique, anti-rollback, spool durable 15I, lecteur vertical segmenté, checkpoint locator/carriers, mesure RSS, gate 1 M ou 10 000 001 points, SLO 50 k, M.1 ou statut public exact. Il ne construit aucune facette, coface, incidence ou cellule globale, aucun Gamma et aucune mosaïque de Delaunay d'ordre supérieur.

Conseil futur : 15L doit lier le manifeste, la chaîne source terminale et la chaîne de segments 15I dans un header--footer durable, fournir un chargeur borné et un recertifier massif réel, puis exercer une reprise après interruption. Le gate complet à un million doit précéder 10 000 001 points. Le diagnostic 50 k de Phase 14 reste une voie résidente séparée, sans sérialisation massive dans son chrono.

### Composant transversal Phase 15 — reçu exact local de prune bloc--bloc

Le SHA `7faecca0107c990ac2a0b0bfed1a15f22dc1d153` valide sur hôte `reference_cpu / hgp_reduced / exact_block_rank_prune_receipt`, sans ouvrir un nouvel incrément de Phase 15 ni modifier ses portes. Le composant authentifie deux nœuds supports LBVH disjoints et une antichaîne canonique de nœuds témoins également disjoints. Pour $K=10$, il ne délivre un reçu `certified_above_rank` qu'après avoir recertifié au moins dix feuilles distinctes et $\max\phi<0$ sur tout le produit; une égalité, un signe positif, une autorité invalide, un chevauchement, une masse insuffisante, un dépassement arithmétique ou plus de 64 propositions reste fail-open. Le reçu process-local et move-only conserve au plus dix nœuds témoins et se revalide contre le nuage, le LBVH, les deux supports et le rang.

Les masses $\lvert A\rvert\lvert B\rvert$ et $2\lvert A\rvert\lvert B\rvert$ qu'il authentifie sont locales au produit. Elles ne peuvent être additionnées globalement qu'après preuve que l'ordonnanceur partitionne sans recouvrement l'univers non dirigé ou dirigé correspondant. Le CTest ciblé passe sous GCC et Clang stricts; il couvre le succès multifeuille, l'ordre canonique, le rejeu du reçu, les caps, chevauchements, masses insuffisantes et signes strictement positif ou exactement nul. L'intégration au proposeur GPU $W=4\,096$, aux 48 workers CPU, à la subdivision fail-open et aux compteurs de Phase 14 reste à réaliser. Ce composant ne construit ni paire globale, ni facette, coface, incidence, cellule Gamma ou mosaïque de Delaunay d'ordre supérieur, et ne prouve ni une hiérarchie HGP complète ni son exactitude publique.

### Travaux

- porter en premier le catalogue exact multi-ordre des paires de rang fermé jusqu'à $K_{\max}+1$ sur des tuiles d'ancres GPU : `morton_yao48_pair_frontier` fusionne ownership Morton, banques Yao48 et prunes de régions, puis le rang exact et le payload fermé par `count + scan` ne traitent que les survivants; fermer `candidate + certified_pruned + unresolved = n(n-1)/2`, interdire tout fallback dense et conserver les implémentations quadratiques plafonnées à 512 points dans les seules cibles `*_reference`;
- après fermeture des paires, implémenter la frontière GPU indépendante des seuls triangles aigus, produire depuis leurs saturés les tétraèdres de support trois, puis chercher indépendamment les seuls tétraèdres bien centrés; tester chaque étage contre un oracle GPU dense borné sans jamais promouvoir ce scan combinatoire en architecture produit;
- intégrer un seul pipeline `exact_sparse_frontier` : LBVH et frontière GPU plate commune, réduction directe du flux `pair` à $k=1$, spécialisations `pair` et `higher` à $k=2$, développement exact des `extra_shell`, merge canonique puis reducer sparse; aucune variante produit sélectionnable et aucun calcul EMST ou Delaunay industriel;
- ne plus étendre ni exécuter à 50 k la voie `stackless_product_batch` répétée rejetée au SHA `17f7b04`; conserver `ExactLbvhYao48Emst` et Borůvka comme oracles hors ligne $k=1$, rejeter leur réalisation CPU mesurée comme chemin produit et comparer le flux de paires sans les appeler depuis le binaire chronométré; le squelette local de rang au plus trois reste un raccord de conception, jamais un substitut au rejeu frais des sources;
- geler `gabriel_fusion_deadline_v1`, Geogram PDEL, l'overlay, le surrogate et les cinq variantes directes comme oracles bornés hors ligne seulement; archiver aussi la fenêtre Morton seule comme heuristique sans autorité, et ne jamais les raccorder comme dépendance, fallback ou correction du produit;
- après scellement des conclusions, conserver leurs fixtures et checkers minimaux, puis retirer les anciennes cibles, configurations et campagnes du build et des benchmarks produit; exiger le reçu `legacy_oracle_cleanup_complete` avant qualification;
- traiter $M\sim cK\ln n$ uniquement comme politique adaptative de performance, sans preuve universelle ni droit de publier un préfixe; toute frontière non vide échoue fermée;
- comparer ce chemin produit unique aux oracles bornés et au sidecar PDEL scellé, puis qualifier séparément son vrai `warm_e2e` 50 k/$K=10$ sous 100 ms;
- planificateur de lots selon cinq budgets typés : device, RAM hôte, scratch et sortie en octets, temps interne en nanosecondes monotones avec réserve non prêtable;
- sur G4, Hyperdisk Balanced pour le boot et les checkpoints durables, et jusqu'à quatre Titanium SSD pour le scratch éphémère rapide;
- instantané `BudgetSnapshot` avant chaque lot, run, merge et sérialisation;
- fermeture complète avant éviction;
- signatures d'événement et d'attache triables;
- événements certifiés, gateways, états DSU et frontières de reprise sérialisés avant éviction; aucun atlas de cellules ni état de couture de mosaïque n'est requis;
- runs checksummés;
- merge externe déterministe;
- sorties mémoire-mappées;
- checkpoint atomique par ordre;
- reprise après kill et préemption;
- estimation pessimiste du scratch avant lancement;
- garde transactionnelle : réserver simultanément ancien état, nouvelle écriture temporaire, pire espace de merge, espace du checkpoint et marge de sécurité; écrire, synchroniser, vérifier, renommer atomiquement, puis seulement supprimer l'ancien état.

### Tests

- fixture où la connexion n'apparaît qu'après l'union du plateau d'échéance, pour interdire un test strict `<a`;
- fixtures positives avec fusion strictement précoce, dont les contre-exemples Gamma$_2$ à six et huit points, afin de prouver que le garde-fou n'exige pas l'égalité des hiérarchies;
- fixtures négatives minimales `late` et `never`, dégénérescence `unsupported`, mutation de niveau source et ordre pré/post-plateau;
- dans la suite d'oracles bornés uniquement, matrice des anciennes variantes directes sur facettes; pour le surrogate, succès précoce et égal puis échecs tardif, non couvrant, non monotone, convention absente ou incompatible et adaptateur absent;
- fermeture des cinq compteurs, digest de décisions et premier témoin obligatoire; le résumé massif sans records ni preuve producteur de la comparaison croisée doit échouer fermé;
- correction par fusion Gabriel minimale suivie d'un second rejeu positif, avec identité du résultat sous permutation des records d'un même plateau;
- contrôle statique que le build, les configurations et les benchmarks produit n'exposent plus les anciennes variantes après `legacy_oracle_cleanup_complete`, tandis que leurs fixtures et checkers bornés restent rejouables;
- résident contre streaming sur tailles communes;
- tailles de lots aléatoires;
- interruption à chaque frontière possible;
- corruption d'un run détectée;
- disque presque plein avant et pendant un merge, sans corruption de l'état durable;
- limites exactes de chaque budget à un octet près et sortie `budget_exhausted` reproductible;
- kill entre écriture, `fsync`, renommage et publication du manifeste;
- reprise avec version incompatible refusée;
- dix millions de points sans OOM lorsque le certificat reste sparse, avec arrêt budgétaire transactionnel sinon.

Après un succès réel à 10 000 001 points, une extension conditionnelle tente 30 000 000, puis 50 000 000, puis 100 000 000 points, strictement dans cet ordre. Chaque échelon exige avant lancement une borne mémoire device--hôte--scratch--sortie admissible, une enveloppe de temps bornée et une frontière de reprise vérifiée; après interruption contrôlée, la reprise doit reproduire le même préfixe. Une construction LBVH seule, un composant isolé ou un pipeline scientifique interrompu ne qualifie pas l'échelon et n'autorise aucune revendication produit.

### Porte de sortie

Résultats octet par octet identiques entre résident et streaming. Reprise déterministe après interruption. Le jalon intermédiaire d'un million valide le mécanisme; la cible produit est dix millions ou davantage lorsque le certificat reste sparse.

## Phase 16 — Plusieurs millions et campagne scientifique

> [!CAUTION]
> Porte d'entrée fermée. Les timings v4/v5 du surrogate point-MST et leurs anciens gardes $k=1$--$k=2$ ne satisfont pas la porte 50 k de la vraie hiérarchie. La Phase 16 reste `blocked` jusqu'au succès du pipeline source facettes--cofaces--incidences--couverture--verticalité, puis de sa vue `min_cluster_size=20`.

### But

Mesurer le domaine pratique et les régimes d'échec honnêtes.

### Tailles

$$n\in\left\lbrace10^3,3\times10^3,10^4,3\times10^4,5\times10^4,10^5,10^6,3\times10^6,10^7\right\rbrace.$$

### Familles

- Poisson homogène et inhomogène;
- mélanges de 1, 2, 4, 8, 16, 32 et 64 amas;
- poids équilibrés puis rapport 1:100;
- anisotropie et mauvais conditionnement;
- ponts, cols, amas emboîtés, bruit et outliers;
- plans, sphères, tores, tubes et filaments;
- nuages LiDAR avec densité variable;
- grilles, couches cosphériques et courbe des moments;
- doublons et perturbations à un ULP.

### Mesures

Pour chaque combinaison, publier correction, statut, compteurs par ordre, temps, pic VRAM, mémoire hôte, disque, fallbacks et taille des sorties. Les résultats sont groupés en régimes volumiques, surfaciques et adversariaux.

### Porte de sortie

Courbes de complexité avec intervalles, seuils de passage au streaming et taxonomie des échecs. Objectif conditionnel : dix millions en moins de dix minutes si le certificat est sparse; sinon arrêt budgétaire explicite.

## Phase 17 — Tour de boules saturées sensible à $H_0$

### Position dans le programme

Cette piste est parallèle et non bloquante. Sa première sous-phase s'ouvre seulement après fermeture des phases 1 et 2A; elle ne remplace ni le flux direct des phases 9–10, ni la preuve Morse M.1 de la phase 12. Toute matérialisation de Čech ou Gamma y reste un oracle borné et n'entre pas dans le chemin produit. Elle exploite les théorèmes S.1–S.6 de l'[audit des boules saturées](math/TOUR_BOULES_SATUREES.md) comme représentation combinatoire de comparaison.

Le prototype reste `backend=reference_cpu`. Tant qu'aucune migration de schéma n'active une base de preuve dédiée, il s'exécute comme oracle de recherche et ne publie pas `public_status=exact`, même si ses objets internes sont calculés exhaustivement.

### 17A — preuve exécutable et oracle CPU borné

Après les phases 1 et 2A :

1. créer un type interne `SaturatedGenerator`, séparé de `CriticalEvent`, avec saturé, boule exacte, niveau, capacité et supports témoins;
2. énumérer exhaustivement les supports affinement indépendants de tailles un à quatre;
3. classifier exactement tous les points contre chaque boule fermée;
4. agréger les supports multiples et dédupliquer par boule exacte et saturé;
5. conserver tous les générateurs, sans pruning par inclusion;
6. construire le graphe d'intersection statique, puis une forêt de Kruskal de poids maximum avec ordre total canonique;
7. comparer à chaque coupe ouverte et fermée l'oracle structurel interne pour $1\leq k\leq n$ : faces de Čech matérialisées lorsque le budget le permet, Gamma exhaustif, graphe d'intersection et forêt seuillée; limiter toute comparaison au contrat v2 à $1\leq k\leq\min(10,n)$;
8. reconstruire séparément `full_pi0`, la réduction `hgp_reduced`, les `coverage_delta` et les applications verticales;
9. traiter chaque lot de niveaux égaux atomiquement et vérifier l'invariance par permutation;
10. émettre les certificats internes `support_universe_complete`, `closed_ball_ranges_complete`, `ball_dedup_complete`, `generator_batches_complete`, `overlap_join_complete`, `generator_msf_complete`, `merge_replay_complete` et `vertical_maps_complete`.

Le système sous test réutilise les prédicats C++ de phase 2A, mais l'oracle Gamma Python `Fraction` garde ses propres miniballs et classifications. Partager la même géométrie entre les deux côtés invaliderait l'indépendance du différentiel.

Le domaine initial est $n\leq14$. La campagne rejoue toutes les fixtures et graines exactes enregistrées par la phase 1, puis au moins $10\,000$ petits nuages par dimension affine avec graines et sorties canoniques conservées. La fixture Gabriel à cinq points doit contenir `ACDE` au niveau $33/2$, reconnaître ce niveau comme non critique pour $D_2$, puis connecter ce générateur à `ABC` au niveau $83886/3563$ exactement à l'ordre deux. La matrice inclut les shells cosphériques, supports multiples, niveaux égaux et une famille dont le rang saturé croît jusqu'à $n$ alors que l'ordre observé reste petit.

### 17B — range reporting et forêt insertionnelle

Cette sous-phase attend en plus la fermeture de la phase 4. Elle remplace le balayage global uniquement par des requêtes fermées certifiées et étudie :

- index spatial pour `closed_ball_range` avec shell complet;
- listes inversées `postings[x]` et comptage exact de $\lvert S\cap T\rvert$;
- activation des sommets et arêtes par lots exacts;
- mise à jour d'une forêt de poids maximum depuis la forêt précédente et toutes les arêtes du nouveau lot;
- snapshots persistants, journaux de remplacements et rejeu des coupes;
- pruning par inclusion désactivé dans la baseline exacte, puis variante expérimentale comparée avec contraction, rewiring et provenance explicites;
- propositions issues du raffinement top-$m$, de Delaunay, d'ANN ou de descentes, toujours resaturées exactement et traitées avec la sémantique scientifique interne `partial_refinement` sans certificat d'exhaustivité; aucune sortie publique v2 ne les sérialise avant migration contractuelle.

La propriété insertionnelle ne permet de libérer les anciennes arêtes non retenues qu'après certification de leur génération complète. Toute suppression de générateur dominé exige une preuve et une règle distinctes; elle ne réutilise pas silencieusement le lemme d'insertion.

Chaque checkpoint enregistre les checksums de l'entrée et de la configuration, le catalogue et la déduplication actifs, les postings, la forêt courante, le dernier lot entièrement committé, le curseur du flux restant, l'identifiant de l'ordre total canonique et le journal de rejeu. Le suffixe non traité est régénérable déterministement depuis ces checksums et ce curseur. Un état au milieu d'un lot n'est jamais publiable; la reprise l'annule ou le rejoue intégralement.

### 17C — shadow benchmark et éventuelle promotion

Cette sous-phase attend la fermeture de la phase 9 pour disposer de la baseline top-$m$. Elle s'exécute hors CI, sur activation explicite, avec arrêt budgétaire pour $n\in\left\lbrace16,24,32,48,64,96,128\right\rbrace$ et pour les familles volumique, surfacique, en amas et adversariale. Chaque manifeste fixe avant exécution les budgets de temps, RAM hôte, scratch et sortie; les observations censurées sont publiées comme telles. Mesurer au minimum :

- $C_U$ par taille de support, $M_{\mathrm{sat}}$ et $L_{\mathrm{sat}}=\sum_S\lvert S\rvert$;
- distribution des capacités $\lvert S\rvert$, memberships et `peak_active_inclusion_maxima`, défini comme le nombre maximal de générateurs actifs maximaux par inclusion; compter séparément le coût du join d'inclusion nécessaire à cette métrique;
- longueurs $d_x$ des postings et $P_{\mathrm{post}}=\sum_x\binom{d_x}{2}$;
- paires uniques, pic de l'accumulateur et arêtes examinées;
- remplacements de forêt, octets de l'historique et coût des requêtes de coupe;
- temps de saturation, déduplication, join, Kruskal et rejeu, pic RAM et bit-complexité exacte;
- comparaison des temps, pics et octets avec la voie actuelle;
- comparaison séparée des compteurs structurels et de leurs exposants empiriques avec $\sum_m(M_m+P_m+V_m+J_m)$, sans additionner des unités incompatibles.

À $n=50\,000$, $\binom{n}{4}=260\,385\,417\,812\,487\,500$ : l'énumération brute est un no-go explicite. Les $M-1$ arêtes de la forêt résidente ne bornent ni les memberships, ni le join, ni le scratch, ni l'historique. Le pruning ne récupère pas les coûts déjà payés.

Une voie de production ne peut être proposée qu'après :

1. une génération output-sensitive évitant l'univers de tous les quadruplets;
2. un certificat de complétude des générateurs;
3. un join d'intersections certifié dont le régime dense est budgété;
4. une conversion démontrée vers le `MergeForest`, `coverage_log` et les morphismes verticaux;
5. une persistance déterministe et reprenable;
6. une migration contractuelle ajoutant une base distincte, par exemple `saturated_ball_overlap_proved`;
7. zéro différence aux portes G2–G4, sans présenter l'accord expérimental comme une preuve de complétude.

Le qualificatif output-sensitive n'abolit pas le pire cas : seule la borne $M=O(n^{4})$ est établie ici et aucune borne universelle sous-quartique n'est démontrée pour cette famille. La cible est un surcoût proche de la sortie dans le régime sparse et un arrêt budgétaire explicite dans le régime dense, jamais une promesse sous-quartique universelle.

### Porte de sortie

La validation de 17A ouvre un jalon **oracle interne exact borné**, sans fermer la phase ni autoriser la production scalable. La phase 17 ferme après l'expérience 17B–17C, la publication de tous les compteurs et une décision documentée : promotion contractuelle si toutes les conditions sont prouvées, ou arrêt de la piste scalable en conservant l'oracle petit $n$ et un prototype hybride à sémantique scientifique interne `partial_refinement`. Un no-go de performance honnête peut donc fermer cette phase de recherche; un désaccord mathématique, un faux statut ou une complétude indécidable ne le peut pas.

## Phase 18 — Durcissement et jalons de release

### Travaux

- audit de sécurité mémoire et sanitizers;
- compatibilité de schémas;
- documentation API et tutoriels;
- exemples de coupes recouvrantes;
- licence de chaque dépendance;
- SBOM et versions épinglées;
- tests CPU sur chaque push;
- tests GPU manuels ou planifiés avec budget explicite;
- procédure de reproduction d'un papier;
- changelog et limites connues.

### Jalon `v1-correctness`

- `backend=reference_cpu, profile=full_pi0` stable sur le domaine exhaustif annoncé;
- `backend=reference_cpu, mode=certified, profile=hgp_reduced` stable avec `gamma_exhaustive_reference` sur le domaine exhaustif annoncé;
- `backend=cuda_g4, profile=hgp_reduced` ne peut obtenir `public_status=exact` qu'après activation d'une base de réduction complétée en incidences effectivement prouvée; Gabriel brut reste conditionnel;
- `profile=full_pi0` activé en production uniquement si la phase 12 est fermée;
- toute dégénérescence non couverte retourne `public_status=unsupported_degeneracy` sans `forest_semantics`, ou `mode=budgeted, forest_semantics=partial_refinement, public_status=conditional`, jamais `public_status=exact`;
- aucune dépendance à une ressource GCP permanente;
- résultats de référence signés et reproductibles;
- rapport de correction lié à un commit et aux preuves de portes.

### Jalon `v1-interactive-scalable`

Il dépend de `v1-correctness`, de la Phase 12 fermant M.1 et d'une migration contractuelle versionnée qui active la base de preuve exacte du flux direct. Il exige en plus : G6 atteint à 50 000 points, G7 atteint à un million, et la preuve dédiée `three_sparse_3m_exact_runs` constituée d'au moins trois exécutions génériques sparse avec `forest_semantics=exact, public_status=exact` à trois millions sans OOM et avec reprise vérifiée. Aucun statut limité ne satisfait G7b. Une réponse contrôlée à dix millions est également obligatoire : elle porte soit `forest_semantics=exact, public_status=exact` si le certificat reste sparse, soit `mode=budgeted, forest_semantics=partial_refinement` avec `public_status=conditional` ou `public_status=budget_exhausted`, checkpoint et diagnostic complet. Seul `public_status=exact` valide le SLO conditionnel de dix minutes.

## 6. Matrice de dépendances

| phase | dépend de | bloque |
|---:|---|---|
| 0 | documentation actuelle | toutes |
| 1 | 0 | 5, 6, 8–13, 17A |
| 2A | 0–1 | 2B, 4–13, 17A |
| 2B | 2A, 3 | 4–13 GPU |
| 3 | schémas de 0 | GPU 4–16 |
| 4 | 2A–2B, 3 | 6, 8–9, 17B |
| 5 | 1–4 | revendications hiérarchiques GPU |
| 6 | 1–4 | 12 |
| 7 | 2B–4 | 8–9 |
| 8 | 1–4, 7 | oracle borné de profondeur zéro, non bloquant pour le flux produit |
| 9 | 1, 2A, 4, 7 | 10 et baseline comparative de 17C |
| 10 | 5, 9 | 11 et promotion exacte; les prototypes 14–15 peuvent mesurer plus tôt un statut conditionnel |
| 11 | 10 | tour réduite publiable |
| 12 | 1, 6, 9–11 | piste topologique complète |
| 13 | 2A–2B, 12 | domaine non générique de la piste topologique |
| 14 | 10–11 | piste produit, SLO 50k |
| 15 | 9–11 | piste produit, mécanisme validé à un million puis cible 10 M+ certifiée ou arrêt budgétaire honnête |
| 16 | 14–15 | domaine pratique |
| 17 | 1 et 2A pour 17A; 4 pour 17B; 9 pour 17C | piste de référence puis décision scalable, sans bloquer la voie principale |
| 18 | phases livrées | release |

## 7. Portes go/no-go globales

| observation | décision obligatoire |
|---|---|
| un prédicat diffère de la référence | arrêt de la phase et réduction du cas |
| un overflow tronque une cellule | défaut bloquant |
| top-$k$, shell ou rang incomplet | défaut bloquant |
| événement manquant sur l'oracle | défaut bloquant |
| profil réduit de référence différent de Gamma exhaustif | défaut mathématique ou logiciel bloquant |
| flot Gabriel qui invente une connexion absente de Gamma | défaut bloquant de la garantie positive |
| flot Gabriel qui manque une connexion Gamma | divergence attendue à sérialiser comme `partial_refinement`, jamais `exact` |
| tour saturée différente de Gamma à une coupe ouverte ou fermée | contradiction à minimiser et fixture permanente avant toute optimisation |
| générateur saturé tronqué à $K_{\mathrm{eff}}+1$ | complétude fausse; interdire `exact` |
| forêt de générateurs seulement maximale, ou join d'intersections incomplet | interdire toute équivalence à Gamma et tout statut `exact` |
| attache différente de $\Gamma_k$ | `full_pi0` reste non certifié |
| lot égal dépendant de l'ordre | défaut bloquant |
| carré vertical non commutatif | défaut bloquant |
| dégénérescence hors preuve | `public_status=unsupported_degeneracy` sans forêt, ou `mode=budgeted, forest_semantics=partial_refinement, public_status=conditional` sans assertion d'absence |
| cellule enfant reconstruite non fermée | interdire `exact` et conserver violateurs, co-ties et amorce de contraintes |
| budget scratch insuffisant pour la transaction suivante | checkpoint durable puis `budget_exhausted` avant écriture |
| pic supérieur à 80 % de VRAM | activer le streaming avant d'agrandir |
| croissance intermédiaire forte en régime volumique | ouvrir la phase 17 |
| p95 50k supérieur ou égal à 100 ms | ne pas revendiquer le SLO principal; publier séparément si le seuil secondaire sous une seconde est atteint |
| p95 50k supérieur ou égal à une seconde | ne revendiquer ni le SLO principal ni l'objectif secondaire |
| checkpoint non reproductible | aucune campagne Spot massive |
| instance exactement ciblée non confirmée `TERMINATED` | incident opérationnel bloquant |

## 8. Discipline GCP pour chaque campagne

Tout agent qui utilise la G4 suit exactement :

1. vérifier projet, zone, quota et instance cible;
2. créer si nécessaire avec `deploy.sh`, durée GCE au plus huit heures;
3. avant tout démarrage non interactif, créer une clé SSH de session non chiffrée dans un chemin physique hors dépôt, la protéger localement, l'inscrire dans OS Login avec une expiration bornée par la durée GCE, vérifier ce profil et conserver l'échéance UTC absolue exacte;
4. démarrer uniquement avec `start_and_verify.sh`, en lui transmettant explicitement cette clé;
5. exiger une échéance GCE future dans la borne : `terminationTimestamp` s'il est exposé et cohérent, sinon `lastStartTimestamp + maxRunDuration` à partir d'une génération fraîche et d'une durée relue et bornée; si la validation tolère 300 secondes d'écart, retenir comme borne sûre la somme nominale diminuée de ces 300 secondes;
6. exiger que l'arrêt invité soit armé et lisible;
7. lancer le preflight Blackwell;
8. estimer temps, VRAM, RAM, scratch et sortie avant le benchmark;
9. fixer une échéance de travail laissant par défaut au moins trente minutes avant cette borne GCE sûre pour checkpoint, copie du manifeste et arrêt; une marge d'au moins quinze minutes est admise uniquement pour une campagne transactionnelle reprenable dont chaque unité dure au plus 240 secondes, avec checkpoint après chaque unité et vérification, copie et nettoyage bornés par le temps restant;
10. ne lancer aucune nouvelle unité après cette échéance et checkpoint avant l'échéance;
11. sur succès, échec ou interruption, exécuter `stop_and_verify.sh`;
12. exiger l'état `TERMINATED` de l'instance exactement ciblée, puis révoquer et supprimer la clé de session; transmettre la même échéance UTC absolue à chaque SSH/SCP pour interdire tout renouvellement ou réimport sans expiration; si l'arrêt reste non certifié avec sa génération, conserver la clé uniquement sous son échéance initiale pour la reprise ciblée;
13. inventorier et signaler les autres VM E-HGP actives sans bloquer la clôture, et ne jamais les arrêter, modifier ou supprimer sans autorisation explicite;
14. inscrire états initial et final dans le manifeste.

Une fermeture du terminal SSH, une préemption attendue ou la fin du processus ne prouve pas l'arrêt de la VM.

## 9. Format du compte rendu d'un agent

Chaque compte rendu de phase doit répondre dans cet ordre :

```text
Résultat
    phase, profil, statut obtenu

Preuves et tests
    commandes, oracles, tailles, graines, différences

Performance
    matériel, protocole, p50/p95, mémoire, compteurs

Limites
    hypothèses, dégénérescences, budgets, points ouverts

Artefacts
    fichiers, schémas, checkpoints et commit

GCP
    projet/zone/instance, état initial, coupe-circuits, état final TERMINATED

Prochaine porte
    condition précise avant la phase suivante
```

Si aucune VM n'a été utilisée, la section GCP le dit explicitement. Si l'état final ne peut être vérifié, l'agent arrête son compte rendu normal et fournit immédiatement les commandes de vérification et d'arrêt à l'utilisateur.

## 10. Première séquence recommandée

Les sept prochains lots de travail doivent être :

1. phase 0 : contrat v2, énoncé candidat M.1, obligations de preuve et schémas;
2. phase 1 : oracle exhaustif et générateur de fixtures;
3. phase 2A : prédicats CPU filtrés et fuzzing;
4. phase 3 : environnement CUDA reproductible;
5. phase 2B puis phase 4 : prédicats GPU et oracle spatial;
6. phase 5 : EMST contre Gabriel sur CPU de référence puis sur GPU;
7. phase 7 : spike Paragram isolé sur G4.

Cette séquence donne rapidement une vérité terrain, un cas $k=1$ incontestable et une mesure réaliste de la primitive GPU. Elle évite que les choix de bibliothèque ou de layout figent prématurément un objet mathématique incomplet.

Les phases 2A, 2B, 3 et 4 sont fermées et le jalon 17A reste prêt comme expérience CPU indépendante, sans déplacer la voie principale. L'oracle spatial brute-force exact et le premier Morton-LBVH à bornes rationnelles certifiées sont livrés sur `reference_cpu`; la référence CUDA exhaustive est qualifiée au SHA `01be0f150ee35a01bc939d9240b0a5675e3ae800`; le filtre CUDA borné de rejet AABB strict l'est au SHA `24e33d4fc80d2b5c687c939d9240fa50571d1951`; le parcours LBVH résident parallèle recertifié l'est au SHA `c846ed7b253840ef6fe1f0f39f7f10c63af64b8e`.

Les phases 9, 10 et 11 sont administrativement closes comme candidats conditionnels. La phase 14 reste `ready` avec `reference_cpu`, le profil `hgp_reduced`, le mode scientifique `certified`, le déploiement `architecture_only` et `public_status=not_claimed`, tandis que la phase 15 est `in_progress` sur la voie massive parallèle. La Phase 11 conserve `all_naturality_squares_replayed=false` et `vertical_maps_complete=false`; la complétude verticale directe, la fidélité globale des carriers, M.1, la voie CUDA produit complète et le protocole p95 restent nécessaires avant toute qualification finale.

La phase 5 demeure à l'état `ready` comme ancre $k=1$ ouverte; ses backends historiques restent `cuda_g4` pour les propositions et `reference_cpu` pour la recertification, la décision et la contraction. Ses cinq premiers jalons CPU livrent l'EMST exact, le catalogue global rang-deux/Gabriel, leur réduction, le différentiel indépendant à $n=14$, la forêt compacte à stockage linéaire et le producteur Borůvka exact vérifié sur le LBVH. La primitive GPU à graine fixe est qualifiée sur G4 au SHA `9f29ffcb9ba8ca66f3cfc7c0c9285c34cbeee70e`; la boucle hybride monolithique, son contexte producteur résident, son rejeu GPU indépendant et ses contractions CPU exactes le sont au SHA `c199651d86e861eb755357986d036889839578d4`; l'intégration chunkée de toutes les rondes, son budget de confiance, sa borne physique du payload candidat et son rejeu frais le sont au SHA `6d944132d2f7d261a934a1864788c2fb7a81831f`; la graine Morton bornée, sa recertification exacte monotone, ses audits et son rejeu indépendant le sont au SHA `7c4933b678cbc6d9860e33596522ab971c0c5df5`. Le benchmark Morton final au SHA propre `4cbdb2bb7f0fb9decc9ede1c9a313727cb8b93ed` établit empiriquement que la fenêtre fixe conserve un travail quadratique sur `uniform` et `clusters`; elle reste un initialiseur de cutoff. La recherche external-1NN exacte élimine ensuite le payload candidat et son benchmark G4 au SHA `a81d8e50e4655a2f1b6acad74bbffddbc98ff0ba`, étendu à 16 384 points, observe un dernier exposant entre $1.090$ et $1.240$ avec un maximum de 235 visites par source. Ce résultat reste empirique et `benchmark_only`. La réduction explicite locale du témoin recertifié en `K1CompactForest` est maintenant implémentée et vérifiée sous GCC et Clang stricts, sans modifier le statut du témoin source ni publier la tour globale. L'obstruction quadratique ferme négativement la preuve scalable pour la frontière indépendante par source actuelle. Le parcours self-dual alternatif est certifié sur hôte, neutralise ce témoin, borne sa pile par $2H+1$ et ses visites par $n(n+1)-1$; sa chaîne offre chaque graine recertifiée aux deux composantes incidentes, décide directement les minima par composante, reconstruit l'enveloppe après chaque contraction et exige un rejeu frais sans minima ponctuels. Le mode courant dédupliqué traite l'union des ancêtres mixtes une fois en ordre bottom-up et borne sa maintenance linéairement par baisse, sans borner le travail total; le work-profile hôte v5 est validé et se projette exactement vers v4, v3 et v1. La chaîne persistante reste `frozen_initial` sous son contrat v4, et aucune qualification CUDA/G4 ou modification de statut public n'en découle. Une amélioration globale sous-quadratique reste ouverte avec le passage au-delà de l'ancre $k=1$. En préparation de la Phase 6, toujours `ready`, les jalons 6.1 à 6.22 sont validés sur hôte avec `reference_cpu/certified`, `full_pi0` comme autorité Gamma et projection explicite vers `hgp_reduced`; 6.7 ferme les labels terminaux des $\lvert U\rvert$ bras, 6.8 construit la coupe Gamma strictement ouverte bornée, 6.9 raccorde les classes terminales complètes aux composantes pré-événement, 6.10 contracte exhaustivement et simultanément toutes les facettes et cofaces du niveau exact, 6.11 superpose les seules provenances événementielles fournies et complètes aux groupes exhaustifs, 6.12 construit le catalogue critique exhaustif borné et ses lots H0, 6.13 projette chaque transition vers la sémantique locale `hgp_reduced`, 6.14 déroule les niveaux d'un ordre dans un journal compact et 6.15 en rejoue les coupes; 6.16 raccorde les rôles H0 à l'histoire, 6.17 raccorde tous les bras catalogués aux cibles strictes `full_pi0`, 6.18 factorise ces objets dans un journal typé, 6.19 relie chaque cible à sa racine locale pré-lot ou à son singleton omis et 6.20 compose cette liaison avec chaque bras. Le jalon 6.21 reconstruit enfin un chemin strict compact et rejouable par candidat, sans dédupliquer les cibles ou racines partagées. Sa portée courante est `bounded_n14_k10_single_order_event_local_typed_critical_arms_with_replayable_strict_descent_paths_linked_to_external_full_pi0_targets_and_separate_frozen_pre_batch_local_hgp_reduced_root_or_omitted_singleton_dispositions_only` : les `Attachment`, identifiants publics ou durables, H5, O3, M.1, le DAG global, le pointer-jumping, les plateaux, l'attache verticale, la transaction de forêt `full_pi0`, la forêt multi-ordre et tout `public_status` restent ouverts.

À la date historique du jalon 6.18, la Phase 6 restait `ready` et la Phase 5 seule `in_progress`; le chemin demeurait `reference_cpu/full_pi0` projeté vers `hgp_reduced/certified`. Le registre courant a depuis placé la Phase 5 à `ready` et ouvert la Phase 9. Validé sur hôte strict sous GCC et Clang, il conserve une unique histoire mono-ordre 6.14 et y joint exhaustivement les quatre sémantiques de labels, les selles, classes terminales, bras et cibles strictes `full_pi0`, tandis que les deux overlays sources restent transitoires et que leurs annotations réduites restent non autoritatives. La portée courante devient `bounded_n14_k10_single_order_exhaustive_gamma_groups_typed_catalog_h0_roles_and_strict_full_pi0_arm_targets_with_separate_hgp_reduced_effect_annotations_only`; le lien cible--racine, les objets `Attachment`, les identifiants durables ou publics, l'attache verticale, M.1, le DAG global, le pointer-jumping, les plateaux, la forêt multi-ordre et tout `public_status` restent ouverts.

Le jalon 6.19 ferme maintenant ce lien cible--racine local sans changer l'état des phases. Le journal 6.18 reste externe et fraîchement recertifié; chaque cible `full_pi0` rejoint par égalité de famille complète l'unique racine `hgp_reduced` du snapshot pré-lot, ou devient un singleton explicitement omis. Les dix capacités, leur balayage exhaustif de domaine, l'affectation exactement-une-fois, les préparations bornées et le commit simultané sont validés sur hôte strict sous GCC et Clang. La portée courante devient `bounded_n14_k10_single_order_full_pi0_target_families_to_frozen_pre_batch_local_hgp_reduced_roots_with_explicit_isolated_singletons_only`; les identifiants restent locaux et aucun `Attachment`, identifiant durable ou public, attache verticale, certificat M.1, transaction de forêt `full_pi0`, DAG global, pointer-jumping, plateau, forêt multi-ordre ou `public_status` n'est encore publié.

Le jalon 6.20 compose ce raccord avec chacun des bras typés, toujours sans changer l'état des phases. Les sources 6.18 et 6.19 restent externes et sont recertifiées; un candidat dense conserve la clé événement-locale, la cible `full_pi0`, la disposition et l'éventuelle racine locale de chaque bras. Les cibles partagées ne dédupliquent jamais les bras. Le préflight récursif, le cap unique $A\leq5824$, les coutures, les diagnostics atomiques, `q2`, le terminal commun, les mutations et le nuage jumeau sont validés par builds stricts GCC et Clang et un CTest GCC ciblé. La portée courante devient `bounded_n14_k10_single_order_event_local_typed_critical_arms_to_strict_full_pi0_targets_and_frozen_pre_batch_local_hgp_reduced_root_or_explicit_omitted_singleton_candidates_only`; aucun chemin rejouable, `Attachment`, identifiant durable ou public, attache verticale, certificat M.1, transaction de forêt `full_pi0`, DAG global, pointer-jumping, plateau, forêt multi-ordre ou `public_status` n'est encore publié.

Le jalon 6.21 matérialise maintenant le chemin transitoire de 6.17 pour chaque candidat 6.20, toujours sans changer l'état des phases. Les trois sources restent externes et sont recertifiées; un catalogue frais et une famille 6.7 par selle suffisent ensuite à reconstruire le germe, la chaîne stricte et le terminal. Le payload compact garde les données analytiques rejouables, vérifie les deux facettes contre la cible `full_pi0` et recopie seulement la disposition et la racine locale séparée. Les cinq caps conservatives, les trois coutures, les diagnostics atomiques, les fixtures et les mutations sont validés par builds stricts GCC et Clang et un CTest GCC ciblé. La portée courante devient `bounded_n14_k10_single_order_event_local_typed_critical_arms_with_replayable_strict_descent_paths_linked_to_external_full_pi0_targets_and_separate_frozen_pre_batch_local_hgp_reduced_root_or_omitted_singleton_dispositions_only`; aucun `Attachment`, identifiant durable ou public, H5, O3, M.1, attache verticale, transaction de forêt `full_pi0`, DAG global, pointer-jumping, plateau, forêt multi-ordre ou `public_status` n'est encore publié.

Le jalon 6.22 ajoute maintenant les identités scientifiques durables sans changer l'état des phases. Les quatre sources 6.18--6.21 restent externes et sont recertifiées; les projections complètes donnent les vrais `CriticalEvent.event_id` v2 et chaque shell frais donne exactement un tuple `(event_id, order, removed_shell_id)` par bras, trié et relié à son chemin. SHA-256, les collisions sémantiques, les quatre caps, les quatre coutures, le cas `q2`, sa permutation, le miroir multi-événement et les mutations échouant fermées sont validés par targets stricts GCC et Clang et CTests GCC ciblés. La portée courante devient `bounded_n14_k10_single_order_v2_critical_event_ids_and_canonical_arm_identity_tuples_from_recertified_internal_replayable_paths_only`; aucun `attachment_id`, `Attachment`, `target_node_id`, lot égal-niveau, H5, O3, M.1, attache verticale, transaction de forêt `full_pi0`, DAG global, plateau, forêt multi-ordre ou `public_status` n'est encore publié.
