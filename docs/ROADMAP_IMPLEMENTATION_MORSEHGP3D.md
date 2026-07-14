# Feuille de route d'implémentation de MorseHGP3D

> **Mission.** Construire un backend 3D hiérarchique, certifiable et GPU-friendly pour $1\leq k\leq10$, rapide sur quelques milliers à quelques dizaines de milliers de points et capable de traiter plusieurs millions de points par streaming. Cette feuille de route est écrite comme un protocole exécutable par de futurs agents ChatGPT.

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

1. backend `reference_cpu`, profil `full_pi0`, sur petits $n$;
2. mode `certified`, profil `hgp_reduced`, sur `cuda_g4`, toujours comparé à `reference_cpu` aux tailles accessibles;
3. morphismes verticaux du profil réduit;
4. après la phase 11, ouvrir en parallèle la piste topologique 12–13 (`full_pi0` puis dégénérescences) et la piste produit 14–16 (latence puis streaming du profil réduit);
5. réunir seulement les jalons dont les portes propres sont fermées;
6. étendre ensuite les dégénérescences du profil réduit ou complet sans bloquer l'autre piste;
7. mode `budgeted` sensible à $H_0$.

Le profil réduit arrive avant le profil complet parce qu'il est directement garanti par le K-graphe de Gabriel du manuscrit. Le profil complet demeure l'objectif final, avec un statut séparé tant que ses attaches ne sont pas toutes certifiées.

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

Aucune phrase ne doit confondre profil réduit et profil complet. Chaque statut public est calculable à partir des champs du certificat. M.1 reste un contrat cible jusqu'à la preuve de la phase 12. Aucun code CUDA avant cette porte.

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
- égalité réduit–Gabriel sur les composantes non triviales.

### Porte de sortie

Toutes les fixtures et au moins $10\,000$ petits nuages aléatoires par dimension affine passent. Toute différence produit automatiquement un fichier minimal reproductible.

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

### But

Construire l'unique index global utilisé par l'énumération, les rangs et les descentes.

### Travaux

- validation des coordonnées;
- canonisation des identifiants et détection exacte des doublons;
- Morton codes et LBVH;
- 1-NN exact avec exclusion d'au plus $m_{\star}$ IDs;
- top-$k$ exact et shell complet jusqu'au rang $s_{\max}$;
- comptage intérieur d'une sphère avec rejet dès que le rang dépasse $s_{\max}$;
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

### But

Valider la première hiérarchie avant tout ordre supérieur.

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
- jusqu'à plusieurs millions de points pour l'EMST seul.

### Porte de sortie

Les coupes, niveaux et multifurcations des deux voies sont identiques après canonisation. Cette porte bloque toute revendication sur $k>1$.

## Phase 6 — Miniballs et descentes

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

## Phase 7 — Audit de la primitive de puissance

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

### But

Fermer exactement le diagramme ordinaire dans la boîte $\Omega$ et éprouver le lemme de séparation aux sommets.

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
- site caché gagnant seulement dans l'intérieur provisoire;
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
- tentative d'union géométrique des fragments désactivée dans la v1 et testée seulement comme prototype futur différentiel.

### Portes go/no-go

Après les profondeurs $\min(3,m_{\star})$, $\min(5,m_{\star})$ et $m_{\star}$, estimer l'exposant de croissance sur Poisson volumique. Si les intermédiaires deviennent nettement superlinéaires dans le régime favorable, suspendre les micro-optimisations et ouvrir la phase 17. Une configuration adversariale superlinéaire n'est pas un échec de correction.

## Phase 10 — Catalogue, Gabriel et `hgp_reduced`

### But

Livrer le premier profil mathématiquement certifié de bout en bout.

### Travaux

- convertir chaque événement de rang $k+1$ en label $S=I\cup U$;
- émettre ses $k+1$ facettes et son hyperarête;
- vérifier le critère Gabriel par le rang;
- trier les niveaux par filtre puis comparateur exact;
- construire les lots;
- implémenter hyper-Kruskal déterministe;
- traiter $q=0$ comme naissance réduite, $q=1$ comme croissance et $q\geq2$ comme multifusion;
- activer toutes les facettes du lot et conserver les `coverage_delta`;
- ne supprimer comme redondante qu'une hyperarête sans facette ni couverture nouvelle;
- restituer les unions de points recouvrantes;
- produire la forêt pour chaque $k$.

### Tests

- K-graphe de Gabriel exhaustif contre production;
- composantes avant et après chaque lot;
- même résultat clique, étoile et hyperarête;
- événements redondants;
- multifusions et niveaux égaux;
- recouvrements de points;
- EMST pour $k=1$.

### Porte de sortie

Pour tous les petits cas et toutes les coupes, `hgp_reduced` coïncide avec le théorème 5. Le certificat prouve catalogue complet, niveaux exacts, lots complets et absence de dégénérescence hors contrat.

## Phase 11 — Tour verticale

### But

Transformer dix forêts en une hiérarchie ordre–échelle cohérente.

### Travaux

- implémenter `locate_reduced_root(k,Q,a)` par remplacement intrus–support strictement descendant;
- choisir canoniquement l'intrus et le support, enregistrer chaque facette partagée et vérifier $\beta(Q')<\beta(Q)$;
- traiter par pointer-jumping les chaînes indépendantes jusqu'à un simplexe de Gabriel;
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

Chaque image est unique et tous les carrés commutent. Le périmètre réduit de la flèche est explicitement marqué lorsque la source ou la cible isolée n'appartient pas au profil.

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

## Phase 17 — Oracle direct sensible à $H_0$

### Déclencheur

Cette phase s'ouvre si les compteurs de la phase 9 montrent que fermer les cellules top-$m$ domine largement la taille du catalogue utile.

### Question

Construire un branch-and-bound qui certifie qu'une région ne contient aucun centre bien centré de rang au plus onze, sans produire toutes les cellules d'ordre intermédiaire.

### Pistes autorisées

- bornes de rang sur nœuds BVH;
- cônes de directions et tests de convexité;
- séparation par supports de taille au plus quatre;
- DTM ou entropie pour ordonner la recherche;
- candidats ANN avec reranking;
- bornes probabilistes uniquement pour le mode budgété.

### Exigence

Une région ne peut être supprimée en mode exact que par une borne déterministe globale. Jusqu'à preuve de complétude, cette phase utilise `mode=budgeted, forest_semantics=partial_refinement` et retourne `public_status=conditional` ou `public_status=budget_exhausted`; elle se compare au catalogue exact sur toutes les tailles accessibles.

### Porte de promotion

Zéro événement manquant sur l'oracle et la campagne exacte ne suffit pas : une preuve de couverture des régions est requise avant promotion dans `certified`.

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
- `backend=cuda_g4, mode=certified, profile=hgp_reduced` stable et capable d'obtenir `public_status=exact` sur le domaine générique;
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
| 1 | 0 | 5, 6, 8–13 |
| 2A | 0–1 | 2B, 4–13 |
| 2B | 2A, 3 | 4–13 GPU |
| 3 | schémas de 0 | GPU 4–16 |
| 4 | 2A–2B, 3 | 6, 8–9 |
| 5 | 1–4 | revendications hiérarchiques GPU |
| 6 | 1–4 | 12 |
| 7 | 2B–4 | 8–9 |
| 8 | 1–4, 7 | 9 |
| 9 | 8 | 10, 17 |
| 10 | 5, 9 | 11, 14–16 |
| 11 | 10 | tour réduite publiable |
| 12 | 1, 6, 9–11 | piste topologique complète |
| 13 | 2A–2B, 12 | domaine non générique de la piste topologique |
| 14 | 10–11 | piste produit, SLO 50k |
| 15 | 9–11 | piste produit, million exact |
| 16 | 14–15 | domaine pratique |
| 17 | profils de 9 et 16 | backend direct futur |
| 18 | phases livrées | release |

## 7. Portes go/no-go globales

| observation | décision obligatoire |
|---|---|
| un prédicat diffère de la référence | arrêt de la phase et réduction du cas |
| un overflow tronque une cellule | défaut bloquant |
| top-$k$, shell ou rang incomplet | défaut bloquant |
| événement manquant sur l'oracle | défaut bloquant |
| profil réduit différent de Gabriel exhaustif | défaut mathématique ou logiciel bloquant |
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
3. démarrer uniquement avec `start_and_verify.sh`;
4. exiger un `terminationTimestamp` futur dans la borne;
5. exiger que l'arrêt invité soit armé et lisible;
6. lancer le preflight Blackwell;
7. estimer temps, VRAM, RAM, scratch et sortie avant le benchmark;
8. fixer une échéance de travail laissant au moins trente minutes avant le coupe-circuit GCE pour checkpoint, copie du manifeste et arrêt;
9. ne lancer aucune nouvelle unité après cette échéance et checkpoint avant l'échéance;
10. sur succès, échec ou interruption, exécuter `stop_and_verify.sh`;
11. exiger l'état `TERMINATED` de l'instance exactement ciblée;
12. inventorier et signaler les autres VM E-HGP actives sans bloquer la clôture, et ne jamais les arrêter, modifier ou supprimer sans autorisation explicite;
13. inscrire états initial et final dans le manifeste.

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

1. phase 0 : énoncé candidat M.1, obligations de preuve et schémas;
2. phase 1 : oracle exhaustif et générateur de fixtures;
3. phase 2A : prédicats CPU filtrés et fuzzing;
4. phase 3 : environnement CUDA reproductible;
5. phase 2B puis phase 4 : prédicats GPU et oracle spatial;
6. phase 5 : EMST contre Gabriel sur CPU de référence puis sur GPU;
7. phase 7 : spike Paragram isolé sur G4.

Cette séquence donne rapidement une vérité terrain, un cas $k=1$ incontestable et une mesure réaliste de la primitive GPU. Elle évite que les choix de bibliothèque ou de layout figent prématurément un objet mathématique incomplet.
