# Progression Phase 8 — boîte, diagramme ordinaire et supports de profondeur zéro

> [!IMPORTANT]
> Phase `8`, backends `reference_cpu` et `reference_python`, profil `generic_core`, mode `certified`. La porte d'entrée est satisfaite par les Phases 1, 4 et 7 fermées. Les jalons 8.1 à 8.5 certifient la boîte de clipping, la fermeture mono-cellule, le diagramme ordinaire clippé entier, son accord avec un atlas affine indépendant et l'extraction de ses supports naturels de profondeur zéro, toujours dans le domaine borné à huit sites. Ils ne ferment ni la Phase 8, ni le catalogue Morse H0, ni aucun statut public.

> [!IMPORTANT]
> Depuis la relecture des Parties I–II du manuscrit le 21 juillet 2026, cette phase est gelée comme oracle cellulaire borné. Étendre ses cellules top-$m$ reconstruirait la combinatoire de la mosaïque d'ordre $K$ que MorseHGP3D doit éviter. La voie produit est désormais le flux direct LBVH/GPU de Phase 9; les acquis 8.1–8.5 restent des différentiels et des témoins locaux.

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

## Jalon 8.3 — décision d'architecture

Le jalon sain après la fermeture mono-cellule est une transaction exacte sur tous les propriétaires, suivie d'une réconciliation globale. Chaque propriétaire repart de l'amorce vide canonique de 8.2; aucun germe fourni par une heuristique et aucun transcript GPU ne peuvent donc sélectionner la topologie. Un préflight global réserve les huit fermetures locales, la fusion des sommets et toutes les intersections de cellules avant de construire la première cellule. Une insuffisance demeure atomique.

Cette voie évite deux impasses : extrapoler une incidence depuis un accord flottant moyen, ou confondre toute intersection non vide de cellules avec une strate canonique. Le coût de rejeu est volontairement dupliqué pour garder le vérificateur frais et indépendant du payload non fiable; il reste acceptable sous la borne $n\leq8$ et devra être factorisé avant toute extension de domaine.

## Contrat 8.3 livré

Le schéma est `morsehgp3d.phase8.exact_bounded_ordinary_diagram_closure.v1`, avec la base `exact_all_owner_cell_closure_vertex_occurrence_bijection_barycentric_contact_carrier_quotient_v1` et la portée `bounded_n8_all_ordinary_cells_auditable_contacts_and_reciprocal_natural_strata_only`. Le résultat embarque aussi les trois mots binary64 canoniques de chaque point : même un reçu d'insuffisance reste lié exactement au nuage fiable.

Les sommets locaux de même position rationnelle sont fusionnés seulement si leur distance nearest exacte, leur shell complet et leur masque de boîte coïncident. Pour tout sommet global $z$, l'ensemble de ses propriétaires observés doit être exactement son shell co-1-NN complet $N(z)$; cette bijection exclut simultanément les incidences manquantes et surnuméraires.

Pour toute requête non singleton $Q$ telle que $K_Q\neq\varnothing$, le contact auditable est $K_Q=\bigcap_{i\in Q}C_i=\left\lbrace z\in\Omega:Q\subseteq N(z)\right\rbrace$. Ses sommets sont donc exactement les sommets globaux dont le shell contient $Q$; une intersection vide ne produit aucune ligne. La moyenne rationnelle de tous ces sommets appartient à l'intérieur relatif de $K_Q$. Un oracle nearest global frais y reconstruit le shell porteur, égal à l'intersection des shells des sommets de $K_Q$. Le contact est canonique si et seulement si ce shell porteur vaut $Q$; sinon il reste publié comme `noncanonical_quotient_contact`, ce qui rend les quotients dégénérés auditables sans inventer de face.

Un contact canonique sans face artificielle commune est classé par l'identité exacte $\mathrm{rang}_{\mathrm{aff}}(Q)+\dim(K_Q)=3$ : rang un pour une face naturelle, deux pour une arête et trois pour un sommet. Une face de boîte commune produit `box_supported_contact` et n'est jamais promue en strate naturelle.

Les vingt et un caps indépendants couvrent au plus 8 cellules, 56 constructions, 7728 triplets et requêtes de sommets, 86576 incidences, 61824 distances et entrées de shell locales, 48 lots et 48 ajouts simultanés avec lot maximal 6, 2288 occurrences finales et sommets globaux, 18304 entrées de shell globales, 247 contacts, 1016 identifiants de requête, 1976 identifiants porteurs, 565136 références contact-sommet, 247 témoins et 1976 distances de témoins.

## Preuve globale bornée 8.3

La fermeture 8.2 prouve chaque $C_i$ contre la table complète. La fusion exacte et la bijection propriétaires-shell établissent alors les sommets et toutes leurs incidences globales. L'égalité définissant $K_Q$ découle directement de $z\in C_i$ si et seulement si $i\in N(z)$ sur une intersection de cellules ordinaires. La moyenne de tous les sommets d'un polytope non vide est dans son intérieur relatif; les différences de distances carrées étant affines, les égalités actives sur ce témoin sont exactement celles actives sur tout $K_Q$. Le rejeu nearest frais identifie ainsi sans hypothèse de position générale le shell porteur maximal. Enfin, la dualité affine dimension-rang classe seulement les porteurs canoniques qui ne sont pas soutenus par la boîte.

La preuve couvre donc tout le diagramme ordinaire clippé et ses strates naturelles réciproques dans la portée $n\leq8$. Elle ne couvre ni extraction d'événement Morse, ni hiérarchie, ni extension asymptotique de l'algorithme.

## Validation courte 8.3

Les quatre CTests ciblés `h_polytope_reference`, `point_cloud_aabb`, `ordinary_cell_closure` et `ordinary_diagram_closure` passent sous GCC 13.3 en 4,61 secondes et sous Clang 18.1 en 4,09 secondes. Le nouveau target 8.3 prend respectivement 4,21 et 3,71 secondes avec warnings stricts.

La matrice 8.3 couvre :

- singleton, paire, triangle et tétraèdre;
- quatre sites colinéaires clairsemés, sans expansion abusive vers l'ensemble des parties;
- contact soutenu par la boîte, séparé des strates naturelles;
- carré cocirculaire, dont les diagonales deviennent des quotients du shell quatre plutôt que de fausses faces;
- cube cosphérique, avec 247 contacts auditables, 12 faces, 6 arêtes de shell quatre, 1 sommet de shell huit et 228 quotients non canoniques;
- permutation canonique du nuage;
- chacun des vingt et un budgets juste insuffisant, puis toute valeur au-dessus de la borne de confiance;
- mutations du manifeste, de chaque couche de cellule, des occurrences, shells, contacts, masques, audits, claims et du nuage fiable;
- liaison exacte au nuage même pour un reçu d'insuffisance.

## Jalon 8.4 — atlas affine indépendant

L'oracle `reference_python` décode localement les mots binary64 et construit directement chaque intersection non vide $K_Q$ par RREF rationnelle, inégalités nearest et faces de boîte. Il n'importe aucune primitive de production. Les singletons reconstruisent les cellules; les sous-ensembles non singleton reconstruisent les contacts avant tout accès à la projection du producteur. Compacité, paramétrisation affine et énumération de toutes les familles de frontières indépendantes donnent un contrôle exhaustif dans la portée `bounded_n8_independent_affine_subset_oracle_only`.

Le dump C++ exige d'abord la vérification fraîche complète de 8.3, puis sérialise seulement manifeste, boîte, claims et projection sémantique. Le comparateur refuse JSON non canonique, clés ou objets dupliqués, identifiants hors plage et incohérences géométriques avant de confronter positions rationnelles, cellules, sommets, distances, shells, propriétaires, masques, contacts, carriers, dimensions, rangs, témoins et kinds.

Le batch réel utilise un unique sous-processus et seize cas fixes : cardinalités un à huit, zéros signés, sous-normaux, paire oblique, contact porté par la boîte, collinéarité, carré exact et perturbations d'un ULP, tétraèdre avec ou sans centre, courbe des moments, shell sept, cube exact et perturbé, puis trois permutations. Il passe avec les producteurs GCC et Clang stricts. Les six tests unitaires couvrent aussi les neuf caps exacts, chaque insuffisance atomique et tout dépassement de confiance; `tools/check_oracle_independence.py` confirme l'absence d'import de production.

## Jalon 8.5 — extraction naturelle de profondeur zéro

Le backend `reference_cpu` expose `build_exact_bounded_depth_zero_natural_supports` et son vérificateur frais. Après préflight, il exige un diagramme 8.3 complet et fraîchement certifié, puis parcourt seulement les carriers naturels. Tous leurs sous-supports de tailles deux à quatre sont proposés, triés et dédupliqués; les quotients et contacts de boîte restent auditables dans la source sans jamais devenir des supports émis. Le header du producteur ne dépend pas du catalogue critique exhaustif.

Pour chaque candidat, le centre circonscrit, le niveau et les barycentriques sont recalculés exactement depuis les sites. Seuls les supports minimaux déclenchent une partition globale de boule fermée. Un intérieur strict non vide est différé. À intérieur vide, un shell plus grand devient un diagnostic pertinent selon le rang du support, même lorsque le rang fermé du shell dépasse la fenêtre; un shell égal au support est accepté seulement dans la fenêtre. Les tailles deux, trois et quatre doivent pointer respectivement vers une face, une arête et un sommet naturels de dimensions deux, un et zéro.

La preuve de complétude de ce seul étage part de tout support minimal à intérieur strict vide : son centre appartient à l'intérieur de la boîte et au contact de son shell global. Ce contact est un carrier naturel, donc l'énumération exhaustive de ses sous-supports retrouve le support. Elle ne couvre pas les profondeurs positives et l'absence de diagnostic 8.5 ne prouve pas `RelevantGP` global.

Les six capacités au plafond `n=8` valent 247 contacts source, 4704 propositions brutes, 13440 références brutes, 154 supports uniques, 504 références uniques et 1232 classifications. Les six insuffisances sont testées sans construire le diagramme du cube et précèdent toute copie ou vérification de source. Une source hostile n'est copiée dans aucun reçu; même une source qui affirme `complete` est rejetée si son transcript frais diffère.

La suite ciblée passe en Release strict sous GCC et Clang. Elle couvre singleton vide, tétraèdre régulier pour les trois tailles, fenêtre `K_max=1`, pentagone cocirculaire de shell cinq, seuil triangle à un ULP, support porté par la boîte, permutation, budgets et falsification de chaque couche. Le catalogue exhaustif confirme dans les deux sens les supports acceptés et diagnostics sur le pentagone, uniquement comme oracle de test. Les exécutions directes finales prennent environ 3,1 secondes sous chaque toolchain dans l'environnement courant; les CTests ciblés passent aussi.

## Décision de réutilisation et de débit

Cet atlas est gelé à huit sites et ne sera pas étendu en moteur de Voronoï général. Toute future baseline plus large doit d'abord évaluer un adaptateur épinglé vers Geogram ou une bibliothèque mature équivalente; elle restera une comparaison non autoritative jusqu'au rejeu exact. Le chemin produit reste GPU-first, avec proposition massive sur `cuda_g4`, recertification exacte côté hôte, cible principale de passage complet avec p95 `warm_e2e` strictement inférieur à 100 ms autour de 50 000 points pour $K_{\max}\leq10$, objectif secondaire strictement inférieur à une seconde sur le même protocole, puis streaming transactionnel et budgeté à dix millions de points ou davantage.

## Limites et suite saine

La Phase 8 reste `ready` et sa porte de sortie reste ouverte, mais 8.6 n'est plus le prochain jalon produit. Il demeure un éventuel complément d'oracle borné : injecter séparément les minima singleton de rayon nul, bâtir les lots H0 d'ordre un, produire le `CatalogCertificate` et fermer `closed_parent_orders[1]` seulement si le diagramme, l'extraction et toutes les files sont complets sans overflow ni diagnostic bloquant. Le chantier actif passe à la Phase 9. Il ne faut ni élargir l'atlas rationnel, ni confondre ce contrôle borné avec le chemin de débit. Le rejeu imbriqué actuel demeure une dette admise uniquement dans le petit domaine de preuve.

Aucun `CatalogCertificate`, lot H0, événement Morse public, hiérarchie, `closed_parent_orders[1]`, mesure de débit ou `public_status=exact` n'est produit ici. Les propositions CUDA qualifiées en Phase 7 restent disponibles pour un futur accélérateur, mais ne participent encore à aucune décision 8.3 ou 8.5.

## GCP

GCP non utilisé : ces jalons hôte courts ne justifient ni création ni démarrage de VM.
