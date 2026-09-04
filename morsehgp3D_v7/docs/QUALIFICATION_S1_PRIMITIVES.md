# Qualification des primitives de S1 — état au 4 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 1. Conclusion et périmètre

Le [théorème S1, § 6](../audits/S1_COURANT.md#6-théorème-géométrique-conditionnel-et-rle)
est désormais **démontré conditionnellement** : toute boule minimale pertinente
atteint le catalogue RLE avec sa bonne arité, sous les contrats primitifs nommés.
L'existence du propriétaire et du seed, les covers, le front, les secteurs,
la corde et les cellules ne constituent plus des lemmes géométriques ouverts.
Cette note ne les rouvre pas et ne modifie aucun rapport indépendant.

Trois niveaux de justification sont distingués :

- **Démontré** : argument de domaine ou invariant explicitement donné, sous ses
  préconditions ; ce n'est pas une preuve de conformité de tout compilateur.
- **Couvert par portes** : exécutions et mutants identifiés dans un reçu v7 ;
  jamais une preuve universelle obtenue par échantillonnage.
- **Manquant** : raccord précis non établi par les pièces examinées, à fermer
  avant une qualification industrielle universelle.

Les primitives CPU sont la cible de cette note. Aucun test lourd, compilation
nouvelle, modification de code ou appel GCP n'a été réalisé pour la rédiger.
Les priorités d'optimisation sont désormais mono-CPU, puis multi-CPU, puis GPU.
Le [contrat de performance](CONTRAT_PERFORMANCE.md) demandé est **50k points,
toute la tour K=1..10 en moins d'une seconde** ; le repli vers toute la tour
K=1..5 n'intervient que si l'objectif 1..10 échoue, puis viennent plusieurs dizaines de millions de
points. Ce sont des objectifs à mesurer, pas une conséquence de S1 ni un
résultat du reçu Release sur hôte partagé.

## 2. Autorité des preuves et des exécutions

Les autorités mathématiques courantes sont [S1](../audits/S1_COURANT.md),
les [filtres flottants](../audits/FILTRES_FLOTTANTS_COURANTS.md) et les
[cellules](../audits/CELLULES_COURANT.md). Leurs manifestes propres sont
[s1_sources.json](../audits/receipts_20260904/s1_sources.json),
[float_sources.json](../audits/receipts_20260904/float_sources.json) et
[cell_sources.json](../audits/receipts_20260904/cell_sources.json).
L'autorité des exécutions ici citées est le
[reçu Release](../receipts/release_20260904/summary.json), pas un résultat v6.

Ce reçu rapporte 279 portes `gate`, zéro échec et zéro saut, plus deux
invocations autonomes du banc d'incidences, normales et optimisées. Les
sources sont stables et les 31 binaires testés sont inchangés. Le produit
avant/après a le SHA-256
`8a99b5cf5dc2c0622947f6b34880803e4ed347608ea61d56f3e080137bfc4a6b`.
La commande réellement enregistrée est :

```bash
ctest --test-dir /workspaces/E-HGP/build/v7 --output-on-failure --no-tests=error -L '^gate$' --parallel 2 --output-junit /workspaces/E-HGP/morsehgp3D_v7/receipts/release_20260904/ctest.junit.xml
```

Elle a rendu 0 en 1929,494 s sur hôte partagé : durée de qualification, pas
mesure de performance du moteur. Le
[JUnit](../receipts/release_20260904/ctest.junit.xml) et
[l'inventaire](../receipts/release_20260904/inventory.stdout) donnent les
noms exécutés. La configuration est Release ; le reçu ne prétend pas à une
reconstruction hermétique de toutes les cibles : son étape `build_missing`
construit seulement la nouvelle cible `mhgp7_perm_residence_gate` et conserve
les autres binaires déjà construits.

À la rédaction, deux contrôles de lecture ont rendu 0 : vérification de
tous les fichiers du `receipt_manifest.json`, puis comparaison de toutes
les sources `src/`, CMake et des cinq tests primitifs ci-dessous au manifeste
`sources_before.json`. Ces constats ne s'étendent pas automatiquement à un
delta ultérieur. Commande reproductible de contrôle du reçu, depuis son dossier :

```bash
jq -r '.[] | "\(.sha256)  \(.path)"' receipt_manifest.json | sha256sum -c --quiet
```

Les cinq tests relus sont `selftest.cpp`, `linked_arcs_gate.cpp`,
`cell_grid_oracle.cpp`, `perm_sort_gate.cpp`, `perm_residence_gate.cpp`.
Les fixtures `receipts/conformite_v5/` restent un différentiel historique
explicitement importé, jamais la preuve autonome des primitives v7.

## 3. Index, parcours, tris et clé primitive

| Contrat concret | Démontré / source | Couverture v7 du reçu Release | Raccord restant |
|---|---|---|---|
| Profil des positions et identités | `p3_in_profile` impose 0..65535 ; `build_cloud_index` refuse les PointId dupliqués ; le pipeline refuse ensuite les positions dupliquées | `mhgp7_tree_selftest`, `mhgp7_contrat_echec`, portes d'entrée/archive | L'appel bas niveau direct à l'index doit annoncer ses préconditions de cardinal, distinctes de celles déjà imposées par le pipeline |
| Cardinal admissible aux indices | `run_pipeline_into` impose `kMaxTreePositions=2^30-1` avant l'index ; argument de largeur ci-dessous | Portes caps et arithmétique sur frontières sans allocation géante ; pas de run d'un milliard de points | Une porte de cette garde précise et de chaque conversion bas niveau n'est pas identifiée ici ; ne pas confondre avec la garde des candidats `2^32-1` |
| Morton bijectif, buckets sans perte, arbre radix | `morton.hpp`, `cloud_index.hpp` : interlacement de 16 bits, tri `(Morton, PointId)`, CSR puis Karras | `mhgp7_tree_selftest` : 3000 points, seeds 3/4, boîtes contenant leurs plages et permutation d'entrée | Formaliser l'invariant complet de partition parent/enfants, atteignabilité unique, racine et précondition non-zéro de `clzll` ; les tests actuels ne le prouvent pas universellement |
| Parcours des rectangles/covers, pas de double crédit | La preuve géométrique du front et S1 composent les populations ; `edge_cover.hpp` utilise des handles d'antichaîne | `mhgp7_wspd_ledger`, `mhgp7_wspd_ownership`, mutants cap/split, `mhgp7_fused_descent`, `mhgp7_cover_coef4` et mutant coefficient | Relier les invariants de chaque pile/plage à ceux de l'index, sans supposer qu'un accord de masse seul exclut une omission compensée par un doublon |
| Tri stable et permutation in situ | Théorème explicite dans `parallel/sort.hpp` : ordre total `(classe less, rang original)`, ramassage par cycles ; fusion gauche avant droite | `mhgp7_perm_sort`, plancher, mutants scatter/partial/tie-desc/unstable ; `mhgp7_perm_residence` et ses trois mutants ; `mhgp7_thread_failure` | La conformité de `std::sort`, de la bibliothèque, de la concurrence et du comparateur reste liée à l'implémentation/ABI exécutée, pas à toute bibliothèque possible |
| PGCD et clé de boule primitive | Argument ci-dessous ; `keys.hpp:101`, `intmath.hpp:66` ; clé lexicographique, coefficient quadratique positif | `mhgp7_linked_arcs_u16` reconstruit les clés avec PGCD/division indépendants OBig384 ; son mutant d'oracle i64 rend le dépassement observable | Pas de porte autonome `ugcd128` couvrant systématiquement zéros, transitions 64/128 bits et cas limites identifiée dans les 279 |
| RLE conserve l'arité minimale | S1 § 6 + `candidates.hpp:28` : clé puis arité puis représentation du niveau ; `std::unique` garde le premier | `mhgp7_sweep_oracle`, `mhgp7_linked_arcs_u16`, mutants RLE/composition | La conclusion dépend de S1 et du tri conformes ; elle ne requiert pas d'énumérer tous les supports alternatifs |

### Cardinal de l'arbre : la garde produit existe

Dans [caps.hpp](../src/core/caps.hpp) et
[run.hpp:412](../src/pipeline/run.hpp#L412), $n\leq2^{30}-1$ implique
$m\leq n$ pour les positions uniques. Les casts de buckets en u32 et des
indices en i32 sont alors représentables. Dans la recherche exponentielle
de Karras, `lmax` ne dépasse pas $2^{30}$ : à cette valeur le voisin est
nécessairement hors domaine et le test s'arrête avant le prochain décalage.
Même `i+lmax` reste inférieur à $2^{31}$ ; les références `-1-u` sont également
représentables. Les poids cumulés sont au plus n et les masses de paires
inférieures à $2^{60}$, donc tiennent en u64. Cet argument suppose les types
usuels de l'ABI CPU qualifiée et ne prouve pas à lui seul la topologie Karras.
50k et 100M satisfont cette borne ; cela ne garantit ni RAM ni temps viable.

### PGCD et canonisation : invariant, non simple test

À chaque étape d'Euclide, `gcd(x,y)=gcd(y,x%y)` ; le reste non nul diminue
strictement. `ugcd128` applique cet invariant jusqu'à ce que y tienne en
u64, puis reprend exactement `gcd(y,x%y)` dans `ugcd64`. Les deux cas zéro
sont traités avant division. `uabs128` évite la négation directe du minimum
signé grâce à `-(v+1)+1` en non signé. Dans `ball_key_reduce`, le coefficient
initial a est strictement positif ; le PGCD reste positif et ne dépasse
pas a, donc sa conversion en i128 est sûre sous les bornes suivantes.
Le PGCD égal à 1 autorise la sortie anticipée. Diviser tous les coefficients
par leur PGCD conserve la forme et donne l'unique représentant entier
primitif de la demi-droite rationnelle positive des coefficients (a,b,c).
Le signe a>0 est une précondition
des formes q2/q3/q4, pas une normalisation supplémentaire cachée dans le PGCD.

## 4. Largeurs entières : bornes séparées des portes

Poser $M=65535<2^{16}$. Les bornes suivantes, volontairement larges,
sont déduites des expressions des lanes sur positions u16, pas de leurs
valeurs observées dans les tests. Elles supposent que la promotion indiquée
dans le code précède chaque multiplication ; elles ne constituent pas
l'inventaire exhaustif de tous les intermédiaires du front.

| Expression / domaine | Borne suffisante et type | Justification / limite |
|---|---|---|
| Différence d'une coordonnée, distance carrée et produit scalaire de deltas | $|d_i|<2^{16}$ ; $|d\cdot e|\leq3M^2<2^{34}$, i64 | Trois produits de deltas ; une composante de croix est au plus $2M^2<2^{33}$ |
| Coordonnées affines du cover | $|2z_i-a_i-b_i|<2^{17}$ ; $|q|<2^{36}$, i64 et conversion binaire64 exacte | Somme de trois carrés puis soustraction d'une distance carrée ; moins de 53 bits |
| Gram q3 | $0<G<2^{68}$ ; coefficients $E(D-F),D(E-F)$ de module $<2^{69}$ ; $|W_i|<2^{86}$ | $G=DE-F^2\leq DE$, positif sur seed aigu ; somme de deux produits pour W |
| Forme et puissance q3 | $|N_i|=|W_i-Gd_i|<2^{87}$ ; $|B_i|<2^{87}$ ; $|C|<2^{105}$ ; puissance développée $<2^{107}$, i128 | Ces bornes plus lâches que certains commentaires suffisent : trois produits par somme, coordonnées $<2^{16}$ |
| Affine/Jung/corde | $|L|<2^{108}$ ; $|J|<2^{105}$ ; $|B_z|<2^{51}$ ; $\widehat\mu<2^{53}$ ; $|L-c\widehat\mu B_z|<2^{110}$, i128 | $J=D^2(3G-2AX^2BX^2)$ ; $|c|\leq4$ ; les carrés/comparaisons de Jung peuvent dépasser i128 et passent par U320 |
| Cramer q4 | Entrées de M $<2^{17}$, cofactors $<2^{35}$, $|\det|<2^{54}$, $|N'_i|<2^{71}$, i128 | Trois termes pour det et chacun des numérateurs ; les bornes publiées plus larges $2^{57}$ et $2^{72}$ restent valides |
| Signe de centre q4 | Volume orienté de module $<2^{51}$ en i64 ; `rc=N'-det*dp` puis déterminant de face de module $<2^{107}$ en i128 | `rc` a module $<2^{72}$, le cofacteur de deux deltas $<2^{33}$ et la somme contient trois termes ; aucun produit géant n'est ramené en i64 |
| Niveaux | q3 : num $<2^{102}$, den $<2^{70}$ ; croisement $<2^{172}$ en U192. q4 : num $<2^{146}$, den $<2^{114}$ ; croisement $<2^{260}$ en U320 | q3 multiplie trois distances ; q4 additionne trois carrés des numérateurs et utilise det² ; bornes q4 volontairement celles, plus larges, du contrat |
| Base des secteurs et grille | Facteurs de base bornés par la boucle de 128 itérations, donc $|u_i|,|v_i|<2^{24}$ ; Gram de base $<2^{50}$ et déterminant $<2^{100}$ | Avec $|N_i|<2^{87}$, produits seed/base et extrémités restent sous $2^{114}$ ; le localisateur multiplie ensuite en binaire64, pas en i128 |

Les valeurs non nulles des dénominateurs sont positives par les gardes de
support ; `det==0` n'est jamais une émission q4. Les signes d'orientation
sont canonisés ensemble dans [q4_form](../src/lanes/q4.hpp#L48).
La positivité géométrique et l'identité de Cramer sont traitées par les
preuves S1 ; le tableau traite leur réalisation arithmétique bornée.

Les produits à limbs ont également une obligation algorithmique :
`mul_128x128_192` exige que le produit tienne sur 192 bits, contrairement
à un produit général 128×128. Les accumulations de ses colonnes et de
`mul_192x128_320` additionnent un nombre fixe de mots de 64 bits dans u128
avant propagation des retenues ; aucune colonne n'approche 128 bits.
La troncature finale n'est sûre que sous la borne du résultat. Pour la
somme de trois carrés, la borne $2^{146}$ exclut aussi le débordement du
dernier mot de U192. Les tests doivent réfuter une retenue erronée ; ils ne
peuvent remplacer cette précondition pour chaque appel.

**Couverture existante, avec limites.** `mhgp7_arith_selftest` vérifie des
racines aux frontières, quelques produits U192/U320 et 20 000 produits DI128
contre `__int128` : il ne fournit pas une preuve exhaustive des produits
larges. `mhgp7_linked_arcs_u16` utilise un oracle OBig384 indépendant pour
des clés et puissances, avec un témoin explicite dépassant i64.
`mhgp7_sweep_oracle` énumère des supports mais réutilise q3/q4/PGCD du produit :
il juge la couverture du générateur, pas indépendamment ces primitives.

**Manquant pour fermer l'inventaire entier.** Établir un grand-livre exhaustif
des expressions du front, de chaque changement d'échelle et conversion,
de la division plancher et des racines `floor_sqrt`/`isqrt128_floor`, avec
préconditions et borne de chaque intermédiaire ; rattacher les comparaisons
larges et Cramer à des portes autonomes d'oracle indépendant avec retenues
et signes adverses. Les racines à graine flottante sont corrigées par des
comparaisons entières, mais exigent notamment une graine finie convertible
et des carrés de correction représentables.

Deux noms de portes cités dans les commentaires hérités,
`mhgp7_level_cmp` et `mhgp7_q3_affine`, **ne figurent pas dans CMake ni les
tests v7 relus**. Aucun des 279 succès ne doit leur être attribué. Ne pas
importer tacitement une qualification v4/v5/v6 pour combler ce manque.

## 5. Binaire64, FMA et compilation effective

Les [filtres](../audits/FILTRES_FLOTTANTS_COURANTS.md) prouvent leurs marges
affine/Jung/corde avec les arrondis réellement comptés, y compris `lh±E` ;
les [cellules](../audits/CELLULES_COURANT.md) prouvent la surcouverture du
localisateur, y compris ses bornes finales. Les bornes de magnitude et de
granularité de ces preuves excluent débordement et sous-normaux dans leurs
domaines. Leur validité géométrique et numérique conditionnelle est acquise.

| Hypothèse d'exécution | Réalisation observée | Qualification restant attachée au binaire |
|---|---|---|
| RN binaire64, conversions correctement arrondies | `float_filter_runtime_enabled` teste `FE_TONEAREST` ; `__FAST_MATH__` désactive le filtre | Attestation ABI/binary64 et conversions i128→double ; pas de porte autonome de tous les modes d'arrondi identifiée |
| FMA correctement arrondie | `std::fma` explicite dans `float_filter.hpp:43` et la borne | Épingler compilateur, bibliothèque mathématique, cible matérielle et options ; un simple résultat plausible n'est pas une preuve FMA |
| Pas d'évaluation élargie pour le localisateur | `kCellLocateEvalOk=(FLT_EVAL_METHOD==0)` ; environnement refusé ⇒ grille non construite | Relier la macro effective au binaire, ne pas déduire sa valeur du seul nom « x86_64 » |
| Graphe sans réassociation, environnement stable | Contrat explicite des preuves ; `float_on` capturé à l'entrée de la génération | Interdire les options incompatibles, documenter les obligations des callbacks/threads et conserver les options effectives ; `__FAST_MATH__` seul ne couvre pas toutes les options partielles dangereuses |
| Repli exact sans perte | Borne infinie ou décision indécidable ⇒ voie entière ; `grid.built` garde les localisations | Portes directes on/off, égalités et changements d'environnement encore à identifier/compléter ; la grille possède déjà son oracle rationnel et trois mutants |

La configuration locale relue contient CMake 3.28.3, GNU C++ 13.3.0,
`/usr/bin/c++`, CUDA désactivé. `flags.make` affiche :

```text
-O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror
```

Il n'y a dans cette ligne ni fast-math ni réassociation explicitement
demandée. Cela ne constitue pas une attestation complète des valeurs par
défaut, de la contraction, du graphe machine ou de l'environnement flottant.
Le reçu scelle les hashes de CMakeCache/CTest/Makefile, pas un audit universel
du compilateur. La prochaine qualification doit archiver les commandes
complètes de compilation/édition de liens, versions et macros pertinentes,
la bibliothèque et l'architecture, puis les relier au SHA du binaire livré.
Une compilation GCC 11, Clang ou CUDA est un nouveau domaine à qualifier ;
le bootstrap d'outillage GCP et sa sonde C++20 ne transfèrent pas le reçu CPU.

## 6. Fermeture suivante, sans relancer la géométrie

1. Figer une matrice de préconditions des primitives, avec grand-livre des
   largeurs et preuve des invariants d'index/parcours qui ne sont aujourd'hui
   que testés ; conserver la borne produit de cardinal déjà en place.
2. Donner des portes v7 autonomes aux produits larges, PGCD/Cramer et filtres,
   avec juges réellement indépendants et planchers de non-vacuité. Une porte
   supplémentaire qualifie des exécutions ; elle n'établit pas une borne.
3. Qualifier d'abord une commande mono-CPU précise et son environnement
   numérique ; étendre ensuite aux commandes multi-CPU, puis au GPU, chacune
   avec son binaire et ses propres reçus. Ne pas attribuer les stubs au device.
4. Réutiliser S1 comme théorème conditionnel fermé et décharger explicitement
   chacune de ses préconditions pour ce domaine d'exécution. L'archive,
   l'exactitude horizontale complète, la verticale et les coûts gardent leurs
   contrats distincts ; ils ne sont pas de nouveaux verrous géométriques S1.

Aucun statut public n'est promu par cette cartographie. GCP non utilisé.
