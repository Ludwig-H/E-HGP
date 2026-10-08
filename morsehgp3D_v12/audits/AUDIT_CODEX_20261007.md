# Audit Codex — état courant v12

8 octobre 2026. Dernière mesure C `02b735d6b` contre `902041f66` ; A livré `bcd742c05`, encore sans G4.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Derniers temps G4 : catalogue C adopté, contrat FULL 100 ms non tenu.**
[Campagne contre-validée](../receipts/audit_reponses_20261008/session_t2dc_admission/README.md), u21/W48,
ng00–02 : 39 885 / 35 551 / 45 845 sites. Médianes chaudes, ms :

| Mesure | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| Catalogue K5 avant → après | 34,29→26,90 | 29,86→23,76 | 36,52→26,85 |
| FULL K5 après, informatif | **153,86** | **122,66** | **155,64** |
| Catalogue K10 après, informatif | 102,74 | 84,04 | 98,21 |

C K5 : 10 processus × 9 chaudes par bras/trame ; FULL : 3 × 9. Lot et quatre leviers adoptés,
A/A valide, mutant détecté par empreinte sous code 0. 393 journaux, 2 888 passes catalogue et 216 FULL admises.
[Sources et arrêt certifiés](../receipts/audit_reponses_20261008/session_t2dc_provenance/README.md).
Même ancien pool des deux côtés. [Pic mémoire commun hôte+GPU](../receipts/audit_reponses_20261008/session_t2dc_documentation/README.md), pas RSS/hôte isolé.

**37 trames, six séquences : dernier FULL = 241,31 ms de médiane, maximum contractuel 467,92 ms**,
[session K](../receipts/audit_reponses_20261008/session_k_full/README.md), 185 chaudes. La nouvelle campagne mesure
seulement leur **catalogue** : médiane 40,75 ms, maximum des médianes 71,74 ms. Aucun nouveau FULL K10.
Derniers CPU FULL K5, session K : **441,07 / 368,33 / 447,08 ms** ; GPU FULL K10 : **793,44 / 603,34 / 715,31 ms**.
Face à la v11 : GPU plus rapide, CPU plus lent sur l'historique non apparié ; pas de gain causal déduit.
FULL englobe Cloud/index→T/M/V/R ; lecture, masque, validation, FUL1 et libération hors mur.

[Scénario de recouvrement après C](../receipts/audit_reponses_20261008/session_t2dc_recouvrement_modele/README.md) :
96,05 / 78,67 / 103,48 ms, **à coûts inchangés** ; maximum des médianes processus 104,76 ms sur ng02.
A modifie travail, allocations et concurrence : ce scénario n'est ni une borne ni une prédiction de ses temps.

**Petits nuages MES-C : critères non tenus ; MES-C2 en contrelecture.** [147 nuages admis](../receipts/audit_reponses_20261008/session_c_admission/README.md),
u21 ; médiane des médianes par nuage, deux visites chaudes, une Session/configuration :

| FULL, ms | 1 fil | 4 fils | 48 fils |
| --- | ---: | ---: | ---: |
| CPU K5 | 136,21 | 45,83 | **32,65** |
| GPU K5 | 21,68 | **13,07** | 13,44 |
| CPU K10 | 507,44 | 152,80 | 62,95 |

132 réels seuls CPU/GPU K5 W48 : 32,24/13,06 ms. C1/C2 CPU OLS : 20,34 ms + 11,75 µs/site,
seuils 2 ms et 3,727 µs/site. Quasi-sphères 3k/10k : quatre refus `wide_leaf`, C3 non tenu.
4 009 passes complètes/2 666 chaudes ; GPU K10 interrompu au délai global, aucune statistique complète.
[Arrêt certifié](../receipts/audit_reponses_20261008/session_mes_c_provenance/README.md),
[juge corrigé en 24dec](../receipts/audit_reponses_20261008/mes_c_correction_24dec/README.md), campagne toujours sur 83ed.
[Diagnostic apparié](../receipts/audit_reponses_20261008/session_c_diagnostic/README.md) : C porte l'écart CPU/GPU ;
les 41 réels de 100–300 sites ralentissent tous à W48/W4, aussi dans G/TMVR.

**Massifs : dernières prises FULL K5.** [L1r](../receipts/audit_reponses_20261008/session_l1r_admission/README.md),
une chaude : Boreas n10 sans sol, 1,51 M sites, **GPU 10,025 s / CPU 22,344 s** ;
Marseille sans sol 2,47 M : GPU 10,955 s ; Scion sans sol 3,44 M : 41,951 s ; Meadow 6,18 M : 35,325 s.
10 succès/8 refus mémoire, B1/B2/B4 non tenus, B3 non évalué. [L2 CPU froide](../receipts/audit_reponses_20261008/session_l2_admission/README.md) :
Paris sans sol 9,11 M **112,866 s**, brut 14,55 M **165,789 s** ; ETH3D 16,83 M `wide_leaf`, Lyon 24,02/32,41 M `memory_budget`.
Aucun chaud/GPU/K10/digest en L2. Aucun plafond universel en sites ni gain causal L1/L1r.

**Aide développeur.** A : [fenêtre réelle des feuilles](../receipts/audit_reponses_20261008/t2d_a_fenetre_patch/README.md)
toujours à corriger en bcd. [Comparaison A](../receipts/audit_reponses_20261008/t2d_a_comparaison/README.md) proposée sur le même binaire ;
[admission A](../receipts/audit_reponses_20261008/t2da_integration/README.md) : route/isolation/cohorte encore permissives, patch proposé. C : [mathématiques relues](../receipts/audit_reponses_20261008/t2d_c_integration_math/README.md) ;
[cohorte commune 5f5c](../receipts/audit_reponses_20261008/t2dc_cohorte_livree/README.md) close, résidus du lecteur inchangés.
[Pool 5b](../receipts/audit_reponses_20261008/pool_equipes/README.md) : synchronisation/modèle favorables ; campagne C2 en contrelecture, gain isolé non acquis.
[Extension de feuille](../receipts/audit_reponses_20261008/feuille_large_proposition/README.md) proposée : supports croisés, census K-certifié,
indices u32 ; shell64 et compteurs à préserver. Complétude modélisée, coût combinatoire non résolu.
Autres propositions CPU, S*, T/K1, R et limites dans le registre. Audit : sources/Python, aucun moteur ni GCP.
