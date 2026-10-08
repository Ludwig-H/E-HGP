# Audit Codex — état courant v12

8 octobre 2026. Mesures K `c9ac60f20`, L1r `ea62cd691`, L2 `a2c2fccfd`, MES-C `83ed7620d`.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**SemanticKITTI : contrat de 100 ms non tenu.** [Session K admise](../receipts/audit_reponses_20261008/session_k_full/README.md),
u21/W48, ng00–02 : 39 885 / 35 551 / 45 845 sites.

| Médiane chaude FULL, ms | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| GPU K5, 5 processus × 9 chaudes | **159,30** | **127,63** | **163,27** |
| CPU K5, 3 × 4 | **441,07** | **368,33** | **447,08** |
| GPU K10, 3 × 4, informatif | 793,44 | 603,34 | 715,31 |

**37 trames, six séquences : médiane 241,31 ms, maximum contractuel 467,92 ms** (185 passes chaudes).
610 passes admises, empreintes concordantes. Mur Cloud/index→T/M/V/R ; lecture, masque, validation,
FUL1 et libération exclus. [Sources](../receipts/audit_reponses_20261008/session_k_sources/README.md),
[arrêt](../receipts/audit_reponses_20261008/session_k_provenance/README.md) vérifiés ; hash ELF/compilation, CPU·s et CPU K10 absents.
Face à la v11 : GPU plus rapide, CPU plus lent (historique non apparié).
[Recouvrement parfait à coûts inchangés](../receipts/audit_reponses_20261008/session_k_recouvrement_modele/README.md) :
31/37 maxima resteraient >100 ms ; scénario comptable, aucune borne sur A refondu.

**Petits nuages : dernière campagne MES-C admise, critères non tenus.**
[Médianes chaudes FULL](../receipts/audit_reponses_20261008/session_c_admission/README.md) des 147 médianes par nuage,
u21, deux visites chaudes/nuage, une Session/configuration :

| ms | 1 fil | 4 fils | 48 fils |
| --- | ---: | ---: | ---: |
| CPU K5 | 136,21 | 45,83 | **32,65** |
| GPU K5 | 21,68 | **13,07** | 13,44 |
| CPU K10 | 507,44 | 152,80 | 62,95 |

Sur les 132 réels seuls, CPU/GPU K5 W48 : **32,24/13,06 ms**. C1/C2 CPU : ordonnée OLS 20,34 ms,
pente 11,75 µs/site ; seuils 2 ms et 3,727 µs/site. Quasi-sphères 3 000/10 000 sites : quatre refus `wide_leaf`, C3 non tenu.
34/60 processus tentés, 4 009 passes complètes dont 2 666 chaudes. GPU K10/W1 interrompu au délai global restant ;
aucune statistique GPU K10 complète. Verdict refusé. [Provenance/arrêt certifiés](../receipts/audit_reponses_20261008/session_mes_c_provenance/README.md).
[Correction du juge 24dec](../receipts/audit_reponses_20261008/mes_c_correction_24dec/README.md) vérifiée : cohorte/verdict clos,
11 mutants détectés ; aucun transfert de pin à la campagne 83ed. [Comparabilité v11 limitée](../receipts/audit_reponses_20261008/mes_c_statistique/README.md).

**LiDAR massifs : derniers murs FULL K5, u21/W48.** L1r : une passe chaude ; L2 : une passe initiale froide.

| Scène | Sites | GPU, s | CPU, s | Prise |
| --- | ---: | ---: | ---: | --- |
| Boreas n10 sans sol | 1 513 483 | **10,025** | **22,344** | L1r chaude |
| Marseille sans sol | 2 465 285 | 10,955 | — | L1r chaude |
| Scion sans sol | 3 439 371 | 41,951 | — | L1r chaude |
| Meadow scan1 | 6 181 091 | 35,325 | — | L1r chaude |
| Paris sans sol | 9 111 422 | — | **112,866** | L2 froide |
| Paris brut | 14 551 520 | — | **165,789** | L2 froide |

[L1r](../receipts/audit_reponses_20261008/session_l1r_admission/README.md) : 10 succès/8 refus mémoire,
18/30 passes dont huit chaudes ; B1/B2/B4 non tenus, B3 non évalué. Boreas n10 CPU/GPU : FUL1 identique.
[L2](../receipts/audit_reponses_20261008/session_l2_admission/README.md) : deux succès CPU, ETH3D 16,83 M `wide_leaf`,
Lyon 24,02/32,41 M `memory_budget` ; aucun chaud, GPU, K10 ni digest. B1–B4 non évalués.
Succès à 6,71 M et refus à 5,20 M : aucun plafond universel en sites, aucun gain causal L1/L1r.

**Travail développeur et prochaine contrelecture.**
CPU/C : [symétrie et garde redondante](../receipts/audit_reponses_20261008/cpu_live/README.md) proposées ;
[réemploi S*](../receipts/audit_reponses_20261008/catalogue_radix_reuse/README.md),
[rangs C→T/K1](../receipts/audit_reponses_20261008/t_naissances_reutilisees/README.md),
[R classe unique](../receipts/audit_reponses_20261008/registre_classe_unique_patch/README.md) à qualifier.
A : [fenêtre des feuilles](../receipts/audit_reponses_20261008/t2d_a_fenetre_patch/README.md) à corriger.
B : [pilote corrigé](../receipts/audit_reponses_20261008/t2d_b_admission_reprise/README.md), quatre blocs FULL informatifs permissifs.
C `02b735d6b` : [relecture mathématique favorable](../receipts/audit_reponses_20261008/t2d_c_integration_math/README.md),
chaînes/niveaux/propriété conservés sur les chemins relus ; résultats G4 en cours d’admission.
[Juge C](../receipts/audit_reponses_20261008/t2dc_integration/README.md) : prises étrangères ignorées au verdict mais incluses au tableau ;
patch de cohorte commune proposé. Métadonnées de mutants en échec encore permissives.
[Diagnostic MES-C](../receipts/audit_reponses_20261008/session_c_diagnostic/README.md) : C porte l’écart CPU/GPU ;
41/41 réels de 100–300 sites ralentis à W48/W4 sur les deux voies, surtout G/TMVR quand C est sur GPU.
[Pool `5b3362bbd`](../receipts/audit_reponses_20261008/pool_equipes/README.md) : synchronisation favorable, modèle 22 766 états ;
gain G4 à mesurer. Session C sur `02b` sans ce pool ; future comparaison : même pool dans les deux bras (recette proposée).
[Grande feuille](../receipts/audit_reponses_20261008/wide_leaf_semantique/README.md) : liste candidate ≥257, pas une coquille ; repli global ouvert.
Autres états, preuves et limites dans le registre. Audit : Python et sources ; aucun moteur, GCP ni donnée sous licence.
