# Base de connaissances pour la v10 de MorseHGP3D

Date : 28 septembre 2026. Destinataire : le concepteur de `morsehgp3D_v10`.

```text
phase=exploration_v9_hors_registre (audit) -> conception v10
backend=cpu_reference
profile=quantized_u18_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
GCP non utilisé pour cet audit
```

## 0. Mode d'emploi

### 0.1 Provenance

Ce document fusionne les sorties de quinze lentilles d'audit (L01 à L15) et les verdicts de leurs vérificateurs adverses.

- Arbre audité : `/workspaces/E-HGP/build/v9-open-worktree`, base `ce8a649dd`, diff non commis compris (`experiments/tower_clustering_20260928/{cluster.py,run_tower.py}` et `experiments/synthetic_bench_20260928/README.md`).
- Sauf mention contraire, les chemins sont relatifs à `morsehgp3D_v9/` dans cet arbre.
- Scripts, sorties et empreintes des mesures d'audit : `/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/morsehgp3D_v10/audits/audit_v9_20260928/preuves/<lentille>/`. Ils sont hors dépôt ; ce ne sont pas des reçus.
- Quand un vérificateur a corrigé une lentille, **la correction prévaut**. Les chiffres ci-dessous sont les chiffres corrigés.

Chaque énoncé porte un statut :

- **PROUVÉ** : théorème ou lemme avec preuve et fixture ;
- **MESURÉ** : reçu rejouable, ou mesure d'audit rejouée à l'identique ;
- **AFFIRMÉ** : prose sans reçu ;
- **FAUX** : réfuté par une fixture ou une mesure.

### 0.2 État observé du dossier v10 au moment de la rédaction

Pendant l'audit, `morsehgp3D_v10/` a été ouvert sur `main` : commits `d7a1459dd`, `a0d92fd6e`, `6b4b4ccee`, `93710d076`, `95109dac2`, plus un fichier non suivi `bench/synthetic/select.py`. Son README annonce déjà plusieurs choix que cet audit recommande :

- générateur par boîtes de centres, et abandon du WSPD ;
- doublons traités comme des multiplicités ;
- tête C∩X avec condensation HDBSCAN exacte, contre `sklearn.cluster.HDBSCAN` pris comme adversaire ;
- référence exacte `reference/hgp10_ref.py`.

Ce document **n'audite pas** cette v10. Il fournit la connaissance dont elle a besoin et les pièges à éviter. Chaque décision v10 doit se relire contre les sections 2, 6 et 7.

### 0.3 Verdict en douze points

1. **Objet.** L'objet v9 est sain et bien défini : pour chaque K, forêt de fusion exacte de π0(L_K(a)) avec niveaux rationnels, plateaux atomiques, multifusions, continuations datées et verticales. PROUVÉ sous régularité, et testé exhaustivement pour n ≤ 14 (T2).
2. **Complétude.** Elle n'est que relative au catalogue émis (`complete_relative`). Une zone aveugle est mesurée : les boules de fusion seule à l'ordre Kmax (q2 à p=Kmax−1, q3 à p=Kmax−2), soit 26 à 29 % du catalogue à K5. Des retraits artificiels y publient une tour fausse sans refus. Aucune omission naturelle du générateur n'a été observée.
3. **Générateur v9 (indexé par arêtes, front WSPD).** Il est exact, mais pas sensible à la sortie :
   - quadratique sur les amas séparés (≈ 0,44·n² paires développées) ;
   - cubique sur les coquilles creuses (compteurs) ;
   - linéaire sur les gaussiennes du banc et sur LiDAR à l'échelle des trames.
4. **Alternative prototypée et mesurée : énumération par boîtes de centres (CBLE).** Environ 450 lignes. Elle reproduit exactement les **comptes** du catalogue v9 sur 14 entrées et coûte 4 à plus de 100 fois moins de CPU. Deux limites : une contre-fixture cocirculaire la fait exploser sans refus, et elle ne produit ni identifiants ni condensés.
5. **Tour v9.** Correcte mais lourde :
   - Builder de 3 097 lignes en en-tête seul ;
   - plusieurs chemins équivalents ;
   - 224 o par boule et 6,4 Go de RSS à K10 pour 40k sites ;
   - chemin critique sériel par ordre.
6. **Clustering du 28 septembre.** Il **ne lit pas la tour FULL**. Il reconstruit un arbre de Kruskal sur les seules cofaces Gabriel, réintroduisant la régression E5 corrigée la veille. C'est la cinquième récidive de cette erreur depuis la v3.
7. **« beats HDBSCAN's oracle » (cda636b5e).**
   - Vrai à la lettre sur 34 couples sphériques, n ≤ 2000.
   - Faux sur l'objet (ce n'est pas FULL).
   - Gonflé par une condensation défectueuse.
   - Mesuré contre un oracle restreint à `min_samples = min_cluster_size`, que HDBSCAN(`min_samples`=2, `mcs`=√n) bat lui-même.
   - Sans reçu versionné.
8. **Condensation.** `condense()` s'écarte de HDBSCAN quand un cluster se brise en enfants tous sous le seuil : la descente continue au lieu de terminer le cluster. Le correctif tient en une ligne ; il redonne sklearn à l'identique à K=1 (40/40).
9. **Meilleure tête mesurée, sans vérité terrain.** EOM à l'échelle de densité λ = r^(−z) avec z ≈ 3, `min_samples` 2 ou 3, `mcs` = √n, et remplissage du bruit : 0,831 à 0,839 d'ARI contre 0,683 pour l'oracle du banc. Le gain vient surtout du remplissage. Elle tourne sur la bifiltration k-NN, pas sur la tour.
10. **Aucun gain de la géométrie exacte de la tour n'est établi à K fixé.** La tranche π0(L_K) et le graphe d'accessibilité mutuelle d'HDBSCAN sont entrelacés à un facteur 2 sur le rayon.
11. **Méthode.**
    - CI v9 rouge pendant 40 poussées ;
    - reçus perdus sous `/tmp` ;
    - nuages KITTI versionnés malgré l'engagement ;
    - documentation-journal : 72,6 k lignes de Markdown pour 33,5 k lignes de code ;
    - 31 leviers booléens.
12. **Performance G4.** « K5 < 1 s » est MESURÉ, mais sur un chrono interne (`chain_total`). Le mur processus vaut 1,5 à 1,9 s. 100 ms est hors de portée des algorithmes connus.

---

## 1. Spécification exacte et implémentable de l'objet

### 1.1 Entrée et domaine numérique v9

- Sites X ⊂ Z³, coordonnées dans [0, 2^18) = [0, 262 143] (grille 1 mm, « u18 »). Refus à 262 144. Contrôle d'empaquetage : la collision (0,1,0)/(0,0,65536) doit être détectée (fixture v9 `tests/tower/arith_u18_gate.cpp`).
- `PointId` = rang d'entrée dans la chaîne. La v5 et la v4 exigeaient des `PointId` u32 arbitraires, distincts du rang Morton : c'est la porte `--relabel-gate`, à reprendre.
- **Doublons refusés en v9** (`src/gen/pipeline/prepared_cloud.cpp:71-76`, clé 54 bits triée). Or la spécification racine compte avec multiplicité (`docs/SPECIFICATION_MORSEHGP3D.md:57-67, 91`). Le banc synthétique refuse une scène en présence de doublons (`experiments/synthetic_bench_20260928/bench_datasets.py:313-314`). Recommandation v10 : **multiplicités**. Le compte d'intérieurs est pondéré par la multiplicité et la coquille est faite des positions uniques, comme les buckets v4. Il faut graver une fixture de doublons.
- n ≥ 2 et K_eff = min(Kmax, n), avec Kmax ≤ 10 (`kFacetMaxK = 10`). L'intérieur d'une boule du catalogue compte au plus 9 sites (`kBallInteriorMax = 9`, `src/tower/forest/ball_data.hpp:21`), ce qui découle de la fenêtre avec q_min ≥ 2.
- **Coquille > 12 sites** : la v9 refuse **toute la chaîne** (`kUnsupportedDegeneracy`, raison `chain_shell_above_12`, `src/chain/tower_chain.cpp:940-943`, `src/tower/forest/local_plateau.hpp:47-48`). Ce n'est pas un théorème. La sphère entière de rayon 5 porte 30 points entiers, et la v8 a exercé des coquilles de 30 sites (`docs/FAUSSES_PISTES.md:69`).
- **Séparation WSPD s ≥ 8** : refusée en entrée (`tower_chain.cpp:1326`). Elle n'entre dans **aucun** certificat ; ce n'est qu'un réglage de granularité (voir 5.1 pour la directive utilisateur).

### 1.2 Objet continu et modèle discret

- Niveaux : rayons **carrés** a = r², rationnels exacts.
- Multicouverture : $L_{K}(a)=\lbrace y\in\mathbb{R}^{3} : \lvert X\cap \bar{B}(y,\sqrt{a})\rvert\geq K\rbrace$.
- Inclusions : $L_{K}(a)\subseteq L_{K}(b)$ pour $a\leq b$, et $L_{K+1}(a)\subseteq L_{K}(a)$.
- La sortie est le foncteur π0 sur $\lbrace 1,\dots,K_{\mathrm{eff}}\rbrace^{\mathrm{op}}\times\mathbb{R}$ (`docs/SPECIFICATION_MORSEHGP3D.md:85-103`).
- **Modèle discret Γ_K(a)** (manuscrit, Th. 2, pages PDF 86-87 ; Prop. 5) :
  - sommets = K-sous-ensembles F tels que β(F) ≤ a, où β est le rayon carré de la miniboule (MEB) ;
  - arêtes entre F et F′ si $\lvert F\cup F'\rvert=K+1$ et $\beta(F\cup F')\leq a$ ;
  - coupe ouverte : on remplace ≤ par < ;
  - les composantes de Γ_K(a) sont celles de L_K(a). Preuve de type nerf : les régions témoins sont convexes compactes.
- **FULL** garde les facettes isolées ; `hgp_reduced` les retire, sauf à K=1. La v9 calcule FULL.
- K=1 : n singletons au niveau 0, dans l'ordre des `PointId`. Les boules q2 à p=0 réalisent l'EMST (registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md:48-49`).

### 1.3 Catalogue de boules

Une entrée par boule **distincte** : type `tower::BallData`, `src/tower/forest/ball_data.hpp:24-33`, 224 o en v9.

**Clé primitive** (A, Bx, By, Bz, C) : 5 × i128, A > 0, pgcd 1, commune à toutes les arités. La sphère vérifie $A\lVert x\rVert^{2}+B\cdot x+C=0$. Son centre vaut $-B/(2A)$ et $r^{2}=\lVert B\rVert^{2}/(4A^{2})-C/A$. Un site x est strictement intérieur si et seulement si $A\lVert x\rVert^{2}+B\cdot x+C<0$.

- Contrôle fait sur la fixture Euler : D = (1,−20,0,0,0) donne le centre (10,0,0) et r² = 100 ; T = (4,−880,−39,0,48000) donne le centre (110, 39/8, 0) et r² = 7921/64.

**Niveau** : r² exact (en v9, `ExactLevel` = U192/i128 non réduit, 48 o). Deux égalités de représentation ne sont pas des égalités de valeur (`lanes/level.hpp:9-11`) : comparer par produits croisés.

**Support** : q_min ∈ {2, 3, 4}, la plus petite arité d'un support **positif**, c'est-à-dire un support dont le centre est dans l'intérieur relatif de son enveloppe convexe. q_min est recalculé sur la coquille (`tower_chain.cpp:974`).

**I** = sites strictement intérieurs, avec p = |I| ≤ 9. **U** = coquille complète, avec u = |U| ≥ q_min.

**Admission** : $p+q_{\min}\leq\min(K_{\max}+1,n)$ (`tower_chain.cpp:990, 996`). Ce sont exactement les points critiques d'indice ≤ 1 des fonctions distance-au-k-ième-voisin pour k ≤ Kmax (Reani–Bobrowski ; `docs/math/CATALOGUE_CRITIQUE_3D.md` §1-3).
- Seuils de rejet par arité : $h_{q}=K_{\max}+2-q$, avec p < h_q (`src/gen/wspd/front.hpp:151` ; PROUVÉ, `docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md` §5.3.1).
- **Attention** : p+u ≤ Kmax n'est **pas** une règle d'admission. La coquille à 7 points donne une naissance à K5 (voir fixture).

**Calendrier** : la boule B intervient aux ordres $K\in[p+q_{\min}-1,\min(K_{\max},p+u)]$ (`src/tower/forest/full_ball_tower.hpp:1474-1475, 2016-2033`).

**Recensement exact** : la chaîne recalcule clé et niveau depuis le support, recense I et U sur l'index, et exige :
- les mêmes profondeur et coquille que le générateur ;
- pour une coquille régulière, coquille = support positif déclaré (R-29, `tower_chain.cpp:944-968`).

### 1.4 Contenu d'un bloc (B, K)

**Boule régulière** (u = q_min, coquille = support) :
- à K = p+u : **naissance** sans représentant, avec la contribution I∪U ;
- à K = p+u−1 : les u représentants (I∪U)∖{s}, pour s ∈ U, qui sont des facettes à résoudre (`count_block_at`/`visit_block_at`, `full_ball_tower.hpp:2843-2910`).

**Coquille étendue** (u > q_min) : quotient local `ShellTable::rank(K)` (`src/tower/forest/local_plateau.hpp:100-165`).
- Si K ≤ p : un hub analytique, une seule composante qui couvre tout.
- Sinon, on pose t = K − p. Les sommets sont les t-sous-ensembles T ⊆ U **stricts** (c ∉ conv T), reliés en étoile par les (t+1)-sous-ensembles stricts.
- Contribution = U ∖ ∪(couvertures strictes), plus l'indicateur « intérieur » si aucune composante stricte et p > 0. Ce n'est pas un delta global disjoint.
- Coût O(u·2^u), d'où le plafond 12.
- Levier v10, non encore gravé : ces composantes strictes sont celles de l'ensemble $\Lambda_{t}=\lbrace v\in S^{2} : \lvert\lbrace x\in U : \langle v,x-c\rangle>0\rbrace\rvert\geq t\rbrace$, le lien inférieur de la note C d'Euler. Un arrangement exact de u grands cercles, en O(u²) cellules, lèverait le plafond. Il faut graver l'équivalence en fixtures (supports multiples, antipodes) avant de remplacer la table.

### 1.5 Résolution d'un représentant F (|F| = K)

La résolution se fait dans l'état **antérieur au lot** (`full_ball_tower.hpp:2772-2819`, `static_terminal` :2095-2146) :

1. MEB exacte de F. La v9 la propose en double par Welzl, puis la vérifie en exact.
2. Si la clé de cette MEB a une ancre A[K, B] (K dans la fenêtre de B) : on retourne la racine normalisée d'avant le lot.
3. Sinon, on cherche un intrus strict z : parcours en profondeur depuis la racine de l'index (`intruder_work`, :2061-2086). Si aucun intrus n'existe, c'est un refus `full_ball_missing_weak_terminal`.
4. On échange un sommet du support contre z. Le couple (rayon, |coquille retenue|) décroît **lexicographiquement**. La décroissance stricte du seul rayon est FAUSSE : fixture `actual_equal_radius_descent`, contrôle :2814 (−1 exactement à rayon égal).
5. La descente s'achève sur une ancre ou un semis (I∪U des boules avec K = p+u).

Preuve : `morsehgp3D_v7/audits/receipts_plateaux_full_20260906/BALL_ANCHORS.md` §§2-5. Les K-sous-ensembles de I∪U sont connexes après fermeture (graphe de Johnson connexe), et une facette de MEB B est incluse dans I∪U.

Un terminal manquant est un **refus**, jamais un saut. La descente lexicographique est **un** moyen correct, pas une obligation : tout terminal géométrique fermé de la fenêtre convient.

### 1.6 Lots, naissances, continuations, multifusions

- Les boules de niveau **exactement égal** forment un lot. Tous les représentants sont résolus dans l'état d'avant le lot. Les blocs sont ensuite regroupés par racines pré-lot partagées, avec un petit union-find par lot (`order_lot`, :1277-1352).
- Chaque groupe devient une action :
  - 0 racine : **naissance**, un seul bloc, une contribution ;
  - 1 racine : **continuation**, contribution sans nœud, ou bloc inerte ;
  - ≥ 2 racines : **multifusion**, **jamais binarisée**.
- Les ancres ne sont installées **qu'après** la fermeture du lot (`close_lot`, :2946-3030).
- Validité du regroupement : deux boules de même niveau ne partagent aucune facette nouvelle (unicité de la miniboule), donc elles ne se relient que par des racines antérieures au lot.
- Continuation : fixture ABCZ (`growth_ABCZ`, `audits/b_full_continuation_origin_20260927`). Mesuré sur LiDAR et uniforme : naissances = contributions, aucune continuation (`audits/b_full_a_real_20260927/RESULTATS.md`). C'est OBSERVÉ, pas prouvé ; la voie générale reste obligatoire.

### 1.7 Verticales

- Naissance d'ordre K : image = ancre A[K−1, B], normalisée à la coupe **fermée** du niveau de création (`full_ball_tower.hpp:2965, 3019`). Cette ancre est déjà racine à la fin de son lot, donc aucune requête union-find n'est nécessaire (L04-F11).
- Multifusion : racine commune des images des parents, contrôle `full_ball_vertical_naturality` (:2936).
- Requête : normalisation dans l'histoire de l'ordre K−1 inférieure à la coupe demandée.
- Suffisance « une ancre par naissance » : `conditional_theorem` (registre l.134). À l'échelle, seule la naturalité aux multifusions est contrôlée. **Aucun juge d'échantillon des verticales à l'échelle** : il faut en ajouter un.

### 1.8 Sortie par K (v9 : `FullCoverageCertificate`, `full_coverage_certificate.hpp:231-265`)

- Nœuds : niveau, parents en CSR, successeurs.
- Contributions datées : population, masque de coquille u16, drapeau « intérieur ».
- `lower_nodes` : image dans l'ordre K−1 à la coupe fermée du niveau de création.
- Banque de populations partagée. Défaut d'alias mutable ouvert (`full_coverage_certificate.hpp:95`) : ne pas exposer de banque adoptée par déplacement.
- **Publier aussi `anchor[K][bloc actif]`** (u32). La v9 le jette (`full_ball_tower.hpp:222-227`). C'est l'interface exacte dont le clustering a besoin pour rattacher facettes et cofaces à leur composante FULL.

### 1.9 Invariants à l'exécution (à conserver)

- **Une seule composante finale par K** (`full_ball_tower.hpp:677, 1402-1404`).
- **Euler** : pour K ≤ min(Kmax−2, n), $E_{K}=n\cdot[K=1]+\sum_{B}e_{K}(B)=1$, sinon refus `chain_catalogue_euler_violated` (`tower_chain.cpp:2024-2039`).
  - $e_{K}(B)$ est le coefficient de $t^{K-1}$ dans $t^{p}\sum_{T\subseteq U,\ c\in\mathrm{conv}(T)}(t-1)^{\lvert T\rvert-1}$ (`tower_chain.cpp:866-873, 967, 976-989`).
  - C'est une condition **nécessaire seulement**.
- Ancre cible strictement antérieure et fermée ; racine antérieure au lot ; naissances distinctes, une contribution par naissance ; décroissance lexicographique de la descente ; naturalité verticale.
- Échec **transactionnel** : aucune tour partielle publiée, priorité au plus petit K en cas d'échecs multiples.
- Sortie bit-identique quel que soit le nombre de fils : porte `mhgp9_chain_static_paths` (0/1/4/8 fils, même `tower_digest`).

### 1.10 Statuts

- v9 : `complete_relative_to_cross_checked_catalogue[_sealed_in_process_census|_payload]` (`tower_chain.cpp:42, 2113-2118`), `kUnsupportedDegeneracy`, `kInvalidInput`, `kResourceExhausted` (refus GPU de capacité, sans repli), `kInvariantViolated`.
- Défauts constatés :
  - tout `std::invalid_argument`, même interne, devient `kInvalidInput` (`tower_chain.cpp:2125`) ;
  - les statuts `exact_full_regular` et `exact_full_quotient_certified` promis par `docs/PLAN_V9.md:109-111` n'existent pas ;
  - dix énumérations de statut coexistent.
- Recommandation v10 :
  - un seul statut typé, avec des codes de raison énumérés ;
  - publier séparément trois choses : l'exactitude arithmétique, la reconstruction relative au catalogue et la complétude des clés (juges) ;
  - publier les compteurs de boules à coquille étendue, car elles dépendent d'une extension non registrée (135 à 572 par trame à K5) ;
  - ne jamais écrire « complet » sans « relatif ».

### 1.11 Digest

`tower_digest` (FNV-64, `tower_chain.cpp:1249-1282`) hache la représentation **brute non réduite** du niveau du premier bloc de chaque lot. `catalogue_digest` hache des indices en rang Morton, pas des `PointId`. Ce sont des digests de représentation, pas des digests sémantiques.

Recommandation : digest canonique v10 (niveaux réduits, `PointId`, numérotation par (K, rang, clé minimale du groupe)), plus un exporteur de compatibilité v9 séparé pour les campagnes appariées.

Épingles LiDAR v9 utiles au différentiel (08/000000, K5) : tour `67450c64611075b1`, catalogue `5ad1fe09354411ba`. Épingles brutes : b00 K5 `11f6a8e1a7f28127`, K10 `1a315a5241510296`.

---

## 2. Théorèmes invocables et fixtures d'égalité

### 2.1 Ce qu'on peut invoquer

| Énoncé | Statut | Référence |
| --- | --- | --- |
| Th. 2 : K-polyèdres ≡ amas discrets K-NN ; Prop. 5 : adjacences élémentaires suffisent ; Th. 4 : simplexe séparant ⇒ Gabriel (position générale) | `theorem_external` | registre l.30, 38-39 ; manuscrit PDF 86-89 |
| Fenêtre, indice et multiplicité de Reani–Bobrowski : seuls les rangs s ∈ {K, K+1} modifient H0 | `theorem_external` (position générale) | registre l.42-46 |
| Rang K+1 ⇔ K-simplexe de Gabriel ; l'étoile remplace la clique | `proved_here` | registre l.47, 95 |
| Attache silencieuse d'une coface non-Gabriel = continuation q=1 ; Gabriel + toutes les attaches reconstruit Γ | `proved_here` | registre l.115, 117 |
| Th. 4.2 : inertie H0 si p+q ≥ K_eff+2, même irrégulière | `proved_here` | registre l.120 ; `docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md` §5.3.1 |
| Naissances FULL = minima Gabriel de cardinal K ; minima + multifusions reconstruisent FULL ; portails ; une ancre par naissance suffit | `conditional_theorem` **sous régularité** | registre l.131-134 ; `morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md` §1.1, §6 |
| Tour saturée S.1-S.6 : les saturés engendrent Čech ; forêt de Kruskal de poids maximal ; sans position générale | `proved_here` | registre l.144-148 ; `docs/math/TOUR_BOULES_SATUREES.md`. Excellent second oracle borné |
| Identité d'Euler par le nerf : $\chi(L_{K}(r))=\sum_{J}(-1)^{\lvert J\rvert-K}\binom{\lvert J\rvert-1}{K-1}\mathbf{1}[\rho(J)\leq r]$ | preuve finie, **absente du registre** | `audits/CONTRELEC_EULER_PAR_NERF_20260923.md` |
| Lemme de la première cofacette : une omission isolée d'ordre haut p+u ≤ Kmax est refusée par la tour (575/575 retraits refusés à 8k) | conditionnel, non inscrit | `audits/LEMME_PREMIERE_COFACETTE_OMISSION_20260923.md` |
| Lemmes des voies q3/q4 sans atlas (L1-L11, L15, suffisance du cover) | `proved_here`, locaux à une arête certifiée | registre §V9-S4, l.1265-1288 |
| Lemme du citron : $\lVert c-m\rVert^{2}\leq D/12$ (q3 aiguë), $\leq D/8$ (q4 positive) ; α3 = 3, α4 = 2 | preuve en prose v8, refaite en audit | `src/gen/lanes/q34_dead_lanes.hpp:31-38` ; `docs/audit_v8/11_objet_et_preuves.md` §3 ; **à inscrire** |
| Théorèmes v8 : H (rangs), frère, Pool terminal, familial, corde, fenêtre [L,U], couches duales, atlas q3, collectif d'arête | preuves dans les notes `morsehgp3D_v8/docs/P0_*` | **non consolidés au registre** |
| Couche aval : échelle multi-ordres λ_z(k,a) décidable exactement ; routage laminaire et antichaîne EOM ; m_τ ≤ 1 sous F = ∂C | `proved_here` (l.31, 33-34, 151) ; réduction DSU `conditional_theorem` (l.32) | `docs/math/HIERARCHIE_DE_POINTS_MULTI_ORDRES.md` |
| Borne de sortie Ω(N²) feuilles FULL à K fixé ≥ 2 | prouvée v7 | `morsehgp3D_v7/docs/CROISSANCE_ET_BORNE_DE_SORTIE.md` |
| Encadrement de la première couverture : $d_{K}(x)/2\leq\alpha_{K}(x)\leq d_{K}(x)$ (rayon, x compté) | preuve courte | `audits/b_point_hierarchy_k_20260927/README.md:52-57` ; **à inscrire** |
| Entrelacement tranche/HDBSCAN : si $\mathrm{mreach}_{K}(x,y)\leq\varepsilon$ alors $[x,y]\subseteq L_{K}((3\varepsilon/2)^{2})$ ; les K sites d'une boule de rayon r sont à accessibilité mutuelle ≤ 2r | argument d'audit (L14, corrigé par son vérificateur) : facteur 2 en rayon, 4 en a | **à rédiger et inscrire** avant usage ; il borne la distance entre arbres, pas l'ARI d'une sélection EOM |
| Lemme de complétude CBLE (gardes et dominateurs) | preuve élémentaire (§3.5) | **à inscrire** avec fixtures et mutants ; il faut un second lemme pour l'exactitude du compte p |

### 2.2 Faux en général (ne jamais réutiliser)

- Prop. 6 et Th. 5 du manuscrit (graphe de Gabriel élagué, K-MST) : registre l.40-41, fixture E5.
- Flot des seules cofaces Gabriel de cardinal K+1 ⇒ FULL (l.129). Flot de Gabriel brut ⇒ `hgp_reduced` (l.98).
- Unions de points comme invariant (l.128).
- Minima Gabriel avec leurs seules adjacences induites : deux fixtures à 4 points (§2.4).
- Catalogue Gabriel de fenêtre ≡ catalogue d'ordre-Voronoï pour les poids du §9.1 (l.153).
- Troncature des saturés à K+1 ; fold v4 des dix forêts ; MST d'accessibilité mutuelle comme HGP pour k ≥ 2 (l.71).
- Formule générique « naissance à p+u / fusion à p+u−1 » sur une coquille étendue : le carré à 4 points donne une seule naissance (`audits/ERRATUM_B_AUDIT_C_OBJET_ET_EULER_20260923.md` §1).
- Décroissance stricte du rayon dans la descente (fixture `actual_equal_radius_descent`).
- Plafonds présentés comme théorèmes : coquille ≤ 12 (« par théorème » dans `morsehgp3D_v5/README.md:34-36`), degré q2 ≤ 12, pile Morton ≤ 8.
- Euler plus Kmax+2 (restriction clé par clé) comme preuve de complétude : contre-exemple exact à 13 points (§2.4).
- α3 appliqué à q4, témoin non strict, citron sans arête maximale ni positivité, propagation d'un compte au lieu des rangs, bornes aux seuls coins (F1), Z restreint aux graines (F9), racines q4 comparées par produits de degré 9 (il faut le déterminant de degré 5), carte des centres à Q = 2^44 en 18 bits (`morsehgp3D_v9/docs/FAUSSES_PISTES.md`).

### 2.3 Trous du registre (à combler avant d'invoquer)

- Extension non régulière (quotient de coquille, ancres par (K, BallKey), contributions datées, descente à rayon égal). Esquisse B1-B8, oracle sur 33 nuages dégénérés, **aucune entrée**. Elle est pourtant active sur toutes les trames : 135 à 572 coquilles étendues par trame à K5.
- Complétude du générateur (front, témoins, Pool, citron, atlas, inductions q3/q4) : dispersée dans les notes v8. Le registre dit lui-même « ni preuve de complétude des clés jamais proposées » (l.1288).
- Invariant d'Euler, lemme de la première cofacette, lemme de la demi-boule diamétrale (utilisé par `tests/chain/q3_sample_judge.cpp:12`).
- La ligne l.151 attribue au moteur v9 la convention « facettes Gabriel seules ». `measure.py:28` et le commit 268ad5a80 l'attribuent en plus à la thèse. Les deux attributions sont FAUSSES :
  - Déf. 29 (PDF 115, imprimée 89) : sommets = facettes d'au moins un K-simplexe de Gabriel ;
  - Alg. 1, étape 3 (PDF 126, imprimée 100) : même définition ;
  - HGP-old (`hypergraph.py:245-266`) : K+1 facettes par simplexe.

  La convention de la thèse est donc F = ∂C. La phrase du §9.1 (PDF 122), « F_K correspond aux simplexes de Gabriel », est ambiguë et explique la mauvaise lecture.

Recommandation : un registre v10 court et autonome, d'une vingtaine d'énoncés. Une ligne de tableau par énoncé : statut, prémisses, fixture d'égalité, porte.

### 2.4 Fixtures d'égalité à graver (coordonnées du dépôt)

Toutes les coordonnées sont entières. K est l'ordre, Kmax l'ordre maximal de la tour testée.

**E5**
- Fichier : `tests/fixtures/regressions/gabriel_point_set_counterexample.json`, id `gabriel-point-set-counterexample-5-points-v1`.
- Points : A=(0,0,7), B=(0,9,6), C=(1,4,0), D=(0,0,1), E=(4,1,2).
- À K=2 et a = 83886/3563 (coupe fermée) : Γ_2 a une seule composante de 8 facettes sur {A..E} ; le graphe Gabriel donne {AB, AC, BC} et {AD, AE, CD, CE, DE}.
- Témoin : AC est attachée dès 33/2 par les cofaces non-Gabriel ACD et ACE ; la fusion Gabriel est retardée à 24.
- Cofaces Gabriel et niveaux : ABC 83886/3563, ADE 189/17, BCD 53/2, BCE 24, CDE 162/25.
- Porte obligatoire sur **tout** producteur ou consommateur de hiérarchie.

**Régression silencieuse A–E** : A=(0,1,0), B=(2,5,0), C=(4,1,0), D=(1,0,0), E=(3,0,0), Kmax=2. Parents perdus sans portails silencieux (`morsehgp3D_v8/audits/FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md`).

**Graphe induit des minima**, deux fixtures à 4 points :
- A=(1,1,7), B=(5,2,1), C=(7,2,2), D=(5,2,8) : la fusion vraie 477/34 est manquée ;
- fixture de l'auditeur v7 : fusion 169/9 retardée à 41/2 (`morsehgp3D_v7/docs/SQUELETTE_MINIMA_GABRIEL.md`, `tests/full_gabriel_minima_quotient_gate.py`, `morsehgp3D_v7/audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md`).

**Carré cosphérique de côté 2** : (0,0,0), (2,0,0), (2,2,0), (0,2,0).
- Euler $e_{K}=(1,-3,1,1)$.
- À K2 : quatre parents et aucune naissance diagonale (`tests/tower/full_coverage_certificate_gate.cpp:183`, `local_plateau_gate.cpp:206`, où la couverture fermée de rang 3 vaut 15).

**Triangle rectangle** : (0,0,0), (2,0,0), (0,2,0), (0,0,10).
- Niveau 2, support minimal {1, 2}, coquille {0, 1, 2}.
- Attendu : quotient certifié ou refus explicite, jamais une acceptation silencieuse (`tests/fixtures/regressions/delaunay_gabriel_unsupported_degeneracy_right_triangle.json`).
- Voir aussi AB/ABC et la matrice `gabriel_carrier_strict_interior_extra_shell_matrix.json`.

**Coquille à 7 points** (`shell7_window`) : (10,5,0), (0,5,0), (5,10,0), (5,0,0), (8,9,0), (2,1,0), (9,8,0), Kmax=5. Elle donne une naissance à K5 alors que p+u > smax : le filtre p+u ≤ smax est faux, seul p+q_min ≤ smax est démontré.

**Fixtures de la porte de la tour** (`tests/tower/full_ball_tower_gate.cpp:128-151`) :

| Nom | Points | Kmax |
| --- | --- | --- |
| `pair` | (0,0,0), (2,0,0) | 2 |
| `square` | ci-dessus | 4 |
| `growth_ABCZ` (continuation) | (1,8,0), (5,10,0), (9,8,0), (5,0,0) | 4 |
| `growth_redundant` | idem + (10,6,0), (9,1,0) | 4 |
| `inert_ball` | (2,2,2), (2,0,0), (0,2,0), (0,0,2), (0,0,0) | 5 |
| `support3` | (0,0,0), (2,2,0), (2,0,2) | 3 |
| `support4` | idem + (0,2,2) | 4 |
| `u16_tetra` | (0,0,0), (65534,65534,0), (65534,0,65534), (0,65534,65534) | 4 |
| `u18_tetra` | même motif avec 262142 | 4 |
| `u18_corners` | (262143,0,0), (0,262143,0), (0,0,262143), (262143,262143,262143), (131071,131072,1) | 5 |
| `portal_equal` | (0,2,0), (2,4,0), (4,2,0), (2,0,0), (2,2,0), (2,1,0) | 6 |
| `actual_equal_radius_descent` | (35,100,0), (139,48,0), (152,139,0), (48,139,0), (100,36,0), (101,37,0), (99,37,0), (100,200,0) | 4 |
| `growth_ABCZ_doubled_lot` | ABCZ + la même figure décalée de +200 en x | 3 |
| `inert_ball_doubled_lot` | inert_ball + (100,100,100), (102,102,102) | 3 |

Plus les cas l.569-573 (`present_but_wrong_rank`, `post_seed_square_partial`). La porte fait tourner ces fixtures avec des `PointId` épars (u32 max, 17, 0, 902, 2^31, …).

**Contre-exemple Euler + Kmax+2** : 13 points u18 (`audits/CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md`).
- Boule D : support (0,0,0), (20,0,0) ; intérieur (8,1,1), (9,2,2), (11,1,3), (12,3,1).
- Boule T : support (100,0,0), (120,0,0), (110,16,0) ; intérieur (108,4,1), (110,4,2), (112,5,1), (109,6,2).
- Polynômes $-t^{4}+t^{5}$ et $t^{4}-2t^{5}+t^{6}$ : omettre D à K5 et K7 et T à K7 passe Euler et la restriction K7→K5 clé par clé. La version à 23 points fait de même pour K12→K10.
- La **tour** refuse ces omissions (8/8, par la connexité finale ou `missing_weak_terminal`), mais c'est un effet de petite taille.

**Gabriel contre ordre-Voronoï** : (0,1,0), (4,1,0), (1,2,0), (1,0,0), K=2, z=2 (`experiments/point_dendrogram_20260927/test_catalogue_counterexample.py`, porte `mhgp9_non_gabriel_incidence_counterexample_v1`, mutant `single_ratio`).
- $T_{\mathrm{Gab}}=(2,18/25,68/25,68/25)$ ; $T_{\mathrm{Vor}}=(3,43/25,161/50,161/50)$.
- $m_{CD}=1$ contre $136/161$ : trois rapports de masse distincts.

**Témoin de convention « gabriel »** (L11) : (0,9,0), (24,9,0), (12,27,0), (12,0,0), K=2.
- Cofaces [0,1,2] à 169 et [0,1,3] à 144 ; la facette {0,1} a la boule 144 avec le point 3 intérieur (non-Gabriel).
- Convention `gabriel` : 2 racines définitives. Čech et FULL : connexes dès 169.

**Témoin à 5 points** (L06) : (4,11,5), (5,2,5), (8,2,2), (11,3,1), (11,5,1), K=2. La facette de Gabriel (0,4) n'est couverte que par la coface (0,2,4), dont les autres facettes ne sont pas de Gabriel : 2 racines contre 1 pour FULL.

**Plateau à trois cofaces** (combinatoire, L08) : (0,1,3), (0,2,4), (0,3,4) au même β = 4. Toutes les permutations de l'ordre doivent donner un seul nœud à 7 enfants. Le `merge_tree` v9 perd (0,4) et (2,4) dans 1 permutation sur 6 et emboîte deux nœuds de même niveau dans 3 sur 6.

**Condensation dorée** (L08, L11) :
- 8 facettes de masse 1, paires à β = 1, quadruplets à β = 4, racine à β = 16, seuil 3, λ = 1/r. Attendu (HDBSCAN et HGP-old) : stabilité 1,0 par enfant. La v9 donne 7,0.
- Variante à 16 points : HDBSCAN retient 2 clusters, la v9 fautive 4.

**Prédicat témoin** :
- α3 appliqué à tort en q4 : tétraèdre (0,0,0), (60,0,0), (20,42,0), (28,10,49), z = (28,−12,−12) ;
- contacts non stricts : (32,34,28) et (32,32,32).

**Coquille > 12**, deux entrées à fournir :
- (a) les 30 points entiers de norme 25, translatés de (5,5,5) : permutations de (±5,0,0) et de (0,±3,±4). La v9 refuse ;
- (b) contre-fixture adverse de L13 : 108 points exactement cocirculaires sur $x^{2}+y^{2}=1105^{2}$, plus 2 000 points de bruit à z ≥ 3000 (`morsehgp3D_v10/audits/audit_v9_20260928/preuves/verif_L13_F2/adv_c108.u32le`). La v9 refuse (`chain_shell_above_12`, `max_shell` 109) ; le prototype CBLE dépasse 300 s sans refus.

**MEB K7** : contre-fixture nominale de l'auditeur historique, en v7 (`morsehgp3D_v7/audits/NOTE_CLAUDE_COEUR_MEB_20260911.md`, `receipts_coeur_meb_20260911/` ; porte `tests/tower/anchor_meb_gate.cpp`). Les coordonnées sont dans ces fichiers ; à recopier telles quelles.

**Générateur** : tétraèdre sans face q3 acceptée (K3), 75 sites cosphériques, racine sur la borne g6, `prune_annulus`, contre-exemple de propriété D4, carré coplanaire, quadrilatère AC/BD à p=Kmax−1 (`tests/gen/`, `tests/chain/`).

**Bornes de domaine** : 262 143 accepté, 262 144 refusé ; collision (0,1,0)/(0,0,65536).

**Échelle, sans fixture** : triangle de Gabriel isolé de la scène `bridge` (n=2000, medium, graine 2026092800), sommets (1070, 1484, 1944), obtus en 1944. C'est une instance naturelle de Prop. 6 ; on la régénère depuis le générateur du banc.

---

## 3. Algorithmes et prédicats exacts réutilisables

### 3.1 Arithmétique u18 (PROUVÉ et testé)

- Entiers exacts i64/i128/U192/U320, sans jitter. Le flottant n'est admis que comme filtre certifié avec repli exact, compilé avec `-ffp-contract=off -fno-fast-math -frounding-math`, et coupé sous `__FAST_MATH__` ou hors `FE_TONEAREST`.
- Bornes :
  - clé q3 : A < 2^76, |B| < 2^96, |C| < 2^116 ;
  - clé q4 : A < 2^60, |B| < 2^81, |C| < 2^100 ;
  - puissance q3 < 2^117 ;
  - comparaison de racines L5 par pivot < 2^97 ;
  - atlas en i64 à Q = 2^20 ;
  - centre q3 par division longue (2^137 dépasse i128 : ne jamais former scale·x) ;
  - intermédiaires q2 < 2^44, exacts aussi en binary64.
- Les 42 bornes simples sont vérifiées par `audits/check_u18_bounds_20260922.py`.
- **La garde `certified_cell` à 2^117 n'a aucune marge** : un passage à 19 bits lèverait une exception (audit C §2.3). En v10, graver chaque borne par `static_assert` paramétré par le domaine et la tester aux extrêmes (262 143). Plusieurs commentaires v9 sont restés en 16 bits (`q34_witness_search.hpp:86-88`, `.cpp:100`, `q3_ball_census.hpp:79`, `wspd_q34.cpp:721`).
- Niveaux : tri exact unique par filtre double certifié (marge $1-2^{-46}$, `src/tower/lanes/level.hpp:79-88`) et repli U320, puis **rangs u32** (`level_run`, `full_ball_tower.hpp:1934-2000`, déjà utilisé en interne par la voie statique). La v10 doit propager ces rangs dans la **sortie** (la v9 recopie un `ExactLevel` de 48 o par nœud et par contribution).
- Le pgcd binaire est portable CPU/GPU (`src/gpu/lanes.hpp:175-190`).

### 3.2 Prédicats q2

- Boule diamétrale de {a, b}. On pose $H=(z-a)\cdot(b-z)$ : intérieur strict ⇔ H > 0, coquille ⇔ H = 0.
- Puissance : $4H=\lVert b-a\rVert^{2}-\lVert 2z-(a+b)\rVert^{2}$ (`src/gen/pipeline/q2_census.cpp:49-72`).
- Clé `Q2BallKey` = (2c en u32, |ab|² en u64).
- `h_minimum(A,B,{z})` : H est bilinéaire en (a,b) et concave en z, donc son minimum exact sur un produit de boîtes est atteint aux coins (`src/gen/spindle/predicates.hpp:81-98`). Les bornes exactes de 4H sur {a}×B×Z sont dans `q2_prepared_bounds.hpp`.
- Lemme (C, `audits/c_audit_20260923/lectures/L2_rapport.md` §3.4) : z intérieur ⇒ |z−a| < |ab| et |z−b| < |ab|, donc $p\leq\min(\rho_{a}(b),\rho_{b}(a))$, où ρ compte les sites strictement plus proches. Aucune violation sur 2,4 M paires LiDAR (MESURÉ). Il sert de porte « kNN ⇒ q2 » et de voie rapide : 24 à 27 % des supports LiDAR ont un rang < K. Mais 0,15 à 0,6 % sont des ponts longs de rang ≥ 64K, **nécessaires** à la hiérarchie : un générateur fondé sur les seuls k plus proches voisins est incomplet.

### 3.3 Prédicats q3/q4 (PROUVÉ)

- Propriété : **plus longue arête du support** (jamais de la coquille), départagée par la paire d'identifiants triée (`wspd_q34.cpp:434-437`, `q4_local.cpp:27-37`).
- Graine q4 canonique : plus petit identifiant parmi les faces aiguës incidentes à l'arête maximale. Au moins une existe, car $\sum\lambda_{v}H(v)=-2\lVert c-m\rVert^{2}<0$ (`audits/COMPLETUDE_Q4_CLE_REMBOURREE_20260923.md:29-35`, `q4_local.cpp:454`).
- Témoin de lentille strict : H > 0 et $\alpha H^{2}>\Xi$, avec $\Xi=\lVert(z-a)\times(b-z)\rVert^{2}$, α3 = 3, α4 = 2 (`src/gen/spindle/predicates.hpp:130-160`, `q34_witness_search.cpp:130-170`). Équivalent angulaire : l'angle azb dépasse 120° en q3 et 125,26° en q4. Les témoins universels vivent dans un cône de demi-angle 60° (q3) ou 54,7° (q4) autour de ab.
- Cover suffisant : R + |c−m| < |ab|, donc toute boule possédée est dans $\lVert 2z-a-b\rVert^{2}\leq 4D$ (D = |ab|²).
- Plan bissecteur :
  - on écrit $2(c-m)=u_{1}A+u_{2}B$ avec A, B entiers orthogonaux à v = b−a ;
  - on pose $L_{z}(u)=\lVert w\rVert^{2}-\lVert v\rVert^{2}-2w\cdot(u_{1}A+u_{2}B)$ avec w = 2z−a−b ;
  - z est intérieur ⇔ $L_{z}<0$.

  La profondeur d'un centre est le niveau d'un arrangement de droites ; le centre q3 est le pied d'une perpendiculaire, le centre q4 un sommet ℓ_x ∩ ℓ_y.
- Certificat de cellule : T intérieurs uniformes sur une cellule fermée, disque par disque (`q34_dead_lanes.cpp:137-213`). Balayage q4 à seaux de lentille S4b (L1, L2, L5, L6, L9), élagage L11 (44,9 % du cover à K5), ordre axial L10.
- Dédoublonnage par clé primitive ; recensement de la chaîne indépendant du générateur.

### 3.4 MEB exacte

Welzl proposé en double, puis vérifié exactement. Noyau accéléré v7 (paire diamétrale d'abord, extrêmes, canonisation sur la coquille) : 4,12× sur banc, zéro divergence sur 198 000 cas (`morsehgp3D_v7/audits/NOTE_CLAUDE_COEUR_MEB_20260911.md`).

Welzl à base non bornée : fausse piste fermée. MEB incrémentale : levier de la phase 0, puisqu'un seul site est échangé par pas de descente.

### 3.5 Générateur recommandé : énumération par boîtes de centres (« CBLE », L13)

**Lemme de complétude** (PROUVÉ, élémentaire). Soit Q une boîte de centres et (c, r) une boule admissible avec c ∈ Q, dont l'intérieur ouvert contient p ≤ K−1 sites.

- (a) **Gardes.** Soit S0 un ensemble quelconque de K sites distincts. Au moins un y ∈ S0 vérifie |y−c| ≥ r, donc |x−c| ≤ |y−c| pour tout site x du support, de la coquille ou de l'intérieur. On garde x s'il existe y ∈ S0 tel que $\min_{c'\in Q}(\lVert x-c'\rVert^{2}-\lVert y-c'\rVert^{2})\leq 0$. La fonction est affine en c′, donc on l'évalue exactement au coin choisi par les signes.
- (b) **Dominateurs.** Si au moins K sites y vérifient $\max_{c\in Q}(\lVert y-c\rVert^{2}-\lVert x-c\rVert^{2})<0$, alors x n'appartient à aucune boule admissible centrée dans Q : ces K sites y seraient strictement intérieurs. On exclut x. Le pool de dominateurs est formé des 3K sites les plus proches du centre de la boîte.
- (c) Un **second lemme**, par récurrence, est **à écrire** : les sites qui ont au plus K−1 sites strictement plus proches dans la liste parente survivent aux gardes et aux dominateurs. C'est ce qui rend le compte p exact.
- (d) Le centre est dans conv(U), donc dans la boîte englobante des candidats : une boîte disjointe de cette enveloppe est vide.

**Algorithme.**
1. Octree dyadique à coordonnées entières. Chaque boîte filtre la liste de son parent par gardes puis dominateurs.
2. On subdivise tant que |N(Q)| > M (M = 16 à K5, 24 à K10).
3. Énumération locale exhaustive dans chaque feuille :
   - q2 : milieu dans Q ;
   - q3 : triangle aigu strict, circumcentre dans Q ;
   - q4 : droite des centres du triplet ∩ Q, puis quatrième point, centre strictement intérieur par 4 orientations.
4. Recensement sur N(Q) en entier.

Les feuilles **demi-ouvertes** partitionnent le domaine des centres : chaque boule est émise dans l'unique feuille qui contient son centre. Seules les coquilles étendues sont dédoublonnées, localement, par clé rationnelle réduite. Les listes de feuilles sont aussi un **oracle K-NN exact** en tout point (même lemme) : elles peuvent servir aux descentes de la tour.

**Validation MESURÉE (L13 et son vérificateur).**
- Identité par (q, p) et par coquille étendue avec un oracle brut sur 60 petits nuages dégénérés (grille 5³ comprise), et 45/45 avec un oracle indépendant en Fraction.
- **Comptes identiques à la v9**, par q_min, par coquille étendue et `max_shell`, sur :
  - uniforme, amas et terrain à 8k, 16k, 32k (K5) ;
  - LiDAR 08/000200 sans sol à K5 (1 407 885 boules) et K10 (5 483 320 = [977 534, 3 019 193, 1 486 593], 1 301 coquilles étendues) ;
  - deux plans à 8k et 16k ;
  - trame brute b02 avec sol (125 526 sites) : 3 071 514 boules, identiques à R22 probe_32.

**Limites à lever en v10.**
- Aucune borne de pire cas. La contre-fixture cocirculaire (108 points) fait exploser le temps (plus de 512 feuilles de côté 1 avec m ≈ 109). Il faut un **refus explicite** quand m > M au côté 1, ou un traitement du plateau.
- Préfiltres flottants non certifiés : droite du triplet contre la boîte, avec une marge de 1e-3.
- Carré de r² en i128 dans la clé de dédoublonnage : débordement possible.
- Doublons non gérés : un doublon produit une q2 de rayon nul et 67 coquilles étendues parasites.
- Seuls les **comptes** sont identiques à la v9 : ni enregistrements, ni identifiants, ni condensés.
- Coût local en O(m³) par feuille. Le port GPU (un warp par feuille, m ≤ 24) reste à faire.

**Piste v3 « cellules de centres ».** Elle a été fermée pour des raisons d'ingénierie, pas par un contre-exemple (`morsehgp3D_v3/audits/PISTES_FERMEES.md`). La rouvrir exige, selon la règle du dépôt, le lemme inscrit, des fixtures et une porte de coût sur les familles adverses.

### 3.6 Construction de la tour, plus simple que la v9 (L04 §8)

- **Atlas de blocs** : $\mathrm{cell}(B,K)=\mathrm{base}[B]+K-\mathrm{lo}[B]$ (idée v7, `morsehgp3D_v7/docs/OBJETS_PARALLELES_TOUR_20260911.md` §2). Tous les tableaux sont indexés par bloc actif, jamais K × catalogue.
- **Rangs u32** partout et une seule table des niveaux exacts réduits.
- **Phase 0** (cibles statiques) : hachage exact des classes de requêtes, MEB proposée puis vérifiée, descente, semis ; en parallèle sur les classes **et** sur les K. La v9 sérialise la phase 0 d'un ordre à l'autre, et c'est ce qui fait le chemin critique à K10. Leviers à mesurer :
  - saut au centre D5 de l'auditeur C (5-10 ms estimés à K5, 100-125 ms à K10, non mesurés) ;
  - MEB incrémentale.

  La recherche d'intrus depuis un nœud local ne donne qu'un gain constant (≈ 57 visites par requête) : la super-linéarité en K vient du **nombre** de requêtes (×10,7 de K5 à K10), pas du redémarrage à la racine.
- **Phase A** : union-find maigre en structure de tableaux, sans allocation par lot. Écriture **directe** dans le CSR final (niveau, début des parents, successeur, bloc de naissance). L'encodeur-valideur devient un juge de porte.
- **Parallélisme intra-ordre**, seulement si le contrat LiDAR l'exige : arêtes (bloc → cible) pondérées par rang, MSF de Borůvka (preuves K1 au registre l.90-94), dendrogramme parallèle, puis contraction des nœuds internes de même rang (≥ 2 anciens : multifusion ; 1 : continuation ; 0 : naissance). L'équivalence avec le regroupement v9 est attendue mais **non prouvée**.
- **Verticales** : naissance = ancre (B, K−1), sans union-find ; fusion = requête d'ancêtre hors ligne ; naturalité en passe parallèle ou échantillonnée.
- **Populations implicites** : identifiant de boule vers I/U du catalogue en CSR de `PointId`.
- **Un seul ordonnanceur**. La v9 crée 409 fils par construction à K5 et 889 à K10 (sursouscription).
- **Un seul algorithme**, identique à 1 et à W fils. La voie temporelle v7 reste un juge de test.
- Proxy d'audit (union-find maigre séquentiel, synthétique) : 48-89 ms pour la taille K5 de 08/000000, contre 148 ms pour A(K5) en v9. Gain ×2-3, pas ×10 ; pour 100 ms il faudrait du parallélisme intra-ordre.

### 3.7 À ne pas porter (fermé par la mesure)

- Front WSPD indexé par paires comme énumérateur q3/q4. Sur huit amas, ≈ 0,44·n² paires sont développées (tout le produit inter-amas). Cause : l'index isole chaque amas, le front émet les 28 produits inter-amas en rectangles entiers, et chacun contient une paire survivante, donc aucun test universel ne peut le tuer. Sur coquilles creuses, le certificat du cœur diamétral est cubique.
- Cascade rectangle → paire + cache → cœur diamétral → cover → atlas/voies ; cover reconstruit deux fois ; cinq parcours d'index par arête sur le chemin CPU.
- Double implémentation q3/q4 (moteur atlas v8 et voies jumelles hôte/GPU) tenue égale compteur par compteur ; noyaux GPU contraints à rejouer l'ordre séquentiel.
- Trois index spatiaux (kd du générateur, Karras de la tour, copie plate GPU) et quatre types de clé.
- Recensement triple (générateur, chaîne, tour) et sceau R-29 échantillonné au 1/64, avec un résidu déclaré de lecture hors bornes (`full_ball_tower.hpp:190-206`).
- Environ 4,2 kLOC compilées hors chemin produit (T24-T27, `local_credits`, `tube_credits`, `axis_q2`, `q2_census` joint ou coopératif, dispatcher Donate, `core/fixed_signed.hpp`), et une douzaine de modes jamais utilisés par la chaîne.
- Pistes fermées de `docs/archive/abandoned/README.md` et des FAUSSES_PISTES v3, v7, v8, v9 :
  - fenêtre Morton ou préfixe kNN comme autorité ;
  - mosaïque de Delaunay d'ordre supérieur ;
  - pavage rhomboïdal global (38,7 M cellules en 904 s à n = 20 000, K = 4) ;
  - histogrammes locaux ; Window30 et couches duales en remplacement de Local28 ;
  - jobs grossiers ; GPU par petits lots synchrones ;
  - dix constructeurs copiés ; crédit par nœuds (+27 à +32 % de CPU) ; 16 plus proches voisins globaux ; index des selles seul.

### 3.8 Juges (à garder et à étendre)

- **T2** : `tests/tower/census_tower_oracle.hpp` avec `oracle/tower/local_plateau_oracle.hpp` (Gram en `boost::rational<cpp_int>`). Il juge Γ_K exhaustivement pour n ≤ 14 ; `check_cut` (`full_ball_tower_gate.cpp:232-289`) apparie par identité chronologique, coupes ouvertes et fermées, verticales sur toutes les sous-facettes.
  - Défauts v9 : T2 de chaîne à **Kmax=10 seulement**, sur 3 nuages. À n = 12, presque rien n'y est élagué (0/66 voies mortes prouvées).
  - v10 : T2 **aléatoire multi-K**, au moins 200 nuages de n ≤ 12 dans des boîtes serrées, K = 1..min(10,n), entrée permutée, IDs épars, W = 1 et W max, avec planchers sur les voies mortes et rejetées aux petits K.
- **Mini-T2 Python/Fraction** (`morsehgp3D_v10/audits/audit_v9_20260928/preuves/L06_tests_oracles/mini_t2.py`) : 70 nuages, K = 1..5, 13 696 coupes, 0 désaccord avec la chaîne v9 (MESURÉ). Il sert de second oracle en autre langage ; il ne porte que sur les comptes de composantes.
- **Juge d'échelle K = 1 contre l'EMST exact** : ordre 1 = EMST, niveaux d²/4, identique sur 8k serré, 8k large et 08/000000 (39 885 sites). En C++ avec un k-d tree, il coûte quelques ms. Il ne voit que l'ordre 1 : c'est un garde-fou de plomberie, pas de complétude.
- **Juges d'échantillon indépendants** :
  - `tests/chain/q2_sample_judge.cpp`, avec plancher `--min-top` à p = Kmax−1 ;
  - `tests/chain/q3_sample_judge.cpp`, qui vise p = Kmax−2, la zone aveugle ;
  - `chain_absent_keys_gate.cpp` (semi-indépendant : il réutilise `anchor_meb`, `ball_census` et `ShellTable`, et ignore les coquilles de plus de 12 sites) ;
  - juge brut à supports indépendants de C (`audits/c_raw_support_judge_20260925` : q2, q3 et q4, 675 à 1 024 clés par cas, soit 0,008 à 0,026 %, 14 mutants tués).

  À porter à 8k/16k/32k en nuit, et à rejouer à chaque changement de chaîne. Le dernier vert à l'échelle date du 24 septembre.
- **Oracles de composant** : `tests/gen/exact_ball_oracle.hpp` (Gram rationnel), `morsehgp3D_v8/audits/oracle_q3q4_20260915/oracle_q3q4.cpp` (i128, inventaire q3 pour n ≤ 250 et q4 pour n ≤ 60), tour saturée S.1-S.6. `reference/morsehgp3d_oracle` refuse tout catalogue non générique (`oracle.py:320-328`), d'où un usage limité sur la grille de 1 mm.
- **Mutants** : registre compilé (`src/tower/core/mutants.hpp`), un seul registre, **déterministe**. Le mutant `task_pool_mutant_join_closed` dépend d'une course : 6 plantages sur 20 en local, 12 échecs de CI sur environ 59. À proscrire. Mécanisme de copie mutée à conserver : `tests/gen/mutants.json`, cause stderr exacte ; ne pas garder la substitution de texte ambiguë.
- **Codes de sortie exacts** : `cmake/run_expect.cmake` (0 conforme, 1 désaccord du juge, 2 refus, 3 invariant violé, 4 mutant tué ; tout signal est un échec).

---

## 4. Chiffres de référence

Sauf mention contraire : grille 1 mm, séparation s = 8, G4 = g4-standard-48 (EPYC 9B45, 24 cœurs × SMT2, 185 Go, RTX PRO 6000 Blackwell 97,9 Gio, sm_120), W48.

### 4.1 Taille du catalogue (MESURÉ)

**Uniforme 3D, par site et par niveau p** : q2 ≈ 3,7 ; q3 ≈ 4,4·(p+1) ; q4 ≈ 1,6·(p+1)(p+2)/2 (loi de Blaschke–Petkantschin).

- Total par site ≈ 3,7K + 2,2K(K−1) + 1,6·C(K,3), soit ≈ 0,27 K³ en tête.
- Mesuré : 421,6 par site à K10 (32k), 386 à 8k (effets de bord), 74 à 79 à K5.

**LiDAR sans sol** (08/000000, 000100, 000200 ; 39 885, 35 551, 45 845 sites) : 30,7 à 32,8 boules par site à K5, 119,6 à 138,2 à K10. q4 y représente 27 % contre 45 % en uniforme. Brut avec sol (123-126 k sites) : 22-25 par site à K5, 86-93 à K10.

**Ordre de grandeur** : n·Θ(K³) ; pire cas O(n²K²) pour q4 ; borne de sortie Ω(n²) FULL. Un seul niveau k = 10 représente 37 à 43 % du catalogue K10.

### 4.2 Registre G4 R22 (`receipts/g4_tower_r22_20260926`, 560 fichiers, SHA OK)

| Cas | Chaîne GPU (s) | Chaîne moteur CPU (s) | Mur processus GPU (s) | CPU·s GPU / moteur |
| --- | ---: | ---: | ---: | ---: |
| 00/K5 | 0,926 (0,940) | 2,770 | 1,718 | 17,3 / 114,9 |
| 01/K5 | 0,760 | 2,180 | 1,519 | — |
| 02/K5 | 0,983 (une passe ; 0,965 dans `g4_q3_payload`) | 3,000 | 1,874 | — |
| 00/K10 | 2,961 | 8,405 | 6,029 | 76,0 / 344,9 |
| 01/K10 | 2,269 | ≈ 6,1 | 4,783 | — |
| 02/K10 | 2,888 | ≈ 8,6 | 5,989 | — |
| brut b00/K5 | 1,991 | 5,614 | 3,478 | — |
| brut b00/K10 | 6,010 | 14,877 | 11,904 | — |

- Bruts K5 : 1,81 à 2,03 s ; K10 : 5,31 à 6,01 s.
- **L'écart entre mur et chaîne vient surtout des digests de vérification**, pas du produit :
  - K5 : 0,48 s de digests sur 0,76-0,89 s d'écart ;
  - K10 : 2,35 s de digests ;
  - contexte CUDA 121-166 ms ; bassin épinglé 38-40 ms (K5) et 190-201 ms (K10).
- Mode résident : mesuré seulement **sur la même trame rejouée** (`g4_core_warm_20260927`, 943 ms au premier passage, médiane chaude 923 ms). Jamais sur des trames distinctes.

**Phases à 00/K5** (probe_0, ms). q34 = 482, décomposé ainsi :

| Étape | ms |
| --- | ---: |
| front CPU | 98 |
| S2 filtre témoin | 96 (noyau 64,7) |
| S3 certificats | 119 (noyau 90,8) |
| S4 voies | 81 (noyau 55,3 ; attente des ouvriers 79) |
| colle hôte | environ 84-88 |

Hors q34 :
- fusion 28 ;
- recensement 102 ;
- tour 289, dont validation 34, phase 0 84 (38 pour l'ordre K5), phase A de l'ordre K5 148, images 28, encodage 39 ;
- q2 105 et son recensement précoce 70, recouverts par l'appareil (`q2_wait` = 0) ;
- index du générateur 12,2.

Nsight (`docs/PROFIL_FULL_NSYS_20260927.md`, `receipts/full_nsys_20260927/r2`) :
- médiane chaude 912 ms ;
- noyaux 207,9 ms par passage (certificats 90,1, soit 43 %) ;
- union des activités GPU 213 ms : **GPU inactif 77 %** ;
- aucun recouvrement entre activités GPU ;
- tour CPU 277-303 ms.

**Phases à 00/K10** (probe_2) :
- q34 1 106 (front 168, filtre 157, certificats 272, voies 302) ;
- recensement 500 ;
- tour 1 212, dont validation 117, phase 0 738 (résolution 564 ; ordre K10 : 208), phase A de l'ordre K10 585, images 125, encodage 93.

Le vrai chemin critique à K10 passe par la phase 0 sérialisée des ordres K10 à K8, puis la phase A de l'ordre K8 : environ 1 201 ms.

**Générateur à 00/K5** :
- q2 : 9,16 M paires candidates, 457 k acceptées ;
- q34 : 3,13 M rectangles, 23,7 M paires développées, 2,04 M survivantes ;
- 708 686 arêtes demandées aux voies, dont 312 064 émettrices ;
- 849 780 présentations (691 284 q3 et 158 496 q4) ;
- recensement 100,7 M visites (535 M à K10).

**Tour** :

| Grandeur | K5 | K10 |
| --- | ---: | ---: |
| Boules | 1 306 696 | 5 512 670 |
| Nœuds | 1 541 750 | 7 426 215 (1 638 573 à l'ordre K10) |
| MEB | 1,29 M | 11,3 M |
| Visites d'intrus | 31,7 M | 403 M |
| Naissances à K10 | — | 4 414 230 |

**Mémoire (RSS de pointe)** :
- 00/K5 : 1,56 Go (bras GPU, dont 256 Mo épinglés) et 1,21 Go (moteur) ;
- 00/K10 : 6,36 Go (GPU, dont 1,28 Go épinglés) et 4,47 Go (moteur) ;
- brut b02/K10 : 10,5 Go et 8,8 Go.

Soit environ 810 o par boule côté moteur. Tailles d'enregistrement (vérifiées par `static_assert`) :

| Type | Taille |
| --- | ---: |
| `BallData` | 224 o |
| `BallKey` | 80 o |
| `ExactLevel` | 48 o |
| `FullNode` | 64 o |
| `FullDatedContribution` | **80 o** |
| requête de phase 0 | 56 o |
| `Presentation` | 112 o |

Sortie FULL (cinq tableaux) : 207,5 Mo à K5 et 1,007 Go à K10, dont 56-57 % de copies de niveau.

**Contribution du GPU.** Le ×3 sur la chaîne mélange l'appareil et un autre algorithme (S4a/S4b sans atlas, qui n'existent que sur GPU et dans un jumeau hôte séquentiel). Aucune implémentation CPU native et optimisée du même algorithme n'a été mesurée. À CPU·s égal, le jumeau suggère que l'essentiel du gain vient de l'appareil.

### 4.3 Uniformes, terrain et amas à 8k, 16k, 32k (local, W4, hôte partagé ; `receipts/q3_payload_local_20260926`)

| Famille (K5) | Chaîne 8k / 16k / 32k (s) | Catalogue 8k / 16k / 32k | Commentaire |
| --- | --- | --- | --- |
| uniforme | 4,31 / 9,58 / 20,79 | 594 386 / 1 234 454 / 2 531 823 | nœuds 629 404 / 1 301 794 / 2 660 312 ; phase 0 = 1 322 ms sur 2 132 ms de tour à 32k |
| terrain | 0,91 / 2,13 / 3,67 | — | linéaire |
| huit amas aux coins (`eight_corner_clusters_splitmix64_v1`, u16) | 21,9 / 83,4 / 306,7 | 510 758 / 1 099 180 / 2 305 835 | paires développées 28,35 / 112,77 / 449,65 M (×3,98), dont 99,4 % rejetées une à une ; filtre q34 221 s à 32k ; plan par facteurs E ×3,94 |

Cette recette d'amas n'est **pas** le générateur du banc HDBSCAN : le banc utilise des gaussiennes.

### 4.4 Familles adverses (mesures d'audit, W2, compteurs déterministes)

- **shells** du banc (sphères creuses), K5 :
  - q34 : 57,5 / 179,9 / 1 438,5 s ;
  - `cover_sites` : 228 M / 1,60 G / 12,4 G ;
  - `core_sites` : pente 3,00 ;
  - émissions q3+q4 linéaires : 96,6 k / 188,8 k / 375 k.

  À K2 : export 15,2 s à 8k et 366 s à 32k. Mécanisme : noyau diamétral moyen de 1 212 sites sur 18,2 M arêtes survivantes (cordes presque antipodales), 97 G tests uniformes.
- **Deux plans séparés par un vide** (K5) : v9 54,7 s → 239,8 s de 8k à 16k (×4,38) ; catalogue 271 403 → 487 951.
- **Familles gaussiennes du banc** à K2 (spherical, filaments, hierarchical, bridge, heteroscedastic) : quasi linéaires, chaîne de 1,4 à 11 s de 8k à 32k. `hierarchical` à K5 : paires développées ×3,15 puis ×3,20.
- **LiDAR, coupes emboîtées radiales** : les exposants y sont des artefacts de densité (disque qui grandit). La chaîne reste entre p = 0,73 et 1,49. N'en tirer aucune pente.

### 4.5 CBLE contre v9 (2 fils, comptes identiques)

| Famille | CBLE 8k / 16k / 32k (s) | v9 | µs CPU par boule (CBLE) |
| --- | --- | --- | ---: |
| uniforme K5 | 2,10 / 4,37 / 8,97 | 4,31 / 9,58 / 20,79 (W4) | 7,1 |
| amas K5 | 1,90 / 4,11 / 8,68 | 21,9 / 83,4 / 306,7 (W4) | 7,4-7,5 |
| terrain K5 | 0,24 / 0,53 / 1,18 | 0,91 / 2,13 / 3,67 | 3,0-3,3 |
| plans K5 | 1,10 / 2,03 / 3,69 | 54,7 / 239,8 / — (W2) | ≈ 8 |
| LiDAR 02 K5 | ≈ 23-26 CPU·s | 125,7 CPU·s moteur ; 17,9 CPU·s bras GPU | ≈ 18 |
| LiDAR 02 K10 | 97 CPU·s (48,5 s mur) | 366,8 CPU·s moteur ; 70,3 CPU·s bras GPU | ≈ 18 |
| uniforme K10 8k / 32k | 10,2 / 40,9 | — | 6,1-6,6 |

Les chaînes v9 incluent la tour (≈ 10 %) ; CBLE ne fait que compter.

### 4.6 Clustering (banc du 28 septembre ; ARI avec le bruit prédit compté comme une classe)

250 exécutions à n ≤ 8000 : 51 scènes moins celles à 32k, 5 graines. Sources : `morsehgp3D_v10/audits/audit_v9_20260928/preuves/L14_clustering_research/L14_*.csv` et `receipts/synthetic_bench_20260928/r1/baselines.csv`.

| Méthode | ARI | Couverture |
| --- | ---: | ---: |
| HDBSCAN « défaut » du banc (mcs = ms = 20) | 0,527 | 0,82 |
| « oracle » du banc (mcs seul, ms lié) | 0,683 | 0,85 |
| HDBSCAN défaut + remplissage au plus proche voisin | 0,683 | 1,00 |
| tour d'avant correctif (cda636b5e, K2, z = 1, Gabriel, √n) | 0,707 | 0,88 |
| tour d'après correctif (HEAD ce8a649dd) | 0,644 | 0,93 |
| même tour avec la racine exclue (diff non commis) | ≈ 0,73 (+0,051 sur l'oracle, 195-55) | — |
| HDBSCAN ms = 2, mcs = √n (sans vérité) | 0,738 | 0,90 |
| HDBSCAN ms = 3, mcs = √n | 0,700 | 0,88 |
| oracle 2D (ms × mcs) | 0,782 | 0,90 |
| oracle 2D + remplissage | 0,852 | 1,00 |
| EOM λ = r^(−3), ms = 2, √n + remplissage | 0,837 | — |
| EOM λ = r^(−ẑ), ms = 2, √n + remplissage borné ρ = 2 | 0,839 | 0,99 |

Coût de la chaîne Python du 28 septembre : 16,5 s par scène à 8k (médiane), contre 0,51 s pour HDBSCAN. `merge_tree` recopie des ensembles de membres : Σ|members| = 78 M à 8k et 1,46 G à 32k. Pic de 4 Go à 8k ; à 32k, arrêt pendant `merge_tree`, et environ 65 Go estimés.

---

## 5. Objectifs et contrats hérités

### 5.1 Hiérarchie K-NN (contrat v9)

- **Objet** : tour FULL v7, K1..K5 puis K1..K10, moteur entier 18 bits (grille 1 mm).
- **Entrées de contrat** : trames SemanticKITTI **sans sol** de 30 à 60 k sites (08/000000, 000100, 000200). Trames brutes avec sol (≈ 123-126 k) en extension.
- **Chronos visés sur G4** : 1 s, puis 100 ms. Atteint : K5 < 1 s en chaîne interne sur les trois trames sans sol ; non atteint en mur processus ni sur trames brutes. 100 ms est jugé **infaisable** avec les algorithmes connus (R-30, auditeur C). Horizon de refonte : 0,25-0,5 s à K5 et 0,7-1,4 s à K10.
- **Séparation WSPD s ≥ 8** : directive utilisateur du 14 septembre (« ne jamais prendre s < 8, cela ne signifie rien »). Mesuré : s = 8 plus rapide que 10 et 12 (R24-B, une passe). La séparation ne certifie rien : si la v10 abandonne le WSPD, la directive devient sans objet. **À confirmer avec l'utilisateur** ; ne jamais utiliser s < 8 tant qu'un WSPD existe.
- **Tailles d'intérêt** : 8k, 16k, 32k (64k en extension). Toute pente, tout coût et toute mémoire s'y mesurent, sur des **familles homogènes**, jamais sur des coupes radiales emboîtées ni sur des petits nuages (réservés aux oracles de correction).
- **Directive d'échelle** (2 septembre) : priorité de 10^4 à 10^7 points, multi-CPU puis GPU, garde-fous minimaux. Directive du 22 septembre : un livrable qui fonctionne ; trancher les détails soi-même.
- **Feu vert G4** : pour la v10, oui, mais uniquement par les scripts gardés (§8.2).

### 5.2 Budgets dérivés de la taille de sortie

- 100 ms à K10 sur 46k sites LiDAR exigent environ **55 M boules/s** de bout en bout, soit environ 18 ns par boule. À K5, environ 14 M boules/s (70 ns par boule).
- 10^6 points LiDAR à K10 donnent environ 1,2·10^8 boules (≈ 7 Go à 60 o par boule), à traiter en flux par niveau ou par région. À 18 µs CPU par boule (CBLE), cela fait environ 2 200 CPU·s : le GPU est obligatoire pour tenir la seconde.
- Toute cible de temps doit s'exprimer **relativement à la sortie** (boules/s, nœuds/s), jamais en extrapolant des gains par levier.

### 5.3 Cibles réalistes proposées (à faire valider)

- CPU, 8 fils, tour et clustering compris : ≤ 1 s à 8k et ≤ 5-10 s à 32k sur **toutes** les familles du banc.
- GPU résident sur trames distinctes : 250-400 ms à K5 et 0,7-1,4 s à K10 sans sol. 100 ms reste un sujet de recherche, conditionné à une réduction de travail d'au moins ×2,3.
- Budget mémoire : ≤ 60 o par boule (catalogue) et ≤ 16 o par nœud (sortie), publiés à 8k, 16k, 32k.

### 5.4 Objectif clustering

Tirer de la tour FULL la hiérarchie de clusters la plus pertinente et **battre HDBSCAN** sur les bancs synthétiques, avec des reçus rejouables. Battre HDBSCAN signifie battre :
- (a) HDBSCAN apparié aux mêmes réglages ;
- (b) le meilleur HDBSCAN sans étiquettes ;
- et approcher l'oracle 2D, déclaré supervisé.

Directives utilisateur :
- z (poids ψ(t) = 1/t^z) = dimension intrinsèque ; z = 1 pour l'équité face à HDBSCAN, z = 2 pour les surfaces LiDAR ;
- la thèse est une **source critiquable**, pas une autorité ; toute divergence est explicite et argumentée ;
- HGP-old est le code de la thèse, donc l'oracle de correction sur petits nuages (clique du manuscrit contre chemin du code ; `min_samples` = K+1 sur le chemin d'ordre K).

---

## 6. Clustering : thèse, HGP-old, v9, têtes candidates

### 6.1 Formules du §9.1 de la thèse (PDF 122-123, imprimées 96-97)

- ψ(t) = t^(−p), où p est la **dimension ambiante** dans le texte (p = 3 en 3D). Le tableau SIPU utilise ψ = 1/t ; birch2 est réparé a posteriori par 1/r² (p. 103).
- $S_{\tau}=\sum_{\sigma\supset\tau,\ \lvert\sigma\rvert=K+1}\psi(\rho(\sigma))$.
- $T_{x}=\sum_{\tau\ni x}S_{\tau}$ ; $w_{x\tau}=S_{\tau}/T_{x}$ ; $m_{\tau}=\sum_{x\in\tau}w_{x\tau}$.
- Vote : $V_{x}(c)=\sum_{\tau\ni x,\ \ell(\tau)=c}S_{\tau}$ ; le point prend $\arg\max_{c}$.
- Chaque point couvert distribue une masse 1 : $\sum_{\tau}w_{x\tau}=1$ et $\sum_{\tau}m_{\tau}$ = nombre de points couverts.
- $m_{\tau}\leq 1$ est PROUVÉ sous F = ∂C (car $T_{x}=K\sum_{\sigma\ni x}\psi_{\sigma}$). En convention « facettes Gabriel seules », on n'a que $m_{\tau}\leq K$ (1,21 observé).
- Le seuil `min_cluster_size` est une **masse** (défaut √n). La masse totale reste le nombre de points couverts dans les deux conventions.
- Aucun théorème ne relie m_τ ou l'EOM pondérée à une masse de probabilité. La Prop. 7 garantit seulement une partition.

### 6.2 Algorithme 1 (PDF 126, imprimée 100)

1. Delaunay d'ordre K.
2. Triangles (K-simplexes) de Gabriel.
3. Graphe dual : sommets = facettes d'au moins un simplexe de Gabriel (F = ∂C).
4. Chaque simplexe relie « (deux de) ses trois arêtes » ; texte ambigu entre clique et chemin.
5. Scores S_τ **avant** l'arbre couvrant.
6. MST.
7. Condensation « comme HDBSCAN ».
8. EOM, puis vote.

Ce graphe Gabriel brut est exactement la construction que E5 réfute comme hiérarchie exacte.

### 6.3 HGP-old (code de la thèse ; licence non commerciale, lecture seule comme spécification)

Sémantique implémentable, nom proposé `hgp91_old`, reconstituée par L11.

**Défauts** : K = 2, expZ = 2,0 (`core.py:51`), `min_cluster_size` = round(√n_core) (`core.py:141-144`), `min_samples` forcé à K+1 (`core.py:146-149`), méthode `eom`.

**Étapes** :
- **E0** — Catalogue : Delaunay puis triangulations régulières itérées sur les barycentres des k-parties (poids de puissance $\lvert c\rvert^{2}$ − moyenne des $\lvert p\rvert^{2}$, `_geometry_binding.cpp:221-405`). On obtient les (K+1)-parties à **cellule de Voronoï d'ordre K+1 non vide**, un ensemble strictement plus grand que Gabriel en position générale. Rayon = MEB² (CGAL), poids w = ρ^z.
- **E1** — Graphe dual : facettes F = ∂C ; chaque σ produit un **chemin** de K arêtes de poids w(σ) (`_cython.pyx:880-894`).
- **E2** — Scores avant le MST : $S_{\tau}=\sum 1/w(\sigma)$, plafonné à 1e12 ; $T_{x}$ ; $m_{\tau}$.
- **E3** — Kruskal, tri `argsort` instable.
- **E4** — Condensation ascendante par lots de poids float32 égaux, λ = 1/(w + 1e-12). Chaque facette est un atome de masse m_τ dès le départ. Pour une racine de masse ≥ m :
  - aucun cluster dedans : nouvelle feuille née à λ ;
  - un seul cluster : il est prolongé ;
  - au moins deux clusters : ils meurent et un parent naît.

  Les multifusions ne sont pas binarisées (`_cython.pyx:399-659`).
- **E5** — Sélection EOM (les enfants gagnent si leur somme dépasse strictement la stabilité propre ; **racine sélectionnable**), feuilles, ou coupe r_cut en unités ρ^z.
- **E6/E7** — Étiquettes de facettes, puis vote argmax (égalité au plus petit indice). **Bruit ⇔ aucune facette étiquetée**.

**Pièges** :
- float32 partout ;
- `weight_face` ignoré ;
- bruit N(0, 1e-5) avec le backend geogram ;
- **`min_samples` > K+1 corrompt silencieusement** la voie d'ordre K : largeur min_samples, lue avec un pas K+1 (`_cython.pyx:776-777` est un simple `pass`). La simulation donne 29 faces distinctes au lieu de 34. L'exemple du README d'HGP-old (`min_samples=5, K=2`) le déclenche ;
- sur les voies gudhi, `min_samples` devient une distance-cœur.

HGP-old n'est pas installé (`import hgp_clusterer` échoue). La porte « reproduit HGP-old » passe donc par une **réimplémentation MIT indépendante** de cette sémantique, en Fraction.

**HGP-Clusterer3D** (`/workspaces/E-HGP/HGP-Clusterer3D`, licence jamais libre) : même catalogue. Son λ normalisé par la médiane des rayons vaut une constante h^z fois celui d'HGP-old, donc la sélection EOM est identique en exact. Perturbations 1e-8 et 1e-12. L'affirmation contraire de `cluster.py:22-24` est FAUSSE.

### 6.4 Écarts entre prose et code, à déclarer (L11)

| Écart | Prose | Code |
| --- | --- | --- |
| D1 catalogue | Gabriel | ordre-Voronoï K+1 |
| D2 connexion | clique (Déf. 29) | chemin (mêmes composantes, arêtes MST différentes) |
| D3 exposant | ψ = 1/t^p (p ambiante) | 2,0, 1,0 ; SIPU en 1/t |
| D4 échelle λ | ψ(ρ) | 1/(ρ^z + ε), h^z en C3D ; HDBSCAN 1/d |
| D5 naissance d'une facette | ρ(τ) (Déf. 21) | atome inerte jusqu'à sa première coface |
| D6 exactitude | Prop. 6 / Th. 5 | faux en général (E5) ; l'exactitude du catalogue Voronoï est inconnue |
| D7 `min_samples` | — | corruption au-delà de K+1 ; distance-cœur sur voie gudhi |
| D8 racine | « comme HDBSCAN » | sélectionnable (HDBSCAN l'exclut par défaut) |
| D9 bruit | Prop. 7, partition | « aucune facette étiquetée » : beaucoup moins de bruit, couverture favorisée |
| D10 unités de coupe | — | ρ^z (old) contre rayon (C3D) |

Sonde n = 12, oracle flottant (L11 et vérificateur) : les deux catalogues préservent l'ensemble de points des composantes non triviales. Les désaccords de partition des facettes sont **tous des retards d'attache** (type A, 0 à 6 facettes par instance, toutes non-Gabriel avec ≥ 2 intrus), jamais des scissions. La datation des facettes, donc des masses, dépend de la sémantique d'attache : « première coface » (HGP-old) ou « naissance Čech ρ(τ) dans FULL » (objet exact). À déclarer.

Critique statistique de la thèse (tableau 9.3, p. 101-103) :
- une seule exécution ;
- `min_samples` d'HDBSCAN non déclaré ;
- traitement du bruit dans l'ARI non déclaré (le notebook d'HGP-old calcule l'ARI hors bruit prédit) ;
- jeux exclus ;
- birch2 réparé a posteriori (HDBSCAN 0,996 contre HGP 0,441) ;
- HGP classe davantage de points dans 11 jeux sur 15.

« HDBSCAN ne fait jamais mieux » n'est pas démontré.

### 6.5 Ce que la v9 a fait (chronologie) et ses défauts

| Date / dossier | Tête | Résultat | Statut |
| --- | --- | --- | --- |
| 27 sept., `audits/b_point_hierarchy_k_20260927` | `first_coverage` / `entry_vote` sur T_K, EOM commun | +0,011 moyen, sens inversé sur le lot d'évaluation (0,8647 contre 0,9165) | MESURÉ, reçu privé |
| 27 sept., `experiments/weighted_clustering_20260927` | masses §9.1 + **attaches FULL corrigées** + vote | mixte ; premier port Gabriel seul = 26 composantes contre 1 (E5), corrigé | MESURÉ, reçu r2 |
| 27 sept., `point_dendrogram_20260927` | routage exclusif emboîté | 7 victoires, 3 nuls, 3 défaites sur 13 scènes | MESURÉ |
| 27 sept., `synthetic_clustering_20260927` | routage K5 | 0,5574 contre 0,4198 ; 96 % du gain dû à `first_coverage` ; marge par graine +0,021 puis +0,255 ; médiane +0,0074 ; reçus sous `/tmp` perdus | suspendu (seulement dans `experiments/README.md:17-20`) |
| 28 sept., `experiments/tower_clustering_20260928` | cofaces Gabriel → Kruskal Python → condensation → EOM → vote | « beats HDBSCAN's oracle » | FAUX sur l'objet ; chiffres sans reçu |

**Défauts du pipeline du 28 septembre** (tous vérifiés) :

1. **Topologie.** `cluster.py:67-122` (`facet_levels`, `merge_tree`) unit les facettes des seules cofaces Gabriel exportées ; `measure.read_export` ne lit que `cofaces` et `catalogue`. La forêt FULL (`native.nodes`, `native.roots`) est dans le **même JSON** mais n'est jamais lue.
   - Racines, graine du banc 2026092800, n = 2000, medium : hierarchical 18 / 88 / 416 (convention gabriel) et 10 / 18 / 89 (boundary) à K = 2 / 3 / 5 ; spherical 6 à K2. FULL a 1 racine partout.
   - À K2, les racines parasites pèsent < 1 et changent l'ARI de moins de 0,0003. À K ≥ 3, la fragmentation est structurelle (5 grosses composantes jamais fusionnées).
   - La convention `boundary` (celle de la thèse) est **aussi** fausse : 9 niveaux sur 352 sur `fixture_tiny64_k2`, 2 racines à K3. C'est la Prop. 6.
   - La conclusion « monter K fragmente » (ce8a649dd) est un artefact : **non étayée**, ni vraie ni fausse.
2. **Condensation.** `cluster.py:210` (`elif child in big or not big: stack.append(child)`). Quand aucun enfant n'atteint le seuil, la descente continue et les facettes tombent à leur **propre** λ de naissance au lieu du λ de la scission. Cela contredit HDBSCAN (sklearn `_tree.pyx:204-216`), HGP-old (`_cython.pyx:538-557`), `weighted_eom.py:248-250` et la docstring de `condense` (l.134-138).
   - Seule l'EOM est touchée ; la sélection `leaf` est identique.
   - Différentiel K = 1 contre sklearn HDBSCAN(`min_samples`=1) : HEAD 31/40, cda636b5e 8/40, correctif d'une ligne (`elif child in big:`) 40/40.
   - Le sens de l'effet n'est pas uniforme : shells 0,41 → 1,00, mais spherical hard 0,831 → 0,176.
3. **Racines non seuillées.** `cluster.py:176` met toutes les racines en file sans test de masse ; une racine sans enfant est retenue par EOM comme par `leaf`.
4. **Plateaux.** `merge_tree` indexe `merged` par une clé périmée (`cluster.py:99-120`) : perte silencieuse de facettes ou emboîtement de nœuds de même niveau. Latent sur le banc (0 perte, 1 emboîtement). Le bon code est `weighted_model.py:138-158`.
5. **Convention par défaut `gabriel`** (`run_tower.py:101`, 116 dans le diff) : ne pas la reprendre.
6. **Facettes Gabriel manquées** : `measure.read_export` exige |I|+|U| = K (`measure.py:85`), ce qui omet les K-parties faiblement Gabriel des coquilles étendues. Registre l.66 : Q porté est Gabriel ⇔ I ⊆ Q.
7. **Échelle.** Membres recopiés par nœud, JSON rationnel (166 Mo à 32k), Fraction/Decimal-60, `getcontext().prec` global (`measure.py:60`).
8. **Revendication.**
   - cda636b5e : +0,179, 34-0-0 contre l'oracle, sur 17 scènes × 2 graines, n ∈ {500, 2000} (source : `tour_smoke.csv`, hors dépôt).
   - Après correctif : +0,070 (29-5-0), avec 5 effondrements à 1 cluster (racine sélectionnable).
   - Correctif + racine exclue + condensation conforme : +0,108 (32-0-2) sur ces 34 couples.
   - Sur toute la famille sphérique, avant correctif : +0,187 (106-4) contre l'oracle, mais seulement +0,046 contre HDBSCAN(ms = 2, √n) et −0,049 contre ms = 2 avec remplissage.
   - Tour + racine exclue : perd sur hierarchical (−0,49, 0-20) et shells (−0,40, 0-20).
9. **Oracle du banc** (`baselines.py:21, 43-65`) : `min_samples=None` = `mcs` sous sklearn, EOM seul. Ce n'est pas une borne supérieure. Il choisit mcs = 5, borne basse de sa grille, dans 54 % des scènes. L'oracle 2D choisit `min_samples` = 1 dans 148 cas sur 160.
10. **Code non commis** :
    - `allow_single_cluster=False` est appliqué racine par racine d'une forêt, ce qui n'est pas la règle HDBSCAN ;
    - la docstring « leaf domine d'environ six centièmes » a été écrite **avant** toute mesure de `leaf` (inférence tirée de l'ancien code), puis contredite (tour_v5 : eom 0,721 contre leaf 0,694) ;
    - `log_leaf` ≡ `lambda_leaf`.

**Ce qui est juste et à garder** :
- le correctif « parent scindé » (ce8a649dd) ;
- les masses calculées sur toutes les cofaces **avant** l'arbre couvrant ;
- l'action N-aire par plateau (intention) ;
- les invariants du §9.1 contrôlés (Fraction pour z pair, Decimal avec marges pour z impair) ;
- le vote avec départage déterministe ;
- la publication conjointe des variantes ;
- `compare.py` (appariement par (scène, graine), inversions de graine signalées).

La bonne chaîne du 27 septembre :
- `weighted_model.py` pour les masses et le vote (**pas** comme topologie : son graphe est le graphe Gabriel brut) ;
- `full_weighted_tree.py` et `native_attachment_export.cpp` pour la topologie FULL avec attaches ;
- `weighted_eom.py` pour la condensation (racine unique exigée, sortie au λ de scission, conservation dyadique exacte).

Le couplage de `native_attachment_export.cpp` avec `audits/b_full_a_manifest_20260927/native_a.hpp` (copie instrumentée générée, 2 484 lignes, épinglée par SHA) est à supprimer : il faut publier les ancres dans la sortie (§1.8).

### 6.6 Lien exact avec HDBSCAN

- **Sémantique C∩X** : x entre dans la hiérarchie au niveau $a=d_{K}(x)^{2}$, où $d_{K}$ est la distance au K-ième site, **x compris**. Sa composante est celle de L_K(a) qui le contient. C'est exactement la distance-cœur d'HDBSCAN avec `min_samples` = K au sens sklearn (le point compte). La hiérarchie de points est emboîtée par construction.
- **Première couverture** : $d_{K}(x)/2\leq\alpha_{K}(x)\leq d_{K}(x)$.
- **Entrelacement** : facteur 2 en rayon (4 en a) entre π0(L_K) ∩ X et le graphe d'accessibilité mutuelle d'HDBSCAN (bifiltration « cœur », Blaser–Brun–Gardaa–Salbu, arXiv 2405.01214). À K fixé, HDBSCAN(`min_samples`=K) approxime donc déjà la tranche en O(n log n). Un gain de clustering doit venir de la **tête** (sélection, échelle, bruit, choix de K, usage des verticales), ou il faut le **démontrer** par un témoin de même tête sur l'arbre d'accessibilité mutuelle.
- **Convention K ↔ `min_samples`** : C∩X donne ms = K ; HGP-old (K-ième voisin hors le point) donne ms = K+1. Les deux diffèrent de 0,04 d'ARI sur le banc (0,738 contre 0,700). **Publier les deux**, et déclarer la convention primaire.

### 6.7 Têtes candidates (toutes consomment une interface π0 à deux paramètres : arbres T_k et verticales)

Deux producteurs : la **tour exacte** et la **bifiltration k-NN** (témoin obligatoire).

**A — EOM à l'échelle de densité sur une tranche** (première candidate).
- K ∈ {2, 3}, sémantique C∩X, condensation en comptes de points, mcs = round(√n), λ = r^(−ẑ).
- ẑ estimé sans vérité : maximum de vraisemblance de Levina–Bickel (k = 10), ou rapports verticaux α_{k2}(x)/α_{k1}(x) de la tour.
- EOM avec racine exclue ; politique de bruit déclarée (abstention, remplissage borné ρ = 2, ou complet) ; hiérarchie exportée.
- Mesuré sur le témoin k-NN : 0,831-0,839, stable pour ms de 2 à 8 **avec** remplissage.
- Mais :
  - ẑ ≈ 2,8-3,1 sur 7 familles sur 8 (2,1 sur shells ; 2,9-3,0 sur les filaments épais à l'échelle k-NN), donc ẑ ≈ z = 3 fixe (+0,0004) ;
  - le gain z = 3 contre z = 1 est de +0,039 à +0,045 seulement, concentré sur bridge, filaments, spherical et anisotropic, et négatif sur hierarchical ;
  - le remplissage seul apporte +0,156 ;
  - contre l'oracle 2D avec remplissage : −0,015 (IC contenant 0) ;
  - réglages choisis sur ces mêmes 250 exécutions : **aucune graine tenue à l'écart** ;
  - faible sur hierarchical (0,52-0,58) et heteroscedastic (0,66-0,71) ;
  - le remplissage détruit sous 30 % de bruit (0,624 → 0,441).
- Coût : O(n log n + n·Kmax) après la tour.

**B — Choix de K par consensus vertical** (K qui maximise l'ARI avec K±1) : 0,79-0,81, sans gain sur un K petit fixé. Utile comme indice de confiance.

**C — Tranche γ de Rolle–Scoccola** (JMLR 25(258), 2024 ; Persistable, JOSS 2023). Ligne de (Kmax, 0) à (1, a0) dans le plan (k, a), choisie par le vignoble de proéminence. Théorie la plus solide (stabilité, consistance au sens de Hartigan), non mesurée ; exige les verticales. L'aplatissement façon ToMATo par plus grand écart a échoué dans le proxy (0,51-0,59, sur-découpage).

**D — Masses du §9.1 + vote**, fidèle à la thèse. Uniquement avec les attaches FULL, jamais le graphe Gabriel. Catalogue nommé (∂C des cofaces de Gabriel, ou ordre-Voronoï). Datation déclarée. Mixte le 27 septembre.

**E — Échelle log ou sortie à plusieurs niveaux.** Bonne sur hierarchical (0,74-0,90), destructrice ailleurs (bridge 0,43, spherical 0,18). Seulement comme second niveau publié.

**Remarque sur `hierarchical`.** Un seuil de **masse** entre 83 (sous-amas) et 250 (groupe) sépare correctement hierarchical et spherical : HDBSCAN(mcs = 75, ms = 5) donne 0,80 / 0,79. C'est une convention de masse, pas de contraste. Aucune règle purement invariante d'échelle ne résout les deux.

### 6.8 Spécification proposée de la tête v10 (point de départ, à préenregistrer)

1. **Entrée** : tour FULL (ou témoin k-NN), K choisi, `anchor[K][bloc]`.
2. **Hiérarchie de points C∩X** : chaque x entre à $d_{K}(x)^{2}$ dans la composante FULL qui le contient. Arbre condensé en CSR (format `mhgp9_condensed_point_tree_v1`, `audits/b_gaussian_point_clustering_20260927/CONDENSATION.md`). Tailles en post-ordre, intervalles DFS ; **aucun ensemble de membres**.
3. **Condensation HDBSCAN exacte par lots de niveaux égaux**, multifusions N-aires :
   - un enfant sous le seuil fait tomber ses points au λ de la scission ;
   - si aucun enfant n'atteint le seuil, le cluster courant se termine ;
   - un parent porte ses points jusqu'à la scission ;
   - racine unique, née à λ = 0, **exclue** par défaut (`allow_single_cluster` déclaré).
4. **λ = r^(−z)** avec z déclaré (1 pour l'équité, ẑ en variante nommée), le même z pour les masses et pour λ. Pas de mode hybride.
5. **Sélection** : EOM et `leaf` publiés ensemble. La primaire est désignée d'avance.
6. **Bruit** : politique déclarée. Publier la couverture.
7. **Portes** : §7.6.

---

## 7. Protocole d'évaluation recommandé

### 7.1 Défauts du banc v9 à corriger (`experiments/synthetic_bench_20260928`, reçu r1 `22cb1895b`)

Le générateur est déterministe avec digest SHA-256, et le reçu des références est rejouable (255/255 digests). Il est bon à garder. Les défauts :

- **Oracle trop étroit** : `mcs` seul, `min_samples` lié. Ce n'est pas une borne supérieure.
- **Niveaux définis par l'échec d'un réglage arbitraire** (mcs = ms = 20, alors que le défaut sklearn est mcs = 5 et donne 0,589 global et 1,000 sur shells). Résultats bimodaux :
  - medium et hard : réussite ou effondrement de la sélection EOM ;
  - extreme : effondrement systématique ;
  - spherical medium publié 0,73 (graines de calibration 11-15), mais 0,40 sur les graines du plan.

  La porte de calibration tourne sur les graines de réglage, avec une tolérance de 0,18 sur 3 graines alors que la doc annonce 0,15 sur 5.
- **ARI avec le bruit compté comme une classe** sur des vérités presque sans bruit : il confond couverture et qualité. Le passage à l'ARI « rejets en singletons » **change le signe** des agrégats log_eom et leaf de la tour.
- **Familles hors spherical** seulement à n = 2000 ; pas de 16k ; bruit seulement au point central.
- **`hierarchical`** : vérité au seul niveau grossier, sans sous-labels (`bench_datasets.py:227-251`).
- **Survivants silencieux** : `run_tower.py` écarte les scènes refusées du CSV ; `compare.py:96` saute les paires manquantes.
- **Agrégats mal filtrés** dans `SUMMARY.json` : `centre_by_groups['8']` = 0,527 mélange trois niveaux de bruit. Colonne « oracle par scène » du README non commis incohérente (oracle < défaut).
- **Unité statistique** : la graine, pas la paire (tirages communs entre niveaux). Aucun intervalle de confiance, aucun test, aucune correction multiple.

### 7.2 Données du banc v10

- Les huit familles, **toutes** à n = 500, 2 000, 8 000, 16 000, 32 000 ; bruit à 0, 10 et 30 % pour chaque famille.
- Familles à ajouter : lunes et spirales 3D, Chainlink, Atom (densité imbriquée), uniforme sans structure (test d'`allow_single_cluster`), mélange surface + filament + blob (cas LiDAR), points aberrants structurés.
- `hierarchical` avec **sous-labels** et métrique à deux niveaux.
- Suites publiques épinglées par hash, données brutes hors Git :
  - FCPS (Hepta, Tetra, Atom, Chainlink) ;
  - SIPU (Flame, Spiral, Aggregation, Compound, Pathbased, Jain, R15, D31, S1-S4 ; 15 jeux du tableau 9.3) ;
  - Chameleon t4.8k et t7.10k ;
  - 2D plongée en z = 0. Vérifier d'abord que le moteur accepte la coplanarité totale à 8k-32k ; validé seulement à n ≤ 312.
- Doublons après quantification : multiplicités, jamais une scène écartée.

### 7.3 Adversaires

**Sans étiquettes** :
- HDBSCAN aux défauts de la bibliothèque (mcs = 5) ;
- HDBSCAN **apparié** (mcs = √n, ms = K **et** ms = K+1, EOM et leaf) ;
- HDBSCAN réglé par DBCV ;
- HDBSCAN + remplissage au plus proche voisin ;
- OPTICS-ξ ; GMM-BIC.

**Supervisés, déclarés comme tels** :
- oracle 2D (mcs × ms ∈ {1, 2, 3, 5, 8, 12, 20, 32} × eom/leaf × `allow_single_cluster` × epsilon), avec publication du taux de choix au bord de la grille ;
- liaison simple et Ward au vrai k ; GMM au vrai k ;
- plafond à modèle connu par cellule. GMM initialisée aux vraies moyennes : ≈ 0,9 à « extreme » sur 5 familles sur 8.

Nommer chaque référence par ses paramètres (`hdbscan_mcs20_ms20`, `hdbscan_lib_default`, `hdbscan_matched_K`).

### 7.4 Métriques

- ARI avec le bruit en bloc (héritage) ; ARI avec rejets en singletons ; ARI et AMI sur les points couverts ; AMI ; couverture ; précision et rappel du bruit ; F1 macro hongrois ; écart au nombre de groupes.
- Métriques d'arbre, indépendantes de l'extraction : pureté du dendrogramme, F1 du meilleur nœud par classe. Pour `hierarchical` : un score contre chacun des deux niveaux.
- Décision fondée sur l'ARI singletons ou l'AMI hors bruit, **avec** couverture minimale déclarée.

### 7.5 Statistique et préenregistrement

- Fichier versionné qui fixe la configuration primaire (K, z, seuil de masse, sélection, sémantique de forêt et de bruit), avec le hash du code.
- Réglage sur des **graines et familles de développement** ; gel ; puis exécution **unique** sur des graines de test disjointes. Toute variante calculée est publiée ; la primaire reste celle désignée d'avance.
- Appariement (scène, graine) ; au moins 10 graines jusqu'à 8k et au moins 5 au-delà.
- Par famille : Wilcoxon signé et test des signes sur les moyennes par graine ; bootstrap hiérarchique (graines puis scènes) avec IC à 95 % ; correction de Holm entre familles.
- Publier la moyenne par couple **et** la moyenne équilibrée par famille. La famille sphérique pèse 110 couples sur 250 et renverse les agrégats.
- Une scène refusée ou en échec compte comme ARI = 0, et elle est publiée.
- Victoire = borne basse de l'IC > 0 au global, et aucune perte significative non publiée.

### 7.6 Portes du clustering (dès le premier jour)

- **G1** — K = 1, masses unitaires : étiquettes identiques à sklearn HDBSCAN(`min_samples`=1, `allow_single_cluster`=False, même mcs), à permutation près, sur au moins 20 nuages, en EOM et en leaf. Cette porte aurait tué les deux défauts de la v9.
- **G2** — Connexité : nombre de racines = composantes FULL (une par K à a = ∞), et ensembles de points égaux au π0 FULL sur des coupes d'échantillon. Fixtures permanentes : E5, témoin à 4 points (convention gabriel), témoin à 5 points, `fixture_tiny64_k2`. Mutant « Gabriel seul » tué. Une porte « une racine » est **nécessaire mais pas suffisante** : sur E5, le graphe Gabriel finit à une racine avec une date fausse.
- **G3** — Condensation dorée : 8 facettes (stabilité attendue 1,0), bascule EOM à 16 points (2 clusters), plateau à trois cofaces sur toutes ses permutations, égalité → parent, racine dans les deux politiques.
- **G4** — Invariants du §9.1 exacts (z pair) : Σm = nombre de points couverts, partition de l'unité, m ≤ 1 sous ∂C.
- **G5** — Vote doré : départage au plus petit indice ; règle de bruit déclarée.
- **G6** — Catalogue : Voronoï d'ordre K+1 par programmation linéaire exacte, et Gabriel par MEB exacte, pour n ≤ 12-14.
- **G7** — Mutants tués avec cause exacte : filtre `gabriel`, descente « tous petits », ms ≠ K+1, λ indépendant de z, parent vidé à la scission, racine non seuillée.
- **G8** — Équivariance par permutation et renumérotation des `PointId` ; sortie bit-identique quel que soit le nombre de fils.
- **G9** — Échelle 8k/16k/32k : invariants globaux et juge d'échantillon ; aucun juge en O(n³).
- **Ablation E1 obligatoire** : même tête sur la tour et sur l'arbre d'accessibilité mutuelle, même K, même z. Si l'IC de ΔARI contient 0, la tour n'est **pas** justifiée par la qualité du clustering. Règle de décision proposée : adopter la tête sur la tour seulement si E1 ≥ −0,01 et si elle bat HDBSCAN défaut + remplissage **et** l'oracle 2D brut sur les graines tenues à l'écart.

### 7.7 Portes de la tour et du catalogue

- T2 aléatoire multi-K ; juge K = 1 contre EMST ; juges d'échantillon q2/q3/q4 et Euler en nuit à 8k/16k/32k ; porte « kNN ⇒ q2 » (lemme p ≤ min(ρ_a, ρ_b)) et « EMST ⊂ présentations p = 0 » en O(nK log n) ; différentiel de digests v9/v10 (exporteur de compatibilité) sur uniforme, terrain, amas et trames LiDAR.
- Juge d'échantillon des **verticales** (absent en v9).
- Porte de **coût** sur les familles adverses (shells, huit amas aux coins, deux plans, cocirculaire) à 8k/16k/32k : toute pente > 1,3 du travail pour une sortie linéaire bloque la porte.
- Une douzaine de mutants causaux sur l'objet : census non strict, coupe ouverte/fermée, plateau scindé, verticale décalée, fenêtre K au lieu de K+1, classe q2/q3/q4 omise.
- **Chronomètre honnête** :
  - frontière écrite ;
  - mur processus, mur résident sur **trames distinctes** (p50/p95, répétitions entrelacées) et chaîne interne publiés ensemble ;
  - digests de vérification hors mesure produit ;
  - chaque exécution chronométrée a un digest égal à la référence jugée ;
  - compteurs de travail indépendants de W ;
  - mutant « phase sautée » tué par le digest.
- Gains muraux appariés et entrelacés sur la chaîne entière seulement. Les sommes par K restent en diagnostic.

### 7.8 Reçus

Un reçu autonome, versionné dans le dépôt, rejouable depuis un clone neuf, contient :
- les entrées régénérables par graine, avec empreintes **par entrée** ;
- le commit, le hash du binaire, la commande ;
- les CSV de la méthode **et** des références ;
- les compteurs déterministes en tête, les temps en second ;
- la preuve `TERMINATED` pour G4.

Aucune dépendance à `/tmp` ni à `build/`. Un message de commit n'est jamais une preuve.

---

## 8. Pièges pratiques

### 8.1 Erreurs récurrentes de méthode (v3 → v9)

- **Réduction « cofaces Gabriel seules = hiérarchie »** : cinq occurrences (v3, v4/v5, audit de reprise v8, port du 27 septembre, port du 28 septembre). Seul remède : fixtures objet (E5, A–E, 4 points induits, coquille à 7 points, carré K2, triangle rectangle, K = 1, K = n) **imposées par CMake** à tout module qui produit ou lit une hiérarchie, avec un mutant « Gabriel seul » tué.
- **Chrono de composant présenté comme contrat** : v7 (189 ms de noyaux pour 419 s de tour), v9 (`chain_total` contre mur).
- **Chiffres sans reçu** : archives de conversation (v8), captures sous `/tmp` perdues (27 septembre), message de commit (28 septembre).
- **Portes vacantes** : `WILL_FAIL` et regex en v3 ; règle de digest v29 qui ne pouvait rien refuser (74fbcc362) ; label `scale32000` posé sur un diagnostic de 0,01 s. Toute porte naît avec un mutant qui la fait refuser et un plancher de non-vacuité.
- **Pentes tirées de petites tailles ou de coupes radiales emboîtées.**
- **Certificats qui coûtent plus qu'ils ne rapportent** : crédit par nœuds (+27 à +32 %), `near_sites`, index des selles, micro-leviers q34.
- **Revendication antérieure à un correctif**, jamais rétractée : cda636b5e. Publier un erratum.
- **Plafonds pris pour des théorèmes** ; régularité supposée (un nuage u16 n'est pas régulier : quatre coquilles supplémentaires à K10 sur 50k).
- **Élargissement de domaine** : le test de plateau i128 v7 est faux à 18 bits (`PASSATION v9:506-508`) ; huit énoncés faux dans `ELARGISSEMENT_18_BITS_20260922.md` (v8), alors que le code est juste.

### 8.2 Environnement

- **Boost** est absent du conteneur. Recette : `apt-get download libboost1.83-dev=1.83.0-2.1ubuntu3.2` (sha256 519ecf2c…), `dpkg-deb -x`, puis `-DBOOST_ROOT`. `FATAL_ERROR` si Boost manque, sinon 25 juges disparaissent en silence (v8). Boost doit rester facultatif pour le produit et réservé aux tests et oracles.
- **VM G4** :
  - g4-standard-48 SPOT, us-central1-b, cible `devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35` ;
  - pilote 580.173.02 (CUDA 13.0), nvcc 12.9.41, **g++ 11.4** (`-Werror=maybe-uninitialized` diffère), **CMake 3.22.1** (pas de `CUDA_STANDARD 20` : échec S1) ;
  - Nsight 2025.3.1 incompatible avec le pilote 13.0 ;
  - préflight à faire avec **cette** chaîne d'outils (build/v9-cuda322 existe localement ; oublié le 27 septembre : `.rsp` exigé à tort).
- **Sessions G4** :
  - uniquement `gcp-migration/start_and_verify.sh` et `stop_and_verify.sh` (double coupe-circuit, label `project=e-hgp`, `maxRunDuration` de 30 s à 8 h, seule vraie garde) ;
  - certifier `TERMINATED` sur la cible exacte ;
  - un seul exécuteur de session générique piloté par un plan JSON, pas un jeu de scripts par expérience (v9 : environ 38 tentatives en six jours, dont 7 sans mesure : préemption, rupture de stock, clé OS Login expirée, clé en 0644, binaire exclu de la capture par `tower_session_v9.py:526`) ;
  - **capturer le binaire et la commande** ;
  - pas de nouveau schéma de sonde à chaque levier (v1 → v30 en quatre jours) ; schéma additif ;
  - ne jamais éditer un script bash en cours d'exécution ;
  - un redémarrage du conteneur perd `/tmp` et le trap : faire `describe`, puis `stop_and_verify` avec les variables explicites ;
  - pas de SSH concurrents en collecte.
- **GPU** (quand il viendra) :
  - une session device par processus, buffers résidents qui ne font que croître, flux avec copies asynchrones épinglées ;
  - enchaînement sur le device sans aller-retour hôte ;
  - appels découpés en **lots bornés avec repli par lot**. La v9 fait un appel unique limité à 2^31−1 paires (entiers CUB), dont le refus fait échouer toute la chaîne ; seuil vers n ≈ 70k sur la recette d'amas. Le report par arête des voies existe déjà et est à garder ;
  - capacités indépendantes de la carte ;
  - aucun noyau ne porte un travail quadratique ;
  - GPU seulement pour un étage data-parallèle qui domine un profil CPU propre (plus de 30 % environ), mesuré contre une implémentation **CPU native du même algorithme**, jamais contre un jumeau hôte d'émulation de warps.
- **`/tmp` et disque** : `/tmp` est effacé au redémarrage ; `/workspaces` (63 Go) était plein à 90 % avec 139 arbres `build/v9-*` (12 Go). Faire `df` avant toute campagne.
- **Git** :
  - l'index est partagé : exiger `git diff --cached --quiet` avant `git add`, jamais `git add -A` ;
  - pas de chaîne add + commit + push sans garde ;
  - pas de `git worktree prune` ni de `gc --prune=now` (trois commits d'agents orphelins : 38f420085, 6d38ed95a, e181dec27) ;
  - jamais de `ctest`, même `-N`, dans des builds épinglés ;
  - `pgrep -f` se reconnaît lui-même : attendre une sentinelle créée seulement en cas de succès ;
  - heredoc shell toujours quoté ; `check_docs` sans pipe.
- **CI** :
  - une CI verte bloque tout commit (la v9 est restée rouge 40 poussées, cause 9df1b4cb2 : lecteur de reçu lié au validateur vivant du worker G4) ;
  - aucun lecteur de reçu historique dans la CI rapide ;
  - mutants déterministes seulement ;
  - tests Python du consommateur dans CTest (ceux de `tower_clustering` n'y étaient pas) ;
  - l'étape des selftests G4 est sautée quand ctest échoue : la mettre dans un job séparé.
- **Budget CI** : la v9 fait 17,5 min de build et 24 min de tests sur `--parallel 2`, dont 66 % d'auto-différentiel et 1,8 à 3,2 % de juges de l'objet. Viser une CI rapide de moins de 5 min, dominée par les juges ; différentiels lourds en nuit ; une seule porte « matrice de leviers ».

### 8.3 Données tierces

- **Aucun octet KITTI dans Git** (`AGENTS.md:46`). La v9 l'a pourtant fait : `audits/s4a_cpu_scene02_physical_panel_20260924/inputs/` (84 fichiers, 21 nuages dérivés de 08/000200) et `audits/s4a_ground_hot_quarter_20260923/inputs/`. La v8 a versionné 114 Mo de dérivés et les scans bruts (`q34_spatial_20260921/gcp_package_r1/snapshot.tar.gz`). Le dépôt est **public, MIT** ; KITTI est non commercial à partage à l'identique.
- Prévoir `data/` ignoré par Git, un fetcher à sha256 attendus, et un contrôle CI par manifeste qui couvre aussi les **membres d'archives**. La réécriture d'historique est une décision de l'utilisateur.
- HGP-old et HGP-Clusterer3D : lecture comme spécification seulement, jamais importés ; sorties gelées comme fixtures seulement si la licence le permet (question ouverte). Poids Pointcept (Sonata, etc.) : CC-BY-NC.

### 8.4 Organisation

- README stable d'une page (objet, API, commandes) ; un ÉTAT court, réécrit et non ajouté (environ 150 lignes) ; journaux dans `receipts/`.
- La v9 : README-journal de 277 lignes, PASSATION de 1 066 lignes, `ETAT_COURANT` de 2 743 lignes, 349 entrées d'audit, 628 commits en 6 jours, texte collé sans espaces non détecté par `check_docs`.
- Un seul chemin par défaut, égal à la configuration mesurée. Les variantes vont dans `experimental/`, hors bibliothèque, jusqu'à une ablation appariée ; ensuite elles deviennent le défaut et l'ancien chemin est supprimé.
  - La v9 : 38 options de chaîne dont 31 booléens, 17 refus de dépendance écrits à la main ; configuration la plus rapide à 11-12 leviers du défaut de la bibliothèque ; deux leviers mesurés négatifs ou neutres conservés.
- Unités de moins de 500 lignes ; tour compilée en `.cpp` (et non 35 recompilations de `tower_chain.cpp` pour les mutants) ; pas de mutants ni de chronos dans les en-têtes produit ; un petit grand-livre de compteurs non vérifiés, réduits en fin de travail.
- Aucun `#include` depuis `audits/` dans le produit ou les expériences.
- Registre des preuves mis à jour **avant** d'invoquer un énoncé ; l'audit ne certifie rien.
- Fonctionnement à plusieurs acteurs (développeur, auditeurs) : chaque audit décisionnel est versionné dans `morsehgp3D_v10/audits/`. L'audit géant du 28 septembre (39 agents) n'existe que dans un scratchpad de session.

---

## 9. Questions ouvertes à trancher (utilisateur ou concepteur)

1. **Objet du clustering.** Tranche π0 FULL d'ordre K avec C∩X (exacte, entrelacée avec HDBSCAN), masses du §9.1 sur attaches FULL (fidèle à la thèse), ou exploitation multi-K des verticales (tranche γ, contribution nouvelle à déclarer comme telle) ?
2. **Univers de poids du §9.1**, si on les garde : ∂C des cofaces de Gabriel (Alg. 1, m_τ ≤ 1), ordre-Voronoï (HGP-old/C3D) ou contributions datées de FULL ? Et quelle datation : première coface, ou naissance Čech ρ(τ) ?
3. **Convention K ↔ `min_samples`** pour l'adversaire apparié : sklearn ms = K (C∩X) ou K+1 (HGP-old) ? Proposition : publier les deux, déclarer la primaire.
4. **Politique de bruit** : abstention, remplissage borné ou complet ? Elle déplace l'ARI de ±0,15 à 0,2.
5. **z** : fixé par famille, estimé globalement (ẑ ≈ 3 en pratique), ou par branche (scènes LiDAR mêlant surfaces et volumes) ?
6. **Directive s ≥ 8** : devient-elle sans objet si la v10 n'a plus de WSPD ?
7. **Complétude à l'échelle** : un catalogue à Kmax+2 (K ≤ 12, intérieur ≤ 11, coût à mesurer ; nécessaire, pas suffisant), ou des juges d'échantillon stratifiés sur la zone aveugle (déjà présents en v9, à porter) ?
8. **Coquilles de plus de 12 sites** : quotient par arrangement de grands cercles (Λ_t), ou refus compté et publié ? Pour le générateur CBLE, un refus explicite en cas d'explosion de feuille est obligatoire.
9. **Multiplicités** : naissances à a = 0 aux ordres ≤ m pour un site de multiplicité m ; effet sur la comparaison à HDBSCAN (qui compte le point lui-même).
10. **Parité bit à bit avec `tower_digest` v9**, ou digest canonique v10 plus exporteur de compatibilité ?
11. **Contrat de latence** : processus froid par trame ou processus résident multi-trames ? Digests de vérification dans ou hors du chrono ?
12. **Historique Git** : réécrire pour retirer les dérivés KITTI v8/v9, ou exception documentée ? Récupérer ou non les trois commits orphelins ?
13. **Parallélisme intra-ordre** de la tour (MSF, dendrogramme, contraction des plateaux) : nécessaire hors du contrat LiDAR 100 ms ? L'équivalence avec le regroupement v9 est à prouver avant usage.
14. **Voie rapide k-NN** : kNN exacts à K (24 à 27 % des supports q2 LiDAR) en combinaison avec le générateur par centres, ou inutile une fois CBLE adopté ?
