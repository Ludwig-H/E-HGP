# E-HGP — MorseHGP3D

MorseHGP3D construit des hiérarchies 3D multi-ordres sans matérialiser la mosaïque de Delaunay d'ordre supérieur. Le dépôt sépare la source géométrique HGP sur les simplexes, la réduction aval en une hiérarchie laminaire de points et les rendus plats de clustering.

L'objet scientifique implémenté est le HGP-Clusterer du [manuscrit de thèse de Louis Hauseux](docs/references/MANUSCRIT_THESE_HAUSEUX.pdf), *Utilisation de graphes pour la classification et l'extraction de structures. Généralisation à des interactions d'ordre supérieur* (copie auteur du 5 juillet 2026). La Partie I, « du Single-Linkage à ses fondements », chapitres 2 à 5, pages PDF 35 à 76, fixe le cadre Single-Linkage de référence; la Partie II, « La généralisation du Single-Linkage avec des interactions d'ordre supérieur », chapitres 6 à 9, pages PDF 77 à 134, définit les K-polyèdres du complexe de Čech, la hiérarchie HGP et sa pratique. Ces deux parties sont la définition normative de l'objet source de ce dépôt.

> [!IMPORTANT]
> État courant : la Phase 15 reste `backend=reference_cpu`, `profile=hgp_reduced`, `mode=budgeted`, `deployment_status=architecture_only`, `public_status=not_claimed`. Son réducteur aval utilise le mode `exact_relative_multi_order_laminar_point_projection_v1` : il est disponible et testé relativement à une tour de $T_1$ à $T_K$ déclarée complète et exacte par son producteur, puis liée à son payload par reçus. Le réducteur n'authentifie pas cette vérité amont. Le producteur géométrique complet de la tour, sa qualification GCP et les capacités 50 000 ou 10 000 001 points ne sont pas terminés. Aucun benchmark ne promeut ce statut.

## Chantier actif : morsehgp3D_v7

Le chantier actif est [`morsehgp3D_v7/`](morsehgp3D_v7/README.md), sur `main` : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Il reprend la v6 avec un port épinglé et ses propres qualifications. La Phase 15, l'API de points et les mesures anciennes décrites plus bas restent celles de la ligne enregistrée ; elles ne sont pas des résultats v7.

L'[audit mathématique courant](morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md) ferme sous régularité la suffisance d'un certificat HGP FULL : minima Gabriel de cardinal K et vraies multifusions de cardinal K+1, avec parents résolus. Il n'est pas nécessaire de conserver tous les niveaux Gamma. Les portails restent indispensables pour reconstruire les bons parents ; leur producteur compact reste à qualifier. FULL conserve le recouvrement des points, contrairement à une projection ponctuelle laminaire. Les sources F du moteur réduit sont conservées comme témoins, sans promotion en producteur FULL.

Le [contrat v7](morsehgp3D_v7/docs/CONTRAT_PERFORMANCE.md) impose d'abord l'optimisation mono-thread, puis multi-CPU et GPU : **50 000 points, toute la tour K=1..10 sous une seconde**, repli K=1..5 si nécessaire, puis cible 100 ms une fois ce premier jalon atteint. Les comparaisons WSPD s=8/10/12 et les nuages de plusieurs dizaines de millions sur G4 restent requis. **Aucun de ces contrats de bout en bout n'est atteint** ; voir la [passation v7](morsehgp3D_v7/PASSATION.md) pour les mesures et refus récents.

## API de hiérarchie de points

L'en-tête public [`morsehgp3d/morsehgp3d.hpp`](morsehgp3d/include/morsehgp3d/morsehgp3d.hpp) et la cible CMake `morsehgp3d::morsehgp3d` exposent une seule voie aval :

1. recevoir les forêts horizontales de tous les ordres, leurs coutures verticales et les simplexes projectables avec leurs reçus;
2. ordonner exactement les niveaux de densité avec l'exposant rationnel positif `exp_z`;
3. distribuer les contributions simplexe--point selon `inverse_radius` ou `uniform`, puis appliquer des poids rationnels entre ordres;
4. construire le merge tree multi-ordres et router chaque point une seule fois, de façon descendante et irréversible;
5. produire une coupe `lambda_cut`, une coupe de rayon `dbscan_radius` ou une sélection `excess_of_mass` de type HDBSCAN.

Chaque point possède un terminal unique. Les clusters d'une coupe ou d'une sélection forment donc une antichaîne et sont deux à deux disjoints; un point ne peut pas recevoir deux étiquettes. Aucun argument `splitting` n'est présent dans le cœur.

La fonction `build_exact_point_hierarchy` refuse une source déclarée surrogate ou incomplète, une déclaration d'exactitude absente et un payload incohérent avec son identifiant. L'appelant peut recalculer cet identifiant : ce contrôle lie le contenu, mais n'authentifie pas la vérité scientifique de la déclaration amont. Le reçu annonce seulement `exact_reduction_of_bound_payload=true`; l'autorité scientifique de la tour n'est pas rejouée et `public_exact_status_claimed` reste faux.

## Architecture active

Le chemin produit amont vise une source sparse exacte : catalogue multi-ordre des paires de rang fermé utile, frontière indépendante des triangles aigus, frontière des tétraèdres bien centrés, incidences silencieuses, forêts horizontales et applications verticales. Il évite les catalogues globaux de cellules, cofaces et incidences; les oracles exhaustifs restent bornés et hors du chemin produit.

Le nouveau module de points ne remplace pas cette source. Il consomme une tour sous autorité externe et n'invente aucune complétude au moyen d'un MST de points, d'un graphe de voisinage ou d'une approximation numérique.

## Exactitude, tests et performances

- La [présentation mathématique](docs/math/HIERARCHIE_DE_POINTS_MULTI_ORDRES.md) définit les niveaux multi-ordres, les poids, le canal `stay`, la laminarité et les trois rendus.
- Le [plan de validation](docs/TEST_PLAN_MORSEHGP3D.md) distingue les fixtures du réducteur, l'unique comparaison comportementale `morsehgp3d.point_hierarchy_sklearn_differential` sur neuf points et les preuves de la source.
- Le [rapport de performances](docs/PERFORMANCE_MORSEHGP3D.md) donne les mesures historiques avec leur provenance et leur périmètre exact, puis le protocole qui devra qualifier 50 000, 1 000 000, 10 000 001 et 30 000 000 points.

À ce jour, la tentative HGP de référence à 50 000 points est censurée après au moins 300,000014 s sans hiérarchie complète. Les mesures à 10 M et 30 M concernent seulement une frontière partielle de composant. Le p95 historique de 95,791070 ms appartient à un point-MST rejeté et archivé; ce n'est pas une mesure de MorseHGP3D.

### Écart historique au contrat 50 000 points — ligne enregistrée

Trois postes le composent, et un seul domine. Le détail et ses certificats sont dans le [rapport de session du 8 août 2026](docs/research/RAPPORT_SESSION_20260808.md); le tableau se lit en secondes sur 48 cœurs contre un contrat de 1 s, à $K=5$.

| poste | état | écart |
|---|---|---:|
| étage paire, chemin device | diagnostic direct frais : frontière seule 2,396 s, processus froid 3,928 s; aucune classification exacte aval | **au moins 2,69 ×** avant l'aval exact |
| étage higher, coût unitaire | 204,78 → 25,49 µs par visite, à sortie bit-à-bit identique | **1,18 ×** |
| étage higher, génération arité 3 | 12,02 candidats par record à 50 000 points, régime certifié | 12,4 s |
| étage higher, génération arité 4 | aucun test géométrique dans la boucle; ~2 300 candidats par record | ~2 083 s |
| **aval, fermeture de descente de facette** | **non traité** | **$1{,}2\cdot10^{6}$ ×** |

Deux réserves normatives accompagnent ces chiffres. Aucune mesure higher à 50 000 points n'est complète : toutes sont censurées par un garde opérationnel. Cinq artefacts historiques `scale_probe.v1` portent cependant un placeholder d'arité quatre jamais exécuté avec `completeness_guaranteed=true`; ce champ ne certifie rien et les artefacts restent immuables. Le schéma v2 ajoute `applicable`, `executed` et `floating_rejections_certified`. Le dernier reste faux : l'implémentation actuelle n'a pas encore d'intervalles extérieurs et de repli exact pour tous ses rejets `binary64`. La chaîne de reprise reste elle aussi volontairement non scellable tant qu'elle ne porte ni curseur contigu des paires, ni identité entre audit et payload. Enfin, la sélectivité dépend de la famille : sur `balanced_multiscale_clusters`, l'une des trois familles de la porte P0, la borne tangente ne mord pas du tout.

### Piste sparse RNG--Jung

L'[audit mathématique exact](docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md) distingue l'objet sparse souhaité de deux raccourcis invalides. Un RNG ponctuel suivi de la cascade bornée $\alpha_2$ puis $\alpha_3$ depuis la plus grande arête incidente reste incomplet sur une fixture rationnelle de rang fermé 11, même avec la règle généreuse au maximum des extrémités. Continuer cette règle jusqu'au point fixe récupère la fixture mais peut propager les grandes échelles et n'a aucune borne sparse prouvée. Le surgraphe local $G_\tau$ est en revanche complet si ses rayons sont majorés par un certificat de rang; il se représente par un CSR de points et ne matérialise aucune mosaïque de Delaunay d'ordre supérieur.

Pour l'arité quatre, une paire diamètre transforme chaque troisième point en un demi-plan dans un disque de Jung. Le rang fermé 11 limite les centres recherchés aux profondeurs zéro à sept : une ancre ayant $m$ droites produit au plus $8m$ sommets candidats, contre $\binom{m}{2}$ actuellement. La construction théorique coûte $O(m\log m)$ en temps espéré par ancre à rang fixé. La complexité globale reste conditionnelle au nombre d'ancres et à la somme de leurs voisinages; au pire elle peut redevenir cubique, puis quartique si $K$ croît avec $n$. Le [diagnostic direct G4 à 50 k](docs/validation/phase15_rng_jung_g4_20260808/RESULTATS.md) explique pourquoi la frontière GPU reste à 2,396 s et pourquoi la sonde higher de 120 s est en réalité séquentielle sur CPU. Aucun contrat G4 n'est revendiqué.

La sous-porte `P15-HOCUDA-P0` construit maintenant la première primitive CUDA de ce chemin : un range-report LBVH `count--scan--emit` produit des cordes de Jung en CSR résident, marque séparément les carriers compatibles avec la paire diamètre et conserve toute ambiguïté `binary64` en `fail_open`. Elle ne matérialise ni matrice paire--point, ni cliques, ni cellules, cofaces ou mosaïque de Delaunay d'ordre supérieur. Elle reste toutefois `proposal_only` : ses ancres Morton bornées ne sont pas complètes, aucun niveau peu profond ni support terminal n'est encore produit, et sa capacité explicite interdit toute revendication du contrat 50 k.

La trajectoire historique 50 k place devant ce CSR un self-join LBVH par blocs. Un recouvrement fixe de la boîte des centres de Jung permet de certifier huit témoins strictement intérieurs par patch au rang fermé 11 et de rejeter un bloc entier de paires; une borne ambiguë subdivise au lieu de rejeter. Dans le régime favorable, le travail dépend du nombre de blocs, d'ancres résiduelles et de lignes actives plutôt que des $\binom{n}{2}$ paires; le pire cas dense reste possible et doit finir en résidu budgétaire, jamais en mosaïque globale. Cette ligne enregistrée visait d'abord le p95 `warm_e2e` sous 100 ms. La priorité demandée pour la v7 est désormais une seconde, puis 100 ms ; dans les deux cas, un résultat de composant isolé ne suffit pas.

## Construction locale

```bash
cmake -S morsehgp3d -B build/morsehgp3d -DMORSEHGP3D_BUILD_TESTS=ON
cmake --build build/morsehgp3d --parallel
ctest --test-dir build/morsehgp3d --output-on-failure
python tools/check_docs.py
python tools/check_implementation_status.py
```

## Lire le dépôt

1. Les Parties I et II du [manuscrit](docs/references/MANUSCRIT_THESE_HAUSEUX.pdf), pages PDF 35 à 134, définissent l'objet HGP source.
2. La [spécification](docs/SPECIFICATION_MORSEHGP3D.md) fixe les profils et statuts publics.
3. La [hiérarchie de points multi-ordres](docs/math/HIERARCHIE_DE_POINTS_MULTI_ORDRES.md) fixe l'API aval exacte-relative.
4. Le [registre des preuves](docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md), la [roadmap](docs/ROADMAP_IMPLEMENTATION_MORSEHGP3D.md), le [plan de tests](docs/TEST_PLAN_MORSEHGP3D.md) et l'[état des phases](docs/implementation_status.toml) portent l'autorité opérationnelle.
5. L'[index documentaire](docs/README.md) relie les contrats, preuves, validations et archives.

## Archives et sécurité GCP

Les voies falsifiées sont recensées dans [`docs/archive/abandoned/`](docs/archive/abandoned/README.md). Le point-MST surrogate est isolé sous [`morsehgp3d/archive/surrogates/point_mst_v6/`](morsehgp3d/archive/surrogates/point_mst_v6/README.md) et les prototypes non livrés sous [`morsehgp3d/archive/obsolete/`](morsehgp3d/archive/obsolete/phase15_prototypes/README.md); rien de ces répertoires n'entre dans le build, l'installation ou l'API publics.

Toute session GPU passe par les scripts gardés de [`gcp-migration/`](gcp-migration/README.md), sur une G4 `SPOT` avec deux coupe-circuits, puis se termine par la certification `TERMINATED` de la cible exacte. Les règles normatives sont dans [`AGENTS.md`](AGENTS.md).

## Licences

La licence MIT couvre le code actif et la documentation du projet. Elle ne relicencie ni [`HGP-old/`](HGP-old/), qui conserve sa licence historique non commerciale, ni les PDF de [`docs/references/`](docs/references/), dont les conditions sont documentées fichier par fichier.
