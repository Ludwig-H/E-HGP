# Audit Codex — état courant v12

8 octobre 2026, 17:16 UTC. Dernier FULL admis : R1 `47feedc96`, livraison `150392f99`.
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
q=d+1 conserve lignes/ordre des enfants ; FUL1 ne sérialise pas RCSR. [Six mutants locaux](../receipts/audit_reponses_20261008/r1_mutants_execution/README.md)
tués par code et six portes G4 relues, preuves distinctes ; assertions/ELF locaux non archivés.

**Pas de nouveau FULL CPU pour ng00–02.** [Protocole prêt](../receipts/audit_reponses_20261008/full_cpu_actualisation_protocole/README.md),
non exécuté : sonde sans `--device`, K5/K10,39 processus/312 passes, pin à fixer. C domine83–84% du FULL CPU M ;
finition déjà parallèle, feuille16 sans gain global face à24. Comparaison v11 non appariée, CPU encore plus lent.
FULL exclut lecture, masque, validation, FUL1 et libération. Fenêtres recouvertes non additionnables ;
`cpu_ns` n’est pas une latence CPU seule, `--sequentiel` conserve le catalogue GPU.

**A6b livré `f2c106d93`, verdict indisponible.** [Reprise du 8 octobre](../receipts/audit_reponses_20261008/a6b_reprise_sans_resultats/README.md) :
arrêt certifié, machine déjà arrêtée à 17:03 ; aucun reçu final, code worker ni résultat local à 17:08.
Sources/protocole clos, **aucun nouveau chrono A6b**. Récupérer les primaires avant toute adoption/rejet.
[Clôture locale](../receipts/audit_reponses_20261008/a6b_cloture_locale/README.md) : 753 Passed/1 Skipped, deux mutants tués par code ;
src/tour raccordés à Git, cinq écarts hors produit natif. Pont release/acquire favorable sous préconditions ;
portes ciblées, aucune qualification générale C++. Priorité G = tranches **réclamées**, pas terminées.
[Borne N](../receipts/audit_reponses_20261008/a6_admission_n/README.md) non implantée : somme des W plus grands maxima par tâche.
Règle inchangée : IC haut agrégat grandes<0,95, chaque ng<1,01, A/A±1,5% ; aucun transfert des anciens résultats A6.

**[T1-d2 admis](../receipts/audit_reponses_20261008/session_t1d_admission/README.md)** : C GPU K5 **26,23/23,20/26,46 ms**,
IC95 hauts1,0034/1,0063/1,0082, seuil1% C seul. Port strict intégré150392, attendu36 corrigé8b9eab.
Budgets :12 succès/24 refus ; MHGP12DP omet niveaux/table, FUL1 joué sans ces budgets.
[Identité complémentaire proposée](../receipts/audit_reponses_20261008/t1d_identite_flux_proposition/README.md),46 injections,
avec [complément CTest46 requis](../receipts/audit_reponses_20261008/t1d_identite_flux_ctest/README.md).
[Mémoire proposée](../receipts/audit_reponses_20261008/t1d_metadonnees_proposition/README.md) : plan compté/coexistence réservée,
propriétaires en pseudocode. Publication niveaux/rassemblement absents du détail appareil, inclus dans C.

**Petits LiDAR** : [C3](../receipts/audit_reponses_20261008/session_c3_admission/README.md), 132 réels W48/cache8 Gio,
K5 CPU/GPU25,003/7,856 ms ; K10 46,420/14,571 ms. Huit refus `wide_leaf` difficiles ; W1 absent,
W48 pénalise≤150 sites. [Deux campagnes cache](../receipts/audit_reponses_20261008/session_c3_apparies/README.md)
refusées A/A : aucune absence d'effet conclue.

**Massifs — record GPU actualisé.** [L2t admis](../receipts/audit_reponses_20261008/session_l2t_admission/README.md) :
**Paris sans sol, 9 111 422 sites, FULL K5 en 46,453s à froid**, catalogue GPU/tour CPU W48, u21.
Quatre cas : un succès, trois refus mémoire sans stade ni budget nommé ; aucun FUL1 ni chrono chaud.
Source 8b9 identique à R1 ; archive/arrêt clos, ELF initial seul. Capacité GPU gardée après finition :109,3 Mo.
Paris brut 14,552M et Lyon 24,017/32,413M refusent ; SMI ne prouve pas une cause hôte.
Record [CPU L2](../receipts/audit_reponses_20261008/session_l2_admission/README.md) : **Paris brut 14,552M en 165,789s froid**.

[L1t admis](../receipts/audit_reponses_20261008/session_b1t_admission/README.md) : K5 GPU chaud Boreas 1,513M **7,185s**,
Marseille sans sol 2,465M **8,095s**, Meadow 6,181M **29,621s** ; une chaude par cas, aucune série statistique.
K10 GPU **froid** : Boreas 1,513M **38,620s**, Marseille 2,465M **38,521s**. Marseille brut 6,709M K5 **25,076s froid**.
Sous 8 Gio sur Boreas K5 : GPU 8,201s chaud, CPU 19,484s froid ; comparaison non appariée, FUL1 égaux.
L1t : 19 processus/23 FULL, 14 succès/5 refus ; FUL1 seulement jusqu’à1,6M, hash ELF final absent.

**CST-0243 ouvert** : TU Wien sans sol 5,200M réussit 32,856s à froid puis refuse la passe 1.
[Diagnostic](../receipts/audit_reponses_20261008/b1t_tuwien_memoire/README.md) : 79,99Gio CUDA gardés, 8,012Gio de marge au budget suivant ;
stade inconnu, instrumentation proposée sous protocole distinct. [Porte à qualifier](../receipts/audit_reponses_20261008/resident_reemploi_couverture/README.md) :
trois succès immédiats après finition complète/front rendu, budget figé ; seuil Pool non transférable à CUDA.
Les corrections documentaires L1t/L2t restent proposées : budgets, SMI et RSS distincts, aucun plafond universel en sites.

Sources, métadonnées et vérifications Python uniquement ; aucun moteur lancé par l'auditeur.
