# Lecteur HGP-old et thèse : rapport du 29 septembre 2026

Cadre : `phase=exploration_v10_hors_registre`, `public_status=not_claimed`, lecture seule. Rien n'a été modifié ni commité dans le dépôt. GCP non utilisé. HGP-old est lu comme une spécification : je le cite par `fichier:ligne` (racine `HGP-old/`) sans en recopier de code.

Pour la thèse, je cite la page PDF ; la page imprimée vaut PDF − 26. Une source s'est révélée décisive : la référence [65] de la thèse, `docs/references/pdfs/hauseux-et-al-hgp-applied-network-science-2026.pdf` (« ANS 2026 »). Les tableaux 9.1 à 9.3 en proviennent.

Notations :
- ρ(σ) : rayon de la miniboule de σ (Déf. 25).
- α_m(x) : rayon de la plus petite boule fermée contenant x et m − 1 autres sites. L'entrée `cover` de la v10 est α_K.
- z = `expZ`, et ψ(t) = t^(−z).

## 0. L'essentiel

1. **Objet.** HGP-old calcule Γ_K (Déf. 21) sur un catalogue restreint, C_old(K) : les (K+1)-parties dont la cellule de Voronoï d'ordre K+1 est non vide. Les niveaux sont ρ(σ)^z en float32 ; les facettes F = ∂C_old sont reliées en chemin.
   - Contrôle exact indépendant sur 43 petits nuages, E5 compris : mêmes ensembles de points que FULL à tous les niveaux.
   - Le graphe de Gabriel (Déf. 29) échoue sur E5 et sur 1 nuage aléatoire sur 40.
   - Seul écart pour HGP-old : des retards d'attache de facettes (24 facettes sur 40 nuages).
2. **Entrée des points.** La masse unité de x est répartie sur ses cofaces au prorata de ψ, puis par facette. Une facette ne compte qu'à partir de sa première coface catalogue. La première facette de x s'attache exactement à **α_{K+1}(x)**, et non à α_K(x). HGP-old est donc d'un ordre plus strict que le théorème 2 et que l'entrée `cover` de la v10.
3. **Tête.**
   - Condensation HDBSCAN ascendante par lots d'égalités float32, avec λ = ρ^(−z) et un seuil `min_cluster_size` (mcs) comparé à une masse fractionnaire.
   - Une condensation par composante connexe ; la racine est sélectionnable ; EOM à inégalité stricte.
   - Étiquetage par vote argmax des S_τ ; un point est du bruit si et seulement si aucune de ses facettes n'est étiquetée.
4. **Défauts.** K = 2, `min_samples` forcé à K+1, z = 2, mcs = round(√n), EOM, CGAL « safe », float32.
   - La thèse prescrit ψ = t^(−p) avec p la dimension **ambiante**.
   - Les notebooks suivent en partie cette règle (z = 8 pour les huiles d'olive, 4 pour Maya) ; le banc SIPU tourne en z = 1.
5. **Résultats.** Le tableau 9.3 est le tableau 4 d'ANS 2026, et ses conditions diffèrent du § 9.1 :
   - affectation dure « première facette » (ANS p. 11), et non le vote du § 9.1 ;
   - HDBSCAN à `min_samples = 3` ;
   - ARI « sur tous les points, bruit compris » (ANS note 6, p. 18), ce qui récompense la couverture.

   Le snapshot HGP-old (7 juillet 2026) n'est vraisemblablement pas le code de ce tableau.
6. **Explication des bons résultats.** Par ordre de poids : la sémantique des amas discrets, que la v10 confirme ; la convention d'ARI ; z = dimension ambiante et K petit. Pas les masses fractionnaires.

---

## (a) Filtration

### Thèse

- **Objet (Déf. 20-22, PDF 83-84 ; théorème 2, PDF 86-87).**
  - Γ_K(X, r) a pour sommets les (K−1)-simplexes τ de Čech(X, r), c'est-à-dire ρ(τ) ≤ r.
  - Deux sommets τ, τ' sont adjacents dès que τ ∪ τ' ∈ Čech(X, r). Le texte l'appelle lui-même « clique percolation ».
  - Théorème 2 : les K-polyèdres sont X ∩ δ_r(C), pour C une composante de L_K(r) = {y : |B(y, r) ∩ X| ≥ K}. Le point de départ est l'estimateur K-NN f̂_K ∝ r^(−p) de la Déf. 7 (PDF 45-46).
- **Adjacences élémentaires (Prop. 5, PDF 112).** Les (K+1)-parties suffisent.
- **Réduction par Gabriel.**
  - Théorème 4 (PDF 114-115) : un simplexe K-séparant est de Gabriel.
  - Déf. 29 (PDF 115-116) : sommets = facettes d'au moins un K-simplexe de Gabriel, donc F = ∂C ; une clique par simplexe, de poids ρ(σ).
  - Prop. 6 et théorème 5 (PDF 116-117) : faux en général (E5, audit v9).
  - Déf. 31 et théorème 6 (PDF 117-119) : tout Gabriel est « porté » par Del_K.
  - Algorithme 1 (PDF 126), pour K = 2 : Gabriel extrait de la Delaunay ; l'étape 4 relie « (deux de) ses trois arêtes-facettes ».

### Code HGP-old

- **Catalogue.** `orderk_delaunay3(M, min_samples − 1)` (`src/hgp_clusterer/hypergraph.py:67-77`).
  - Le C++ part des arêtes de Delaunay (`_geometry_binding.cpp:147-180`).
  - Il itère ensuite une triangulation régulière des barycentres des k-parties précédentes, avec les poids d'Aurenhammer |c|² − moyenne |q|² (l. 226-267). Il garde les unions de deux sommets adjacents de cardinal k+1 (l. 269-323) et dédoublonne (l. 349-401).
  - Niveau : miniboule² (CGAL Min_sphere, `CGALDelaunay/orderk_delaunay_cpp/src/kernels.hpp:172-186, 289-303` ; Welzl Geogram, `kernels_geogram.hpp:427-432`), puis `^(expZ/2)` et conversion en float32 (`hypergraph.py:119-128`).
- **Caractérisation (démonstration faite ici).** σ, de cardinal K+1, a une cellule de Voronoï d'ordre K+1 fermée non vide si et seulement si σ est porté par Del_K (Déf. 31).
  - Sens ⇒ : on rétrécit la boule vers un point le plus éloigné jusqu'à ce qu'un second point de σ touche la sphère ; au centre obtenu, σ∖{s} et σ∖{t} sont deux choix de K plus proches voisins.
  - Conséquences : l'itération énumère exactement les sites d'ordre K+1 (arithmétique exacte, position générale), et C_old ⊇ Gabriel (théorème 6). À K = 1, ce sont les arêtes de Delaunay de poids d²/4 (`_geometry_binding.cpp:182-219`).
- **Graphe dual.**
  - Les sommets sont toutes les facettes à K points des σ du catalogue, soit F = ∂C_old (`_cython.pyx:757-878`).
  - Chaque σ donne un **chemin** de K arêtes de poids w(σ), dans l'ordre des sommets retirés (`_cython.pyx:880-894`). Le repli Python fait de même (`hypergraph.py:247-273`).
  - Ce chemin donne les mêmes composantes que la clique à tout niveau, donc le même arbre condensé ; seules les arêtes de l'arbre couvrant diffèrent.
- **Arbre couvrant.** Kruskal sur `np.argsort` instable (`core.py:224-233`), avec arrêt anticipé (`_cython.pyx:218-226`). Chaque composante de la forêt est condensée séparément (`core.py:240-272`). En position générale, le graphe des facettes contient le graphe d'adjacence de Del_K : il est connexe, donc il n'y a qu'une racine.
- **Autres voies (hors défaut pour d ≤ 5).** Le choix `auto` (`hypergraph.py:44-58`) prend `orderk_delaunay` si d ≤ 5, ou si d ≤ 10 et n ≤ 1000. Sinon :
  - gudhi Rips, avec coupure de longueur d'arête et distance-cœur `max(filt, core)` (`hypergraph.py:160-192`) ;
  - DelaunayCech, qui ne prend que les K-simplexes de la Delaunay d'ordre 1.

  Ces deux voies ne calculent pas le même objet.

### Contrôle exact indépendant

Script `check_catalogue_old.py`, écrit depuis les mathématiques. Niveaux en `Fraction`, séparabilité par programme linéaire flottant à marge (aucune marge proche de 0). Nuages : les 3 fixtures E5, L11 et L06, plus 40 nuages aléatoires (n de 7 à 9, K = 2 ou 3, graine 20260929). Comparaison à chaque niveau critique.

| Mesure | Résultat |
| --- | --- |
| Ensembles de points des composantes non triviales, C_old contre FULL | égaux, 43 nuages sur 43 |
| Même comparaison, Gabriel (Déf. 29) contre FULL | faux sur E5 (1 niveau) et sur 1 nuage aléatoire sur 40 |
| Composantes C_old à cheval sur deux composantes FULL | 0 |
| Facettes attachées plus tard que leur première coface FULL | 24 facettes sur les 40 nuages |
| Première attache de chaque point à α_{K+1}(x) | 100 % des points |

C_old répare donc E5. Ce sont des indices sur petits nuages, pas une preuve.

## (b) Entrée des points, poids

- **Code.**
  - S_τ = Σ_{σ ∈ C_old, σ ⊃ τ} 1/w(σ) = Σ ρ(σ)^(−z), somme en float32, plafonnée à 1e12 quand w ≤ 1e−12 (`_cython.pyx:839-848`).
  - T_x = Σ_{τ ∋ x} S_τ (`core.py:205-208`).
  - m_τ = S_τ Σ_{x∈τ} 1/T_x, avec 1/T = 0 si T = 0 (`core.py:210-215`).
  - Forme close (dérivée ici) : w_xτ = S_τ/T_x = (1/K) Σ_{σ⊃τ} π_x(σ), où π_x(σ) = ψ(ρσ) / Σ_{σ'∋x} ψ(ρσ'). La masse de x se répartit donc sur ses cofaces au prorata de ψ, puis par 1/K sur les K facettes de chaque coface qui contiennent x.
  - Propriétés : Σ_τ w_xτ = 1 ; Σ_τ m_τ = nombre de points couverts ; m_τ ≤ 1, car T_x = K Σ_{σ∋x} ψ.
  - Quand z → ∞, on retrouve l'entrée dure à la plus petite coface.
- **Défaut.** `expZ = 2.0` (`core.py:51`). Le même z sert à ψ et à λ.
- **Datation.**
  - Chaque facette est un atome de masse m_τ présent dès le départ, mais isolé. Elle ne se connecte qu'à sa **première coface du catalogue**, et non à ρ(τ) comme le veulent les Déf. 21 et 25.
  - La première facette de x se connecte exactement à α_{K+1}(x) = min_{|σ|=K+1, σ∋x} ρ(σ). Par échange avec le Fait 12, ce minimiseur est de Gabriel, donc dans C_old.
  - D'où : x a une facette dans C au niveau r ⇒ x ∈ X ∩ δ_r(C ∩ L_{K+1}(r)), avec égalité à des retards d'attache près. Le théorème 2 dit X ∩ δ_r(C).
- **Thèse, § 9.1 (PDF 122-123, formules vérifiées sur l'image).**
  - S_τ = Σ_{σ⊃τ, |σ|=K+1} ψ(ρ(σ)), avec ψ(t) = 1/t^p « par défaut », p étant la dimension ambiante de X ⊂ R^p ; « toute fonction décroissante » est admise.
  - Les formules de T_x et m_τ sont identiques au code.
  - F_K y est dit « correspondre aux simplexes de Gabriel ».
  - SIPU utilise ψ = 1/t (PDF 127) ; « poser 1/r² » répare birch2 (PDF 129).
- **Paramètre ignoré.** `weight_face` est stocké mais jamais utilisé (`core.py:48, 68, 179-190`). Le wrapper hérité, commenté, avait trois modes (`core.py:393, 649-729`), dont `unique` : toute la masse d'un point sur sa facette de r minimal, c'est-à-dire l'entrée dure « première facette » à α_{K+1}.

## (c) Condensation

- Algorithme : `condense_tree_cython` (`_cython.pyx:399-659`).
  - Lots d'arêtes dont l'écart de poids reste ≤ `epsilon_fusion`, par défaut 0 (l. 469-480). Les fusions ne sont donc N-aires qu'aux égalités exactes en float32.
  - λ = 1/(w + 1e−12) = ρ^(−z) (l. 482-483).
  - Union par masse (l. 500-501).
- Traitement de chaque racine après un lot, si sa masse est ≥ mcs (l. 538) :
  - aucun cluster suivi : naissance d'une feuille ; ses membres entrent à λ_naissance (l. 539-557) ;
  - un seul cluster : prolongation ; les nouveaux atomes entrent à λ (l. 559-580) ;
  - au moins deux clusters : les enfants meurent, avec stabilité += Σ_join − n·λ, et un parent naît (l. 582-618).
- Stabilité : S(C) = Σ_{τ∈C} m_τ (λ_join(τ) − λ_mort(C)). La racine a λ_mort = 0 (l. 627-638).
- **mcs** est comparé à la masse fractionnaire Σ m_τ ; défaut round(√n_core) (`core.py:141-144`).
- **Composantes qui ne fusionnent jamais** : condensées séparément (`core.py:240-272`), chacune avec sa racine.
- **Précision.** Tout est en float32, et la stabilité est une différence de grandeurs voisines, donc sujette à cancellation.
- **Équivalence avec la v10.** Mêmes règles que la descente de la v10 (`CLUSTER_v2`, OP5), hors arithmétique et hors racine.

## (d) Sélection

- **EOM** (`clustering.py:157-228`) : les enfants ne gagnent que si Σ best(enfants) > stab(parent) strictement (l. 199-201). Les racines sont **sélectionnables** : le parcours part de `_roots_of_Z` (l. 212-227), sans exclusion.
- **Autres méthodes** : `leaf` (toutes les feuilles), et une coupe DBSCAN par un flottant exprimé en unités ρ^z (`clustering.py:368-401`).
- **`splitting`** : un rappel remplace la décision locale ; c'est le « hacker HDBSCAN » du § 5.2 de la thèse, PDF 70 (`clustering.py:638-758`). Dans le notebook MarieBenchmark, il juge la pureté contre la vérité terrain : c'est supervisé.
- **Thèse, Déf. 19 (PDF 64-65)** : Ê(C) ∝ Σ(λ̂_x − λ̂_min), avec λ̂_x = 1/r_x pour HDBSCAN. ANS p. 8 recommande ρ̂_x = r_x^(−d).

## (e) Étiquetage

- **Facettes.** Une facette reçoit l'étiquette du cluster retenu si son `initial_membership` est dans le sous-arbre de ce cluster (`clustering.py:358-363, 632-637`).
- **Points.** Vote argmax_c Σ_{τ∋x, ℓ(τ)=c} S_τ (`core.py:297-324`). C'est équivalent à Σ S_τ/T_x de la thèse ; à égalité, le plus petit indice gagne.
- **Bruit.** Un point est du bruit si et seulement si **aucune** de ses facettes n'est étiquetée (`core.py:321-324`). Autrement dit, x est étiqueté dès qu'une de ses facettes a rejoint le cluster retenu avant sa mort. C'est l'amas discret, couvert via L_{K+1}, au niveau de mort : l'étiquetage le plus généreux possible.
- **Options.**
  - `label_all_points` : remplissage par 1-plus-proche-voisin, désactivé par défaut (`core.py:365-381`) ;
  - sous-échantillonnage : propagation par 5-plus-proches-voisins (`core.py:346-363`).
- **Thèse** : Prop. 7 et Alg. 1 étape 9 (1-NN optionnel).

## (f) Défauts et écarts prose / code

**Défauts** (`core.py:41-60, 123, 141-149, 173-176`) :
- K = 2 ; `min_samples` absent ou ≤ K devient K+1 ; mcs = round(√n) ; `method='eom'` ;
- `expZ = 2` ; `precision='safe'` (prédicats exacts, constructions en double) ; backend `cgal` ;
- `label_all_points=False` ; `epsilon_fusion=0` ;
- coordonnées converties en float32 ; bruit N(0, 1e−5) ajouté avec le backend Geogram.

| Réf. | Écart | Prose | Code / pratique |
| --- | --- | --- | --- |
| D1 | Catalogue | Gabriel (§ 9.1, Alg. 1) | ordre-Voronoï K+1 : ⊇ Gabriel, même objet en points sur le contrôle, répare E5 |
| D2 | Connexion des facettes | clique (Déf. 21, 29) | chemin : mêmes composantes |
| D3 | Exposant | ψ = t^(−p), p ambiante | z = 2 par défaut ; 1 (SIPU, percolation), 2, 3 (Colab), 4 (Maya), 8 (huiles) |
| D4 | Échelle λ | 1/r pour SIPU ; 1/r² répare birch2 | 1/(ρ^z + 1e−12), même z que ψ |
| D5 | Datation d'une facette | ρ(τ) | première coface du catalogue ; première entrée d'un point à α_{K+1} |
| D6 | Exactitude | Prop. 6, théorème 5 | faux en général (E5) ; C_old non démontré, indices favorables |
| D7 | `min_samples` > K+1 | — | le catalogue a la largeur `min_samples`, mais le comparateur lit avec un pas K+1 (`_cython.pyx:38-43, 801`) et l'extraction lit par ligne (l. 865-867) ; la vérification est un simple `pass` (l. 776-777) ; le repli Python tronque (`hypergraph.py:255-256`). L'exemple du README (`min_samples=5, K=2`, l. 59-65) déclenche le défaut |
| D8 | Racine | « comme HDBSCAN » | sélectionnable |
| D9 | Bruit | Prop. 7 | bruit seulement sans aucune facette étiquetée |
| D10 | Unités de la coupe | — | ρ^z |
| D11 | Tableau 9.3 | présenté après le vote du § 9.1 | ANS p. 11 : « assign each point to the first (K−1)-simplex it enters » |
| D12 | Version du code | Alg. 1 (Gabriel depuis Delaunay) | snapshot HGP-old `eba0e123a` du 7 juillet 2026 : ordre-Voronoï et masses fractionnaires. ANS p. 24 : résultats K=2 issus du code « of the submitted version » ; SemanticKITTI et nanopore produits par un code optimisé non publié |
| D13 | Adversaire HDBSCAN | Déf. 16 : x compté, soit sklearn `min_samples = K` | notebooks : bibliothèque `hdbscan` à `min_samples = K+1`, soit sklearn K+2 (`sklearn/cluster/_hdbscan/hdbscan.py:597-601`). ANS SIPU : « min_samples = 3 » pour K = 2. Simulation du tableau 7.1 : K autres voisins, soit sklearn K+1 (`PercolationRate.ipynb`, `dists20[:, K]/2`). La Déf. 23 imprimée (\|B(x, 1/2)\| ≥ K, PDF 92) ne correspond ni à la remarque « K = 2 retire les feuilles » (PDF 100) ni au notebook |
| D14 | Huiles d'olive | ANS : mcs 20, centrage puis ℓ2 | notebook : mcs 15, `expZ=8`, ℓ2 sans centrage |

## (g) Résultats empiriques rapportés

- **Tableau 7.1** (PDF 100, protocole PDF 99). Vitesse de percolation v = λ̂_ε/λ̂_{1−ε}, avec ε = 3 %, T = 100 tirages, n = 100 000 points uniformes (10 000 en p = 4), r = 1/2.
  - En 3D, HGP passe de 0,563 à 0,732 quand K va de 1 à 5 ; le Robust Single-Linkage tombe de 0,563 à 0,399 ; DBSCAN monte de 0,563 à 0,625.
  - C'est un indice de percolation sur un processus de Poisson homogène, pas un score de clustering.
  - La partie K → ∞ (§ 7.5) repose sur un résultat admis et des conjectures.
  - Les cœurs y comptent un voisin de plus, et le notebook par défaut (n = 10⁶, T = 1, d = 2) n'est pas le protocole de la thèse.
- **Tableaux 9.1 et 9.2** (PDF 127-128). Huiles d'olive (572 × 8), K = 2, une seule exécution.
  - 8 groupes ; 412 points classés directement (72 %), dont 96,1 % justes.
  - Après extension 1-NN : 91,1 % justes, ARI 0,890, contre Persistable 0,848.
  - Aucun chiffre HDBSCAN ; ANS p. 18 dit seulement « 6 micro-clusters ».
- **Tableau 9.3** (PDF 128, identique au tableau 4 d'ANS). Conditions : 15 jeux SIPU en 2D, K = 2, mcs = √n, ψ = 1/t et λ = 1/r des deux côtés, EOM, HDBSCAN à `min_samples = 3`, ARI sur tous les points bruit compris, une exécution par jeu, 5 jeux exclus.
  - ARI : HGP meilleur sur 10 jeux, égal sur 4, pire sur birch2 (0,441 contre 0,996).
  - Part des points classés : plus élevée pour HGP sur 11 jeux, égale sur 2, plus basse sur 2 (birch2, worms_2).
  - ANS note 5 : ρ̂ = 1/r^d donne le bon nombre de groupes sur a1, a2, a3 et d31 (sans chiffres).
- **ANS tableau 5 (SemanticKITTI).** PQ_th de 0,876, 0,888 et 0,829 pour K = 1, 2, 3 : K = 3 fait moins bien que K = 1. Conditions : masques sémantiques oracle, coupe DBSCAN guidée par des a priori de classe (notebook : K = 3, mcs = 10, `EXP_Z = 1`), code non publié.
- **Log SemanticKITTI.** `tests/SemanticKITTI/logs/evaluation_errors_log.txt:36-44` donne LSTQ 0,825, sans paramètres liés.
- **README et FICHE_TECHNIQUE.** Aucun chiffre. La fiche affirme Prop. 6 et le théorème 5 (l. 21), faux en général.
- **Notebooks.** Aucune sortie numérique sauvegardée.

## HGP-old contre v10

| Aspect | HGP-old | v10 |
| --- | --- | --- |
| Objet | Γ_K sur C_old, float32, facettes reliées en chemin, retards d'attache | tour FULL exacte : rayons carrés rationnels, plateaux N-aires, une racine par ordre, verticales |
| Entrée d'un point | fractionnaire : w_xτ sur les facettes ; première attache à α_{K+1}(x) ; couverture via L_{K+1} ; masse partagée entre branches | `cover` : unité à α_K(x)² dans la composante de la première boule couvrante ; `core` : d_K(x)² |
| Masse | m_τ ≤ 1 par facette ; mcs sur masse fractionnaire | multiplicité entière par point |
| λ | ρ^(−z), z = 2, couplé à ψ | ρ^(−z) (`head.cpp:58-61`), z ∈ {1, ẑ global} ; pas de ψ |
| Condensation | ascendante, égalités float32 | descendante sur plateaux exacts (`head.cpp:65-151`) ; même sémantique |
| Racine et composantes | sélectionnable, une condensation par composante | racine exclue (`head.cpp:172-173, 193-194`), une seule racine |
| EOM | enfants seulement si somme strictement plus grande | même règle (`head.cpp:174-179`) |
| Étiquettes | vote S_τ ; bruit si aucune facette étiquetée ; 1-NN optionnel | lignée puis remplissage borné b(ρ) (`methods.py:97-114`) ; variante vote : boule couvrante de plus bas niveau (`mhgp10_cluster.cpp:148-175`) |
| ARI | tous points, bruit compris (ANS) ; hors bruit (notebook Colab) | ARI_s, bruit en singletons (`metrics.py`) |
| Adversaire | `hdbscan` à `min_samples = K+1` (sklearn K+2) | sklearn à `min_samples = K` |

## Ce qui explique les bons résultats de HGP-old

Lecture critique ; aucune de ces sources n'est un oracle.

1. **Sémantique des amas discrets.** Le chapitre 7 la justifie : HGP identifie les composantes avant de dilater, alors que HDBSCAN dilate avant d'identifier. HGP-old l'applique par son vote généreux.
   - La v10 la valide déjà : l'entrée `cover` gagne +0,05 à +0,10 sans remplissage, et +0,02 à +0,04 avec b(1,5).
   - C'est le seul mécanisme de HGP-old dont l'effet est établi hors de ses propres bancs.
2. **Convention d'ARI.** Compter le bruit comme une classe, sur une vérité SIPU sans bruit, récompense la couverture. ANS le reconnaît lui-même (note 6). La part des marges du tableau 9.3 due à cette convention n'est pas mesurable ici.
3. **z = dimension ambiante et K petit.**
   - Un z grand pousse l'EOM vers les feuilles ; z petit favorise les parents.
   - La thèse répare birch2, formé de groupes filiformes, en passant z à la dimension ambiante (2 en 2D).
   - Le diagnostic `shells` de la v10 va dans le même sens : z = 3 = p retient les 8 coquilles là où ẑ = 2,1 échoue.
   - Hypothèse à mesurer : z = 3 plutôt que ẑ ≈ 1 sur `filaments`. La grille en cours contient z = 3.
   - Tous les résultats rapportés sont à K = 2 ; sur SemanticKITTI, K = 3 fait moins bien que K = 1. Le paradoxe du choix de K n'est donc pas propre à HDBSCAN.
4. **Adversaire décalé.** HDBSCAN tourne à sklearn K+2 dans les notebooks. Sur le banc v10, sklearn s'améliore quand `min_samples` augmente ; ce décalage n'explique donc pas l'avantage de HGP-old.
5. **Ne l'expliquent pas.** Les masses fractionnaires (ANS a utilisé l'entrée dure, et le vote v10 égale l'étiquette par l'arbre suivie de b(1,5) à 0,002 près), la racine sélectionnable et la condensation par composante (une seule composante en position générale).

**Variante testable sans nouvelle géométrie.** On peut reproduire la sémantique d'ANS avec la tour : entrée à α_{K+1}(x), dans la composante de L_K obtenue par la verticale K+1 → K appliquée au nœud `cover` d'ordre K+1.

## Corrections aux documents existants

- **Audit v9, A9-67.** Le constat « Réfuté : même biais dans le tableau 9.3 » est à inverser. ANS (note 6, p. 18) déclare l'ARI calculé sur tous les points, bruit compris. Le notebook Colab, qui calcule l'ARI hors bruit, porte sur un autre banc et n'a aucune sortie sauvegardée.
- **`CLUSTERING_DEPUIS_LA_TOUR_20260929.md` § 2.** « `min_samples` non déclaré, bruit non déclaré » est vrai pour la thèse, faux pour sa source. Le fond de l'objection tient : une seule exécution par jeu, et un ARI qui récompense la couverture.
- **« Convention K+1 » (audit v9, A9-66).** Il y a en fait trois conventions, décrites en D13.

## Fichiers

Tous sous `/workspaces/E-HGP/build/v10-persist/audit_hier/lecteur_hgp_old_these/` :
- `check_catalogue_old.py` et `check_catalogue_old_40.txt` : contrôle exact du catalogue et sa sortie ;
- `page_30.txt` à `page_140.txt` et `these_p1_140.txt` : texte extrait de la thèse ;
- `ans2026.txt`, `cn2024.txt` et `biblio.txt` : texte extrait d'ANS 2026, de l'article Complex Networks 2024 et de la bibliographie.