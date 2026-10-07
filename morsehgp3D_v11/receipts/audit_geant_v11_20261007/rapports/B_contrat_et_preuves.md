# Rapport B — Contrat mathématique de la v11 et ses preuves (audit critique)

7 octobre 2026. Lecteur B, en lecture seule, sur `main` à `e968aba8d` (moteur gelé `ac081a06f`).

Cadre : `phase=exploration_v11_hors_registre` (close), `backend=cpu_reference`, `profile=quantized_u21_input_only`,
`public_status=not_claimed`. **GCP non utilisé.**

- Aucun fichier du dépôt n'a été créé ou modifié : `git status` est identique à l'instantané initial (34 lignes) et
  aucun `.pyc` n'a été produit.
- Python : 3.12.1, l'interpréteur local. Le codespace n'a pas de Python 3.10. Toujours lancé en `-S -B`, avec
  `TMPDIR` dans mon dossier de travail.

Légende :
- **[V]** vérifié par lecture du code ou du document, ou par un recalcul.
- **[R]** rejoué par une exécution (résultat cité dans l'annexe).
- **[I]** inférence.

Références : chemins relatifs à `morsehgp3D_v11/`, sauf mention « racine ».
- `MATH` désigne `docs/MATHEMATIQUES.md`.
- `REG` désigne le registre racine `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`.

## 0. Verdict en dix points

1. **Le contrat est mathématiquement sain [V].** J'ai relu les 940 lignes de `MATH` et refait chaque preuve :
   - M1–M2, G1–G4, T1–T6, P1–P5, J1–J3 ;
   - les lemmes A, P, W, B, C, D, E, F, G, H et la suffisance de Kruskal ;
   - les lemmes de calcul : table de populations, R, V3, diamètre, cliques, zonogone, M3/E4, niveaux différés, réemploi
     vertical, MST/contraction.

   Je n'ai trouvé aucune erreur de fond. T2 (une trace est stricte si et seulement si elle est séparable) remplace bien
   la position générale partout.

2. **Les preuves n'ont pas toutes le même degré de rédaction [V].**
   - Complètes : § 1–3, T1, T2, T3, T5, tout le § 10.
   - Esquisses : T4 au § 5, complété seulement par le lemme P (§ 10.3) ; T6 au § 6 ; J3 au § 8.
   - J3 n'est prouvé complètement que dans `morsehgp3D_v9/audits/CONTRELEC_EULER_PAR_NERF_20260923.md` (`559c8ab84`).
   - Le document dit lui-même résumer ses preuves (`MATH:388`).
   - Sources hors dépôt : la source catalogue L01 (`build/v11-persist/audit_v10/L01_MATH_CATALOGUE.md`, avec
     `preuves_l01_math_catalogue/`) et le brouillon assemblé. Seuls L02 et L03 ont un instantané versionné dans les
     reçus.

3. **Le registre couvre mal la v11 [V].**
   - Il n'a que quatre sections V11 (`REG:1293`, `1322`, `1339`, `1352`).
   - Aucune ligne propre à la v11 pour M1–M2, G1–G4, T1–T6, P1–P5, J2, ni pour les lemmes de calcul (T1 n'est couvert
     que par le théorème 2 du manuscrit, `REG:30`).
   - **La suffisance de Kruskal (`MATH` § 10.10 bis) n'y figure pas**, alors que la passation, l'audit final (§ 4.2) et
     le rapport A l'affirment au statut `proved_here`.
   - Défaut de forme : la ligne du lemme E est absorbée dans la ligne `false_in_general` de D2 par un `\n` littéral
     (`REG:1370`, introduit par `5adf6a59f`). `tools/check_docs.py` ne le détecte pas.

4. **Constat nouveau sur `MHGP11SP` v2 [R sur un modèle des règles écrites].** La sélection de Kruskal et `S*` suivent
   l'ordre des `SiteIdx`, c'est-à-dire l'ordre de Morton. La sortie publiée n'est donc **pas équivariante par
   translation entière**, contrairement à ce qu'affirment `MATH` § 10.10 et `docs/SORTIES.md` § 10.

   Sur le témoin du cercle, `S*` saute d'un diamètre à l'autre : la sortie v2 est **plus** instable que la v1 qu'elle a
   remplacée.

5. **L'oracle borné tient ses comptes [R].**
   - Tous ses compteurs sont rejoués à l'identique : 1 362 ordres, 48 234 coupes, 13 029 nœuds ; 22 mutants tués et
     3 équivalents ; supports S1 : 210 nuages, 951 ordres, 13 mutants.
   - La suite complète (5 617 nuages) **a tourné sur G4**. La passation ne le dit pas.
   - Le différentiel contre les vidages v10 n'a été joué qu'en local le 2 octobre : les matrices G4 n'avaient pas
     `MHGP11_V10_FROZEN_DIR`.

6. **Les budgets numériques sont exacts [V].** « Seul q3 est dur (216M⁶) » est juste, mais seulement pour la puissance
   et le côté.

   La passation décrit mal un piège :
   - la « garde i64 cubique dépassée dès u21 » était un trou de couverture des portes, pas un défaut du produit (le
     code élargit correctement) ;
   - « LevelSource » et « racine 2^127−1 » désignent un seul et même défaut.

7. **La galerie des témoins existe, mais elle est dispersée [V].**
   - Il n'y a **pas de `tests/fixtures/` dans la v11** : les témoins sont écrits en ligne dans les tests Python et C++,
     et dans les reçus.
   - Le témoin « 9 sites pour 80 feuilles » n'existe que comme modèle dans un reçu : il n'est pas gravé dans une porte
     native.
   - Plusieurs contradictions n'ont pas leur ligne au registre, contrairement à la règle de `CLAUDE.md`.

8. **Le juge J2 (ordre 1 contre l'arbre couvrant euclidien minimal) est énoncé mais jamais implanté [V].** C'était le
   juge d'échelle de la forêt le moins coûteux.

9. **Les identifiants entrent en collision.** P, T, J2, G4, V3, R, E1 et D2 désignent chacun plusieurs objets différents
   dans les documents, le code et le canal d'audit [V]. C'est un vrai risque de lecture pour la v12.

10. **Pour la v12 :**
    - garder les énoncés et l'oracle ;
    - réécrire l'organisation : un seul contrat numéroté sans doublons, des identifiants à préfixe, une ligne de registre
      par énoncé, un catalogue de témoins lisible par une machine ;
    - rendre canonique l'ordre de départage des sorties.

## 1. Carte du modèle, de l'objet au calcul

### 1.1 Entrée, niveaux, conventions (`MATH` § 1–2, `docs/ARCHITECTURE.md` § 3)

**Entrée.**
- n sites **distincts**, de poids 1, sur la grille entière [0, 2^B)³.
- B = 21 par défaut ; 18 et 24 sont compilables (`src/num/budgets.hpp:11`).
- Une entrée pondérée est refusée (`unsupported_degeneracy`).

**Niveaux.**
- Un niveau est un **rayon carré** rationnel `num/den`, avec `den > 0`, jamais réduit.
- Les niveaux se comparent par produits croisés exacts (`src/num/level.hpp:32`) et sont indexés par des rangs denses
  `LevelRank`.

**Boule critique.**
- b = (c, λ) est la plus petite boule d'une partie d'au moins deux sites.
- I_b = {‖x−c‖² < λ}, U_b = {‖x−c‖² = λ}, P_b = I_b ∪ U_b ; p = |I_b|, m = |U_b|.
- Un **support** est une partie affinement indépendante de U_b dont l'intérieur relatif contient c.
- q = q_min est le plus petit cardinal d'un support, entre 2 et 4.
- `S*` est le premier support de cardinal q dans l'ordre lexicographique des `SiteIdx` (`MATH:15–28`).

**Primitives exactes (`MATH:49–71`).**
- Le centre s'écrit c = a + N/D, avec D > 0 ; il existe une formule par arité.
- La puissance est H_b(y) = D‖y−a‖² − 2N·(y−a) : négative à l'intérieur, nulle sur la coquille, positive à l'extérieur.
- Un support q3 exige un triangle strictement aigu ; un support q4, quatre coordonnées barycentriques strictement
  positives.
- Un triplet qui sert à **chercher** un tétraèdre n'a pas à être aigu : un préfixe q3 obtus ne coupe pas q4.

### 1.2 L'objet FULL et T1 (`MATH` § 4)

**Définitions.**
- W_F(a) = ∩_{x∈F} B̄(x, √a), et L_k(a) = ∪_{|F|=k} W_F(a).
- Γ_k(a) a pour sommets les k-parties telles que β(F) ≤ a. Toute (k+1)-partie G telle que β(G) ≤ a relie toutes ses
  faces de cardinal k.
- Coupe stricte : boules **ouvertes**, β < a, et L_k^<(a) = ∪_{a′<a} L_k(a′).

**T1 [V].**
- Énoncé : π₀Γ_k(a) ≅ π₀L_k(a), en compatibilité avec les inclusions de niveaux. Il en va de même aux coupes strictes,
  et les verticales viennent de L_k ⊆ L_{k−1}.
- Preuve complète :
  - W_F ∩ W_{F′} = W_{F∪F′} est non vide si et seulement si β(F∪F′) ≤ a ;
  - les échanges d'un élément à l'intérieur de F∪F′ forment un chemin dans Γ_k ;
  - une réunion finie de convexes compacts, ou de convexes ouverts, a les composantes de son graphe d'intersection ;
  - la verticale ne dépend pas de la face choisie, puisque toutes les faces sont reliées par F.
- Registre : couvert par le théorème 2 (`theorem_external`, `REG:30`) et la proposition 5 (`REG:38`). Aucune ligne v11.

**FULL.** Une forêt par ordre :
- fusions N-aires, aucun parent au niveau de son enfant ;
- racine unique pour k ≤ n ;
- applications verticales vers l'ordre k−1, lues à la coupe **fermée**.

### 1.3 Catalogue et complétude conditionnelle (`MATH` § 3)

**Catalogue.** Cat_K = {b : p+q ≤ K+1}. La fenêtre d'événements est [p+q−1, p+m] ∩ [1, K].

| Id | Énoncé | Preuve (résumé) | Code |
| --- | --- | --- | --- |
| G1 | z domine strictement x sur la boîte fermée Q si et seulement si ‖x−l‖² − ‖z−l‖² > Σᵢ max(0, 2sᵢ(xᵢ−zᵢ)). On retire x s'il a au moins K dominateurs distincts, pris n'importe où dans X. | Minimum d'une forme affine sur la boîte ; l'égalité conserve le candidat. | `src/catalogue/boxes.cpp:41-76` (`>` strict), `internal.hpp:119` |
| G2 | Si c ∈ Q et p < K, alors P_b ⊆ L. Si p ≥ K, L contient K intérieurs. Un recensement sur L qui trouve moins de K intérieurs est donc exact, coquille entière comprise. | Le K-ième voisin est à distance au moins √λ. | `src/catalogue/leaf.cpp:472` |
| G3 | On pose θ_r = K+1−r et on rejette un préfixe de cardinal r dont l'union des dominateurs dépasse **strictement** θ_r. | Un dominateur d'un sommet est strictement intérieur. Aucun préfixe de S* n'est rejeté, car p ≤ θ_q ≤ θ_r. | `leaf.cpp:323`, `leaf_device.hpp:310` |
| G4 | Sur une partition finie, demi-ouverte et entièrement traitée, la sortie est exactement Cat_K. La boîte peut être rétrécie à ∏[min Lᵢ, max Lᵢ+δ). | G1–G3 préservent S* ; le propriétaire du centre est unique ; c ∈ conv(U_b) ⊆ bbox(L). | `src/catalogue/frontier.cpp`, `adaptive_frontier.cpp` |

G4 est **conditionnelle**. Elle ne prouve ni la terminaison d'une politique de subdivision, ni une borne de travail.
Un plafond produit un refus (`wide_leaf`), jamais une sortie dite complète.

**M1 et M2 [V].**
- M1 : B(F) existe et est unique. Une boule contenant F est B(F) si et seulement si son centre est dans l'enveloppe
  convexe des points de F situés sur sa frontière.
- Preuve de M1 : l'identité du milieu donne l'unicité ; Σtᵢ‖xᵢ−y‖² = λ + ‖c−y‖² donne la suffisance ; une séparation
  stricte donne la réciproque ; Carathéodory donne au plus 4 sites.
- M2 : si S est un support de b et S ⊆ F ⊆ P_b, alors B(F) = b.
- Code : `src/num/sphere.cpp`, `src/tower/meb.cpp` (MEB bornée à 12 sites), `src/tower/locate.cpp:1` (identification
  par M1/M2).

### 1.4 Événements locaux : T2, morceaux, T3, plateau (`MATH` § 5 et § 10.3)

**T2, trace stricte [V].**
- Énoncé : pour F ⊆ P_b, β(F) < λ si et seulement si F ∩ U_b est séparable (c ∉ conv(F∩U)).
- Sens direct : par M1. Réciproque : déplacer légèrement le centre le long d'un séparateur.
- **Morceaux locaux.**
  - Si p ≥ k, il y a un seul morceau.
  - Sinon, on pose t = k−p. Les morceaux sont les composantes du graphe des t-parties séparables de U_b, deux sommets
    étant reliés si leur réunion est séparable.
- L'image dans le global est une **surjection** : des chemins extérieurs à P_b peuvent réunir des morceaux. Il faut
  dédupliquer les racines.
- J'ai vérifié la réduction de tout chemin strict induit à des traces comprimées I∪A : une arête stricte G a une trace
  séparable qui contient les deux A.
- Code : `src/tower/cells.cpp:1, 76-130` et `cells_classify.cpp:1`. Le test sur A utilise la MEB de A, pas celle de
  I∪A.

**T3, fenêtre [V].**
- Si |P_b| < k, il ne se passe rien. Si |P_b| = k, la partie naît isolée (par l'unicité de M1).
- Pour 1 ≤ t ≤ q−2, toutes les t- et (t+1)-parties sont séparables : un seul morceau, aucun événement. D'où la fenêtre
  [p+q−1, p+m] et le catalogue p+q ≤ K+1.
- En coquille régulière (m = q) : jonction des q morceaux à k = p+q−1, naissance à k = p+q.

**T4 et lemme P, plateau atomique.** Au § 5, T4 n'est qu'une recette. Sa forme démontrée est le lemme P
(`MATH:486-528`) [V] :
1. Un sommet neuf F de niveau λ n'appartient qu'à P_{B(F)} parmi les boules de niveau λ, et ses arêtes de niveau λ ont
   la même boule.
2. Une boule de niveau λ hors de W_K (p+q ≥ K+2) ne touche qu'une composante stricte, dont les K-parties couvrent tous
   ses sites.
3. Les composantes de Γ_K(λ) qui contiennent un sommet strict correspondent aux composantes du biparti H_λ (composantes
   strictes × cellules de W_K). Les autres sont exactement les naissances de W_K.

Code : `src/tower/forest_plateau.cpp:40-114` (publication en fin de plateau, enfants strictement plus bas), avec
`ForestBuilder::close`.

### 1.5 Descente, mémo, suffisance, verticales (`MATH` § 6)

**T5 [V].**
- Règle : si p ≥ k, prendre une k-partie quelconque de I_b ; sinon prendre I_b ∪ A avec A séparable de cardinal t ;
  sinon s'arrêter, c'est une naissance.
- Chaque pas fait strictement baisser β (par T2) et reste dans la composante de F à la coupe fermée β(F), par les
  échanges dans P_b.
- Terminaison : la décroissance est stricte sur un ensemble fini, au plus C(n,k)−1 pas.
- Le terminal n'est pas unique : sur {0,2,4} à k = 2, deux paires sont possibles.
- Code : `src/tower/descent.cpp:103-147` (t < q donne une trace analytique, sinon MEB de A) et `:186-195` (contrôle
  d'une baisse stricte, refus `tower_invariant`).

**Mémo daté [V].**
- Un mémo de la cellule (b, k) est valide à la coupe fermée a ≥ λ_b, ou à la coupe ouverte a > λ_b.
- La graine d'une descente vaut à partir de **β_initial**, jamais de β_terminal (témoin X = {0,2,4,6}).
- Code : `src/tower/descent_memo.cpp:78` et `docs/DESCENT_MEMO.md`.

**T6, suffisance constructive conditionnelle.**
- Énoncé : catalogue complet, populations exactes, représentants qui couvrent chaque composante stricte rencontrée,
  descentes valides et T4 suffisent à reconstruire FULL.
- Au § 6, c'est une récurrence en trois lignes. Pour l'arbre d'ordre K, elle est complétée par les lemmes A–E.
- Les verticales reposent sur deux observations. Les (k−1)-faces de P_b sont reliées à λ_b (lemme A à l'ordre k−1).
  Une fusion a l'image de ses enfants remontée à son niveau.
- L'argument est correct [V] mais n'est rédigé qu'en quelques phrases (`MATH:237-241`).
- Code : `src/tower/forest_vertical.cpp` (`build_forests`) et `forest_ancestor_sweep.hpp:44-62`. Toutes les fusions
  d'un niveau sont activées avant une requête fermée.

### 1.6 Sortie d'ordre K (`MATH` § 10)

**W_K et vocabulaire.**
- W_K = {b ∈ Cat_K : p+m ≥ K}.
- Une **naissance** est une boule de W_K sans K-partie stricte. Une **cellule** est une autre boule de W_K.
- Une boule est **forte** si p+q ≤ K ≤ p+m, **faible** si K = p+q−1.

Tous les lemmes du § 10 figurent au registre en `proved_here` (`REG:1362-1373`) ; mes vérifications et le code sont
dans le tableau.

| Lemme | Énoncé (résumé) | Vérification de la preuve [V] | Code |
| --- | --- | --- | --- |
| A | Les K-parties de P_b sont dans une même composante de Γ_K(λ_b). On a c_b ∈ W_F(λ_b) si et seulement si F ⊆ P_b. att(b) est le nœud vivant à la coupe **fermée** λ_b, lu après tout le plateau. | Complète : échanges dans P_b. | `src/tower/attachment.cpp` |
| P | Plateau (ci-dessus). | Complète. Le cas « composante sans sommet strict » est traité. | `forest_plateau.cpp` |
| W | 1. Une boule hors de W_K ne crée ni ne réunit aucun nœud. 2. Retirer ses liaisons de Γ_K change pourtant T_K. 3. Toute liaison de Gabriel a sa boule dans W_K. 4. Le théorème 4 tient sans position générale. | Complète. W.2 est un calcul sur E5 et D2. W.4 passe par z ∈ I_b∖G et deux (K+1)-parties strictes qui partagent une face. | lecture du vrai Γ_K par les descentes |
| B | Rôles : une naissance de W_K correspond bijectivement à une naissance de T_K (K ≥ 2). Une cellule a le rôle fusion si rang(att) = r_b, interne sinon. | Complète, par P.3. | `attachment.cpp:58, 133` |
| C | ant(b) est l'ensemble des nœuds de coupe **ouverte** des traces comprimées. Au rôle interne, ant(b) = {att(b)}. Au rôle fusion, ant(b) ⊆ enfants(att(b)). La réunion sur les boules de fusion d'un nœud v égale enfants(v). | Complète. Les échanges sont possibles car \|F∩U\| > t. | `attachment.cpp` |
| D | Une graine g de trace stricte, avec a_g ≤ a′ < λ_b, donne u = anc^<_{λ_b}(g) dans ant(b), puis att(b) par la règle du parent. | Complète. N'exige **pas** β(F) ≤ ℓ(r_b−1) (témoin D2). | `src/tower/seed_log.hpp`, `attachment.cpp` (journal E1) |
| E | Pour toute K-partie de P_b, l'ancêtre fermé à λ_b de la naissance terminale est att(b). | Complète. | juge E2 hors produit (`tests/tower/attach_judge.cpp`) |
| F | Q_b est la famille des parties non séparables **minimales** de U_b (Carathéodory strict). Prédicats exacts par arité. B(Q) = b, donc les Q_b sont disjoints. | Complète. | `src/supports/enumerate.cpp` |
| G | `kparties_reliees` = C(p+m, K) ; `cofaces` = Σ_j C(p, K+1−j)·N_j ; `strict_traces` = C(m,t) − N_t ; etc. | Complète. Valeurs régulières recalculées : jonction K+1, 1, q, q, 1 ; naissance 1, 0, 0, 1, 0. | `src/supports/counts.cpp` |
| H | À K ≥ 2, pts(C_v(a)) = ∪{P_b : att(b) ⪯ v, λ_b ≤ a}, et les boules fortes suffisent. | Complète : construction de F₁ de trace inférieure à q, contradiction avec la minimalité. Les sites couverts égalent pts(C) [V]. | `src/points/incidences.cpp:24-86` |

**Proposition de suffisance et sélection de Kruskal au plateau (`MATH:918-940`) [V].**
- Énoncé : naissances, boules de fusion, rangs, att et ant déterminent T_K. Kruskal parcourt les boules de fusion de v
  dans l'ordre des `BallIdx` et garde une boule dès qu'elle réalise au moins une union.
- La conclusion est juste. Mais la preuve écrite tire la connexité finale du lemme C.3, qui ne donne que l'égalité des
  réunions.
- La connexité vient en fait de P.3 : la composante de H_λ qui forme v est connexe, et toutes ses cellules ont le rôle
  fusion pour v (B.2).
- Code : `src/supports/hierarchy.cpp:38`, `supports.hpp:213-222`.
- Le lecteur exige « chaque boule réalise une union, enfants finalement connexes » (`bench/mhgp11_formats.py:838-868`).

### 1.7 De FULL aux points : P1–P5 (`MATH` § 7)

| Id | Énoncé | Preuve [V] | Code / registre |
| --- | --- | --- | --- |
| P1 (core) | x entre à D_k(x) dans la composante de L_k(D_k(x)) qui contient le point x. | Toutes les k-parties de la boule B(x, √D_k) sont reliées. | export core ; aucune ligne |
| P2 (cover) | A_k(x) = min_{F∋x} β(F) ; E_k(x) est un **ensemble** de composantes fermées ; D_k/4 ≤ A_k ≤ D_k. | Complète. | aucune ligne |
| P3 | Une première entrée a un témoin **fort**. E_k(x) est l'ensemble des composantes fermées des témoins de ce niveau. | Complète : construction d'une trace de moins de q sites. Sa forme datée est le lemme H. | `points/incidences.cpp` |
| P4 | À ordre fixé, toute règle qui attache chaque site une fois est laminaire. Entre ordres, ce n'est pas vrai (sept sites). | Triviale ; contre-exemple rejoué [R]. | `points/hang.cpp` ; pas de ligne pour le témoin à sept sites |
| P5 | Entrelacement de FULL en **rayon** : L_k^X(r²) ⊆ L_k^Y((r+ε)²) ; \|α^X − α^Y\| ≤ ε avec α = min_z max(‖z−x‖, ρ_k(z)). La date LCA n'hérite pas de cette stabilité. | Complète. J'ai vérifié la formule de α par cas sur le rang de x. | `REG:1304-1305` (trilemme, 3ε) |

### 1.8 Juges J1–J3 (`MATH` § 8)

**J1 [V].** Cat_{K′} filtré par p+q ≤ K+1 redonne Cat_K, puisque p et q sont intrinsèques. Le juge est implanté dans
`bench/catalogue_euler.cpp`, hors produit (`REG:1339-1350`).

**J2 [V].** À l'ordre 1, les composantes sont celles de l'arbre couvrant euclidien minimal coupé à 4a (propriété du
cycle). **Aucune porte ne l'implante.** J'ai cherché dans `tests/*/tests.cmake` et dans le banc : le seul « J2 » du code
désigne autre chose, à savoir la droite des centres du catalogue (`center_line_cache.hpp:1`).

**J3, Euler [V, R].**
- Identité : n·[k=1] + Σ_b e_k(b) = 1, avec e_k(b) = Σ_{A⊆U, c∈conv A, \|A\|≥t} (−1)^{\|A\|−t} C(\|A\|−1, t−1).
- La preuve complète est celle du nerf de la v9. On applique la valuation d'Euler aux intersections de boules, qui sont
  convexes. On utilise 1_{t≥K} = Σ_j w_{j,K} C(t,j) avec w_{j,K} = (−1)^{j−K} C(j−1, K−1), puis on regroupe par MEB.
- La forme polynomiale t^p Σ_T (t−1)^{\|T\|−1} redonne exactement le coefficient de `MATH:352` [V].
- Cat_{K+2} suffit pour juger k ≤ K, car q ≤ 4 : `REG:1349`.
- Le juge est nécessaire, jamais suffisant : compensation à 5 sites, recalculée à la main (triangle +1 de centre
  (5,5), paire −1 de centre (40,5), niveau 25) et rejouée par le reçu `math_locks_review` [R].
- Le catalogue s'arrête à K ≤ 12 (`docs/CATALOGUE.md:145`, « K dans 1..12 ») : J3 ne peut donc juger que les ordres
  1 à 10 [V].

### 1.9 Lemmes de calcul (raccourcis exacts)

Chaque élagage cite son lemme dans le code (`docs/ARCHITECTURE.md` § 1, règle 10). Aucun de ces lemmes n'a de ligne au
registre.

| Lemme | Énoncé | Preuve [V] | Code |
| --- | --- | --- | --- |
| Table de populations | Si une k-partie F égale P_b, alors B(F) = b (M2), p < k et t = m. Le pas est terminal, avec β_initial = β_terminal = λ_b. Une telle b est toujours dans Cat_K. | Complète : p+q ≤ p+m = k ≤ K. | `src/tower/population_lookup.hpp:2`, `.cpp:156` |
| R (census par masques, feuille J3 de la v10) | Un dominateur d'un générateur est intérieur ; un dominé est extérieur. Un site présent dans les deux unions est un invariant violé. | Complète. | `src/catalogue/leaf.cpp:243` ; `docs/CATALOGUE.md:113` |
| V3 (bornes entières) | Minorant exact de H sur les **points entiers** de la boîte : l'entier le plus proche de c_j, ramené dans la boîte. Majorant au coin le plus éloigné. | Complète (parabole convexe par axe). **Réservé aux sites** : sur un segment q2, la valeur est 0 aux sites et −D/2 au milieu. | `src/num/lattice_bounds.hpp`, `src/index/census.cpp` ; `docs/INDEX.md:77-92` |
| Diamètre | Si la boule diamétrale de la paire la plus éloignée (la première dans l'ordre lexicographique) contient F, c'est la MEB. Sinon, aucun support q2. | Complète. | `src/tower/meb.hpp:41` ; `docs/MEB_DIAMETRE.md` |
| Cliques | Un support centré dans Q est une clique du graphe des paires sans dominance stricte. | Complète. | `src/catalogue/small_pair_graph.hpp:2` |
| Zonogone (« lemme Z ») | La droite équidistante de trois points rencontre la boîte fermée si et seulement si le zonogone contient 0, testé sur les normales des côtés. | Complète. | `src/num/center_region.cpp` ; `docs/CENTER_REGION.md` |
| M3 / E4 | Le centre circonscrit d'un triangle aigu est intérieur au triangle médian. Un centre intérieur au tétraèdre est dans la boîte de ses sommets. | Complètes. | `leaf.cpp:189, 201` ; toujours actifs sur CPU (audit final, § 16) |
| Niveaux q3/q4 différés | `Level` n'est jamais lu par `power`, `side`, `orientation` ou `strictly_inside`. On matérialise après l'admission, avec la même formule brute. | Complète (signatures de `src/num/geometry.hpp:80-166`). | `Q3Candidate`, `Q4Candidate` |
| Réemploi vertical régulier | Si m = q, la face R = I∪(U∖{u}) est une trace stricte. Si F ≠ R, F∪R = P et β(P) = λ_b : image identique à la coupe fermée. | Complète. Limité à m = q. | `src/tower/regular_vertical_seeds.hpp:29` ; `docs/FULL_REGULAR_VERTICAL_REUSE.md` |
| MST/contraction (auditeur, O7) | Sur les étoiles de graines pondérées par rang, tout arbre couvrant minimal a les composantes du graphe par seuil. Contracter les chaînes de fusions de même rang redonne la forêt N-aire. | Complète (propriété du cycle). Le MST ne transporte **pas** les compteurs logiques (triangle (0,0), (3,0), (1,2)). | `receipts/audit_plan_gpu_20261006/mathematics/REPORT.md` ; non implanté ; **non inscrit** |

## 2. Audit critique des preuves

### 2.1 Complètes

- M1, M2, G1–G3, T1, T2 (morceaux et surjection compris), T3, T5, P1–P5, J1, J2.
- J3, mais dans le document de la v9.
- A, P, W.1–4, B, C, D, E, F, G, H.
- Tous les lemmes de calcul du § 1.9.

### 2.2 Esquisses, conditionnels, appuis empiriques

- **T4 (§ 5)** : c'est une recette (« graphe biparti avec les événements »). Le lemme P l'établit, avec les boules hors
  fenêtre (P.2) et l'isolement des naissances (P.1). La v12 doit énoncer T4 *comme* le lemme P.
- **T6** : récurrence esquissée. La partie « verticales » tient en cinq lignes, correctes. La v12 doit l'écrire comme un
  théorème complet, avec ses hypothèses : catalogue complet par rang, graines datées β_initial < λ.
- **J3** : `MATH:356-358` résume en deux lignes une preuve qui n'est complète qu'en v9 (valuation d'Euler sur l'anneau
  polyconvexe, puis regroupement par MEB).
- **G4 et T6 sont conditionnels par construction.** L'exactitude du moteur vaut « sauf refus ». Aucune terminaison et
  aucune borne sous-quadratique ne sont démontrées.
- **Kruskal** : citation fautive (C.3 au lieu de P.3), voir § 1.6.
- **Appuis empiriques** :
  - E1 = E2 et l'identité FULL/v10 à l'échelle reposent sur des portes ;
  - les trois mutants « équivalents » de la référence (`jump_any`, `first_rep_last`, `hull_strict_triangle`) sont
    justifiés mathématiquement (liberté de T5, lemme F), mais ne sont exercés que sur quatre fixtures chacun
    (`reference/ref_mutants.py:138-151`, `test_ref.py:415-445`).
- **L'identification π₀L_k = π₀Γ_k (T1, théorème 2) est invoquée, pas re-vérifiée.** La contre-épreuve de l'auditeur
  (`receipts/audit_geant_20261005/math`) en vérifie la partie combinatoire : graphe d'intersection complet contre graphe
  de Johnson, par une route d'adjacence propre. Elle **partage les MEB de l'étage A** (son propre `REPORT.md` le dit,
  l'audit final l'omet).

### 2.3 Registre

**Aucun statut V11 n'est plus fort que sa preuve écrite** [V]. Les écarts :

1. **Lignes manquantes.**
   - Le cœur du contrat : M1–M2, G1–G4 (complétude conditionnelle), T1–T6, P1–P5, J2.
   - La suffisance de Kruskal (§ 10.10 bis).
   - Les lemmes de calcul du § 1.9, dont le lemme MST/contraction.
   - La passation (§ 7) le demande pour MST/contraction, mais l'ancienne ligne générique « contraction des plateaux par
     composantes fortement connexes : `proof_obligation` » (`REG:243`) reste ouverte. Le lemme P et MST/contraction la
     ferment pour les deux constructions v11 ; il faut l'écrire avec cette portée.
2. **Contradictions non inscrites**, contrairement à la règle de `CLAUDE.md` : garder toutes les boules de rôle fusion
   (cycle du triangle en K1) ; « feuilles ≤ sites » ; F3 par nombre d'instructions ; F2 par degré ; mémo valide dès
   β_terminal ; sept sites entre ordres ; {0,2,5} cover ≠ MR₂-bord ; SPv2 et translation (§ 2.5).
3. **Défaut de forme.**
   - `REG:1370` contient `|\n|` en littéral : la ligne « E — … `proved_here` » est rendue **à l'intérieur** de la
     ligne `false_in_general` de D2 (introduit par `5adf6a59f`).
   - [R] `tools/check_docs.py` sort en code 1 sur 213 liens morts, tous dans les reçus de `morsehgp3D_v10`. Aucun ne
     concerne la v11, et le `\n` n'est pas détecté.
4. **Citations instables.**
   - La ligne « 3ε » (`REG:1305`) cite « réponse Q1 » via `audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md`, une
     note réécrite en place. Son Q1 actuel (l. 283) traite d'autre chose. La preuve est dans
     `receipts/points_answers_20261003/tower_math/README.md`.
   - De même, `MATH:5` renvoie aux « corrections Q1–Q5 acceptées » dans cette note : leur texte n'existe plus qu'à
     `b5162fff2` et dans `receipts/audit_dialogues_20261004/REPONSE_CLAUDE_VERROUS_MOTEUR_20261002.md.snapshot`.
5. **Mentions périmées.**
   - La ligne D dit « la réalisation E1 reste à qualifier » ; elle l'a été sur G4 le 6 octobre (`docs/SORTIES.md`
     § 11).
   - [I, domaine D] La ligne insertion (`REG:1306`) et la note courante de l'auditeur (« l'impossibilité générale Q6
     sous trois axiomes est fausse », note l. 1052) méritent d'être réconciliées.

### 2.4 Incohérences documentaires et collisions de notation [V]

**Documents.**
- **Deux § 10.10** dans `MATH` (l. 866 et 918), déjà relevés par la passation.
- **Le § 10 n'a pas été mis à jour après la décision du 6 octobre.**
  - § 10.6 : plafond de 24 sites. En v2, le seul plafond est m ≤ 255 (`supports.hpp:219-221`).
  - § 10.7 : bornes C(35,13).
  - § 10.8 et § 10.9 décrivent la réalisation par tous les Q_b (v1).
  - § 10.10 (invariance) ne vaut que pour la v1.
- **En-tête de profil** : `MATH:8` annonce u18 et le § 10 u21. `ARCHITECTURE.md` § 3 dit « 21 par défaut » et § 7.3
  « profil initial 18 bits ». L'option CMake des matrices G4 annonce encore « seul 18 est qualifié ».
- « **cellule** » désigne la paire (boule, ordre) aux § 5–7, mais une boule de W_K non naissance au § 10.3.

**Collisions d'identifiants.**

| Identifiant | Sens multiples |
| --- | --- |
| T3 / T5 | `descent.cpp:1` appelle la descente « T3 », `MATH` l'appelle T5 |
| J2 | juge de l'arbre couvrant euclidien minimal, ou droite des centres du catalogue |
| J3 | juge d'Euler, ou feuille J3 de la v10 |
| P1 | core, règle d'ancrage P₁ des points, items P1/P2 et P9/P10 de l'audit |
| G4 | lemme, ou machine |
| V3 | levier des bornes entières, ou section V3 du registre |
| R | lemme R, contrats R1–R7, raccord R2 de la v10 |
| E1 | journal des graines, ou expérience de sortie plate |
| m | taille de coquille, ou seuil de qualification des points |
| T1 | théorème, ou item d'audit T1 |

### 2.5 Constat nouveau : SPv2 n'est ni équivariant par translation ni plus stable que la v1

**Les règles écrites.**
- `MATH:932-940` : Kruskal parcourt les boules de fusion « dans l'ordre des `BallIdx` (niveau, puis S*) ».
- `S*` est le premier support d'arité q dans l'ordre lexicographique des `SiteIdx`, qui sont les rangs de Morton
  (`src/catalogue/catalogue.hpp:53`, `sort_indices.cpp:29`).
- L'ordre de Morton n'est pas invariant par translation.

**Premier témoin : la translation [R, script `spv2_translation.py`].**
- Le modèle reproduit ces règles.
- Triangle équilatéral (0,1,1), (1,0,1), (1,1,0) à K = 1 : trois boules diamétrales de niveau 1/2, toutes de rôle
  fusion, avec un tiers site extérieur à chacune.
- Les arêtes gardées changent selon la translation : {02, 12} sans translation, {01, 02} pour (1,0,0), {01, 12} pour
  (0,1,0).
- `docs/SORTIES.md` § 10 (« les listes propres comme ensembles […] invariants ») et `MATH` § 10.10 surdéclarent donc.
  T_K, les niveaux et les rôles restent invariants. Le **certificat publié** ne l'est pas.

**Second témoin : la stabilité [R, `sstar_circle.py` et `circle_roles.py` sur l'oracle S1].**
- Sur le cercle de l'audit (coordonnées de la fixture, n = 3, 5, 1023), la boule de niveau d² est de rôle **fusion** à
  K = 2 avant et après la perturbation.
- `S*` vaut **BD** avant et **AC** après : le support publié saute d'un rayon entier (d), contre au moins d/4 pour le
  porteur v1.
- PASSATION § 4 et l'audit final (§ 4.3), qui présentent la v2 comme le remède à l'instabilité du porteur, sont donc
  inexacts. La v2 répondait à une décision de taille de sortie, pas de stabilité.

[I] Pour la v12 : départager par l'ordre lexicographique des **coordonnées** des sites. Il est invariant par
translation, comme la numérotation de T_K par centres. Déclarer la non-stabilité de `S*`.

### 2.6 Corrections aux documents de passation

| Affirmation | Verdict |
| --- | --- |
| « Lemmes A–H … et la suffisance de Kruskal ont statut `proved_here` » au registre (rapport A § 2 ; audit final § 4.2) | **Faux pour Kruskal** : aucune ligne. La ligne E est mal formée. |
| « M1/M2, G1–G4, T1–T6, P1–P5 et J1–J3 sont démontrés » | Vrai sur le fond. T4, T6 et J3 sont des esquisses dans `MATH` ; aucune ligne au registre ; J2 jamais implanté. |
| Galerie dans « `tests/fixtures/` » (PASSATION § 3) | Ce dossier n'existe pas dans la v11. Seuls E5 et le polyèdre sont dans `tests/fixtures/regressions/` à la racine. |
| « 9 sites pour 80 feuilles » gravé en fixture (audit final § 12) | Modèle du reçu seulement. La porte native utilise 3 000 sites (27 048 feuilles, `00800dd88`). |
| « Garde i64 du test cubique dépassée dès u21 » | Trou de **couverture** de porte. Le produit élargissait déjà (`Int<3B+5>` en i128 à u21) ; l'auditeur le dit dans `receipts/audit_independant_20261002/center_region_published_review_15`. |
| « LevelSource » et « Racine 2^127−1 » listés comme deux pièges | Un seul défaut : témoin R = 2^127−1, corrigé par `510dae50e`, frontière gravée en `38b76701b` (`tests/head/head_test.cpp`, porte `huge`). |
| Contre-épreuve de l'auditeur « comparée au nerf » | Exact, mais elle partage les MEB de l'étage A. |
| Oracle « 1 362 ordres … » | Exact [R]. Il manque la suite complète jouée sur G4 : 5 617 nuages, 38 633 ordres, 3 365 240 coupes, 671 214 nœuds (`receipts/audit_independant_20261002/g4_qualification_review_3/…/LastTest.log`, `audit_g4_finl_20261005/summary.json`). |
| « Vidages v10 identiques sur 190 nuages » | Établi en local le 2 octobre (`reference/README.md`). Aucun reçu G4 trouvé : `MHGP11_V10_FROZEN_DIR` est vide dans les provenances de build G4 que j'ai lues. |
| v2 = remède à l'instabilité du porteur | Faux (§ 2.5). |
| J3 « à 8 000 / 16 000 / 32 000 et sur les trames, K5 et K10 » | Synthétique à K5 seulement ; LiDAR à K5 et K10 (`tests/catalogue/tests.cmake:138-164`). |

## 3. Doctrine numérique

### 3.1 Budgets `constexpr` [V] (`src/num/budgets.hpp`)

Ici M = 2^B et toute différence de coordonnées est inférieure à M en valeur absolue. Toute expression se calcule en
`Int<bits>` : i64 jusqu'à 63 bits, i128 jusqu'à 127, puis `Wide`. Un dépassement est une erreur de compilation
(`integer.hpp:13-28`).

| Expression | Bits | B18 | B21 | B24 | Justification |
| --- | --- | ---: | ---: | ---: | --- |
| produit scalaire / produit vectoriel | 2B+2 / 2B+1 | 38 / 37 | 44 / 43 | 50 / 49 | inférieur à 3M² / 2M² |
| déterminant | 3B+3 | 57 | 66 | 75 | six produits, inférieur à 6M³ |
| N (q3) / D (q3) | 5B+5 / 4B+5 | 95 / 77 | 110 / 89 | 125 / 101 | 24M⁵ / 24M⁴ (produit vectoriel de vecteurs quelconques ≤ 2M²) |
| N (q4) / D (q4) | 4B+5 / 3B+4 | 77 / 58 | 89 / 67 | 101 / 76 | 18M⁴ / 12M³ |
| `side` (puissance) | 6B+8 | 116 | 134 | 152 | 216M⁶ < 256M⁶ |
| niveau num / den | 8B+12 / 6B+8 | 156 / 116 | 180 / 134 | 204 / 152 | q4 : 3(18M⁴)² < 1024M⁸ ; (12M³)² < 256M⁶ |
| comparaison de niveaux | 14B+20 | 272 | 314 | 356 | toujours `Wide` |
| comparaison de centres | 9B+11 | 173 | 200 | 227 | toujours `Wide` |
| orientation avec centre | 7B+9 | 135 | 156 | 177 | certificat, sinon `Wide` |

**Que veut dire « q3 = 216M⁶ » [V] ?**
- C'est la somme des magnitudes de H = D‖v‖² − 2N·v avec D < 24M⁴ et \|N_j\| < 24M⁵ : 72M⁶ + 3·48M⁶ = 216M⁶.
- C'est natif seulement à B18 (moins de 2^116). Même avec la borne serrée « trois points d'un même cube »
  (\|produit vectoriel_j\| < M²), on trouve 90M⁶ > 2^127 à B21 : q3 reste dur.
- q1 (3M²), q2 (12M²) et q4 (72M⁵ < 2^127, `static_assert(5B+7 ≤ 127)`) sont natifs aux trois profils
  (`predicates.cpp:47-63`).
- « Seul q3 est dur » ne vaut donc que pour la puissance et le côté. Les comparaisons de niveaux et de centres sont
  toujours larges ; elles passent par les clés F3/F4.

### 3.2 Trois voies et certificats [V]

`center_power` et `center_side` (`predicates.cpp:43-102`) suivent trois voies :

| Voie | Quand | Garantie |
| --- | --- | --- |
| **native** | `Budget::side` ≤ 127, ou arité ≠ 3, ou q3 certifié | prouvée |
| **contrôlée** | sinon, d'abord | `__builtin_mul/add_overflow` ; tout drapeau abandonne la valeur, sans résultat partiel |
| **Wide** | si la voie contrôlée échoue | exacte, à partir des coefficients d'origine |

**Certificat de puissance q3**, calculé une fois par la fabrique (`power_certificate.hpp`) :
- conditions : 0 < D < 2^(123−2B) et \|N_j\| < 2^(124−B) ;
- seuils : 2^87/2^106, 2^81/2^103, 2^75/2^100 selon le profil ;
- somme des magnitudes inférieure à 15·2^123 < 2^127 pour **tout** point du profil [V].

**Certificat d'orientation** (`orientation_certificate.hpp`) :
- conditions : D < 2^(124−3B) et \|N_j\| < 2^(124−2B) ;
- T_j < 2^(125−2B), chaque produit est inférieur à 2^125, la somme à 3·2^125 [V].

**Poids q4** : si L ≤ 2^20, tous les intermédiaires valent au plus 117L⁶ < 2^127 ; sinon `Wide` sur 6B+7 bits
(`docs/Q4_POIDS_PRESENTATION.md`) [V].

### 3.3 Flottant F1–F6 [V] (`docs/ARCHITECTURE.md` § 4)

- **F1** : aucune décision n'est prise en flottant.
- **F2** : noyaux binary64 exacts si la somme des |termes développés| reste inférieure à 2^53.
- **F3** : exposition propagée **par expression**. Conversion : E = 1. Produit de n facteurs : ΣE + n − 1. Quotient :
  E_a + E_b + 2. Somme de même signe : max + n − 1.
- **F4** : on ne déclare x < y que si x̃ < c·ỹ, avec c = 1 − 2^−40 ≤ (1−u)^(E_x+E_y+1). J'ai vérifié la chaîne
  d'inégalités, arrondi du produit compris.
- **F5** : auto-test, sans valeur de preuve.
- **F6** : filtre de signe, avec τ = 2^(q+e−51) ≥ 2EuM.

**Implantation de F3/F4 vérifiée** (`src/catalogue/sort_level_key.hpp`) :
- 64 bits de tête tronqués puis convertis : E = 2 par entier, 6 pour le quotient ;
- `static_assert(2·6+1 ≤ 4096)` ;
- les zéros sont décidés exactement.

Le filtre F6 sur la puissance a été retiré (gain d'environ 1 %, `docs/PERFORMANCE_FULL.md`, fin).

**Réfutations rejouées [R].**
- **F3 par nombre d'instructions** : N = 2^53+1, D = 2^53, un quotient puis quatre carrés. Le binary64 donne 1, l'erreur
  réelle vaut environ 8u, au-delà de la borne « 7 instructions » de 7u. L'exposition réelle est E = 63. Reçu
  `floating_bounds/check.py` rejoué octet pour octet (`4ae1ee46…`).
- **F2 par degré** : x = 32767, A = 512x³ ∈ [2^53, 2^54). En arrondi au plus proche, (A+1) − A − 1 donne −1
  (`math_locks_review/check.py`, identique au reçu). J'ai refait le calcul de l'arrondi pair.

### 3.4 Pièges de bornes intermédiaires

| Piège | Vérification |
| --- | --- |
| s = 2^21−1, triangle aigu (0,0,0), (s,0,0), (1,s,0), témoin (s,s,s) | [V] Recalcul : D = 2s⁴, N = (s⁵, s⁵−s⁴+s³, 0). Le premier produit vaut 6s⁶ ≈ 2^128,6, la puissance finale 2s⁶+2s⁵−2s⁴ < 2^127. Non certifié, car D ≈ 2^85 > 2^81. Repli contrôlé puis `Wide`. |
| Triangle (0,0,0), (L,L,0), (L,0,L) | [V] H = 4L⁶ au quatrième coin, premier produit 12L⁶ (`PREDICATS_I128_CONTROLES.md`). |
| Garde i64 cubique à u21 | Témoin 0, (m,m,0), (m,0,m), boîte [0,1]³ : 2m³ − 2m² > INT64_MAX dès u21 [V]. Le produit élargissait déjà ; c'est une porte ajoutée (`region_cubic_width`), pas un défaut corrigé. |
| Export u24 | [V] Tétraèdre régulier, L = 2^24−1 : niveau 12L⁸/16L⁶, soit 196 et 148 bits. Trois mots de 64 bits ne suffisent pas (192 bits) ; corrigé par `3bd4d734e` (quatre mots, format version 2). |
| `LevelSource` / 2^127−1 | Racine admise en u128, calculée en i128 : `rt+1` déborde (témoin abstrait). Corrigé par `510dae50e` (garde 2^100), frontière exacte gravée par `38b76701b`. Le `check.py` du reçu `audit_s10_root_guard_20261005` **n'est pas rejouable** : le dossier `snapshot/` est absent [R]. |
| Contrat de sites, pas de boîte | Le minorant V3 n'est valide que sur les points entiers [V]. |

## 4. L'oracle borné (`reference/`)

### 4.1 Ce qu'il établit, et son indépendance réelle [V]

**Les étages.**
- **Étage A** (`hgp11_ref/definition.py`) : la définition Γ_k, exhaustive. La MEB est le minimum des circonsphères des
  parties de 1 à 4 points, en `Fraction`/Gram, sans Welzl ; c'est correct par M1 et l'unicité.
- **Étage B** (`constructive.py`) : la voie du moteur, avec les **formules du moteur** (`intgeom.py`, c = a + N/D).
  C'est un miroir constructif, pas un juge arithmétique indépendant.
- Le juge exige B = A champ par champ.
- `interval_oracle.py` n'importe que `fractions` ; `supports.py` (S1) n'importe que `definition` et `model`.

**Indépendance vis-à-vis du moteur.**
- Les portes natives de la forêt chargent **uniquement** `model.py` et `definition.py` (`tests/tower/forest_oracle.py`,
  l. 1-30) : le moteur C++ est jugé contre la définition.
- Les seules choses partagées sont les enregistrements de `model.py` et la convention de numérotation, écrite trois fois.
- T1 est **invoqué** (théorème 2), pas re-vérifié. Sa partie combinatoire est jugée par la route de l'auditeur, que
  j'ai rejouée.

### 4.2 Couverture affirmée et couverture rejouée

| Affirmation | Rejoué ici (Python 3.12.1, `-S -B`) |
| --- | --- |
| 1 362 ordres, 48 234 coupes, 13 029 nœuds, 342 nuages | [R] `reference_fast_ok nuages=342 ordres=1362 coupes=48234 noeuds=13029`, en 20,8 s. Identique sous `-O` et en 3 processus. Compteurs détaillés : naissances 8 205, fusions 4 824 (dont 1 580 N-aires), 760 boules étendues, 264 pondérées, 145 sauts de descente. |
| 22 mutants tués | [R] 22 renvoient le code 4, 3 équivalents le code 0 (`first_rep_last`, `hull_strict_triangle`, `jump_any`). Chaque mutant ne joue que 2 à 4 fixtures désignées. |
| S1 : 210 nuages, 951 ordres, 13 mutants | [R] `reference_supports_ok … boules=15062 supports=16943 noeuds=12441 coupes=48074`, 51 faits, 0 écart, en 43 s. Identique sous `-O`. 13 sur 13 mutants tués. |
| Primitives sphere5 | [R] `sites=24 supports=828 q2=12 q3=24 q4=792 N2=12 N3=288 N4=3906 refus=2`. |
| Cinq faits de projection | [R] `projection_contracts_ok faits=5`, aussi sous `-O`. |
| Vidages v10 sur 190 nuages | Non rejoué : les binaires figés `c764e121a` sont absents. Aucun build local ne correspond au pin (le plus proche, `v10-verify-tower`, est antérieur). |
| Suite complète (5 617 nuages) | Non rejouée ici. **Passée sur G4** (§ 2.6). |
| Contre-épreuve de l'auditeur (65 ordres) | [R] `check_math.py`, sur le worktree épinglé `238734f1d`, sort un JSON **identique** au reçu. |

### 4.3 Limites

- **Tailles.** n ≤ 14 pour l'étage A (K ≤ 10). S1 s'arrête à K ≤ 5 et à des coquilles de 12 sites au plus ; un seul
  témoin à K élevé (carré et intérieurs, K1–K12).
- **Profils.** « Aucun profil au-delà de 18 bits n'est exercé » dans la suite rapide (`reference/README.md:97`). Les
  exceptions : le cercle n = 1023 en u21, et deux nuages u24 chez l'auditeur.
- **Descentes.** 145 sauts sur la suite rapide, contre des millions sur une trame LiDAR.
- **Poids.** Les étages A et B acceptent les doublons ; le moteur les refuse. La sémantique des copies est jugée entre
  A et B sur 34 nuages, sans contrat écrit (`MATH` § 9).

## 5. Galerie des contradictions

| Témoin | Énoncé réfuté | Fixture / test | Registre | Rejoué |
| --- | --- | --- | --- | --- |
| **E5** : A=(0,0,7), B=(0,9,6), C=(1,4,0), D=(0,0,1), E=(4,1,2), K=2 | Prop. 6 de la thèse (Gabriel préserve les K-polyèdres) ; théorème 5 par héritage ; « retirer les liaisons hors W_K laisse T_K inchangé » (boule AC à 33/2, fusion à 83886/3563, fusion parasite à 24) | racine `tests/fixtures/regressions/gabriel_point_set_counterexample.json` ; `tests/oracle/test_gabriel_counterexample.py` ; `reference/test_supports.py::e5_window` ; natif `mhgp11_tower_attach_fixtures` | l. 40-41 et 1365 | [R] 4/4 et fait conforme |
| **Sept sites** {0,10,11,26,27,45,46} | une hiérarchie commune conserve les groupes core des différents ordres : {0,10,11} à K1 (25 ; 225/4) contre {10,11,26,27} à K2 (64 ; 361/4), et 0 n'entre qu'à 100 | `reference/test_projection_contracts.py::test_core_descendants_cross_between_orders` | aucune (seulement un analogue à six sites, l. 1317) | [R] |
| **{0,2,4}**, k=2 ({0,2,4+δ}) | stabilité de la projection LCA ; existence d'un singleton équivariant | `…::test_first_cover_lca_is_discontinuous` (s = 1, 1000) | trilemme, l. 1304 | [R] |
| **{0,2,5}**, k=2 | cover et MR₂-bord (v10) donnent les mêmes blocs avant EOM (IoU 1 contre 2/3) | `…::test_cover_keeps_ab`, `test_mr_border_has_no_intermediate_ab`. La formule MR₂ est réécrite en ligne, sans scikit-learn. | aucune | [R] |
| **Cercle** A, B, C, D puis B_t | stabilité du porteur ∪conv Q (v1) : saut d'au moins d/4 | `test_supports.py::circle_witness` (n = 3, 4, 5, 1023) | l. 1374 | [R] ; et § 2.5 pour la v2 |
| **D2** : A=(2,10), B=(18,10), C=(10,20), Z=(9,3), W=(11,3), K=2 | β(F) ≤ ℓ(r_b−1), avec 41 < 64 < 1681/25 ; « composantes ouvertes = fermées au rang r_b−1 » | `test_supports.py::d2_witness` ; `tests/tower/attach_test.cpp:63-70` ; `mhgp11_tower_attach_e1e2` ; mutant « inégalité devenue garde » | l. 1370 (ligne fusionnée avec E) | [R] |
| **X = {0,2,4,6}**, k=2 | graine ou mémo valide dès β_terminal (β_initial 9, β_terminal 1) | `tests/tower/memo.cpp:15`, `memo_support.hpp:24` | aucune | natif, non rejoué |
| **Triangle K1** (0,0,0), (1,1,0), (1,0,1) | « boules de rôle fusion = arbre couvrant » (cycle) ; « première boule seule » (site isolé) | `tests/cli/supports_spanning_reader_gate.py` ; natif `tests/cli/cli_supports_oracle.py:302` | aucune | [R] `conforme cas3` |
| **Sphère** x²+y²+z² = 5 (24 sites) | rien n'est réfuté : témoin de capacité au plafond v1 | `test_supports.py --suite=primitives` ; natif `mhgp11_supports_unit_sphere5` | — | [R] |
| **9 sites** : cube {0,16}³ et (8,8,8), K5, feuilles 8/32 | « feuilles ≤ sites » (159 nœuds, 80 feuilles, 648 entrées) | modèle seulement : `receipts/audit_gpu_euler_20261004/centre_leaf_counterexample/check.py` ; la porte native `mhgp11_tower_full_leaf_lanes` utilise 3 000 sites | aucune | [R] modèle identique au reçu |
| **R = 2^127−1** | une racine en u128 se convertit sans risque (`rt+1`) | natif `tests/head/head_test.cpp` (`huge`) | aucune | non rejouable (§ 3.4) |
| **(2,162,50) et (8,98,32)** | des dates √t+√m−√q apparemment distinctes se départagent en `Decimal` ou en binary64 (les deux valent 5√2) | `bench/points_gate.py:203` ; `receipts/hm_review_20261003/check_radicals.py` | analogue √2/8 (l. 1333) | [V] calcul |
| **Euler à 5 sites**, plus T13/T23 | J3 certifie la complétude | `tests/catalogue/euler_limits.py:31-35` | l. 1350 | [R] reçu ; [V] calcul |
| **Tétraèdre et son centre** | Cat_{K+1} suffit au juge d'Euler | `receipts/audit_gpu_euler_20261004/euler` | partiel (l. 1349) | non rejoué |
| **Trois réfutations du polyèdre** | moins de k aberrants n'agissent pas ; l'identité des chaînes contractées est stable ; les réalisations s'emboîtent entre ordres | racine `tests/fixtures/regressions/polyhedron_order_k_counterexamples.json` ; `tools/check_polyhedron_order_k_counterexamples.py` ; `tests/oracle/test_polyhedron_order_k_counterexamples.py` | l. 154-156 | [R] `PASS (3 cases)`, 11/11 |
| N = 2^53+1 ; x = 32767 ; s = 2^21−1 ; tétraèdre u24 | F3 ; F2 ; bornes intermédiaires ; export | reçus `floating_bounds`, `math_locks_review`, `catalogue_native_side_review_5` ; porte `mhgp11_tower_points_export_width` | aucune | [R] / [V] |
| **Nouveau** : triangle (0,1,1), (1,0,1), (1,1,0) | la sortie v2 est équivariante par translation | aucune fixture | aucune | [R] modèle |

## 6. Pour la v12

### 6.1 À garder tel quel

**Les énoncés.**
- La définition de FULL (§ 4) : coupes fermées et ouvertes à boules **ouvertes**, plateaux N-aires, verticales à la
  coupe fermée.
- T2, le critère de séparabilité qui remplace la position générale.
- Le lemme P comme définition opératoire du plateau.
- Les lemmes A–E : rattachement sans descente supplémentaire. Seuls les ensembles de nœuds coïncident entre coupe
  ouverte λ_b et coupe fermée de rang r_b−1.
- F, G, H, W.1–4, et G1–G4 (complétude conditionnelle).
- Les lemmes de calcul du § 1.9 : table de populations, R, V3 réservé aux sites, diamètre, cliques, zonogone,
  réemploi, MST/contraction.

**L'oracle.**
- L'étage A, S1, l'oracle d'intervalles, les cinq faits de projection et leurs mutants, dont les six mutants de
  vivacité.
- Les familles SplitMix64.

**La doctrine numérique.**
- Budgets `constexpr` par expression, certificats calculés dans les fabriques, trois voies.
- `Level` non réduit et `LevelRank`.
- F1–F6 dans leur forme corrigée (F3 par expression), avec les clés F3/F4 du tri (E = 6, c = 1 − 2^−40).

### 6.2 À réécrire, et pourquoi

1. **Un seul contrat normatif**, aux preuves complètes et non résumées :
   - y importer la preuve de J3 par le nerf et les preuves de L01, aujourd'hui hors dépôt ;
   - énoncer T4 comme le lemme P, et T6 avec ses verticales ;
   - corriger la citation de Kruskal (P.3) ;
   - supprimer le doublon § 10.10 et les parties v1 périmées ;
   - harmoniser le profil d'en-tête.
2. **Des identifiants à préfixe et uniques** (MATH-T2, CAT-G3, NUM-F3, LEM-P, WIT-D2…). Chacun relié à une ligne de
   registre, un endroit du code, une porte et un mutant. Cause : les collisions du § 2.4.
3. **Le registre.**
   - Ajouter les lignes manquantes (§ 2.3) et réparer `REG:1370`.
   - Ne jamais citer une note vivante : citer un reçu ou un commit.
   - Faire contrôler par `check_docs` les `\n` littéraux dans les tableaux.
4. **Un catalogue de témoins lisible par une machine**, à la racine dans `tests/fixtures/regressions` : énoncé réfuté,
   coordonnées, attendus exacts, commande de rejeu en Python nu et porte native. Une ligne de registre obligatoire par
   contradiction. Graver « 9 sites / 80 feuilles », le triangle équilatéral de la translation et le saut de `S*` du
   cercle.
5. **Les départages des sorties.**
   - Ordre total invariant par translation : ordre lexicographique des coordonnées pour `S*` et pour l'ordre de Kruskal.
   - Déclarer la non-équivariance restante (permutations et réflexions d'axes) et la non-stabilité de `S*`.
6. **L'oracle.**
   - L'étendre aux profils produit (coins u21 et u24 dans une famille), à K ≥ 6 et aux coquilles de 13 à 24 sites pour
     S1, et à de longues descentes.
   - **Implanter J2** à l'échelle.
   - Fermer le différentiel v10/v11 sur trames entières, ou retirer la revendication « même objet »
     (`docs/ARCHITECTURE.md` § 6).

### 6.3 Questions mathématiques ouvertes, par priorité

**P0, conditionnent le contrat LiDAR.**
1. **Multiplicités.** Il faut le contrat pondéré, le « modèle par copies » : T2, T3 et G1–G3 avec des poids, naissances
   nulles, fenêtres non contiguës. L'oracle l'implante déjà : A sur les copies, B par le quotient de Gordan, B = A sur
   34 nuages [V]. Il reste à l'écrire et à le prouver. Le moteur refuse aujourd'hui ces entrées.
2. **Coquilles étendues.**
   - Il faut un quotient polynomial prouvé.
   - Plafonds actuels : `max_leaf` 256 (une cosphère de 270 sites est refusée), 24 en v1, m ≤ 255 en v2.
   - C(m,t) traces : une coquille de 150 sites à K8 donne au moins C(69,8) traces (`docs/FULL_FORESTS.md:215`).
3. **Borne de travail et terminaison** de la politique de subdivision. Aujourd'hui : aucune borne sous-quadratique, et
   un refus certifié sur les feuilles larges.

**P1.**

4. **Juges d'échelle de la forêt** : J2 à k = 1, un juge d'échantillon de parties, le différentiel sur trames.
5. **Forêt parallèle.** Inscrire le lemme MST/contraction et le contrat des compteurs (logiques ou physiques), avant
   tout port. Clore ou reformuler la ligne `REG:243`.
6. **Canonicité et identité.** Équivariance des sorties ; identité des nœuds de vie inférieure à 2δ (61 à 67 % des
   nœuds K5, audit final § 4.4).

**P2.**

7. **Partition T > 0** compatible avec u24 : comparer QE à Dr, sous la condition 4B+5+T ≤ 127.
8. **Points.** Constante 3 minimale sur l'image géométrique (`proof_obligation`, `REG:1308`) ; règle stable par
   insertion (domaine D).
9. **J3 au-delà de K = 10.** Il exigerait Cat_{K+2} au-delà de la limite de 12.
10. **Polyèdre A_k(r)** : un représentant robuste.

## Annexe : commandes exécutées

Toutes en lecture seule, avec `PYTHONDONTWRITEBYTECODE=1` et `TMPDIR` dans mon dossier. Sorties dans
`/tmp/claude-1000/-workspaces-E-HGP/6acaaf62-b44a-41e7-b5d6-81e4c7b6b7f1/scratchpad/agent_B/`.

**Oracle `reference/`.**
- `test_projection_contracts.py` (normal et `-O`) : code 0.
- `test_ref.py --suite=fast` (normal, `-O`, `--jobs=3`) : code 0.
- 25 lancements `test_ref.py --inject=…` : 22 en code 4, 3 en code 0.
- `test_supports.py` (normal et `-O`) : code 0.
- `--suite=primitives` : code 0.
- 13 lancements `test_supports.py --inject=…` : code 4.

**Racine.**
- `tools/check_polyhedron_order_k_counterexamples.py` : `PASS (3 cases)`.
- `python3 -B -m unittest tests.oracle.test_polyhedron_order_k_counterexamples tests.oracle.test_gabriel_counterexample` :
  15 sur 15.
- `tools/check_docs.py` : code 1, 213 liens morts, tous dans les reçus de `morsehgp3D_v10`.

**`tests/`.**
- `tests/cli/supports_spanning_reader_gate.py --bench bench` : `conforme cas3`.
- `tests/num/*_oracle.py --selftest` (9 modèles) : tous en code 0.

**Reçus d'auditeur.**
- `floating_bounds/check.py`, `math_locks_review/check.py`, `centre_leaf_counterexample/check.py` et
  `audit_geant_20261005/math/check_math.py` (sur le worktree `238734f1d`) : sorties identiques aux reçus.
- `audit_s10_root_guard_20261005/check.py` : non rejouable (`snapshot/` absent).

**Mes scripts.**
- `spv2_translation.py` : translation de la v2.
- `sstar_circle.py` et `circle_roles.py` : saut de `S*`, rôles lus dans l'oracle S1.

**Non joué.** Aucune porte native (pas de build), aucun différentiel v10. GCP non utilisé.
