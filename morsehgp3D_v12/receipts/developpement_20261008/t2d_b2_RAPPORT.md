# T2-d-B2 : ouverture et coût interne de l'étage G dans la Session recouverte

Rapport de l'agent du chantier B, rédigé le 8 octobre 2026 entre 10:35 et 13:08 UTC, heures lues par `date -u`. Le
journal complet, minute par minute, est `../RAPPORT.md` (section « Tranche T2-d-B2 »).

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 pour le catalogue
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé
```

**Base : main `72f622a55`**, sur consigne du coordinateur après la session M. Une variante du lot couvre la tête
courante de `main` : `fd84039c0`, qui intègre A6, et `6497ed3b5`. **Rien n'a tourné sur G4 : aucun gain n'est
qualifié.**

## Mesure d'abord

Profil de G par poste (sonde de G profilée, `MHGP12_TOWER_PROFILE`) sur kitti_ng_08_002119 (99 099 sites), K5,
3 fils, passe 2. Ce sont des temps-fils, gonflés par la contention (charge 15 à 24 pour 8 cœurs).

| poste | ms | part |
| --- | ---: | ---: |
| t1 (LEM-T1 : `find_support` puis F dans P_b) | 6 004 | 27 % |
| trace (formation des représentants) | 3 395 | 15 % |
| census saturé | 3 121 | 14 % |
| sonde (index des naissances) | 2 943 | 13 % |
| proposition (DWelzl) | 2 327 | 10 % |
| census complet | 1 866 | 8 % |
| arrêt (cible de fenêtre) | 1 642 | 7 % |
| certificat | 844 | 4 % |

L'index des naissances de l'ouverture prend 535 à 683 ms par passe à 3 fils. À cette charge, les temps locaux de
bout en bout ne départagent pas des leviers de quelques pour cent.

Leviers examinés et **non retenus**, mesure à l'appui :

- **t1, le premier poste.**
  - Son coût vient de la dichotomie indirecte de `find_support` (environ six défauts de cache dépendants) et de la
    CSR privée du catalogue.
  - Une table S* → boule côté tour coûterait 100 à 200 Mo et autant de construction que les index.
  - C'est un travail de fin d'étage du catalogue (chantier C).
- **Mémoire de census.**
  - 29 % des censuses sont des répétitions.
  - Mais une mémoire déterministe à 1 comme à 48 fils reste dans une tranche de 256 cellules.
  - Elle n'éviterait qu'environ 7 % des censuses, soit environ 1,5 % de G.
- **Préchargement de la trace.** Impossible sans l'API privée de la CSR.

## Leviers livrés

| levier | patch | fichiers | effet attendu |
| --- | --- | --- | --- |
| **B2-T** : index des naissances de tous les ordres construits ensemble | `t2d_b2_T_tables.patch` | `populations.cpp`, `pipeline.cpp` (`build_tables`), `radix.cpp`, `internal.hpp`, portes, mutants | environ 11 invocations du Pool au lieu de 44 à K5 (99 à K10) à l'ouverture de `build_tower` ; rien de visible à 3 fils |
| **B2-S** : seconde recherche du support évitée | `t2d_b2_S_relecture.patch` | `resolve.cpp`, `internal.hpp`, porte, mutants | une `find_support` de moins par passage au census (252 152 par passe sur ng00 K5) : environ 1,5 à 2 % de G |
| **B2-C** : census à plat | `t2d_b2_C_census.patch` | `num/guard.hpp`, `num/guard.cpp`, `index/bounds.hpp`, `index/census.cpp`, `index/census_workspace.cpp`, porte, mutants | census à **0,54–0,57** du temps de la base (microbanc stable), 0,62 de ses instructions ; environ −9 % du temps-fil de G |
| pilote G4 | `t2d_b2_pilote.patch` | `microbancs/mes_t2d_b2/` | |
| **lot** = B2-T + B2-S + B2-C + pilote | `t2d_b2_lot.patch` | 20 fichiers | |
| variante du lot sur `fd84039c0` | `t2d_b2_lot_sur_fd84039c0.patch` | même contenu ; `tower.json` recomposé (48 + 9, plancher 57) | |

Chaque patch s'applique seul sur `72f622a55`. La variante s'applique sur `fd84039c0` et sur `6497ed3b5` ; à cette
dernière tête, les trois manifestes et le style sont conformes. Les empreintes sont dans `SHA256SUMS`.

### B2-T et B2-S

- **B2-T.**
  - `PopulationTable::build_all` joue chaque phase en une invocation du Pool pour toutes les tables : comptage,
    remplissage, tri par base, répertoire, tri des seaux, fiches.
  - `build` est le cas d'une table et `radix_sort_many` généralise le tri par base, si bien qu'il n'y a qu'une
    implantation.
  - Refus inchangés : une tranche en échec marque sa table ; l'issue est celle du plus petit ordre en échec.
  - Écart déclaré au confinement demandé : `radix.cpp` et `internal.hpp` sont touchés aussi.
  - Un défaut de la première version (un tableau inactif recevait un résultat vide) a été trouvé par les portes
    existantes, puis corrigé ; il est fixé par un mutant.
- **B2-S.** `lem_t1` signale quand la table vient de répondre « absent » pour le support proposé. `locate` saute
  alors la seconde `find_support` si le support certifié égale le support proposé.

### B2-C : census à plat

**Diagnostic.** L'algorithme de `noyau_g.hpp` est celui du produit : même parcours préfixe sans pile, même
arithmétique i128. L'écart tient aux enveloppes. Callgrind sur 40 000 requêtes de ng00 :

| poste du census de base | instructions |
| --- | ---: |
| `power_sign` (hors ligne) | 294 M |
| corps du parcours | 242 M |
| `bound_signs` (hors ligne) | 179 M |
| `side` (hors ligne) | 94 M |
| `Point::make` (hors ligne) | 20 M |
| total | 829 M |

S'y ajoute un `Result` construit puis relu par nœud et par site.

**Ce qui change.** Il n'y a qu'une implantation, sans seconde voie :

- **Formes à plat.** `GuardedSphere::bound_signs(Box, signs, ledger)`, `side_site(x, y, z, side, ledger)` et
  `power_sign` rendent une `Outcome`. Elles écrivent leur valeur dans un argument et sont en ligne
  (`[[gnu::always_inline]]`).
- **Voies.** La puissance des voies native et certifiée se calcule en ligne, même formule. `slow_sign`, hors ligne,
  garde la préparation cassée, l'essai contrôlé et le repli large, inchangés. Mêmes compteurs de voies, mêmes refus.
- **API publique.** Les formes `Result` (`bound_signs(Box)`, `side(Point)`, `side_offset`) sont des enveloppes de
  ces mêmes fonctions.
- **Parcours.** Les deux parcours (`census.cpp`, `census_workspace.cpp`) ne construisent plus de `Result` par nœud
  ni par site. Un site se lit par ses coordonnées.
- **Inchangés.** La voie générique (`Point::make`, `LatticeSphere`), le verrou par requête, le stockage, la coquille
  et le rappel.
- **Norme.** La norme i64/i128 est choisie statiquement aux profils 21 et 24 : l'étendue d'un support de sites ne
  dépasse pas B.

**Réponse à la note de l'auditeur** (`receipts/audit_reponses_20261008/b2_census_raccord`, `4adbedb1e`, sur une
capture de ma première version) :

- **Entrée par coins bruts : retirée.** Le parcours passe la `Box` du nœud, qui garantit lo ≤ hi (précondition de
  `std::clamp`) et le profil.
- **Domaine du site : vérifié.** `side_site` rend `coordinate_out_of_domain` au-delà de `kCoordMax`, le refus de
  `Point::make`. Le test est un OU et une comparaison, et il est vide au profil 32.
- **Mutant remplacé.** Celui qui violait la précondition de `std::clamp` a été remplacé.
- **Façades publiques ordinaires** : conservées.

**Mesures locales.** Les requêtes sont réelles, capturées pendant G sur ng00–02 K5 (252 152 / 188 039 / 182 299,
à témoins), et rejouées par `outils/census_bench_registre.cpp`.

| trame | base (ns/req.) | B2-C (ns/req.) | rapport | saturé | complet |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 | 2 908 | 1 650 | **0,567** | 0,580 | 0,582 |
| ng01 | 2 843 | 1 526 | **0,537** | 0,563 | 0,568 |
| ng02 | 2 672 | 1 511 | **0,566** | 0,572 | 0,568 |

- Conditions : un fil, minimum de 5 répétitions, médiane de 3 tours alternés, stables à ±5 % malgré une charge de
  11 à 15.
- Chaque `CensusLedger` entier (14 champs), le genre, I et U entrent dans l'empreinte requête par requête :
  **mêmes empreintes** que la base.
- Callgrind : 829 M → 513 M d'instructions (0,619).
- Le microbanc G-APP donnait 0,570 à 0,585 : le census du produit rejoint la source unique.

**Oracle G-APP** (`microbancs/mes_g_appareil`, voie hôte, lié à la bibliothèque de la variante) :

- **zéro écart** de census à l'octet entre le produit à plat et `noyau_g.hpp`, sur 252 152 (ng00 K5), 290 357
  (médiane K5), 646 621 (maximum K5) et 1 765 151 (ng00 K10) requêtes ;
- sondes et propositions sans écart ;
- mutant « côté nul » vu.

## Portes et mutants (local, 3 fils)

**Lot final** (72f622a55 + B2-T + B2-S + B2-C + pilote)

- `ctest -LE long` : **727 sur 727**, aucune porte sautée.
- Construction sans avertissement ; `check_style` `style_ok fichiers=329`.

**Identité FUL1 contre la base**

- Conditions : Session recouverte, sonde par défaut avec le cache de blocs, `--digest`.
- Cas : ng00–02 K5 et K10 ; uniformes 8 000 / 16 000 / 32 000 K5 ; kitti_ng_08_002119, kitti_ng_00_001896,
  kitti_ng_00_000648 K5.
- **12 cas sur 12 identiques** pour chacun de :
  - lot B2-T + B2-S ;
  - B2-C seul ;
  - lot final ;
  - variante sur `fd84039c0`.

**Déterminisme 1 contre 8 fils** (lot final)

- `g_determinism` ng00 K5 rend la ligne de la porte LiDAR (`e5a81154fb1b15f1`) ; ng02 rend `d66647a043f682da`.
- FUL1 de la Session identique sur ng00 K5 et u32000.

**Leviers seuls**

- B2-T seul, B2-S seul : portes ciblées 38 sur 38.
- B2-C seul : 149 portes num et index au profil 21.
- B2-C aux profils 24 et 32 (`MHGP12_MODULES=num;index`) : 151 sur 151 à chacun.

**Variantes**

- Sur `9815c19b9` : 241 portes ciblées sur 241.
- Sur `fd84039c0` : 245 sur 245, portes de A6 comprises, et identité 12 sur 12.

**Nouvelles portes**

- `mhgp12_tower_index_build_all` : 91 contrôles.
- `mhgp12_tower_unit_lem_t1_table` : 200 contrôles.
- `mhgp12_num_local_guard_site_lanes` : 165 contrôles au profil 21. Elle couvre les quatre voies, le refus hors
  profil et des registres égaux à ceux de `side_offset`.

**Mutants : 23 tués sur 23, tous par code**

- B2-T et B2-S (9) :
  - tables : `tables_refus_ignore`, `tables_tranche_non_marquee`, `tables_derniere_en_echec`,
    `tables_masque_ignore` ;
  - tri par base : `radix_many_sans_echange`, `radix_many_chiffre_decale`, `radix_many_inactif_vide` ;
  - relecture : `relecture_sans_egalite`, `lem_t1_manque_toujours`.
- B2-C, num (6) :
  - nouveaux : `plat_site_hors_pave_sur_sphere`, `plat_voie_en_ligne_partout`, `plat_voie_non_comptee`,
    `plat_site_hors_domaine_admis` ;
  - réancrés : `garde_boite_partielle_rejetee`, `garde_coin_proche`.
- B2-C, index (8) :
  - nouveaux : `plat_site_permute_emprunte`, `plat_site_permute_possede`, `plat_registre_garde_absent` ;
  - réancrés : `contact_exterieur`, `contact_interieur`, `coquille_limitee_a_k`, `census_borne_continue`,
    `census_emprunte_borne_continue`.
- Un survivant au premier passage : `plat_site_permute_emprunte` passait sa porte `borrowed_fixtures`, dont les
  fixtures sont symétriques par permutation des axes. Avec la porte `guarded_fixtures`, il est tué.
- Planchers des manifestes : num 80, index 25, tour 48 sur 72f622a55 (57 dans la variante).

## Mesure G4 proposée (non jouée)

**Plan `plan_t2d_b2.json`.**

| commande | plafond |
| --- | ---: |
| construction par défaut | 900 s |
| `socle_ctest` | 600 s |
| `t2d_b2_pilote` | 2 100 s |
| `mutants_index_num_tour` | 600 s |
| cumul | 4 200 s |

- Durée estimée : environ 30 min.
- Données : environ 45 Mo (ng00–02, `g4_kitti_v12set_xyz.tar`, `v12_src_72f622a55.tar.gz`).

**Où partir.** La session peut partir de `72f622a55` + lot ou de `fd84039c0` + variante. Les bras du pilote sont
toujours reconstruits depuis l'archive épinglée de 72f622a55. Le lecteur FULL durci de `main` admet les sorties de
leurs sondes : harnais local rejoué depuis l'arbre de la variante, voie CPU.

**Mesure décisive : le mur FULL de la Session recouverte.**

- B2-T n'agit que dans la Session, à son ouverture, sur le chemin critique. B2-S et B2-C agissent dans les deux
  voies.
- Trames décisives : ng00, ng01, ng02, kitti_ng_02_001606, kitti_ng_08_001176.
- La plus grande trame est publiée en information.

**REGLE_T2D_B2.** Écrite à 09:44 UTC, révisée à 09:48 UTC (base, trames), puis à **11:22 UTC** (bras census, B2-C),
toujours avant toute mesure G4.

- **Bras** : avant (archive 72f622a55, SHA-256 `28c46f46…82b4`), avant_bis (A/A), tables, relecture, census, après.
  - Ils sont reconstruits par substitutions exactes.
  - Les quatre bras substitués, rejoués localement depuis l'archive, égalent les branches.
- **Identité** : FUL1 avec `--digest`.
- **Campagne** : sans `--digest`, K5, `--device`, 48 fils, 10 tours × 8 passes, ordre tournant sans inversion.
- **Statistique** : médiane des passes 2..P ; rapport bras/avant par tour ; moyenne géométrique et IC 95 % par
  bootstrap (10 000 tirages, graine 20261008).
- **Adopté** si l'identité est établie (ng00 = `3a2bfb4f…`) et si la borne haute de l'IC est sous 1 sur chacune des
  cinq trames.
- **Rejeté** sinon.
- **Refusé** dans chacun de ces cas : prise ou identité absente, environnement incomplet ou GPU occupé, bras non
  reconstruit, binaire ou journal changé, auto-test en échec, trop peu de tours, A/A hors de ±1,5 %.
- **Leviers jugés** : `lot_b2`, `tables`, `relecture`, `census`.

**Informations sans verdict**, ajoutées à 11:44 UTC sans changer la règle :

- G séquentiel (`--sequentiel`) sur ng00 et la trame médiane, bras avant, relecture, census et après, 3 tours.
- Raison : B2-S et B2-C agissent aussi dans `resolve_tower`.

**Validation locale.**

- Auto-test du juge : 8 cas.
- Harnais à six bras, voie CPU, deux bouts LiDAR, depuis l'arbre du lot puis depuis la variante :
  - identité établie ;
  - aucune erreur d'admission ;
  - information séquentielle lue ;
  - refus attendus d'un essai.

## Points ouverts

1. **G4 non joué.** Ordres de grandeur que j'avance, sans valeur de règle :
   - B2-C : environ −9 % du temps-fil de G, moins dans le mur FULL ;
   - B2-S : environ −1 % du mur FULL de ng00 ;
   - B2-T : 0 à −3 %, selon le poids réel des invocations à 48 fils.
2. **t1 reste le premier poste** (27 %). Il relève du chantier C.
3. **Norme i64 au profil 32.** Le choix i64/i128 de la norme (s + 2 ≤ 30) n'a toujours ni fixture ni mutant au
   profil 32. Il faudrait un support certifié d'étendue ≥ 30 en voie certifiée.
4. **`main` avance vite.** La variante est rebasée sur `fd84039c0` et s'applique sur `6497ed3b5` ; à la tête
   suivante, `tower.json` sera à recomposer de la même manière (mutants de main, puis les 9 de B2).
