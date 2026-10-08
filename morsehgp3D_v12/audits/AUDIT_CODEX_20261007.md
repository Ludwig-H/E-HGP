# Audit Codex — état courant v12

8 octobre 2026. Dernière contrelecture : A `5f5c0c83f` contre `27eca166b`, MES-C2 `27eca166b`.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Derniers temps G4 : A adopté, contrat FULL 100 ms non tenu.**
[Campagne admise](../receipts/audit_reponses_20261008/session_t2da_admission/README.md), u21/W48,
ng00–02 : 39 885 / 35 551 / 45 845 sites. FULL GPU K5 ; cinq processus × neuf chaudes par bras/trame :

| Médiane des cinq médianes, ms | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| Avant A | 147,47 | 117,21 | 150,41 |
| Après A | **99,74** | **81,03** | **97,80** |

ng00 atteint **101,68 ms au maximum des médianes processus** ; aucune garantie 100 ms déduite des trois médianes.
61 processus/850 passes admises, 46 objets FUL1 comparés identiques. Catalogue C et pool communs aux deux bras.
[Correction de notre lecture du bras avant](../receipts/audit_reponses_20261008/session_a_provenance/README.md) :
archive réelle **27eca**, pas le 902 d'un ancien commentaire ; aucun gain C/pool indu ne doit être imputé à cette comparaison.

**37 trames, six séquences : FULL 230,79→160,57 ms de médiane des médianes par trame**,
maximum de ces médianes **440,29→319,74 ms** ; pire passe chaude **448,21→327,27 ms**.
Trois processus × une visite chaude/trame ; après A, 26/37 médianes et 79/111 passes dépassent 100 ms.
Les prises K10 d'identité ne constituent pas une nouvelle campagne chaude K10.
Derniers CPU FULL K5, [session K](../receipts/audit_reponses_20261008/session_k_full/README.md) :
**441,07 / 368,33 / 447,08 ms** ; GPU FULL K10 : **793,44 / 603,34 / 715,31 ms**.
Face à la v11 : GPU plus rapide, CPU plus lent sur l'historique non apparié ; pas de gain causal déduit.
FULL englobe Cloud/index→T/M/V/R ; lecture, masque, validation, FUL1 et libération hors mur.
A est mesuré avec `--recouvert` ; bascule de la voie par défaut encore à qualifier.

[Session G4 arrêtée](../receipts/audit_reponses_20261008/session_a_provenance/README.md), mais **`failed_remote`** :
719/719 portes rapides et CTest mutants passés ; LiDAR six portes passées, puis MES-M0 interrompu au plafond externe180s.
[Diagnostic du timeout](../receipts/audit_reponses_20261008/session_a_timeout/README.md) : chaîne séquentielle,
limite propre7200s, environ131s restants après les six portes ; pas de deadlock A établi, gate à rejouer.

**Priorité développeur : distinguer les fins observées et les durées.** [Diagnostic A](../receipts/audit_reponses_20261008/session_a_diagnostic/README.md) :
R5 est le dernier marqueur **43/45 fois sur ng02 et 94/111 sur les 37 trames** ; ordres1/2 surtout sur ng00/ng01.
Ces marqueurs ne prouvent pas seuls le chemin critique causal. G inclut du travail forêt concurrent ;
P+C+G+queue n'épuise pas le mur (résidu médian ng00/ng02 : 2,39/1,78 ms).
Tracer les attentes et descendants de R5 avant d'écarter le noyau K5 ou de tout attribuer aux ordres1/2.

**Petits nuages MES-C2 : trois critères non tenus, campagne complète.** [Admission indépendante](../receipts/audit_reponses_20261008/session_c2_admission/README.md),
source `27eca166b`, u21, 147 nuages ; médiane des médianes par nuage, deux visites chaudes :

| FULL, ms | CPU 4 fils | CPU 48 fils | GPU 4 fils | GPU 48 fils |
| --- | ---: | ---: | ---: | ---: |
| K5 | 45,92 | **28,95** | 12,73 | **10,15** |
| K10 | 152,57 | **53,01** | 48,83 | **24,58** |

132 réels seuls CPU/GPU K5 W48 : 28,02/9,68 ms. C1/C2 CPU OLS : 16,45 ms + 11,57 µs/site,
seuils 2 ms et 3,727 µs/site. Huit refus quasi-sphères 3k/10k aux deux K/voies, tous `wide_leaf`.
56 processus joués, 3 608 passes complètes/2 392 chaudes ; aucune expiration ni prise manquante.
[Sources et arrêt certifiés](../receipts/audit_reponses_20261008/session_c2_provenance/README.md).
Comparaison MES-C descriptive : 882 empreintes communes égales ; C et pool changés ensemble,
aucun gain isolé du pool. W1 absent du plan C2 ; ses temps antérieurs restent dans le reçu MES-C.

**Massifs : dernières prises FULL K5.** [L1r](../receipts/audit_reponses_20261008/session_l1r_admission/README.md),
une chaude : Boreas n10 sans sol, 1,51 M sites, **GPU 10,025 s / CPU 22,344 s** ;
Marseille sans sol 2,47 M : GPU 10,955 s ; Scion sans sol 3,44 M : 41,951 s ; Meadow 6,18 M : 35,325 s.
10 succès/8 refus mémoire, B1/B2/B4 non tenus, B3 non évalué. [L2 CPU froide](../receipts/audit_reponses_20261008/session_l2_admission/README.md) :
Paris sans sol 9,11 M **112,866 s**, brut 14,55 M **165,789 s** ; ETH3D 16,83 M `wide_leaf`, Lyon 24,02/32,41 M `memory_budget`.
Aucun chaud/GPU/K10/digest en L2. Aucun plafond universel en sites ni gain causal L1/L1r.

**Autres suivis.** A : [fenêtre réelle des feuilles](../receipts/audit_reponses_20261008/t2d_a_fenetre_patch/README.md)
toujours à corriger au pin mesuré5f. [Dépendances A](../receipts/audit_reponses_20261008/t2d_a_dependances/README.md) :
1 578 obligations/399 états, ownership favorable ; terminaison atomique non prouvée. [Comparaison A](../receipts/audit_reponses_20261008/t2d_a_comparaison/README.md) proposée sur le même binaire ;
[admission A](../receipts/audit_reponses_20261008/t2da_integration/README.md) : route/isolation/cohorte encore permissives, patch proposé. C : [mathématiques relues](../receipts/audit_reponses_20261008/t2d_c_integration_math/README.md) ;
[cohorte commune 5f5c](../receipts/audit_reponses_20261008/t2dc_cohorte_livree/README.md) close, résidus du lecteur inchangés.
[Pool 5b](../receipts/audit_reponses_20261008/pool_equipes/README.md) : synchronisation/modèle favorables ; campagne C2 admise, gain isolé non acquis.
[Extension de feuille](../receipts/audit_reponses_20261008/feuille_large_proposition/README.md) proposée : supports croisés, census K-certifié,
indices u32 ; shell64 et compteurs à préserver. Complétude modélisée, coût combinatoire non résolu.
Autres propositions CPU, S*, T/K1, R et limites dans le registre. Audit : sources/Python, aucun moteur ni GCP.
