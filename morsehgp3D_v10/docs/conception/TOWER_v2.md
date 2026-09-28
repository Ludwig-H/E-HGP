# TOWER_v2 — Tour FULL de morsehgp3D_v10 à partir du catalogue (conception révisée)

28 septembre 2026. Sous-système : **TOUR FULL**, du catalogue de boules critiques jusqu'aux forêts par ordre, aux
contributions datées, aux verticales et aux points d'entrée des consommateurs. Ce document **remplace
`TOWER_v1.md`** après la critique adverse (`crit_TOWER/`). C'est une conception, pas un état du dépôt : aucun fichier
suivi n'a été modifié. GCP non utilisé.

```text
phase=exploration_v10_hors_registre
backend=cpu_reference            (voie GPU : § 14)
profile=quantized_u18_input_only (multiplicités natives, § 3.9)
mode=conception_v10
public_status=not_claimed
```

**Sources.** Celles de la v1 : `lenses.json` (L01, L04, L06, L07, L10, L12, L14), prototypes de session, `cble.cpp`,
code v9 à `ce8a649dd`, reçus R22, documents v7, SPEC § 2–4, registre des preuves. S'y ajoutent :

- la critique `crit_TOWER/` : `t2_bench.cpp`, `cble_dump.cpp`, `rep_stats.cpp`, `knn_descent_sim.py`,
  `eqrad_count.py`, `chain_len*.py`, `euler_check.py`, `judge_n11.*` et leurs sorties ;
- les conceptions sœurs `ARCH_v2.md` (§ 5.3, § 5.6, § 6.8, § 7.3, P13, P17) et `CLUSTER_v2.md` (§ 4.1.5,
  contrat `TowerOrderView`), `EVAL_v2.md` (PO-E15) ;
- les expériences écrites pour cette révision, dans `tower_v2_spike/` (§ 17). Ce sont des mesures et des contrôles de
  conception, rejouables par les commandes du § 17. Ce ne sont pas des reçus de phase.

Convention : « mesuré » renvoie à une sortie du § 17 ou de la critique, avec l'hôte et la charge ; « estimé » désigne
une hypothèse de conception, à mesurer. Tous les temps locaux ont été pris sur le codespace (EPYC 7763, Zen 3, 8 vCPU),
**chargé** (moyenne de charge 6 à 10 pendant les mesures). Ils valent comme ordres de grandeur et comme rapports entre
variantes mesurées dans la même passe, jamais comme chronomètre de contrat.

---

## 0. Réponse à la critique

| Point | Verdict | Preuve ou mesure | Correction |
| --- | --- | --- | --- |
| **B1** Cœur séquentiel sous-estimé ×3–4 | **fondé** pour le Kruskal *par lots* de la v1 | Sur la structure réelle de l'ordre Kmax de 08/000200 (feuilles et arêtes de `t2_bench`), j'ai mesuré mon Kruskal par lots optimisé (union par taille) à 27–47 ms (K5) et 77–127 ms (K10). La critique mesurait 55 et 141–151 ms. La v1 annonçait 7–14 et 20–40 ms : c'était faux | Le Kruskal par lots disparaît du chemin critique (§ 6.5). |
| **B1** (cause) | **identifiée** | Le coût venait surtout du traitement des lots : 47–52 % des lots à arêtes ont plusieurs arêtes, et chacun paie tri, recherche dichotomique et DSU local. Le noyau **arête par arête sans lots** coûte 6,6–9,2 ms (K5) et 15,9–22,3 ms (K10) avec préchargement, sur les mêmes arêtes et dans les mêmes passes. Le parcours final des listes (4–5 et 15–26 ms), compris dans le Kruskal par lots, devient T2b, parallèle : à périmètre égal, le noyau est 3 à 6 fois plus rapide | T2a = union-find et concaténation de listes seulement. Les multifusions sont regroupées **après coup, en parallèle**, par la règle cartésienne (PO-T18). Sortie **identique** au Kruskal par lots sur les deux ordres réels et sur une variante à plateaux grossis (§ 17.1) |
| **B2** TOWER contredit ARCH P13 | **fondé** | ARCH § 6.8 demande T2 ≤ 15 ms à K5 ; la v1 renvoyait le dendrogramme parallèle à plus tard | Construction intra-ordre conçue maintenant (§ 6.5) : noyau minimal séquentiel, puis ordre des feuilles par classement de liste parallèle, puis matérialisation parallèle de la forêt à plateaux. Le noyau de l'ordre Kmax est lancé en tête et recouvert par l'étage G des autres ordres (§ 11.2). Porte d'équivalence avec le Kruskal par lots (§ 13.3) |
| **B2** (remède proposé : dendrogramme parallèle Wang–Yu–Gu–Shun) | **partiellement réfuté par la mesure** | Implémenté (diviser pour régner par rangs, adapté aux plateaux) et **égal** à la référence sur les données réelles. Mais son travail (110–387 ms CPU à W1 ; 47–203 ms de mur à W4) vaut 5 à 10 fois celui du noyau minimal suivi du parcours des listes (≈ 13 ms à K5, ≈ 37 ms à K10) (§ 17.1) | Il est gardé comme option de profondeur d ∈ {0, 1, 2}, avec le noyau minimal comme cas de base. L'option n'est adoptée que sur mesure G4 (D-T6). Elle n'est pas nécessaire au contrat K5 (§ 11.3) |
| **M1** Plancher « rayon égal » inatteignable | **fondé** | Preuve au § 9 (PO-T4) : chaque saut fait décroître strictement λ. La branche « λ égal, σ décroît » était fausse | Plancher retiré. I3 devient « λ strictement décroissant à chaque saut ». La fixture `actual_equal_radius_descent` est requalifiée (§ 13.2) |
| **M2** Validation muette sur les omissions du recensement | **fondé** | Valider les sites **listés** ne dit rien des sites **omis**. I5 interroge les mêmes listes L(Q) que le générateur | Juge indépendant du recensement J-CENSUS (§ 13.4) ; phrase de statut exacte (§ 8.4). Validation géométrique complète dans les portes, échantillonnée en mode latence (§ 8.2) |
| **M3** Régime de descente réel non exercé | **fondé** | Critique : sur LiDAR, 30–37 % des représentants non semés sautent, jusqu'à 6 sauts. Mes mini-nuages de balayage (n = 10–13) ne sautent presque pas pendant la construction (262 résolutions à 1 saut, 3 à 2 sauts, sur 13 992). En revanche, sur **toutes** les K-parties de nuages en **amas** (n = 9–13), que la T2 résout de toute façon, on trouve 1 747 descentes à 3 sauts et 139 à 4 sauts sur 200 nuages. Six nuages à 3 et 4 sauts ont été **jugés conformes** par l'oracle Γ_K, soit 23 387 contrôles. Le mutant « saut centré sur un site du support » survit à 15 des 16 fixtures de la v1 ; les nuages à descentes longues le tuent tous (§ 17.4) | Fixtures gravées à 3 et 4 sauts (§ 13.2) ; famille « amas » dans la T2, avec planchers ≥ 1 000 K-parties à ≥ 3 sauts et ≥ 50 à ≥ 4 (§ 13.1). Juge de descente indépendant J-DESC, stratifié, avec planchers sur trames (≥ 1, ≥ 2, ≥ 3, ≥ 5 sauts) (§ 13.4). Mutants du saut (§ 13.3) |
| **M4** Entrées pondérées et planes sans juge d'échelle | **fondé** | — | Euler pondéré **dérivé** par intégration d'Euler sur les copies (PO-T19) : 340 nuages dégénérés, 4 746 boules, 3 270 contrôles, 0 échec ; mutant tué 40/40. Quotient **circulaire** exact en O(u log u) pour toute coquille coplanaire au centre, donc toutes les boules d'une entrée z = 0, sans plafond u ≤ 16 (PO-T20) : 14 827 contrôles de quotient et 4 813 d'Euler, 0 échec. T2 pondérée jusqu'à 12 sites ; fixtures z = 0 ; recensement de u sur les suites publiques (§ 13) |
| **M5** Leviers T2 mal fondés | **fondé** | Mesures : 52,2 % (K5) et 46,6 % (K10) des lots à arêtes n'ont qu'une arête ; 34,4 % et 40,1 % des fusions ont au moins 3 parents | Préfiltre MSF (Borůvka) et renumérotation Morton **supprimés**. Phrase « 99 % » retirée. D-T6 réécrite (§ 16.2) |
| **M6** Juge indépendant de la topologie K ≥ 2 mince | **fondé** | K1 = EMST n'exerce ni G ni M | Juge des multifusions par certificats J-MF : descentes indépendantes (échange d'intrus réécrit dans l'oracle), connexité des parents par les cellules du lot, séparation par les cellules échantillonnées (§ 13.4) |
| mineurs | fondés, sauf mention | voir § 16.3 | tous intégrés |

**Ce qui ne change pas.** L'objet, la chaîne *résolveur pur par saut K-NN → minima fixes → forêt par rangs → index
d'intervalles → verticales à la coupe fermée*, l'équivalence avec les ancres v7/v9, la formule d'Euler étendue. La
critique les a revérifiés : 12 nuages génériques supplémentaires à n = 10–11 (48 064 contrôles), 1 134 contrôles d'Euler
dégénérés. Elle a aussi mesuré, sur 08/000200, un étage G moins cher que la v9 : semis H_K à 72–78 %, 1,2–1,3 MEB
par représentant non semé.

---

## 1. Résumé exécutif

1. **Objet inchangé.** Pour chaque K ≤ K_eff, la tour contient :
   - la forêt de fusion de π0(L_K(a)), avec naissances et multifusions non binarisées ;
   - les continuations à contributions datées et les plateaux atomiques par niveau exact ;
   - le quotient local de Gordan des coquilles étendues ;
   - les verticales à la coupe fermée ;
   - K1 = sites au niveau 0, et une racine finale par K.
2. **Géométrie pure, puis topologie.** L'étage G résout chaque représentant par une fonction pure :
   semis H_K, MEB exacte, recherche de la boule, et au besoin un **saut K-NN**. Chaque saut fait décroître λ
   **strictement** (PO-T4 corrigé). L'étage M donne `Min(cellule)` par sauts de pointeurs.
3. **Forêt d'un ordre en trois temps** (nouveau, § 6.5) :
   - **T2a**, seul étage séquentiel : union-find et concaténation de listes, arête par arête, dans l'ordre des clés,
     **sans traitement de lot**. Mesuré à 6,6–9,2 ms (K5) et 15,9–22,3 ms (K10) sur la structure réelle de
     08/000200. Ces chiffres ne comptent que les arêtes semées ; il faut compter environ ×1,3 pour la totalité ;
   - **T2b**, parallèle : ordre des feuilles π et tableau des jonctions J, par classement de liste ;
   - **T2c**, parallèle : les nœuds, multifusions comprises, sont lus sur (π, J) par la **règle cartésienne**. Deux
     jonctions égales dans un même intervalle maximal appartiennent au même nœud. PO-T18 le prouve exact pour tout
     ordre de traitement à l'intérieur d'un rang.
4. **Chemin critique.** Le noyau T2a de l'ordre Kmax est lancé dès que ses arêtes existent. G, M et T1 sont ordonnancés
   par ordre décroissant, si bien que le noyau est recouvert par l'étage G des ordres inférieurs (§ 11.2). Sur GPU, où G
   rétrécit, le noyau redevient visible : ≈ 4,5–12 ms estimés à K5 sur G4, sous le budget T2 ≤ 15 ms d'ARCH § 6.8.
5. **Une seule primitive de requête** : WA(nœud, seuil) sur l'index d'intervalles, avec un arbre de maxima par blocs
   de 16 jonctions. Elle sert aux ancres, aux contributions, aux verticales, aux entrées C∩X (`entry_node`,
   `entry_level`, `entry_pos`, `entry_eq`) et au localisateur de facettes.
6. **Multiplicités et entrées planes** :
   - Euler pondéré dérivé et vérifié (O-W2, ARCH P17 : proposé `proved_here`) ;
   - coquilles circulaires traitées exactement en O(u log u), quel que soit u. Toutes les boules d'une entrée z = 0
     sont dans ce cas ;
   - table pondérée des coquilles 3D en O(u·2^u), u ≤ 16.
7. **Vérification.** La v1 re-vérifiait des théorèmes ; la v2 cherche des fautes d'implémentation.
   - Validation géométrique complète et naturalité complète : dans les portes seulement ; échantillonnées en mode
     latence.
   - Trois juges indépendants à l'échelle : recensement, descentes stratifiées par nombre de sauts, multifusions par
     certificats.
   - Plus EMST (K1), Euler, et le différentiel v9.
8. **Coûts estimés, G4, CPU seul, W48** (§ 12), dans les scénarios d'ARCH : tour K5 LiDAR ≈ 33–53 ms en optimiste,
   70–110 ms en pessimiste ; tour K10 ≈ 0,19–0,28 s ou 0,40–0,58 s. L'étage G fait 65 à 75 % du travail. Le noyau
   séquentiel n'est plus le plafond sur CPU. Mémoire : ≈ 75 o persistants par boule, pic ≤ 100 o
   (ARCH_v2 § 5.6).

---

## 2. Ce que la v9 fait inutilement compliqué

| v9 (lieu, mesure) | Pourquoi c'est inutile | v10 |
| --- | --- | --- |
| Résolveur couplé à l'état : un représentant se résout vers une ancre installée **après** la fermeture d'un lot antérieur (`order_block_lean`, `full_ball_tower.hpp:1240-1275`) | la cible géométrique est une fonction pure du catalogue ; seule sa **racine** dépend du temps | `resolve` pur (§ 6.2), `Min` par sauts de pointeurs (§ 6.3) |
| Recherche d'intrus depuis la racine de l'index à chaque pas, un intrus échangé contre le premier sommet du support (`intruder_work`, l.2061-2086) : 7,1 M requêtes et 403 M nœuds visités à K10 | un saut K-NN donne d'un coup un ensemble de rayon strictement inférieur (PO-T4) | saut K-NN sur l'oracle de feuilles (§ 6.2) ; l'échange d'intrus devient le **juge** indépendant J-DESC (§ 13.4) |
| Tableau d'ancres dense de la taille du catalogue pour chaque K (l.1355) : ≈ 220 Mo à K10 | un bloc n'existe qu'aux ordres de sa fenêtre | atlas `cell(b, K) = base[b] + K − lo(b)` |
| Six chemins « même objet », 254 lignes d'orchestration de « même première panne » | un seul algorithme suffit | un chemin ; variantes neutres dans les tests seulement (§ 13.3) |
| Lots par DSU local alloué, tri des propriétaires, vecteurs par groupe (`order_lot`, l.1277-1352). Mesuré ici : 47–53 % des lots à arêtes en ont plusieurs, 34–40 % des fusions ont ≥ 3 parents | le regroupement par lot n'a pas besoin d'être séquentiel : les jonctions égales d'un même intervalle suffisent (PO-T18) | noyau arête par arête, regroupement parallèle a posteriori (§ 6.5) |
| Ré-encodage final (`build_full_coverage_certificate`) : 39 ms (K5) et 93 ms (K10) sur le chemin critique | la sortie se lit sur (π, J) | CSR écrit par la matérialisation parallèle ; le validateur structurel devient un juge de porte |
| `ExactLevel` de 48 o recopié par nœud et par contribution ; digest sur la représentation brute | deux écritures d'une même fraction changent le digest sans changer l'objet | rangs u32, digests canoniques réduits (§ 6.7) |
| Banque de populations (≈ 9 M allocations à K10) | une population est celle d'une boule du catalogue | populations implicites |
| Images de naissance par DSU monotone rejoué | image d'une naissance = ancre de la même boule à K−1 | verticales = requêtes WA parallèles (§ 6.6) |
| 409 à 889 fils créés par construction (L04-F7) | la géométrie est plate | un `sched::Pool` (ARCH § 7) |
| Sceau à validation échantillonnée 1/64 avec résidu mémoire-dangereux | la validation *structurelle* O(1) protège la mémoire ; la validation *géométrique* re-vérifie un théorème du générateur | structure : toujours, toutes les boules ; géométrie : portes complètes, échantillon en latence (§ 8.2) |
| Coquille > 12 : refus de toute la chaîne ; doublons refusés | ni l'un ni l'autre n'est une borne mathématique | coquilles circulaires exactes pour tout u ; 3D u ≤ 16 par table, refus typé par boule au-delà ; multiplicités natives |
| Ancres jetées ; le consommateur les ré-inférait (régression E5 du 28 septembre) | l'interface exacte avec le clustering, ce sont les ancres | ancres publiées, entrées C∩X, localisateur de facettes |
| Mutants en `#if` dans l'en-tête produit (24 branches) | un mutant appartient au harnais | copies mutées au configure (ARCH § 9.5) |

La v10 garde de la v9 : la sémantique bloc par boule, le quotient local `ShellTable`, la MEB proposée en double puis
vérifiée exactement, le semis par populations de naissance, les rangs u32 sur un tri exact certifié, la T2, Euler et
l'échec transactionnel.

---

## 3. Objet normatif

### 3.1 Entrée

- `SiteTable` (GEN § 7) : positions distinctes u18 en ordre de Morton (`SiteIdx`), poids `w_s ≥ 1`, CSR site →
  `PointId`. On note `W = Σ w_s`, `K_eff = min(Kmax, W)` et `Kmax ≤ 10` pour la tour publique.
- `Catalogue` (GEN § 7). Il contient les boules critiques bien centrées, en ordre canonique (rang, S*), avec :
  - I puis U, triés par `SiteIdx` ;
  - `n_interior`, `n_shell` et `qmin`, comptés en **positions** ;
  - les drapeaux « coquille étendue », « coquille pondérée » et « coquille circulaire » (nouveau, § 5.3) ;
  - `level_rep` ;
  - une construction jusqu'à `kcat` (= Kmax, ou Kmax + 2 en mode certifié).
- Oracle géométrique (GEN § 8, lemme O) : `lookup(sphère)` et `knn_closed(c, k)`.

### 3.2 Objet

L_K(a) = { y : Σ_{s : |y − s|² ≤ a} w_s ≥ K }. Les niveaux sont des rayons **carrés** rationnels exacts. La sortie
est le foncteur π0 sur `{1..K_eff}^op × ℝ`, avec les inclusions horizontales (a ≤ b) et verticales
(L_{K+1} ⊆ L_K).

Modèle discret (Th. 2 du manuscrit, `theorem_external`, étendu aux multiensembles par PO-T16) :
- Γ_K(a) a pour sommets les K-sous-multiensembles F avec β(F) ≤ a, où β est la MEB du support ;
- ses arêtes sont les (K+1)-sous-multiensembles présents ;
- la coupe ouverte remplace `≤` par `<`.

### 3.3 Boules, fenêtres, cellules

Pour une boule b : centre c, niveau λ_b, intérieur strict I, coquille U, `p_w = w(I)`, `u_w = w(U)`. `q` est le
nombre de **positions** du plus petit support positif.

- Fenêtre : `lo(b) = max(1, p_w + q − 1)` et `hi(b) = min(Kmax, W, p_w + u_w)`. La cellule `(b, K)` existe pour
  `lo ≤ K ≤ hi`.
- Boules de rayon nul, une par site : `I = ∅`, `U = {s}`, `q = 1`, λ = 0, fenêtre `[1, min(Kmax, w_s)]`.
- Admission (contrat du générateur) : `p_w + q ≤ kcat + 1`. Une boule livrée hors admission est comptée et ignorée.

### 3.4 Quotient local (Gordan)

À l'ordre K, on pose `t = K − p_w ≥ 1`.

- Un t-multiensemble A de U est **strict** si c ∉ conv(supp A), où conv est l'enveloppe fermée.
- Sommets : les t-multiensembles stricts. Arêtes : chaque (t+1)-multiensemble strict relie ses faces.

Deux cas :
- aucune composante : la cellule est une **naissance**, de population I ∪ U ;
- sinon : un représentant par composante (`I ∪ A`). La contribution vaut
  `D = U ∖ ⋃ supp(sommets stricts)`, en positions.

Coquille régulière non pondérée (U = support, u = q) : naissance à K = p + u. À K = p + u − 1, il y a u composantes,
de représentants `I ∪ U ∖ {s}`, et la contribution est vide.

### 3.5 Lots et actions (sémantique v7/v9)

- Les cellules d'un même ordre et de même niveau exact forment un **lot**.
- Chaque représentant est rattaché à sa composante de Γ_K dans l'état **pré-lot** (coupe ouverte). Les cellules qui
  partagent une racine pré-lot sont groupées.
- Par groupe :
  - 0 racine : naissance ;
  - 1 racine : continuation ou bloc inerte ;
  - ≥ 2 racines : **une** multifusion, dont les parents sont exactement ces racines.
- L'ancre d'une cellule est le nœud qui porte sa composante après fermeture du lot.

### 3.6 Contributions datées

- Chaque cellule de contribution non vide dépose `(segment = son ancre, niveau λ_b, boule, masque de coquille)`.
- Seules les naissances portent I. Une contribution explicite n'a pas de drapeau I : c'était un champ mort en v1.
- La couverture d'un nœud à une coupe est l'union des contributions admises de son sous-arbre.
- Une continuation ne crée pas de nœud et ne réécrit pas la date de la naissance (ABCZ).

### 3.7 Verticales

Chaque nœud d'ordre K ≥ 2 porte son image dans la forêt K−1, prise à la coupe **fermée** de son niveau :
- naissance de b : ancre de (b, K−1) ;
- multifusion : composante, à λ_v, de l'image de n'importe quel parent (naturalité).

### 3.8 Terminaux et bornes

- K1 : feuilles = boules de rayon nul au rang 0.
- K = W : une seule naissance.
- Une racine finale par K.
- Échec transactionnel.

### 3.9 Multiplicités

- Multiensembles partout.
- La fenêtre basse `p_w + q − 1`, avec q compté en positions, est exacte (TOWER_v1 § 2.9, prototype pondéré).
- K1 n'est pas un cas particulier : ses feuilles sont les cellules K = 1 des boules de rayon nul.
- Euler pondéré : § 9, PO-T19.

---

## 4. Architecture : étages

```text
Catalogue + SiteTable + oracle de feuilles (GEN)
   │
 P   préparation     validation structurelle de toutes les boules (O(1) chacune) ; validation géométrique
   │                  (portes : toutes ; latence : 1/64) ; fenêtres, atlas ; quotients locaux (circulaire,
   │                  table 3D, classes pondérées) ; table des naissances H_K ; Euler (pondéré compris)   [parallèle]
 G   géométrie        représentants → resolve(K, F) → cellule terminale ; pointeur par bloc ; entrées C∩X
   │                  [parallèle, pur ; tâches ordonnées par K décroissant]
 M   minima           Min(cellule) par sauts de pointeurs, par ordre                                     [parallèle]
 T1  arêtes           hyperarêtes datées en étoile entre feuilles, émises triées par clé, par ordre      [parallèle]
 T2a noyau            union-find + concaténation de listes, arête par arête, SANS lots
   │                  [séquentiel par ordre ; ordres en parallèle ; l'ordre Kmax démarre en premier]
 T2b ordre            ordre des feuilles π et jonctions J par classement de liste                         [parallèle]
 T2c forêt            règle cartésienne sur (π, J) : nœuds, multifusions, successeurs, intervalles, parents,
   │                  numérotation canonique                                                             [parallèle]
 Q   requêtes         arbre de maxima par blocs ; WA : ancres, contributions, verticales, naturalité
   │                  (échantillonnée en latence), entrées C∩X                                            [parallèle]
 Tower (forêts, contributions, verticales, ancres, index d'intervalles, entrées) + digests hors chronomètre
```

Aucune facette de Γ_K ni cellule de Delaunay d'ordre supérieur n'est matérialisée. Les représentants sont des
multiensembles transitoires de l'étage G ; les arêtes relient des naissances et sont O(boules) ; le noyau T2a ne voit
que des indices de feuilles.

Il y a trois structures distinctes, comme dans la v7 :

- le graphe daté sur les naissances (arêtes de T1), qui certifie les coupes ;
- (π, J), plan de calcul et index de requête ;
- la forêt FULL, qui est la sortie.

---

## 5. Structures de données

Types d'ARCH § 4.1 : `SiteIdx`, `BallIdx`, `LevelRank`, `NodeIdx` (`enum class : u32`), `Buffer<T>` compté par
`MemoryBudget`, `Csr<T>`. Tout index interne est u32. Un dépassement rend `resource_exhausted/index_overflow_u32`.

```cpp
namespace mhgp10::tower {
enum class CellIdx : u32 {};   // cellule (boule, K) de l'atlas ; boules de rayon nul avant le catalogue
enum class LeafIdx : u32 {};   // naissance d'ordre K, dense par K, dans l'ordre des cellules (rang, S*)
enum class EdgeIdx : u32 {};   // arête d'ordre K, dense par K, dans l'ordre des clés
}
```

### 5.1 Vue du catalogue et poids

- Les tableaux de GEN § 7 sont lus sans copie.
- La tour ajoute `pw` et `uw` (u8), **saturés à kcat + 1**, seulement pour les boules à intérieur ou coquille
  pondérés. La saturation est sans perte :
  - la fenêtre ne dépend de `p_w` et de `u_w` qu'à travers des comparaisons avec K ≤ kcat ;
  - Euler ne lit que les coefficients de degré < kcat.

  Les poids par site restent en u32 dans `SiteTable`, et la somme `W` est en u64. Cela corrige le débordement u16
  signalé par la critique.
- Boules de rayon nul : une par site, au rang 0, tables implicites. Elles sont placées avant les boules du catalogue,
  dans l'ordre de `SiteIdx`.

### 5.2 Atlas (inchangé)

```cpp
struct Atlas {                     // indexé par BallIdx (réelles puis virtuelles)
  Buffer<u32> base;                // préfixe exclusif des largeurs ; cell(b,K) = base[b] + K - lo(b) ..... 4 o/boule
  Buffer<u8>  lo, width;           // ........................................................................ 2 o/boule
  Buffer<u32> kstart;              // par K : début de la liste des cellules de K
  Buffer<CellIdx> by_k;            // cellules groupées par K, en ordre de cellule (= ordre de lot) ...... 4 o/cellule
};
```

### 5.3 Quotients locaux

Chaque boule non régulière reçoit exactement **une** des trois tables. Les boules régulières non pondérées
(U = support, u = q, poids 1) suivent le chemin analytique du § 3.4.

**(a) Coquille circulaire** : la coquille est coplanaire au centre, soit rang{s − c : s ∈ U} ≤ 2. C'est le cas de
**toutes** les boules d'une entrée z = 0.

```cpp
struct CircleShell {               // O(u) octets ; aucun plafond sur u
  Csr<u16> order;                  // positions de U en ordre angulaire exact autour de c
  Csr<u32> win_w;                  // W_j = w(A_j), A_j = demi-cercle semi-ouvert [θ_j, θ_j + π) (fenêtre de j)
  Csr<u32> link_w;                 // w(A_j ∩ A_suivant(j)), cyclique
};
```

- Le quotient se lit sur les fenêtres (PO-T20) :
  - une fenêtre est vivante à l'ordre K si W_j ≥ t ;
  - deux fenêtres consécutives vivantes sont reliées si `link_w ≥ t` ;
  - les composantes sont les arcs de fenêtres reliées, lus sur le cycle ;
  - la couverture est l'union des fenêtres vivantes ; il y a naissance si aucune fenêtre n'est vivante.
- Représentant canonique d'une composante : les t premières copies, en ordre angulaire, à partir de la fenêtre de
  plus petit `SiteIdx` de la composante.
- Coût : O(u log u) par boule, puis O(u) par ordre.

**(b) Table 3D** (coquille étendue ou pondérée, non circulaire, u ≤ 16) :

```cpp
struct ShellClassTable {           // une entrée par boule 3D étendue ou pondérée (rare : 135-572 par trame K5)
  Buffer<BallIdx> ball;            // trié
  Csr<u16> supports;               // supports positifs minimaux (masques sur U)
  Csr<CellLocal> per_cell;         // par K de la fenêtre : composantes, représentant (comptes par position),
                                   // masque de contribution, coefficient d'Euler e_K
};
```

- Non pondérée : `ShellTable` v9, en O(u·2^u), port de `local_plateau.hpp`.
- Pondérée : **classes de supports** (PO-T21). Une classe est un ensemble strict S de positions avec
  |S| ≤ t ≤ w(S) ; elle est connexe en interne.
- Règle d'union : pour chaque S strict avec |S| ≤ t+1 ≤ w(S), on unit les classes faisables parmi
  `{S si |S| ≤ t} ∪ {S ∖ {y} : y ∈ S}`.
- Coût : O(u·2^u) par ordre. C'est ce qui remplace l'énumération « (T, T′) par masques » en O(4^u) de la v1.
- u > 16 : refus typé `unsupported_degeneracy/shell_quotient_budget`, **qui publie la boule fautive**. Ce cas ne
  concerne plus que les coquilles 3D non circulaires, par exemple des points de réseau sur une sphère.

### 5.4 Table des niveaux

- `rank[b]` et `level_rep[rank]` viennent de GEN § 5.4. Le rang 0 est le niveau nul.
- La tour ne recalcule un niveau exact que pour trois usages : I3, les comparaisons `D_K(x)` contre niveau
  (`entry_pos`), et la sortie canonique.

### 5.5 Index des naissances H_K (inchangé)

- Contenu : pour chaque K, les naissances dont la population entière (avec copies) a exactement K éléments.
- Clé : le K-multiensemble trié.
- Disposition : seaux par plus petit site, dichotomie sur le deuxième site, comparaison complète contre le CSR du
  catalogue.
- Coût : 100–200 ns par consultation (correction de la critique : deux à trois défauts de cache).
- Levier mesurable, neutre, décrit au § 16.2 D-T2 : la jointure triée sur empreinte additive.

### 5.6 Interface géométrique (inchangée)

```cpp
struct TowerGeometry {
  std::optional<BallIdx> lookup(const ExactSphere&) const;              // boule du catalogue de cette sphère
  void knn_closed(const ExactCenter&, u32 k, KnnOut& out) const;        // N_k(c) exact, poids, ex æquo compris
};
```

### 5.7 Sortie par ordre

```cpp
struct OrderForest {
  u8 K;
  // nœuds en ordre canonique : (rang, plus petite feuille du sous-arbre) — § 6.7
  Buffer<LevelRank> rank;          // ...................................................... 4 o/nœud
  Csr<NodeIdx> parents;            // off 4 o/nœud + 4 o/parent ; parents croissants
  Buffer<NodeIdx> successor;       // kNone à la racine ....................................... 4 o/nœud
  Buffer<NodeIdx> image;           // nœud d'ordre K-1 à la coupe fermée ; kNone à K1 ........ 4 o/nœud
  Buffer<u32> leaf_lo, leaf_n;     // intervalle du sous-arbre dans π ......................... 8 o/nœud
  // index d'intervalles (déterministe ; non canonique entre algorithmes de T2 ; hors digest linéaire)
  Buffer<LeafIdx> pi;              // position → feuille ...................................... 4 o/feuille
  Buffer<NodeIdx> leaf_node;       // feuille → nœud .......................................... 4 o/feuille
  Buffer<CellIdx> leaf_cell;       // feuille → cellule de naissance (population) ............. 4 o/feuille
  Buffer<LevelRank> jrank;         // J[i] : jonction entre positions i et i+1 (L-1) .......... 4 o/feuille
  Buffer<NodeIdx> jnode;           // nœud créateur de la jonction ............................ 4 o/feuille
  Buffer<LevelRank> jblock;        // maxima par blocs de 16 jonctions + arbre implicite ...... ≤ 0,6 o/feuille
  // marques
  Buffer<NodeIdx> anchor;          // par cellule d'ordre K (via Atlas::by_k) ................. 4 o/cellule
  Buffer<Contribution> contrib;    // {segment, rang, cellule, masque de coquille} 16 o, rares
  // entrées C∩X, par site (contrat CLUSTER_v2 § 4.1.5)
  Buffer<NodeIdx> entry_node;      // composante de L_K(D_K(x)) qui contient x ................ 4 o/site
  Buffer<u64> entry_level;         // D_K(x) entier exact (carré de distance, x et poids compris) 8 o/site
  Buffer<LevelRank> entry_pos;     // plus grand rang de niveau ≤ D_K(x) ...................... 4 o/site
  Buffer<u8> entry_eq;             // 1 si D_K(x) = niveau(entry_pos) ......................... 1 o/site
};
struct Tower {
  std::shared_ptr<const Catalogue> cat; std::shared_ptr<const SiteTable> sites;
  Atlas atlas; LocalTables local; std::array<OrderForest, 11> orders; u8 keff;
  TowerStatus status;              // § 8.3
};
```

- `jtree` (arbre complet, jusqu'à 16 o par feuille, correction de la critique) est remplacé par `jblock` :
  - un maximum par bloc de 16 jonctions, soit une ligne de cache de J ;
  - un arbre implicite sur les blocs, de 2·⌈L/16⌉ entrées.

  Une descente parcourt au plus 16 entrées contiguës de chaque côté, puis l'arbre des blocs. L'index coûte alors
  ≈ 20,6 o par feuille, soit ≈ 16,5 o par boule à K10.
- `entry_pos` et `entry_eq` sont demandés par CLUSTER_v2 (§ 4.1.5) ; ils sont calculés dans Q (§ 6.6).
- Accès dérivés sans stockage :
  - population d'une naissance : I ∪ U de la boule de `leaf_cell` ;
  - « w descend de v » ⟺ l'intervalle de w est inclus dans celui de v.

---

## 6. Algorithme

### 6.1 Étage P : préparation (parallèle par boule)

1. **Validation structurelle, toujours, de toutes les boules** (≈ 5–10 ns par boule, estimé). Elle contrôle :
   - les bornes des `SiteIdx` ;
   - I et U triés et disjoints, S* ⊆ U, tailles ≤ plafonds ;
   - la cohérence des drapeaux et les poids saturés.

   C'est elle qui protège la mémoire.
2. **Validation géométrique** : recalcul exact de la sphère depuis S*, barycentriques strictement positives,
   puissance < 0 sur I et = 0 sur U, q recalculé pour les coquilles non régulières.
   - Mode `checks=full` (portes, références) : toutes les boules.
   - Mode `checks=sampled` (latence, défaut produit) : 1/64, tirées par `mix64(BallIdx ⊕ sel) mod 64 = 0`.
     Le sel est publié dans le statut.

   Ce contrôle re-vérifie un théorème du générateur sur les sites **listés**. Il ne dit rien des sites omis
   (§ 8.4, J-CENSUS).
3. **Fenêtres et atlas** : `lo`, `width`, préfixe `base`, `by_k` par comptage puis remplissage.
4. **Quotients locaux** : une tâche par boule non régulière. On choisit d'abord la table circulaire (entrée z = 0 :
   drapeau global, aucun test ; sinon rang exact des vecteurs centrés), puis la table 3D (u ≤ 16), sinon on refuse
   par boule.
5. **H_K** : collecte des naissances de population entière de cardinal K, puis tri.
6. **Euler** (PO-T17, PO-T19) : pour 1 ≤ K ≤ min(W, kcat − 2),
   `#{s : w_s ≥ K} + Σ_{b, λ_b > 0} e_K(b) = 1`, par une réduction parallèle en i64. Les coefficients sont :
   - boule régulière non pondérée : `e_K(b) = (−1)^{u−1−j} C(u−1, j)`, avec `j = K − 1 − p` ;
   - boule quelconque, pondérée comprise : coefficient de `y^{K−1}` dans
     `y^{p_w} · Σ_{T ⊆ supp U, c ∈ conv T} (y−1)^{|T|−1} · Π_{s∈T} [w_s]_y`, où `[w]_y = 1 + y + … + y^{w−1}` et
     conv est fermée ;
   - coquille circulaire (forme close, PO-T20) : `y^{p_w} · ([u_w]_y − Σ_j [w_j]_y · y^{W_j − w_j})`.

   Les polynômes sont tronqués au degré kcat : en i64 pour les formes closes, en i128 avec contrôle de débordement pour
   les tables 3D (§ 7). Contrairement à la v1, **la porte Euler est active sur les entrées pondérées**.

### 6.2 Étage G : résolution des représentants (parallèle, pur)

**Représentants.** Pour chaque cellule non naissance `(b, K)` :
- régulière : `f_s = I ∪ (U ∖ {s})` pour `s ∈ U` ;
- circulaire, 3D ou pondérée : `I ∪ A_j`, un par composante, dans l'ordre de la table.

Chaque représentant est un K-multiensemble trié de `SiteIdx`, dans un tableau fixe (K ≤ 12). À K1 le représentant est
un site, et la règle « une seule position » le résout directement.

**Résolveur.**

```text
resolve(K, F):                                  // F : K-multiensemble trié ; renvoie une cellule d'ordre K
  prev ← +∞ ; sauts ← 0
  répéter :
    si H_K contient F : renvoyer cette naissance
    si supp F = {s} : renvoyer cell(boule de rayon nul de s, K)              // K ≤ w_s
    m ← meb_exact(supp F)                                                    // support S, centre c, niveau λ_m
    exiger λ_m < prev, sinon invariant_violated/descent_not_decreasing       // I3, PO-T4 : décroissance STRICTE
    prev ← λ_m
    b ← geometry.lookup(m)
    si b existe :
        exiger K ≤ hi(b), sinon invariant_violated/window_above              // I3b, PO-T3
        si lo(b) ≤ K : renvoyer cell(b, K)                                   // terminal
        N ← K plus proches de c dans I_b ∪ U_b                               // K < lo(b) (PO-T4, cas 1)
    sinon :
        N ← knn_closed(c, kcat) ; p' ← poids strictement intérieur (plafonné à kcat)
        si p' ≤ kcat − 1 : q' ← q_min de la coquille fermée (table locale si u' > |S|)
                           si p' + q' ≤ kcat + 1 : refuser catalogue_incomplete/missing_key(m)     // I5
        N ← K premiers de N                                                  // (PO-T4, cas 2)
    F ← N ; sauts ← sauts + 1
```

- Ordre des plus proches : puissance exacte, puis `SiteIdx` ; les copies d'un site sont consécutives.
- `meb_exact` : port du noyau v9 (proposition Welzl en double, vérification exacte, retrait des barycentriques
  nulles, repli par énumération des supports de 2 à 4 points). Il a été qualifié sur 198 000 cas v7, sans aucun repli
  sur 11,3 M appels à K10 (R22).
- **Pointeur** : `ptr(b, K) = resolve(K, premier représentant)`. On exige `rank(ptr) < rank(b)` (I4).
- **Compteurs** publiés : `hk_hits`, `meb_calls`, `lookups`, `knn_closed_calls`, et l'histogramme `jumps[0..7+]` par
  ordre. Critique, sur 08/000200 :
  - K5 : 1,17 MEB et 0,38 saut par représentant non semé, jusqu'à 4 sauts ;
  - K10 : 1,29 MEB et 0,54 saut, jusqu'à 6 sauts ;
  - dans les deux cas, 100 % des sauts passent par `knn_closed`.
- **Ordonnancement** : les tâches de G sont émises par K décroissant, l'ordre Kmax en tête (§ 11.1).

### 6.3 Étage M : minima fixes (par ordre)

- `Min(c) = c` pour une naissance ; `Min(c) = Min(ptr(c))` sinon. Les pointeurs d'une cellule d'ordre K restent dans
  l'ordre K : le calcul se fait donc **par ordre**, dès la fin de G(K). La critique avait relevé l'attente globale de
  la v1.
- Sauts de pointeurs en double tampon, jusqu'au point fixe, en au plus ⌈log2(longueur de chaîne)⌉ + 1 tours.
- `leaf(c)` : rang de la naissance `Min(c)` parmi les naissances de l'ordre, en ordre de cellule.

### 6.4 Étage T1 : hyperarêtes datées (par ordre)

- Pour chaque cellule non naissance d'ordre K, on forme `T = tri_unique({ leaf(Min(target_j)) })`. Si `|T| ≥ 2`, on
  émet les arêtes `(T[0], T[i])` pour `i ≥ 1`, avec la clé `(rank(b), indice de cellule, i)`. Elles sortent triées
  par comptage, préfixe et remplissage.
- **Plus de préfiltre MSF.** Le noyau T2a écarte une arête interne pour deux `find` (≈ 5–10 ns). Un Borůvka coûterait
  davantage que ce qu'il économise (critique M5, confirmé).

### 6.5 Étage T2 : la forêt d'un ordre, sans Kruskal par lots

**Propriété (P).** Soit G_t le graphe des feuilles muni des arêtes de rang ≤ t. Un couple (π, J), avec π une
permutation des feuilles et J[i] ∈ rangs ∪ {∞} la jonction entre les positions i et i+1, **satisfait (P)** si, pour
tout t, les composantes connexes de G_t sont exactement les intervalles maximaux de π dont toutes les jonctions
internes sont ≤ t. PO-T18 : tout (π, J) qui satisfait (P) détermine la forêt FULL de l'ordre par la règle cartésienne
(T2c). T2a et T2b construisent un tel couple ; l'option D&C en construit un autre (PO-T22).

**T2a : noyau séquentiel minimal** (un fil par ordre, sans allocation, sans lot).

```text
état : V[ℓ] = {parent, size, head, tail} (16 o), NJ[ℓ] = {next, jrank} (8 o), ℓ ∈ 0..L−1
initialisation : parent = ℓ, size = 1, head = tail = ℓ, next = ∞
pour i dans l'ordre des clés (préchargement de V[E[i+16].a] et V[E[i+16].b]) :
    ra ← find(E[i].a) ; rb ← find(E[i].b)            // demi-compression
    si ra = rb : continuer                             // arête interne (cycle ou même lot)
    NJ[tail(ra)] ← (head(rb), E[i].r)                  // liste(ra) puis liste(rb) ; jonction sur la queue de ra
    union par taille ; la racine retenue reçoit head(ra), tail(rb), size(ra) + size(rb)
```

- **Aucun traitement de lot.** Les arêtes d'un même rang sont unies une à une, et l'ordre des unions à l'intérieur
  d'un rang est l'ordre des clés. Le noyau ne publie **aucun nœud** : il ne peut donc pas binariser une multifusion.
  C'est ce qui le distingue du motif `false_in_general` du registre (« traitement séquentiel de niveaux égaux »).
  Les multifusions naissent en T2c.
- Mesuré sur l'ordre Kmax réel de 08/000200 (§ 17.1), dans les mêmes passes que le Kruskal par lots :

  | | K5 : 361 211 feuilles, 566 911 arêtes | K10 : 937 257 feuilles, 1 380 911 arêtes |
  | --- | ---: | ---: |
  | Kruskal par lots, union par taille (référence) | 27,4–47 ms | 77–127 ms |
  | Kruskal par lots de la critique | 55–58 ms | 141–151 ms |
  | **noyau T2a, préchargement 16** | **6,6–9,2 ms** | **15,9–22,3 ms** |
  | noyau T2a sans préchargement | 7,4–10,8 ms | 27,8–35,8 ms |

  Le temps du Kruskal par lots comprend son parcours final des listes (3,9–5,4 ms à K5, 14,5–25,7 ms à K10) ; celui du
  noyau ne le comprend pas, puisque ce parcours devient T2b, parallèle. À périmètre égal, le noyau est 3 à 6 fois plus
  rapide. Les arêtes des représentants non semés manquent (≈ 30 % selon la critique). On compte donc ×1,3 : ≈ 9–12 ms et
  ≈ 21–29 ms en local.

**T2b : π et J par classement de liste parallèle** (Helman–JáJá, déterministe).

- Les règles sont les têtes des listes des racines, plus les feuilles ℓ ≡ 0 mod 64.
- Chaque règle parcourt sa sous-liste jusqu'à la règle suivante, en parallèle, et écrit (règle, rang local).
- On fait un préfixe séquentiel sur la liste réduite (L/64 éléments), en prenant les racines par indice croissant.
- Le placement parallèle écrit ensuite `π[pos] = ℓ` et `J[pos] = jrank(ℓ)`, ou ∞ en fin de liste.
- En fin d'ordre il y a une seule liste, sinon on refuse `catalogue_incomplete/root_count` (I6).

Mesures locales :
- parcours séquentiel de référence : 3,9–5,4 ms (K5) et 14,5–25,7 ms (K10) ;
- classement de mon prototype : 9,8–35,7 ms à W1. Il ne passe pas à l'échelle sur l'hôte chargé, et ses deux passes
  de numérotation y sont encore séquentielles.

La variante mesurée « union-find à potentiels » (§ 17.1) supprime le classement :
- chaque sommet garde son décalage relatif au parent, et la concaténation décale la liste de rb de size(ra) ;
- la position finale est une somme sur le chemin, calculée par une passe parallèle en lecture seule ;
- elle produit **le même** (π, J) : noyau 10,2–11,4 ms (K5) et 24,4–26,4 ms (K10) ; placement parallèle de 5,2–5,3 ms à W1 et 1,8 ms à W4 (K5 ; une
  passe à charge 16 : 19 ms), et de 14,2–14,3 ms à W1 et 7,0–10,6 ms à W4 (K10).

Décision D-T5 : classement de liste en produit, parce que le noyau à listes est 20 % plus rapide et que c'est lui le
chemin séquentiel. La variante à potentiels est la solution de repli, si le classement de l'ordre Kmax à K10 ne tient pas 3 ms à W48 sur G4.

**T2c : matérialisation cartésienne (parallèle).**

1. `jblock` : maxima par blocs de 16, puis arbre implicite sur les blocs.
2. Pour chaque jonction i (J[i] fini), deux descentes :
   - `l'(i) = 1 + max{ j < i : J[j] > J[i] }` (ou 0) ;
   - `first(i) = min{ j ≥ l'(i) : J[j] ≥ J[i] }`.

   La jonction i **ouvre** un nœud si `first(i) = i`. Toutes les jonctions de même valeur dans un même intervalle
   maximal appartiennent au nœud de leur `first` : c'est le regroupement des plateaux.
3. Nœuds de fusion = jonctions ouvrantes, numérotées par préfixe. `node(i) = node(first(i))`.
4. Intervalle d'une fusion ouverte en f, de rang r : `leaf_lo = l'(f)`,
   `h' = min{ j ≥ f : J[j] > r }` (ou L − 1), et `leaf_n = h' − l'(f) + 1`.
5. Successeur :
   - fusion : nœud de la plus petite des deux jonctions bornantes, J[l' − 1] et J[h'] (si elles sont égales, c'est
     le même nœud) ;
   - feuille en position p : même règle, avec J[p − 1] et J[p].
6. **Numérotation canonique** (§ 6.7) :
   - clé `(rang, plus petite feuille du sous-arbre)` ;
   - plus petite feuille par minimum d'intervalle sur π (minima par blocs de 16 et table creuse sur les blocs) ;
   - tri par base parallèle des clés de fusion, fusionné avec les feuilles (clé `(ρ(ℓ), ℓ)`) ;
   - renumérotation de `successor`, `jnode` et `leaf_node`.
7. `parents` : comptage par successeur, préfixe, remplissage, puis tri de chaque liste par identifiant.
8. I6 et I7 tiennent par construction. On les contrôle en O(L).

Mesures de T2c (étapes 1 à 5 et 7, sans la numérotation canonique) :
- K5 : 35,6–47,8 ms CPU à W1, 12,8–17,2 ms à W4 ;
- K10 : 95,9–131 ms CPU à W1, 35,2–38,3 ms à W4.

Sortie **égale** au Kruskal par lots dans toutes les passes (§ 17.1). Le levier mesurable est le calcul de
`first(i)` par ANSV (plus proche valeur ≥ à gauche) par blocs de 4 096, avec pile, les non-résolus passant par
`jblock`. Il remplacerait les deux descentes, gain attendu ×2–3 (estimé), avec une porte de neutralité.

**Option D&C (profondeur d ∈ {0, 1, 2}, constante pour tout W ; d = 0 en V10-2).**

- On coupe les arêtes à une frontière de rang proche de l'arête médiane ; un plateau n'est jamais coupé. Si un seul
  rang porte plus de la moitié des arêtes, le cas « plateau » se résout par composantes connexes parallèles.
- Les composantes légères sont calculées par un union-find sans verrou, qui accroche à l'indice le plus petit : la
  partition et les étiquettes sont déterministes.
- Le sous-problème léger (sommets touchés) et le sous-problème lourd (composantes contractées, arêtes internes
  écartées) sont résolus en parallèle, avec T2a et T2b comme cas de base.
- Le recollement range les blocs légers dans l'ordre π du lourd et insère les jonctions lourdes (préfixes parallèles).

Mesuré égal sur les deux ordres réels (§ 17.1) :
- travail de 110–387 ms CPU à W1, soit 5 à 10 fois celui du noyau suivi du parcours (≈ 13 ms à K5, ≈ 37 ms à K10) ;
- W4 : 47–203 ms de mur.

Il n'est adopté que si une mesure G4 montre que T2a(Kmax) dépasse le budget visible (§ 11.3), ce qui ne peut arriver
que sur la voie GPU à K10.

### 6.6 Étage Q : index d'intervalles et requêtes

**Requête.** `WA(v, seuil)` : composante de l'ordre K à la coupe `≤ seuil` qui contient v, définie si
`rank(v) ≤ seuil`. La coupe ouverte à r est le seuil r − 1.

```text
WA(v, s):
  [l, h] ← [leaf_lo(v), leaf_lo(v) + leaf_n(v) − 1]
  l' ← 1 + max{ i < l : J[i] > s } (ou 0)          // bloc courant (≤ 16 lectures), puis arbre des blocs
  h' ← min{ i ≥ h : J[i] > s } (ou L − 1)
  si l' = h' : renvoyer leaf_node[π[l']]
  renvoyer jnode[i*] pour un i* ∈ [l', h') tel que J[i*] = max J[l'..h'−1]   // tout argmax convient (PO-T10)
```

Les requêtes sont triées par `leaf_lo` avant exécution. La critique a mesuré 71–75 ns par requête sur la structure
réelle.

**Marques** (toutes indépendantes, parallèles) :

| Marque | Formule | Nombre à K10 LiDAR (estimé) |
| --- | --- | ---: |
| ancre d'une naissance | sa feuille | 4,41 M |
| ancre d'un bloc | `WA(leaf_node[Min], rank(b))` | ≈ 5,6 M |
| contribution explicite | segment = ancre | rare |
| image d'une naissance (b, K) | `anchor_{K−1}[cell(b, K−1)]` | 4,41 M |
| image d'une fusion v | `WA_{K−1}(image(première feuille de v), rank(v))` | 3,01 M |
| naturalité (I8) | pour chaque parent P : `WA_{K−1}(image(première feuille de P), rank(v)) = image(v)` | 7,4 M en mode `full` ; 1/16 des fusions en mode `sampled` |
| entrée C∩X de x | `entry_level = D_K(x)` ; `entry_pos` = plus grand rang de niveau ≤ D_K(x), par dichotomie exacte U192 ; `entry_eq` ; `entry_node = WA(leaf_node[Min(resolve(K, N_K(x)))], entry_pos)` | 0,40 M |

- `D_K(x)` compte x lui-même et les poids : `D_K(x) = 0` si `w_x ≥ K`. `N_K(x)` = K plus proches de x (distance
  entière exacte, puis `SiteIdx`, copies comprises).
- **Localisateur de facettes** (API, § 9.1 du manuscrit) :
  - `locate_facet(K, F) → (β(F), cellule terminale, feuille)` ;
  - `component(K, F, a) = WA(leaf_node[Min(term)], a)`.

  Les représentants d'une cellule sont exportables sur demande. C'est la parade structurelle à la régression E5.

### 6.7 Numérotation canonique et digests

- **Nœuds** : ordre par (rang, plus petite feuille du sous-arbre). La clé est unique. Deux nœuds de même rang ont des
  sous-arbres disjoints, et une naissance de rang r n'appartient à aucune fusion de rang r (PO-T2).
- **Feuilles** : ordre des cellules de naissance (rang, S*).
- **Parents** : croissants.
- **K1** : feuilles en ordre de `SiteIdx`.

Cette numérotation ne dépend que de la forêt. Elle ne dépend ni de π, ni de l'algorithme de T2 (noyau ou D&C), ni du
nombre de fils, ni de l'ordre d'émission du générateur, ni des `PointId` (PO-T24). La v1 triait les fusions « par plus
petit parent » : cela imposait une dépendance séquentielle par rang.

- `tower_digest_v10` : SHA-256 d'une sérialisation linéaire dans l'ordre canonique. Elle contient :
  - les niveaux réduits en octets gros-boutistes, les parents, les successeurs, les images ;
  - les feuilles, décrites par leur population en `PointId` triés ;
  - les contributions et les ancres.

  **π, `leaf_lo`, `leaf_n`, J, `jnode` et `jblock` en sont exclus** : ce sont des données d'index, déterministes pour
  une version donnée mais non canoniques entre algorithmes de T2.
- `tower_merkle_v10` : empreinte sans numérotation, inchangée depuis la v1 (§ 5.7 de la v1).
- **PO-E15 d'EVAL, acceptée.** H(racine_k) et la section de l'ordre k du digest linéaire n'encodent que l'objet
  d'ordre k : niveaux réduits, populations en `PointId`, structure, images vers k − 1. Ils ne contiennent aucun numéro
  de rang, ni Kmax, ni kcat.
- Différentiel v9 : exporteur `tests/v9diff/v9_export.cpp` contre l'arbre v9 à `ce8a649dd`. Il reproduit d'abord le
  digest v9 épinglé, puis calcule `tower_merkle_v10`.

### 6.8 Contrat avec les consommateurs (CLUSTER_v2 § 4.1.5)

| `TowerOrderView` | Fourni par |
| --- | --- |
| `rank` non décroissant en identifiant | numérotation (rang, plus petite feuille) |
| `successor` | T2c |
| `leaf_lo`, `leaf_n`, `jrank` | T2b/T2c ; (P) garantit le théorème PG de CLUSTER pour tout algorithme de T2 |
| `entry_node`, `entry_level`, `entry_pos`, `entry_eq` | Q (§ 6.6) |
| `parents` (balayage différentiel) | T2c |
| `level_double(rang)` | binary64 pur, recalculé depuis la boule représentante |
| `compare(rang, u64)` | exact U192 |
| `facets()` | localisateur (§ 6.6) |

La tête ne dépend d'aucun algorithme de T2 : c'est aussi l'exigence de CLUSTER_v2. Le résolveur des points est celui
de la tour (saut K-NN). Terminaison et composante : PO-T4, PO-T11.

---

## 7. Prédicats et bornes arithmétiques (u18)

Table des largeurs d'ARCH § 17.5, complétée pour la tour :

| Quantité | Borne | Type |
| --- | --- | --- |
| coordonnées, différences | < 2^18 | i32 |
| carrés de distance entre sites, `D_K(x)` | < 3·2^36 < 2^38 | u64 |
| clé de sphère q3 (A, B, C) | A < 2^76, \|B\| < 2^96, \|C\| < 2^116 | i128 |
| clé de sphère q4 (A, B, C) | A < 2^60, \|B\| < 2^81, \|C\| < 2^100 | i128 |
| puissance d'un site `A·\|x\|² + B·x + C` | < 2^118 | i128 |
| barycentriques du centre sur S* | < 2^136, < 2^155 | i192 signé |
| niveau `N/D` d'un support q4 | N < 2^166, D < 2^122 | U192 / U128 |
| comparaison de niveaux `N1·D2 − N2·D1` | < 2^288 | U320 |
| niveau contre entier `D_K(x)` (`entry_pos`, `entry_eq`) | `N` contre `D·D_K` < 2^160 | U192 |
| **coquille circulaire** : vecteur centré homogène `v_s = D·s − N` (q ≤ 3 sur une entrée plane) | composantes < 2^97 | i128 |
| **coquille circulaire** : orientation `det2(v_j, v_k)` dans le plan dominant du cercle | < 2^196 | i256 signé (filtre double à borne prouvée, repli exact) |
| **coplanarité au centre** (entrée non plane) : `det3(v_a, v_b, v_s)` | < 2^293 | i320 signé |
| **Euler**, coquille circulaire (forme close) | \|coef\| ≤ u + 1 | i64 |
| **Euler**, table 3D (u ≤ 16) : somme sur 2^u parties, polynômes tronqués au degré kcat | pas de borne fine écrite | i128 avec contrôle de débordement (`__builtin_*_overflow`) ; dépassement : `resource_exhausted/euler_overflow` (jamais observé) |
| borne du filtre double des niveaux | erreur relative < 2^−49, marge 1 − 2^−46 | double puis exact |

- Sur une entrée z = 0, la projection dominante est le plan (x, y), et la coplanarité est globale : un drapeau
  d'entrée, sans aucun test par boule.
- Chaque largeur fait l'objet d'un `static_assert` dans `arith/widths.hpp` et d'un test contre BigInt aux coins u18
  (0 et 262 143). La collision `(0,1,0)/(0,0,65536)` est testée.
- Dans `resolve`, la décroissance stricte passe par le filtre double certifié, puis par U320.
- Aucun flottant ne décide ; `__FAST_MATH__` est interdit ; l'arrondi au plus proche est vérifié à l'entrée.

---

## 8. Invariants, modes de contrôle, statuts, complétude

### 8.1 Invariants de produit

| Id | Invariant | Mode | Raison de refus |
| --- | --- | --- | --- |
| I1 | structure de chaque boule (bornes, tris, disjonction, tailles, drapeaux, poids saturés) | toujours, toutes | `invariant_violated/catalogue_structure` |
| I1g | géométrie de la boule (sphère depuis S*, barycentriques, puissances des sites listés, q) | `full` : toutes ; `sampled` : 1/64 | `invariant_violated/census_mismatch` |
| I2 | Euler par ordre, `K ≤ min(W, kcat − 2)`, **entrées pondérées comprises** | toujours | `catalogue_incomplete/euler` |
| I3 | λ strictement décroissant à chaque saut de chaque descente | toujours | `invariant_violated/descent_not_decreasing` |
| I3b | `K ≤ hi(MEB(F))` à chaque pas (PO-T3) | toujours | `invariant_violated/window_above` |
| I4 | pointeur strictement antérieur à sa cellule | toujours | `invariant_violated/pointer_not_strict` |
| I5 | aucune sphère admissible absente du catalogue sur une descente | toujours | `catalogue_incomplete/missing_key` |
| I6 | une racine finale par ordre | toujours | `catalogue_incomplete/root_count` |
| I7 | `L − 1 = Σ_fusions (\|parents\| − 1)` ; rang d'un nœud < rang de son successeur | toujours | `invariant_violated/forest_identity` |
| I8 | naturalité des verticales (tous les parents) | `full` : toutes les fusions ; `sampled` : 1/16 | `invariant_violated/vertical_naturality` |
| I9 | cellule (b, K−1) présente pour toute naissance d'ordre K ≥ 2 | toujours | `invariant_violated/window_empty` |
| I10 | une image est une racine de K−1 à la coupe fermée de son niveau | toujours | `invariant_violated/vertical_closed_cut` |

### 8.2 Modes `checks=full` et `checks=sampled`

- Seules différences : le nombre de boules de I1g et de fusions de I8. **La sortie est identique** dans les deux
  modes, sinon c'est un refus. Le mode et le sel figurent dans le statut.
- `full` : portes, références jugées, reçus de qualification.
- `sampled` : chronomètre de contrat. Il économise ≈ 0,3 CPU·s à K5 et ≈ 1,3 CPU·s à K10 (estimé : validation
  60–100 ns par boule, naturalité 7,4 M requêtes à 75 ns).
- C'est la réponse au constat de la critique : D-T12 et I8 re-vérifiaient des théorèmes, contre le § 3.2 du plan de
  tests.

### 8.3 Statuts publiés

- Succès : `complete_relative`, avec les champs `{ kcat, checks, sel, euler_checked_up_to, circle_balls,
  table_balls, weighted_balls, shell_max_3d, jumps_hist, meb_calls, knn_closed_calls, t2_algo }`.
- Refus :
  - `invalid_input` (API) ;
  - `unsupported_degeneracy/shell_quotient_budget` (coquille 3D non circulaire, u > 16, boule publiée) ;
  - `catalogue_incomplete/{euler, missing_key, root_count}` ;
  - `invariant_violated/*` ;
  - `resource_exhausted/*`.
- Refus concurrents : on publie celui du plus petit (K, étage, indice), puis de la plus petite raison.
- Jamais `exact` : la tour est exacte **relativement** au catalogue.

### 8.4 Complétude : phrase de statut exacte

> La tour est exacte relativement au catalogue. Elle ne certifie pas la complétude des recensements I et U d'une
> boule présente : c'est une obligation du générateur (listes L(Q) certifiées par induction, GEN), contrôlée par le
> juge indépendant J-CENSUS dans les portes d'échelle, jamais en produit. I5, Euler et I6 détectent certaines
> omissions de **boules** ; aucune omission de **site** dans une boule présente n'est visible de la tour seule, et
> I5 partage ses listes avec le générateur.

La zone aveugle v9 (q2 à p = Kmax−1, q3 à p = Kmax−2) reste invisible à la tour seule quand l'omission ne change ni
les racines ni aucune descente. Quatre parades, dont le statut publie celles qui ont été exécutées :
- Euler à `kcat = Kmax + 2` (mode certifié) ;
- I5 ;
- J-CENSUS et J-MF dans les portes ;
- le juge par boîtes de GEN.

---

## 9. Obligations de preuve

« Esquisse » désigne l'argument écrit ici, à rédiger en note avant le code correspondant. « Fixture » désigne une
égalité gravée. Les numéros PO-T1 à PO-T17 sont ceux de la v1 ; seules les lignes modifiées sont détaillées.

| Id | Énoncé | Statut visé | Fixtures d'égalité |
| --- | --- | --- | --- |
| PO-T1 | Tout K-multiensemble d'une boule fermée b, `K < p_w + u_w`, est dans une même composante de Γ_K(λ_b) | proved_here | carré K2, octaèdre pondéré |
| PO-T2 | Un représentant (I ∪ A, A strict) a `β < λ_b` | proved_here | paire, support3, support4 |
| PO-T3 | `F ⊆ b̄` et MEB(F) = b ⟹ `K ≤ p_w + u_w` ; si de plus `F ⊇ I` (toutes copies), `K ≥ p_w + q` | proved_here | shell7_window |
| **PO-T4** | `resolve` termine ; **chaque saut fait décroître λ strictement** ; la cellule rendue est d'ordre K, de niveau ≤ β(F), reliée à F dans Γ_K(β(F)) fermé ; un refus n'a lieu que si une sphère admissible manque | proved_here (preuve ci-dessous) | E5, portal_equal, fixtures de descente (§ 13.2) |
| PO-T5 | `Min(c)` est dans la composante fermée des facettes de c à λ_c ; les pointeurs forment une forêt enracinée aux naissances | proved_here | inert_ball ×2 |
| PO-T6 | La composante pré-lot de tout représentant r contient `Min(resolve(r))` | proved_here | silent_A_E, E5 |
| PO-T7 | Préfiltre MSF neutre | **retiré du produit** (Borůvka supprimé) ; reste vrai (registre, « expansion étoilée ») et sert à PO-T22 | — |
| PO-T8 | La forêt v10 reproduit les groupes, actions, parents et ancres de la v9 | proved_here (§ 10) | ABCZ, carré, E5 |
| PO-T9 | Verticales et naturalité | conditional_theorem → proved_here | inert_ball |
| PO-T10 | WA par intervalles : la composante au seuil s est l'intervalle maximal de jonctions ≤ s ; son nœud crée toute jonction de valeur maximale de l'intervalle | proved_here (corollaire de PO-T18) | pair |
| PO-T11 | C∩X : `x ∈ L_K(a) ⟺ D_K(x) ≤ a` ; composante = celle de `N_K(x)` | proved_here | oracle C∩X brut |
| PO-T12 | Quotient local (tables) = composantes strictes | conditional_theorem → proved_here | carré, triangle rectangle |
| PO-T13 | Hors fenêtre, un bloc est inerte | proved_here | mutant `window_lo` |
| PO-T14 | Sortie indépendante de W et de l'ordre d'émission | proved_here | W1/W8/W48 |
| PO-T15 | Semis exact | proved_here | variante `no_hk` |
| PO-T16 | Multiplicités : Th. 2 sur multiensembles, fenêtre basse en positions | proof_obligation → proved_here | paire (3,3), triangle (3,1,1), octaèdre pondéré, point de poids K |
| PO-T17 | Euler par le nerf, sites distincts | proved_here | carré (1, −3, 1, 1), contre-exemple Euler/Kmax+2 |
| **PO-T18** | Tout (π, J) qui satisfait (P) détermine la forêt FULL de l'ordre par la règle cartésienne ; le noyau T2a suivi de T2b produit un tel (π, J) quel que soit l'ordre des arêtes à l'intérieur d'un rang | **proved_here** (preuve ci-dessous) ; remplace au registre la ligne « contraction des plateaux par composantes fortement connexes » (`proof_obligation`) | carré K2 (4 parents), E5, A–E ; mutant `mat_no_plateau_grouping` |
| **PO-T19** | Euler pondéré : `#{s : w_s ≥ K} + Σ_b e_K(b) = 1` pour 1 ≤ K ≤ W, avec le polynôme du § 6.1 | **proved_here** (preuve ci-dessous) ; lève ARCH P17 / O-W2 | 340 nuages (§ 17.2), mutant `euler_shell_ignores_weight` |
| **PO-T20** | Coquille coplanaire au centre : quotient par fenêtres angulaires, et Euler par la forme close | **proved_here** (preuve ci-dessous) | 240 nuages plans (§ 17.3), îlots de réseau, rectangle |
| **PO-T21** | Table pondérée par classes de supports en O(u·2^u) | **proved_here** (preuve ci-dessous) | 150 nuages (§ 17.3) |
| **PO-T22** | L'option D&C produit un (π, J) qui satisfait (P) | proved_here (preuve ci-dessous) | variante de test `dc1`, `dc2` |
| **PO-T23** | Classement de liste et union-find à potentiels produisent le (π, J) du parcours des listes | proved_here (trivial, écrit) | égalité bit à bit (§ 17.1) |
| **PO-T24** | La numérotation (rang, plus petite feuille) ne dépend que de la forêt | proved_here | variantes `dc1` / `pot` : même `tower_digest_v10` |

### 9.1 PO-T4 : décroissance stricte (correction de M1)

Soit m = MEB(supp F), de centre c et de niveau λ_m. On a F ⊆ b̄(m), et il existe S* ⊆ supp F sur la sphère avec
c ∈ relint conv S*. Soit p le poids strictement intérieur et q le nombre minimal de positions d'un support positif. Un
saut n'a lieu que dans deux cas.

1. **m est au catalogue** (boule b) **et K n'est pas dans sa fenêtre.** Comme F ⊆ b̄, on a
   K = |F| ≤ p + u = hi(b) (K ≤ Kmax et K ≤ W). Donc K < lo(b) = p + q − 1, c'est-à-dire K − p ≤ q − 2.
2. **m est absente, sans refus.** Soit p ≥ kcat ≥ K. Soit p + q ≥ kcat + 2, d'où K − p ≤ kcat − p ≤ q − 2.

Soit N les K plus proches de c.
- Si K ≤ p, N est dans la boule ouverte, donc β(N) ≤ max_{x∈N} |x − c|² < λ_m.
- Sinon N contient tout l'intérieur, plus K − p ≤ q − 2 copies de la coquille, portées par au plus q − 2 positions.
  Si c ∈ conv(supp N ∩ sphère), la face minimale de ce polytope qui contient c est l'enveloppe d'un support positif
  d'au plus q − 2 positions : cela contredit la minimalité de q. Par Gordan, il existe v avec
  ⟨v, x − c⟩ > 0 sur la coquille de N. Le centre c + εv réduit strictement la distance à ces points, et l'intérieur
  garde sa marge pour ε petit : β(N) < λ_m.

Conséquences :
- La terminaison découle du nombre fini de niveaux.
- La liaison se fait par PO-T1 appliqué à m : F et N sont des K-multiensembles de b̄(m) avec K < p + u. En effet,
  K = p + u forcerait F = N, puis MEB(N) = m, ce qui contredit la décroissance. Tous les niveaux traversés sont
  ≤ β(F).
- **La branche « λ égal, poids retenu sur la sphère décroissant » de la v1 n'existe pas.** Le compteur σ est retiré,
  et I3 devient strict. Mesures concordantes de la critique : 0 pas à rayon égal sur 16 fixtures, 300 nuages
  {0..2}³ (1,37 M MEB) et les échantillons LiDAR.

### 9.2 PO-T18 : règle cartésienne (réponse à B2 et au registre)

**Définitions.** Les rangs sont des entiers denses. G_{<t} = G_{t−1}. Les nœuds de la forêt FULL de l'ordre K sont :
- les feuilles (naissances) ;
- les couples (C, t) où C est une composante de G_t qui réunit au moins deux composantes de G_{t−1}.

C'est exactement un groupe de lot à ≥ 2 racines (§ 3.5 et § 10, point 2). Les parents de (C, t) sont les nœuds
courants des composantes de G_{t−1} contenues dans C. Toute arête de rang t relie des feuilles nées à un rang < t
(PO-T2).

**(A) Le noyau T2a suivi de T2b satisfait (P).**
- Invariant, après les arêtes de rang ≤ t : chaque composante du DSU est une liste chaînée contiguë. Ses jonctions
  internes sont les rangs des arêtes qui ont concaténé ses morceaux, donc ≤ t.
- Les arêtes suivantes ne créent de jonctions (de rang > t) qu'aux bords de ces listes.
- Dans π final, chaque composante de G_t est donc un intervalle de jonctions internes ≤ t, bordé par des jonctions
  > t ou ∞. C'est (P).
- L'ordre des unions à l'intérieur d'un rang n'intervient pas.

**(B) Tout (π, J) qui satisfait (P) donne la forêt par la règle cartésienne.**
- Par (P), une composante C de G_t correspond à l'intervalle maximal I_t(C) de jonctions ≤ t.
- C est un nœud (C, t) ⟺ C n'est pas une composante de G_{t−1} ⟺ max J sur I_t(C) = t : sinon I_t(C) serait déjà
  maximal au seuil t − 1.
- Les parents sont les composantes de G_{t−1} dans C, c'est-à-dire les morceaux de I_t(C) séparés par les jonctions
  égales à t. Chaque morceau est une feuille (une position) ou un nœud (I′, max < t).
- Le successeur d'un nœud d'intervalle I est la composante suivante qui contient I : elle se forme au plus petit
  seuil où I s'étend, soit min(J[l′−1], J[h′]). Si les deux valeurs sont égales, c'est le même nœud, parce que tout
  ce qui les sépare est < elles.
- Unicité : le nœud (I, t) a pour première jonction ouvrante le plus petit i ∈ I avec J[i] = t. C'est exactement le
  test `first(i) = i` de T2c.

**Corollaires.**
- PO-T10 : l'argmax de WA.
- Les multifusions ne sont jamais binarisées. Le mutant `mat_no_plateau_grouping` (chaque jonction ouvre son nœud)
  reproduit le motif `false_in_general` du registre, et il est tué par le carré K2 et par E5.
- CLUSTER_v2, théorème PG : il ne dépend que de (P).

### 9.3 PO-T19 : Euler pondéré (réponse à M4 ; ARCH P17)

- Les copies d'un site sont des boules distinctes de même centre.
- Pour une famille finie de boules fermées de rayon √a et un point y, avec n(y) le nombre de copies qui contiennent y,
  on a l'identité `1[n(y) ≥ K] = Σ_{A : |A| ≥ K} (−1)^{|A|−K} C(|A|−1, K−1) · 1[y ∈ ∩A]`. Elle se vérifie par
  l'identité binomiale sur n(y).
- χ est une valuation sur les ensembles polyconvexes, et χ(∩A) = 1 si ∩A ≠ ∅, ce qui équivaut à β(A) ≤ a. Donc
  `χ(L_K(a)) = Σ_{A : |A| ≥ K, β(A) ≤ a} (−1)^{|A|−K} C(|A|−1, K−1)`.
- On regroupe les parties A par leur boule minimale b. On a MEB(A) = b ⟺ A ⊆ b̄ et c ∈ conv(A ∩ sphère), avec conv
  fermée.
- Avec `x^{|A|}`, la série génératrice de ces A vaut `(1+x)^{p_w} · Σ_{T ⊆ supp U, c ∈ conv T} Π_{s∈T} ((1+x)^{w_s} − 1)`.
- La transformation `Σ_K C(j−1, K−1)(−1)^{j−K} y^{K−1} = (y−1)^{j−1}` envoie `x^j` sur `(y−1)^{j−1}`. On obtient
  `Σ_K e_K(b) y^{K−1} = y^{p_w} Σ_{T : c ∈ conv T} (y−1)^{|T|−1} Π_{s∈T} [w_s]_y`.
- La boule de rayon nul d'un site donne `[w_s]_y`, soit 1 pour chaque K ≤ w_s.
- Pour a ≥ β(X), L_K(a) est une union de convexes qui contiennent tous un point commun. Elle est étoilée, donc
  χ = 1 pour K ≤ W, et 0 au-delà.
- Pour w ≡ 1, on retrouve la formule de la v1, avec `n·[K=1]`.
- Portée du contrôle produit : `K ≤ kcat − 2`. Une boule contribue aux ordres `p_w + 1 ≤ K ≤ p_w + u_w`, et le
  catalogue est admis jusqu'à `p_w + q ≤ kcat + 1` avec q ≤ 4 positions.
- Vérifié sur 340 nuages dégénérés (grilles, plans, poids 1–4), par énumération exacte de toutes les boules
  critiques : 4 746 boules, 3 270 contrôles (tous les K ≤ W + 1), 0 échec. Le mutant « coquille sans poids » échoue
  sur 40 nuages sur 40.

### 9.4 PO-T20 : coquilles circulaires (réponse à M4)

Si U et c sont coplanaires, U est sur un cercle de centre c. Pour T ⊆ U, on a c ∉ conv T ⟺ T tient dans un
demi-cercle ouvert.

- Fenêtres : `A_j = {j} ∪ {k : det(v_j, v_k) > 0}`. C'est le demi-cercle semi-ouvert [θ_j, θ_j + π), et il tient
  dans un demi-cercle ouvert. Les antipodes sont exclus.
- Tout T strict est contenu dans A_j pour un **unique** j ∈ T : son premier point dans le sens trigonométrique.
  Deux candidats j et k donneraient θ_k ∈ (θ_j, θ_j + π) et θ_j ∈ (θ_k, θ_k + π), ce qui est contradictoire.
- **Quotient.**
  - Les t-multiensembles d'une fenêtre de poids ≥ t sont connexes entre eux : on échange une copie à travers un
    (t+1)-multiensemble de la fenêtre. Si le poids vaut t, il n'y a qu'un sommet.
  - Une arête (t+1)-multiensemble stricte tient dans une fenêtre : les arêtes ne franchissent pas les fenêtres.
  - A_i ∩ A_k est un arc contenu dans toute fenêtre dont le début est entre les deux. Les liaisons se ramènent donc
    aux fenêtres consécutives, avec `w(A_j ∩ A_suivant) ≥ t`.
  - La couverture est l'union des fenêtres vivantes.
- **Euler.** On décompose chaque T strict par son premier point :
  `Σ_{T strict} (y−1)^{|T|−1} Π [w]_y = Σ_j [w_j]_y · Π_{k ∈ A_j ∖ j} y^{w_k} = Σ_j [w_j]_y · y^{W_j − w_j}`.
  Le total sur les T non vides vaut `[u_w]_y`, d'où la forme close du § 6.1.
- **Vérifié** contre le quotient pondéré brut et l'énumération d'Euler, sur 240 nuages plans (îlots de réseau
  x² + y² ∈ {25, 50, 65}, rectangles, grilles, poids 1–3, u ≤ 9) : 14 827 contrôles de quotient, 4 813 contrôles
  d'Euler, 0 échec.

### 9.5 PO-T21 : table pondérée en O(u·2^u)

- Sommets : t-multiensembles stricts, regroupés par support exact S. La classe S existe ⟺ S est strict et
  |S| ≤ t ≤ w(S). Elle est connexe : on échange une copie par un (t+1)-multiensemble de support S si w(S) ≥ t + 1 ;
  sinon elle est réduite à un sommet.
- Une arête G, de support S strict avec |S| ≤ t+1 ≤ w(S), a pour faces :
  - la classe S, si y est de multiplicité ≥ 2 dans G ;
  - la classe S ∖ {y}, si y est de multiplicité 1.
- Toutes les classes faisables de `{S} ∪ {S ∖ {y}}` sont reliées :
  - si |S| ≤ t, chaque S ∖ {y} faisable est reliée à S par un G qui double un autre site ;
  - si |S| = t + 1, un seul G (multiplicités 1) relie toutes les S ∖ {y}.
- Vérifié contre le quotient brut du prototype pondéré : 11 852 contrôles, u ≤ 8, 0 échec.

### 9.6 PO-T22 : option D&C

On coupe au rang r*. Le résultat léger satisfait (P) pour t < r* sur ses composantes ; le lourd satisfait (P) pour
t ≥ r* sur les composantes contractées.

- t < r* : les composantes de G_t sont celles du graphe léger. Chacune est dans un bloc léger, bordée par des
  jonctions légères > t ou par des jonctions lourdes ≥ r* > t.
- t ≥ r* : les composantes de G_t sont des réunions de blocs légers entiers (jonctions internes < r* ≤ t) le long des
  composantes lourdes. Les blocs sont consécutifs dans l'ordre π lourd, joints par des jonctions lourdes ≤ t.
- On conclut par récurrence sur la profondeur. Les arêtes lourdes internes à une composante légère sont écartées
  (propriété de coupe, PO-T7).

---

## 10. Équivalence avec la sémantique v7/v9 des ancres

1. **Même racine pré-lot.** L'ancre v9 `anchor[T_9(r)]` est installée après la fermeture du lot de T_9(r). r est
   relié aux facettes de T_9(r) à un niveau ≤ β(r) < λ (PO-T1, PO-T4). Donc `racine_pré-lot(anchor[T_9(r)])` est la
   composante de Γ_K(λ⁻) qui contient r, et PO-T6 donne la même pour la v10. Les terminaux peuvent différer, pas les
   racines.
2. **Mêmes groupes.** Les groupes v9 sont les composantes du graphe biparti (cellules du lot, racines pré-lot). Deux
   racines sont dans un même groupe ⟺ une chaîne d'hyperarêtes de rang λ les relie ⟺ elles sont dans une même
   composante de G_λ, qui réunit alors ≥ 2 composantes de G_{λ−1}. C'est le nœud (C, λ) de PO-T18.
3. **Mêmes ancres.** L'ancre v10 est `WA(Min, λ)` : le nœud du groupe, ou la racine inchangée (continuation, bloc
   inerte).
4. **Mêmes contributions.** Même règle locale, même date, même segment.
5. **Mêmes verticales.** Naissance : même nœud (L04-F11). Fusion : égalité par naturalité (I8).
6. **Plateaux.** Les représentants d'un lot ont `β < λ` (PO-T2). Les multifusions naissent de la règle cartésienne
   et ne sont jamais binarisées (PO-T18). L'atomicité v9 (racines lues avant toute union) et la règle cartésienne
   donnent les mêmes groupes (point 2).
7. **Continuations.** Toujours traitées par la voie générale ; le mutant `drop_continuations` est tué par ABCZ.
8. **Descente.** Correction de la v1 : aucun saut ne garde la même sphère, et chaque saut fait décroître λ
   strictement (PO-T4). La fixture v9 `actual_equal_radius_descent` testait l'échange d'intrus v9. Elle devient une
   fixture de **l'échange d'intrus de J-DESC** (§ 13.4), et, côté tour, une fixture de descente ordinaire.
9. **Ancres inertes.** Publiées pour les consommateurs ; le pointeur du bloc suffit à la descente.

La validation exacte de la chaîne contre Γ_K reste celle de la v1 : 3 450 nuages et 16 fixtures, 1 340 534 contrôles.
La critique y ajoute 12 nuages à n = 10–11 (48 064 contrôles). Le spike ajoute l'égalité de T2 réorganisé avec le
Kruskal par lots sur la structure réelle LiDAR (§ 17.1).

---

## 11. Parallélisme, déterminisme, chemin critique

### 11.1 Ordonnancement

- **Pool.** Un seul `sched::Pool` (ARCH § 7), sans aucun fil créé ailleurs ni parallélisme imbriqué (R6).
- **Grains.** 256 cellules pour P et G ; 4 096 cellules pour M et T1 ; une tâche par ordre pour T2a ; 16 384 éléments
  pour T2b, T2c et Q.
- **Ordre d'émission** (correction du mineur « étage M global ») :
  - les tâches de G sont émises par **K décroissant** ;
  - un compteur par ordre déclenche M(K), puis T1(K), dès que G(K) est fini ;
  - T2a(K) est poussée comme **une** tâche à la fin de T1(K), et occupe un ouvrier pendant que les autres continuent
    G, M et T1 des ordres inférieurs ;
  - T2b(K) et T2c(K) sont des boucles parallèles lancées par l'orchestrateur à la fin de T2a(K), jamais depuis
    T2a elle-même ;
  - Q(K) suit T2c(K) ; les images de l'ordre K + 1 attendent l'index de l'ordre K.
- **Déterminisme.** Les écritures se font à des positions précalculées (comptage, préfixe, remplissage). Le noyau
  suit l'ordre des clés. Les règles du classement de liste sont tirées par indice. La numérotation est canonique.
  Aucun résultat ne dépend de l'ordre d'arrivée. Les refus parallèles sont réduits par (K, étage, indice).

### 11.2 Dépendances

```text
P ──► G(Kmax) ──► M(Kmax) ──► T1(Kmax) ──► T2a(Kmax) ──► T2b/T2c(Kmax) ──► Q(Kmax)
  └─► G(Kmax−1) ─► M ─► T1 ─► T2a(Kmax−1) ─► T2b/T2c ─► Q(Kmax−1) ─► images de Kmax
  └─► …                                                  (les images de K+1 attendent l'index de K)
```

Le noyau séquentiel T2a(Kmax) tourne **pendant** l'étage G des ordres inférieurs. Sur 08/000200, l'ordre Kmax porte
39 % des représentants à K5 (1,39 M sur 3,61 M) et 21 % à K10 (3,51 M sur 16,7 M), selon la critique.

### 11.3 Chemin critique et plafond d'Amdahl

Les scénarios sont ceux d'ARCH § 6.3–6.4 : pessimiste ρ = 1 et S48 = 24 ; optimiste ρ = 0,6 et S48 = 30.

| | K5 LiDAR (08/000200) | K10 LiDAR (08/000200) |
| --- | ---: | ---: |
| T2a(Kmax), mesuré en local, arêtes semées | 6,6–9,2 ms | 15,9–22,3 ms |
| T2a(Kmax), toutes arêtes (×1,3), en local | ≈ 9–12 ms | ≈ 21–29 ms |
| T2a(Kmax) sur G4 (ρ ∈ [0,5 ; 1]) | ≈ 4,5–12 ms | ≈ 10–29 ms |
| G de tous les ordres (§ 12.2), mur W48 G4 | ≈ 22–79 ms | ≈ 0,14–0,44 s |
| G des ordres < Kmax, mur (fenêtre qui recouvre T2a(Kmax)) | ≈ 13–48 ms | ≈ 0,11–0,35 s |
| **T2a visible sur CPU** | **0–1 ms** (recouvert, sauf si G est rapide) | **0** (recouvert) |
| T2b + T2c(Kmax) à W48 (mesuré 50–60 et 165 ms CPU à W1 en local, ÷ S48) | ≈ 1–3 ms | ≈ 3–7 ms |
| plafond à fils infinis (T2a + T2b/T2c de l'ordre Kmax) | ≈ 6–15 ms | ≈ 13–35 ms |
| v9 mesuré (R22) : tour / part séquentielle (lots sous contention de 409 à 889 fils, L04-F7 ; pas un rapport de vitesse) | 289–304 / ≈ 215 ms | 1 212 / ≈ 800 ms |

- **Sur CPU**, le noyau séquentiel n'est plus sur le chemin critique : l'étage G le recouvre.
- **Sur GPU** (§ 14), G rétrécit à quelques millisecondes et T2a redevient visible : ≈ 4,5–12 ms à K5, dans le budget
  « T2 ≤ 15 ms » d'ARCH § 6.8 ; ≈ 10–29 ms à K10.
- Pour descendre sous ≈ 10 ms à K10 sur GPU, on active l'option D&C (d = 1 ou 2), dont la division du noyau par 2
  à 4 est estimée à partir des tailles de sous-problèmes, ou un dendrogramme GPU par contraction d'arbre
  (PANDORA, ICPP 2024). Pistes V10-x, sur mesure G4.
- ARCH P13 : la porte « balayage ≤ 20 ms à K5 » est tenue avec d = 0 dans les deux scénarios, sous l'hypothèse
  ρ ≤ 1 que la session C0 doit mesurer.

---

## 12. Coûts attendus et mémoire

### 12.1 Volumes (mesurés)

| Entrée | Boules | Cellules/boule | Représentants Σ_K | Non semés Σ_K | Feuilles Σ_K / ordre Kmax | Arêtes semées, ordre Kmax |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 K5 (R22) | 1 306 696 | ≈ 1,67 | 3 621 785 | — | 897 776 / 341 081 | — |
| ng00 K10 (R22) | 5 512 670 | ≈ 1,81 | 17 389 031 | — | 4 414 230 / 979 350 | — |
| 08/000200 K5 (cble, critique) | 1 407 885 | 1,67 | 3 609 776 | 802 733 | — / 361 211 | 566 911 |
| 08/000200 K10 (cble, critique) | 5 483 320 | 1,81 | 16 718 792 | 4 744 081 | — / 937 257 | 1 380 911 |
| uniforme 8k / 16k / 32k K5 (cble) | 0,59 / 1,23 / 2,53 M | 1,61 | 2,96–2,97 par boule | — | — | — |
| uniforme 8k / 32k K10 (cble) | 3,09 / 13,49 M | 1,77 | 3,36–3,37 par boule | — | — | — |

### 12.2 Modèle de coût

Coûts unitaires locaux, en CPU·s sur le codespace. « Mesuré » renvoie au § 17 ou à la critique ; le reste est estimé.

- consultation H_K : 100–200 ns (estimé) ;
- MEB exacte ≤ 12 points : 0,3 µs à K5, 0,5 µs à K10 (estimé) ;
- recherche de sphère : 0,1–0,2 µs (estimé) ;
- saut K-NN : 0,4–0,8 µs (estimé) ;
- entrée C∩X (`knn_closed` puis `resolve`) : 1–2 µs (estimé ; la v1 l'avait omise) ;
- WA : 71–75 ns (mesuré par la critique).

| Étage | K5 LiDAR, CPU·s local | K10 LiDAR, CPU·s local | Base |
| --- | ---: | ---: | --- |
| P (`checks=sampled`) | 0,10–0,15 | 0,4–0,6 | structure 5–10 ns ; géométrie 1/64 ; Euler |
| G : H_K | 0,36–0,72 | 1,7–3,3 | 3,61 M / 16,7 M consultations |
| G : MEB, recherche, sauts | 0,49–0,71 | 4,6–6,3 | 0,94 M / 6,1 M MEB ; 0,30 M / 2,56 M sauts (critique) |
| G : entrées C∩X | 0,23–0,46 | 0,46–0,92 | n × Kmax résolutions |
| M, T1 | 0,05–0,08 | 0,3–0,4 | sauts de pointeurs, arêtes |
| T2a (tous ordres, séquentiels mais concurrents) | 0,02–0,03 | 0,08–0,12 | mesuré sur l'ordre Kmax ; Σ ≈ 2,5× et 4,7× |
| T2b + T2c + numérotation | 0,17–0,19 | 0,8–1,0 | mesuré sur l'ordre Kmax ; Σ des feuilles |
| Q (index, ancres, contributions, verticales, entrées ; naturalité 1/16) | 0,25–0,30 | 1,2–1,4 | ≈ 3,5 M / 16 M requêtes WA |
| **Tour, CPU total en local** | **≈ 1,7–2,6** | **≈ 9,5–14** | |
| **Tour sur G4, W48, optimiste / pessimiste** | **≈ 33–53 / 70–110 ms** | **≈ 0,19–0,28 / 0,40–0,58 s** | ρ·CPU/S48, plus T2a visible (0 sur CPU) |
| v9 mesuré (R22) | 289–304 ms | 1 212 ms | |
| budget ARCH § 6.5 (tour) | 0,07–0,30 s | 0,40–0,65 s | |

Lectures :
- **L'étage G est 65 à 75 % de la tour.** Ses trois postes sont H_K, la MEB et les entrées C∩X. C'est la cible GPU,
  et celle du levier « jointure triée » (D-T2).
- La tour K5 CPU seule ne tient pas, à elle seule, un contrat total de 100 ms en scénario pessimiste. ARCH § 6.8 le
  réserve à la voie GPU.
- **Synthétiques** (G4 W48, pessimiste puis optimiste, estimés à 1,2–1,7 µs CPU par boule à K5 et 2,0–2,5 µs à K10) :
  - uniforme K5 8k / 16k / 32k : ≈ 30–42 / 62–87 / 125–180 ms, et ≈ ×0,35 en optimiste ;
  - uniforme K10 8k / 32k : ≈ 0,26–0,32 / 1,1–1,4 s ;
  - banc de clustering (K ≤ 3, ≈ 25 boules par site, 32k) : ≈ 25–35 ms.

  En local, à 3 fils effectifs sur l'hôte chargé, il faut multiplier par ≈ 8–10.
- La porte de coût porte sur des **compteurs déterministes** par boule : `meb_calls`, `jumps_hist`, `hk_hits`,
  `pointer_rounds`, `edges`, `wa_queries`. Leur rapport par doublement 8k → 16k → 32k doit rester ≤ 1,15
  (ARCH § 6.7).

### 12.3 Mémoire (octets par boule au-delà du catalogue, K10 LiDAR)

Hypothèses par boule : 1,81 cellule, 1,32 nœud, 0,80 feuille, ≈ 1,5 arête Σ_K (toutes arêtes), 3,05 représentants.

| Poste | Persistant | Transitoire | Libéré après |
| --- | ---: | ---: | --- |
| atlas (`base`, `lo`, `width`, `by_k`) | 13,2 | — | — |
| tables locales, H_K | — | 6,4 | G(K) |
| `target` (CSR des représentants) | — | 19,8 | T1(K) |
| `ptr` / `Min` en double tampon | — | 14,5 | M(K) (7,2 de feuilles jusqu'à T1(K)) |
| arêtes (a, b, r : 12 o) | — | 18 | T2a(K) |
| état du noyau (V 16 o et NJ 8 o par feuille) | — | 19 | T2b(K) |
| temporaires de T2c (l′, first, préfixe : 12 o par jonction) | — | 10 | T2c(K) |
| nœuds (rang, CSR des parents, successeur, image, intervalle : 28 o par nœud) | 37 | — | — |
| index (π, `leaf_node`, `leaf_cell`, J, `jnode`, `jblock` : 20,6 o par feuille) | 16,5 | — | — |
| ancres (4 o par cellule) | 7,2 | — | — |
| entrées C∩X (21 o par site et par K) | 1,5 | — | — |
| **Total** | **≈ 75** | **≤ 41 en G, puis ≤ 37 en T, si tous les ordres étaient en vol** | |

- Les transitoires sont libérés **par ordre**, et les structures persistantes d'un ordre ne sont allouées qu'en T2c
  de cet ordre. Avec au plus la moitié des ordres entre T1 et T2c à la fois (l'ordre Kmax en tête, D-T17), le pic
  compté visé est **≤ 100 o par boule** (ARCH_v2 § 5.6 : 551 Mo à K10 sans sol). C'est une estimation : la porte
  « budget » le mesure à 8k/16k/32k et sur les trames.
- K10 LiDAR : ≈ 5,5 M × 75 o ≈ 415 Mo persistants pour la tour. La v9 mesurait 6,36 Go de RSS processus.
- Correction de la critique : `jtree` coûtait jusqu'à 16 o par feuille, pas « ≤ 8 ». Il est remplacé par `jblock`
  (≤ 0,6 o par feuille). L'alignement avec ARCH se fait sur ARCH_v2 § 5.6 (≤ 100 o de pic) ; les 320 Mo étaient une
  valeur d'ARCH_v1.

---

## 13. Portes, juges, mutants, campagnes

Les codes de sortie sont exacts (`run_expect.cmake`) : 0 conforme, 1 désaccord du juge, 2 refus avant calcul,
3 plancher ou invariant violé, 4 mutant tué. Un signal est un échec. Labels CTest : `gate`, `oracle`, `scale8000`,
`scale16000`, `scale32000`, `lidar`. Règle du plan de tests § 3.2 : ce qu'un théorème garantit est invoqué, pas
re-parcouru. Les juges d'échelle cherchent des **fautes d'implémentation** ; aucun n'est exhaustif.

### 13.1 T2 multi-K aléatoire dégénérée (oracle, établit la vérité)

- **Oracle.** Port de `census_tower_oracle.hpp` : Γ_K par sous-multiensembles à copies étiquetées, n ≤ 14. S'y
  ajoutent `check_cut`, BigRat de test, et les mutants de l'oracle v9.
- **Contrôles, à chaque niveau critique, coupes ouverte et fermée, pour chaque K ≤ K_eff :**
  - partition des K-parties présentes (via `resolve` et WA) ;
  - composantes vivantes et couvertures ;
  - images verticales, et « image = racine à la coupe fermée » ;
  - entrées C∩X de chaque site.
- **Familles de nuages.**
  - Grilles `{0..2}³` à `{0..4}³` ; génériques u18 ; coins u18 ; fixtures.
  - **Entrées planes z = 0** : îlots de réseau x² + y² ∈ {25, 50, 65}, rectangles, grilles, et nuages de type SIPU
    réduits.
  - **Mini-nuages de balayage** : lignes anisotropes, n ≤ 14.
  - **Amas** : 2 ou 3 amas serrés (±3) plus 30 % de points épars, n = 9–13. C'est la famille qui produit les
    descentes longues : 1 747 K-parties à 3 sauts et 139 à 4 sauts sur 200 nuages (§ 17.4).
  - **Nuages pondérés jusqu'à 12 sites**, poids total ≤ 16 : c'est la T2 pondérée demandée par M4.
- **Tirages.** Kmax dans `[1, n]` ; doublons dans 20 % des nuages ; `PointId` épars et inversés ; entrée permutée.
- **Catalogues amputés** (critique M3, dernier mutant). Pour ≥ 200 nuages en mode certifié, on retire une boule
  admissible visitée par une descente. On attend un refus `catalogue_incomplete/{missing_key, euler, root_count}`
  (code 2). La copie mutée qui désactive I5 doit être tuée par la T2 (code 4).
- **Planchers** (`--min-*`). La v1 exigeait « ≥ 100 sphères à rayon égal » ; ce plancher était inatteignable et il
  est **retiré** (§ 9.1).

  | Plancher | Valeur |
  | --- | ---: |
  | nuages, en sortie de phase (en CI) | ≥ 5 000 (200) |
  | contrôles de composantes | ≥ 10^6 |
  | coquilles non régulières, dont ≥ 100 circulaires à u ≥ 5 et ≥ 100 par table 3D | ≥ 500 |
  | continuations | ≥ 50 |
  | multifusions à ≥ 3 parents | ≥ 200 |
  | sites de poids ≥ 2 | ≥ 200 |
  | nuages plans | ≥ 300 |
  | nuages pondérés à ≥ 8 sites | ≥ 100 |
  | représentants de la tour à ≥ 1 saut | ≥ 2 000 |
  | représentants de la tour à ≥ 2 sauts | ≥ 100 |
  | K-parties résolues (contrôles de partition) à ≥ 3 sauts | ≥ 1 000 |
  | K-parties résolues à ≥ 4 sauts | ≥ 50 |

  Les descentes de 3 et 4 sauts apparaissent sur les K-parties que la T2 résout pour ses contrôles de partition, pas
  sur les représentants de la construction : aux petites tailles, ceux-ci sont presque tous semés ou terminaux. Le
  régime des **représentants** à ≥ 3 sauts, celui du LiDAR, est porté par J-DESC à l'échelle (§ 13.4).

### 13.2 Fixtures (gravées, permanentes)

**Reprises de la v1**, chacune avec son `tower_digest_v10` épinglé et son attendu mathématique :
- pair ; E5 ; A–E ; carré de côté 2 (e_K = (1, −3, 1, 1), multifusion K2 à 4 parents) ;
- growth_ABCZ et growth_redundant ; inert_ball ×2 ; shell7_window ;
- support3, support4 ; u16/u18_tetra, u18_corners ; portal_equal ;
- triangle rectangle ; K = n ; contre-exemple Euler/Kmax+2 ;
- pondérées : paire (3,3), triangle (3,1,1), octaèdre mixte, point de poids K, tous identiques ;
- continuation a×2 + b (ARCH P7w).

**Nouvelles :**
- **plane_rectangle** : rectangle aligné 2a × 2b, 4 points cocycliques, plus un point intérieur (critique M4) ;
- **plane_lattice_25 / 65** : îlots de 12 et 16 points sur x² + y² = 25 et 65, plus le centre. Chemin circulaire ;
- **plane_lattice_325** : 24 points sur un cercle. C'est une fixture **locale** : quotient circulaire et Euler contre
  l'énumération brute, u = 24 étant hors de portée de la T2. Elle prouve la levée du plafond u ≤ 16 pour les
  entrées planes ;
- **sphere_lattice_3d** : points de réseau sur x² + y² + z² = 9 (30 points, non coplanaires). On attend le refus
  typé `shell_quotient_budget` avec la boule publiée ;
- **weighted_euler** : trois nuages pondérés du § 17.2, avec leurs polynômes attendus ;
- **descent_k4_clusters136**, **descent_k3_clusters27**, **descent_k3_clusters37**, **descent_k3_lines116**,
  **descent_k3_ring27**, **descent_k3_ring111** : les six nuages du § 17.4, jugés conformes par l'oracle Γ_K, avec
  descentes de 3 et 4 sauts. Coordonnées gravées, translatées
  dans u18 (la famille « lignes » produit une ordonnée −1). Ce sont des générateurs déterministes, sans aucun octet
  KITTI. On y ajoute les K-parties exactes qui sautent 3 ou 4 fois, avec leur chaîne de niveaux attendue ;
- **actual_equal_radius_descent**, requalifiée. Côté tour, c'est une descente ordinaire ; attendu : 0 pas à rayon
  égal. Côté J-DESC, elle teste l'échange d'intrus (pas à rayon égal, σ décroissant), ce qu'elle testait en v9 ;
- **amputated_{A_E, square, E5}**, en mode certifié (kcat = Kmax + 2) : chaque boule admissible est retirée tour à
  tour. Pour chaque retrait, trois issues sont possibles :
  - un refus `catalogue_incomplete/*`, ce qui est l'attendu ;
  - une sortie égale à l'oracle, ce qui est compté : c'est un retrait sans effet sur l'objet ;
  - une sortie publiée différente de l'oracle. C'est une faute de la zone aveugle ; elle devient une fixture et une
    ligne du registre avant de continuer.

### 13.3 Mutants et variantes neutres

Les mutants sont des copies mutées au configure, jamais des branches du produit. Code 4 attendu.

| Mutant | Faute simulée | Tué par |
| --- | --- | --- |
| `window_lo` | fenêtre basse `p + q` | pair |
| `mat_no_plateau_grouping` | chaque jonction ouvre son nœud (multifusion binarisée) | carré K2, E5 |
| `drop_continuations` | contributions de bloc omises | ABCZ |
| `first_rep_only` | arêtes du seul premier représentant | pair |
| `vertical_open` | image à la coupe ouverte | inert_ball |
| `wa_open_as_closed` | WA ignore la coupe ouverte | pair |
| `wa_argmax_left_bound` | WA prend la jonction bornante au lieu de l'argmax intérieur | T2, carré |
| `knn_ignores_weight` | saut K-NN sans les copies | T2 pondérée |
| `knn_from_support_site` | saut K-NN centré sur un site du support au lieu de c | fixtures de descente du § 13.2 (I3 ou T2) ; il survit à 15 des 16 fixtures de la v1 (§ 17.4) |
| `knn_tiebreak_pointid` | départage des plus proches par `PointId` | porte `--relabel` (équivariance) |
| `pointer_nonstrict` | pointeur accepté à rang égal | I4 |
| `lookup_misses_extended` | la recherche ignore les coquilles étendues | carré, triangle rectangle (I5) |
| `i5_disabled` | refus I5 sauté | catalogues amputés (T2) |
| `euler_skip_last` | Euler omet l'ordre kcat − 2 | contre-exemple Euler/Kmax+2 |
| `euler_shell_ignores_weight` | coquille pondérée comptée à poids 1 dans Euler | weighted_euler, T2 pondérée |
| `circle_link_nonconsecutive` | liaison entre fenêtres non consécutives oubliée ou ajoutée | plane_lattice_*, quotient brut |
| `class_union_missing_S` | règle d'union sans la classe S | T2 pondérée |
| `hk_no_verify` | H_K comparé sur l'empreinte seule (empreinte faible dans la copie mutée) | T2 |
| `kernel_junction_on_head` | jonction déposée sur la tête de rb au lieu de la queue de ra | EMST, T2 |
| `list_rank_shift` | décalage d'une position dans le placement | EMST, T2 |
| `mat_successor_max` | successeur choisi sur la plus grande jonction bornante | T2, I7 |
| `entry_without_self` | D_K(x) sans x | oracle C∩X, cohérence points–verticales |
| `contrib_open_cut` | contribution déposée à la coupe ouverte | ABCZ (couverture) |
| `census_drop_u_site` / `census_drop_i_site` | recensement du générateur amputé d'un site (copie de GEN) | J-CENSUS |

**Variantes neutres**, qui doivent rester **identiques** (porte d'équivalence) :
- sans H_K ;
- `dc1`, `dc2` (option D&C) ;
- `pot` (union-find à potentiels) ;
- `lotkruskal`, le Kruskal par lots de référence (ci-dessous) ;
- `checks=full` contre `checks=sampled` ;
- W ∈ {1, 2, 8, 48}.

**Porte d'équivalence T2** (demandée par la critique B2) :
- le Kruskal par plateaux par lots, séquentiel (celui de la v1, § 17.1), est gardé **dans `tests/`** comme
  référence ;
- sur chaque entrée des portes d'échelle et des trames, la liste canonique des nœuds (rang, plus petite feuille,
  nombre de feuilles, nombre de parents, clé du successeur) doit être égale à celle du produit ;
- coût : 0,1–0,3 s par ordre, hors chronomètre.

### 13.4 Juges d'échelle

| Juge | Principe | Tailles, planchers |
| --- | --- | --- |
| K1 = EMST | forêt K1 = dendrogramme à plateaux d'un EMST entier indépendant (`tower_merkle_v10` restreint à K1) | uniforme, terrain, huit amas 8k/16k/32k ; 3 trames sans sol ; 1 brute |
| Euler | I2 à `kcat = Kmax + 2`, **pondéré compris** | 8k/16k/32k K5 ; trames K5 et K10 ; suites publiques pondérées |
| **J-CENSUS** (M2) | recensement indépendant, décrit ci-dessous | ≥ 20 000 boules par (entrée, K) à 8k–32k ; ≥ 50 000 par trame ; toutes les strates (q, p) non vides |
| **J-DESC** (M3) | descente indépendante par échange d'intrus, stratifiée par nombre de sauts, décrite ci-dessous | trames K5 : ≥ 2 000 à ≥ 1 saut, ≥ 500 à ≥ 2, ≥ 200 à ≥ 3 ; K10 : idem, plus ≥ 20 à ≥ 5 ; synthétiques : histogramme publié, planchers fixés à la première mesure, jamais baissés |
| **J-MF** (M6) | certificats de multifusion, décrits ci-dessous | ≥ 1 000 fusions par ordre et par entrée, en strates de 2, 3 et ≥ 4 parents ; ≥ 1 000 cellules non fusion |
| cohérence points–verticales | pour **tous** les sites et tous les K ≥ 2 : `WA_{K−1}(image(entry_K(x)), D_K(x)) = WA_{K−1}(entry_{K−1}(x), D_K(x))` | toutes (O(n·K)) |
| local borné | BFS sur Γ_K(a) depuis N_K(x), arrêt à 5 000 K-parties (v1) | 8k/16k/32k ; trames |
| équivalence T2 | § 13.3 | toutes |
| structure | I6, I7, I10 rejoués hors produit ; validateur structurel complet | toutes |
| déterminisme | `tower_digest_v10` égal à W1/W8/W48 et sur entrée permutée ; égal après bijection si les `PointId` sont renommés | 8k ; trames |
| budget | pic compté ≤ 100 o/boule ; ≥ 85 % du pic RSS | 8k/16k/32k ; trames |

**J-CENSUS : juge indépendant du recensement.**
- **Échantillon** : les boules avec `mix64(BallIdx ⊕ sel_juge) mod 64 = 0`, plus **toutes** les boules non
  régulières.
- **Structure propre au juge** : une grille uniforme sur les sites, de pas égal à 4 fois la distance médiane au plus
  proche voisin, écrite dans `tests/oracle/`. Elle ne partage rien avec l'oracle de feuilles.
- **Calcul** :
  - centre homogène recalculé depuis S* par la formule de l'oracle (i256 / BigInt) ;
  - boîte flottante de la boule, élargie d'une unité ;
  - puissance exacte de chaque site des cellules touchées.
- **Égalités exigées** : multiensembles I et U (poids compris), q (énumération brute si u ≤ 8, table sinon),
  fenêtre.
- **Coût** : ≈ 2–5 µs par boule (estimé).
- **Mutants** : `census_drop_u_site`, `census_drop_i_site`.

**J-DESC : juge de descente.**
- **Échantillon.** Le produit enregistre, dans un crochet de test, le nombre de sauts de chaque représentant. On tire
  dans chaque strate (≥ 1, ≥ 2, ≥ 3, ≥ 5 sauts), de façon déterministe.
- **Descente indépendante** : l'échange d'intrus v9 est réécrit dans `tests/oracle/descent_v9.hpp`.
  - Tant que MEB(F) n'est pas une cellule de fenêtre d'ordre K, on remplace le premier site du support (par
    `SiteIdx`) par une copie strictement intérieure absente de F, trouvée dans la grille du juge.
  - La décroissance est lexicographique (λ, σ). Les pas à rayon égal y sont possibles et attendus : ils sont
    exercés par `actual_equal_radius_descent`.
  - MEB par énumération exhaustive des supports en BigRat, avec au plus 781 supports pour 12 points.
  - Terminal : `T_ind`.
- **Contrôle** : `WA(anchor(T_ind), r − 1) = WA(leaf_node[Min(resolve(ρ))], r − 1)`, où r est le rang de la cellule du
  représentant ρ. La composante pré-lot doit être la même.
- **Coût** : 0,5–2 ms par représentant (estimé).
- **Disponibilité sur 08/000200** (critique, par extrapolation des échantillons) : ≈ 36 000 représentants à ≥ 3 sauts
  à K10, ≈ 4 800 à K5. Les planchers sont donc tenables.

**J-MF : certificats de multifusion.**

Pour une fusion v tirée (ordre K, rang r, parents P_1..P_m), on prend les cellules du lot ancrées sur v et, pour
chacune, tous ses représentants avec leur descente J-DESC et leur composante pré-lot `C_ρ = WA(anchor(T_ind), r − 1)`.
On vérifie :
- (a) chaque `C_ρ` appartient à {P_i}. Tue une mauvaise attribution de parent ;
- (b) le graphe sur {P_i}, où chaque cellule relie tous ses `C_ρ` (PO-T1), est connexe. Tue une fusion parasite ;
- (c) chaque P_i est atteint. Tue un parent en trop.

Pour une cellule non fusion tirée (continuation ou bloc inerte) :
- (d) tous ses `C_ρ` ont la même composante à la coupe fermée r, égale à son ancre. Tue une fusion manquante.

La géométrie est indépendante (descentes, arithmétique, grille) ; seuls les noms de composantes viennent de la tour, et
(a)–(d) les attaquent des deux côtés. Coût estimé : 10–20 s par entrée.

### 13.5 Différentiel v9

Inchangé depuis la v1, § 12.5 :
- tour seule sur le catalogue v9 exporté, puis chaîne complète, par `tower_merkle_v10` ;
- entrées : uniforme, terrain, huit amas 8k/16k/32k, et les trames sans sol K5/K10 ;
- toute différence devient une fixture minimale et une ligne du registre avant de continuer.

La v9 ne transporte ni doublons ni coquilles > 12. Ces cas relèvent de la T2, d'Euler pondéré, de J-CENSUS,
de J-DESC et de J-MF.

### 13.6 Suites publiques (préalable aux bancs d'EVAL)

Avant tout banc, pour chaque suite publique quantifiée (SIPU, FCPS, etc., à la quantification d'EVAL), on exécute
catalogue et tour, puis on publie :
- `circle_balls` et le u maximal circulaire ;
- `shell_max_3d` non circulaire ;
- le nombre de sites de poids ≥ 2 ;
- le statut.

Toute coquille 3D non circulaire à u > 16 est déclarée **à l'avance** comme refus attendu : jamais une défaite
silencieuse. Les suites SIPU et FCPS en z = 0 passent entièrement par le chemin circulaire.

### 13.7 Campagnes de performance

Protocole d'EVAL § 12 et d'ARCH § 15 : sonde unique, reçu ancré au commit, mur externe, passes entrelacées. Chaque
passe publie par étage et par ordre :
- `meb_calls`, `jumps_hist`, `hk_hits`, `pointer_rounds`, `edges` ;
- `t2a_ms`, `t2b_ms`, `t2c_ms`, `wa_queries` ;
- le pic compté et le mode `checks`.

Les leviers se mesurent en A/B appariés sur G4, avec reçu :
- ANSV par blocs dans T2c ;
- jointure triée pour H_K ;
- option D&C d ;
- potentiels contre classement de liste.

### 13.8 CI

T2 ≥ 200 nuages (≤ 90 s), fixtures, mutants, équivalences W1/W2, équivalence T2 et `dc1` sur les nuages de la T2 :
moins de 3 min des 10 min de la CI d'ARCH. Les juges d'échelle vont dans les labels `scale*` et `lidar`, exécutés en
local et sur G4 avec reçu.

---

## 14. Voie GPU (après V10-4a, ARCH P14)

- **Sur l'appareil** : G, M, T1, T2b, T2c et Q.
  - G : file de représentants à fils persistants ; H_K en tableaux triés ; MEB en double puis vérification entière ;
    `lookup` et `knn_closed` sur l'oracle de feuilles résident ; repli CPU par élément, compté.
  - T2b et T2c : classement de liste, descentes dans `jblock`, tri par base des clés canoniques.
- **Sur l'hôte : T2a** seulement.
  - Les arêtes de l'ordre descendent sur l'hôte : 12 o par arête, ≈ 22 Mo à K10, ≈ 1–2 ms en PCIe 5 (estimé).
  - Les listes NJ remontent : 8 o par feuille.
  - Les ordres inférieurs continuent sur l'appareil pendant le noyau de l'ordre Kmax.
- **Projection, pas promesse** : tour K5 ≈ 20–35 ms (G ≈ 8–15, T2a visible 4,5–12, T2c et Q 3–6) ; K10 ≈ 50–100 ms.
  La v1 annonçait 15–25 et 50–80 ms, avec un T2 sous-estimé : la critique avait raison.
- **Pour K10 sous ≈ 60 ms** : option D&C d = 1–2 sur les 48 fils CPU, libres pendant que le GPU travaille ; ou
  dendrogramme par contraction d'arbre sur l'appareil (PANDORA). Pistes V10-x, sur mesure.
- **Porte** : CPU = GPU au `tower_digest_v10` sur les trames et sur 8k/16k/32k.

---

## 15. Plan d'implémentation

### 15.1 Fichiers (`morsehgp3D_v10/src/tower/`, estimé ≈ 2 700 lignes, aucun fichier > 400)

| Fichier | Contenu | Lignes |
| --- | --- | ---: |
| `atlas.hpp/.cpp` | fenêtres, cellules, `by_k`, boules de rayon nul, poids saturés | 200 |
| `shell_table.hpp/.cpp` | `ShellTable` 3D (port de `local_plateau.hpp`), classes pondérées O(u·2^u) | 350 |
| `circle_shell.hpp/.cpp` | ordre angulaire exact, fenêtres, quotient, Euler clos | 220 |
| `validate.cpp` | structure (toujours), géométrie (`full` / `sampled`), Euler pondéré | 220 |
| `birth_index.hpp/.cpp` | H_K | 150 |
| `meb.hpp/.cpp` | MEB exacte ≤ 12 points (port du noyau v9) | 300 |
| `resolve.hpp/.cpp` | représentants, résolveur, pointeurs, histogramme des sauts | 260 |
| `minima.cpp` | sauts de pointeurs par ordre, feuilles | 100 |
| `edges.cpp` | hyperarêtes en étoile, triées | 120 |
| `t2_kernel.cpp` | noyau T2a | 90 |
| `list_rank.cpp` | classement de liste parallèle (et variante à potentiels, en test) | 150 |
| `cartesian.cpp` | T2c : `jblock`, jonctions ouvrantes, intervalles, successeurs, parents, numérotation canonique | 300 |
| `interval_index.hpp/.cpp` | WA sur `jblock` | 120 |
| `marks.cpp` | ancres, contributions, verticales, naturalité, C∩X, localisateur | 240 |
| `tower.hpp/.cpp` | orchestration par ordre, modes, statuts, budget | 180 |

**Tests** (`tests/tower/`, ≈ 2 800 lignes) :
- oracle T2 pondéré et plan, `check_cut`, fixtures ;
- juges J-CENSUS, J-DESC (échange d'intrus, grille propre), J-MF ;
- EMST, cohérence points–verticales, local borné ;
- Kruskal par lots de référence ; option D&C (`dc_split.cpp`, test seulement tant que D-T6 ne l'adopte pas) ;
- exporteur v9.

Les prototypes Python (`tower_v10_*.py`, `euler_weighted_check.py`, `circle_quotient_check.py`,
`weighted_table_check.py`) restent le second langage.

### 15.2 Jalons (dans la phase V10-2 d'ARCH)

0. **T-0**, pendant la session G4 C0 d'ARCH (V10-0) : `bench/proto/pdendro.cpp` (ce spike, commis) sur les vidages
   cble des 3 trames, à W1, W24 et W48. Il publie T2a, le classement de liste, T2c et `pot`. Sorties : ρ pour T2a et
   S48 pour T2b/T2c. Décisions D-T5 et D-T6 écrites avant le code produit de T2.
1. **T-1** : atlas, tables (circulaire, 3D, classes), validation, Euler pondéré, H_K.
   Porte :
   - Euler et comptes de cellules égaux à la v9 sur les catalogues exportés ;
   - tables contre le quotient brut (portage C++ des trois scripts du § 17) ;
   - fixtures planes.
2. **T-2** : MEB, résolveur, pointeurs, minima.
   Porte :
   - T2 réduite « pré-lot » ;
   - I3, I3b, I4 et I5 verts ;
   - histogramme des sauts sur 08/000200 K5 et K10, qui doit reproduire la critique à ± 5 % ;
   - J-DESC local sur 08/000200 K5.
3. **T-3** : T1, T2a, T2b, T2c, Q, marques, verticales, digests.
   Porte :
   - T2 complète, avec ses planchers ;
   - équivalence T2 ;
   - fixtures et les 22 mutants de la tour ; les deux mutants de recensement relèvent de T-4 (J-CENSUS) ;
   - W1/W8 ; `dc1`/`pot` neutres.
4. **T-4** : échelle.
   Porte :
   - EMST, Euler à kcat = Kmax + 2 ;
   - J-CENSUS, J-DESC et J-MF avec leurs planchers ;
   - cohérence points–verticales, local borné ;
   - différentiel v9 (tour seule, puis chaîne) ;
   - budget mémoire ;
   - recensement des suites publiques.
5. **T-5** (V10-4a) : leviers mesurés sur G4 (A/B appariés, reçus). Adoption, ou fausse piste écrite.
6. **T-6** (V10-4b) : voie GPU (§ 14).

---

## 16. Risques et décisions

### 16.1 Risques

- **R1 — L'étage G domine** (65 à 75 % de la tour).
  - Le saut K-NN n'a pas de borne de pas prouvée. Mesuré par la critique : au plus 4 sauts à K5 et 6 à K10 sur
    08/000200.
  - Les coûts unitaires de G sont estimés, pas mesurés.
  - Parades : GPU ; jointure triée pour H_K ; mesure dès T-2.
- **R2 — Rendement parallèle de T2b/T2c inconnu sur G4.** Localement, sur l'hôte chargé, T2c passe de W1 à W4 avec
  un facteur 2,8–3,6, et le classement de liste de mon prototype ne passe pas à l'échelle. Parades : variante à
  potentiels (placement mesuré ×3 de W1 à W4) ; ANSV par blocs ; décision en T-0 sur G4.
- **R3 — Noyau T2a mesuré sur arêtes semées seulement.** Il manque ≈ 30 % des arêtes ; le ×1,3 est une hypothèse.
  Mesure réelle à T-3 sur 08/000200. Sur CPU le noyau est recouvert ; sur GPU il est visible.
- **R4 — Coquilles 3D non circulaires à u > 16** (données de réseau 3D) : refus par boule. Le chemin circulaire
  couvre les entrées planes. L'arrangement de grands cercles Λ_t reste une piste V10-x.
- **R5 — Zone aveugle de complétude** : § 8.4. Seuls le mode certifié et les juges la couvrent.
- **R6 — Différentiel v9** : il faut relancer la v9 par entrée (huit amas 32k ≈ 5 min ; uniforme 32k K10 : plusieurs
  minutes et ≈ 10 Go). Coût accepté une fois.
- **R7 — Descentes longues de représentants** : les fixtures à 3 et 4 sauts (§ 17.4) portent sur des K-parties, pas
  sur des représentants. Le régime « représentant à ≥ 3 sauts » du LiDAR n'est exercé que par J-DESC sur les trames
  réelles, relues localement et jamais versionnées ; leur empreinte figure dans le reçu.
- **R8 — Coût de J-DESC et J-MF** (≈ 10–20 s par entrée, en BigRat) : acceptable dans les portes, exclu de la CI.

### 16.2 Décisions tranchées

| Id | Décision | Raison |
| --- | --- | --- |
| D-T1 | Résolveur pur par saut K-NN ; I3 **strict** en λ | PO-T4 corrigé : chaque saut décroît strictement |
| D-T2 | H_K par consultation triée (100–200 ns). Levier mesurable : jointure triée sur empreinte additive `H(X) = Σ mult·h(site)` (empreinte d'un représentant = `H(pop(b)) − h(s)` en O(1)), avec **vérification exacte** des paires appariées | exact et déterministe ; la jointure réduirait ≈ ×1,5–2 le poste le plus lourd de G (estimé) ; pas de code non mesuré |
| D-T3 | Minima fixes, **par ordre** | pointeurs internes à l'ordre ; supprime l'attente globale |
| D-T4 | Hyperarêtes en étoile entre minima ; aucune pour un bloc à un seul minimum | inchangé |
| **D-T5** | T2 = noyau arête par arête **sans lots**, puis classement de liste, puis matérialisation cartésienne ; variante à potentiels en repli | noyau séquentiel mesuré 3 à 6 fois plus rapide que le Kruskal par lots (parcours final des listes déduit), sortie identique (§ 17.1) ; exactitude par PO-T18 |
| **D-T6** | Aucun préfiltre MSF, aucune renumérotation Morton. Option D&C (d ≤ 2) seulement si une mesure G4 montre T2a(Kmax) visible au-delà du budget (voie GPU) | leviers mesurés nuls ou négatifs (critique M5) ; D&C mesuré 5–10 fois le travail du noyau et du parcours, sans gain à W4 |
| D-T7 | Index d'intervalles avec `jblock` (blocs de 16) | ≈ 20,6 o par feuille au lieu de ≤ 32 ; descentes d'une ligne de cache |
| D-T8 | Images de fusion par une naissance descendante ; naturalité complète en `full`, 1/16 en `sampled` | aucune dépendance entre nœuds ; plan de tests § 3.2 |
| D-T9 | Ancres publiées ; entrées C∩X avec `entry_pos` et `entry_eq` ; localisateur de facettes | contrat CLUSTER_v2 § 4.1.5 |
| **D-T10** | Numérotation canonique (rang, plus petite feuille du sous-arbre) | indépendante de π et de l'algorithme de T2 ; parallèle ; stabilise les épingles |
| D-T11 | Digest linéaire sans données d'index ; Merkle inchangé ; PO-E15 acceptée | objet seulement |
| **D-T12** | Validation structurelle toujours ; géométrique complète en `full`, 1/64 en `sampled` | la structure protège la mémoire ; la géométrie re-vérifie un théorème |
| **D-T13** | Coquilles : fenêtres circulaires (tout u), table 3D u ≤ 16, classes pondérées O(u·2^u) ; refus par boule seulement en 3D non circulaire au-delà de 16 | PO-T20, PO-T21 ; entrées z = 0 entièrement couvertes |
| **D-T14** | Multiplicités natives, **Euler pondéré actif** | PO-T19 |
| D-T15 | I5 est un refus de produit, déclaré **non indépendant** | mêmes listes que le générateur |
| D-T16 | Pas de dédoublonnage des représentants | la critique confirme : ×1,13–1,19 seulement |
| D-T17 | Transitoires libérés par ordre ; au plus la moitié des ordres entre T1 et T2c ; Kmax en tête | pic ≤ 100 o par boule sans allonger le chemin critique |
| **D-T18** | Trois juges d'échelle indépendants (J-CENSUS, J-DESC, J-MF), portes seulement | M2, M3, M6 |
| **D-T19** | Modes `checks=full` / `checks=sampled`, sortie identique | latence contre jugement ; la sortie ne dépend pas du mode |
| **D-T20** | `pw` et `uw` en u8 saturés à kcat + 1 ; poids par site en u32, W en u64 | débordement u16 signalé par la critique |

### 16.3 Constats mineurs de la critique

| Constat | Verdict | Traitement |
| --- | --- | --- |
| Étage G confirmé (semis 72–78 %, dédoublonnage ×1,13–1,19) ; argument « comptes v9 comme borne haute » invalide (1,29 M = `anchor_hits` + `intruder_queries`) | fondé | argument retiré ; volumes de G repris de la critique (§ 12.2) |
| Euler étendu confirmé ; K ≤ kcat − 2 est la bonne borne | confirmé | § 6.1 ; justification au § 9.3 |
| `jtree` jusqu'à 16 o par feuille ; index 19–26 o par boule ; écart avec ARCH | fondé | `jblock` ; alignement sur ARCH_v2 § 5.6 (§ 12.3) |
| Table pondérée en O(4^u) si on lit la v1 littéralement | fondé | classes de supports en O(u·2^u), vérifiées (PO-T21) |
| `pw`, `uw` en u16 | fondé | D-T20 |
| D-T12 et I8 re-vérifient des théorèmes | fondé | modes `full` / `sampled` (§ 8.2) |
| Étage M global | fondé | M par ordre, Kmax en tête (§ 11.1) |
| Mutants manquants | fondé | § 13.3. Le mutant Borůvka est sans objet, le préfiltre étant supprimé |
| Coût H_K 100–200 ns | fondé | § 5.5, § 12.2 |
| `pos` ambigu ; drapeau I mort ; question 4 mal posée | fondé | `leaf_lo` et positions de π d'un côté, `entry_pos` pour les rangs de l'autre ; drapeau retiré (§ 3.6) ; question remplacée (§ 18) |
| WA à 71–75 ns ; neutralité MSF ; groupes v9 ; fenêtre basse pondérée | confirmés | repris |

---

## 17. Mesures et contrôles de conception (`tower_v2_spike/`)

Ce sont des expériences rejouables, pas des reçus de phase. Elles entreront dans `bench/proto/` et `tests/` du dépôt au
jalon T-0 ; les empreintes des fichiers sont dans `tower_v2_spike/SHA256SUMS`, et les commandes exactes avec leurs
sorties dans `tower_v2_spike/RUNS.txt`.

**Hôte.** Codespace EPYC 7763 (Zen 3), 8 vCPU. Il est partagé avec d'autres sessions : la moyenne de charge valait
6 à 16 pendant les mesures. Les rapports entre variantes d'une même passe sont fiables ; les temps absolus sont
bruités ; le passage à l'échelle de W1 à W4 est **sous-estimé**.

**Entrées.**
- Sites : 08/000200 sans sol, 45 845 sites en grille de 1 mm. Fichier local
  `morsehgp3D_v9/audits/s4a_cpu_scene02_physical_panel_20260924/inputs/full_full.u32le` ; aucun octet n'est copié
  dans le dossier de conception.
- Catalogues : vidages `cble_dump` de la critique (`lidar02_K5.cat`, 1 407 885 boules ; `lidar02_K10.cat`,
  5 483 320 boules), régénérables par `crit_TOWER/cble_dump`.

### 17.1 Spike T2 : `pdendro.cpp`

**Structure mesurée.** C'est l'ordre Kmax, construit exactement comme dans `crit_TOWER/t2_bench.cpp` :
- feuilles : naissances à p + u = K ;
- arêtes en étoile des représentants **semés** ;
- niveaux flottants rangés.

La critique l'estime à ≈ 30 % d'arêtes en moins que la réalité.

**Voies comparées, sur l'objet.** L'objet comparé est la liste canonique des nœuds : (rang, plus petite feuille,
nombre de feuilles, nombre de parents, clé du successeur).
- `seq` : Kruskal par plateaux par lots (union par taille, DSU local par lot). C'est la référence, et sa sortie
  enregistre directement les multifusions ;
- `lean` : noyau T2a, suivi du parcours des listes puis de la matérialisation cartésienne ;
- `list_rank` : classement de liste parallèle ; on exige π et J bit à bit égaux à ceux du parcours ;
- `pot` : union-find à potentiels puis placement parallèle ; même exigence ;
- `dc` : diviser pour régner par rangs, avec pour cas de base `seq` ou `lean` ;
- `mat` : matérialisation depuis le (π, J) du `seq`.

| Ordre Kmax de 08/000200 | K5 | K10 |
| --- | ---: | ---: |
| feuilles / arêtes semées | 361 211 / 566 911 | 937 257 / 1 380 911 |
| lots à arêtes / dont à une seule arête | 360 784 / 188 476 (52,2 %) | 827 933 / 386 081 (46,6 %) |
| nœuds / fusions / fusions à ≥ 3 parents | 614 700 / 253 489 / 87 137 (34,4 %) | 1 549 091 / 611 834 / 245 102 (40,1 %) |
| racines (arêtes non semées absentes) | 5 212 | 23 185 |
| `seq`, Kruskal par lots (parcours final des listes compris) | 27,4–46,9 ms | 76,8–127,5 ms |
| **`lean`, noyau T2a, préchargement 16** | **6,6–9,2 ms** | **15,9–22,3 ms** |
| `lean` sans préchargement | 7,4–10,8 ms | 27,8–35,8 ms |
| parcours séquentiel des listes | 3,9–5,4 ms | 14,5–25,7 ms |
| `list_rank`, W1 / W4 | 9,8–10,4 / 11,4–22,9 ms | 33,8–35,7 / 15,3–37,4 ms |
| `pot` : noyau ; placement W1 / W4 | 10,2–11,4 ; 5,2–5,3 / 1,8 ms | 24,4–26,4 ; 14,2–14,3 / 7,0–10,6 ms |
| matérialisation cartésienne, W1 / W4 | 35,6–47,8 / 12,8–17,2 ms | 95,9–131 / 35,2–38,3 ms |
| `dc` (B de 8 Ki à 256 Ki, cas de base `seq` ou `lean`), W1 / W4 | 110–133 / 47–113 ms | 194–387 / 93–203 ms |
| **égalités** (`dc`, `mat`, `lean`, `list_rank`, `pot` contre `seq`) | **toutes vraies, dans toutes les passes** | **toutes vraies, dans toutes les passes** |

**Variante à plateaux grossis** (`--grid`, niveaux divisés par 256 et arrondis). Elle donne 16 371 lots, dont
63 % à plusieurs arêtes, et 50 % de fusions à ≥ 3 parents. Les mesures, à K5 et W4 :
- `seq` : 116–120 ms ;
- `lean` : 7,5 ms ;
- matérialisation : 11,6–20,9 ms ;
- toutes les égalités sont vraies.

Lecture : plus les plateaux sont gros, plus le Kruskal par lots coûte, alors que le noyau ne voit pas les plateaux.

Commande type (compilation `-std=c++20 -O3 -march=native -fopenmp -Wall -Wextra -Werror`) :

```text
pdendro <sites.u32le> <catalogue.cat> <K> --threads=W --reps=7 --B=65536 [--grid] [--lean-base]
```

Le code de sortie vaut 0 si toutes les égalités tiennent, 1 sinon.

### 17.2 Euler pondéré : `euler_weighted_check.py`, `euler_weighted_mutant.py`

- Méthode : énumération exacte (Fractions) de **toutes** les boules critiques de nuages pondérés dégénérés, soit
  toutes les sphères de supports de 2 à 4 positions dont le centre est dans l'enveloppe fermée. Puis contrôle de
  l'identité du § 6.1 pour tous les K ≤ W + 1.
- Nuages : n = 3–7, grilles 2–6, 30 % plans, poids 1–4.
- Résultats :
  - `euler_weighted_check.py 40 1` : 40 nuages, 605 boules, 392 contrôles, 0 échec ;
  - `euler_weighted_check.py 300 1000` : 300 nuages, 4 141 boules, 2 878 contrôles, 0 échec ;
  - mutant « coquille comptée sans poids » : 40 nuages en échec sur 40.

### 17.3 Tables locales : `circle_quotient_check.py`, `weighted_table_check.py`

Référence dans les deux cas : `tower_v10_weighted.local_rank`, qui énumère les multiensembles.

- **Coquilles circulaires** (quotient par fenêtres, Euler clos) :
  - `circle_quotient_check.py 1 40` : 2 637 contrôles de quotient et 853 d'Euler, u ≤ 9, 0 échec ;
  - `circle_quotient_check.py 1000 200` : 12 190 et 3 960, 0 échec.
- **Classes pondérées** : `weighted_table_check.py 1 150` : 11 852 contrôles, u ≤ 8, 0 échec.

### 17.4 Recherche de descentes longues : `long_descent_search.py`, `long_descent_any.py`

- `long_descent_search.py 1 60 0` : 60 mini-nuages de balayage, n = 10–13, Kmax = 3–10, construction de la tour
  seule. Histogramme des sauts par résolution : 0 saut 13 727, 1 saut 262, 2 sauts 3. Aucun nuage à ≥ 3 sauts.
- `long_descent_any.py {clusters, ring, lines} 1 200 3` : toutes les K-parties (2 ≤ K ≤ Kmax ≤ 6) de 200 nuages par
  famille, n = 9–13. Une première version bouclait sur les singletons (K = 1, hors du résolveur du prototype) ; elle a
  été corrigée, et ses journaux vides ont été écrasés. Histogrammes des sauts par K-partie :
  - amas : 0 saut 27 301, 1 saut 131 579, 2 sauts 11 501, **3 sauts 1 747, 4 sauts 139** ; 13 nuages à ≥ 3 sauts ;
  - anneau autour d'un cœur : 26 194, 139 829, 6 205, **39** ; 2 nuages ;
  - lignes : 23 085, 147 506, 1 675, **1** ; 1 nuage.
- **Jugés par l'oracle Γ_K du prototype** (`run_case`), tous conformes, 23 387 contrôles de composantes au total :
  - amas : graine 136 (n = 13, Kmax = 6, 4 sauts, 6 269 contrôles), graine 27 (n = 12, Kmax = 6, 3 sauts, 4 487),
    graine 37 (n = 13, Kmax = 6, 3 sauts, 3 581) ;
  - lignes : graine 116 (n = 13, Kmax = 3, 3 sauts, 2 459) ;
  - anneau : graine 27 (n = 12, Kmax = 6, 3 sauts, 4 841), graine 111 (n = 10, Kmax = 4, 3 sauts, 1 750).

  Les coordonnées sont dans `long_any_*.log`.
- `jump_mutants_check.py` (`jump_mutants.log`) : mutants du saut sur les 16 fixtures du prototype, puis sur ces nuages.
  Le nuage « lignes 116 », translaté dans u18, est rejugé conforme (2 459 contrôles).
  - `knn_from_support_site` (plus proches pris autour d'un site du support au lieu du centre) : il **survit à 15 des
    16 fixtures de la v1** ; seule `actual_equal_radius_descent` le tue. Il est tué par les **quatre** nuages à
    descentes longues essayés : I3 sur amas 136, le juge Γ_K sur lignes 116, anneau 27 et anneau 111. C'est la
    démonstration du constat M3 : les fixtures de la v1 n'exerçaient pas le saut ;
  - `knn_k_minus_one_plus_far` (K − 1 plus proches, plus le point le plus lointain de la boule fermée) : il survit
    partout. Ce n'est pas une faute : toute K-partie de la boule fermée dont la MEB est strictement plus petite reste
    dans la composante (PO-T1, PO-T4). Le choix exact de N relève du déterminisme et de l'équivariance
    (mutant `knn_tiebreak_pointid`, porte `--relabel`), pas de la T2.
- Lecture : aux petites tailles, les longues descentes viennent d'amas serrés entourés de points épars. La boule
  minimale d'une K-partie étalée contient alors beaucoup de points intérieurs, et chaque saut vers les K plus proches
  resserre sur un amas. Ce régime est proche de celui des trames LiDAR, où la surface est dense et les objets épars.

---

## 18. Questions ouvertes pour l'utilisateur

1. **Mode certifié** `kcat = Kmax + 2` (Euler à tous les ordres publiés, catalogue ≈ ×1,4 à K10) : par défaut pour
   les campagnes, ou seulement pour les références jugées et les reçus de qualification ?
2. **Mode de contrôle du chronomètre** : `checks=sampled` (validation géométrique 1/64, naturalité 1/16, sortie
   identique à `full`) comme défaut du produit, `full` étant réservé aux portes et aux références. Est-ce acceptable ?
3. **K10 à 100 ms** : ce n'est pas un contrat de cette conception. Sur GPU, le noyau T2a de l'ordre 10 reste visible
   (≈ 10–29 ms estimés). Si ce contrat devient prioritaire, faut-il planifier dès V10-4b l'option D&C (CPU pendant
   le GPU) ou un dendrogramme GPU par contraction d'arbre ?
4. **Numérotation K1** : ordre géométrique (`SiteIdx`, Morton des positions, proposé) ou ordre des `PointId` comme en
   v9 ? L'exporteur de compatibilité restitue l'ordre v9 dans les deux cas.
5. **Portes sur trames réelles** : les planchers J-DESC des représentants à ≥ 3 sauts reposent sur les trames
   SemanticKITTI, relues localement et jamais versionnées. Les fixtures synthétiques à 3 et 4 sauts ne portent que sur
   des K-parties. Accepte-t-on qu'une porte de phase dépende de fichiers de trames présents sur la machine, avec leur
   empreinte publiée dans le reçu ?
