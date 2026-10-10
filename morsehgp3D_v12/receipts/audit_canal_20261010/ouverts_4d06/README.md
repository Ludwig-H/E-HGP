# Conservation des suivis ouverts — 10 octobre 2026

Lignes intégrales du registre au pin `4d06b7584602fd9a8bcf96dcee6b100c757c0d55`.
Le [registre courant](../../../audits/CONSTATS.md) reste la seule autorité des états.
Seule la dixième cellule des huit lignes est résumée ; les neuf premières restent
octet pour octet identiques. Aucun constat ne change d’état. Les liens historiques
sont relocalisés sans changement de cible. Ce déplacement ne rejoue aucune preuve.

| Origine dans audits/CONSTATS.md | Destination intégrale |
| --- | --- |
| Ligne 26, CST-0018 | [CST-0018](#cst-0018) |
| Ligne 29, CST-0021 | [CST-0021](#cst-0021) |
| Ligne 30, CST-0022 | [CST-0022](#cst-0022) |
| Ligne 56, CST-0211 | [CST-0211](#cst-0211) |
| Ligne 76, CST-0233 | [CST-0233](#cst-0233) |
| Ligne 77, CST-0234 | [CST-0234](#cst-0234) |
| Ligne 80, CST-0237 | [CST-0237](#cst-0237) |
| Ligne 86, CST-0243 | [CST-0243](#cst-0243) |

[Contrôle de conservation](conservation.json) : source, extraction et résumés hachés ;
neuf colonnes et toutes les cibles des liens vérifiées avant publication.

## CST-0018

| Identifiant | Constat | Date | Rôle | Classe | Gravité | Pin | Témoin | État | Preuve de clôture |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `CST-0018` | juges de leviers : un banc refusé doit interdire toute adoption, comme une prise ou une preuve manquante (auditeur Codex, point 1) | 2026-10-07 | auditeur (Codex), inscrit par le développeur | mesure | majeure | `13c52bc60`, `AUDIT_CODEX_20261007.md` | — | en cours | [Historique conservé](../../audit_canal_20261010/README.md#cst-0018) : T1-d2/R1 admis, A6/A6b/B3 refusés ou rejetés ; résidus de preuve cache2/C3/GAPP/L1p ouverts. [A6c adopté relu](../../audit_reponses_20261010/session_a6c_admission/README.md) :85 processus, identités/règle exactes ; calibration non indépendante. [B3b clos relu](../../audit_reponses_20261010/session_b3b_admission/README.md) : clés et lot passent, scan rejeté ; [clés seules recommandées](../../audit_reponses_20261010/b3b_stats/README.md), gain1,44–2,85%, aucun bénéfice ajouté du scan établi. Juge v1 incomplet, [v2 proposé](../../audit_reponses_20261008/b3_identite_proposition/README.md) non embarqué ; codes G/stderr/fermeture ELF restent partiels. |

## CST-0021

| Identifiant | Constat | Date | Rôle | Classe | Gravité | Pin | Témoin | État | Preuve de clôture |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `CST-0021` | preuve et chrono : 3 285 vidages pour 3 303 tentatives ; distinguer les régimes dits chauds, les frontières du temps FULL et la provenance effective du binaire (auditeur Codex, point 4) | 2026-10-07 | auditeur (Codex), inscrit par le développeur | mesure | majeure | `13c52bc60`, `AUDIT_CODEX_20261007.md` | — | ouvert | M3/M4 : quatre journaux historiques absents, cinq binaires B non hachés ; aucune falsification démontrée. Refus de substitution/modification pendant l’appel vérifiés ; [D](../../audit_cd_corrections_20261007/campagnes/README.md) rattache cinq binaires : obstacle levé pour D, sans clôture globale. [K](../../audit_reponses_20261008/session_k_provenance/README.md) : sources, résultats et arrêt vérifiés, [342 fichiers code/tests/config](../../audit_reponses_20261008/session_k_sources/README.md) identiques à `c9ac60f20`, mais compilation interne sans hash binaire rapatrié ; temps admis, chaîne source→binaire incomplète. [Historique](../../audit_canal_20261008/suivis/README.md#cst-0021). |

## CST-0022

| Identifiant | Constat | Date | Rôle | Classe | Gravité | Pin | Témoin | État | Preuve de clôture |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `CST-0022` | identité par chaîne, couverture datée, hyperarêtes de Kruskal et polyèdre sont des contrats distincts (auditeur Codex, point 5) | 2026-10-07 | auditeur (Codex), inscrit par le développeur | document | mineure | `13c52bc60`, `AUDIT_CODEX_20261007.md` | — | ouvert | [Prélecture R](../../audit_registre_branches_20261007/README.md) : ant(b) distinct du seul arbre pi0 ; [admission CSR corrigée](../../audit_tmvr_admission_20261008/README.md). [Livraison u21 `7398aed7d`](../../audit_reponses_20261008/tmvr_livraison/README.md) : R et les branches ouvertes intégrés, neuf chaînes CPU conformes, 27 mutants acquis au code identique. Cela ne confond pas couverture datée, identité de chaîne et contrat du polyèdre. [R proposé](../../audit_reponses_20261008/registre_classe_unique/README.md) : copie des enfants seulement si cellule contributrice unique (`q=d+1`), preuve/modèle sans natif ; [patch proposé](../../audit_reponses_20261008/registre_classe_unique_patch/README.md), application textuelle vérifiée, branches CSR/mémoire/concurrence à qualifier sur G4. |

## CST-0211

| Identifiant | Constat | Date | Rôle | Classe | Gravité | Pin | Témoin | État | Preuve de clôture |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `CST-0211` | pré-vol mémoire qualifié de borne à partir de lois empiriques par site ; cette prévision ne garantit pas l'absence de refus en cours de calcul | 2026-10-07 | auditeur (Codex) | mémoire | majeure (avant port) | `c0bb99fd8`, `docs/ARCHITECTURE.md` § 4.6 | [domaines de bornes et compteurs nécessaires](../../audit_suivi_20261007/echelle/README.md) | en cours | [Historique](../../audit_canal_20261008/suivis_t1d/README.md#cst-0211). [T1-d](../../audit_reponses_20261008/t1d_produit/README.md) : métadonnées variables hors budget, contenus Buffer comptés ; planification non couverte par les seules bornes de tranche. [Proposition mémoire](../../audit_reponses_20261008/t1d_metadonnees_proposition/README.md) : Bin comptés/coexistence réservée, propriétaires en pseudocode ; aucun natif ni admission transactionnelle qualifiés. [L2t admis](../../audit_reponses_20261008/session_l2t_admission/README.md) : Paris sans sol9,111M calculé ; trois refus sans stade/budget nommé. SMI ne localise pas la ressource limitante, aucune borne universelle par site. |

## CST-0233

| Identifiant | Constat | Date | Rôle | Classe | Gravité | Pin | Témoin | État | Preuve de clôture |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `CST-0233` | finition catalogue CPU : rangs, préfixes CSR, aplatissement et table en série, contrairement au scan bloqué de la v11 ; assemblage+table 122,5–171,3 ms à K5 | 2026-10-07 | auditeur (Codex) | mesure | majeure (performance produit) | `58d384721`, `assemble.cpp`, `table.cpp` ; session F2 | [prises brutes](../../audit_performance_20261007/mesures/README.md), [scan proposé : preuve, 3 537 confrontations et quatre mutants](../../audit_performance_20261007/assemblage/README.md) | en cours | [Historique CPU](../../audit_canal_20261008/suivis_t1d/README.md#cst-0233) : finition parallèle depuis8ba. [FULL N](../../audit_reponses_20261008/session_fulln_admission/README.md), puis [FULL O A6c](../../audit_reponses_20261010/session_fullo_admission/README.md) : CPU354,73/298,50/355,14ms. [Décomposition](../../audit_reponses_20261010/fullo_temps/README.md) : C>100ms sur36/36 CPU. [Comparaison v11](../../audit_reponses_20261010/v11_v12_cpu_gpu/README.md) : CPU O +13,1–17,0 %, encore +12,5–16,2 % sans P ; descriptive. [Census CPU proposé](../../audit_reponses_20261008/cpu_census_reduction/README.md), sans natif/gain. [A6b rejeté](../../audit_reponses_20261008/session_a6b_admission/README.md), [A6c adopté](../../audit_reponses_20261010/session_a6c_admission/README.md) : grandesGM0,854248, gardesng respectées ;CST0244/0245 ouverts. [T1-d produit](../../audit_reponses_20261008/t1d_produit/README.md) et [admission T1-d2](../../audit_reponses_20261008/session_t1d_admission/README.md) conservées. |

## CST-0234

| Identifiant | Constat | Date | Rôle | Classe | Gravité | Pin | Témoin | État | Preuve de clôture |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `CST-0234` | feuille CPU J3 : chaque paire évaluée deux fois et census continué après rejet logique ; quinze compteurs identiques à la v11 ne décrivent pas ce travail physique | 2026-10-07 | auditeur (Codex) | mesure | majeure (travail évitable) | `58d384721`, `leaf_common.hpp`, `leaf_census.hpp` | [sept feuilles instrumentées ; variante d'arrêt CPU, émissions mot à mot et compteurs conservés](../../audit_performance_20261007/feuilles/README.md) ; aucun gain temps acquis | en cours | [Historique pré-M](../../audit_canal_20261008/suivis_fullm/README.md#cst-0234) : propositions, pool et campagnes petites tailles conservés. [Relecture CPU M](../../audit_reponses_20261008/cpu_feuilles_finition/README.md) : paires uniques, arrêt census, claim linéaire intégrés ; popcount sans gain natif isolé. Feuille16/24 déjà comparée dans I : moins de coût aux feuilles mais parcours accru, C légèrement plus lent à16. Aucune accélération présumée par le ratio de combinaisons ; plan diagnostic puis FULL au même binaire fourni. [C3](../../audit_reponses_20261008/session_c3_admission/README.md) :132 petits réels K5CPU/GPU W48 25,003/7,856 ms ; W1 absent, W48 ralentit la sous-cohorte≤150 sites. Temps C et OLS ne séparent pas calcul et coordination. |

## CST-0237

| Identifiant | Constat | Date | Rôle | Classe | Gravité | Pin | Témoin | État | Preuve de clôture |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `CST-0237` | nouvel objectif des petits nuages sans refus de largeur non couvert par T1 : max_leaf 256, coquille 64, m sur u8 ; q_min≤4 ne borne pas ces tailles | 2026-10-07 | auditeur (Codex) | document | majeure (capacité avant extension) | code `58d384721`, objectif `5c5fc7109` | [bornes effectives, liste candidate distincte de coquille, preuve et protocole d'extension](../../audit_performance_20261007/mathematiques/COMPLEMENT_SESSION_G.md) ; refus réels ETH3D 16,83 M en L2 et quasi-sphères 3k/10k CPU/GPU en [MES-C](../../audit_reponses_20261008/session_c_admission/README.md) | en cours | [Sens exact et carré transversal](../../audit_reponses_20261008/wide_leaf_semantique/README.md) : liste candidate ≥257 dans une boîte de centres, pas une coquille. [Extension proposée](../../audit_reponses_20261008/feuille_large_proposition/README.md) : tuples croisés u32/census K-certifié, shell64 ; modèle 315 catalogues, compteurs et coût massif ouverts ; huit refus quasi-sphères confirmés en MES-C2. Dév. 7 oct. : [réponse](../../developpement_20261007/reponse_audit_performance.md) [Raffinement des centres](../../audit_reponses_20261008/feuille_large_raffinement/README.md) : lemme local exact, égalités persistantes, sans borne globale ni gain. |

## CST-0243

| Identifiant | Constat | Date | Rôle | Classe | Gravité | Pin | Témoin | État | Preuve de clôture |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `CST-0243` | Session résidente : TU Wien sans sol réussit une première FULL K5 puis refuse la même trame en passe1 pour mémoire | 2026-10-08 | auditeur (Codex) | mémoire | majeure (réemploi résident) | `caf9585e4`, Session G4 L1t, 5 199 758 sites | [19 journaux relus, préfixe conservé](../../audit_reponses_20261008/session_b1t_admission/README.md) : passe0 32,855789464s puis libération, code2 memory_budget sans seconde FULL | ouvert | Budget hôte160Gio/appareil88Gio, cache8Gio ; arrêt certifié, ELF initial commun mais hash final absent. Refus observé, étage et budget responsables non publiés. La première passe ne valide pas la répétition chaude ; pas de cause hôte déduite du pic SMI. [Diagnostic source](../../audit_reponses_20261008/b1t_tuwien_memoire/README.md) : finition résidente prouvée,8,012Gio de marge ; cause ouverte. Instrumentation proposée, protocole distinct. [Porte de réemploi proposée](../../audit_reponses_20261008/resident_reemploi_couverture/README.md) : trois succès immédiats, budget figé, complet après front rendu ; seuil Pool non transférable à CUDA. [B1o ae8](../../audit_reponses_20261010/mesb1o_temps/README.md) reproduit le refus après24,818s ; même capacité appareil85 886 193 388B. Aucun correctif qualifié. |
