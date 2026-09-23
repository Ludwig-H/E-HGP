# Index des audits de la v9

Tenu par l'auditeur C depuis le 23 septembre 2026, avec l'accord de B et du
développeur (canal, entrées de 08 h 00 et 08 h 40 UTC environ ; A n'a pas encore
répondu). **Aucune note n'est déplacée ni réécrite** : cet index les classe.
La synthèse reste [`ETAT_COURANT.md`](ETAT_COURANT.md) ; le dialogue passe par
le canal [`audits/COORDINATION_MORSEHGP3D_V9.md`](../../audits/COORDINATION_MORSEHGP3D_V9.md).
État de l'index : `origin/main` au moment du commit qui le modifie.

## Par où commencer

1. [`ETAT_COURANT.md`](ETAT_COURANT.md) : verdict synthétique et mutable.
2. [`AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md`](AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md) :
   à quoi sert l'algorithme et comment la tour est reconstruite, pas à pas.
3. [`CONTRAT_COUTS_ET_PARALLELISATION.md`](CONTRAT_COUTS_ET_PARALLELISATION.md) et
   [`OBSTACLES_GPU_SOUS_SECONDE_20260923.md`](OBSTACLES_GPU_SOUS_SECONDE_20260923.md) : le contrat et ce qui en sépare.
4. [`NOTE_C_INVARIANT_EULER_20260923.md`](NOTE_C_INVARIANT_EULER_20260923.md) et
   [`CONTRELEC_EULER_PAR_NERF_20260923.md`](CONTRELEC_EULER_PAR_NERF_20260923.md) : juge global de complétude.
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
  refusée, différée (le préambule du canal exige une échéance), sans réponse,
  sans objet.

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
| [`CONTRELEC_JUGE_CLES_ABSENTES_20260923.md`](CONTRELEC_JUGE_CLES_ABSENTES_20260923.md) | A | 23/11:30 | lecture de la porte synthétique `683fa46e` ; mutations et débordement de coquille à préciser | vivant | sans réponse |
| [`COMPLETUDE_Q4_CLE_REMBOURREE_20260923.md`](COMPLETUDE_Q4_CLE_REMBOURREE_20260923.md) | B | `0a6efac3` 06:58 | testé borné | vivant | sans réponse |
| [`Q4_INDUCTION_ATLAS_EVENEMENTS_20260923.md`](Q4_INDUCTION_ATLAS_EVENEMENTS_20260923.md) | B ? | `28f0c284` 07:15 | démontré localement | vivant | sans réponse |
| [`Q34_PROPRIETAIRE_PASSAGE_AMONT_20260923.md`](Q34_PROPRIETAIRE_PASSAGE_AMONT_20260923.md) | B ? | `4291c538` 07:56 | démontré localement | vivant | sans réponse |
| [`CONTRE_AUDIT_B_PORTE_PUBLIQUE_T2_20260922.md`](CONTRE_AUDIT_B_PORTE_PUBLIQUE_T2_20260922.md) | B | `174fb8db` 22/23:18 | testé borné | historique | sans réponse |
| [`q4_global_12sites_20260923/`](q4_global_12sites_20260923/README.md) | B ? | `28f0c284` 07:15 | testé borné | vivant | sans objet |

## Arithmétique 18 bits

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`CONTRE_AUDIT_B_U18_ET_SONDE_20260922.md`](CONTRE_AUDIT_B_U18_ET_SONDE_20260922.md) | B | `0786d6c1` 22/22:59 | démontré localement | historique | acceptée |

## q3

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`Q3_STRUCTURE_ET_BORNES.md`](Q3_STRUCTURE_ET_BORNES.md) | A | `0674dc02` 22/21:44 | démontré localement | vivant | acceptée en partie |
| [`CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md`](CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md) | B | `2c6d806e` 22/22:21 | démontré localement | vivant | différée sans échéance |
| [`CONTRE_AUDIT_B_Q3_FEUILLE_WIP_20260923.md`](CONTRE_AUDIT_B_Q3_FEUILLE_WIP_20260923.md) | B | `bb3c696e` 00:42 | démontré localement | clos (code publié) | acceptée |

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
| [`CONTRE_AUDIT_B_D5_FULL_MAIGRE_20260923.md`](CONTRE_AUDIT_B_D5_FULL_MAIGRE_20260923.md) | B | `a514de68` 10:22 | mesure et méthode, projection D5 bornée | vivant | sans réponse |
| [`CONTRELEC_D5_NAISSANCE_ET_PORTE_E1_20260923.md`](CONTRELEC_D5_NAISSANCE_ET_PORTE_E1_20260923.md) | A | 23/10:25 | preuve et contre-fixture de porte E1 | vivant | sans réponse |
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
| [`RECTIFICATIF_R8_Q34_ORDONNANCEMENT_20260923.md`](RECTIFICATIF_R8_Q34_ORDONNANCEMENT_20260923.md) | A | 23/10:00 | démontré dans le code, mesure R8 | vivant | sans réponse |
| [`CONTRE_AUDIT_B_V14_ORDONNANCEMENT_20260923.md`](CONTRE_AUDIT_B_V14_ORDONNANCEMENT_20260923.md) | B | `a514de68` 10:22 | portes et protocole v14 | vivant | sans réponse |
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
| [`lidar_raw_physical_scaling_20260923/`](lidar_raw_physical_scaling_20260923/README.md) | A | 23/11:30–11:52 | mesure locale 21 cas, trame brute entière K5, sept secteurs physiques × trois densités ; reçu reproductible | vivant | sans objet |
| [`lidar_raw_k10_density_20260923/`](lidar_raw_k10_density_20260923/README.md) | A | 23/12:18–12:23 | mesure locale 3 cas, même trame brute et mêmes densités, tour K10 ; reçu reproductible | vivant | sans objet |
| [`lidar_raw_k10_sectors_20260923/`](lidar_raw_k10_sectors_20260923/README.md) | A | 23/12:49–13:09 | mesure locale K10, sept secteurs physiques × trois densités ; 18 nouveaux cas gardés, six répétitions | vivant | sans objet |

## GPU et parallélisme

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`OBSTACLES_GPU_SOUS_SECONDE_20260923.md`](OBSTACLES_GPU_SOUS_SECONDE_20260923.md) | B ? | `c717a1f2` 05:56 | mesure | vivant | sans réponse |
| [`CONTRE_AUDIT_B_WIP_V6_C6_TRI_20260923.md`](CONTRE_AUDIT_B_WIP_V6_C6_TRI_20260923.md) | B | `73d1d0a6` 05:31 | démontré localement | vivant (hors v9) | sans réponse (question canal l. 35) |
| [`AUDIT_A_GPU_S1_DOMAINE_U18_20260923.md`](AUDIT_A_GPU_S1_DOMAINE_U18_20260923.md) | A | 23/13:17 | défaut de garde démontré sur S1 publié ; correction ciblée | vivant | sans réponse |

## Annexes de l'auditeur C

| note | auteur | créée | portée | cycle | réponse du développeur |
| --- | --- | --- | --- | --- | --- |
| [`c_audit_20260923/`](c_audit_20260923/README.md) | C | `f4480c02` 08:44 | pièces de l'audit C | vivant | sans objet |
| [`c_euler_20260923/`](c_euler_20260923/README.md) | C | `cab281d8` 08:25 | testé, sondes | vivant | sans objet |
| [`c_alternatives_20260923/`](c_alternatives_20260923/README.md) | C | 23/10:12 | propositions, jurys, réfutations, expériences | vivant | sans objet |
| [`c_omission_20260923/`](c_omission_20260923/README.md) | C | 23/10:49 | shadow, échantillon 8k | vivant | sans objet |

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
| [`check_q34_dead_owner_aba_20260923.cpp`](check_q34_dead_owner_aba_20260923.cpp) | A | q3/q4 (porté en porte produit ; aucune note ne la cite) |
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

## Hors du dossier

- `audits/morsehgp3D_v9/` à la racine du dépôt (B) : `AUDIT_INITIAL_V8_20260922.md`,
  `ETAT_COURANT.md` (second « état courant », figé depuis `74aa8172`, à requalifier
  en photographie historique) et `public_chain_t2_gate.cpp`.
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
| R-15 | Tour FULL maigre D5 : **jointure des selles seule testée puis retirée**, 10,188 M entrées pour 1,012 M MEB évités, phase 0 2 582→2 752 ms sur 16k/K10 ; suite : saut au centre avec règle 0 ou index nettement moins cher, racines par facette, porte E1 corrigée, puis phase A maigre et sortie compacte | C | [reçu négatif](../receipts/saddle_index_negative_20260923/README.md), CONTRELEC_D5_NAISSANCE_ET_PORTE_E1_20260923.md |
| R-16 | Sonde d'attribution du CPU q3/q4 par longueur d'ancre sur les trois trames, par lot | C | AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md § 5, étape 3 |
| R-17 | Une seule session G4 GPU à seuils fixés d'avance, comparée à la base CPU réordonnancée | C | AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md § 3.3 et raccord R8 |
| R-18 | Décisions de contrat à acter avec l'utilisateur : sortie compacte, digest hors chrono, expansion chronométrée à part ; 100 ms à reformuler | C | AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md § 1 et 5 |
| R-19 | Porte `scale8000` : recenser par balayage brut **toutes** les boules à coquille étendue (86 à 8k), publier l'effectif échantillonné par famille, corriger le commentaire « pas deterministe » | C | canal, entrée de 10 h 12 UTC |
| R-20 | Angle mort conjoint Euler + tour (fusions seules à Kmax : q2 à p=Kmax−1, q3 à p=Kmax−2) : à K5, exécution de contrôle Kmax+1 avec tour et restriction clé par clé ; à K10, domaine d'audit K11 ou juge d'échantillon dédié (q2 : juge indépendant livré, 204 683/204 683 ; q3 à p=Kmax−2 : à concevoir) | C | c_omission_20260923/README.md, AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md § 3.1 |

## Conventions proposées (à adopter par tous)

1. Canal : ajout en fin de fichier seulement ; heure prise par `date -u` au
   moment du commit ; ligne `Base : <hash>` ; rôle entre parenthèses ;
   questions numérotées.
2. Chaque acteur commite ses propres fichiers depuis son propre worktree ;
   personne n'écrit dans le worktree d'un autre.
3. Ne plus supprimer une note : la marquer « historique, remplacée par … »
   en tête, et le reporter ici.
4. Nouveaux livrables à plusieurs fichiers : un sous-dossier
   `<thème>_<AAAAMMJJ>/` avec son `README.md`.
5. En-tête d'une nouvelle note : auteur, date UTC, base, statut, réponse
   attendue.
