# Audit Codex — état courant v12

8 octobre 2026. Cache2b : `bdfca8fb1` ; M : `957e9784f` ; petits C3 : `72f622a55` ; massifs L1p : `c648b3857`.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**LiDAR sans sol : les trois ng passent, le contrat multi-séquences 100 ms reste non tenu.**
[FULL M](../receipts/audit_reponses_20261008/session_m_admission/README.md) et [cache2b](../receipts/audit_reponses_20261008/cache2_admission/README.md), u21/W48 à chaud,
ng00/01/02 : 39 885 / 35 551 / 45 845 sites. Médianes des médianes par processus, en ms :

| FULL | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| GPU K5 sans cache, cache2b | **94,74** | **77,78** | **96,41** |
| GPU K5 cache 8 Gio, cache2b | **87,16** | **71,80** | **88,12** |
| CPU K5, cache désactivé | **381,80** | **323,10** | **385,94** |
| GPU K10, cache désactivé | **593,26** | **442,82** | **504,79** |

FULL M sans cache : maximum des médianes processus K5 **95,99 ms**, sous-contrat ng tenu.
**37 trames, six séquences : médiane160,64 ms, pire médiane par trame319,78 ms** ;
maximum contractuel **358,86 ms**, ici aussi maximum des185 chaudes. 25/37 médianes par trame dépassent100 ms.
Voie GPU : catalogue CUDA puis tour CPU. FULL couvre Cloud/index→T/M/V/R ; lecture, masque, validation, FUL1
et libération hors mur. G est une fenêtre recouverte, TMVR la queue après G ; ne pas sommer les médianes.

**[Cache2b admis](../receipts/audit_reponses_20261008/cache2_admission/README.md), cache adopté.**
Référence cache0, A/A conforme ; rapports cache/ref 0,9221/0,9254/0,9120,
IC95 hauts 0,9273/0,9310/0,9173. Lot informatif de 37 trames avec cache :
**médiane 147,77 ms, pire médiane 297,53 ms**, contre 160,23/318,93 sans cache.
Deux secondes visites/trame : distinct du protocole contractuel M.
[Provenance/arrêt clos](../receipts/audit_reponses_20261008/cache2_provenance/README.md) : ELF initial/final identique,
fermeture après les Sessions ; relevé environnement antérieur au bloc informatif. 730 portes CPU/u21 passantes.
Pic actif hors cache inactif. **Aucun nouveau CPU/K10** ; le défaut cache8 Gio n'y est pas qualifié sur ng00–02.
M précède le correctif0241 ; ses anciennes fermetures ELF ne sont pas complétées par cache2b.

**Petits LiDAR : [MES-C3 admise](../receipts/audit_reponses_20261008/session_c3_admission/README.md), critères C1–C3 non tenus.**
132 réels seuls, cache8 Gio, W48 ; médiane des deux chaudes par nuage, puis médiane des132, en ms :

| FULL | CPU | GPU |
| --- | ---: | ---: |
| K5 | **25,003** | **7,856** |
| K10 | **46,420** | **14,571** |

Maxima des médianes réelles : K5 CPU/GPU134,07/32,64 ms ; K10 515,63/199,35 ms.
Huit refus `wide_leaf` persistent dans la cohorte difficile.
W1 absent ; sur≤150 sites, CPU W4/W48 vaut6,08/10,88 ms.
[C3 appariés cache](../receipts/audit_reponses_20261008/session_c3_apparies/README.md) : les deux campagnes
refusées par A/A, donc aucune conclusion d'absence d'effet. [Provenance](../receipts/audit_reponses_20261008/session_c3_provenance/README.md) :
arrêt clos, ELF final absent ; comparaison C2 descriptive, plusieurs changements.

**Priorités de calcul.** [CPU](../receipts/audit_reponses_20261008/cpu_feuilles_finition/README.md) : C318,77/272,70/320,92 ms
sur ng00–02, soit83–84 % FULL. Finition déjà parallèle ; même gratuite, FULL conditionnel268–321 ms.
Paires uniques et arrêt census intégrés ; feuille16 sans gain global face à24. Comparaison v11 non appariée : GPU plus rapide, CPU encore plus lent.
[Noyau](../receipts/audit_reponses_20261008/a6_noyau_raccord/README.md) : R5 dernier sur99 099 sites ; les fins ne
sont pas des durées propres. À P/C/G inchangés, queue gratuite laisse238,32 ms et22/37 médianes>100 ms.
MSF/rejeu reste écarté du prochain A6. [Publication des indices A6](../receipts/audit_reponses_20261008/a6_indices_concurrence/README.md) :
pont release/acquire proposé. [Fragment de 47 événements](../receipts/audit_reponses_20261008/a6_prefixe_relaxed/README.md)
admis par les clauses mémoire examinées, exclu par le pont ; ni trame déclenchante ni échec natif établis.
[21 grandes trames × 3 bras](../receipts/audit_reponses_20261008/a6_identites_complementaires/README.md) : FUL1 égales,
CPU/u21/K5/W3. Ces 63 sorties ne closent pas CST-0242. [A6 livré](../receipts/audit_reponses_20261008/a6_livraison_0242/README.md) en `30a69104a` :
mêmes corps relaxed, pont applicable, preuve de préfixe ouverte. [Première tentative G4](../receipts/audit_reponses_20261008/session_a6_tentative/README.md) :
worker1, arrêt certifié, archive non rapatriée faute de marge disque ; aucun temps A6 admis.
Codespace nettoyé le8 à12:39 UTC : environ8,4 Go libérés, 8,9 Gio disponibles.
[Admission N](../receipts/audit_reponses_20261008/a6_admission_n/README.md) : borne par tâche proposée, aucun gain mesuré.

**[G-APP2 admis ; D1 et D2 rejetés](../receipts/audit_reponses_20261008/gapp2_admission/README.md).**
Source9815 : 75 chaudes K5 décisives ; identités exactes, aucun refus.
D1 : borne IC95 propositions GPU/CPU0,511–0,525 >0,50 ; deux totaux sur trois >0,15.
D2 : propositions0,960–1,096 >0,20, totaux0,218–0,247 >0,10. K10 reste informatif.
Lots préparés et certification CPU après chrono : **aucun gain G/FULL intégré**. [Preuve des issues](../receipts/audit_reponses_20261008/gapp2_issues_contrat/README.md) :
boule minimale unique sous certification exacte ; mécanisme et issue distincts, aucun amendement acquis.
[Raccord CPU B2-C](../receipts/audit_reponses_20261008/b2_census_raccord/README.md) statiquement cohérent :
API/replis conservés, bornes signées vérifiées ; préconditions brutes à expliciter, gain à mesurer.

**CST-0241 clos** : [retrait CPU/u21 corrigé, preuve et primaires](../receipts/audit_reponses_20261008/a_terminaison_native/README.md). Concurrence générale non close.

**Juges.** [Livraison85 vérifiée](../receipts/audit_reponses_20261008/lecteurs_livraison_85db/README.md) ;
[garde LF proposée](../receipts/audit_reponses_20261008/lf_code_processus/README.md) : codes de type entier strict.
[Port GAPP2 proposé](../receipts/audit_reponses_20261008/gapp2_journal_strict/README.md) : phases/cohorte/codes,
22 cas/5 mutants Python ; les cinq contrefaçons refusées, jugement réel inchangé (D1/D2 rejetés).
[Juge A6 proposé](../receipts/audit_reponses_20261008/a6_pilote_admission/README.md) : cinq fausses adoptions
refusées après fermeture du plan/cohorte/A-A ; mode essai corrigé avant Markdown. Aucun chrono réel déclaré faux.

**[Massifs L1p admis](../receipts/audit_reponses_20261008/session_l1p_admission/README.md), sourcec648 :**
Boreas sans sol1,513M : **GPU7,298s / CPU20,056s** ; GPU Marseille sans sol2,465M8,349s,
Scion sans sol3,439M30,982s, Meadow6,181M30,565s. Une seule chaude par cas cité.
18 processus/15 scènes :10 succès,8 refus mémoire ; B1/B2/B4 non tenus, B3 non évalué.
[Sources/arrêt clos](../receipts/audit_reponses_20261008/session_b1p_provenance/README.md), mêmes entrées L1r,
39 fichiers natifs changés : comparaison descriptive, aucun gain causal isolé. FUL1 seulement jusqu'à1,6M.
Meadow : C/G/queue2,720/7,571/19,783s ; Boreas CPU C13,959s sur20,056s. Pas de seuil universel en sites.
Plus grands succès : GPU, Marseille brut 6,709 M en 26,602 s à froid ;
[L2 CPU](../receipts/audit_reponses_20261008/session_l2_admission/README.md), Paris brut 14,552 M en 165,789 s à froid ;
[raffinement certifié proposé](../receipts/audit_reponses_20261008/feuille_large_raffinement/README.md), égalités massives ouvertes.
