# Progression Phase 8 — boîte paddée et fermeture mono-cellule

> [!IMPORTANT]
> Phase `8`, backend `reference_cpu`, profil `generic_core`, mode `certified`. La porte d'entrée est satisfaite par les Phases 1, 4 et 7 fermées. Les jalons 8.1 et 8.2 certifient respectivement la boîte de clipping et la fermeture d'une cellule ordinaire bornée à huit sites; ils ne ferment ni le diagramme global, ni la Phase 8, ni aucun statut public.

## Jalon 8.1 — décision d'architecture

Le premier jalon de Phase 8 reste entièrement sur l'hôte. Le problème est un scan exact linéaire et ne bénéficie ni d'un transcript GPU ni d'un benchmark long. Une primitive commune calcule désormais l'AABB exacte des sites; le LBVH l'utilise pour ses extrema globaux et vérifie que les témoins recomposés par l'arbre coïncident avec elle.

Le type `ExactDyadicAabb3` demeure un POD de six mots. La provenance, les témoins, les marges et la décision fail-closed vivent dans un certificat séparé, sans changer l'ABI des consommateurs existants.

## Contrat 8.1 livré

Le schéma est `morsehgp3d.phase8.strictly_padded_dyadic_aabb.v1`, avec la règle `adjacent_finite_binary64_per_face_v1` et la base de preuve `exact_axis_extrema_strict_padding_convex_interior_v1`.

Pour chaque axe $d$, le scan calcule exactement $a_d=\min_i x_{i,d}$ et $b_d=\max_i x_{i,d}$, puis conserve le plus petit `PointId` parmi les témoins ex æquo. Il choisit ensuite $\ell_d=\mathrm{pred64}(a_d)$ et $u_d=\mathrm{succ64}(b_d)$, les voisins binary64 finis immédiats. Les voisins sont calculés sur les mots IEEE seulement; ils ne dépendent ni d'une opération flottante, ni du mode d'arrondi, ni de FTZ ou DAZ.

Le certificat enregistre l'AABB source, ses six témoins, la boîte $\Omega$, les six marges rationnelles exactes et les comptes $3n$ évaluations de coordonnées et $6(n-1)$ comparaisons d'extrema. Le vérificateur reconstruit ces données depuis le nuage et rescane toutes les inégalités strictes.

Si un voisin extérieur serait une infinité, la décision est `unsupported_finite_binary64_range`. Les masques rapportent simultanément toutes les faces concernées et aucun certificat ni boîte partielle n'est publié. Un clamp vers le plus grand fini violerait l'inclusion stricte et reste interdit.

## Preuve locale 8.1

Pour tout site et tout axe, $\ell_d<a_d\leq x_{i,d}\leq b_d<u_d$. Ainsi $X\subset\mathrm{int}(\Omega)$. L'intérieur de la boîte est convexe, donc $\mathrm{conv}(X)\subset\mathrm{int}(\Omega)$. La preuve ne suppose ni dimension affine trois, ni position générale; elle couvre singleton, axes constants, colinéarité et coplanarité.

Cette propriété suffit aussi pour les futurs centres critiques : tout centre certifié dans $\mathrm{conv}(U)$ pour $U\subseteq X$ reste strictement intérieur à $\Omega$. Elle ne prouve pas que les cellules ou leurs colonnes sont complètes.

## Validation courte 8.1

Les targets `morsehgp3d_point_cloud_aabb_tests`, `morsehgp3d_h_polytope_reference_tests` et `morsehgp3d_spatial_lbvh_tests` passent sous GCC 13.3 et Clang 18.1 avec warnings stricts. Le CTest ciblé dure environ trois centièmes de seconde par toolchain dans l'environnement courant.

La matrice couvre :

- zéros signés et voisins sous-normaux de zéro;
- frontière de binade et manipulation pure des mots;
- extrema ex æquo, permutation et témoins canoniques;
- derniers mots encore paddables avant les deux extrema finis;
- échec atomique simultané sur une face basse et une face haute;
- mutations des faces, témoins, marges, compteurs, claims, masques et nuage;
- accord exact avec l'AABB racine du LBVH;
- initialisation du H-polytope $C_0$ à huit sommets et six faces artificielles.

## Jalon 8.2 — décision d'architecture

Le plus petit incrément utile n'est ni un benchmark GPU ni un simple renommage de la réparation Phase 7. Il s'agit d'une vraie boucle de révélation progressive pour une cellule ordinaire unique : reconstruction exacte, 1-NN global à tous les sommets, lot gelé de shells complets, puis nouvelle ronde. La borne $n\leq8$ garde l'oracle exhaustif court et permet de fermer la preuve de terminaison avant d'étendre le domaine.

La piste « site violateur uniquement dans l'intérieur provisoire » a été abandonnée comme impasse mathématique : la différence de deux distances carrées est affine et tout maximum positif sur un polytope compact est atteint à un sommet. La fixture saine est un site absent de l'amorce, masqué par des concurrents successifs puis révélé à des sommets exacts.

## Contrat 8.2 livré

Le schéma `morsehgp3d.phase8.exact_ordinary_cell_vertex_nn_closure.v1`, la base `exact_affine_vertex_revelation_complete_nearest_shell_monotone_queue_v1` et la portée `bounded_n8_single_ordinary_cell_only` séparent l'amorce demandée, le germe effectif et l'ensemble fermé. Une amorce vide reçoit le plus petit concurrent extérieur; un singleton n'en reçoit aucun.

Pour une ronde $t$, chaque sommet $z$ conserve son shell co-1-NN global complet $S(z)$. Le lot $A_t=(\bigcup_{z}S(z))\setminus(J_t\cup\left\lbrace i\right\rbrace)$ est calculé sans modifier $J_t$ pendant le scan, puis trié, dédupliqué et ajouté simultanément. L'absence du propriétaire identifie un shell strictement violateur; sa présence avec un identifiant omis identifie une égalité active manquante. Toute ronde non terminale croît strictement et la ronde terminale certifie une file vide.

Le budget maximal après germe réserve 7 constructions, 966 triplets et sommets conservatifs, 10822 incidences conservatives, 966 requêtes, 7728 distances exactes et entrées de shell, 7 tests stricts du propriétaire et 6 ajouts. Une insuffisance est atomique et ne publie aucune ronde. Le vérificateur reconstruit d'abord le résultat attendu depuis les entrées fiables; un transcript différent est rejeté avant analyse profonde. L'oracle final emploie lui aussi la table fiable reconstruite, jamais les identifiants du payload observé.

## Preuve locale 8.2

Le propriétaire est strictement intérieur à $\Omega$ et satisfait strictement chaque bisecteur d'un site canonique distinct; sa cellule contient donc un voisinage ouvert. Si la file finale est vide, toute contrainte omise est strictement négative à chaque sommet : une valeur positive placerait un autre site dans le shell nearest, et une valeur nulle le placerait dans le shell du propriétaire. Par maximalité affine aux sommets, ces contraintes restent strictement négatives sur toute la cellule. L'ensemble fermé produit ainsi la même cellule et les mêmes incidences actives que la table complète, sans exiger qu'il conserve les plans strictement redondants.

## Validation courte 8.2

Le target `morsehgp3d_ordinary_cell_closure_tests` passe sous GCC 13.3 et Clang 18.1 avec warnings stricts en moins d'une seconde. Le même target passe en 5,22 secondes sous ASan et UBSan après le durcissement du chemin de rejet. La matrice couvre :

- singleton, paire et germe extérieur canonique;
- mauvaise amorce lointaine révélant un violateur proche;
- site lointain strictement redondant laissé hors de l'ensemble fermé mais égalité avec l'oracle normalisé;
- trois lots successifs sur les sites colinéaires $0,2,4,8,16$;
- co-incidence tangentielle et shells complets de face, arête et sommet du cube;
- permutation de l'amorce;
- tous les caps exacts puis chacun juste insuffisant, avec payload géométrique nul;
- mutations de chaque couche, dont identifiant hors plage et amorce observée non triée, rejetées sans indexer le payload non fiable;
- comparaison à l'oracle toutes contraintes par sommets, faces artificielles et identifiants actifs normalisés.

## Limites et suite saine

La Phase 8 reste `ready` et sa porte de sortie reste ouverte. Le jalon suivant doit appliquer ce contrat borné à toutes les cellules ordinaires d'un petit nuage, puis réconcilier les incidences naturelles réciproques entre propriétaires avant toute extraction d'événement. Le backend CUDA qualifié en Phase 7 pourra proposer les triplets, mais la décision restera un rejeu `reference_cpu`.

Aucun `CatalogCertificate`, événement Morse, hiérarchie, `closed_parent_orders[1]`, mesure de débit ou `public_status=exact` n'est produit ici.

## GCP

GCP non utilisé : ces jalons hôte courts ne justifient ni création ni démarrage de VM.
