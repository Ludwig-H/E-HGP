# Contre-vérification adverse : corpus des auditeurs (toutes versions)

4 octobre 2026, rédigé entre 13 h 16 et 13 h 25 UTC (`date -u`). Contre-vérificateur de
[`../fouille/auditeurs.md`](../fouille/auditeurs.md) pour l'audit des transpositions (`../CONTEXTE.md`).
**Rappel de l'utilisateur : l'objectif du contrat reste 100 ms** (FULL K = 5, trame sans sol de 30 000 à
60 000 sites, G4 48 fils ; K = 10 si possible).

```text
phase=exploration_v11_hors_registre (contre-vérification, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Méthode. Lecture de `origin/main` depuis `build/v11-claude-20261003` (`git show`), des notes v9/v10 citées, des deux
cartes (`../cartes/`), et du diff non commis du développeur dans ce worktree. Un seul calcul rejoué :
`python3 -B ../fouille/auditeurs_pts4_borne.py` (code 0, 13 h 17 UTC), plus deux petites lectures Python des JSON de
`pts4_review_20261003/case_metadata.json.gz`. Étiquettes : **M** mesuré (reçu nommé), **E** estimé, **C** conjecturé.

## 0. Le dépôt a bougé pendant la fouille (fait nouveau, lu à 13 h 16 UTC)

La fouille lit `origin/main` = `2b1abb6a5` et cite deux travaux « non publiés ». Depuis :

| Commit / état | Contenu | Effet sur les verdicts |
| --- | --- | --- |
| `17514012b` (13 h 06 UTC) | les réponses R1–R7 des auditeurs sont **publiées** dans `audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md` (§ « Réponses R1–R7 ») et `receipts/audit_selfreview_20261004/` (`counter_arena/`, `cell_memo/`, `subgrid_bounds/`, `flat_verdict/`, …) | [WIP-A] n'est plus un brouillon : les citations des idées 04, 05, 07, 08 sont vérifiables sur `origin/main` |
| `56216392e` (13 h 13 UTC) | **q3 différé porté** : `Q3Candidate` (`src/num/geometry.hpp`), `leaf.cpp`, `sphere.cpp`, `meb.cpp`, `predicates.cpp`, mutants, `tests/num/q3_candidate_test.cpp`, `tests/tower/meb_deferred_test.cpp` ; « FULL dumps byte-identical on ng00 and ng02 », « G4 A/B to follow » | idée 06 déjà dans la v11 |
| diff non commis du worktree développeur | `bench/points_flat_gate.py` (`LINES` avec `('eom', 2)`, fixtures `F4_z2`, `F4b_z1/z2`), `bench/points_flat_claims.py` nouveau (`h_l2_claim` : Holm **et** borne IC > −0,02, fixtures −0,021 et −0,02), `points_flat_summary.py`, porte CTest `mhgp11_tower_points_flat_claims` | idée 08 en cours de correction, pas encore sur `origin/main` |

## 1. Verdicts

| Id | Verdict | Une ligne |
| --- | --- | --- |
| auditeurs-01 | **garder** | comptes rejoués à l'identique ; la cible effective tombe vers 54–77 ms sur 08/000000 ; une session G4 de mesure, aucun risque moteur |
| auditeurs-02 | **a_mesurer** | le lemme est prouvé ; le gain ne l'est pas, il est surestimé (publication et balayage sont déjà en pipeline) et le mode par lots perd le recouvrement |
| auditeurs-03 | **a_mesurer** | la ventilation par arité manque bien ; le q2 couplé seul vaut ≤ 1–2 ms |
| auditeurs-04 | **a_mesurer** | constat exact dans le code ; gains conjecturés, la prémisse « ×25 → ×30 » n'est pas démontrée |
| auditeurs-05 | **a_mesurer** | certificat juste ; l'attribution des +9 % de pas au mémo est une estimation, et les pas évités peuvent être des succès de table bon marché |
| auditeurs-06 | **ecarter** | déjà porté par `56216392e` ; reste l'A/B G4 annoncé par le développeur |
| auditeurs-07 | **a_mesurer** | contrat d'exactitude publié (R7) ; aucun chiffre GPU v11, historique défavorable ; dernier recours |
| auditeurs-08 | **garder** | défaut vérifié sur `origin/main` ; correction en cours non commise ; reste à publier et rejouer sur G4 |
| auditeurs-09 | **ecarter** | hors du contrat 100 ms ; c'est un chantier v11 déjà écrit par les auditeurs v11, pas une transposition |

## 2. Examen idée par idée

### auditeurs-01 — Borne haute du contrat (50 000–60 000 sites, plusieurs séquences, maximum)

**Preuve rejouée (M).** `auditeurs_pts4_borne.py` rend exactement les chiffres de la fouille : 127 trames c08,
32 462–98 560 sites, 85 au-dessus de 60 000 ; nœuds d'ordre 5 ∝ n^1,386 ; boules K10 ∝ n^1,439 ; [50 000, 60 000] :
5 trames, médiane ×1,30, maximum ×1,87 (c08_003412, 58 418 sites, 1 076 856 nœuds, 11 066 031 boules K10 = ×2,01).

**Absente de la v11 (lu).** `docs/DEVELOPPEMENT.md` l. 64 : « Aucun résultat K10/GPU/multi-millions/plusieurs
séquences/points/HDBSCAN acquis » ; l'audit v11 publié : « 100/200 ms, GPU, temps sur plusieurs séquences, massif et
points natifs restent ouverts » ; aucune trame > 45 845 sites dans les chronos [Q]/[AB7].

**Objections adverses, sans renverser l'idée.**

1. c08_003412 est un **cas atypique** : 18,4 nœuds d'ordre 5 par site, contre 13,3–14,4 pour les quatre autres trames de
   [50 000, 60 000] (c08_000502 : 755 016 / 54 329 ; c08_001518 : 751 226 / 52 890 ; c08_001881 : 701 462 / 52 132 ;
   c08_002992 : 719 264 / 55 777). Le ×1,87 est un vrai maximum de l'échantillon, mais la masse de la plage est à
   ×1,22–1,31.
2. La référence 08/000000 est elle-même **lourde pour sa taille** (576 371 / 39 885 = 14,45 par site) : c08_000054,
   39 873 sites, n'a que 465 522 nœuds (11,7 par site). Le facteur dépend du choix de la référence.
3. Masque : la préparation (`bench/points_lidar_prepare.py`, l. 12–15) compile la sonde Patchwork++ v8 épinglée et
   vérifie le masque de 08/000000 contre l'empreinte v8 `9db3fe5c…` : c'est le masque du contrat. Mais PTS4 a tourné
   sur un instantané `e26b48055`, **antérieur** au commit du script (`6c88fe0ed`) : provenance probable, non épinglée.
4. « Temps ∝ nœuds d'ordre 5 » reste **E** : le catalogue suit les boules K5 (non publiées par PTS4), la résolution les
   pas et census ; les durées PTS4 (22 processus × 4 fils) ne disent rien du chrono W48.
5. Constat de portée à remonter à l'utilisateur, non à trancher ici : sur cet échantillon de la séquence 08 (choisi pour
   ses objets), **85 trames sur 127 dépassent 60 000 sites sans sol** ; la borne « 30 000–60 000 » du contrat exclut donc
   une bonne part des trames réelles de cette séquence.

**Gain révisé.** Aucun gain de vitesse : la cible se corrige. Sur 08/000000 : ≈ 77 ms (médiane 50–60 k) à ≈ 54 ms
(pire trame), soit ÷5,4 à ÷7,7 depuis 412 ms au lieu de ÷4,1 (**E**). **Mesure à faire** : une session G4 FULL K5 W48,
cinq prises, en ajoutant c08_003412, c08_001518, c08_002992 et deux ou trois trames d'autres séquences, préparées par
`points_lidar_prepare.py` épinglé, avec les compteurs de travail ; publier le maximum.

### auditeurs-02 — Publication sans barrière de plateau (lemme du maximum d'ID, v9)

**Preuve lue.** `morsehgp3D_v9/audits/PHASE_A_MAX_ID_COMPOSANTE_20260923.md` existe et dit bien ce qu'on lui prête :
lemme avec **preuve par induction** sur les plateaux (§ « Lemme du maximum d'ID »), contrôle combinatoire sur
3 000 historiques abstraits (`check_phase_a_temporal_max_id_20260923.py`), « pas qualification de FULL », et « Ce n'est
pas encore un résultat de performance v9 ». La note exige aussi que le coût de la hiérarchie de composantes soit
« publié séparément ; la seule forêt minimale ne livre pas automatiquement ces requêtes ».

**Compatibilité v11 (lu, `src/tower/forest_plateau.cpp` l. 70–94, `forest_build.cpp` l. 300–330).** Naissances
pré-numérotées dans [0, b) par blocs (`birth_block`) ; chaque fusion prend `NodeIdx{result.count_}` croissant. Une
composante sans fusion ne contient qu'une naissance (une continuation n'ajoute pas de nœud), donc le maximum des
marques est bien la racine. **Différence à reprendre** : dans un plateau, la v11 numérote les groupes dans l'ordre des
indices de racine DSU (`forest_sort(touched…, a < b)`, l. 71), pas « par premier ordinal de bloc » comme la v9.
L'identité des IDs exige de reproduire **cette** règle.

**Absente de la v11 (lu).** `forest_concurrent.cpp` l. 273–274 : `parallel_for(kmax, 1, &publish…)`, une tâche par
ordre ; aucune occurrence de Borůvka ni de maximum d'ID dans `src/`.

**Le gain annoncé est surestimé.**

- Publication et balayage vertical **ne s'additionnent pas** : `forest_pipeline.cpp` l. 86–94, le suiveur
  `follow(upper)` consomme la progression des ordres `upper − 1` et `upper` pendant leur publication. À W48 b872, la
  queue des verticales vaut déjà 0,0 ms (CARTE_V11 § 2.1). La chaîne exposée par une résolution ÷3 est donc de l'ordre
  du maximum des deux (publication de l'ordre 5 : 34–46 ms de travail à W1), plus un retard, **≈ 35–50 ms (E)**, et non
  « ~60–90 ms ».
- Une publication par lots (Borůvka sur toutes les arêtes de l'ordre) **attend la fin de la résolution de cet ordre** :
  elle perd le recouvrement que le pipeline offre aujourd'hui. Chemin critique = résolution + forêt parallèle +
  hiérarchie + verticales. On ne gagne que si cette somme passe sous le maximum actuel (résolution, publication
  séquentielle).
- Le coût de la hiérarchie de composantes (arbre de Kruskal parallèle sur E ≈ 1,35 M traces, V ≈ 0,58 M nœuds) n'est
  chiffré nulle part ; « 5–10 ms par chaîne » est **C**.

**Ce qui reste solide.** Une chaîne séquentielle de 34–46 ms ne tient pas dans une cible effective de 54–77 ms
(idée 01) : ce levier sera nécessaire si le reste descend. **Mesure à faire** : sidecar G4 sur les mêmes catalogues
(ng00, ordre 5) : forêt minimale + hiérarchie + IDs à W48, identité octet pour octet des nœuds, parents, `next`,
ancres, contributions, populations et verticales ; poursuivre seulement si le total reste sous ~10 ms. Pas une piste
fermée.

### auditeurs-03 — Census des descentes : ventiler par arité, puis q2 couplé

**Preuve lue.** `receipts/audit_heritage_20261004/q2_coupled/README.md` : 2P(z) = Σ(2z_j − C_j)² − S, exemple P = 142
contre 2P = −36, modèle 17 → 13 bornes et 8 tests ponctuels inchangés, 633 gardes. Le reçu dit lui-même : « Les
captures actuelles ne ventilent pas l'arité : aucun gain ou pourcentage de temps annoncé. »

**Absente de la v11 (lu).** `src/index/census_workspace.cpp` l. 47–64 : parcours depuis la racine, `power_bound_signs`
à chaque nœud ; le ledger compte déjà `passes`, `nodes`, `bounds`, `outside_blocks`, `inside_blocks`, mais
`bench/full_probe.cpp` l. 118–138 ne publie que `census_calls` et `census_point_tests`, et rien par arité.
`predicates.cpp` : `bound_terms` l. 108, `center_power_bounds` l. 146 (lignes décalées par `56216392e`, contenu identique).

**Doctrine.** Respectée si le helper reste privé, tag 2 par fabrique, sans publier 2P comme `PowerBounds`.

**Gain révisé.** (a) Aucun en soi. Utile parce que le census coûte ≈ 3,9 µs par appel en v11 contre ≈ 1 µs en v10 pour
le même nombre d'appels (CARTE_V10_VITESSE § 3.4, **E**), et que ce surcoût vient du parcours (nœuds et sites
visités), pas de l'arithmétique (F6 ≈ 1 %, retiré) : il faut publier nœuds/bornes/blocs **par arité** avant de
choisir. (b) q2 couplé : ≤ 0,2–0,6 % du CPU, ≈ 1–2 ms à W48 (**E**), pas un levier des 100 ms. **Mesure à faire** :
une prise G4 FULL K5 W48 sur ng00–02 avec ledger census publié et ventilé par arité (q1/q2/q3/q4) ; A/B du q2 couplé
seulement si les sphères q2 dépassent ~30 % des bornes visitées.

### auditeurs-04 — Compteurs locaux et arènes dans la boucle chaude (R1, R3)

**Code vérifié sur `origin/main` (après `56216392e`).** `leaf.cpp` : `checked_add` l. 41, 65, 69, 84–93, 161, 166,
202–203, 214, 261, 267 ; `boxes.cpp` (`filter`) : `storage.allocate(parent.size(), …)` par nœud ;
`core/buffer.cpp` : `budget_reserve` (CAS sur `used`, CAS sur `peak`) puis `::operator new`, et `fetch_sub` à la
libération. Le constat tient.

**Preuves.** La capsule `counter_arena/normal.json` (publiée par `17514012b`) donne `checks: 2313`,
`native_execution: false`, et pour m = 1024 : census/incidences 46 821 361 844 224 < 2^46 = 70 368 744 177 664,
préfixes 45 723 987 200. La borne de non-débordement est donc vérifiée ; le gain ne l'est pas.

**Objections.**

- Le ×1,371 de la feuille J3 v10 (`CONTRE_AUDIT_PROTO_CPU_20260929.md` § 1) mesure une **réécriture entière** de la
  feuille (SoA, masques, census par masque), pas le retrait des compteurs : ce n'est pas une preuve pour R1.
- Les ≈ 0,5 milliard d'additions vérifiées par trame (E) à 1–3 cycles chacune donnent ≈ 0,15–0,5 s de CPU W1, soit
  ≈ 3–10 % de la passe unique (4,75 s) : **≈ 5–17 ms à W48 (E)**, sous l'hypothèse que les compteurs ne sont pas déjà
  gardés en registre par le compilateur.
- Arènes : les indices W48 (`buffer_acquire` + `buffer_release` 0,86 %, fautes de page 3,1 % en cumul, verrou noyau
  1,55 %) se recouvrent en partie ; la prémisse « passage à l'échelle de ×25 à ×30 » n'est pas démontrée, la carte v11
  (§ 2.3) attribuant le plafond de la passe au SMT (×3,3–3,7 de W8 à W48 pour ×3 cœurs). Gain plausible **≤ 5–8 ms
  (E/C)**, pas 25 ms.

**Mesure à faire.** Deux A/B G4 séparés, passe unique et FULL, W1 et W48, ledger et dumps identiques octet pour octet :
(1) accumulateurs locaux à la feuille avec un flush `checked_add` ; (2) `Buffer` privé par tâche (marque/rewind),
pic des réservations publié. Pas une piste fermée.

### auditeurs-05 — Mémo de cellule typé par sa date de validité (R5)

**Preuve lue.** Réponse R5 publiée (AC11 § R1–R7, point 5) et `cell_memo/` (30 gardes Fraction) : certificat exact sur
modèle Γ2, pas un cache natif. La v11 a bien `DescentMemo`, hors de 16379, et le pipeline est refusé s'il est actif
(CARTE_V11 § 5.1, `pipeline_lanes`).

**Objections.**

- « +9 % de pas faute de mémo » est une **attribution** de la carte v10 (§ 3.4), pas une mesure : la v11 a d'autres
  chemins (table I∪U consultée à chaque pas, 3,62 M succès sur 4,80 M pas sur ng00).
- Les ≈ 0,4 M pas évitables seraient en bonne part des pas bon marché, déjà servis par la table : le coût moyen de
  0,6–0,7 µs par pas surestime l'économie.
- Un partage concurrent doit coexister avec le pipeline à 39 résolveurs : un nouvel objet partagé sur le chemin le plus
  chaud de la tour.

**Gain révisé.** ≤ 8 % de la résolution, soit **≤ 7–9 ms à W48 à K5 (borne haute, E)**, probablement moins ; plus
utile à K10. **Mesure à faire** : d'abord un compteur G4 (sans cache) du nombre de descentes dont (cellule, ordre) est
déjà résolu avec λ_b valide, sur ng00–02 à K5 et K10 ; ne porter que si ces répétitions dépassent ~5 % des pas
**coûteux** (hors succès de table).

### auditeurs-06 — Niveau q3 différé

**Déjà dans la v11.** `56216392e` (13 h 13 UTC) : `Q3Candidate` porté au catalogue et à la MEB bornée, mêmes compteurs,
dumps FULL identiques sur ng00/ng02 (local), mutants tués, A/B G4 annoncé. Le gain borné (≤ 1–1,5 % du CPU W1,
`Sphere::through` = 2,37 % [AB7]) est de toute façon hors de l'échelle des 100 ms. **Écarté comme transposition** ;
l'A/B G4 du développeur suffit.

### auditeurs-07 — Route GPU sous le contrat d'exactitude (R7)

**Preuve lue.** R7 publiée (AC11, point 7) : « Exact device, ou `unresolved` repris exactement sur CPU avant
admission ; un débordement/refus n'est jamais un rejet géométrique ». Historique v9 « cinq ports GPU … jamais plus
d'environ 10 % de bout en bout » (`AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md`) ; l'estimation de la feuille
sur GPU à 17–52 ms (L05) n'est pas vérifiée ; aucune option CUDA en v11.

**Lecture.** Ce n'est pas une idée de gain mais un cadre ; le gain est inconnu (**C**). Les routes GPU fermées
(`docs/archive/abandoned/README.md`) le sont pour leurs mécanismes (vagues pilotées par l'hôte, relances par paire) :
une route résidente n'est pas fermée. Si l'idée 01 confirme une cible de ÷5 à ÷8, les leviers CPU listés ici
(02 à 05, ≤ 10–20 ms chacun) ne suffiront pas, ce qui rend cette voie plausible en dernier recours.

**Mesure à faire.** Un banc G4 isolé : noyau de feuille (≤ 16 sites à K5, q2/q3/q4, i64/i128, `unresolved` repris sur
CPU) sur les feuilles exportées de ng00, transferts et canonicalisation compris, comparé à la passe unique CPU
(167–180 ms à W48) ; abandonner si le total device ne passe pas sous ~40 ms.

### auditeurs-08 — Sortie plate E1 : bras z = 2 et borne IC de H_L2

**Défaut vérifié sur `origin/main`.** `bench/points_flat_gate.py` l. 38 : `LINES = (('eom', 1), ('eom', 3),
('leaf', 1))` ; `bench/points_flat_summary.py` l. 220 (`T_eom2` primaire de H_L1) et l. 237
(`rows[name]['claimed'] = adj < 0.05`, sans IC). Le témoin des auditeurs est publié (`flat_verdict/`, 89 gardes ;
« Deux conditions avant les campagnes E1 »).

**Correction en cours, non commise** (worktree développeur) : z = 2 ajouté aux `LINES`, fixtures `F4_z2` et
`F4b_z1/z2` (seule fixture où z = 2 se distingue de z = 1), `points_flat_claims.py` avec `h_l2_claim` (Holm < 0,05
**et** borne basse > −0,02 strictement) et fixtures −0,021 / −0,02, porte CTest stdlib `checks10`.

**Verdict garder** : utile, bon marché, préalable à P08. Reste à faire : commit, puis rejeu G4 de la porte plate
contre l'oracle avec le bras z = 2 avant les mesures primaires. Aucun gain de vitesse.

### auditeurs-09 — Port natif de la hiérarchie de points et de la tête plate

**Preuve lue.** Temps Python relus dans `case_metadata.json.gz` pour c08_000054 (claudepts3) : `margin_r` 3,086 s à
k = 5 et 7,417 s à k = 10 ; HDBSCAN 4,215 s à k = 5 (sous contention, 22 processus). Le contrat Q8 (≤ 2n − 1 nœuds, DP,
`PointRadiusDate`, budgets 8192 bits avec refus) est **un texte de l'audit v11 lui-même** (AC11 § « Contrat natif
encore à construire ») ; `bench/points_radius.py` utilise déjà LCA, classes de radicaux et `sign_of_radicals(…,
budget=8192)` avec `Refusal` (l. 61–117).

**Écarté comme transposition** : hors du chrono FULL et du contrat 100 ms ; rien à reprendre d'une version
antérieure qui ne soit déjà dans le plan v11 ; le gain natif « ≥ ×10 » est **C**. Le chantier reste légitime pour la
priorité points/HDBSCAN, sous son propre registre.

## 3. Synthèse pour le contrat de 100 ms

- Le seul apport de cette source qui **change la décision** est l'idée 01. Si la trame la plus lourde de la plage compte
  ×1,3–1,9 du travail de 08/000000, il faut ÷5 à ÷8 et non ÷4.
- Aucune autre idée de ce corpus ne rapporte plus de ~10–20 ms à W48 selon les estimations révisées (02 : jusqu'à
  ~35–50 ms de chaîne séquentielle à casser, mais seulement une fois la résolution divisée ; 03 : ≤ 2 ms ; 04 : ≤ 5–17 ms
  puis ≤ 5–8 ms ; 05 : ≤ 7–9 ms ; 06 : déjà fait).
- Ordre de mesure proposé sur G4 : (1) la matrice élargie (01), avec le ledger census ventilé (03a) et le compteur de
  répétitions (05) dans la même prise ; (2) les A/B compteurs et arènes (04) ; (3) le sidecar de publication (02) ;
  (4) le banc GPU (07) seulement si (1)–(3) laissent un écart supérieur à ÷2.

FIN
