# Progression Phase 8 — boîte dyadique strictement paddée

> [!IMPORTANT]
> Phase `8`, backend `reference_cpu`, profil `generic_core`, mode `certified`. La porte d'entrée est satisfaite par les Phases 1, 4 et 7 fermées. Ce jalon certifie uniquement le confinement strict du nuage et de son enveloppe convexe dans une boîte de clipping; il ne ferme aucune cellule, aucun diagramme et aucun statut public.

## Décision d'architecture

Le premier jalon de Phase 8 reste entièrement sur l'hôte. Le problème est un scan exact linéaire et ne bénéficie ni d'un transcript GPU ni d'un benchmark long. Une primitive commune calcule désormais l'AABB exacte des sites; le LBVH l'utilise pour ses extrema globaux et vérifie que les témoins recomposés par l'arbre coïncident avec elle.

Le type `ExactDyadicAabb3` demeure un POD de six mots. La provenance, les témoins, les marges et la décision fail-closed vivent dans un certificat séparé, sans changer l'ABI des consommateurs existants.

## Contrat livré

Le schéma est `morsehgp3d.phase8.strictly_padded_dyadic_aabb.v1`, avec la règle `adjacent_finite_binary64_per_face_v1` et la base de preuve `exact_axis_extrema_strict_padding_convex_interior_v1`.

Pour chaque axe $d$, le scan calcule exactement $a_d=\min_i x_{i,d}$ et $b_d=\max_i x_{i,d}$, puis conserve le plus petit `PointId` parmi les témoins ex æquo. Il choisit ensuite $\ell_d=\mathrm{pred64}(a_d)$ et $u_d=\mathrm{succ64}(b_d)$, les voisins binary64 finis immédiats. Les voisins sont calculés sur les mots IEEE seulement; ils ne dépendent ni d'une opération flottante, ni du mode d'arrondi, ni de FTZ ou DAZ.

Le certificat enregistre l'AABB source, ses six témoins, la boîte $\Omega$, les six marges rationnelles exactes et les comptes $3n$ évaluations de coordonnées et $6(n-1)$ comparaisons d'extrema. Le vérificateur reconstruit ces données depuis le nuage et rescane toutes les inégalités strictes.

Si un voisin extérieur serait une infinité, la décision est `unsupported_finite_binary64_range`. Les masques rapportent simultanément toutes les faces concernées et aucun certificat ni boîte partielle n'est publié. Un clamp vers le plus grand fini violerait l'inclusion stricte et reste interdit.

## Preuve locale

Pour tout site et tout axe, $\ell_d<a_d\leq x_{i,d}\leq b_d<u_d$. Ainsi $X\subset\mathrm{int}(\Omega)$. L'intérieur de la boîte est convexe, donc $\mathrm{conv}(X)\subset\mathrm{int}(\Omega)$. La preuve ne suppose ni dimension affine trois, ni position générale; elle couvre singleton, axes constants, colinéarité et coplanarité.

Cette propriété suffit aussi pour les futurs centres critiques : tout centre certifié dans $\mathrm{conv}(U)$ pour $U\subseteq X$ reste strictement intérieur à $\Omega$. Elle ne prouve pas que les cellules ou leurs colonnes sont complètes.

## Validation courte

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

## Limites et suite saine

La Phase 8 reste `ready` et sa porte de sortie reste ouverte. Le jalon suivant doit fermer une première cellule ordinaire par une boucle monotone : amorce sûre, reconstruction H-polytope, recherche exacte de tous les violateurs et co-minimiseurs aux sommets, ajout simultané, réconciliation des incidences, puis file vide. Le backend CUDA qualifié en Phase 7 pourra proposer les triplets, mais la décision restera un rejeu `reference_cpu`.

Aucun `CatalogCertificate`, événement Morse, hiérarchie, mesure de débit ou `public_status=exact` n'est produit ici.

## GCP

GCP non utilisé : ce jalon CPU linéaire ne justifie ni création ni démarrage de VM.
