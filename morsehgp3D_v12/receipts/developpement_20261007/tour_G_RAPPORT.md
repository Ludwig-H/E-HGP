# Rapport de l'agent « étage G » (src/tower/, résolution) — morsehgp3D_v12

```text
phase=exploration_v12_hors_registre
backend=cpu_reference
objet=full_pi0
quantification=quantized_u21_input_only (profils 21, 24, 32 compilés et testés)
public_status=not_claimed
GCP non utilisé
```

Base : HEAD de `main` = `6710723399876266d1efd7b45cf0d6adcd49a862` (671072339).
Copie de travail : `v12_tour_G/repo/morsehgp3D_v12` (git archive HEAD).

## Journal (heures `date -u`)

- 2026-10-07 19:03 UTC : début ; copie de travail extraite.
- 19:16 UTC : lectures faites (CLAUDE.md, CONTRAT_TOUR, CONTRAT_NUMERIQUE, ARCHITECTURE § 4.2 et § 5, OBJET § 4,
  constructive.py `resolve_v12`/`_forest_v12`, catalogue.hpp, meb_cert.hpp, welzl_proposal.hpp, descente v11 :
  descent, locate, canonical, population_lookup, cells, cells_classify, meb). `INTERFACE.md` publié (première forme).

## Décisions de conception (au fil de l'eau)

1. **Représentants = toutes les traces strictes** des cellules de fenêtre non naissances (ordre lexicographique des A,
   comme `cells.cpp` de la v11), et non un représentant par morceau comme l'oracle : c'est le compteur d'objet
   « représentants (traces strictes des cellules de fenêtre) » du § 8 (1 350 288 à l'ordre 5 de ng00 K5 = les traces
   de la v11), et le différentiel v11 se fait trace par trace. Les morceaux ne servent qu'au genre jonction / inerte
   (compteur d'objet « cellules inertes »). Même forêt : les traces d'un même morceau sont unies sous le niveau de la
   cellule (A ∪ A' séparable ⇒ (k+1)-partie de niveau inférieur).
2. **Séparabilité par témoins** (coquilles étendues) : X ⊆ U est non séparable ssi X contient un support de la sphère
   (paire de milieu, triangle strictement aigu coplanaire au centre, tétraèdre contenant strictement le centre :
   Carathéodory + minimalité), prédicats exacts de `num` sur le centre de S* ; pas de plus petite boule par partie
   (la v11 testait β(A) < λ par `bounded_meb`). Plafond déclaré : C(m, t) ≤ `kMaxCellCombinations` par cellule, sinon
   refus `cell_capacity`.
3. **Date initiale contrôlée** : la décroissance stricte part du rang de la cellule de la jonction (le premier pas,
   sonde comprise, doit être de rang ou de niveau strictement inférieur) ; contrôle compté (`controls`), et l'étage
   exige `controls = plus petites boules + succès de sonde` (contrôle avant toute sortie par saut).
4. **Canonisation par positions** parmi F ∩ sphère (même convention que S* du catalogue, `CST-0113`), pour que la table
   S* → boule réponde dès que S*(b) ⊆ F.
- 19:21 UTC : catalogues v12 exportés (sonde du catalogue, HEAD) pour ng00–02 à K5 et K10 (`cats/`, jamais versés) ;
  coquilles étendues : au plus m = 5 sites, C(m, t) ≤ 10 par cellule (ng00 K5 : 227 boules étendues, 403 cellules non
  triviales ; ng02 K10 : 1 301 et 2 461). Plafonds déclarés retenus : `kMaxCellCombinations = 4096` (C(m, t) par
  cellule) et `kMaxShellWitnesses = 4096` (supports d'une coquille), soit plus de 400 fois le maximum observé.

## Premiers résultats (sonde `mhgp12_tower_probe`, profil 21, local)

- 19:34 UTC : module compilé (`supports.cpp`, `cells.cpp`, `cells_stage.cpp`, `populations.cpp`, `resolve.cpp`, `stage.cpp`, `proposal.hpp`), sonde `bench/tower_probe.cpp` + exportateur de test `bench/tower_export.hpp`. ng00 K5 : objet à l ordre 5 = 341 081 naissances, 448 805 cellules (107 étendues, 50 inertes), 1 350 288 représentants (= traces de la v11) ; 963 244 arrêts à la première sonde (= v11) ; 482 533 plus petites boules (v11 : 568 919), 186 511 censuses (128 870 saturés, 57 641 complets dont 3 au catalogue ; v11 : 217 325), 296 025 arrêts sur cellule, chaîne max 11. Empreinte de la résolution identique à 1 et 3 fils (`51dec6bb…`, format provisoire de l export, remplacé ensuite). Temps local à un fil : 2,67 s (ordre 5 : 1,58 s) ; réplique v12 du MES-M7 local : 1,255 s à l ordre 5 avec plus de travail : à profiler (non prioritaire, T2-c).
- 19:43 UTC : portes de la tour au vert (construction `-DMHGP12_MODULES=tower`, profil 21) : `mhgp12_tower_unit` (9 groupes : cibles et capacité, WIT-D2, WIT-MEMO, WIT-T1-CARRE côté tour, census saturé du catalogue, pas inerte sous la fenêtre, plafonds `cell_capacity` et WIT-SPHERE50, budget, déterminisme 1 contre 8 fils) et `mhgp12_tower_oracle` (code 0, `g_oracle_ok nuages=342 doublons_refuses=34 cibles=21225 cibles_cellule=2359 cellules=7821 inertes=876`) : chaque cible ÉGALE à celle de `resolve_v12` (politique `v12_indices`) et dans la composante attendue à la coupe ouverte de sa jonction (forêt de l étage B) ; traces = exactement les t-parties séparables ; cellules et genres (inerte) = ceux de la référence. `check_style` (262 fichiers) et `check_constats` verts.
- 19:57 UTC : **différentiel v11 à K5 conforme** (`tests/tower/g_diff_v11.py`, vidages de `mhgp12_vidage` construit dans `vidage_build/` contre `v12_lecteur/v11/build/libmhgp11.a`, SHA-256 `050532a9…` = rapport de `source_v11.py`) : ng00 3 622 258 représentants, ng01 3 012 810, ng02 3 850 037, **tous** dans la composante de la graine v11 à la coupe ouverte de leur jonction, et même **graine identique** pour 100 % (cible naissance = graine ; cible cellule c' = graine v11 de la première trace de c', que la descente de la v11 suit par `strict_trace`) ; naissances, cellules et **traces (masques, ordre)** identiques à la v11 à chaque ordre. Comptes de l étage G (`runs/table_g_k5.md`) :

| trame | k | naissances | cellules (inertes, étendues) | représentants | cibles naissance | cibles cellule | 1re sonde | sondes après pas | plus petites boules (t1 / cert_census) | censuses saturés / complets | sauts catalogue / census | pas inertes | contrôles | chaîne max |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | --- | --- | --- | ---: | ---: | ---: |
| ng00_k5 | 1 | 39885 | 101138 (49, 49) | 202326 | 202326 | 0 | 0 | 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 | 0 | 0 |
| ng00_k5 | 2 | 101089 | 166266 (38, 86) | 410941 | 367367 | 43574 | 354229 | 13138 | 57039 / 2381 | 2381 / 0 | 13465 / 2381 | 0 | 426787 | 5 |
| ng00_k5 | 3 | 166228 | 249527 (34, 72) | 673325 | 558355 | 114970 | 525776 | 32579 | 153027 / 12743 | 12743 / 0 | 29295 / 12743 | 8762 | 724125 | 8 |
| ng00_k5 | 4 | 249493 | 341136 (54, 89) | 985378 | 789015 | 196363 | 729558 | 59457 | 253921 / 50517 | 50142 / 375 | 30364 / 50142 | 27569 | 1093453 | 11 |
| ng00_k5 | 5 | 341081 | 448805 (50, 107) | 1350288 | 1054263 | 296025 | 963244 | 91019 | 296022 / 186511 | 128870 / 57641 | 0 / 128870 | 57638 | 1536796 | 11 |
| ng01_k5 | 1 | 35551 | 89186 (27, 27) | 178399 | 178399 | 0 | 0 | 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 | 0 | 0 |
| ng01_k5 | 2 | 89159 | 142465 (27, 54) | 352018 | 316940 | 35078 | 307013 | 9927 | 45014 / 1993 | 1993 / 0 | 9936 / 1993 | 0 | 363947 | 5 |
| ng01_k5 | 3 | 142438 | 211403 (32, 59) | 566651 | 473150 | 93501 | 448036 | 25114 | 121754 / 10074 | 10074 / 0 | 21976 / 10074 | 6277 | 604978 | 6 |
| ng01_k5 | 4 | 211371 | 283231 (24, 56) | 813237 | 656469 | 156768 | 611533 | 44936 | 199585 / 37770 | 37557 / 213 | 23056 / 37557 | 19974 | 893824 | 11 |
| ng01_k5 | 5 | 283207 | 369751 (25, 49) | 1102505 | 866339 | 236166 | 798081 | 68258 | 236163 / 138202 | 96800 / 41402 | 0 / 96800 | 41399 | 1240704 | 12 |
| ng02_k5 | 1 | 45845 | 118890 (93, 93) | 237873 | 237873 | 0 | 0 | 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 | 0 | 0 |
| ng02_k5 | 2 | 118797 | 182948 (125, 218) | 454626 | 413926 | 40700 | 404240 | 9686 | 50449 / 1841 | 1841 / 0 | 9749 / 1841 | 0 | 466216 | 5 |
| ng02_k5 | 3 | 182823 | 273994 (115, 240) | 728362 | 607831 | 120531 | 583745 | 24086 | 148153 / 9172 | 9172 / 0 | 21048 / 9172 | 6574 | 765156 | 7 |
| ng02_k5 | 4 | 273879 | 361441 (112, 230) | 1035224 | 842407 | 192817 | 794909 | 47498 | 239352 / 34513 | 34279 / 234 | 24849 / 34279 | 21920 | 1116272 | 9 |
| ng02_k5 | 5 | 361326 | 471060 (121, 239) | 1393952 | 1097401 | 296551 | 1025951 | 71437 | 296552 / 136773 | 91606 / 45167 | 0 / 91606 | 45155 | 1530713 | 9 |

- 20:06 UTC : **mutants causaux tués 5/5** (`tests/mutants/tower.json`, `run_mutants.py --jobs 3`, profil 21, code 0, `mutants_ok module=tower mutants=5 tues=5`) : `t1_sans_s_dans_f` (porte `witness_t1_square`), `arret_sous_la_fenetre` (`inert_below_window`), `decroissance_apres_le_saut` (`fact_saturated` : le compte des contrôles le voit), `cible_cellule_prise_pour_naissance` (`witness_t1_square`), `census_sans_seuil` (`fact_saturated`) ; tous par code (verdict de `run_expect`). Rapport : `runs/mutants_tower.json`.
- 20:07 UTC : effet structurel de l arrêt à la première cellule de fenêtre (`G-L2`, `LEM-T3`), comptes déterministes (travail de la v11 lu dans le journal de `mhgp12_vidage`, celui de la v12 dans la sonde) : sur ng00–02 à K5, ordres 2 à 5, sondes 12 992 350 → 10 762 971, plus petites boules 3 125 857 → 2 719 521 (−13,0 %), censuses 717 024 → 622 490 (−13,2 %) :

| trame | k | sondes v11 / v12 | plus petites boules v11 / v12 (évitées) | censuses v11 / v12 (évités) |
| --- | ---: | --- | --- | --- |
| ng00 | 2 | 475662 / 426787 | 64721 / 59420 (8.2 %) | 2515 / 2381 (5.3 %) |
| ng00 | 3 | 861202 / 724125 | 187877 / 165770 (11.8 %) | 14000 / 12743 (9.0 %) |
| ng00 | 4 | 1338895 / 1093453 | 353517 / 304438 (13.9 %) | 57674 / 50517 (12.4 %) |
| ng00 | 5 | 1919207 / 1536796 | 568919 / 482533 (15.2 %) | 217325 / 186511 (14.2 %) |
| ng01 | 2 | 403029 / 363947 | 51011 / 47007 (7.8 %) | 2080 / 1993 (4.2 %) |
| ng01 | 3 | 714492 / 604978 | 147841 / 131828 (10.8 %) | 11078 / 10074 (9.1 %) |
| ng01 | 4 | 1086978 / 893824 | 273741 / 237355 (13.3 %) | 43250 / 37770 (12.7 %) |
| ng01 | 5 | 1541599 / 1240704 | 439094 / 374365 (14.7 %) | 160996 / 138202 (14.2 %) |
| ng02 | 2 | 510995 / 466216 | 56369 / 52290 (7.2 %) | 1943 / 1841 (5.2 %) |
| ng02 | 3 | 902072 / 765156 | 173710 / 157325 (9.4 %) | 10172 / 9172 (9.8 %) |
| ng02 | 4 | 1344953 / 1116272 | 309729 / 273865 (11.6 %) | 39263 / 34513 (12.1 %) |
| ng02 | 5 | 1893266 / 1530713 | 499328 / 433325 (13.2 %) | 156728 / 136773 (12.7 %) |
| total | 2–5 | 12992350 / 10762971 | 3125857 / 2719521 (13.0 %) | 717024 / 622490 (13.2 %) |

- 20:31 UTC : construction complète au profil 21 (`full21`) : `ctest -LE long` 621/622 au premier passage, seul échec `mhgp12_core_unit_reasons` (table gravée des raisons de `tests/core/status_test.cpp` : les 5 raisons de la tour y sont ajoutées, taille 24 → 29, dernière raison `cell_capacity`) ; corrigé, porte verte. Deux mutants ajoutés (`table_s_etoile_muette` → `catalogue_missing_ball`, `controle_croise_decale` → `census_mismatch`, sur le cercle du carré). **Écart à la consigne des fils** : un `ctest -R tower` sans `-LE long` a relancé la campagne de mutants avec `MHGP12_MUTANT_JOBS=0` (cœurs de la machine) pendant environ 6 min ; résultat : 7/7 mutants tués (`runs/mutants_tower_7.json`, code 0).
- 20:40 UTC : portes d échelle et de déterminisme (`g_determinism.py`, sonde à 1 et 8 fils enregistrée ; jouée ici à 1 et 3 fils, puis `mhgp12_tower_scale8000` par ctest à 1 et 8 fils, conforme) : empreinte de la résolution (sections de l export, en-tête exclu : identique aux trois profils) et compteurs identiques entre fils ; invariants globaux par ordre (contrôles = plus petites boules + succès de sonde ; une chaîne par représentant). Uniformes K5 : 8 000 sites 1 780 799 représentants (cellules 600 630 = boules du catalogue : une jonction par boule régulière), 16 000 : 3 674 998, 32 000 : 7 550 507 ; ng00 K5 : 3 622 258. Temps du script (catalogue + étage G, 1 puis 3 fils) : 18 s, 41 s, 89 s, 51 s.
- 20:49 UTC : **profils 24 et 32** (`-DMHGP12_MODULES=core;tower`, modules touchés par le patch) : construction sans avertissement et `ctest -LE long` 129/129 à chaque profil, dont `mhgp12_tower_scale8000/16000/32000` avec les **mêmes empreintes** qu au profil 21 (sorties identiques sur les mêmes coordonnées), l oracle borné, les témoins, `core_unit_reasons` et `mhgp12_style`. (Profil 21 complet : voir 21:06.)
- 20:50 UTC : ng00 **K10** (sonde, 3 fils, local, informatif) : conforme ; représentants par ordre = traces de la v11 (MES-M7 K10 : 410 941 … 3 831 491, identiques) ; étage G 10,1 s à 3 fils (résolution 9,56 s, tables de populations 385 ms, comptage 39 ms, remplissage 84 ms), pic RSS 1,74 Go (catalogue compris). Comptes :

| trame | k | naissances | cellules (inertes, étendues) | représentants | cibles naissance | cibles cellule | 1re sonde | sondes après pas | plus petites boules (t1 / cert_census) | censuses saturés / complets | sauts catalogue / census | pas inertes | contrôles | chaîne max |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | --- | --- | --- | ---: | ---: | ---: |
| ng00_k10 | 1 | 39885 | 101138 (49, 49) | 202326 | 202326 | 0 | 0 | 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 | 0 | 0 |
| ng00_k10 | 2 | 101089 | 166266 (38, 86) | 410941 | 367367 | 43574 | 354229 | 13138 | 58654 / 766 | 766 / 0 | 15080 / 766 | 0 | 426787 | 5 |
| ng00_k10 | 3 | 166228 | 249527 (34, 72) | 673325 | 558355 | 114970 | 525776 | 32579 | 162738 / 3032 | 3032 / 0 | 39006 / 3032 | 8762 | 724125 | 8 |
| ng00_k10 | 4 | 249493 | 341136 (54, 89) | 985378 | 789015 | 196363 | 729558 | 59457 | 296082 / 8356 | 8356 / 0 | 72150 / 8356 | 27569 | 1093453 | 11 |
| ng00_k10 | 5 | 341081 | 448805 (50, 107) | 1350288 | 1054263 | 296025 | 963244 | 91019 | 462139 / 20394 | 20391 / 3 | 108479 / 20391 | 57638 | 1536796 | 11 |
| ng00_k10 | 6 | 448753 | 564564 (48, 101) | 1760463 | 1356456 | 404007 | 1226318 | 130138 | 647249 / 45159 | 45155 / 4 | 146679 / 45155 | 96567 | 2048864 | 12 |
| ng00_k10 | 7 | 564515 | 694497 (42, 93) | 2217846 | 1689477 | 528369 | 1515812 | 173665 | 847829 / 96738 | 96736 / 2 | 171470 / 96736 | 147992 | 2634044 | 14 |
| ng00_k10 | 8 | 694453 | 829440 (56, 101) | 2709793 | 2051517 | 658276 | 1825751 | 225766 | 1024322 / 202893 | 202887 / 6 | 159226 / 202887 | 206828 | 3278734 | 16 |
| ng00_k10 | 9 | 829383 | 979387 (37, 94) | 3248111 | 2440406 | 807705 | 2157837 | 282569 | 1127044 / 429880 | 406350 / 23530 | 64145 / 406350 | 278726 | 3997332 | 20 |
| ng00_k10 | 10 | 979350 | 1138325 (30, 67) | 3831491 | 2860552 | 970939 | 2518056 | 342496 | 970939 / 957933 | 597705 / 360228 | 0 / 597705 | 360228 | 4789424 | 21 |

- 21:06 UTC : profil 21 complet final : `ctest -LE long` **628/628** (une porte sautée : sentinelle `lidar` sans `MHGP12_DATA_DIR`), dont les portes d échelle de la tour ; puis, avec les données et `MHGP12_V11_TOWER_DIR`, `mhgp12_tower_determinism_lidar_ng00_k5` (1 contre 8 fils, ligne gravée) et les trois `mhgp12_tower_diff_v11_ng0x_k5` (3 fils) conformes par ctest. `check_style` 264 fichiers et `check_constats` verts. Variable `MHGP12_V11_TOWER_DIR` ajoutée à la liste du § 7 de `docs/ARCHITECTURE.md`. Mesure de temps refaite à 21:05 : non significative (charge du codespace partagé : moyenne 22 sur 8 cœurs, compilations d autres agents ; tous les temps ×4) ; les temps retenus sont ceux de 19:34 (1 fil : étage G 2,83 s, résolution 2,67 s ; 3 fils : 1,05 s), indicatifs.

## Synthèse (livrable)

**Patch** : `v12_tour_G/patch_tour_G.diff` (26 fichiers, +3 471 / −6 ; SHA-256 `ce77145717e16169e48860e8bbb2113c04fb8fcd772ef56a0c3d4c4d8726c94d`), base `main` =
`6710723399876266d1efd7b45cf0d6adcd49a862` ; vérifié par `git apply --check` puis application sur une archive neuve de
cette base (arbre identique à la copie de travail) ; s'applique aussi tel quel sur `58761b36d` (HEAD de `main` à
20:51 UTC : aucun des chemins touchés n'a changé entre les deux). Aucune donnée KITTI ni vidage dans le patch.

**Interface** : `v12_tour_G/INTERFACE.md` (publiée à 19:16, révisée à 20:01).

### Portes (codes exacts)

| Porte | Label | Code | Ce qu'elle juge |
| --- | --- | ---: | --- |
| `mhgp12_tower_unit_*` (9 groupes + inventaire) | unit fast | 0 | cibles et capacité (`tower_capacity` aux bornes 2^31−1 / 2^32−1) ; WIT-D2 (trace AB née après ℓ(r_b−1), aucune garde) ; WIT-MEMO (dates : rang de toute cible < rang de sa jonction ; {0,6} sous la jonction de niveau 4 refusée `tower_invariant`) ; WIT-T1-CARRE côté tour (`lem_t1` exige S ⊆ F ; BD : route exacte, census COMPLET du catalogue = `fact_complete_census_in_catalogue`, arrêt sur la cellule (cercle, 2)) ; `fact_saturated_in_catalogue` (census saturé d'une sphère du catalogue, contrôle avant le saut) ; pas inerte sous la fenêtre ; `cell_capacity` (30 sites cosphériques) et WIT-SPHERE50 (`shell_capacity` du catalogue) ; budget (`memory_budget`, aucune réservation perdue) ; déterminisme 1 contre 8 fils |
| `mhgp12_tower_oracle` (+ `_opt`) | oracle fast | 0 | suite rapide, 342 nuages (34 doublons refusés D8) : 21 225 cibles **égales** à `resolve_v12` (`v12_indices`) et dans la composante attendue à la coupe ouverte de la jonction (forêt de l'étage B) ; traces = t-parties séparables ; cellules, genres, compteurs de l'objet |
| `mhgp12_tower_diff_v11_ng00/01/02_k5` | lidar long | 0 | différentiel v11 (`MHGP12_V11_TOWER_DIR`) : 3 622 258 / 3 012 810 / 3 850 037 représentants, 100 % dans la composante de la graine v11 (et graine identique) ; naissances, cellules, traces identiques |
| `mhgp12_tower_scale8000/16000/32000` (+ `_opt`) | scale | 0 | 1 contre 8 fils : empreinte et compteurs identiques ; invariants globaux ; mêmes empreintes aux profils 21, 24, 32 |
| `mhgp12_tower_determinism_lidar_ng00_k5` | lidar long | 0 | idem sur ng00 (1 contre 8 fils, par ctest) |
| `mhgp12_mutants_tower` (+ `_manifest`) | mutant long | 0 | 7/7 mutants tués : `t1_sans_s_dans_f`, `arret_sous_la_fenetre`, `decroissance_apres_le_saut`, `cible_cellule_prise_pour_naissance`, `census_sans_seuil`, `table_s_etoile_muette` (→ `catalogue_missing_ball`), `controle_croise_decale` (→ `census_mismatch`) |
| `mhgp12_tower_probe_usage` | fast | 2 | usage de la sonde |
| `mhgp12_style`, `check_constats` | fast | 0 | 264 fichiers ; profil 21 complet `ctest -LE long` 628/628 ; profils 24 et 32 (`core;tower`) 129/129 |

### Écarts au contrat et remarques (aucune contradiction mathématique trouvée)

1. **Lieu de la classification et de la table de populations** : le § 2 du contrat les place dans la fin d'étage du
   catalogue, « gardée par la Session » ; la mission les met dans `src/tower/` (étage G), reconstruites à chaque appel.
   Coût local : tables 37 ms à 3 fils sur ng00 K5, **385 ms à K10** (construction séquentielle par ordre, déterministe) :
   à paralléliser ou rendre résidente (Session) en T2-c.
2. **Représentants = toutes les traces strictes** (v11, compteur d'objet du § 8 : 1 350 288 à l'ordre 5 de ng00 K5),
   pas un par morceau comme l'oracle : même forêt (les traces d'un morceau sont unies sous le niveau de la cellule),
   différence seulement sur les coquilles étendues (rares : C(m, t) ≤ 10 sur ng00–02).
3. **Date initiale contrôlée** : la décroissance stricte part du rang de la cellule de la jonction (le pseudo-code part
   d'un « précédent » vide) ; contrôle compté et vérifié globalement (contrôles = plus petites boules + succès de
   sonde) : c'est ce qui tue le mutant « décroissance après le saut ».
4. **Contrôle croisé du census complet** plus fort que le contrat : listes I et U comparées, pas seulement (p, q, m).
5. **Plafonds déclarés** : `kMaxCellCombinations = 4096`, `kMaxShellWitnesses = 4096` (refus `cell_capacity`), coquille
   de census ≤ 64 sites (`shell_capacity`) ; maxima observés C(m, t) = 10, m = 5.
6. **Séparabilité par les supports de U** (Carathéodory) au lieu d'une plus petite boule par partie (v11) : mêmes traces
   (différentiel : masques identiques à ceux de la v11 à chaque ordre).
7. **Temps local plus lent par unité de travail que la réplique v12 de `MES-M7`** : à un fil sur ng00 K5, résolution
   2,67 s (ordre 5 : 1,58 s) contre 2,34 s pour la réplique (ordre 5 : 1,255 s), alors que la v12 fait 13 % de plus
   petites boules et de censuses en moins. Hypothèses à mesurer (T2-c, `MES-M7` sur le produit) : sonde de la table
   vérifiée contre la CSR du catalogue (deux à trois défauts de cache de plus qu'une ligne contiguë), préparation de la
   garde par census, matérialisation du niveau à chaque certificat. Aucun levier (`G-L3` à `G-L7`) n'est implanté.
8. Pas de temps G4 : tous les temps sont locaux (codespace partagé), ils ne décident rien.
