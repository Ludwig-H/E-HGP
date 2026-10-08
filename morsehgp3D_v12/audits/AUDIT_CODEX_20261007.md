# Audit Codex — état courant v12

8 octobre 2026, 18:32 UTC. R1 `47feedc96` reste la référence admise ; A6b `f2c106d93` rejeté.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**B3 livré `545ed987e`, non adopté.** [Prélecture mathématique](../receipts/audit_reponses_20261008/b3_supports_math/README.md) :
clés et appartenance équivalentes sur catalogues produit réussis ; 957 requêtes Python, aucun natif.
Surcoût hôte **16 octets/boule**, copie Pool et transfert GPU ; gain à mesurer.
[Portes B3 relues](../receipts/audit_reponses_20261008/b3_portes_prelecture/README.md) : 746 passées / une sautée sur R1+B3,
preuves distinctes des prototypes A6b. Six mutants préparés, exécutions non admises ; deux portes négatives proposées.
**[CST-0018 reproduit sur le juge B3](../receipts/audit_reponses_20261008/b3_identite_admission/README.md)** :
les trois leviers restent « adoptés » après suppression des 35 journaux d'identité, en mode strict.
[Correctif proposé](../receipts/audit_reponses_20261008/b3_identite_proposition/README.md) : 27 cas et 21 contrôles indépendants,
schéma futur v2, seuils inchangés. À intégrer avec sa porte ; aucune mesure G4 B3 admise.

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

**Suite d'A6b : suivre K2/K3 et le mur entier.** [Diagnostic apparié](../receipts/audit_reponses_20261008/a6b_etages/README.md) :
sur les grandes, dernière fin publiée R(K5) avant dans 126/126 prises ; après, R(K2/K3) dans 123/126.
Les pertes ng00/01 touchent G mais aussi préparation/catalogue/ouverture ; causalité non isolée.
Priorité des aides = tranches G réclamées, pas achevées. Garde acquire sur `g_end_ns` proposée, avec limites d'épilogues.
[Borne scratch N](../receipts/audit_reponses_20261008/a6_admission_n/README.md) non implantée : somme des W plus grands maxima par tâche.

[Retrait A6b contrôlé](../receipts/audit_reponses_20261008/a6b_retrait_r1/README.md), puis rejoué contre **`8a0716e74` livré** :
163 fichiers exacts R1 dont tout `src/`, CST-0241 conservé ; 66 → 55 mutants, onze suppressions et une relocalisation.
[Portes locales après retrait](../receipts/audit_reponses_20261008/a6b_retrait_ctest/README.md) : 746 passées / une sentinelle sautée,
Release/u21 sans CUDA ; ELF déjà reconstruits pour B3, exclus de cette clôture de journaux.
[Corrections documentaires proposées](../receipts/audit_reponses_20261008/a6b_documentation/README.md) : agrégats, IC et priorité G.

**Nouvelles prises CPU/GPU R1 reçues, admission en cours.** FULL-N, source `8a0716e74` :
CPU ng00–02 K5, trois processus × cinq passes, cache 8 Gio ; pas de K10 CPU sur ces trames.
GPU K5/K10 et 37 trames K5 également prévus. Le tableau ci-dessus reste celui des dernières preuves admises.
FULL exclut lecture, masque, validation, FUL1 et libération. `cpu_ns` n'est pas une latence CPU seule ;
`--sequentiel` conserve le catalogue GPU.

[T1-d2](../receipts/audit_reponses_20261008/session_t1d_admission/README.md) et [petits LiDAR C3](../receipts/audit_reponses_20261008/session_c3_admission/README.md) :
mesures antérieures inchangées ; compléments de preuve et limites au registre.

**Massifs — record GPU : Paris sans sol, 9 111 422 sites, FULL K5 en 46,453 s à froid.**
[L2t admis](../receipts/audit_reponses_20261008/session_l2t_admission/README.md), catalogue GPU/tour CPU W48, u21 :
un succès, trois refus mémoire sans stade ni budget nommé ; aucun FUL1 ni chrono chaud.
Source 8b9 identique à R1 ; archive/arrêt clos, ELF initial seul. Paris brut 14,552M et Lyon 24,017/32,413M refusent.
Record [CPU L2](../receipts/audit_reponses_20261008/session_l2_admission/README.md) : Paris brut 14,552M en 165,789 s froid.

[L1t](../receipts/audit_reponses_20261008/session_b1t_admission/README.md) : mesures antérieures conservées,
FUL1 limité à 1,6M, ELF final absent ; aucune série statistique.

**CST-0243 ouvert** : TU Wien sans sol 5,200M réussit 32,856 s à froid puis refuse la passe1.
[Diagnostic](../receipts/audit_reponses_20261008/b1t_tuwien_memoire/README.md) : 79,99 Gio CUDA gardés, 8,012 Gio de marge ; stade inconnu.
Instrumentation proposée sous protocole distinct. [Porte à qualifier](../receipts/audit_reponses_20261008/resident_reemploi_couverture/README.md) :
trois succès immédiats, budget figé, finition complète/front rendu ; seuil Pool non transférable à CUDA.
Corrections documentaires L1t/L2t proposées : budgets, SMI et RSS distincts ; aucun plafond universel en sites.

Sources, métadonnées et vérifications Python uniquement ; aucun moteur lancé par l'auditeur.
