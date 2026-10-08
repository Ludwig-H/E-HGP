# Audit Codex — état courant v12

8 octobre 2026. Cache2b : `bdfca8fb1` ; M : `957e9784f` ; petits C3 : `72f622a55`.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**LiDAR sans sol : les trois ng passent, le contrat multi-séquences 100 ms reste non tenu.**
[FULL M](../receipts/audit_reponses_20261008/session_m_admission/README.md) et [cache2b](../receipts/audit_reponses_20261008/cache2_admission/README.md), u21/W48 à chaud,
ng00/01/02 : 39 885 / 35 551 / 45 845 sites. Médianes des médianes par processus, en ms :

| FULL | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| GPU K5, cache désactivé, dernier apparié cache2b | **94,74** | **77,78** | **96,41** |
| GPU K5, cache 8 Gio, dernier apparié cache2b | **87,16** | **71,80** | **88,12** |
| CPU K5, cache désactivé | **381,80** | **323,10** | **385,94** |
| GPU K10, cache désactivé | **593,26** | **442,82** | **504,79** |

FULL M sans cache, maximum des médianes processus K5 **95,99 ms** : sous-contrat ng tenu selon la règle fixée.
Trois des135 prises chaudes dépassent néanmoins100 ms (maximum brut101,61).
**37 trames, six séquences : médiane160,64 ms, pire médiane par trame319,78 ms** ;
maximum contractuel **358,86 ms**, ici aussi maximum des185 chaudes. 25/37 médianes par trame dépassent100 ms.
Voie GPU : catalogue CUDA puis tour CPU. FULL couvre Cloud/index→T/M/V/R ; lecture, masque, validation, FUL1
et libération hors mur. G est une fenêtre recouverte, TMVR la queue après G ; ne pas sommer les médianes.

**[Dernier cache apparié : cache2b](../receipts/audit_reponses_20261008/cache2_admission/README.md), admis.**
Référence explicite sans cache94,743/77,780/96,413 ms ; A/A conforme à la règle sur les GM.
Rapports cache/ref0,9221/0,9254/0,9120, bornes IC95 supérieures0,9273/0,9310/0,9173 : cache adopté.
**37 trames informatives avec cache : médiane147,77 ms, pire médiane297,53 ms**, contre160,23/318,93 sans cache.
Deux secondes visites par trame ; ce bloc ne remplace pas le protocole contractuel M ci-dessus.
[Provenance et arrêt clos](../receipts/audit_reponses_20261008/cache2_provenance/README.md) :105 journaux/1 362 passes,
ELF initial/final identique, fermeture après les six Sessions. L'environnement « après » précède ces Sessions ;
codes informatifs inférés du pilote clos.730 portes CPU/u21 passantes sur G4, sonde CUDA construite séparément.
RSS cache maximal3,81 Gio ; le pic budgété actif exclut le cache inactif. **Aucun nouveau CPU/K10 dans ce lot.**
[Défaut8 Gio](../receipts/audit_reponses_20261008/cache_defaut_raccord/README.md) aussi CPU/K10 : ng00–02 encore non mesurés ainsi.
[M apparié](../receipts/audit_reponses_20261008/session_m_apparie/README.md) reste historique sans ELF final ;
son bras séquentiel rejeté1,515/1,465/1,529. M précède le correctif0241, contrairement à cache2b.

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
La variante MSF/rejeu reste une alternative écartée du prochain A6 pour garder le recouvrement. [Brouillon A6 N/I](../receipts/audit_reponses_20261008/a6_indices_concurrence/README.md) : propriété des
cohortes et durée de vie relues ; pont `release/acquire` proposé pour rattacher les indices au préfixe courant
(CST-0242). Aucun échec natif démontré ni gain qualifié.

**[G-APP admis, poursuite GPU rejetée](../receipts/audit_reponses_20261008/gapp_admission/README.md).**
Source389,15 processus/90 passes : census GPU/CPU0,091–0,107, sondes0,046–0,053 ; propositions0,738–0,758
échouent au seuil0,50. Ce sont des lots préparés résidents, pas G ni FULL intégrés.
[Lecture native et aide CPU](../receipts/audit_reponses_20261008/g_appareil_natif/README.md) : census partagé CPU
à0,570–0,585 du produit ; validations, formats, sorties et verrou changent ensemble. Proposer un parcours interne
spécialisé sous l'API actuelle, puis mesurer G/FULL ; préserver réentrance, coquilles, replis et compteurs.

**CST-0241 clos : correctif e789 et porte native bdf contre-vérifiés.**
[Preuve pour N](../receipts/audit_reponses_20261008/a_terminaison_porte/README.md) et
[primaires natives](../receipts/audit_reponses_20261008/a_terminaison_native/README.md) : mutant causal après
nettoyage,12 portes ciblées,313 sources. Clôture du défaut de retrait CPU/u21, pas de toute la concurrence.
Les730 portes sur G4 sont attestées séparément par cache2b ; aucun lien établi avec MES-M0.

**Juges.** [Livraison85 vérifiée](../receipts/audit_reponses_20261008/lecteurs_livraison_85db/README.md) ;
[garde de code processus LF proposée](../receipts/audit_reponses_20261008/lf_code_processus/README.md) :
False/0.0/−0.0/2.0 doivent être refusés, quatre témoins causaux et positif conservé.
[GAPP strict proposé](../receipts/audit_reponses_20261008/gapp_journal_strict/README.md) : phases, cohorte et codes,
28 cas/8 mutants Python ; jugement réel conservé. CST-0018 reste partiel, aucun chrono réel déclaré faux.

**[D6 M](../receipts/audit_reponses_20261008/session_m_d6/README.md)** : étages CPU séparés, C u24/u32 +0,8–2,5 %,
G jusqu'à+13,6 %. Seuil FULL<3 % ouvert. ×2048 reste sous2²⁹, sans précision physique nouvelle ;
[erratum](../receipts/audit_reponses_20261008/d6_m_erratum/README.md) : étendues min/max, pas IC.

**Massifs, antérieurs à M :** [L1r](../receipts/audit_reponses_20261008/session_l1r_admission/README.md),
Boreas sans sol1,51M : GPU10,025s / CPU22,344s, une chaude ; dix succès/huit refus mémoire.
[L2 CPU froide](../receipts/audit_reponses_20261008/session_l2_admission/README.md) : Paris9,11M112,866s,
14,55M165,789s ; ETH3D `wide_leaf`, Lyon `memory_budget`. Nouveau L1p sourcec648 en cours, aucun temps admis.
Pas de plafond universel en sites. [Raffinement certifié proposé](../receipts/audit_reponses_20261008/feuille_large_raffinement/README.md),
égalités massives persistantes ; aucune borne globale ni accélération acquise.
