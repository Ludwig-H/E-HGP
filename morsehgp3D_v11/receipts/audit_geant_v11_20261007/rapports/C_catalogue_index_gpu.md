# Rapport C — Étage `domain` (catalogue Cat_K), index et voie GPU de la v11 : audit de clôture

7 octobre 2026. Lecteur C (un des huit lecteurs indépendants). Lecture seule de `/workspaces/E-HGP` à `e968aba8d`
(moteur gelé `ac081a06f`, identique à `733912e65`). Aucune compilation, aucune mesure, **GCP non utilisé**.

```text
phase=exploration_v11_hors_registre (close)
backend=cpu_reference ; voie de banc cuda_g4 (lot de feuilles)
profile=quantized_u21_input_only
public_status=not_claimed
```

**Légende.** Sans marque : vérifié dans le code (chemin:ligne, relatif à `morsehgp3D_v11/` sauf mention), dans un
reçu (chemin) ou dans un commit (SHA). **[I]** : inférence de ma part. **[E]** : estimation reprise d'une note de
travail, jamais une mesure. Les temps sont des mesures G4 (W48, trames sans sol ng00/ng01/ng02 = 39 885 / 35 551 /
45 845 sites, grille 1 mm), sauf mention « local ».

---

## 0. Synthèse

1. **L'étage `domain` est juste et bien compris** : partition de l'espace des centres par boîtes demi-ouvertes,
   listes K-certifiées (G1), énumération locale des supports q2/q3/q4 dans des feuilles de ≤ 32 sites, émission du
   seul support canonique S*, tri exact (clés F3/F4 + repli exact). La voie GPU est exacte par construction
   (chemins i128 certifiés, sinon `unresolved` rejoué par `leaf.cpp` avant admission) ; zéro feuille non résolue sur
   les trames.
2. **Au gel (ng00, K5, à chaud)** : 138 ms en voie GPU `868347:400`, 199 ms en voie CPU `802811`. **Fait que
   l'audit final ne met pas en avant : la frontière et les étages de fin coûtent à eux seuls 48,6 ms en voie GPU
   (46,9 ms en voie CPU)**, soit déjà le budget de 40–50 ms visé pour la v12, même avec un parcours et des feuilles
   gratuits. À K10 : 141 ms sur 490.
3. **Les étages de fin sont linéaires dans le nombre d'émissions** (≈ 19 ns/boule à W48, K5 comme K10) ; la frontière
   est bornée par sa latence (27–28 rondes à ≈ 0,5 ms, Pool qui réveille et attend 47 ouvriers à chaque appel).
4. **Le GPU n'exécute que les feuilles** (≈ 50 ms de travail sur 138), un fil par feuille (3,1–3,4 fils actifs sur
   32, occupation 17–22 %, pile locale 3,2 Kio, ALU 24 %), après le join du parcours, avec un contexte ouvert par
   processus (124–151 ms sous charge). Sa mémoire est proportionnelle aux **feuilles** (2,25 Kio/feuille), pas aux
   émissions : 243 Mo (K5) et 1,42 Go (K10) pour 40 000 sites.
5. **CPU seul, le plancher reste loin du budget** : `domain` W1 = 5,3 s à K5 (ng00) et ×24 de W1 à W48 ; les leviers
   connus ne retirent pas un facteur 4 [I]. Le budget v12 exige un `domain` résident sur le GPU.
6. **Erratum principal pour la v12** : la « feuille J3 complète » **n'a jamais été dans le moteur v10** (`777406b82`)
   ni dans le raccord R2 (`865f5e6`) : c'est un prototype v10 mesuré seulement en local (×1,37 à K5, ×1,55 à K10 sur le
   CPU de l'étage des boîtes, au plus 4 fils). Elle n'explique donc pas les temps G4 de la v10.
7. **Deuxième erratum** : les « extrema q2 couplés » (audit final § 5.4 et § 13) ne sont pas un levier du catalogue,
   mais du census des descentes sur l'index (étage `tree`), hérité de la v8/v9, estimé à 1–2 ms.
8. **Défauts latents relevés** : l'exécuteur partagé fait piloter le même `MemoryBudget` par deux fils en même temps,
   contre le contrat « un seul pilote » ; le repli `unresolved` est série et à un seul espace de travail ; les
   émissions du lot sont matérialisées deux fois (≈ 104 o chacune en u21) avant l'assemblage ; la formule du workspace
   est périmée dans **deux** documents (pas un) ; M3/E4 restent sur la voie CPU malgré l'avis favorable au retrait.
9. **La partition T > 0 se porte directement en u21** (budgets 5B+6+T = 117 ≤ 127 et 2(B+T)+5 = 59 ≤ 63) ; seul u24
   exige la reformulation QE/Dr de l'auditeur. Sur les trames à K5/16, aucune feuille n'est forcée par l'arrêt à la
   largeur 1 (`max_leaf` = 16) : l'effet attendu y est faible [I], à mesurer.
10. **Recommandation** : concevoir la v12 autour d'un `domain` résident (contexte par Session, nuage chargé une fois,
    parcours BFS sur l'appareil, file de feuilles consommée en flux par une feuille data-parallèle, arènes
    proportionnelles aux émissions, tri et assemblage sur l'appareil, repli parallèle), et **commencer par des
    microbancs hors moteur sur les feuilles et les listes vidées de la v11**, avant toute intégration.

---

## 1. L'algorithme exact au gel

### 1.1 Pourquoi un catalogue de boules critiques ; q2, q3, q4

FULL (`docs/MATHEMATIQUES.md` § 4, l. 135–163) est, pour chaque ordre k, la forêt des composantes de
$L_k(a)$, union des intersections de k boules de rayon √a, ou de façon équivalente du graphe Γ_k(a) des k-parties F
avec β(F) ≤ a (β = rayon carré de la plus petite boule englobante, MEB). Les composantes ne changent qu'aux niveaux
β(F) : ce sont les rayons carrés de MEB, les **boules critiques**. Par M1 (§ 1, l. 30–40), toute MEB a un support de
2 à 4 sites affinement indépendants sur sa sphère, dont l'enveloppe convexe contient le centre dans son intérieur
relatif :

- **q2** : paire diamétrale ; centre au milieu, rayon carré |b−a|²/4 ;
- **q3** : triangle **strictement aigu** ; centre circonscrit dans le plan du triangle, intérieur au triangle ;
- **q4** : tétraèdre dont le centre circonscrit est strictement intérieur (quatre poids barycentriques > 0).

Une boule b = (centre, λ) a p sites strictement intérieurs (I_b) et m sites sur la coquille (U_b) ; sa fenêtre
d'événements est [p+q−1, p+m] ∩ [1, K] et Cat_K = {b critique positive : p + q_min ≤ K+1} (§ 3, l. 78–86).
Le catalogue publie, par boule, S*, q_min, p, m, le rang du niveau exact et les populations I puis U, triées par
(niveau exact, S*) (`docs/CATALOGUE.md`, l. 18–24 ; `src/catalogue/catalogue.hpp:52–57`). La tour le consomme
par niveaux et populations, et par la table S* → BallIdx (`src/tower/full_domain.cpp:121–142`), que les descentes
interrogent (4,30 M succès du catalogue à K5 sur ng00, `notes_hors_depot_20261007/gpu_optim/carte_profil_etages.md`
§ 1.3). La partition des centres par boîtes évite toute énumération globale de parties (invariant d'architecture).

### 1.2 Entrée de l'étage et paramètres

`prepare_full_domain` (voie Pool, `src/tower/full_domain.cpp:121–142`) appelle `build_catalogue`
(`src/catalogue/catalogue.hpp:214–217`), puis remplit la table de recherche par CAS. Paramètres du produit
(`src/api/compute.cpp:23–36`) : `leaf_size` = 16 **à tout K**, `max_leaf` = 256, cache J2, tri indirect, frontière
adaptative, assemblage parallèle, passe unique, graphe de paires ; masque moteur 278523
(`src/api/internal.hpp:22`), sans GPU ni cache de blocs. Modes de banc : `802811` (CPU, feuilles 16, cache de blocs)
et `868347:400` (GPU, feuilles 24, part hôte 400 ‰). Contrainte `K+3 ≤ leaf_size ≤ max_leaf ≤ 1024`
(`src/catalogue/catalogue.cpp:9–21`), la même règle M ≥ K+3 que la v10.

### 1.3 Frontière adaptative (préambule)

- **Racine** : liste des n sites, boîte = enveloppe [min, max+1) (`src/catalogue/boxes.cpp:84–96, 171–177`), filtre
  G1 série (`adaptive_frontier.cpp:23–31` → `boxes.cpp:115–141`) : 0,9 ms.
- **Rondes** (`adaptive_prepare.cpp:114–154`) :
  - sélection (`:47–65`) : nœuds divisibles triés par **population intérieure à la boîte demi-ouverte** (`inside`),
    puis taille de liste, chemin, profondeur ; ne sont coupés que les nœuds dont `inside` ≥ max/2 (« lourds
    d'abord »), au plus 1024 − feuilles du plan ;
  - exécution (`adaptive_frontier.cpp:77–86`) : un `parallel_for` sur les 2S enfants, chacun `prepare_node` (filtre G1
    de la liste parente sur la boîte fille, puis ajustement) ;
  - enregistrement (`adaptive_prepare.cpp:67–89`) : `capture` (`:12–32`) **rebalaie en série, sur le pilote,** la
    liste de chaque enfant pour compter `inside` ; publication (`adaptive_frontier.cpp:88–114`).
- **Plan** : au plus 1024 feuilles, vides comprises (`adaptive_frontier.hpp:8`) ; 1023 tâches (1022 sur ng02) et 27–28
  rondes (`receipts/developpement_20261007/diagnostic_domaine/README.md`).
- **Ordre de réclamation** : LPT par **taille de liste** décroissante (`frontier_dispatch.hpp:30–38`).
- **Coût** (ng00, K5, voie GPU, dernière passe chaude, `diagnostic_domaine/claudediag1/gpu_ab_report_diag_k5_24_gpu.json`)
  : racine 0,88 + rondes 14,42 + sélection 0,74 + publication 4,61 = **20,67 ms**. À W1 le préambule ne coûte que
  85,8 ms : il n'accélère que ×4,2 de W1 à W48 (`carte_profil_etages.md` § 1.3). Il est borné par sa latence.

**Précision sur l'audit final (§ 5.1)** : le seuil « population ≥ max/2 » porte sur la population **intérieure à la
boîte**, et le LPT sur la **taille de liste** ; ce sont deux grandeurs différentes.

### 1.4 Passe unique (parcours des boîtes)

`generate_single` (`src/catalogue/single_pass.cpp:181–251`) :

1. ouverture anticipée du contexte CUDA si `cuda_leaves` (`:185`) ; préparation de la frontière (`:193`) ;
2. admission des scratchs et workspaces (`:196–205`) ;
3. **`parallel_for` des 1023 tâches dans l'ordre LPT** (`:215`). Chaque tâche poursuit le DFS (`boxes.cpp:157–169`) :
   - `prepare_node` (`boxes.cpp:115–141`) : réservoir des min(|L|, 3K) sites les plus proches du centre de la boîte,
     tri par insertion (`:16–39`) ; termes par site (`:43–56`) ; **test G1** : x est retiré s'il a K dominateurs
     stricts parmi les témoins sur la fermeture, arrêt au K-ième (`:69–80`, test `:74–76`) ; puis ajustement de la
     boîte à l'enveloppe de la liste (`:131–136`) ;
   - `split_ready` (`:143–155`) : axe le plus long, **coupe au milieu géométrique** (`lo + width/2`), et non à la
     médiane des points (« coupe médiane » de l'audit final est ambigu) ; feuille si `count ≤ leaf_size` ou
     largeur ≤ 1 ; refus `wide_leaf` au-delà de `max_leaf` (`:167`) ;
   - émissions par ordinal en blocs fixes de 256 émissions et 2048 SiteIdx (`single_pass_storage.hpp:10–130`, `:98`) ;
     en voie lot, les feuilles de ≤ 32 sites sont mises en file (`leaf.cpp:451–452`, `leaf_queue.hpp:12–46`).
4. **après le join** : préfixes par ordinal (`single_pass.cpp:100–112`), lot de feuilles (`:115–133`, `:223–225`),
   allocation exacte des sorties (`:227–232`), compactage (`:236–237`).

Profondeur maximale 36 (K5) et 38 (K10), borne 3B ; à K5/16 sur ng00 : 783 071 nœuds, 353 456 feuilles,
379,4 M tests G1 ; à K5/24 : ≈ 272 k nœuds et ≈ 248 M tests (déduits des −510 822 nœuds et −131 150 311 tests,
`receipts/audit_plan_gpu_20261006/README.md`) ; à K10/24 : 1 137 395 nœuds, 1 229 M tests
(`carte_catalogue.md` § 2.2 ; registres `receipts/audit_deep_20261004/performance/derived_optimized.json`).

### 1.5 La feuille (`leaf.cpp`, référence ; `leaf_device.hpp`, source unique hôte/GPU)

`enumerate_leaf` (`src/catalogue/leaf.cpp:450–476`), voie graphe de paires (m ≤ 32 ; au-delà, DFS historique avec
tests de paires, jusqu'à `max_leaf`) :

1. **`prepare`** (`:69–106`) : pour chaque paire i<j, forme affine de la différence des distances sur la fermeture ;
   `base − 2·cmin < 0` : j domine i ; `base − 2·cmax > 0` : i domine j ; sinon arête du graphe de paires
   (bissectrice qui rencontre la fermeture). Deux matrices de masques : `dominance` et sa transposée `dominated`.
2. **`live_rows`** (`:372–383`) : `live[q−2][x]` contient y si |Dom(x) ∪ Dom(y)| ≤ K+1−q (G3 au niveau des paires).
3. **`extend`** (`:304–369`) : DFS lexicographique des cliques ; pour chaque préfixe :
   G3 par popcount de l'union des dominateurs (`:317–323`) ; droites J2 de toutes les faces contenant le dernier site
   (lemme Z, mémo par triplet, `:129–147`, `:325–329`) ; candidat : q2 milieu, q3 strictement aigu + enveloppe M3
   (`:192–199`), q4 orientation ≠ 0 + enveloppe E4 + poids stricts (`:204–218`) ; `center_in_box` demi-ouvert
   (`catalogue.cpp:96–108`) ; puis `census_and_emit`. Prolongement : candidats ∩ lignes vivantes de tout le
   préfixe (`:349–366`), coupure si l'union dépasse déjà K−q (monotonie de G3, `:352–359`).
4. **`census_and_emit`** (`:237–300`) : masques intérieur/extérieur des générateurs (lemme R, `:246–253`), refus
   d'invariant si un site est des deux côtés ; parcours ordonné des sites (contacts du préfixe par curseur, masques,
   sinon `num::side`) ; **rejet au (θ+1)-ième intérieur** (`:273`) ; m ≥ q exigé ; support canonique si la coquille
   est étendue (`support.cpp:90–111` : paire milieu, puis triangle aigu coplanaire au centre, puis tétraèdre
   strictement contenant) ; **émission seulement si S* = présentation génératrice** (`:290`) et p+q_min ≤ K+1
   (`:291`) ; Level matérialisé après admission (q3/q4 différés, `:220–235`).

`leaf_device.hpp` est un **port fidèle séparé** de cette voie (mêmes décisions, mêmes quinze compteurs,
`leaf_device.hpp:1–12`), pas le même texte que `leaf.cpp` : deux implantations sont tenues égales par les portes
(`mhgp11_tower_full_leaf_lanes`) et l'identité des vidages. **[I]** C'est une dette de duplication pour la v12.

### 1.6 Fin de l'étage

- **Lot** (voies GPU) : rassemblement des files (`single_pass_batch.cpp:119–140`), exécuteur (`:184–191`), Level
  recalculé sur l'hôte depuis S* par `Sphere::through` pour **chaque** émission (`:61–99`, `:76–78`), repli série des
  non résolues (`:142–165`).
- **Tri** (`assemble.cpp:110–126`, `sort_indices.cpp:143–178`) : clés binary64 F3 (`sort_level_key.hpp:19–30`),
  passes de tas sur des tranches de 2048, puis ⌈log₂(N/2048)⌉ fusions par tuiles de 4096 avec co-rangs ; comparateur F4
  avec repli exact (`sort_indices.cpp:22–32`, `sort_level_key.hpp:35–40`).
- **Balayage et assemblage par blocs** (`assembly_parallel.cpp:27–128`) : comparaison exacte de chaque paire de
  niveaux adjacents, rangs, décalages, copie CSR.
- **Table S* → BallIdx** : capacité 2^⌈log₂ 2B⌉, charge ≤ 1/2, CAS (`full_domain.cpp:14–20`, `:58–72`, `:134`).

### 1.7 Ce que chaque filtre garantit

| Filtre | Garantie | Source mathématique | Code |
|---|---|---|---|
| G1 (liste) | la liste contient les K plus proches **avec ex æquo** de tout centre de la boîte fermée ; égalité conservée | MATHEMATIQUES § 3, l. 88–100 | `boxes.cpp:58–82` |
| Ajustement | Q ∩ [min L, max L + 1) garde tout centre critique (centre ∈ conv(U_b) ⊆ bbox(L)) | § 3, G4, l. 117–125 | `boxes.cpp:131–136` |
| G2 (census local) | centre dans Q et p ≤ θ_q ≤ K−1 ⇒ I et U complets dans la liste | § 3, l. 102–106 | `leaf.cpp:237–300` |
| G3 (préfixes) | union des dominateurs = minorant de p ; rejet strict au-delà de θ_r = K+1−r ; aucun préfixe de S* rejeté | § 3, l. 108–115 | `leaf.cpp:317–323`, `:352–359` |
| Graphe de paires | dominance stricte ⇔ bissectrice disjointe de la fermeture ⇒ tout support est une clique | `docs/CATALOGUE_SMALL_PAIR_GRAPH.md` | `leaf.cpp:85–104`, `small_pair_graph.hpp` |
| Lignes vivantes | G3 appliqué aux sous-paires du préfixe | idem, § « Certificat » | `leaf.cpp:63–65`, `:372–383` |
| J2 droites (lemme Z) | chaque face d'un support doit avoir sa droite équidistante dans la fermeture ; triplets alignés exclus | `docs/CENTER_REGION.md` | `leaf.cpp:129–147`, `num/center_region.cpp` |
| Propriété | centre dans la boîte **demi-ouverte** : propriétaire unique | § 3, G4 | `catalogue.cpp:96–108` |
| Lemme R | dominateurs d'un générateur : intérieurs stricts ; dominés : extérieurs stricts | `docs/CATALOGUE.md`, § « Census par masques » | `leaf.cpp:246–253` |
| M3 / E4 | conditions nécessaires redondantes avec positivité et propriété (filtres purs) | `docs/CATALOGUE.md`, § « Enveloppes » | `leaf.cpp:192–218` |
| Canonicalisation | seule la présentation S* émet ; unicité de l'émission | § 1, M1–M2 | `leaf.cpp:285–290`, `support.cpp` |
| Complétude | partition finie entièrement traitée ⇒ sortie exactement Cat_K | § 3, G4, l. 122–129 | — |

**[I]** Les lemmes du graphe de paires, de Z, R, M3 et E4 sont dispersés dans trois documents de conception, hors du
contrat `MATHEMATIQUES.md`. La v12 devrait les rassembler dans le contrat, chacun avec ses fixtures d'égalité.

### 1.8 L'index (`src/index/`)

L'index n'est **pas** utilisé par le catalogue, qui construit son propre arbre de boîtes. `GlobalIndex` est l'arbre
radix de Morton (levier V3, `src/index/build.cpp:24–37`, `:76–102`) sur le nuage déjà trié, avec boîtes exactes et
liens `escape` ; construction série en 0,34–0,42 ms (`docs/INDEX.md`). Il sert le census des MEB dans les descentes
(étage `tree`) : `census` en deux passes, saturé à K (`census.cpp:73–90`), ou `CensusWorkspace` en une passe ; bornes
entières `LatticeSphere` (V3) : tests de points ×0,50–0,52 sur les trames (`docs/INDEX.md`). C'est là, et non dans le
catalogue, que porteraient les « extrema q2 couplés » (§ 4.3).

---

## 2. La voie GPU

### 2.1 Ce qui existe

- **Source unique** `leaf_device.hpp` (`MHGP11_LEAF_HD`, `leaf_device_predicates.hpp:8–12`) : `Leaf<Sink>` à
  tableaux fixes de 32 sites (`leaf_device.hpp:19–33`), DFS `extend<Depth>` récursif à la compilation (`:297–334`),
  puits interchangeables : `CountSink`, `AcceptSink` (hôte), `ScratchSink` (comptage avec cases et réservoir
  chaîné), `FillSink` (seconde passe) (`leaf_batch.hpp:45–172`).
- **Contrat R7** (`audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`, l. 1538–1548) : seuls les chemins i128
  dont la borne est prouvée décident. `run_leaf` rend `kUnresolved` (`leaf_device.hpp:338–348`) si :
  - la puissance q3 n'a pas son certificat global (`leaf_device_predicates.hpp:114–135`) ;
  - le certificat d'orientation du centre manque (`:107–113`, `:144–154`) ;
  - l'étendue de la feuille dépasse 2^20 (`kNarrowSpan`, `:196` ; test `leaf_device.hpp:54–55`) ;
  - un invariant que `leaf.cpp` refuserait est violé (`:257`, `:281`, `:245`).

  La feuille est alors rejouée entière par `leaf.cpp` **avant admission** (`single_pass_batch.cpp:142–165`).
  Levier C (`5861c223f`) : sous étendue ≤ 2^20, J2, l'orientation q4 et q3 passent en i32/i64 (bornes 6D³ < 2^63,
  `leaf_device_predicates.hpp:198–204`).
- **Exécuteur CUDA** (`leaf_batch_cuda.cu:349–448`) :
  - blocs d'un warp (`kThreads = 32`, `:20`), un fil par feuille, fils ordonnés par m décroissant (`:155–166`) ;
  - `count_kernel` (`:52–75`), préfixes CUB (`:169–190`), `copy_kernel` et `fill_kernel` des seules feuilles qui
    débordent (`:78–107`) ;
  - retour vers des pages pré-touchées (`:255–304`) ;
  - tous les `cudaMemcpy` sont synchrones et pageables, et le nuage x/y/z est renvoyé à **chaque** lot (`:382–408`).
- **Exécuteur partagé** (`leaf_batch_split.cpp:147–206`) : poids m³ (`:23`), feuilles les plus lourdes à l'hôte
  jusqu'à `host_permille` (`:122–145`) ; le GPU tourne dans un `std::thread` avec un Pool auxiliaire de 4 ouvriers
  **recréés à chaque lot** (`:180–188`) ; fusion dans l'ordre du lot.
- **Contexte** : ouverture anticipée dans un fil, une fois par processus (`leaf_batch_cuda_context.cu:80–94`) ; pool
  mémoire de l'appareil qui garde tout (`:51–60`) ; réservations dans le même `MemoryBudget`
  (`DeviceArray`, `leaf_batch_cuda.cu:123–144`).

### 2.2 Ce qui est mesuré

| Grandeur | Valeur | Source |
|---|---|---|
| Identité CPU/GPU | 372 prises à froid, 84 processus à chaud (identité de la **dernière** passe seulement) | `receipts/developpement_20261004/gpu_g4/README.md` |
| Fils actifs par warp | 3,36 / 32 (S3) ; 3,29 (S6) ; 3,18 à K10 | `gpu_g4`, `carte_gpu_existant.md` § 2.3 |
| Occupation | théorique 16,7–25 %, atteinte 16,6–21,6 % ; 168–210 registres | `gpu_g4` |
| Mémoire locale | 3 248–3 296 o par fil, 2,2–2,4 o utiles par secteur de 32 ; 40,5 % d'attente L1TEX ; L2 à 99 % de succès, DRAM à 1,2 % | idem |
| Modèle de warp | durée ≈ Σ travail des 32 feuilles / 3,14 ; 188 SM × 12 warps ; 5 vagues (K5), 7,35 (K10) ; équilibrage parfait borné à ×0,86–0,93 | `developpement_20261006/wfgpu1_leviers/workflow/D_RAPPORT.md` § 3 |
| Prédicteur du travail | Spearman(cycles, m) = 0,55 / 0,45 ; Spearman(cycles, triplets candidats) = 0,91 / 0,93 | idem § 2 |
| Lot, K5/24, ng00 | 123 581 feuilles : GPU 91 679 en 50,1 ms, hôte 31 902 en 49,8 ms ; exécuteur 51,8 ms, comptage 47,8 ms | `diagnostic_domaine/claudediag1/...k5_24_gpu.json` |
| Lot, K10/24, ng00 | 530 259 feuilles ; exécuteur 207,2 ms, comptage 195,9 ms | `developpement_20261007/cache_blocs/claudecache1/gpu_ab_report_ab_k10_24_gpu.json` |
| Mémoire de l'appareil | 242,8 / 196,5 / 229,0 Mo (K5/24, part 400 ‰) ; 1 418,6 / 1 150,1 / 1 307,9 Mo (K10/24) | mêmes rapports (`device_bytes`) |
| Contexte | 69–78 ms seul ; 124–151 ms en concurrence du parcours (`prefetch_ns`) ; reste attendu à la 1re passe 62–88 ms ; 1er lancement de noyau 2,95 ms (chargement paresseux) | `claudediag1` ; `carte_gpu_existant.md` § 2.6 |
| Feuilles qui émettent | 60 858 / 123 581 (49 %) à K5/24 ; 228 759 / 530 259 (43 %) à K10 | `copied_jobs` des rapports |

### 2.3 Pourquoi la voie est sous-employée

1. **Amdahl.** Le GPU ne fait que les feuilles, environ 50 ms sur 138 à K5. Il reste inactif pendant la frontière, le
   parcours, les étages de fin et toute la tour, soit ≈ 114 ms de forêts.
2. **Join global.** Le lot démarre après le `parallel_for` du parcours (`single_pass.cpp:215` puis `:223`). L4
   (16 sous-lots sur un seul fil) a été plus lent ×1,6–3,5 (`developpement_20261006/l4_recouvrement`). **[I]** Ses
   sous-lots de ≈ 22 k feuilles remplissent moins d'une vague (2 256 warps résidents), et chacun payait synchronisation,
   préfixes et transferts : l'échec réfute ce découpage, pas le principe d'un flux.
3. **Un fil par feuille.** Divergence entière : la moitié des feuilles n'émet rien, d'autres jusqu'à 189 boules (K5) ou
   609 (K10). Pile locale de 3,3 Kio dimensionnée à 32 sites même pour des feuilles de 16 ou 24
   (`leaf_device.hpp:25–31`). Occupation limitée par les registres. La feuille coopérative « par paires » a échoué
   (×1,15–1,38 à K5, `coop3_reconvergence`) parce qu'elle répartissait des **sous-arbres divergents**.
4. **Contexte par processus.** À froid, la voie GPU perd : `domain` 224/205/231 ms contre 208/190/209 ms pour la voie
   CPU (`cache_blocs/claudecache1`, médianes à froid).
5. **Transferts et Levels.** Le Level est recalculé sur l'hôte pour chaque émission (2,8 ms à K5, 13,6 ms à K10).
   Envois synchrones et pageables. Coût faible à K5, mais incompatible avec un flux.
6. **[I]** Le fil de l'appareil attend probablement en boucle active dans `cudaDeviceSynchronize` (ordonnancement
   CUDA par défaut, un seul contexte pour 48 processeurs logiques), et prend un fil matériel aux feuilles de l'hôte
   pendant le partage. Non mesuré.

### 2.4 Ce que demanderait un `domain` résident sur le GPU et en flux

1. **Ressources de Session** : contexte, flux CUDA, pool mémoire et tampons épinglés ouverts une fois ; modules
   chargés d'avance ; nuage SoA chargé une fois par trame (0,5 Mo à 40 k sites). Tout cela dans un budget qui compte
   ensemble l'hôte, la mémoire épinglée et l'appareil (R7).
2. **Parcours sur l'appareil**, en largeur niveau par niveau (≤ 38 niveaux) ou par file de travail :
   - réservoir des 3K plus proches par nœud, sélection au niveau du warp ;
   - filtre G1 par couple (nœud, site), en i64 natif (2B+5 ≤ 63) ;
   - compactage stable par préfixes, enveloppe par réduction segmentée, coupe ;
   - ordre déterministe : clé (distance, rang dans la liste parente) pour le réservoir ; listes dans l'ordre parent.

   Estimation de la note : 5–15 ms à K5 [E] (`carte_catalogue.md`, O2). **Jamais prototypé.**
3. **Feuilles en flux** : chaque feuille née est poussée dans une file de l'appareil, consommée par un noyau de
   feuilles **data-parallèle** sur un second flux ou par un noyau persistant. Deux formes candidates :
   - les phases J3 de la v10 : paires, puis triplets avec la table H, puis quadruplets par ET de trois lignes de H,
     census par vote du warp ;
   - la feuille cohérente Q : tout le warp sur un même préfixe, voies sur les sites du census et sur les combinaisons
     du support canonique (contrat de l'auditeur : premier événement ordonné, θ+1, une seule consultation logique du
     cache, `audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md`, l. 277–347).
4. **Arènes proportionnelles aux émissions** (blocs pris par atomique) au lieu de cases de 2 Kio par feuille.
   L'ordre d'arrivée importe peu : le catalogue final est trié par une clé unique (niveau, S*), et les compteurs
   sont des sommes. Il faut donc des compteurs logiques indépendants de l'ordre de visite.
5. **Fin sur l'appareil** : tri radix des clés F3 (u64 d'un double positif) ; chaînes de voisins non certainement
   ordonnés résolues en exact (sur l'hôte, ou sur l'appareil en multi-mots) ; rangs ; CSR ; table de hachage S*.
   Ensuite, rapatriement compact (8 o par boule et 1 o par incidence, comme aujourd'hui) et Level matérialisé sur
   l'hôte, en parallèle et seulement pour les rangs distincts et les bandes [I].
6. **Repli parallèle U2** : sorties privées par feuille, préfixes ordonnés après join, une porte qui mêle étendues
   2^20 et 2^20+1.
7. **Découplage CPU/GPU entre trames** [I] : à 10 Hz, le `domain` de la trame t+1 sur le GPU peut recouvrir les
   forêts de la trame t sur le CPU. Le débit est alors borné par max(domain, forêts), et non par leur somme. Cela
   suppose une décision explicite sur le contrat : latence ou cadence.

---

## 3. Coûts, modèle et plancher

### 3.1 Répartition mesurée de l'étage (ng00, dernière passe chaude)

| Sous-étage (ms) | K5 GPU `868347:400`, f24 | K5 CPU `802811`, f16 | K10 GPU `868347`, f24 |
|---|---:|---:|---:|
| Frontière (racine / rondes / sélection / publication) | 20,67 (0,88 / 14,42 / 0,74 / 4,61) | 19,87 | 26,42 |
| Passe unique (parcours ; + feuilles en voie CPU) | 37,48 (Σ tâches 1 781 ms CPU) | 152,09 (Σ 7 016 ms) | 141,33 |
| Lot de feuilles (exécuteur) | 51,79 | — | 207,18 |
| Level hôte + rassemblement | 2,80 + 0,35 | — | 13,63 + 1,29 |
| Tri / balayage / assemblage / compactage | 9,98 / 4,43 / 3,41 / 3,24 | 10,04 / 4,40 / 3,30 / 2,82 | 42,76 / 19,35 / 14,73 / 11,47 |
| Allocation / restitution / table | 0,09 / 0,76 / 2,86 | 0,08 / 3,57 / 2,87 | 0,11 / 1,53 / 9,97 |
| **`domain`** | **137,99** | **199,16** | **489,89** |
| dont frontière + fin (hors parcours et feuilles) | **48,6 (35 %)** | **46,9 (24 %)** | **141,3 (29 %)** |

Sources : `developpement_20261007/diagnostic_domaine/claudediag1/gpu_ab_report_diag_k5_{24_gpu,16_cpu}.json`,
`developpement_20261007/cache_blocs/claudecache1/gpu_ab_report_ab_k10_24_gpu.json` (mode `gpu_cache`). L'audit
final reprend exactement la colonne K5 (§ 5.1) : vérifié. La dernière ligne est mon calcul sur ces champs.

### 3.2 Volumes

| | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| Cat5 (boules / par site) | 1 306 696 / 32,8 | 1 095 926 / 30,8 | 1 407 885 / 30,7 |
| Incidences K5 (par boule) | 6 097 121 (4,67) | 5 085 683 (4,64) | 6 514 697 (4,63) |
| Cat10 (boules / par site) | 5 512 670 / 138,2 | 4 383 302 / 123,3 | 5 483 320 / 119,6 |
| Incidences K10 (par boule) | 45 383 538 (8,23) | 35 706 993 (8,15) | 44 413 313 (8,10) |
| Feuilles K5/16 ; K5/24 ; K10/24 | 353 456 ; 123 581 ; 530 259 | 284 835 ; 99 768 ; 430 579 | 323 879 ; 115 657 ; 486 530 |

Sources : `docs/CATALOGUE.md` (table d'Euler), `audit_deep_20261004/performance/derived_optimized.json`, rapports
`gpu_ab` ci-dessus. Les fourchettes de la consigne sont **confirmées** : 31–33 boules/site à Cat5, 120–138 à Cat10.

Nuages uniformes 3D à K5 : 74,7 / 77,1 / 79,3 boules/site à 8 k / 16 k / 32 k sites (`docs/CATALOGUE.md`), soit
≈ 2,4 fois plus que sur les trames LiDAR, dont la géométrie est surfacique. De K5 à K10 : boules ×4,2, incidences
×7,4, feuilles (à 24) ×4,3, tests G1 ×5. **[I]** C'est cohérent avec une croissance en ≈ K^d·n, avec d ≈ 2 sur des
surfaces (la v10 annonçait ≈ 460 boules/site en 3D à K10).

**Recouvrement des feuilles** : à K5/16 on envoie 19,7 Mo d'identifiants de sites pour 353 456 feuilles
(`carte_gpu_existant.md` § 2.6), soit Σm ≈ 4,93 M : chaque site figure en moyenne dans ≈ 124 feuilles. Les
« 13 feuilles par site à K10/24 » de l'audit final (§ 6.3) sont le rapport feuilles/sites (530 259 / 39 885), pas
une mesure de recouvrement.

### 3.3 Modèle de coût (W48, régime chaud) [I, calé sur les mesures ci-dessus]

T_domain ≈ T_frontière + T_parcours + T_feuilles + T_fin, avec :

- **T_frontière ≈ 20–26 ms**, presque constant. Il est borné par la latence de 27–28 rondes : chacune paie un
  `parallel_for` qui réveille et attend les 47 ouvriers (`src/sched/pool.cpp:88–110`, `notify_all` puis acquittement
  de tous), plus des balayages série sur le pilote. Découper les rondes par tranches n'a rendu que ×0,859
  (`frontiere_tranches`).
- **T_parcours ∝ nœuds et tests G1** : ≈ 7 ns de CPU par test G1 à W48, réservoir et allocations compris
  (1,78 s / 248 M tests à K5/24). Le parcours est bien équilibré : 37,5 ms mesurés contre 1,78 s / 48 = 37,1 ms. En
  local, le réservoir pèse plus du tiers du filtre une fois le test vectorisé (`filtre_g1_avx2`).
- **T_feuilles** : avec un fil par feuille sur le GPU, ≈ 36 ns par émission à K10 (195,9 ms / 5,51 M). Sur 48 fils
  de l'hôte avec `leaf.cpp` en ligne, 45–54 ms à K5/16 (`carte_catalogue.md` § 2.2).
- **T_fin ≈ 19 ns par émission** (24,8 ms / 1,31 M à K5 ; 99,9 ms / 5,51 M à K10) : strictement linéaire en N.

Travail total : `domain` W1 = 5 305–5 338 ms à K5/16 (ng00 ; `n1_ab`, `carte_profil_etages.md` § 1.3), soit ×24 de W1
à W48 sur 24 cœurs physiques. N1 a montré que le « coût par nœud » de 6–7 µs à W48 n'était qu'un temps de fil SMT
(`developpement_20261006/n1_ab/README.md`). Le CPU est donc **borné par le travail divisé par 24 cœurs**.

### 3.4 Planchers plausibles sur G4 [I]

- **CPU seul, K5** : même en cumulant J3 (×1,37 local sur l'étage des boîtes), G1 AVX2 (−12 à −17 % sur parcours et
  frontière), un tri radix et une frontière élargie, le travail W1 ne descendrait que vers 3,5–4 s, soit
  **≈ 140–170 ms** à W48. Le budget de 40–50 ms est hors d'atteinte sans GPU.
- **`domain` résident GPU, K5** : parcours 5–15 ms [E], feuilles 15–25 ms si une feuille data-parallèle atteint
  ×3–5 sur le noyau un-fil (68–73 ms pour le lot entier seul, `domain_diagnostic`), fin sur l'appareil 2–5 ms,
  transferts 1–5 ms, Levels et table sur l'hôte 3–6 ms en parallèle : **≈ 25–50 ms**. Le budget est donc plausible,
  mais pas assuré. Le risque est concentré sur le noyau de feuilles : son ×3 n'est pas étayé
  (`PLAN_GPU_FINAL.snapshot.md`, l. 219).
- **K10 résident** : parcours 10–30, feuilles 40–65, fin 5–10, rapatriement compact ≈ 90 Mo (≈ 4 ms épinglé), Levels
  14 ms en parallèle : **≈ 70–130 ms**. Les forêts K10 (≈ 0,9–1,3 s) restent l'obstacle.

---

## 4. Comparaison v10 ↔ v11 et vérification de l'audit final et du rapport B

### 4.1 Ce que fait réellement la v10

| Mécanisme | v10 (`777406b82` ; R2 `865f5e6`) | v11 au gel | Effet connu |
|---|---|---|---|
| Repère des boîtes T6 | sites mis à l'échelle X = x << 6 : boîtes au 1/64 de maille, enveloppe [min X, max X + 1/64), coupe au milieu en 1/64 ; arrêt par **stagnation**, 9 niveaux sans décroissance sous 2^T (`morsehgp3D_v10/src/catalogue/generator.cpp:22–31`, `:567–620`) | T0 entier, enveloppe [min, max+1 mm), arrêt à la largeur 1 (`boxes.cpp:94`, `:148`) | jamais mesuré comme port ; listes, préfixes et filtres diffèrent entre moteurs (`audit_deep_20261004/performance/README.md`) |
| Table M(K) | 12 / 16 / 24 / 28 pour K ≤ 3 / ≤ 6 / ≤ 10 / > 10, calée sur t_boxes CPU (`generator.cpp:641` ; R2 `:645–650`) | API : 16 à tout K (`compute.cpp:26`) ; banc : 16 (CPU K5), 24 (GPU K5, K10) | K10 16 → 24 : −24 à −27 % du mur FULL (`claudediag1`) ; K5 CPU 16 → 24 : +17 à +21 ms (`reservoir3`) |
| Frontière | BFS par niveaux jusqu'à **64·P tâches (3 072 à W48)**, puis seuls les nœuds de plus de n/(64P) sites ; LPT par charge (R2 `generator.cpp:725–780`) | ≤ 1 024 tâches, 27–28 rondes, « lourds d'abord » | non isolé |
| Feuille | « v2 » à masques : paires vivantes P2, **triplets vivants H**, quadruplets par **ET de trois lignes de H**, m ≤ 64 par mot (R2 `generator.cpp:324–476`, commit v10 `6615febaf`) | DFS de cliques, lignes vivantes de **paires** seulement, cache J2, m ≤ 32 | non isolé |
| Feuille J3 | **prototype jamais intégré** : `morsehgp3D_v10/receipts/audit_continu_20260929/performance_corrected/observed/feuille/` (`leaf.hpp`, `J3_feuille_v3.patch`) | ports partiels : R (`9b9244a00`), M3/E4 (`ec55578d9`), voie étroite sur GPU (`5861c223f`) | J3 : ×1,371 (K5) et ×1,548 (K10) sur le CPU de t_boxes, local, ABBA, ≤ 4 fils ; ports v11 à W1 : R neutre (1,000–1,006), M3/E4 **+1,2 à 1,4 %** (`mesures_g4_ab8_diag1`) |
| Filtre de nœud | sans branchement, préfixe S0 puis reste du réservoir (R2 `generator.cpp:500–563`) | boucle avec arrêt au K-ième dominateur (`boxes.cpp:69–80`) | — |
| Tri | `parallel_sort` sur (double approché, S*), bandes réparées en exact (R2 `:813–849`) | tri indirect par tuiles, clés F3/F4 | v11 : 10 ms (K5), 43 ms (K10) |
| Mémoire | `std::vector` hors budget | `Buffer` budgétés, cache de blocs | — |
| Catalogue K5, CPU W48 | **163,5 / 136,9 / 164,3 ms** (u18, 3e passe chaude ; `audit_deep_20261004/performance`) | CPU 200 / 162 / 195 ; GPU 138 / 120 / 141 | — |

**Ce que contient la feuille J3** (en-tête du prototype `leaf.hpp`, l. 1–20 ; phases `:102–401`) :
- D : dominance sans branchement, masques en lignes et en colonnes ;
- P : paires ;
- Z + M : triplets, avec des termes de paire (u_ij, P_ij) calculés une fois et une table H ;
- B : quadruplets ; E4, puis intérieur strict par coordonnées barycentriques **avant** le centre (B4) ;
- R : census limité aux sites que les masques ne décident pas ; côté q2 en i64 ;
- SoA, m ≤ 64 (un mot).

La v11 n'a porté que R, M3/E4 et, sur le GPU seulement, la séparation étroite par étendue. **[I]** La phase H, les
termes de paire de Z et le côté q2 en i64 sont la partie probablement rentable, et elle n'est pas portée. Les ports
faits ne gagnent rien sur le CPU.

### 4.2 Ce qu'on peut en attendre [I]

- **J3** : ×1,3–1,5 sur les feuilles CPU au mieux. Son intérêt pour la v12 est surtout sa **forme en phases**, qui se
  répartit naturellement sur les voies d'un warp (l'en-tête du prototype le prévoit déjà).
- **T > 0** : aucune feuille forcée à K5/16 sur les trames (`max_leaf` = 16 = `leaf_size`, registre c40). L'effet
  viendrait de boîtes plus serrées (1/64 mm), faible à l'échelle centimétrique du LiDAR. Il compterait davantage pour
  des nuages denses ou une grille plus fine.
  - Faisabilité (réponse R6 de l'auditeur, `AUDIT_CONTRATS...md`, l. 1524–1537) : **u21/T6 passe directement**
    (5B+6+T = 117 ≤ 127 ; 2(B+T)+5 = 59 ≤ 63).
  - u24/T6 exige la reformulation « QE contre Dr » (produits de 4B+5+T = 107 bits) et l'élargissement du majorant du
    réservoir (65 bits).
- **M(K)** : à K5, la table v10 donne 16, la valeur CPU de la v11. Le gain est à K10 (24), déjà mesuré au banc mais
  absent du produit. La table v10 a été calée sur le CPU. Il faut une table par K **et par voie** (le GPU préfère 24
  à K5).
- **Frontière v10** : plus large (≈ 12 rondes de BFS jusqu'à 3 072 tâches, au lieu de 27–28 rondes étroites). **[I]**
  C'est un candidat sérieux à l'écart de frontière (20 ms en v11), non mesuré côté v10.

### 4.3 Vérification de l'audit final (§ 5–6) et du rapport B : confirmations et errata

**Confirmés**, tous chiffres revérifiés dans les rapports JSON :
- la table de répartition de § 5.1 ;
- les leviers de § 5.2 et § 6.1, dont : J2 mémorisé ×0,82–0,83 (K5) et ×0,76 (K10) ; levier C ×0,83–0,86 ; partage
  400 ‰ ×0,873–0,907 ; feuilles de 24 à K5 en GPU 192 contre 210 ms ; feuilles de 24 à K10 −24 à −27 % ;
- les rejets de § 5.3 et § 6.2 : frontière par tranches 0,859 ; G1 AVX2 0,895 ; THP 1,064 ; N1 +2,4 à 8,5 ms ;
  L4 ×1,6–3,5 ; coopérative ×1,15–1,38 à K5 ; levier A ×1,11 ;
- les limites de § 6.3 : 3,1–3,4 fils actifs ; occupation 17–22 % ; 196–243 Mo et 1,15–1,42 Go ;
  224/205/231 ms contre 208/190/209 ms à froid ;
- le repli série (`single_pass_batch.cpp:142`) ;
- le retrait de M3/E4 approuvé et non fait : l'auditeur écrit « aucune objection au retrait CPU de M3/E4 »
  (`AUDIT_CONTRATS...md`, l. 1682–1688) et les enveloppes sont toujours dans `leaf.cpp:192–218`.

**Errata et précisions :**

1. **J3 n'est pas un mécanisme de vitesse de la v10.** L'audit final (§ 14.2 : « mécanismes qui faisaient la
   vitesse de la v10 [...] feuille J3 complète ») et la passation (§ 3 : « À porter de la v10 (`777406b82` et son
   raccord R2) [...] la feuille J3 complète ») se trompent. Il n'y a aucun `leaf.hpp` dans
   `morsehgp3D_v10/src/catalogue/` ni dans `build/v10-integration-r2/src/morsehgp3D_v10/src/catalogue/` (0 occurrence
   dans `final.patch`). Le moteur v10 joue la feuille v2 (`enumerate_leaf_masks`). La source de J3 à porter est le
   prototype du reçu d'audit v10. L'écart v10/v11 du catalogue CPU (×1,2) a donc d'autres causes, non isolées [I] :
   - u18 contre u21 (×1,05) ;
   - la feuille v2 à table H ;
   - la frontière plus large ;
   - des boîtes T6 plus serrées.
2. **« Extrema q2 couplés » mal classés** (audit final § 5.4 et § 13, ligne « Catalogue CPU »). C'est un helper du
   **census sur l'index**, pour les descentes, hérité de la v8/v9. L'auditeur précise que « le scan des feuilles du
   catalogue ne serait pas accéléré directement » (`AUDIT_CONTRATS...md`, l. 1882–1897). Gain estimé ≤ 0,2–0,6 % du
   CPU, 1–2 ms (`notes_hors_depot_20261007/audit_transpositions/verif/auditeurs.md`, l. 117–135). De même, le
   « repli U2 » est une recette des auditeurs v11, pas un mécanisme v10. La liste « mécanismes v10 non portés » de la
   consigne mêle donc trois origines.
3. **« Condition U2 du plan GPU » = 40–50 ms** (audit final § 14.1 ; passation § 6.3). Le plan écrit « domaine sous
   **55 ms** » (`PLAN_GPU_FINAL.snapshot.md`, l. 219). Les 40–50 ms sont le partage 50/50 du développeur
   (`QUESTION_CLAUDE_VITESSE_100MS_20261004.md` § B ; `carte_catalogue.md` § 4 dit même 30–40 ms). Le même audit
   emploie en outre « U2 » pour deux objets distincts (condition du plan, et recette de repli des auditeurs).
4. **Workspace** (passation § 9 : « non revérifié ici »). **Tranché.** Le code réserve **deux** matrices de C⌈C/64⌉
   mots, soit 16C⌈C/64⌉ octets : `workspace_memory_bound` et `Workspace::allocate`
   (`src/catalogue/catalogue.cpp:42–51`, `:53–71`), depuis `9b9244a00` (lemme R). S'y ajoutent les lignes du graphe de
   paires (256 o) et le cache J2 (≤ 4 960 o), absents de la formule. L'admission est juste ; c'est un **erratum
   documentaire, présent dans deux fichiers** : `docs/CATALOGUE.md:158` et `docs/CATALOGUE_PARALLELE.md:50`.
5. **« Coupe médiane »** (§ 5.1 ; rapport B) : c'est une bissection au milieu du plus long côté (`boxes.cpp:149`).
6. **« Seules les listes de population ≥ max/2 sont coupées »** : il s'agit de la population **intérieure** à la
   boîte (`adaptive_prepare.cpp:59–63`), pas de la taille de liste.
7. **« 13 feuilles par site »** (§ 6.3) : c'est le rapport feuilles/sites. Le recouvrement réel est Σm/n ≈ 124 à
   K5/16 (§ 3.2).
8. **Rapport B, « La passe qui précède le lot ne dure qu'environ 60 ms, d'où 62 à 88 ms d'attente »** :
   **confirmé**. Frontière + passe unique = 20,7 + 37,5 = 58 ms (`claudediag1`) ; contexte 124–151 ms sous charge ;
   l'attente résiduelle mesurée à la première passe vaut 62–88 ms (`first_pass_device_init_ms`).
9. **« Table support → boule remplie par CAS, ~30 → 3 ms »** (§ 5.2) : précision plutôt qu'erratum. Le 30 ms ne
   vient que du commentaire du code cité comme source (`full_domain.cpp:133`) ; seul le 3 ms est mesuré
   (`lookup_ms` 2,8–2,9).

---

## 5. Dette et défauts latents

1. **Enveloppes M3/E4 sur la voie CPU** (`leaf.cpp:192–218`). Leur coût est mesuré (+1,2 à 1,4 % de la passe unique à
   W1) et le retrait est approuvé, mais il n'a jamais été fait. Leur valeur sur le GPU, demandée séparément par
   l'auditeur, n'a **jamais été mesurée** : aucune session ne les isole.
2. **Workspace** : voir l'erratum 4 du § 4.3. Code correct, deux documents périmés.
3. **Repli `unresolved` série** (`single_pass_batch.cpp:142–165`) : une boucle sur le pilote, un seul espace de
   travail de 32 sites, après la matérialisation. Il n'est jamais exercé sur les trames u21. **[I]** En u24, ou sur
   un nuage épars dont une feuille dépasse 2^20 d'étendue, il peut devenir le chemin critique. La porte décisive U2
   (étendues 2^20 et 2^20+1 mêlées, W1 contre W48, refus tardif) n'existe pas.
4. **Contrat du budget enfreint par l'exécuteur partagé.** `run_leaf_batch_split` fait appeler `admit` et `allocate`
   **en même temps** par le fil de l'appareil et par le pilote (`leaf_batch_split.cpp:182–194`). Or `admit` ne promet
   « aucun refus après admission » que si « rien d'autre n'alloue dans ce budget pendant l'étage »
   (`core/buffer.hpp:98–107`), et `catalogue.hpp:213` affirme « le budget a toujours un seul pilote ». Le compte
   reste juste, car il est atomique (`buffer.cpp:22–35`). **[I]** Sous un budget serré, un refus peut dépendre de
   l'entrelacement : refus tardif, non déterministe, mais sans publication partielle. Non documenté.
5. **Émissions matérialisées deux fois.** Le bloc du lot (`Emission` ≈ 104 o en u21 : `CatalogueBall` 32 o + `Level`
   64 o (`Wide<3>` × 2) + 8 o) est copié dans les sorties de la passe unique, et les deux coexistent jusqu'à la fin de
   `generate_single` (`single_pass.cpp:221–237`). L'assemblage copie ensuite une troisième fois vers les tableaux du
   catalogue (`assemble.cpp:153–169`). À K10 : 5,5 M × 104 o ≈ 573 Mo, deux fois. Pic `domain` K10 1,54 Go
   (`carte_profil_etages.md` § 1.2).
6. **Mémoire de l'appareil proportionnelle aux feuilles** : cases de 2 Kio plus un huitième de réservoir
   (`leaf_batch.hpp:74`, `:191`). **[I]** Pour 1 M sites à K10 (13,3 feuilles/site), ≈ 30 Go ; une arène
   proportionnelle aux émissions ferait ≈ 16 o/boule plus 1 o/incidence, soit ≈ 2,2 Go.
7. **Mémoire retenue hors compte.**
   - Le pool de l'appareil garde tout ce qui lui est rendu (seuil de libération maximal, `leaf_batch_cuda_context.cu:51–60`).
   - Côté hôte, le cache de blocs arrondit chaque bloc à sa classe (jusqu'à +9 %, `buffer.cpp:44–47`, `:90–110`), et
     cet arrondi n'entre pas dans `used`.
   - Ces deux mémoires sont bornées par ailleurs, mais aucune n'est un « octet compté » du budget. C'est la question Y2
     des auditeurs, restée ouverte.
8. **Transferts** synchrones et pageables. Le nuage est renvoyé à chaque lot, et un Pool de 4 ouvriers plus un
   `std::thread` sont créés à chaque lot (`leaf_batch_cuda.cu:382–408`, `leaf_batch_split.cpp:180–188`).
9. **Le produit diffère du banc**.
   - L'API et le CLI n'ont ni GPU, ni cache de blocs, ni feuilles de 24 à K10 (`compute.cpp:23–36`).
   - Le `domain` K10 du CLI coûte 1 740 ms, contre 830–900 ms au banc avec des feuilles de 24 (`carte_profil_etages.md`
     § 1.2).
   - La `Session` crée son budget sans cache (`session.cpp:21`).
10. **Aucune porte CUDA dans la matrice G4.** `tools/g4_matrix.json` et `.py` ne mentionnent ni CUDA ni GPU, et la
    construction CUDA est « hors portes par défaut » (`CMakeLists.txt:68`). Les quatre portes numériques demandées
    n'existent que côté hôte : q3 extrême, q4 aux étendues 2^20 et 2^20+1, préfixe obtus, coquille à q_min = 2.
11. **Identité à chaud limitée à la dernière passe** (`gpu_ab`, champ `scope`) : 162 passes intermédiaires de coop1
    n'ont aucune empreinte.
12. **Duplication `leaf.cpp` / `leaf_device.hpp`** : deux implantations à tenir égales.
13. **Pool** : chaque `parallel_for` réveille et attend tous les ouvriers, même pour deux tranches
    (`pool.cpp:88–110`). **[I]** Ce coût fixe par appel explique la latence des rondes et pèse sur la vingtaine
    d'appels des étages de fin.
14. **Invariants supposés** :
    - « feuilles ≤ sites » était faux (`claudegpu2`) ; la borne est maintenant ≤ 2^40 feuilles par lot
      (`leaf_device_predicates.hpp:30–38`) ;
    - `count + spare < 2^32` est gardé (`leaf_batch_cuda.cu:356–357`) ;
    - le test d'étendue `kNarrowSpan` n'existe qu'au-delà de 20 bits (`leaf_device.hpp:54–55`) ; ses mutants
      `etendue_*` n'étaient effectifs qu'en u21, ce que corrige `38faaf272` en source ; aucun jugement G4 de ces
      mutants n'est établi par ma lecture.
15. **Collisions de noms** :
    - « J2 » désigne le filtre de région du catalogue et le juge d'ordre un (`MATHEMATIQUES.md`, l. 345) ;
    - « J3 » désigne la feuille v10, le juge d'Euler (l. 350) et le juge « niveaux d'entrée » de R2 ;
    - « U2 » désigne la condition du plan et la recette de repli.

    **[I]** C'est une source d'erreurs de lecture ; la v12 devrait tenir un registre des noms.

---

## 6. Recommandations pour l'étage `domain` de la v12 (budget 40–50 ms à K5)

### 6.1 À porter tel quel (port explicite, épinglé à `ac081a06f`, requalifié)

- Le contrat G1–G4 et les filtres de feuille, avec leurs fixtures, **rassemblés dans le contrat mathématique** :
  graphe de paires, G3 et lignes vivantes, monotonie de l'union, lemme Z, lemme R, M3/E4 comme filtres purs. S'y
  ajoutent S* canonique, admission p+q_min ≤ K+1 et niveaux q3/q4 différés.
- `num` : budgets par expression, prédicats certifiés i128, certificats q3 et d'orientation, preuves de la voie
  étroite (6D³ < 2^63).
- Le **contrat R7** : chemin certifié, sinon `unresolved` rejoué en exact avant admission.
- Les clés F3/F4 avec repli exact (`sort_level_key.hpp`).
- Le budget transactionnel, `BudgetReservation` pour l'appareil, le cache de blocs au niveau de la Session.
- Les compteurs locaux de feuille (R1).
- Les juges d'Euler à K+2 et J1, hors produit, comme filets.
- Pour la voie CPU de référence : LPT et lourds d'abord.

### 6.2 À repenser

1. **`domain` résident GPU et en flux** (§ 2.4) :
   - le contexte, les flux, le pool et la mémoire épinglée appartiennent à la Session ;
   - parcours BFS sur l'appareil ;
   - file de feuilles consommée en même temps que le parcours ;
   - arènes proportionnelles aux émissions ;
   - tri radix, rangs, CSR et table sur l'appareil ;
   - rapatriement compact, Levels sur l'hôte pour les seuls rangs distincts et les bandes.
2. **Feuille data-parallèle en source unique**, compilée pour SIMD sur l'hôte et pour CUDA sur l'appareil, en phases
   J3 ou en forme cohérente Q, choisie par microbanc. Elle est instanciée à la taille réelle de feuille. La seule voie
   séquentielle restante est le DFS des feuilles larges. Bannir « un fil par feuille ».
3. **Frontière** :
   - sur le GPU, elle disparaît dans le BFS ;
   - sur le CPU, BFS large (cible 64·P tâches comme la v10), `inside` compté dans la ronde parallèle ;
   - racine sans filtre (lemme V6 cité par `carte_catalogue.md`, O5) ;
   - ordonnanceur à faible coût par appel.
4. **Fin d'étage ≤ 5 ms à K5**, au lieu de 25 ms :
   - aucune `Emission` à 104 o recopiée ;
   - clé u64 et S* compacts ;
   - Level calculé à la demande ;
   - tri radix plus chaînes exactes, puis balayage fusionné avec le tri.
5. **Repli parallèle U2**, budgété, avec sa porte décisive.
6. **Tailles de feuille par K et par voie**, identiques dans l'API et dans le banc.
7. **Partition T > 0** : la mesurer d'abord en u21, où le port est direct. Ne viser u24 qu'avec la reformulation QE/Dr.
8. **Compteurs logiques indépendants de l'ordre de visite**, diagnostics physiques séparés, identité vérifiée à
   chaque passe.
9. **Retirer M3/E4 du CPU**, ou les mesurer sur le GPU dans la nouvelle feuille.
10. **Budget à plusieurs pilotes** explicite : réservations effectives par étage et par flux, au lieu de `admit`
    supposé exclusif.

### 6.3 Risques

- La feuille data-parallèle ×3 n'est pas étayée, et la forme coopérative par paires a échoué.
- Le GPU impose la reproduction exacte des compteurs et de l'ordre du réservoir (clé distance puis rang).
- Il faut une mémoire de listes par niveau sur l'appareil.
- Volume de rapatriement à K10, si la tour reste sur le CPU.
- Davantage de feuilles non résolues en u24.
- Coût de qualification : CUDA incompatible avec les sanitizers de la construction, CMake 3.22 sur la VM, aucune porte
  CUDA aujourd'hui.
- Bruit ±9–12 % à W48.
- Le `domain` seul ne ferme pas les 100 ms si les forêts restent à 91–116 ms.

### 6.4 Mesures à faire d'abord (avant le moteur)

1. **Vider les entrées de la v11 gelée** (feuilles : sites et boîtes ; tâches de frontière ; listes par niveau) sur
   ng00–02 à K5/16, K5/24 et K10/24, ainsi que sur 8 k/16 k/32 k. Puis rejouer **hors moteur** sur G4 :
   - (a) le noyau un-fil de la v11 comme témoin ;
   - (b) une feuille en phases J3 ;
   - (c) une feuille cohérente Q ;

   en exigeant l'identité avec `leaf.cpp`. Seuil à écrire d'avance : comptage ≤ 1/3 du témoin.
2. **c(L) sur l'arbre v11** (tests G1 par taille de liste) et coût par nœud à W1. Jamais mesuré ; c'est la condition
   d'un parcours GPU.
3. **Microbanc du BFS GPU** : identité de l'ensemble final des feuilles (sites, boîtes) avec le CPU.
4. **Fréquence des bandes F4** (voisins non certainement ordonnés) et nombre de rangs distincts, pour dimensionner le
   tri et les Levels sur l'appareil.
5. **Transferts** épinglés contre pageables aux tailles K5/K10 ; coût de session (contexte, chargement des modules,
   attente active ou bloquante).
6. **Différentiel v10/v11 en même session** sur le catalogue par sous-étage, avec T6 contre T0 en u21 et histogrammes
   de feuilles.
7. **Décisions préalables** : régime (résident à 10 Hz ou processus neuf), contrat en latence ou en cadence
   (recouvrement entre trames), périmètre FULL, profil.

---

## Annexe — chiffres de référence (ng00 sauf mention)

| Grandeur | K5 | K10 |
|---|---|---|
| `domain` à chaud, GPU (f24) / CPU (f16) | 138 / 199 ms | 490 ms (GPU) |
| Frontière + fin | 48,6 ms (GPU) ; 46,9 ms (CPU) | 141,3 ms |
| Parcours / feuilles | 37,5 / 51,8 ms | 141,3 / 207,2 ms |
| Boules / incidences | 1 306 696 / 6 097 121 | 5 512 670 / 45 383 538 |
| Feuilles | 353 456 (f16) ; 123 581 (f24) | 530 259 (f24) |
| Tests G1 | 379 M (f16) ; ≈ 248 M (f24) | 1 229 M (f24) |
| Mémoire de l'appareil | 243 Mo | 1,42 Go |
| `domain` W1 (f16) | 5,3 s | — |
| CPU·s par FULL (GPU / CPU) | 9,36 / 12,17 s | 52,3 s (GPU) |

Commits clés vérifiés :
- `ef75dafac` lourds d'abord et LPT ; `9b9244a00` lemme R ; `ec55578d9` M3/E4 ; `56216392e` q3 différé ;
  `0c358261c` compteurs R1 ;
- `82fff7543` et `00800dd88` voie GPU ; `34a8a561d` J2 mémorisé ; `79fa5e9f7` réservoir ; `5861c223f` levier C ;
  `2045ec27c` exécuteur partagé ; `ccdd4db75` cache de blocs ;
- retraits : `830473218` L4 ; `c1675e4c9` N1 ; `d4228f5e5` coopérative ; `64746f985` frontière par tranches ;
  `23b759dfe` G1 AVX2.
