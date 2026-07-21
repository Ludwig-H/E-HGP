# Feuille de route d'implémentation de MorseHGP3D

> **Mission.** Construire un backend 3D hiérarchique, certifiable et GPU-first pour $1\leq k\leq10$, avec deux régimes produit non négociables : p95 `warm_e2e` strictement inférieur à une seconde autour de 50 000 points pour $K_{\max}\leq10$ sur les familles favorables enregistrées; puis streaming transactionnel, reprenable et honnêtement budgeté à dix millions de points ou davantage. Cette feuille de route est écrite comme un protocole exécutable par de futurs agents ChatGPT.

## 1. Règles de conduite pour tout agent

Avant toute modification, l'agent doit lire :

1. [`AGENTS.md`](../AGENTS.md), notamment les règles Git et GCP;
2. la [spécification](SPECIFICATION_MORSEHGP3D.md);
3. le [registre des preuves](math/STATUT_PREUVES_ET_HEURISTIQUES.md);
4. le [registre d'implémentation](implementation_status.toml), qui désigne la phase réellement ouverte;
5. la phase courante de cette feuille de route;
6. les tests et artefacts produits par les phases antérieures.

À chaque intervention, il doit :

- annoncer la phase et le profil affecté;
- indiquer si son code propose, certifie ou réduit;
- ne travailler que sur les artefacts de la phase;
- ajouter les tests avant de changer un statut scientifique;
- conserver un chemin CPU indépendant pour les différentiels;
- refuser toute troncature silencieuse;
- exécuter les validations proportionnées au risque;
- publier les compteurs, versions et hypothèses;
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

1. backend `reference_cpu`, Gamma exhaustif et profil `hgp_reduced` exact sur petits $n$, avec `full_pi0` séparément conditionnel;
2. flot Gabriel positif sur `cuda_g4`, explicitement `partial_refinement`, toujours comparé à Gamma aux tailles accessibles;
3. recherche et preuve d'une réduction complétée en incidences avant toute promotion exacte du backend GPU;
4. morphismes verticaux exacts de la référence et partiels de la voie accélérée;
5. après la phase 11, ouvrir en parallèle la piste topologique 12–13 (`full_pi0` puis dégénérescences) et la piste produit 14–16 (latence puis streaming de la voie réduite au statut effectivement prouvé);
6. réunir seulement les jalons dont les portes propres sont fermées;
7. étendre ensuite les dégénérescences du profil réduit ou complet sans bloquer l'autre piste;
8. après fermeture de la phase 2A, ouvrir sans bloquer la voie principale la phase 17A de tour saturée comme oracle exact borné; ses variantes scalables restent conditionnelles;
9. après migration contractuelle dédiée seulement, mode `budgeted` sensible à $H_0$ pour toute sous-famille de générateurs dont l'exhaustivité n'est pas certifiée.

Le profil réduit arrive avant le profil complet parce que Gamma exhaustif en donne une définition directement testable sur petits nuages. Le K-graphe de Gabriel brut n'en est plus une réalisation exacte autorisée. Le profil complet demeure l'objectif final, avec un statut séparé tant que ses attaches ne sont pas toutes certifiées.

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

Statut opérationnel : `in_progress`; proposition et rejeu indépendant `cuda_g4`, recertification, décision, contraction et réduction compacte locale `reference_cpu`, profil `hgp_reduced`, mode `certified`. La boucle hybride complète chunkée, son budget de confiance, le resserrement Morton borné avec cutoff exact monotone, leurs rejeux indépendants et le témoin EMST local sont qualifiés sur G4. Deux benchmarks séparés en mode `benchmark` distinguent le producteur Morton exhaustif, quadratique sur les familles uniformes et en amas, de la recherche external-1NN exacte sans payload candidat, mesurée jusqu'à 16 384 points avec des exposants empiriques entre 1,090 et 1,240 sur le dernier quadruplement. Ils ne revendiquent ni qualification, ni scalabilité, ni résultat scientifique. Une famille dyadique exacte montre que la frontière point--LBVH indépendante par source exige un travail $\Omega(n^2)$ dans le pire cas du modèle paramétrique. Un parcours self-dual partagé exact neutralise ce bloc sur hôte, ferme la partition des paires, borne sa pile par $2H+1$ et ses visites par $n(n+1)-1$, puis offre chaque graine recertifiée aux deux composantes incidentes avant de réduire directement les cutoffs. Après les enveloppes figée et sparse, le mode `exact_current_maximal_uniform_roots` partitionne les feuilles par racines uniformes maximales, les indexe par composante en CSR, lit tout nœud uniforme depuis le cutoff live et recalcule les maxima mixtes. Son enveloppe est pointwise au plus les enveloppes sparse, figée et dynamique par point; le parcours déterministe domine donc chacune sur visites, expansions, bornes AABB--AABB et distances exactes. Le nouveau mode hôte `exact_current_deduplicated_mixed_ancestors` conserve cette enveloppe mais traite une seule fois l'union des ancêtres mixtes : pour $r$ racines et une union $A$, il compte exactement $\lvert A\rvert+r-1$ découvertes, $r-1$ doublons et au plus $\lvert A\rvert$ recomputations. Cette borne est linéaire par baisse stricte, sans dominance temporelle ni borne globale sous-quadratique. La chaîne Borůvka locale conserve explicitement `frozen_initial` et son `proof_basis` v4. Le schéma hôte v5 ajoute désormais la voie courante dédupliquée au v4 et se projette exactement sur v4, v3 puis v1; sa fixture exerce un doublon positif sans réduction stricte des recomputations et ne change pas le pipeline G4. La réduction explicite ferme une `K1CompactForest` locale après rejeu frais du témoin historique; la qualification CUDA du parcours partagé, une amélioration sous-quadratique, les ordres supérieurs, les applications verticales et le statut public global restent ouverts.

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

Statut opérationnel : `ready`; backend préparatoire `reference_cpu`, profil d'autorité `full_pi0` projeté vers `hgp_reduced`, mode `certified`, portée `bounded_n14_k10_single_order_morse_minimum_saddle_partition_sweep_compared_to_exhaustive_gamma_at_every_activation_level_only`. La phase 5 reste l'unique phase `in_progress`; la porte d'entrée de la phase 6 est néanmoins satisfaite par les Phases 1 et 4 fermées. Les jalons préparatoires 6.1 à 6.6 certifient la miniball locale, son shell global, la famille top-$k$, l'arc canonique strict, son segment analytique, une chaîne issue d'une seule facette, puis le germe initial exact d'un bras critique d'indice un et son raccord. Le jalon 6.7 ferme tous les bras d'un même événement en classes d'identité de facettes terminales; 6.8 construit indépendamment la coupe exhaustive strictement ouverte de Gamma; 6.9 raccorde une famille complète à ses composantes strictement antérieures; 6.10 ferme la transition exhaustive et simultanée de $\Gamma_k^{<a}$ vers $\Gamma_k^{\leq a}$; 6.11 superpose les seules provenances événementielles fournies et complètes aux groupes qui les contiennent; 6.12 construit le catalogue critique exhaustif borné et ses lots H0; 6.13 projette chaque transition 6.10 vers la sémantique locale `hgp_reduced`; 6.14 déroule tous les niveaux exacts d'un ordre fixé dans un journal compact; 6.15 rejoue une coupe ouverte ou fermée depuis un préfixe entier de ce journal. Le jalon 6.16 reconstruit fraîchement 6.12 et 6.14 depuis le même nuage puis raccorde exhaustivement chaque rôle H0 à son slot Gamma fermé par niveau et `closed_point_ids`; 6.17 reconstruit ensuite toutes les familles de bras de toutes les selles d'un ordre et certifie leur composante stricte `full_pi0` par double lookup des facettes initiale et terminale, en gardant les annotations `hgp_reduced` séparées. Le jalon 6.18 réconcilie ces deux sources fraîchement vérifiées dans un unique journal mono-ordre typé. Le jalon 6.19 recertifie ce journal externe, rejoue son histoire une fois sur des snapshots atomiques et raccorde chaque cible non triviale à l'unique racine réduite locale pré-lot de même famille complète; une cible singleton est au contraire certifiée absente de toutes les racines et explicitement omise. Le kind réduit n'est contrôlé qu'après cette décision. Le jalon 6.20 compose ensuite chaque bras avec cette liaison sans nouvelle géométrie et sans dédupliquer les bras partageant une cible. Le jalon 6.21 reconstruit le catalogue puis une famille 6.7 par selle et conserve un chemin strict compact, exact et rejouable pour chaque candidat 6.20, avec sa cible `full_pi0` externe et sa disposition réduite séparée. Le jalon 6.22 calcule les vrais `event_id` v2 par projection scientifique complète et SHA-256 séparé par domaine, puis agrège canoniquement exactement un tuple durable `(event_id, order, removed_shell_id)` par chemin 6.21. Le jalon 6.23 rompt volontairement cette dépendance décisionnelle à Gamma : il construit d'abord une généalogie candidate depuis les seuls minima du catalogue et les terminaux complets 6.7, contracte ensemble toutes les selles d'un même niveau, puis utilise Gamma uniquement comme oracle postérieur à tous ses niveaux d'activation. L'objet reste un falsificateur interne : aucun `attachment_id`, `Attachment`, `target_node_id`, lot public, fermeture de H5, O3 ou M.1, attache verticale, DAG global ou forêt multi-ordre n'est publié, et aucun `public_status` n'est modifié.

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

La base est `exact_catalog_minima_and_strict_arm_terminal_hyperkruskal_partition_sweep_with_posterior_exhaustive_gamma_oracle_v1` et la portée `bounded_n14_k10_single_order_morse_minimum_saddle_partition_sweep_compared_to_exhaustive_gamma_at_every_activation_level_only`. Même un accord sur toutes les fixtures bornées reste un falsificateur logiciel, pas une preuve de O1, O3, O4 ou M.1 et pas une transaction publique `full_pi0`. Une contradiction devient une fixture minimale permanente et met à jour le registre des preuves avant toute optimisation. En l'absence de contradiction dans la campagne ciblée, la feuille de route cesse d'empiler des wrappers locaux de Phase 6 et pivote vers la Phase 7, l'audit de la primitive de puissance nécessaire aux phases de catalogue global; les identifiants publics, H5, M.1, la verticalité et la forêt multi-ordre demeurent des chantiers ultérieurs distincts.

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

Le chemin produit reste GPU-first : génération et filtrage massifs sur `cuda_g4`, décisions combinatoires ambiguës rejouées exactement sur l'hôte, transferts et matérialisation strictement budgetés. Les choix des Phases 8 à 13 doivent préserver les deux cibles de produit : p95 `warm_e2e` inférieur à une seconde pour environ 50 000 points et $K_{\max}\leq10$ sur les familles favorables préenregistrées, puis streaming transactionnel pour dix millions de points ou davantage avec sortie exacte lorsque le certificat reste sparse et arrêt budgétaire honnête sinon. Aucun élargissement d'un oracle de test CPU ne doit retarder ou remplacer cette voie.

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

## Phase 9 — Raffinement incrémental jusqu'au rang effectif

### But

Implémenter l'identité $C_{m+1}=\bigcup P(I,u)$ une profondeur intérieure à la fois.

Le lemme d'arrêt $H_0$ est impératif : pour un événement générique utile de support $U$ avec $\lvert U\rvert\geq2$ et de rang fermé $s\leq s_{\max}$, l'intérieur vérifie $m=\lvert I\rvert=s-\lvert U\rvert\leq m_{\star}$. Il faut fermer les parents $C_0$ à $C_{m_{\star}}$. À la dernière profondeur, on ferme les morceaux $P(I,u)$ et toutes les strates de leur diagramme restreint, mais on ne reconstruit aucun enfant $C_{m_{\star}+1}(Q)$ : un tel parent n'est nécessaire à aucun événement utile. Les facettes et ensembles fermés vivent dans leurs arènes hiérarchiques, distinctes des labels parents.

### Travaux par ordre $m$

1. charger les parents $C_m(I)$ fermés;
2. construire les diagrammes ordinaires de $X\setminus I$ restreints;
3. fermer chaque morceau aux sommets, réconcilier toutes les incidences naturelles du diagramme restreint et en extraire labels, strates candidates et contraintes globalement sûres;
4. si $m<m_{\star}$, émettre les enfants $Q=I\cup\lbrace u\rbrace$, trier et dédupliquer leurs labels sans unir leurs fragments géométriques;
5. si $m<m_{\star}$, reconstruire chaque cellule enfant canonique $C_{m+1}(Q)$ depuis la boîte paddée $\Omega$ et un sous-ensemble sûr de contraintes $Q\times(X\setminus Q)$;
6. à chaque sommet provisoire d'un tel enfant, comparer exactement le plus éloigné dans $Q$ au 1-NN dans $X\setminus Q$, enregistrer toutes les égalités actives, ajouter tous les violateurs, puis reclipper jusqu'à file vide;
7. si $m<m_{\star}$, programmer les cellules canoniques fermées comme parents de la profondeur suivante; si $m=m_{\star}$, ne créer aucune cellule enfant et conserver les morceaux fermés comme témoins terminaux des strates;
8. dédupliquer les strates candidates par clé exacte et les valider contre le diagramme restreint fermé, son parent canonique et l'oracle global de rang;
9. extraire les supports 2, 3 et 4, compter le rang et dédupliquer les événements;
10. checkpoint et publier les compteurs.

Les supports de taille un ont déjà été injectés en phase 5 et ne sont jamais recherchés dans les strates du raffinement.

### Jalons internes

- profondeur $m=0$ : sphères vides et premières selles $k=1$;
- profondeur $m=1$ : événements avec un point strictement intérieur;
- profondeur $m=3$ : validation des trois codimensions et première mesure de croissance;
- profondeur $m=5$ : répétition stable et streaming de cellules;
- profondeur $m=m_{\star}$ : supports de taille deux et rang $s_{\max}$, puis catalogue $H_0$ générique complet; pour $K_{\max}=10$ et $n\geq11$, $m_{\star}=9$ et $s_{\max}=11$.

### Compteurs

Pour chaque $m$, publier $M_m$ parents, $P_m$ morceaux, $V_m$ sommets, $J_m$ colonnes, $R_m$ reclippings, $O_m$ overflows, $C_{m,s}$ événements et pic VRAM.

### Tests

- catalogue complet contre l'oracle $n\leq14$;
- aucun événement supplémentaire;
- chaque événement possède centre, support, shell et rang identiques;
- invariance à l'ordre des parents et des propositions;
- comparaison ancrée–cascade barycentrique sur petites tailles;
- fermeture avec propositions locales volontairement incomplètes;
- pour $m<m_{\star}$, même label enfant découvert depuis deux, trois puis plusieurs parents ou chunks, avec reconstruction canonique identique à l'oracle;
- à $m=m_{\star}$, absence vérifiée de toute allocation ou propagation d'un $C_{m_{\star}+1}$, avec catalogue final identique à l'oracle;
- amorces de contraintes volontairement différentes, y compris l'amorce vide, donnant la même cellule fermée;
- tentative d'union géométrique des fragments désactivée dans la v2 et testée seulement comme prototype futur différentiel.

### Portes go/no-go

Après les profondeurs $\min(3,m_{\star})$, $\min(5,m_{\star})$ et $m_{\star}$, estimer l'exposant de croissance sur Poisson volumique. Si les intermédiaires deviennent nettement superlinéaires dans le régime favorable, suspendre les micro-optimisations et ouvrir la phase 17. Une configuration adversariale superlinéaire n'est pas un échec de correction.

## Phase 10 — Catalogue, Gamma et réduction candidate

### But

Livrer `hgp_reduced` exact sur le backend CPU de référence et une voie Gabriel GPU honnêtement partielle.

### Travaux

- construire Gamma exhaustif avec tous ses sommets, cofaces et incidences sur `reference_cpu`;
- convertir chaque événement de rang $k+1$ en label Gabriel $S=I\cup U$ pour la voie accélérée;
- émettre ses $k+1$ facettes et son hyperarête positive;
- vérifier le critère Gabriel par le rang sans l'assimiler à une preuve d'exhaustivité;
- trier les niveaux par filtre puis comparateur exact;
- construire les lots;
- implémenter hyper-Kruskal déterministe;
- traiter $q=0$ comme naissance réduite, $q=1$ comme croissance et $q\geq2$ comme multifusion;
- activer toutes les facettes du lot et conserver les `coverage_delta`;
- ne supprimer comme redondante qu'une hyperarête sans facette ni couverture nouvelle;
- restituer les unions de points recouvrantes;
- produire la forêt pour chaque $k$.

### Tests

- Gamma exhaustif contre oracle de phase 1;
- flot Gabriel contre Gamma comme sous-relation positive;
- fixture `gabriel-point-set-counterexample-5-points-v1`, avec désaccord exact attendu et sérialisé;
- composantes avant et après chaque lot;
- même résultat clique, étoile et hyperarête;
- événements redondants;
- multifusions et niveaux égaux;
- recouvrements de points;
- EMST pour $k=1$.

### Porte de sortie

Pour tous les petits cas et toutes les coupes, `hgp_reduced` sur `reference_cpu` coïncide avec la réduction de Gamma exhaustif et utilise `gamma_exhaustive_reference`. La voie Gabriel utilise `gabriel_positive_connectivity`, reste `partial_refinement` et ne peut fermer cette phase comme backend exact. Une réduction accélérée exacte exige une preuve nouvelle couvrant les incidences silencieuses.

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

Atteindre ou réfuter proprement la cible de moins d'une seconde en protocole `warm_e2e`.

### Préconditions

- profil `hgp_reduced` certifié;
- prédicats et catalogues corrects;
- aucun JIT;
- runtime et allocateur initialisés, mais nuage encore en mémoire hôte;
- instrumentation NVTX complète.

### Optimisations autorisées

- fusion de kernels sans fusionner proposition et certification;
- CUDA Graphs;
- classes de cellules par complexité;
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

Objectif principal : $n=50\,000$, $K_{\max}=10$, p95 `warm_e2e` inférieur à une seconde sur famille volumique préenregistrée, statut `exact` et pic inférieur à 80 % de VRAM. Cette mesure inclut validation, transfert, LBVH, calcul et matérialisation du résultat; `resident_core` reste diagnostique. Si la cible échoue, publier la phase dominante et la courbe de croissance; ne pas modifier le contrat.

## Phase 15 — Streaming à un million

### But

Traiter un catalogue plus grand que les arènes résidentes sans changer le résultat.

### Travaux

- planificateur de lots selon cinq budgets typés en octets ou secondes : device, RAM hôte, scratch, sortie et temps;
- sur G4, Hyperdisk Balanced pour le boot et les checkpoints durables, et jusqu'à quatre Titanium SSD pour le scratch éphémère rapide;
- instantané `BudgetSnapshot` avant chaque lot, run, merge et sérialisation;
- fermeture complète avant éviction;
- signatures d'incidence triables;
- labels et contraintes sûres sérialisés avant éviction; aucun état de couture géométrique n'est requis;
- runs checksummés;
- merge externe déterministe;
- sorties mémoire-mappées;
- checkpoint atomique par ordre;
- reprise après kill et préemption;
- estimation pessimiste du scratch avant lancement;
- garde transactionnelle : réserver simultanément ancien état, nouvelle écriture temporaire, pire espace de merge, espace du checkpoint et marge de sécurité; écrire, synchroniser, vérifier, renommer atomiquement, puis seulement supprimer l'ancien état.

### Tests

- résident contre streaming sur tailles communes;
- tailles de lots aléatoires;
- interruption à chaque frontière possible;
- corruption d'un run détectée;
- disque presque plein avant et pendant un merge, sans corruption de l'état durable;
- limites exactes de chaque budget à un octet près et sortie `budget_exhausted` reproductible;
- kill entre écriture, `fsync`, renommage et publication du manifeste;
- reprise avec version incompatible refusée;
- un million de points sans OOM.

### Porte de sortie

Résultats octet par octet identiques entre résident et streaming. Reprise déterministe après interruption. Objectif indicatif : un million en moins de 60 secondes lorsque le certificat reste sparse.

## Phase 16 — Plusieurs millions et campagne scientifique

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

Cette piste est parallèle et non bloquante. Sa première sous-phase s'ouvre seulement après fermeture des phases 1 et 2A; elle ne remplace ni le raffinement top-$m$ des phases 8–10, ni la preuve Morse M.1 de la phase 12. Elle exploite les théorèmes S.1–S.6 de l'[audit des boules saturées](math/TOUR_BOULES_SATUREES.md) comme représentation combinatoire directe de Čech et Gamma.

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

Il dépend de `v1-correctness` et exige en plus : G6 atteint à 50 000 points, G7 atteint à un million, au moins trois exécutions génériques sparse avec `forest_semantics=exact, public_status=exact` à trois millions sans OOM et avec reprise vérifiée, et une réponse contrôlée à dix millions. Cette dernière porte soit `forest_semantics=exact, public_status=exact` si le certificat reste sparse, soit `mode=budgeted, forest_semantics=partial_refinement` avec `public_status=conditional` ou `public_status=budget_exhausted`, checkpoint et diagnostic complet. Seul `public_status=exact` valide le SLO conditionnel de dix minutes.

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
| 8 | 1–4, 7 | 9 |
| 9 | 8 | 10 et baseline comparative de 17C |
| 10 | 5, 9 | 11, 14–16 |
| 11 | 10 | tour réduite publiable |
| 12 | 1, 6, 9–11 | piste topologique complète |
| 13 | 2A–2B, 12 | domaine non générique de la piste topologique |
| 14 | 10–11 | piste produit, SLO 50k |
| 15 | 9–11 | piste produit, million exact |
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
| p95 50k supérieur à une seconde | ne pas revendiquer le SLO |
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

Les phases 2A, 2B, 3 et 4 sont fermées et le jalon 17A reste prêt comme expérience CPU indépendante, sans déplacer la voie principale. L'oracle spatial brute-force exact et le premier Morton-LBVH à bornes rationnelles certifiées sont livrés sur `reference_cpu`; la référence CUDA exhaustive est qualifiée au SHA `01be0f150ee35a01bc939d9240b0a5675e3ae800`; le filtre CUDA borné de rejet AABB strict l'est au SHA `24e33d4fc80d2b5c687c939d9240fa50571d1951`; le parcours LBVH résident parallèle recertifié l'est au SHA `c846ed7b253840ef6fe1f0f39f7f10c63af64b8e`. La phase 5 est la phase courante, avec `cuda_g4` comme producteur de candidats et backend du rejeu indépendant, `reference_cpu` pour la recertification, la décision, la contraction et la réduction compacte locale, le profil `hgp_reduced` et le mode `certified`. Ses cinq premiers jalons CPU livrent l'EMST exact, le catalogue global rang-deux/Gabriel, leur réduction, le différentiel indépendant à $n=14$, la forêt compacte à stockage linéaire et le producteur Borůvka exact vérifié sur le LBVH. La primitive GPU à graine fixe est qualifiée sur G4 au SHA `9f29ffcb9ba8ca66f3cfc7c0c9285c34cbeee70e`; la boucle hybride monolithique, son contexte producteur résident, son rejeu GPU indépendant et ses contractions CPU exactes le sont au SHA `c199651d86e861eb755357986d036889839578d4`; l'intégration chunkée de toutes les rondes, son budget de confiance, sa borne physique du payload candidat et son rejeu frais le sont au SHA `6d944132d2f7d261a934a1864788c2fb7a81831f`; la graine Morton bornée, sa recertification exacte monotone, ses audits et son rejeu indépendant le sont au SHA `7c4933b678cbc6d9860e33596522ab971c0c5df5`. Le benchmark Morton final au SHA propre `4cbdb2bb7f0fb9decc9ede1c9a313727cb8b93ed` établit empiriquement que la fenêtre fixe conserve un travail quadratique sur `uniform` et `clusters`; elle reste un initialiseur de cutoff. La recherche external-1NN exacte élimine ensuite le payload candidat et son benchmark G4 au SHA `a81d8e50e4655a2f1b6acad74bbffddbc98ff0ba`, étendu à 16 384 points, observe un dernier exposant entre $1.090$ et $1.240$ avec un maximum de 235 visites par source. Ce résultat reste empirique et `benchmark_only`. La réduction explicite locale du témoin recertifié en `K1CompactForest` est maintenant implémentée et vérifiée sous GCC et Clang stricts, sans modifier le statut du témoin source ni publier la tour globale. L'obstruction quadratique ferme négativement la preuve scalable pour la frontière indépendante par source actuelle. Le parcours self-dual alternatif est certifié sur hôte, neutralise ce témoin, borne sa pile par $2H+1$ et ses visites par $n(n+1)-1$; sa chaîne offre chaque graine recertifiée aux deux composantes incidentes, décide directement les minima par composante, reconstruit l'enveloppe après chaque contraction et exige un rejeu frais sans minima ponctuels. Le mode courant dédupliqué traite l'union des ancêtres mixtes une fois en ordre bottom-up et borne sa maintenance linéairement par baisse, sans borner le travail total; le work-profile hôte v5 est validé et se projette exactement vers v4, v3 et v1. La chaîne persistante reste `frozen_initial` sous son contrat v4, et aucune qualification CUDA/G4 ou modification de statut public n'en découle. Une amélioration globale sous-quadratique reste ouverte avec le passage au-delà de l'ancre $k=1$. En préparation de la Phase 6, toujours `ready`, les jalons 6.1 à 6.22 sont validés sur hôte avec `reference_cpu/certified`, `full_pi0` comme autorité Gamma et projection explicite vers `hgp_reduced`; 6.7 ferme les labels terminaux des $\lvert U\rvert$ bras, 6.8 construit la coupe Gamma strictement ouverte bornée, 6.9 raccorde les classes terminales complètes aux composantes pré-événement, 6.10 contracte exhaustivement et simultanément toutes les facettes et cofaces du niveau exact, 6.11 superpose les seules provenances événementielles fournies et complètes aux groupes exhaustifs, 6.12 construit le catalogue critique exhaustif borné et ses lots H0, 6.13 projette chaque transition vers la sémantique locale `hgp_reduced`, 6.14 déroule les niveaux d'un ordre dans un journal compact et 6.15 en rejoue les coupes; 6.16 raccorde les rôles H0 à l'histoire, 6.17 raccorde tous les bras catalogués aux cibles strictes `full_pi0`, 6.18 factorise ces objets dans un journal typé, 6.19 relie chaque cible à sa racine locale pré-lot ou à son singleton omis et 6.20 compose cette liaison avec chaque bras. Le jalon 6.21 reconstruit enfin un chemin strict compact et rejouable par candidat, sans dédupliquer les cibles ou racines partagées. Sa portée courante est `bounded_n14_k10_single_order_event_local_typed_critical_arms_with_replayable_strict_descent_paths_linked_to_external_full_pi0_targets_and_separate_frozen_pre_batch_local_hgp_reduced_root_or_omitted_singleton_dispositions_only` : les `Attachment`, identifiants publics ou durables, H5, O3, M.1, le DAG global, le pointer-jumping, les plateaux, l'attache verticale, la transaction de forêt `full_pi0`, la forêt multi-ordre et tout `public_status` restent ouverts.

Le jalon 6.18 prolonge cette progression sans changer l'état des phases : Phase 6 reste `ready`, Phase 5 reste seule `in_progress`, et le chemin demeure `reference_cpu/full_pi0` projeté vers `hgp_reduced/certified`. Validé sur hôte strict sous GCC et Clang, il conserve une unique histoire mono-ordre 6.14 et y joint exhaustivement les quatre sémantiques de labels, les selles, classes terminales, bras et cibles strictes `full_pi0`, tandis que les deux overlays sources restent transitoires et que leurs annotations réduites restent non autoritatives. La portée courante devient `bounded_n14_k10_single_order_exhaustive_gamma_groups_typed_catalog_h0_roles_and_strict_full_pi0_arm_targets_with_separate_hgp_reduced_effect_annotations_only`; le lien cible--racine, les objets `Attachment`, les identifiants durables ou publics, l'attache verticale, M.1, le DAG global, le pointer-jumping, les plateaux, la forêt multi-ordre et tout `public_status` restent ouverts.

Le jalon 6.19 ferme maintenant ce lien cible--racine local sans changer l'état des phases. Le journal 6.18 reste externe et fraîchement recertifié; chaque cible `full_pi0` rejoint par égalité de famille complète l'unique racine `hgp_reduced` du snapshot pré-lot, ou devient un singleton explicitement omis. Les dix capacités, leur balayage exhaustif de domaine, l'affectation exactement-une-fois, les préparations bornées et le commit simultané sont validés sur hôte strict sous GCC et Clang. La portée courante devient `bounded_n14_k10_single_order_full_pi0_target_families_to_frozen_pre_batch_local_hgp_reduced_roots_with_explicit_isolated_singletons_only`; les identifiants restent locaux et aucun `Attachment`, identifiant durable ou public, attache verticale, certificat M.1, transaction de forêt `full_pi0`, DAG global, pointer-jumping, plateau, forêt multi-ordre ou `public_status` n'est encore publié.

Le jalon 6.20 compose ce raccord avec chacun des bras typés, toujours sans changer l'état des phases. Les sources 6.18 et 6.19 restent externes et sont recertifiées; un candidat dense conserve la clé événement-locale, la cible `full_pi0`, la disposition et l'éventuelle racine locale de chaque bras. Les cibles partagées ne dédupliquent jamais les bras. Le préflight récursif, le cap unique $A\leq5824$, les coutures, les diagnostics atomiques, `q2`, le terminal commun, les mutations et le nuage jumeau sont validés par builds stricts GCC et Clang et un CTest GCC ciblé. La portée courante devient `bounded_n14_k10_single_order_event_local_typed_critical_arms_to_strict_full_pi0_targets_and_frozen_pre_batch_local_hgp_reduced_root_or_explicit_omitted_singleton_candidates_only`; aucun chemin rejouable, `Attachment`, identifiant durable ou public, attache verticale, certificat M.1, transaction de forêt `full_pi0`, DAG global, pointer-jumping, plateau, forêt multi-ordre ou `public_status` n'est encore publié.

Le jalon 6.21 matérialise maintenant le chemin transitoire de 6.17 pour chaque candidat 6.20, toujours sans changer l'état des phases. Les trois sources restent externes et sont recertifiées; un catalogue frais et une famille 6.7 par selle suffisent ensuite à reconstruire le germe, la chaîne stricte et le terminal. Le payload compact garde les données analytiques rejouables, vérifie les deux facettes contre la cible `full_pi0` et recopie seulement la disposition et la racine locale séparée. Les cinq caps conservatives, les trois coutures, les diagnostics atomiques, les fixtures et les mutations sont validés par builds stricts GCC et Clang et un CTest GCC ciblé. La portée courante devient `bounded_n14_k10_single_order_event_local_typed_critical_arms_with_replayable_strict_descent_paths_linked_to_external_full_pi0_targets_and_separate_frozen_pre_batch_local_hgp_reduced_root_or_omitted_singleton_dispositions_only`; aucun `Attachment`, identifiant durable ou public, H5, O3, M.1, attache verticale, transaction de forêt `full_pi0`, DAG global, pointer-jumping, plateau, forêt multi-ordre ou `public_status` n'est encore publié.

Le jalon 6.22 ajoute maintenant les identités scientifiques durables sans changer l'état des phases. Les quatre sources 6.18--6.21 restent externes et sont recertifiées; les projections complètes donnent les vrais `CriticalEvent.event_id` v2 et chaque shell frais donne exactement un tuple `(event_id, order, removed_shell_id)` par bras, trié et relié à son chemin. SHA-256, les collisions sémantiques, les quatre caps, les quatre coutures, le cas `q2`, sa permutation, le miroir multi-événement et les mutations échouant fermées sont validés par targets stricts GCC et Clang et CTests GCC ciblés. La portée courante devient `bounded_n14_k10_single_order_v2_critical_event_ids_and_canonical_arm_identity_tuples_from_recertified_internal_replayable_paths_only`; aucun `attachment_id`, `Attachment`, `target_node_id`, lot égal-niveau, H5, O3, M.1, attache verticale, transaction de forêt `full_pi0`, DAG global, plateau, forêt multi-ordre ou `public_status` n'est encore publié.
