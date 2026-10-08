# Audit Codex — état courant v12

8 octobre 2026, 17:59 UTC. R1 `47feedc96` reste la référence admise ; A6b `f2c106d93` rejeté.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Le contrat FULL multi-séquences à 100 ms reste non tenu.**
[Deux campagnes A6b relues](../receipts/audit_reponses_20261008/session_a6b_admission/README.md) :
85 processus / 1 306 FULL chacune, 783 chaudes décisives tous bras compris ; sources,
archives, ELF, bootstrap indépendant exact et arrêts clos. La récupération n'est pas une mesure supplémentaire.
Première campagne refusée : A/A ng01 = 0,95694. Répétition rejetée : gain grandes **15,09 %**,
mais IC hauts ng00 **1,011640** et ng01 **1,019481**, au-delà de 1,01. Aucun retrait de prise.

Dernières observations de cette répétition : u21/W48/cache 8 Gio, catalogue GPU puis tour CPU.
Millisecondes ; médianes des cinq médianes de processus, neuf chaudes par processus :

| FULL | ng00, 39 885 sites | ng01, 35 551 sites | ng02, 45 845 sites |
| --- | ---: | ---: | ---: |
| **GPU K5 R1, témoin actualisé** | **80,07** | **65,97** | **83,12** |
| GPU K5 A6b rejeté | 80,69 | 66,67 | 79,58 |
| GPU K10 R1, campagne antérieure | 495,82 | 367,23 | 423,83 |
| CPU K5 M, ancien sans cache | 381,80 | 323,10 | 385,94 |

Sur les **21 grandes trames**, médiane des médianes **152,75 → 132,30 ms** ; pire médiane
289,25 → 241,64 ms, maximum brut 291,92 → 242,39 ms. Les 126 chaudes A6b dépassent 100 ms.
Ce quotient de médianes ne donne pas le gain géométrique apparié de 15,09 %.
37 trames couvertes en identité ; K10 à froid seulement dans A6b. Le K10 chaud ci-dessus reste
celui de [R1 adopté](../receipts/audit_reponses_20261008/session_r1_admission/README.md), trois processus × quatre chaudes/trame.
FUL1 ne sérialise pas RCSR ; [six mutants R1](../receipts/audit_reponses_20261008/r1_mutants_execution/README.md) qualifiés séparément.

**Aide au développeur : suivre K2/K3 et le mur entier.**
[Diagnostic apparié](../receipts/audit_reponses_20261008/a6b_etages/README.md), 261 paires : grandes,
ΔFULL moyen −25,688 ms = notamment ΔG régional +4,393 et Δqueue −30,658 ms.
Dernière fin publiée : R(K5) avant dans 126/126 prises ; après, R(K2/K3) dans 123/126.
Les pertes ng00/01 touchent aussi préparation/catalogue/ouverture ; N/H et aides ne sont pas chronométrés séparément.
Fenêtres de tâches ≠ temps CPU ; cause non isolée.
Priorité des aides A6b = toutes les tranches G **réclamées**, pas achevées ; le compte rendu doit le préciser.
Une garde acquire sur `g_end_ns` est une expérience proposée, avec limites d'épilogues explicites ; gain inconnu.
[Borne scratch N](../receipts/audit_reponses_20261008/a6_admission_n/README.md) non implantée : somme des W plus grands maxima par tâche.

[Retrait A6b contrôlé](../receipts/audit_reponses_20261008/a6b_retrait_r1/README.md), puis rejoué contre **`8a0716e74` livré** :
163 fichiers exacts R1 dont tout `src/`, CST-0241 conservé ; 66 → 55 mutants, onze suppressions et une relocalisation.
Aucune nouvelle qualification native du retrait par cette comparaison.
[Corrections documentaires proposées](../receipts/audit_reponses_20261008/a6b_documentation/README.md) : agrégats, IC et priorité G.

**Pas de nouveau FULL CPU sur ng00–02.** [Protocole](../receipts/audit_reponses_20261008/full_cpu_actualisation_protocole/README.md)
non exécuté : sans `--device`, K5/K10, 39 processus / 312 passes, pin à fixer. C domine 83–84 % du FULL CPU M ;
finition déjà parallèle, feuille16 sans gain global face à24. Comparaison v11 non appariée.
FULL exclut lecture, masque, validation, FUL1 et libération. `cpu_ns` n'est pas une latence CPU seule ;
`--sequentiel` conserve le catalogue GPU.

**[T1-d2 admis](../receipts/audit_reponses_20261008/session_t1d_admission/README.md)** : C GPU K5 26,23 / 23,20 / 26,46 ms,
IC95 hauts 1,0034 / 1,0063 / 1,0082 ; seuil 1 % sur C seul. Port strict intégré150392, attendu36 corrigé8b9eab.
12 succès / 24 refus ; MHGP12DP omet niveaux/table, FUL1 sans ces budgets.
[Identité complémentaire](../receipts/audit_reponses_20261008/t1d_identite_flux_proposition/README.md) proposée : 46 injections,
avec [CTest46 requis](../receipts/audit_reponses_20261008/t1d_identite_flux_ctest/README.md).
[Mémoire proposée](../receipts/audit_reponses_20261008/t1d_metadonnees_proposition/README.md) : plan compté/coexistence réservée,
propriétaires en pseudocode ; niveaux/rassemblement inclus dans C, omis du détail appareil.

**Petits LiDAR** : [C3](../receipts/audit_reponses_20261008/session_c3_admission/README.md), 132 réels W48/cache8 Gio,
K5 CPU/GPU 25,003/7,856 ms ; K10 46,420/14,571 ms. Huit refus `wide_leaf` ; W1 absent, W48 pénalise ≤150 sites.
[Deux campagnes cache](../receipts/audit_reponses_20261008/session_c3_apparies/README.md) refusées A/A : aucune absence d'effet conclue.

**Massifs — record GPU : Paris sans sol, 9 111 422 sites, FULL K5 en 46,453 s à froid.**
[L2t admis](../receipts/audit_reponses_20261008/session_l2t_admission/README.md), catalogue GPU/tour CPU W48, u21 :
un succès, trois refus mémoire sans stade ni budget nommé ; aucun FUL1 ni chrono chaud.
Source 8b9 identique à R1 ; archive/arrêt clos, ELF initial seul. Paris brut 14,552M et Lyon 24,017/32,413M refusent.
Record [CPU L2](../receipts/audit_reponses_20261008/session_l2_admission/README.md) : Paris brut 14,552M en 165,789 s froid.

[L1t admis](../receipts/audit_reponses_20261008/session_b1t_admission/README.md) : K5 GPU chaud Boreas 1,513M 7,185 s,
Marseille sans sol 2,465M 8,095 s, Meadow 6,181M 29,621 s ; une chaude par cas.
K10 GPU froid : Boreas 38,620 s, Marseille 38,521 s. 14 succès / 5 refus ; FUL1 jusqu'à1,6M, ELF final absent.

**CST-0243 ouvert** : TU Wien sans sol 5,200M réussit 32,856 s à froid puis refuse la passe1.
[Diagnostic](../receipts/audit_reponses_20261008/b1t_tuwien_memoire/README.md) : 79,99 Gio CUDA gardés, 8,012 Gio de marge ; stade inconnu.
Instrumentation proposée sous protocole distinct. [Porte à qualifier](../receipts/audit_reponses_20261008/resident_reemploi_couverture/README.md) :
trois succès immédiats, budget figé, finition complète/front rendu ; seuil Pool non transférable à CUDA.
Corrections documentaires L1t/L2t proposées : budgets, SMI et RSS distincts ; aucun plafond universel en sites.

Sources, métadonnées et vérifications Python uniquement ; aucun moteur lancé par l'auditeur.
