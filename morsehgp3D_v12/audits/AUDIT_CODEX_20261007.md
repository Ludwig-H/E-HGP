# Audit Codex — état courant v12

8 octobre 2026. Code relu : bascule `86d7e39d`, B `41d4d828b`. Mesures : A `5f5c` contre `27eca`, B sur902, MES-C2 `27eca`.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Priorité : [CST-0241, terminaison de A](../receipts/audit_reponses_20261008/a_terminaison/README.md).**
Deux fils peuvent se réannoncer indéfiniment après tout travail fini : cycle SC équitable de 16 transitions.
Correctif minimal proposé (`last = fetch_sub(...) == 1`) ; six modèles bornés sans cycle après correction.
Aucun incident natif observé ni lien avec MES-M0 ; intégration et porte native à faire.

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
A était mesuré avec `--recouvert` ; cette route est devenue le défaut en `86d7e39d`.
Les prises CPU, petits nuages, K10 et massifs ci-dessous précèdent cette bascule.

**[Session B close et arrêt certifié](../receipts/audit_reponses_20261008/session_b_provenance/README.md)** :
724 portes socle, **7/7 LiDAR dont MES-M0**, trois CTests mutants passés au code41d4, aucun skip.
Cela ajoute la preuve LiDAR manquante ; l'[ancienne session A](../receipts/audit_reponses_20261008/session_a_timeout/README.md)
reste interrompue au plafond externe180s, sans deadlock A établi.

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

**Massifs, prises antérieures à A :** [L1r](../receipts/audit_reponses_20261008/session_l1r_admission/README.md),
Boreas sans sol 1,51M : GPU **10,025s** / CPU **22,344s**, une chaude. Dix succès/huit refus mémoire ; critères non tenus/non évalués.
[L2 CPU froide](../receipts/audit_reponses_20261008/session_l2_admission/README.md) : Paris9,11M **112,866s**,14,55M **165,789s** ;
ETH3D `wide_leaf`, Lyon `memory_budget`. Sans chaud/GPU/K10/digest L2 ; aucun plafond universel en sites.

**Corrections et suite immédiate.** [Bascule A contre-vérifiée](../receipts/audit_reponses_20261008/t2d_a_bascule_cloture/README.md) :
fenêtre réelle des feuilles et cohorte du juge intégrées exactement ; nominal accepté, trois contre-exemples refusés.
Patch fourni pour demander `--sequentiel` sur le bras après du pilote A, nécessaire depuis le nouveau défaut.
[Lecteur recouvert](../receipts/audit_reponses_20261008/lf_recouvert_gardes/README.md) : neuf corruptions d'horloges encore admises en86d ;
correctif proposé, 61 journaux/850 passes A préservés, quatre mutants causaux. Pas d'ordre imposé entre V et R.

**[B admis sur902](../receipts/audit_reponses_20261008/session_b_admission/README.md)** : 272 journaux/2 616 passes, 2 344 chaudes.
G CPU K5 W48 : **53,092→48,576 / 42,023→38,885 / 48,222→45,007 ms** ; gains géométriques6,8–8,4 %.
Lot, report, témoins et census combiné adoptés ; garde et proposition seules rejetées selon la règle préannoncée.
FULL GPU **séquentiel informatif** : 161,899→157,181 /129,755→126,940 /165,756→164,279 ms.
Ce sont902 et902+B, sans A/C/pool récent ; **aucun transfert au FULL recouvert actuel** ni au lot après retrait de L4.
K5 garde ses compteurs ; K10 change deux routes d'une unité avec objets identiques. B ne traite pas le catalogue CPU dominant.
[Raccord B/A](../receipts/audit_reponses_20261008/t2d_b_raccord_a/README.md) favorable sur emprunts et espaces privés, sans temps déduit.
[Extension de feuille](../receipts/audit_reponses_20261008/feuille_large_proposition/README.md) proposée ; compteurs et coût combinatoire ouverts.
Dépendances A, pool, C et autres propositions CPU/S*/T/K1/R : registre. Audit sources/Python, aucun moteur ni GCP lancé.
