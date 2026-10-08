# Audit Codex — état courant v12

8 octobre 2026. Mesures M : `957e9784f` ; petits C3 : `72f622a55` ; lecteurs `85db49890`, porte native `bdfca8fb1`.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**LiDAR sans sol : les trois ng passent, le contrat multi-séquences 100 ms reste non tenu.**
[FULL M](../receipts/audit_reponses_20261008/session_m_admission/README.md), u21/W48 à chaud,
ng00/01/02 : 39 885 / 35 551 / 45 845 sites. Médianes des médianes par processus, en ms :

| FULL | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| GPU K5, cache désactivé | **94,66** | **78,26** | **94,76** |
| GPU K5, cache 8 Gio, bras apparié | **87,46** | **71,97** | **88,39** |
| CPU K5, cache désactivé | **381,80** | **323,10** | **385,94** |
| GPU K10, cache désactivé | **593,26** | **442,82** | **504,79** |

Sans cache, maximum des médianes processus K5 **95,99 ms** : sous-contrat ng tenu selon la règle fixée.
Trois des135 prises chaudes dépassent néanmoins100 ms (maximum brut101,61).
**37 trames, six séquences : médiane160,64 ms, pire médiane par trame319,78 ms** ;
maximum contractuel **358,86 ms**, ici aussi maximum des185 chaudes. 25/37 médianes par trame dépassent100 ms.
Voie GPU : catalogue CUDA puis tour CPU. FULL couvre Cloud/index→T/M/V/R ; lecture, masque, validation, FUL1
et libération hors mur. G est une fenêtre recouverte, TMVR la queue après G ; ne pas sommer les médianes.

**[Cache apparié M](../receipts/audit_reponses_20261008/session_m_apparie/README.md)** : A/A conforme,
rapports géométriques0,929/0,924/0,918, IC95 supérieurs<1 ; séquentiel rejeté1,515/1,465/1,529.
Sur37 trames, cache : médiane152,62 ms, pire médiane305,89 ms (deux secondes visites/trame, bloc informatif).
RSS maximal environ3,81 Gio, contre2,56–2,63 sans cache ; le budget actif n'inclut pas les blocs inactifs.
Le [défaut72](../receipts/audit_reponses_20261008/cache_defaut_raccord/README.md) correspond au bras mesuré,
mais **CPU/K10 avec cache sur ng00–02 restent non mesurés**. Future ablation : ref/A-A explicitement cache0.
Le pilote joué n'archivait pas l'ELF final : le juge85 refuse cette fermeture, sans invalidation inventée des temps.
M précède le correctif0241 ; ses [sources et arrêt](../receipts/audit_reponses_20261008/session_m_provenance/README.md)
sont contre-vérifiés (304 journaux FULL/apparié/D6,3 056 passes ; aucun moteur exécuté par l'audit).

**Petits LiDAR : [MES-C3 admise](../receipts/audit_reponses_20261008/session_c3_admission/README.md), critères C1–C3 non tenus.**
132 réels seuls, cache8 Gio, W48 ; médiane des deux chaudes par nuage, puis médiane des132, en ms :

| FULL | CPU | GPU |
| --- | ---: | ---: |
| K5 | **25,003** | **7,856** |
| K10 | **46,420** | **14,571** |

Maxima des médianes réelles : K5 CPU/GPU134,07/32,64 ms ; K10 515,63/199,35 ms.
Cohorte complète :147 réguliers,12 difficiles,56 processus/3 608 passes ; huit refus `wide_leaf` persistants.
CPU OLS14,90 ms +10,06 µs/site, paramètres statistiques et non coûts physiques. W1 absent ; sur≤150 sites,
CPU W4/W48 vaut6,08/10,88 ms : le chronomètre C ne suffit pas à séparer calcul et coordination du Pool.
[Deux appariés cache C3](../receipts/audit_reponses_20261008/session_c3_apparies/README.md) :198 journaux/1 836 passes,
les deux campagnes **refusées par A/A** surp5000 (GPU0,973001 ; CPU1,015474>1,015).
Aucune adoption ni rejet statistique du bras sans cache. [Sources72/arrêt clos](../receipts/audit_reponses_20261008/session_c3_provenance/README.md),
ELF final absent ici aussi. Plusieurs changements depuis C2 ; comparaison descriptive, aucun gain causal isolé.

**Priorités de calcul.** [CPU](../receipts/audit_reponses_20261008/cpu_feuilles_finition/README.md) : C318,77/272,70/320,92 ms
sur ng00–02, soit83–84 % FULL. Finition déjà parallèle ; même gratuite, FULL conditionnel268–321 ms.
Paires uniques et arrêt census intégrés ; feuille16/24 déjà comparée dans I sans gain global à16. Ventiler les
sous-postes et mesurer le coût total. V11 historique : GPU plus rapide, CPU encore plus lent, comparaison non appariée.
[Noyau](../receipts/audit_reponses_20261008/a6_noyau_raccord/README.md) : R5 dernier sur99 099 sites ; les fins ne
sont pas des durées propres. À P/C/G inchangés, queue gratuite laisse238,32 ms et22/37 médianes>100 ms.
Notre variante MSF/rejeu conserve l'objet dans le modèle, mais le développeur l'écarte du prochain A6 pour garder
le recouvrement. [Brouillon A6 N/I](../receipts/audit_reponses_20261008/a6_indices_concurrence/README.md) : propriété des
cohortes et durée de vie relues ; pont `release/acquire` proposé pour rattacher les indices au préfixe courant
(CST-0242). Aucun échec natif démontré ni gain qualifié.

**CST-0241 clos : correctif e789 et porte native bdf contre-vérifiés.** [Modèle et raccord](../receipts/audit_reponses_20261008/a_terminaison_raccord/README.md),
[preuve pour N](../receipts/audit_reponses_20261008/a_terminaison_porte/README.md),
[preuves natives](../receipts/audit_reponses_20261008/a_terminaison_native/README.md) : témoin11 contrôles, mutant
causal code1 sur l'attente indue après nettoyage,12 portes ciblées,313 sources raccordées. Portée CPU/u21,
sans preuve de toute la concurrence ni transfert de730 portes/20 répétitions. Aucun lien établi avec MES-M0.

**Juges.** [Livraison85 contre-vérifiée](../receipts/audit_reponses_20261008/lecteurs_livraison_85db/README.md) :
postimages, cohortes, horloges, résumés et hash final ; cinq portes Python,27/17 mutants normal/−O.
[Dernière garde de code processus proposée](../receipts/audit_reponses_20261008/lf_code_processus/README.md) :
False/0.0/−0.0/2.0 ne doivent pas être pris pour des entiers. Quatre témoins causaux, positif préservé.
CST-0018 reste partiel ; aucun chrono réel déclaré faux. Les reçus anciens restent immuables.

**[D6 M](../receipts/audit_reponses_20261008/session_m_d6/README.md)** : C u24/u32 +0,8–2,5 %, G jusqu'à+13,6 %,
étages CPU séparés. Seuil FULL<3 % ouvert. ×2048 reste sous2²⁹, sans précision physique nouvelle ;
[erratum proposé](../receipts/audit_reponses_20261008/d6_m_erratum/README.md) : étendues min/max, pas IC.

**Massifs, mesures antérieures à M :** [L1r](../receipts/audit_reponses_20261008/session_l1r_admission/README.md),
Boreas sans sol1,51M : GPU10,025s / CPU22,344s, une chaude ; dix succès/huit refus mémoire.
[L2 CPU froide](../receipts/audit_reponses_20261008/session_l2_admission/README.md) : Paris9,11M112,866s,
14,55M165,789s ; ETH3D `wide_leaf`, Lyon `memory_budget`. Pas de plafond universel en sites.
[Extension](../receipts/audit_reponses_20261008/feuille_large_proposition/README.md) et
[raffinement certifié](../receipts/audit_reponses_20261008/feuille_large_raffinement/README.md) proposés,
égalités massives persistantes ; aucune borne globale ni accélération acquise.
