# Audit Codex — état courant v12

8 octobre 2026, 15:50 UTC. Dernier FULL admis : R1 `47feedc96`, livraison `150392f99`.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**R1 adopté ; le contrat FULL multi-séquences à 100 ms reste non tenu.**
[Relecture indépendante](../receipts/audit_reponses_20261008/session_r1_admission/README.md) :
112 journaux/1 441 FULL, dont 783 chaudes décisives et 108 K10 informatives, tous bras compris.
Archives, sources, ELF après K5/K10, statistiques et arrêt G4 clos ; aucun écart de rejeu.
u21/W48/cache8 Gio, catalogue GPU puis G/T/M/V/R CPU. Millisecondes, médianes des médianes par processus :

| FULL | ng00, 39 885 sites | ng01, 35 551 sites | ng02, 45 845 sites |
| --- | ---: | ---: | ---: |
| **GPU K5 R1** | **79,60** | **66,10** | **82,81** |
| GPU K5 référence appariée | 82,83 | 67,99 | 85,57 |
| GPU K10 R1, informatif | 495,82 | 367,23 | 423,83 |
| CPU K5 M, ancien sans cache | 381,80 | 323,10 | 385,94 |

K5 R1 : cinq processus × neuf chaudes/trame ; maxima bruts **87,81/80,82/83,95 ms**,
les 135 chaudes après restent sous 100 ms. K10 : trois processus × quatre chaudes/trame, identité stable.
Sur les **21 grandes trames** de 61 198 à 99 099 sites, médiane des 21 médianes :
**157,78→152,78 ms** ; pire médiane 287,93 ms, maximum brut 290,86 ms.
Toutes les 126 chaudes après dépassent100 ms. Gain agrégé 2,66%, IC95 haut 0,97696<0,99 ;
chaque ng respecte son garde-fou 2%, A/A conforme. Les 37 trames servent ici à l'identité,
pas à un nouveau tableau de temps des 37 trames. [Raccord R1](../receipts/audit_reponses_20261008/r1_raccord_math/README.md) :
q=d+1 conserve lignes/ordre des enfants ; FUL1 ne sérialise pas RCSR, les portes R1 restent nécessaires.

**Aucun FULL CPU pur récent.** `cpu_ns` est le travail CPU du processus hybride ;
`--sequentiel` conserve le catalogue GPU. [Protocole CPU prêt](../receipts/audit_reponses_20261008/full_cpu_actualisation_protocole/README.md) :
sonde sans `--device`, K5 prioritaire/K10 informatif, 39 processus/312 passes annoncés,
un seul W48 à la fois ; pin à fixer avant campagne. Plan déclaratif, non exécuté.
[Diagnostic CPU](../receipts/audit_reponses_20261008/cpu_feuilles_finition/README.md) : C représente 83–84% du FULL M ;
finition déjà parallèle, feuille 16 sans gain global face à 24. Comparaison v11 non appariée : GPU plus rapide,
CPU encore plus lent. FULL couvre Cloud/index→T/M/V/R ; lecture, masque, validation, FUL1 et libération exclus.
Fenêtres recouvertes : ne pas sommer leurs médianes. Budget actif, cache inactif et RSS distincts.

**A6b en prélecture, aucun gain encore admis.** [Produit capturé à 15:25](../receipts/audit_reponses_20261008/a6b_produit/README.md) :
19 fichiers non commis, pont leaf release/acquire favorable sous les préconditions écrivain unique/ancêtres
stricts ; fermeture SC protégeant les tampons. Lecture ciblée, aucune qualification native générale.
[Portes v2](../receipts/audit_reponses_20261008/a6b_portes_v2/README.md) : attente/aide tardive et 83 programmes de préfixe
relus favorablement ; appels directs, oracle séquentiel et sémaphores ajoutant HB. Quatre blocs locaux clos,
suite complète encore ouverte au relevé ; ces portes ne qualifient pas tous les entrelacements C++.
La priorité G attend la **réclamation** de toutes les tranches, pas leur achèvement : les aides peuvent
concurrencer les dernières tranches G en vol. Compteur zéro ≠ absence de concurrence.
[Borne N](../receipts/audit_reponses_20261008/a6_admission_n/README.md) toujours applicable : somme des W plus grands
maxima par tâche, cohortes entières ; doublon à retirer seulement dans `region_bytes`. Non implantée.
[Pilote A6b](../receipts/audit_reponses_20261008/a6b_pilote_prelecture/README.md) : fermeture héritée, huit autotests Python ;
référence R1, règle préalable IC haut grandes<0,95 et chaque ng<1,01, A/A±1,5%.
Aucun transfert des anciens résultats A6 ni garantie de gain pour chaque grande trame.
[A6 retiré](../receipts/audit_reponses_20261008/a6_retrait_qualification/README.md), CST-0242 clos par retrait ;
CST-0241 [clos séparément](../receipts/audit_reponses_20261008/a_terminaison_native/README.md).

**[T1-d2 admis](../receipts/audit_reponses_20261008/session_t1d_admission/README.md), coût C stable à1% près.**
C GPU K5 **26,23/23,20/26,46 ms** ; IC95 hauts1,0034/1,0063/1,0082. T1-d1 reste refusé pour format FULL ;
T1-d2 emploie FUL1 séquentiel. [Port strict c31](../receipts/audit_reponses_20261008/t1d_admission/README.md)
intégré tel quel 150392 ; attendu CTest 36 corrigé 8b9eab. Sous budgets : 12 succès/24 refus initiaux,
jusqu'à 3 tranches K5, 7 tranches et 7 lots K10. MHGP12DP omet niveaux et table ; FUL1 joué sans ces budgets.
[Complément d'identité prêt](../receipts/audit_reponses_20261008/t1d_identite_flux_proposition/README.md) :
CPU→libre→budgets, préfixes avant refus inclus, 46 injections/6 mutations Python ; hors mur.
Appliquer aussi le [complément CTest46](../receipts/audit_reponses_20261008/t1d_identite_flux_ctest/README.md) :
l’attendu resté36 ferait refuser les deux portes malgré le succès Python ; raccord corrigé dans ce complément.
Couvre niveaux exacts et requêtes positives S*, pas tous les octets internes ni les requêtes négatives.
[Correction mémoire proposée](../receipts/audit_reponses_20261008/t1d_metadonnees_proposition/README.md) :
plan compté, coexistence ancien/nouveau réservée ; descripteurs propriétaires encore en pseudocode.
Publication des niveaux et rassemblement manquent au détail appareil, mais restent dans le mur C.
Aucun nouveau FULL CPU ou massif dans T1-d ; seuil 1% sur C seul.

[B2 admis](../receipts/audit_reponses_20261008/session_b2_admission/README.md), chronos remplacés ici par R1.
[G-APP2](../receipts/audit_reponses_20261008/gapp2_admission/README.md) rejeté D1/D2, aucun gain FULL acquis.
[Fixtures u32](../receipts/audit_reponses_20261008/b2_norme_u32/README.md) proposées : étendues 30/32, norme i128,
Fraction conforme ; aucun défaut actuel trouvé, porte native u32 encore à exécuter.

**Petits LiDAR** : [C3](../receipts/audit_reponses_20261008/session_c3_admission/README.md), 132 réels W48/cache8 Gio,
K5 CPU/GPU25,003/7,856 ms ; K10 46,420/14,571 ms. Huit refus `wide_leaf` difficiles ; W1 absent,
W48 pénalise≤150 sites. [Deux campagnes cache](../receipts/audit_reponses_20261008/session_c3_apparies/README.md)
refusées A/A : aucune absence d'effet conclue.

**Massifs, derniers résultats clos** : [L1p](../receipts/audit_reponses_20261008/session_l1p_admission/README.md),
Boreas sans sol 1,513M : GPU 7,298s/CPU 20,056s ; Meadow 6,181M : GPU 30,565s, une chaude par cas.
Plus grands succès FULL K5 : **GPU Marseille brut 6,709M en 26,602s à froid** ;
[CPU L2](../receipts/audit_reponses_20261008/session_l2_admission/README.md), **Paris brut 14,552M en 165,789s à froid**.
L1p : 10 succès/8 refus mémoire ; FUL1 seulement jusqu'à 1,6M. Aucun plafond universel ni contrat 100 ms massif.

Sources, métadonnées et vérifications Python uniquement ; aucun moteur lancé par l'auditeur.
