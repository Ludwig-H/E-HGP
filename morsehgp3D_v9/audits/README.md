# Index des audits de la v9

Ouvert par l'auditeur C le 23 septembre 2026, puis mis à jour par les
auditeurs. Les notes de preuve historiques restent à leur place ; cet
index distingue leur portée actuelle.
La synthèse reste [`ETAT_COURANT.md`](ETAT_COURANT.md) ; le dialogue passe par
le canal [`audits/COORDINATION_MORSEHGP3D_V9.md`](../../audits/COORDINATION_MORSEHGP3D_V9.md).
État de l'index : `origin/main` au moment du commit qui le modifie.

## Par où commencer

1. [`ETAT_COURANT.md`](ETAT_COURANT.md) : verdict synthétique et mutable.
2. [`AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md`](AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md) :
   à quoi sert l'algorithme et comment la tour est reconstruite, pas à pas.
3. [`CONTRAT_COUTS_ET_PARALLELISATION.md`](CONTRAT_COUTS_ET_PARALLELISATION.md) et
   [le contre-audit R20/100 ms](CONTRE_AUDIT_B_R20_ET_TRAJECTOIRE_100MS_20260924.md) et
   [le préflight R21/v26](CONTRE_AUDIT_B_PREFLIGHT_R21_V26_20260924.md) :
   le contrat, le dernier coût reçu de la chaîne GPU hybride et les
   verrous architecturaux.
   Le [contre-audit du pool E2 local](CONTRE_AUDIT_B_POOL_E2_WIP_20260924.md)
   distingue réemploi de fils et plafond réel de concurrence ; un
   [reproducteur minimal](pool_e2_exception_probe_20260924.cpp) accompagne
   sa porte de robustesse, sans prétendre à un défaut de résultat produit.
   Le [groupement haché de phase 0](CONTRE_AUDIT_B_GROUP_HASH_WIP_20260924.md)
   est un autre WIP local : ses cibles statiques sont comparées finement,
   mais son intégration avec E2 et son gain G4 restent ouverts.
   La [queue FULL E4](CONTRE_AUDIT_B_QUEUE_E4_WIP_20260924.md) recouvre
   populations et images, avec attribution temporelle et ressources à
   mesurer ; ces trois WIP frères ne sont pas encore un moteur combiné.
   Les [épingles brutes CPU de C](CONTRE_AUDIT_B_EPINGLES_BRUTES_C_PREFLIGHT_20260924.md)
   attendent leur porte de clôture avant d'autoriser R21.
   Le [grand audit C](AUDIT_C_GRAND_AUDIT_V9_20260924.md) et sa
   [contrelecture B](CONTRE_AUDIT_B_GRAND_AUDIT_C_100MS_20260924.md)
   distinguent mesures, projections et contrat inchangé de 100 ms.
   Le [shadow rectangle à moments](moments_rectangle_shadow_20260924/README.md)
   juge négativement un choix naïf de bloc sur un quart LiDAR sans sol.
4. [`NOTE_C_INVARIANT_EULER_20260923.md`](NOTE_C_INVARIANT_EULER_20260923.md),
   [`CONTRELEC_EULER_PAR_NERF_20260923.md`](CONTRELEC_EULER_PAR_NERF_20260923.md) et
   [le contre-exemple](CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md) : invariant
   nécessaire de complétude, insuffisant pour certifier toutes les clés.
5. [`AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md`](AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md) :
   six familles d'implémentations confrontées au contrat, feuille de route ordonnée.
6. Par thème, les tableaux ci-dessous.

## Légende

- **Auteur** : A, B, C (auditeurs), DEV (développeur). « B ? » ou « A ou B » :
  attribution probable (Git ne distingue pas les acteurs ; déduite du nom, du
  texte et des commits voisins) ; l'auteur est invité à la corriger.
- **Portée** (vocabulaire proposé par B) : démontré localement, testé borné,
  mesure (reçu ou recalcul), shadow (expérience hors produit), méthode,
  synthèse. Classement de C, à corriger par l'auteur si besoin.
- **Cycle** : vivant, clos (le commit qui l'a traité), historique, négatif,
  remplacé par une autre note.
- **Réponse du développeur** dans le canal : acceptée, acceptée en partie,
  refusée, différée, sans réponse, sans objet. Le préambule du canal exige
  une échéance pour tout report ; « différée sans échéance » signale une
  réponse incomplète, pas une date que l'index inventerait.

## Cadre, objet et verdict

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`ETAT_COURANT.md`](ETAT_COURANT.md) | DEV puis A et B | `3595725a` 22/21:18 | synthèse | vivant | sans objet |
| [`AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md`](AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md) | C | `f4480c02` 08:44 | démontré localement, mesure | vivant | sans réponse |
| [`AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md`](AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md) | C | 23/10:12 | synthèse, shadow, mesure locale | vivant | sans réponse |
| [`CONTRE_AUDIT_B_ALTERNATIVES_C_20260923.md`](CONTRE_AUDIT_B_ALTERNATIVES_C_20260923.md) | B | `59b370f7` 10:31 | contrelecture des projections G4 | vivant | sans réponse |
| [`ERRATUM_B_AUDIT_C_OBJET_ET_EULER_20260923.md`](ERRATUM_B_AUDIT_C_OBJET_ET_EULER_20260923.md) | B | `0fe53513` 08:52 | démontré localement, contrelecture | vivant | sans réponse |
| [`AUDIT_A_ARCHITECTURE_K_GABRIEL_20260922.md`](AUDIT_A_ARCHITECTURE_K_GABRIEL_20260922.md) | A | `0674dc02` 22/21:44 | méthode | historique | acceptée (canal l. 139) |
| [`CONTRAT_COUTS_ET_PARALLELISATION.md`](CONTRAT_COUTS_ET_PARALLELISATION.md) | A | `0674dc02` 22/21:44 | mesure, méthode | vivant | acceptée en partie |
| [`CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md`](CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md) | A | `0674dc02` 22/21:44 | démontré localement | clos | acceptée |
| [`CONTRE_AUDIT_A_MESURES_PLAN_20260922.md`](CONTRE_AUDIT_A_MESURES_PLAN_20260922.md) | A | `0674dc02` 22/21:44 | mesure | clos | acceptée |
| [`CONTRELECTURE_B_CELLULES_ET_CATALOGUE_20260922.md`](CONTRELECTURE_B_CELLULES_ET_CATALOGUE_20260922.md) | B | `3cf72097` 22/22:04 | méthode | historique | différée sans échéance |
| [`QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md`](QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md) | DEV | `3595725a` 22/21:18 | question | clos (0674dc02) | sans objet |

## Complétude et juges globaux

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`NOTE_C_INVARIANT_EULER_20260923.md`](NOTE_C_INVARIANT_EULER_20260923.md) | C | `cab281d8` 08:25 | démontré localement, testé | vivant | acceptée en partie (sonde/portes v13 publiées ; [limites ouvertes](ETAT_COURANT.md)) |
| [`CONTRELEC_EULER_PAR_NERF_20260923.md`](CONTRELEC_EULER_PAR_NERF_20260923.md) | B | `559c8ab8` 08:40 | démontré localement | vivant | sans réponse |
| [`CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md`](CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md) | B | `bf19240a` 09:28 | démontré localement (faux négatif exact d'Euler et de Kmax+2) | vivant | sans objet |
| [`LEMME_PREMIERE_COFACETTE_OMISSION_20260923.md`](LEMME_PREMIERE_COFACETTE_OMISSION_20260923.md) | A | 23/11:07 | preuve conditionnelle de refus FULL, résolveur interne | vivant | sans réponse |
| [`CONTRE_AUDIT_B_OMISSIONS_ET_PORTEE_REVISION_C_20260923.md`](CONTRE_AUDIT_B_OMISSIONS_ET_PORTEE_REVISION_C_20260923.md) | B | `8b62e4fd` 11:02 | contrelecture de la sonde d'omission C, recherche de condensés | vivant | sans objet |
| [`CONTRE_AUDIT_B_JUGE_Q2_ET_DIGEST_C_20260923.md`](CONTRE_AUDIT_B_JUGE_Q2_ET_DIGEST_C_20260923.md) | B | `074b5da0e` 11:32 | omissions provoquées et 204 683 présentations q2 ; demande de juge q3 désormais historique | vivant pour les mesures | sans objet |
| [`CONTRELEC_JUGE_CLES_ABSENTES_20260923.md`](CONTRELEC_JUGE_CLES_ABSENTES_20260923.md) | A | 23/11:30 | lecture de la porte synthétique `683fa46e` ; mutations et débordement de coquille à préciser | vivant | sans réponse |
| [`CONTRE_AUDIT_B_JUGE_CLES_ABSENTES_20260923.md`](CONTRE_AUDIT_B_JUGE_CLES_ABSENTES_20260923.md) | B | `c179025bf` 11:23 | limites du juge échantillonné de clés jamais émises | vivant | sans réponse |
| [`COMPLETUDE_Q4_CLE_REMBOURREE_20260923.md`](COMPLETUDE_Q4_CLE_REMBOURREE_20260923.md) | B | `0a6efac3` 06:58 | testé borné | vivant | sans réponse |
| [`Q4_INDUCTION_ATLAS_EVENEMENTS_20260923.md`](Q4_INDUCTION_ATLAS_EVENEMENTS_20260923.md) | B ? | `28f0c284` 07:15 | démontré localement | vivant | sans réponse |
| [`Q34_PROPRIETAIRE_PASSAGE_AMONT_20260923.md`](Q34_PROPRIETAIRE_PASSAGE_AMONT_20260923.md) | B ? | `4291c538` 07:56 | démontré localement | vivant | sans réponse |
| [`CONTRE_AUDIT_B_PORTE_PUBLIQUE_T2_20260922.md`](CONTRE_AUDIT_B_PORTE_PUBLIQUE_T2_20260922.md) | B | `174fb8db` 22/23:18 | testé borné | historique | sans réponse |
| [`q4_global_12sites_20260923/`](q4_global_12sites_20260923/README.md) | B ? | `28f0c284` 07:15 | testé borné | vivant | sans objet |
| [`q4_depth_ladder_20260923/`](q4_depth_ladder_20260923/README.md) | A | 23/14:53 | oracle rationnel ; 76 flux complets K4/K5/K10 avec q4 profonde et faces q3 rejetées | vivant | sans objet |

## Arithmétique 18 bits

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`CONTRE_AUDIT_B_U18_ET_SONDE_20260922.md`](CONTRE_AUDIT_B_U18_ET_SONDE_20260922.md) | B | `0786d6c1` 22/22:59 | démontré localement | historique | acceptée |
| [`CERTIFICAT_B_MARGE_JUGE_Q3_U18_20260923.md`](CERTIFICAT_B_MARGE_JUGE_Q3_U18_20260923.md) | B | `d3ece2d0b` 12:45 | preuve conditionnelle IEEE binary64/u18 du filtre du juge q3 | vivant | sans objet |

## q3

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`Q3_STRUCTURE_ET_BORNES.md`](Q3_STRUCTURE_ET_BORNES.md) | A | `0674dc02` 22/21:44 | démontré localement | vivant | acceptée en partie |
| [`CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md`](CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md) | B | `2c6d806e` 22/22:21 | démontré localement | vivant | différée sans échéance |
| [`CONTRE_AUDIT_B_Q3_FEUILLE_WIP_20260923.md`](CONTRE_AUDIT_B_Q3_FEUILLE_WIP_20260923.md) | B | `bb3c696e` 00:42 | démontré localement | clos (code publié) | acceptée |
| [`CONTRE_AUDIT_B_JUGES_C_V4_20260923.md`](CONTRE_AUDIT_B_JUGES_C_V4_20260923.md) | B | `931d37cd1` 13:07 | clés canoniques, ancres isolées et provenance ; reçu v5/v6 publié ensuite | historique pour la recette v4/v5 | sans réponse |
| [`AUDIT_A_JUGE_Q3_V6_LONGUES_INCIDENCES_20260923.md`](AUDIT_A_JUGE_Q3_V6_LONGUES_INCIDENCES_20260923.md) | A | 23/13:48 | reçu v6 historique ; causalité longue au rang critique et refus de code 2 fermés par les portes v7/v8 ultérieures | historique | réponse ultérieure |
| [`CONTRE_AUDIT_B_PORTES_JUGES_R20_20260923.md`](CONTRE_AUDIT_B_PORTES_JUGES_R20_20260923.md) | B | 23/21:05 | patch C R-20 : 34/34 tests locaux utiles comme régression CPU échantillonnée ; ancres rares, index du juge reconstruit, pas de FULL/LiDAR/u18 haut/fixture étendue dédiée | patch non adopté ; ne qualifie pas la complétude | transmis à C/D |

## q4

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`Q4_STRUCTURE_ET_BORNES.md`](Q4_STRUCTURE_ET_BORNES.md) | A | `0674dc02` 22/21:44 | démontré localement | vivant | acceptée en partie |
| [`CONTRE_AUDIT_B_Q4_SHALLOW_20260922.md`](CONTRE_AUDIT_B_Q4_SHALLOW_20260922.md) | B | `174fb8db` 22/23:18 | démontré localement | historique | sans réponse |
| [`CONTRE_AUDIT_B_Q4_NIVEAUX_ORIENTES_20260922.md`](CONTRE_AUDIT_B_Q4_NIVEAUX_ORIENTES_20260922.md) | B | `177fffbd` 22/23:58 | démontré localement | vivant | sans réponse |
| [`CONTRE_AUDIT_B_Q4_K5_DEUX_INTERIEURS_20260922.md`](CONTRE_AUDIT_B_Q4_K5_DEUX_INTERIEURS_20260922.md) | B | `d4942b4e` 22/23:33 | testé borné | historique | sans objet |
| [`DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md`](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md) | A | `573c6869` 00:17 | démontré localement | vivant | sans réponse |
| [`CONTRE_AUDIT_B_DOMINATION_Q4_20260923.md`](CONTRE_AUDIT_B_DOMINATION_Q4_20260923.md) | B | `60ef2a95` 00:20 | démontré localement | historique | sans réponse |
| [`SEUIL_SATURATION_ATLAS_PAR_VOIE_20260923.md`](SEUIL_SATURATION_ATLAS_PAR_VOIE_20260923.md) | A | `763d2b42` 00:40 | démontré localement | vivant | différée sans échéance |

## Coût q3/q4 : réduire le travail avant l'expansion

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`PISTE_B_Q34_RECTANGLES_AVANT_EXPANSION_20260923.md`](PISTE_B_Q34_RECTANGLES_AVANT_EXPANSION_20260923.md) | B | `de26dd7a` 03:03 | démontré localement | vivant | refusée en partie, différée |
| [`FIXTURE_CELLULES_CENTRES_Q34_20260923.md`](FIXTURE_CELLULES_CENTRES_Q34_20260923.md) | B | `e3fb7999` 10:12 | démontré localement (fixture exacte), pas une preuve LiDAR | vivant | sans réponse |
| [`PISTE_B_Q34_NOEUDS_AVANT_COVER_20260923.md`](PISTE_B_Q34_NOEUDS_AVANT_COVER_20260923.md) | B | `f36c140c` 01:59 | démontré localement | vivant | différée |
| [`CONTRE_AUDIT_RECTANGLES_Q34_TICKET_BORNE_20260923.md`](CONTRE_AUDIT_RECTANGLES_Q34_TICKET_BORNE_20260923.md) | B | `ff121022` 08:34 | démontré localement, shadow | vivant | sans réponse |
| [`CERTIFICAT_NOEUDS_CORE_LIDAR_20260923.md`](CERTIFICAT_NOEUDS_CORE_LIDAR_20260923.md) | B | `7f218872` 08:24 | démontré localement | vivant | sans réponse |
| [`moments_multisite_precore_20260924/`](moments_multisite_precore_20260924/NOTE.md) | A | 24/02:00 | certificat exact ≥T intérieurs par bloc de moments avant les formes q3/q4 ; fixture u18 et contrôle rationnel, sélectivité non mesurée | proposition non portée | sans réponse |
| [`CONTRE_AUDIT_B_COVER_BATCH_20260923.md`](CONTRE_AUDIT_B_COVER_BATCH_20260923.md) | B | `bee0609f` 00:09 | démontré localement | vivant | sans réponse |
| [`CACHE_TEMOINS_COUT_VALIDATION_20260923.md`](CACHE_TEMOINS_COUT_VALIDATION_20260923.md) | B ? | `128fb231` 06:26 | mesure | vivant | sans réponse |
| [`LEDGER_VISITES_CACHEES_Q34_20260923.md`](LEDGER_VISITES_CACHEES_Q34_20260923.md) | B | `5270f3df` 07:43 | mesure | clos (4530644b) | acceptée |
| [`CORE_LIDAR_LOCAL_20260923.md`](CORE_LIDAR_LOCAL_20260923.md) | A ou B | `3a18c863` 04:43 | shadow | vivant | sans objet |
| [`SHADOW_HA_Q34_LIDAR_20260923.md`](SHADOW_HA_Q34_LIDAR_20260923.md) | B ? | `34c3164f` 05:44 | shadow | vivant | différée sans échéance |
| [`SHADOW_HA_OCTANT_Q34_LIDAR_20260923.md`](SHADOW_HA_OCTANT_Q34_LIDAR_20260923.md) | B ? | `128fb231` 06:26 | shadow | vivant | sans réponse |
| [`Q34_BLOCS_LIDAR_SHADOW_20260923.md`](Q34_BLOCS_LIDAR_SHADOW_20260923.md) | A ? | `867e68b3` 07:25 | shadow | négatif | sans objet |
| [`CONTRE_AUDIT_B_CHARGEMENT_FORMES_Q34_WIP_20260923.md`](CONTRE_AUDIT_B_CHARGEMENT_FORMES_Q34_WIP_20260923.md) | B | `faa76965` 03:15 | démontré localement | clos (aae9da0e) | acceptée |
| [`CONTRE_AUDIT_B_NOYAU_DIAMETRAL_WIP_20260923.md`](CONTRE_AUDIT_B_NOYAU_DIAMETRAL_WIP_20260923.md) | B | `aed84902` 04:07 | démontré localement | clos (code publié) | acceptée |
| [`CONTRE_AUDIT_B_Q34_PREUVE_CONJOINTE_WIP_20260923.md`](CONTRE_AUDIT_B_Q34_PREUVE_CONJOINTE_WIP_20260923.md) | B | `f2301d91` 02:25 | démontré localement | clos (7f64a279) | acceptée |
| [`CONTRE_AUDIT_B_RECU_VOIES_MORTES_20260923.md`](CONTRE_AUDIT_B_RECU_VOIES_MORTES_20260923.md) | B | `f0929ea6` 01:54 | mesure | clos | acceptée |

## Tour FULL

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`CONTRE_AUDIT_B_D5_FULL_MAIGRE_20260923.md`](CONTRE_AUDIT_B_D5_FULL_MAIGRE_20260923.md) | B | `a514de68` 10:22 + mise à jour | projection D5 bornée ; index seul négatif ; R13 montre le recouvrement statique/lots ; shadow racine pré-lot à garder | vivant | réponse D attendue |
| [`CONTRE_AUDIT_B_PHASE_A_ALLEGEE_20260923.md`](CONTRE_AUDIT_B_PHASE_A_ALLEGEE_20260923.md) | B | 23/soir | cinq paires locales de tour validées, run exact et singleton relus ; phase A plus rapide, tour K5 non stable ; provenance binaire/entrée/log non épinglée | vivant, aucun G4 ni gain de contrat | transmis à D |
| [`CONTRE_AUDIT_B_BROUILLON_PLAT_FULL_WIP_20260923.md`](CONTRE_AUDIT_B_BROUILLON_PLAT_FULL_WIP_20260923.md) | B | 23/21:56, addendum soir | surcharge CSR publique sans validation avant accès ; défaut exécuté sous ASan sur CSR invalide ; gain d'allocations/groupes non mesuré | défaut publié historique, corrigé dans `3765080cf` ; sorties internes valides non accusées | corrigée pour le CSR |
| [`RELECTURE_CORRECTIF_CSR_PLAT_WIP_20260923.md`](RELECTURE_CORRECTIF_CSR_PLAT_WIP_20260923.md) | A | 23/soir | relecture sur sources mutables : validation CSR avant accès, micro-test causal et gate ASan/UBSan passent ; égalité plate/vectorielle limitée aux tailles, allocation plate non injectée | correctif publié dans `3765080cf` ; tests de la relecture non transférés au commit, pas de qualification FULL/G4 dédiée | corrigée pour le CSR ; autres portes ouvertes |
| [`flat_draft_invalid_probe.cpp`](flat_draft_invalid_probe.cpp) | B | 23/soir | banque valide, `batch_begin` public trop court : heap-buffer-overflow dans `FlatDraftSource::actions` sous ASan/UBSan avant `3765080cf` | fixture négative causale historique, pas test de géométrie | corrigée dans le produit |
| [`CONTRE_AUDIT_B_RECU_BROUILLON_PLAT_LOCAL_20260923.md`](CONTRE_AUDIT_B_RECU_BROUILLON_PLAT_LOCAL_20260923.md) | B | 23/22:08 | 11/11 SHA et cinq couples locaux ; K5 FULL −1,7 à −4,3 %, K10 une paire −23,0 % ; pas de payload/commandes/binaires épinglés | prometteur local, défaut CSR du paquet historique corrigé depuis ; ablation G4 dédiée ouverte | réponse D attendue |
| [`CONTRELEC_D5_NAISSANCE_ET_PORTE_E1_20260923.md`](CONTRELEC_D5_NAISSANCE_ET_PORTE_E1_20260923.md) | A | 23/10:25 | preuve et contre-fixture de porte E1 | vivant | sans réponse |
| [`d5_knn_aabb_counterexample_20260923/`](d5_knn_aabb_counterexample_20260923/README.md) | A | 23/soir | vraie requête AABB u18 : 7 sites fermés constants, 29→141 nœuds visités pour 8→64 sites extérieurs ; famille Ω(n) | correction de la borne D5, exactitude du saut intacte | réponse D attendue |
| [`CONTRE_AUDIT_B_INDEX_SELLES_NEGATIF_20260923.md`](CONTRE_AUDIT_B_INDEX_SELLES_NEGATIF_20260923.md) | B | `01552e81` 11:17 | réception locale du port isolé hors produit ; gain absent, trace des hits non reçue | négatif pour ce port | sans objet |
| [`MEB_PROPOSITION_EXACTE_20260923.md`](MEB_PROPOSITION_EXACTE_20260923.md) | B | `85d79753` 06:03 | démontré localement | clos (8e8b83a3) | acceptée |
| [`CONTRE_AUDIT_B_ANCHOR_MEB_DIAMETRE_20260922.md`](CONTRE_AUDIT_B_ANCHOR_MEB_DIAMETRE_20260922.md) | B | `ae88ff1f` 22/23:36 | démontré localement | historique | sans réponse |
| [`CONTRE_AUDIT_B_INDEX_CLES_FULL_20260923.md`](CONTRE_AUDIT_B_INDEX_CLES_FULL_20260923.md) | B | `95e073a5` 06:05 | démontré localement | vivant | acceptée en partie (mesures en attente) |
| [`INTRUS_FULL_PREFIXE_EXACT_20260923.md`](INTRUS_FULL_PREFIXE_EXACT_20260923.md) | A | `573c6869` 00:17 | démontré localement | vivant | sans réponse |
| [`CONTRE_AUDIT_B_PREFIXE_INTRUS_20260923.md`](CONTRE_AUDIT_B_PREFIXE_INTRUS_20260923.md) | B | `d6fc9fe0` 00:22 | démontré localement | vivant | sans réponse |
| [`PREFETCH_GEOMETRIE_FULL_PAR_K_20260923.md`](PREFETCH_GEOMETRIE_FULL_PAR_K_20260923.md) | A | `5cdeec30` 00:03 | démontré localement | clos (684d8fc7) | acceptée |
| [`CONTRE_AUDIT_B_PREFETCH_FULL_20260923.md`](CONTRE_AUDIT_B_PREFETCH_FULL_20260923.md) | B | `bee0609f` 00:09 | démontré localement | historique | sans réponse |
| [`CONTRE_AUDIT_B_FULL_COUTS_ET_INTERFACES_20260922.md`](CONTRE_AUDIT_B_FULL_COUTS_ET_INTERFACES_20260922.md) | B | `b6bd8cdc` 22/22:33 | mesure | historique | sans réponse |
| [`CONTRE_AUDIT_B_FULL_PARALLELE_WIP_20260923.md`](CONTRE_AUDIT_B_FULL_PARALLELE_WIP_20260923.md) | B | `c6a9d18a` 02:00 | démontré localement | clos (code publié) | acceptée |
| [`CONTRE_AUDIT_B_PARALLELISME_Q34_FULL_20260922.md`](CONTRE_AUDIT_B_PARALLELISME_Q34_FULL_20260922.md) | B | `d4942b4e` 22/23:33 | méthode | remplacé par PHASE_A_FULL_LOTS | sans réponse |
| [`PHASE_A_FULL_LOTS_ET_PARALLELISME_20260923.md`](PHASE_A_FULL_LOTS_ET_PARALLELISME_20260923.md) | B | `cb4add26` 06:43 | démontré localement | vivant | sans réponse |
| [`PHASE_A_GRAPHE_TEMPOREL_20260923.md`](PHASE_A_GRAPHE_TEMPOREL_20260923.md) | B ? | `1bbb38e6` 06:59 | démontré localement | vivant | sans réponse |
| [`PHASE_A_MAX_ID_COMPOSANTE_20260923.md`](PHASE_A_MAX_ID_COMPOSANTE_20260923.md) | B | `5270f3df` 07:43 | démontré localement, testé borné | vivant | sans réponse |
| [`phase_a_20260923/`](phase_a_20260923/MANIFEST.md) | B ? | `1bbb38e6` 06:59 | shadow | vivant | sans objet |
| [`CONTRE_AUDIT_B_QUOTIENT_COQUILLE_20260922.md`](CONTRE_AUDIT_B_QUOTIENT_COQUILLE_20260922.md) | B | `d4942b4e` 22/23:33 | démontré localement | vivant | différée sans échéance |
| [`PLATEAUX_GRANDES_COQUILLES_B_20260922.md`](PLATEAUX_GRANDES_COQUILLES_B_20260922.md) | B | `3cf72097` 22/22:04 | démontré localement | vivant | différée sans échéance |
| [`CONTRE_AUDIT_B_RESIDENCE_CHAINE_20260922.md`](CONTRE_AUDIT_B_RESIDENCE_CHAINE_20260922.md) | B | `0786d6c1` 22/22:59 | mesure | vivant | acceptée en partie (RSS par phase non livré) |

## Chaîne et tri

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`AUDIT_DISTRIBUTION_SAMPLE_SORT_20260923.md`](AUDIT_DISTRIBUTION_SAMPLE_SORT_20260923.md) | A ? | `cd0ce60c` 05:30 | mesure | vivant | sans réponse |
| [`CONTRE_AUDIT_B_SAMPLE_SORT_PUBLIE_20260923.md`](CONTRE_AUDIT_B_SAMPLE_SORT_PUBLIE_20260923.md) | B | `610264da` 05:17 | démontré localement | vivant | acceptée en partie |

## Protocole et sessions G4

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`CONTRE_AUDIT_B_R20_ET_TRAJECTOIRE_100MS_20260924.md`](CONTRE_AUDIT_B_R20_ET_TRAJECTOIRE_100MS_20260924.md) | B | 24/soir | reçu R20 vérifié ; budgets K5/K10 ; preuve 64 coins des moments multisites ; trajectoire 100 ms conditionnelle | vivant, aucun contrat acquis | transmis dans la coordination |
| [`RECTIFICATIF_R8_Q34_ORDONNANCEMENT_20260923.md`](RECTIFICATIF_R8_Q34_ORDONNANCEMENT_20260923.md) | A | 23/10:00 | démontré dans le code, mesure R8 | vivant | sans réponse |
| [`CONTRE_AUDIT_B_V14_ORDONNANCEMENT_20260923.md`](CONTRE_AUDIT_B_V14_ORDONNANCEMENT_20260923.md) | B | `a514de68` 10:22 | portes et protocole v14 | vivant | sans réponse |
| [`CONTRE_AUDIT_B_Q2_MASS_FIRST_V16_20260923.md`](CONTRE_AUDIT_B_Q2_MASS_FIRST_V16_20260923.md) | B | `f115a39a9` 11:29 | porte différentielle directe à 64 jobs encore ouverte ; R11 remplace la projection avant G4 | vivant | sans réponse |
| [`RECEPTION_V14_CHRONOS_Q34_20260923.md`](RECEPTION_V14_CHRONOS_Q34_20260923.md) | A | 23/10:20 | méthode (deux mutations acceptées à `fe1142b5`) | vivant | sans réponse |
| [`CONTRE_AUDIT_B_WIP_TOUR_V15_RECOUVREMENT_20260923.md`](CONTRE_AUDIT_B_WIP_TOUR_V15_RECOUVREMENT_20260923.md) | B | `f5ef37f7` 10:32 | contrelecture WIP, défauts d'horloge K1/K5 clos par `33d51efd` et `c19e4b49` ; égalité complète du payload et TSan encore ouverts | historique pour l'horloge | corrigée pour l'horloge |
| [`CONTRELEC_V15_CHRONO_ORDRE_20260923.md`](CONTRELEC_V15_CHRONO_ORDRE_20260923.md) | A | 23/10:34 | mutations K5/K1 et réception du correctif | clos (`c19e4b49`) | corrigée |
| [`CONTRE_AUDIT_B_LECTEURS_1F048_20260923.md`](CONTRE_AUDIT_B_LECTEURS_1F048_20260923.md) | B | `758236b4` 10:07 | méthode (rejeu du lecteur) | vivant | sans réponse |
| [`CONTRE_AUDIT_B_G4_R9_ORDONNANCEMENT_20260923.md`](CONTRE_AUDIT_B_G4_R9_ORDONNANCEMENT_20260923.md) | B, réception A | `bf42ccde` 10:37 | mesure CPU G4 appariée, dossier publié `76436d44` | vivant | sans objet |
| [`RECEPTION_G4_R10_20260923.md`](RECEPTION_G4_R10_20260923.md) | A | 23/11:07 | réception CPU G4 appariée, recouvrement FULL | vivant | sans objet |
| [`CONTRE_AUDIT_B_G4_R10_ET_PASSE2_20260923.md`](CONTRE_AUDIT_B_G4_R10_ET_PASSE2_20260923.md) | B | `3a57a6d6` 11:13 | mesure (R10) | vivant | sans objet |
| [`CONTRE_AUDIT_B_G4_R11_ET_VOISINS_COEUR_20260923.md`](CONTRE_AUDIT_B_G4_R11_ET_VOISINS_COEUR_20260923.md) | B | `2485da41` 11:40 | réception CPU G4 R11 et portée conditionnelle du shadow voisins | vivant | sans objet |
| [`CERTIFICAT_Q34_SOUS_ENSEMBLES_LOCAUX_20260923.md`](CERTIFICAT_Q34_SOUS_ENSEMBLES_LOCAUX_20260923.md) | A | 23/11:48 | preuve : témoins distincts arbitraires ; alternative au k-NN global exact et budget mémoire WIP | vivant | sans objet |
| [`CONTRE_AUDIT_B_G4_R8_20260923.md`](CONTRE_AUDIT_B_G4_R8_20260923.md) | B | `2faf5b59` 09:57 | mesure | vivant ; seuil de tâche rectifié ci-dessus | sans objet |
| [`CONTRE_AUDIT_B_G4_R7B_20260923.md`](CONTRE_AUDIT_B_G4_R7B_20260923.md) | B | `b46826e2` 06:24 | mesure | historique | sans objet |
| [`CONTRE_AUDIT_B_G4_R6_20260923.md`](CONTRE_AUDIT_B_G4_R6_20260923.md) | B | `c17db454` 05:11 | mesure | historique | acceptée en partie (erratum incomplet) |
| [`CONTRE_AUDIT_B_G4_R5_20260923.md`](CONTRE_AUDIT_B_G4_R5_20260923.md) | B | `9d76d157` 03:34 | mesure | historique | acceptée |
| [`CONTRE_AUDIT_B_G4_R4B_CACHE_20260923.md`](CONTRE_AUDIT_B_G4_R4B_CACHE_20260923.md) | B | `f3d671af` 02:59 | mesure | historique | sans objet |
| [`CONTRE_AUDIT_B_G4_R3_20260923.md`](CONTRE_AUDIT_B_G4_R3_20260923.md) | B | `2c40035b` 02:06 | mesure | historique | sans objet |
| [`CONTRE_AUDIT_B_G4_R2_PREFLIGHT_20260923.md`](CONTRE_AUDIT_B_G4_R2_PREFLIGHT_20260923.md) | B | `6de74c75` 00:38 | mesure | historique (R2 refusé) | acceptée |
| [`CONTRE_AUDIT_B_G4_R1_ET_SCHEMA_V2_20260922.md`](CONTRE_AUDIT_B_G4_R1_ET_SCHEMA_V2_20260922.md) | B | `177fffbd` 22/23:58 | mesure | historique | acceptée |
| [`CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md`](CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md) | B | `f0929ea6` 01:54 | méthode | clos (a1d7a9bc) | acceptée |
| [`RECEPTION_V6_IDENTITES_MANQUANTES_20260923.md`](RECEPTION_V6_IDENTITES_MANQUANTES_20260923.md) | B | `8d6b2e41` 02:48 | méthode | clos (e5688680) | acceptée |
| [`CONTRE_AUDIT_B_RECEPTION_V8_GARDES_WIP_20260923.md`](CONTRE_AUDIT_B_RECEPTION_V8_GARDES_WIP_20260923.md) | B | `39e37e9d` 04:18 | méthode | clos (cc4664e5) | acceptée |
| [`PROTOCOLE_TOUR_V10_V5_RUPTURE_20260923.md`](PROTOCOLE_TOUR_V10_V5_RUPTURE_20260923.md) | B ? | `b6b81ba4` 05:47 | méthode | clos (f55ea40c) | acceptée |
| [`CONTRE_AUDIT_B_PORT_V3_SATURATION_TRI_20260923.md`](CONTRE_AUDIT_B_PORT_V3_SATURATION_TRI_20260923.md) | B | `495edc0c` 00:27 | mesure | historique | acceptée |
| [`CONTRE_AUDIT_B_PREMIERE_CAMPAGNE_20260922.md`](CONTRE_AUDIT_B_PREMIERE_CAMPAGNE_20260922.md) | B | `174fb8db` 22/23:18 | mesure | historique | sans objet |

## LiDAR : croissance, découpes, densité

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`CONTRE_AUDIT_LIDAR_SCALING_V12_LOCAL_20260923.md`](CONTRE_AUDIT_LIDAR_SCALING_V12_LOCAL_20260923.md) | B | `87ccf5fc` 08:21 | mesure | clos | sans objet |
| [`CONTRE_AUDIT_PENTE_LIDAR_LOCALE_PARTIELLE_20260923.md`](CONTRE_AUDIT_PENTE_LIDAR_LOCALE_PARTIELLE_20260923.md) | B | `4c3344ea` 07:48 | mesure | remplacé par CONTRE_AUDIT_LIDAR_SCALING_V12_LOCAL | acceptée |
| [`DECOUPES_CAPTEUR_LIDAR_BRUT_ET_GRILLE_20260923.md`](DECOUPES_CAPTEUR_LIDAR_BRUT_ET_GRILLE_20260923.md) | A ou B | `c02d45ac` 08:17 | mesure | vivant | sans réponse |
| [`CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md`](CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md) | A ou B | `bbc41a9c` 08:26 | mesure | vivant | sans réponse |
| [`lidar_density_full_3scenes_20260923/`](lidar_density_full_3scenes_20260923/README.md) | A ou B | `c17f8df8` 08:41 | mesure | vivant | sans objet |
| [`lidar_density_scene02_20260923/`](lidar_density_scene02_20260923/README.md) | A ou B | `c17f8df8` 08:41 | mesure | vivant | sans objet |
| [`lidar_density_sectors_00_01_20260923/`](lidar_density_sectors_00_01_20260923/README.md) | A ou B | `53d8fac3` 08:58 | mesure | vivant | sans objet |
| [`lidar_density_bbox_fixed_20260923/`](lidar_density_bbox_fixed_20260923/README.md) | A ou B | `1200d343` 09:37 | mesure (ablation appariée) | vivant | sans objet |
| [`lidar_scene02_physical_cut_20260923/`](lidar_scene02_physical_cut_20260923/README.md) | A | 23/soir | 08/000200 sans sol K10 : un retour change de quart entre signe quantifié et float32 ; le quart chaud garde p=2,035350 | vivant, une relation vérifiée | sans objet |
| [`lidar_raw_physical_scaling_20260923/`](lidar_raw_physical_scaling_20260923/README.md) | A | 23/11:30–11:52 | mesure locale 21 cas, trame brute entière K5, sept secteurs physiques × trois densités ; reçu reproductible | vivant | sans objet |
| [`CONTRE_AUDIT_B_LIDAR_BRUT_PHYSIQUE_20260923.md`](CONTRE_AUDIT_B_LIDAR_BRUT_PHYSIQUE_20260923.md) | B | `53a2e5d29` 11:50 + erratum | contrelecture de la matrice brute K5 ; champ hors extrémités distingué du total effectivement calculé | vivant pour les mesures corrigées | sans objet |
| [`lidar_raw_k10_density_20260923/`](lidar_raw_k10_density_20260923/README.md) | A | 23/12:18–12:23 | mesure locale 3 cas, même trame brute et mêmes densités, tour K10 ; reçu reproductible | vivant | sans objet |
| [`CONTRE_AUDIT_B_LIDAR_BRUT_K10_20260923.md`](CONTRE_AUDIT_B_LIDAR_BRUT_K10_20260923.md) | B | `82efba156` 12:32 + erratum | contrelecture du plein brut K10 ; sous-total hors extrémités et total payé séparés | vivant pour les mesures corrigées | sans objet |
| [`lidar_raw_k10_sectors_20260923/`](lidar_raw_k10_sectors_20260923/README.md) | A | 23/12:49–13:09 | matrice K10, sept secteurs × trois densités ; ses « formes » sont hors extrémités, [total corrigé](CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md) | vivant, reçu scellé inchangé | sans objet |
| [`lidar_raw_hot_quarter_multiseed_20260923/`](lidar_raw_hot_quarter_multiseed_20260923/README.md) | A | 23/soir | même quart brut chaud, deux nouvelles graines globales × K5/K10 ; K10 p_core 1/2→plein = 2,029/1,971/2,095 selon graine | vivant, un quart et trois graines ; pas de borne globale | sans objet |
| [`lidar_ground_hot_quarter_multiseed_20260923/`](lidar_ground_hot_quarter_multiseed_20260923/README.md) | A | 23/soir | sans sol 08/000200, quart physique chaud K10, trois graines globales ; p_core 1/4→1/2 >2 dans les trois tirages | vivant, un quart et trois graines ; pas de borne globale | sans objet |
| [`s4a_ground_hot_quarter_20260923/`](s4a_ground_hot_quarter_20260923/README.md) | A | 24/00:00 | six bras du quart chaud 08/000200/K10, réemployés et revérifiés dans le panneau complet ci-dessous | historique, reçu conservé comme provenance | sans objet |
| [`s4a_cpu_scene02_physical_panel_20260924/`](s4a_cpu_scene02_physical_panel_20260924/README.md) | A | 24/02:30 | 21 entrées physiques × S3/S4a CPU K10, densités emboîtées ; 21 paires de sorties égales, 18 pentes spatiales, 14 de densité, 9 ratios de découpe, 178 empreintes | reçu clos, une scène/une graine, aucun transfert G4 | sans objet |
| [`s2_half_density_k5_20260923/`](s2_half_density_k5_20260923/README.md) | A | 23/18:20 | S2 CPU K5, plein et deux demi-scènes × trois densités, neuf paires moteur/lot ; six compteurs égaux au v12 | vivant, une scène | sans objet |
| [`ATTRIBUTION_COEUR_ARETES_COUPES_LIDAR_20260923.md`](ATTRIBUTION_COEUR_ARETES_COUPES_LIDAR_20260923.md) | A | 23/16:33 | méthode et identité exacte de l'attribution par arête | protocole exécuté à K5 dans le reçu suivant | sans objet |
| [`edge_matched_core_20260923/`](edge_matched_core_20260923/README.md) | A | 23/17:00 | mesure S2 CPU, 15 couples plein/quarts × densités, trace post-cœur et sélecteur intrinsèque exploratoire | vivant, une seule scène/K5 | sans objet |
| [`lazy_prefix_dead_core_20260923/`](lazy_prefix_dead_core_20260923/README.md) | A | 23/17:28 | shadow S2 CPU du préfixe exact des formes du cœur, plein brut K5 aux trois densités ; 43,42 % de suffixe non consulté au plein, pente résiduelle 2,024 | vivant, potentiel de travail sans gain CPU mesuré | sans objet |
| [`q34_batch_density_quarter_20260923/`](q34_batch_density_quarter_20260923/README.md) | A | 23/15:39 | rejeu S2/v17 CPU apparié sur un quart brut K5 à trois densités ; six sorties vérifiables | vivant, portée bornée | sans objet |
| [`q34_raw_rectangle_mass_20260923/`](q34_raw_rectangle_mass_20260923/README.md) | A | 23/16:00 | histogrammes q3/q4 K5/K10 sur une trame brute entière ; cinq comptes v12 appariés et budget S2a | vivant, mesure locale | sans objet |
| [`s2_segment_mass_20260923/`](s2_segment_mass_20260923/README.md) | A | 23/soir | jointure S2 brute K5 : 0,44 % des rectangles ouverts ont ≥16 survivantes et portent 85,52 % des formes du cœur ; sélecteur pré-cœur mesurable | vivant, sélecteur évalué dans le shadow ci-dessous | réponse D attendue |
| [`s2_segment_panel_20260923/`](s2_segment_panel_20260923/README.md) | A | 23/soir | 15 jointures S2 plein/quarts × densités 1/4,1/2,1 ; F des segments ≥16 augmente sur le plein, mais coût indicatif très variable selon le quart | vivant, seuil non universel | réponse D attendue |
| [`s2_precore_node_shadow_20260923/`](s2_precore_node_shadow_20260923/README.md) | A | 23/soir | certificat exact à huit cellules sur segments S2 lourds : 0,0818–0,1345 % de F du plein fermable pour 4,6–15,3 M visites ; 39 divergences post-cœur jugées sans q3/q4 | vivant, schéma grossier non rentable ; pistes de cellules locales | réponse D attendue |
| [`s2_precell_incidence_20260923/`](s2_precell_incidence_20260923/README.md) | A | 23/soir | crible entier d'incidence par cellule sur les mêmes arêtes S2 : +0,0254 % de F total fermable sur le plein, zéro sur le quart | vivant, simple `E_C` trop lâche ; pas de port produit | réponse D attendue |
| [`paired_guards_precore_20260923/`](paired_guards_precore_20260923/README.md) | A | 23/soir | certificat exact par paires disjointes ; fixture S2 ouverte, puis 27/60 et 40/60 fermetures sur deux échantillons LiDAR lourds | shadow oracle à scan complet, gain de chaîne inconnu | réponse D attendue |
| [`CONTRE_AUDIT_B_PAIRES_GARDES_PRECOEUR_20260923.md`](CONTRE_AUDIT_B_PAIRES_GARDES_PRECOEUR_20260923.md) | B | 23/soir | SHA/lecteurs/formules et 120 `F` recroisés ; 27/60→40/60 reflète aussi plus de q4-seul ; sélection par `F` post-cœur et scan ×28,47 | certificat positif confirmé, sélection indexée pré-cœur et gain de chaîne ouverts | transmis à A/D |
| [`paired_guard_index_shadow_20260923/`](paired_guard_index_shadow_20260923/README.md) | A | 23/soir | top-B par index exact sur 120 arêtes lourdes ; 67 fermées, 6 909 sites testés à B16 ; déclencheur `D≥2²²` toucherait 909 278 survivantes S2 | candidat pré-cœur, gain de chaîne non mesuré | réponse D attendue |
| [`paired_guard_node_blocks_20260923/`](paired_guard_node_blocks_20260923/README.md) | A | 23/soir | certificat exact de produits de nœuds disjoints ; cap4 ferme 32 contre 21 à budget64, 67 contre 67 à budget256 ; 758 preuves LIVE | gain local sous budget, routage et coût global ouverts | réponse D attendue |
| [`CONTRE_AUDIT_B_COUT_NOEUDS_PAIRES_20260923.md`](CONTRE_AUDIT_B_COUT_NOEUDS_PAIRES_20260923.md) | B | 23/soir | second calcul des bornes non compté au pop : cap4/64 88 108 vs 57 794 affichées, cap4/256 129 753 vs 82 160 | preuves intactes, comparaison de coût à corriger | transmis à A/D |
| [`CERTIFICAT_B_PAIRES_GARDES_RECTANGLE_20260923.md`](CERTIFICAT_B_PAIRES_GARDES_RECTANGLE_20260923.md) | B | 23/soir | levée exacte à tout `A×B` par 64 couples de coins ; deux nœuds témoins par 4096 quadruplets ou borne conservative ; fixtures arithmétiques | math établi, premier shadow LiDAR ci-dessous ; aucun port/gain | transmis à A/D |
| [`rect_pair_shadow_b_20260923/`](rect_pair_shadow_b_20260923/README.md) | B | 23/22:04 | tous les 1 747 grands rectangles bruts 08/000000/K5/s8 ; 72/299 positifs fermés, 5,060 M/559,662 M formes fermables (0,904 %) pour 26,845 M coins testés | shadow source/agrégats, palette uniforme insuffisante ; aucune mesure G4 | transmis à A/D |
| [`CERTIFICAT_B_GROUPES_GARDES_Q34_20260923.md`](CERTIFICAT_B_GROUPES_GARDES_Q34_20260923.md) | B | 23/22:18 | généralisation exacte à r gardes et 64 coins ; fixture K5 avec quatre triples fermants mais aucune des 66 paires, plus fixture 64 coins | math/test entier ; un shadow LiDAR borné ci-dessous | réponse D attendue |
| [`rect_guard_triples_b_20260923/`](rect_guard_triples_b_20260923/README.md) | A | 23/soir | 1 747 grands rectangles bruts K5/s8, sélection top64 triples par voie ; deux fermetures positives de plus que paires max, 53 523 F (0,00956 % global) pour 2,547 M triples énumérés sur les 696 replis | reçu/lecteur exact sur deux preuves ; heuristique peu sélective, aucun gain net/G4 | réponse D attendue |
| [`weighted_guard_pairs_20260923/`](weighted_guard_pairs_20260923/README.md) | A | 23/soir | certificat exact de paires à rapport positif fixe sur 64 coins ; quatre rapports testés sur 1 747 grands rectangles bruts K5/s8, +15 fermetures S2-positives et 5,634 M F ; 105 paires/6 720 coins archivés | puissance géométrique accrue, mais 58,086 M coins supplémentaires et aucun gain net/G4 mesuré | réponse D attendue |
| [`paired_guard_dispatch_grid_20260923/`](paired_guard_dispatch_grid_20260923/README.md) | A | 23/soir | crible pré-cœur longueur + occupation : 147 406/3 986 433 arêtes routées, 98,13 % de la masse des gros cœurs ; les grosses arêtes d'un groupe portent 367,510 M formes ; bornes de groupe vérifiées | routage seulement, preuve partagée et coût de chaîne ouverts | réponse D attendue |
| [`paired_guard_group_bvh_20260923/`](paired_guard_group_bvh_20260923/README.md) | A | 23/soir | BVH 6D du plus grand groupe S2 brut K5 : palette pré-cœur de 51 paires, 24 537/67 827 arêtes entièrement closes, F potentiel 153,838 M ; 1,974 M visites de sélection + 187 790 tests uniformes | shadow exact sur un groupe, gain net et croissance globale ouverts | réponse D attendue |

## GPU et parallélisme

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`OBSTACLES_GPU_SOUS_SECONDE_20260923.md`](OBSTACLES_GPU_SOUS_SECONDE_20260923.md) | B ? | `c717a1f2` 05:56 | diagnostic R7b CPU antérieur aux reçus v12/R12/R13 | historique, remplacé pour l'état courant | sans objet |
| [`CONTRE_AUDIT_B_WIP_V6_C6_TRI_20260923.md`](CONTRE_AUDIT_B_WIP_V6_C6_TRI_20260923.md) | B | `73d1d0a6` 05:31 | démontré localement | vivant (hors v9) | sans réponse (question canal l. 35) |
| [`CONTRE_AUDIT_B_PORTE_FILTRE_GPU_20260923.md`](CONTRE_AUDIT_B_PORTE_FILTRE_GPU_20260923.md) | B | `7a7987ad3` 12:58 | budget R11 et préflight du port CUDA S1 ; première lecture historique | vivant | sans réponse |
| [`CONTRE_AUDIT_B_G4_S1_PUBLIE_20260923.md`](CONTRE_AUDIT_B_G4_S1_PUBLIE_20260923.md) | B | 23/13:51 | préflight historique du paquet S1 publié | clos par les deux reçus G4 | réponse D |
| [`CONTRE_AUDIT_B_G4_GPU_S1_SESSIONS_20260923.md`](CONTRE_AUDIT_B_G4_GPU_S1_SESSIONS_20260923.md) | B | 23/15:12 | deux sessions G4, six cas de filtre exact, portée et budget restant | historique S1 ; chaîne hybride mesurée en R12/R13, égalité littérale du catalogue et contrat ouverts | réponse D |
| [`CONTRELECTURE_G4_R12_S2_20260923.md`](CONTRELECTURE_G4_R12_S2_20260923.md) | A | 23/16:25 | reçu G4 S2 : six paires distinctes, attribution lots/CUDA, plancher survivants + tour et diagnostic de croissance | vivant | sans réponse |
| [`CONTRE_AUDIT_B_RACCORD_Q34_BATCH_WIP_20260923.md`](CONTRE_AUDIT_B_RACCORD_Q34_BATCH_WIP_20260923.md) | B | 23/15:10 | préflight du raccord q3/q4 batch : confiance, mémoire, ledger ; sources publiées à `a6d81f9ce` | vivant, qualifications ouvertes | réponse D attendue |
| [`q34_batch_duplicate_gate_20260923/`](q34_batch_duplicate_gate_20260923/README.md) | A | 23/15:20, rejeu 16:14 | doublon à comptes constants refusé par `2059189d` ; mutant intégré rendu causal par `c265a5da` | clos pour le contre-exemple et sa porte | corrigée |
| [`s2_partial_twin_20260923/`](s2_partial_twin_20260923/README.md) | A | 23/16:10 | cas GPU achevé sans jumeau dans un reçu partiel ; `c265a5da` le marque `unpaired_batch_cases` et refuse l'effacement | clos pour le marquage | corrigée |
| [`CONTRE_AUDIT_B_PROTOCOLE_G4_S2_WIP_20260923.md`](CONTRE_AUDIT_B_PROTOCOLE_G4_S2_WIP_20260923.md) | B | 23/15:25, clôture 17:4x | préflight historique S2 ; faux marqueur GPU clos par `f9e6a5527`, différentiel catalogue GPU encore ouvert | défaut de libellé clos, R12 inchangé | corrigée |
| [`CONTRE_AUDIT_B_CROISSANCE_Q34_AVAL_S2_20260923.md`](CONTRE_AUDIT_B_CROISSANCE_Q34_AVAL_S2_20260923.md) | B | 23/15:30 | ratios 8/16/32k et brute : formes cœur/cover restent le coût aval majeur | vivant pour S2 et massif | réponse D attendue |
| [`CONTRE_AUDIT_B_VENTILATION_SURVIVANTS_20260923.md`](CONTRE_AUDIT_B_VENTILATION_SURVIVANTS_20260923.md) | B | 23/soir | sonde CPU sans-sol : phases atlas/voies dominantes, 1,3 % non borné ; même zéro survivant ne suffit pas seul en R12 | diagnostic, pas G4 S3 | réponse D attendue |
| [`CONTRELECTURE_CYCLES_SURVIVANTS_Q34_20260923.md`](CONTRELECTURE_CYCLES_SURVIVANTS_Q34_20260923.md) | A | 23/soir | `rest_cyc` recouvre le cœur ouvert ; TSC ventilé mesure le temps écoulé sous contention, pas les cycles CPU actifs | correction du diagnostic | réponse D attendue |
| [`CONTRE_AUDIT_B_S3_CERTIFICAT_WIP_20260923.md`](CONTRE_AUDIT_B_S3_CERTIFICAT_WIP_20260923.md) | B | 23/soir | préflight S3 ; gardes d'entrée closes par 942, épingles de catalogue par 18d, classement GPU et compte de covers par 46c | R13 S3 reçu ; porte CUDA du lot vide encore ouverte | réponse D |
| [`CONTRE_AUDIT_B_G4_R13_S3_20260923.md`](CONTRE_AUDIT_B_G4_R13_S3_20260923.md) | B | 23/soir | reçu G4 S3 : 18 cas, six épingles ; intervalle CUDA inclut allocations ; port 128 registres post-R13 non mesuré ; 0,611–1,555 M covers GPU reconstruits sur CPU/cas, visites du second parcours hors ledger logique | vivant, coût isolé du rebuild et plages ouvertes à mesurer ; contrat non acquis | addendum D, domaine massif ouvert |
| [`CONTRE_AUDIT_B_PREFLIGHT_G4_R14_V19_20260923.md`](CONTRE_AUDIT_B_PREFLIGHT_G4_R14_V19_20260923.md) | B | 23/22:13 | préflight statique : garde 2 Gio annoncée absente du lanceur ; « kernel/transfert » v19 mélange copies/allocations et lecteur tolère résidu non attribué | R14/R15 reçus ; garde et attribution fine restent ouvertes, aucun essai GCP B | réponse D attendue |
| [`CONTRELECTURE_G4_R14_RECU_20260923.md`](CONTRELECTURE_G4_R14_RECU_20260923.md) | A | 23/soir | reçu G4 R14 : 326 SHA, 18 cas reçus, 12 GPU ; ventilation réelle fermée mais quatre sous-déclarations acceptées par le lecteur ; garde disque absente | mesure relative reçue ; attribution fine et préflight disque ouverts | réponse D attendue |
| [`CONTRELECTURE_G4_R15_S4A_20260923.md`](CONTRELECTURE_G4_R15_S4A_20260923.md) | A | 23/nuit | reçu G4 R15 : 326 SHA, 18 cas, S4a gagne 0,163–0,177 s K5 et 0,374–0,409 s K10 sur paires entrelacées ; reste conditionnel hors arêtes K5 1,072–1,350 s | mesure relative reçue ; coupes/densités S4a et contrat 1 s ouverts | réponse D attendue |
| [`LECTURE_R14_G4_PLANCHER_CONDITIONNEL_20260923.md`](LECTURE_R14_G4_PLANCHER_CONDITIONNEL_20260923.md) | A | 23/soir | paires S3 R14 0,221/0,230 s K5 et 0,794/0,785 s K10 ; S4 gratuit sur survivants laisse 1,024–1,305 s K5 à phases fixes | budget conditionnel, aucun contrat | réponse D attendue |
| [`AUDIT_S4_RESIDENCE_ORDINALS_20260923.md`](AUDIT_S4_RESIDENCE_ORDINALS_20260923.md) | A | 23/soir | S4a GPU q3 / CPU q4 : export hôte compact malgré la résidence, ordinal S2 et masques monotones ; budget arène non borné, coût q4 pondéré ; jumeau CPU/GPU aveugle aux omissions communes | contrelecture de conception, sans port ni chrono | réponse D attendue |
| [`CONTRE_AUDIT_B_PLAN_S4_LEDGER_ET_BUDGET_20260923.md`](CONTRE_AUDIT_B_PLAN_S4_LEDGER_ET_BUDGET_20260923.md) | B | 23/soir | q4 : une graine donne deux émissions dans une fixture v9 directe ; ledger du plan faux à l'unité graine ; enregistrement ≥129 o avant alignement ; routage et budget arène non bornés | préflight historique pour S4a ; unité graine/groupe/émission encore ouverte pour S4b | transmis à D |
| [`s4_q4_multigroup_probe.cpp`](s4_q4_multigroup_probe.cpp) | B | 23/soir | micro-test reproductible u18/K3 de double émission Window30 et Local28 pour une seule graine | test causal direct v9 local ; pas une porte produit | transmis à D |
| [`AUDIT_S4_WIP_EXCEPTIONS_WORKERS_20260923.md`](AUDIT_S4_WIP_EXCEPTIONS_WORKERS_20260923.md) | A | 24/00:00 | `507580243` ferme au code le faux refus sous 65 536 sites, les planchers du jumeau moteur et la comparaison q4 du bras reporté ; le gate de chaîne exercé avait au moins 87 064 occurrences q3 reportée/q4 ouverte ; injection de panne bornée encore utile à la place de l'essai 512 Gio | R15 G4 historique ; réception corrigée après capture, mesure de croissance S4a ouverte | corrigée en partie |
| [`AUDIT_S4_COUPLAGE_GRAINES_COVER_20260923.md`](AUDIT_S4_COUPLAGE_GRAINES_COVER_20260923.md) | A | 23/soir | R14/raw ne joignent pas graines et covers par arête ; plan S4 et port local ont deux répartitions de warp, avec `N≤P+Σg` ; les 146 M tests q3 annoncés sont logiques, les ballots effectifs non mesurés | méthode de mesure/gate par graine ; R15 reçu sans cette ventilation physique | réponse D attendue |
| [`AUDIT_S4A_VALIDATION_ET_ARENE_20260923.md`](AUDIT_S4A_VALIDATION_ET_ARENE_20260923.md) | A | 24/00:00 | S2/S3/S4a refont la validation par point et par nœud hors événements CUDA ; 3 008 warps demandent analytiquement 7,344 Gio de slabs fixes sur 08/000000, avant index/arène ; reports GPU absents du ledger `lanes_*` | capacité calculée, pas pic VRAM mesuré ; validation et massivité non isolées | réponse D en partie |
| [`AUDIT_S4B_J8_BIT_FINAL_20260924.md`](AUDIT_S4B_J8_BIT_FINAL_20260924.md) | A | 24/00:32 + suite | scratch S4b J8 : bit final corrigé ; DESIGN peut faire `m²` comparaisons par seau ; raffinement égal au produit q4 sur une trame mais sous-compte le coût ; port HostGroup WIP : bit valide inter-groupes détecté puis corrigé localement | trois fixtures/gates causaux ; compteurs de liste, raccord de tâches et qualification produit/G4 encore ouverts | transmis via audit |
| [`s4b_compare_gate_v21_20260924/`](s4b_compare_gate_v21_20260924/README.md) | A | 24/02:42 | porte fichier HostGroup S4b WIP : `equal=1` possible avec zéro graine/émission ; fixture tétraédrique à grille J8 dégénérée ; coquilles comparées par empreinte seulement | gate à renforcer, aucun défaut géométrique confirmé | transmis via audit |
| [`s4b_j8_bucket_fixture_20260924.py`](s4b_j8_bucket_fixture_20260924.py) | A | 24/00:50 | oracle entier autonome pour six tailles de la famille de seau q4, jusqu'à 6 554 événements | fixture mathématique, pas exécution du produit | sans objet |
| [`s4b_refined_group_fixture_20260924.py`](s4b_refined_group_fixture_20260924.py) | A | 24/01:30 | oracle entier autonome pour trois tailles de groupe cosphérique, jusqu'à 1 439 événements de même racine | fixture mathématique, groupe local q4 ; FULL refuse la grande coquille | sans objet |
| [`s4b_valid_group_gate_20260924.cpp`](s4b_valid_group_gate_20260924.cpp) | A | 24/01:42, reprise 01:46 | gate HostGroup u18 de deux racines positives dans un même seau : second support erroné dans le premier source WIP, puis `0,1,2,4` correct avec code 0 en `-O1`/`-O2` | défaut local clos ; source encore non publié, porte de flux complet ouverte | transmis via audit |
| [`s3_frontier_barrier_gate_20260923/`](s3_frontier_barrier_gate_20260923/README.md) | A | 23/soir | gate hôte causal du réemploi du frontier : ordre ancien/mutant WAW+WAR, correctif 545c à zéro ; headers épinglés | porte structurelle, pas preuve device | réponse D attendue |
| [`CERTIFICAT_B_REDONDANCE_CELLULE_UNIQUE_20260923.md`](CERTIFICAT_B_REDONDANCE_CELLULE_UNIQUE_20260923.md) | B | 23/soir | preuve : boîte de centres entière redondante avec le citron S2 ; clipping réel ou gardes distincts par sous-cellule, shadow O(R+S) borné | math établi | réponse D attendue |
| [`CERTIFICAT_B_NOEUDS_CORRELES_AVANT_COEUR_20260923.md`](CERTIFICAT_B_NOEUDS_CORRELES_AVANT_COEUR_20260923.md) | B | 23/soir | crédit par sous-cellule et vues `E×C` avec gardes explicites ; premier crible `E_C` sur huit cellules mesuré, faible fermeture | vue `E×C` raffinée et gain de chaîne non mesurés | réponse D en partie |
| [`bbox_clipping_s2_20260923/README.md`](bbox_clipping_s2_20260923/README.md) | B | 23/soir | recomptage exact S2 brut 08/000000/K5 : cellule unique globalement clippée ne peut concerner que 1,494 % des incidences cœur ; script et hashes | plafond potentiel, pas gain | réponse D attendue |
| [`precore_cell_screen_20260923/`](precore_cell_screen_20260923/README.md) | A | 23/soir | autre trace S2, plafond plus strict de chargement entier aux trois densités et quatre quarts ; borne corrélée exacte de sous-cellule avec fixture u18 | crible mesuré, math locale ; gain non testé | réponse D attendue |
| [`CONTRE_AUDIT_B_JUGE_Q3_CRL_V7_20260923.md`](CONTRE_AUDIT_B_JUGE_Q3_CRL_V7_20260923.md) | B | 23/17:00 | juge q3 v7 : strate critique longue ciblée ; garde d'IDs fermée ensuite en v8 avec six mutants et 23 sorties v7 identiques | audit historique, garde close | transmis à C/D |
| [`CONTRE_AUDIT_B_VALIDATION_INDEX_GPU_S1_20260923.md`](CONTRE_AUDIT_B_VALIDATION_INDEX_GPU_S1_20260923.md) | B | 23/13:49 | preuve du coût de la garde brute et certificat linéaire proposé | vivant | sans réponse |
| [`AUDIT_A_GPU_S1_DOMAINE_U18_20260923.md`](AUDIT_A_GPU_S1_DOMAINE_U18_20260923.md) | A | 23/13:17 | contre-exemples fermés par `7565451fc` ; certificat d'index linéaire réutilisable pour S2 | clos pour les défauts, vivant pour le coût | corrigée |
| [`PROPOSITION_B_GPU_STREAMING_S2_20260923.md`](PROPOSITION_B_GPU_STREAMING_S2_20260923.md) | B | `8432e2353` 13:28 | tuilage borné S2a et certificat bloc/ligne S2b avec portes causales | vivant | sans réponse |

## Annexes de l'auditeur C

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`c_audit_20260923/`](c_audit_20260923/README.md) | C | `f4480c02` 08:44 | pièces de l'audit C | vivant | sans objet |
| [`c_euler_20260923/`](c_euler_20260923/README.md) | C | `cab281d8` 08:25 | testé, sondes | vivant | sans objet |
| [`c_alternatives_20260923/`](c_alternatives_20260923/README.md) | C | 23/10:12 | propositions, jurys, réfutations, expériences | vivant | sans objet |
| [`c_omission_20260923/`](c_omission_20260923/README.md) | C | 23/10:49 | shadow, échantillons 8k et juge q2 sur trame sans sol entière | vivant | sans objet |
| [`c_catalogue_digest_20260923/`](c_catalogue_digest_20260923/README.md) | C | 23/19:35 | condensé v18 du catalogue : contrôle sur les trames R12, six épingles R13, porte portée par 18d | six épingles retrouvées dans R13 ; FNV-64, pas égalité littérale | reprise D |
| [`PROPOSITION_C_CATALOGUE_SCELLE_20260924.md`](PROPOSITION_C_CATALOGUE_SCELLE_20260924.md) et [`c_catalogue_scelle_20260924/`](c_catalogue_scelle_20260924/README.md) | C | 24/20:25 | catalogue scellé : positivité des supports réguliers d'abord dans la chaîne, puis passe 1 de la tour sautée sous sceau ; mesure locale de la passe 1 | vivant | sans réponse |
| [`AUDIT_C_GRAND_AUDIT_V9_20260924.md`](AUDIT_C_GRAND_AUDIT_V9_20260924.md) | C | 24/20:49 | grand audit demandé par l'utilisateur : ce qui marche, où va le temps, trames avec sol, classement des changements, 100 ms infaisable avec les algorithmes connus | vivant | sans réponse |
| [`c_raw_pins_20260924/`](c_raw_pins_20260924/README.md) | C | 24/20:49 | épingles CPU des trames brutes b00/b01/b02 de R21 (en cours) | vivant | sans objet |

## Sondes et sorties à la racine du dossier

Elles restent à leur place : au moins 23 notes les citent par chemin relatif et
quatre contiennent leur propre chemin dans leur ligne de compilation. Les
sorties `shadow_ha_*_20260923.txt` appartiennent aux deux notes `SHADOW_HA_*`.

| sonde | auteur | sujet |
| --- | --- | --- |
| [`check_u18_bounds_20260922.py`](check_u18_bounds_20260922.py) | A | u18 |
| [`check_qmin_planes_u18_20260922.py`](check_qmin_planes_u18_20260922.py) | A | u18, q_min |
| [`check_plateau_u18_wide_20260922.cpp`](check_plateau_u18_wide_20260922.cpp) | B | u18, plateaux |
| [`check_q3_shared_u18_20260922.py`](check_q3_shared_u18_20260922.py) | A | q3 |
| [`check_q3_atlas_rational_location_20260923.py`](check_q3_atlas_rational_location_20260923.py) | A | q3 |
| [`check_q3_leaf_palette_20260923.py`](check_q3_leaf_palette_20260923.py) | A | q3 |
| [`check_q4_shallow_lines_20260922.py`](check_q4_shallow_lines_20260922.py) | A | q4 |
| [`check_q4_outward_levels_20260922.py`](check_q4_outward_levels_20260922.py) | A | q4 |
| [`check_q4_without_q3_faces_20260922.py`](check_q4_without_q3_faces_20260922.py) | B | q4 |
| [`check_q4_false_vertex_inside_disc_20260922.py`](check_q4_false_vertex_inside_disc_20260922.py) | B | q4 |
| [`check_q4_k5_interior_chain_20260922.cpp`](check_q4_k5_interior_chain_20260922.cpp) | B | q4 |
| [`check_q4_owner_dominance_20260923.py`](check_q4_owner_dominance_20260923.py) | B | q4 |
| [`check_q4_block_dominance_20260923.py`](check_q4_block_dominance_20260923.py) | A | q4 |
| [`check_q4_lane_threshold_20260923.py`](check_q4_lane_threshold_20260923.py) | A | q4 |
| [`check_q4_global_padding_20260923.cpp`](check_q4_global_padding_20260923.cpp) | B | q4, complétude |
| [`check_cover_batch_u18_20260922.py`](check_cover_batch_u18_20260922.py) | A | q3/q4, cover |
| [`check_q34_core_stream_local_20260923.cpp`](check_q34_core_stream_local_20260923.cpp) | A ou B | q3/q4, cœur |
| [`check_q34_dead_owner_aba_20260923.cpp`](check_q34_dead_owner_aba_20260923.cpp) | A | reproducteur historique ABA, porté dans la porte produit q3/q4 |
| [`q34_block_shadow_20260923.cpp`](q34_block_shadow_20260923.cpp) | A ? | q3/q4, shadow |
| [`shadow_q34_rows_u18_20260923.cpp`](shadow_q34_rows_u18_20260923.cpp) | A | q3/q4, shadow |
| [`shadow_ha_q34_u18_20260923.cpp`](shadow_ha_q34_u18_20260923.cpp) | B ? | palette, shadow |
| [`shadow_ha_octant_q34_u18_20260923.cpp`](shadow_ha_octant_q34_u18_20260923.cpp) | B ? | palette octant, shadow |
| [`check_full_intruder_prefix_20260923.py`](check_full_intruder_prefix_20260923.py) | A | FULL, intrus |
| [`check_meb_proposed_fenv_20260923.cpp`](check_meb_proposed_fenv_20260923.cpp) | B ? | FULL, MEB |
| [`check_phase_a_temporal_max_id_20260923.py`](check_phase_a_temporal_max_id_20260923.py) | B | FULL, phase A |
| [`check_shell_region_quotient_small_20260922.py`](check_shell_region_quotient_small_20260922.py) | B | FULL, coquilles |

## Notes retirées

Supprimées du dossier, lisibles dans l'historique Git.

| note | auteur | retrait |
| --- | --- | --- |
| `CONTRE_AUDIT_B_GCP_SESSION_20260922.md` | B | retirée à 1bbb38e6, lisible à b0afa8c9 |
| `CONTRE_AUDIT_B_PREMIER_G4_20260922.md` | B | retirée à 1bbb38e6, remplacée par CONTRE_AUDIT_B_G4_R1_ET_SCHEMA_V2 |
| `CONTRE_AUDIT_B_G4_R4_PREVOL_20260923.md` | B | retirée à 1bbb38e6 |
| `CONTRE_AUDIT_B_PROTOCOLE_V4_WIP_20260923.md` | B | retirée à 128fb231, lisible à 4d91329f |
| `CONTRE_AUDIT_B_VOIES_MORTES_WIP_20260923.md` | B | retirée à 128fb231 |
| `CONTRE_AUDIT_B_CACHE_TEMOINS_WIP_20260923.md` | B | retirée à 128fb231, remplacée par CACHE_TEMOINS_COUT_VALIDATION |
| `CONTRE_AUDIT_B_TRI_FUSION_WIP_20260923.md` | B | retirée à 128fb231, remplacée par CONTRE_AUDIT_B_SAMPLE_SORT_PUBLIE |
| `CONTRE_AUDIT_B_MEB_PROPOSE_WIP_20260923.md` | B | retirée à 85d79753, lisible à 02d55856, remplacée par MEB_PROPOSITION_EXACTE |
| `FULL_FILTRE_ABSENCE_CLE_20260923.md` | B ? | retirée à 85d79753 |
| `CONTRE_AUDIT_B_PROTOCOLE_G4_FILTRE_S1_WIP_20260923.md` | B | préflight mutable clos par `1c9c1e5d7`/`7565451fc` ; version historique `ac197535a`, statut courant dans `CONTRE_AUDIT_B_G4_S1_PUBLIE_20260923.md` |

## Hors du dossier

- `audits/morsehgp3D_v9/` à la racine du dépôt (B) : `AUDIT_INITIAL_V8_20260922.md`,
  `ETAT_COURANT.md` (photographie historique de `74aa8172`, requalifiée par
  `607e5d7de`) et `public_chain_t2_gate.cpp`.
- Les reçus du développeur sont dans `morsehgp3D_v9/receipts/`.

## Suivi des recommandations

Registre tenu par C à partir du canal. Le développeur répond **dans le canal**
en citant l'identifiant, avec « acceptée », « refusée (raison) » ou « différée
(échéance) ».

| id | recommandation | proposée par | source |
| --- | --- | --- | --- |
| R-01 | Invariant d'Euler dans la sonde et le lecteur G4 ; porte `scale8000` ; protocole Kmax+2 — **porté** : sonde v13 `c768e06a` (refus `chain_catalogue_euler_violated`), porte `a08378da`, lecteurs `50646eef` et `515b3666` ; Kmax+2 exercé sur une fixture synthétique 8k K5→K7, sans campagne sur trames entières ; hygiène de la porte et juge d'échantillon publiés en `96bd6190` ; reste : R-19 | C | NOTE_C_INVARIANT_EULER_20260923.md |
| R-02 | Réparer la CI (selftest `HEAD~1`, chemin absolu de la porte de pente) — **corrigé** `4b6e3aa6` | C | canal, entrée de 08 h 34 UTC |
| R-03 | T2 de chaîne à Kmax ∈ {1,2,3,5,10} ; mutants de recoupe et du cœur FULL | C | AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md § 4 et 6 |
| R-04 | Inscrire au registre : extension non régulière, complétude q2/q3/q4, invariant d'Euler ; corriger le README | C | AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md § 1.5 |
| R-05 | Question C6/tri v6 | B | CONTRE_AUDIT_B_WIP_V6_C6_TRI_20260923.md, canal l. 35 |
| R-06 | Induction globale q4 et passage amont | B | COMPLETUDE_Q4_CLE_REMBOURREE, Q4_INDUCTION_ATLAS_EVENEMENTS, Q34_PROPRIETAIRE_PASSAGE_AMONT |
| R-07 | Couture de la phase A FULL (pré-niveau figé, graphe temporel, max-ID) | B | PHASE_A_* |
| R-08 | Obstacles GPU sous la seconde | B | OBSTACLES_GPU_SOUS_SECONDE_20260923.md |
| R-09 | Ticket possédé du cache ; ticket borné de nœuds témoins (désaccord non arbitré) | A et B | CACHE_TEMOINS_COUT_VALIDATION, ETAT_COURANT, CONTRE_AUDIT_RECTANGLES_Q34_TICKET_BORNE |
| R-10 | Tri unique par histogramme et dispersion (−31,08 M comparaisons) | A | AUDIT_DISTRIBUTION_SAMPLE_SORT_20260923.md |
| R-11 | Répétitions de BallKey, préfixe d'intrus | A et B | INTRUS_FULL_PREFIXE_EXACT, CONTRE_AUDIT_B_PREFIXE_INTRUS |
| R-12 | Collisions, sondes et RSS de l'index de clés ; RSS par phase et par K promis pour R5 | B | CONTRE_AUDIT_B_INDEX_CLES_FULL, CONTRE_AUDIT_B_RESIDENCE_CHAINE |
| R-13 | Erratum des plages R6 ; commentaire « Exact depth » de `q34_dead_lanes.cpp` | B | ETAT_COURANT.md |
| R-14 | Seuil K−2 des arêtes q4 seules | A | SEUIL_SATURATION_ATLAS_PAR_VOIE_20260923.md |
| R-15 | Tour FULL maigre D5 : **jointure des selles seule testée puis retirée**, 10,188 M entrées pour 1,012 M MEB évités, phase 0 2 582→2 752 ms sur 16k/K10 ; suite : saut au centre avec règle 0 ou index nettement moins cher, racines par facette, porte E1 corrigée, puis phase A maigre et sortie compacte ; étape 1 (saut au centre avec règle 0, porte par facette, ledger propre) en conception chez le développeur | C | [reçu négatif](../receipts/saddle_index_negative_20260923/README.md), CONTRELEC_D5_NAISSANCE_ET_PORTE_E1_20260923.md |
| R-16 | Sonde d'attribution du CPU q3/q4 par longueur d'ancre sur les trois trames, par lot | C | AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md § 5, étape 3 |
| R-17 | **S1 réalisé** sur G4 : six cas exacts sans sol K5/K10, filtre isolé 08/000000/K5 à 63,8 ms face au CPU W48 ; S2 et la chaîne mixte sont mesurés en R12, puis S3 en R13 avec six épingles absolues de catalogue. Restent l'égalité littérale des catalogues GPU/CPU et le contrat sous la seconde | C | [reçu S1](../receipts/g4_gpu_s1_20260923/README.md), [reçus R12](../receipts/g4_tower_r12_20260923/README.md) et [R13](../receipts/g4_tower_r13_20260923/README.md), AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md § 3.3 |
| R-18 | Décisions de contrat à acter avec l'utilisateur : sortie compacte, digest hors chrono, expansion chronométrée à part ; 100 ms à reformuler | C | AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md § 1 et 5 |
| R-19 | Porte `scale8000` : recenser par balayage brut **toutes** les boules à coquille étendue (86 à 8k), publier l'effectif échantillonné par famille, corriger le commentaire « pas deterministe » | C | canal, entrée de 10 h 12 UTC |
| R-20 | Angle mort conjoint Euler + tour (fusions seules à Kmax : q2 à p=Kmax−1, q3 à p=Kmax−2) : à K5, contrôle Kmax+1 avec tour et restriction clé par clé ; à K10, K11 hors domaine ou juge échantillonné (juges d'échantillon indépendants : campagne v5, q2 205 182 et q3 286 706 incidences toutes présentes, dont 55 297 clés q3 régulières p=Kmax−2 ; portes v6 `STATUS=0`, mutants tués avec marqueur causal ; portes v7 sur la strate CRL longue au rang critique `STATUS=0` (92/179/17 triangles, clé retirée détectée) ; portes v8, garde d'index avant échantillonnage et six mutants refusés, `STATUS=0`, 23 cas v7 identiques ; port en portes produit **proposé**, v2 après la contrelecture B (`judges_product_gates_v2.patch` : fixture cosphérique à comptes exacts, famille haute u18, coupes LiDAR 8k épinglées, contrôles globaux de clé en double et de boule étendue non appariée, 31 tests `gate` et 28 `scale8000`, 59/59 sur `350f82e66`) ; régression échantillonnée, pas une preuve FULL ; en attente d'adoption) | C | c_omission_20260923/README.md, AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md § 3.1 |
| R-21 | S2 (chemin q3/q4 par lots, CPU puis GPU) : trois couches indépendantes, (1) structure des survivants (fait, `2059189d`), (2) différentiel moteur/lots clé par clé (petits nuages : porte du développeur ; 8k et trame entière K5/K10 en référence CPU : identiques, `results/batch_diff_v1/`), (3) juges d'échantillon et Euler indépendants du filtre ; reste le différentiel avec `q34_gpu_filter` sur G4 (voir R-22) | C | canal, entrée S2 de C ; c_omission_20260923/README.md |
| R-22 | R13 : les 18 cas reproduisent les six épingles v18 de tour/catalogue selon leur trame et K ; porte `mhgp9_catalogue_digest` de coquilles étendues et deux mutants portée par `18d7c69c7` ; **tenu en R13** : sept cas avec certificats S3 et deux avec filtre S2 seul sur l'appareil ; condensé FNV-64 à un fil hors chrono (1,5–2,6 s à K10 en local), non égalité littérale | C | c_catalogue_digest_20260923/README.md, [reçu R13](../receipts/g4_tower_r13_20260923/README.md) |
| R-23 | Préflight S3 : feuille à plusieurs rangs, masques hors des voies de K, lot vide, barrière de frontière, juge par arête dans les deux préflights GPU — **fermé** `942494362` ; `rebuilt_covers` égal au nombre de masques décidés non nuls — **porté** `46c50432c` ; reste un mini-test device du lot vide à pointeurs nuls | B | canal, entrées de 19 h 44, 19 h 54 et 20 h 30 UTC ; CONTRE_AUDIT_B_S3_CERTIFICAT_WIP_20260923.md |
| R-24 | `GPU_executed` et `gpu_completed_cases` dérivés des champs device observés (survivants, warps, temps), non des leviers — **porté** `46c50432c` | B | canal, entrée de 19 h 54 UTC |
| R-25 | Plafond de représentation de l'appel S2/S3 (R, P, S ≤ 2³¹−1 ; environ 26R+9P+9S octets sur l'appareil) : tuiles R/P à ordinaux stables, survivants en flux, index résident entre S2 et S3 ; ouvert, distinct de la preuve de croissance | B | PROPOSITION_B_GPU_STREAMING_S2_20260923.md, canal, entrée de 20 h 01 UTC |
| R-26 | Shadow subdivisé avant le cœur : grille commune ou criblée essayée sur deux régimes S2, au plus 0,1345 % de F total fermable dans ces essais et aucun gain de chaîne ; restent domaines adaptatifs/2D, coût du cover et repli exact | B, A | canal, entrée de 20 h 30 UTC ; CERTIFICAT_B_REDONDANCE_CELLULE_UNIQUE_20260923.md ; s2_precore_node_shadow_20260923/README.md ; s2_precell_incidence_20260923/README.md |
| R-27 | Préflight local d'espace libre avant toute session G4 (seuil de quelques Go sur `/workspaces`) ; pendant R13, le disque était à 100 % (318 Mo libres) jusqu'au nettoyage de 12 Go d'arbres CMake v4–v8. La garde 2 Gio annoncée reste absente du chemin R14 publié, selon le [préflight B](CONTRE_AUDIT_B_PREFLIGHT_G4_R14_V19_20260923.md) et la [contrelecture du reçu](CONTRELECTURE_G4_R14_RECU_20260923.md). Les paires entrelacées R14 sont désormais reçues : gain S3 de 0,221/0,230 s K5 et 0,794/0,785 s K10 sur 08/000000, sans attribution fine noyau/copies. | C | canal, entrée de 20 h 25 UTC ; R14 publié |
| R-28 | S3 construit le cover complet GPU puis le reconstruit pour les arêtes encore ouvertes côté CPU (0,611–1,555 M rebuilds/cas R13), sans ajouter ce second parcours au ledger géométrique. **Diagnostic d'abord** : chronométrer uniquement `Q34EdgeCover::make`, ses visites/tests et la distribution des plages ouvertes W1/W48 ; ensuite comparer export compact par ordinal, repli CPU exact et chaîne ON/OFF. `edges_ms` entier n'est pas un gain revendicable ; même sa suppression fictive laisse K5 >1 s en R13 | B | CONTRE_AUDIT_B_G4_R13_S3_20260923.md, canal 20 h 57 UTC |
| R-29 | Positivité des supports réguliers (q3 strictement aigu, q4 centre strictement intérieur) et `arité ≤ 4` comme invariants de la chaîne (refus `chain_nonpositive_regular_support`) : aujourd'hui seule la passe 1 de la tour les vérifie, et le puits des voies GPU reprend la clé brute ; portes de refus absentes (triangle obtus, tétraèdre à centre extérieur) ; ensuite levier `tower_sealed_catalogue` (passe 1 sautée sous sceau typé, échantillon 1/64), gain projeté 15–26 ms à K5 et 60–82 ms à K10 sur G4 | C | PROPOSITION_C_CATALOGUE_SCELLE_20260924.md |
| R-30 | 100 ms infaisable avec les algorithmes connus sur une G4, dans les quatre cas (K5/K10, avec/sans sol), la sortie explicite étant dans le budget (décision de l'utilisateur) ; cibles étagées proposées : 1 s K5 sans sol (presque atteint), 1 s K5 avec sol (tour GPU requise), 250–500 ms K5 et ≈1 s K10 sans sol en fin de refonte ; 100 ms = horizon de recherche (réduction de travail ≥ 2,3× et tour fondamentalement moins chère) | C | AUDIT_C_GRAND_AUDIT_V9_20260924.md § 1 |
| R-31 | Protocole de mesure : sous-chronos des 80–176 ms hors chrono de q3/q4, CPU de fil par phase de la tour, pic mémoire appareil, séparation téléversement/téléchargement, table `diagnostics` typée hors clés exactes ; titre = configuration par défaut, médiane de ≥ 3 répétitions entrelacées ; contrat (R-18) gelé, tout déplacement de frontière publié comme amendement froid/chaud | C | AUDIT_C_GRAND_AUDIT_V9_20260924.md § 6 A |
| R-32 | Trames avec sol : épingles CPU avant R21 (en cours) ; arène de cover des voies et grosses arêtes (> 2^16 sites, > 2^12 événements) en report par arête ou appels fenêtrés, jamais un refus du cas entier ; publier tailles maximales, reports, traîne, arène, pic appareil, phases de la tour, Euler K10 | C | AUDIT_C_GRAND_AUDIT_V9_20260924.md § 4, 5, 6 C |
| R-33 | Leviers classés par gain (K10 d'abord) : groupement de la phase 0 sans tri de 56 o et radix des niveaux ; E6 ; glu q3/q4 parallèle et session résidente ; recensement dans la fenêtre de l'appareil ; E4/E5 ; phase A parallèle après E6 ; arrêter les micro-leviers K5 sans sol et la prolifération des leviers (préréglages) | C | AUDIT_C_GRAND_AUDIT_V9_20260924.md § 6 B, E |
| R-34 | Confiance : Kmax+2 à K5 sur trames entières et juges stratifiés entiers (angle mort 26,9 % / 11,0 %) ; épingle sans leviers ; GPU jugé sur coupe LiDAR, à K10 et sur chemins de capacité ; bornes des IDs dans `check_lanes_batch` ; TSan et compute-sanitizer ; bras T2 aux leviers G4 ; revue indépendante v22–v26 | C | AUDIT_C_GRAND_AUDIT_V9_20260924.md § 7, 8 |

## Conventions proposées (à adopter par tous)

1. Canal : ajout en fin de fichier seulement ; heure prise par `date -u` au
   moment du commit ; ligne `Base : <hash>` ; rôle entre parenthèses ;
   questions numérotées.
2. Chaque acteur commite ses propres fichiers depuis son propre worktree ;
   personne n'écrit dans le worktree d'un autre.
3. Retirer les notes périmées ou redondantes après transfert de leur seul
   constat encore utile vers l'état courant ou l'index ; consigner le retrait
   ici. L'historique Git conserve les versions anciennes.
4. Nouveaux livrables à plusieurs fichiers : un sous-dossier
   `<thème>_<AAAAMMJJ>/` avec son `README.md`.
5. En-tête d'une nouvelle note : auteur, date UTC, base, statut, réponse
   attendue.
