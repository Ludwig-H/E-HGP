# Audit Codex — état courant v12

10 octobre 2026. **A6c adopté ; B3-K, clés seules, adopté en `2aaed1847`.**
Cadre : `exploration_v12_hors_registre`,
`cpu_reference ; cuda_g4 pour le catalogue`, `full_pi0`, `quantized_u21_input_only`,
`not_claimed`. Autorité : [registre](CONSTATS.md).

[A6c admis](../receipts/audit_reponses_20261010/session_a6c_admission/README.md) sur sa cohorte :
21 grandes, médiane 152,44→132,33 ms ; seuil 43 900 calibré sur ces scènes, réserves ci-dessous.

**Session O : derniers temps FULL chauds, u21/W48/cache 8 Gio.**
[Admission](../receipts/audit_reponses_20261010/session_fullo_admission/README.md),
[statistiques indépendantes](../receipts/audit_reponses_20261010/fullo_temps/README.md).
GPU signifie catalogue GPU puis tour CPU. Médianes chaudes réunies, en ms :

| FULL | ng00, 39 885 sites | ng01, 35 551 sites | ng02, 45 845 sites |
| --- | ---: | ---: | ---: |
| **GPU K5** | **80,36** | **66,93** | **79,34** |
| **CPU K5** | **354,73** | **298,50** | **355,14** |
| GPU K10 | 492,64 | 365,52 | 422,87 |

37 trames/six séquences, cinq secondes visites chacune : médiane des médianes **116,59 ms**,
pire médiane **239,77 ms**, maximum **241,89 ms** ; **15/37** sous 100 contre 14 dans N.
**Contrat non tenu.** Sources 496/CTests 755/archive vérifiées ; codes/stderr individuels et
hashes finaux des sondes absents dans O : relecture concordante sous ces limites.
Lecture, masque, ouverture, validation, empreinte et libération hors mur ; CPU K10/37 absents.

**Priorités performance.** Dans O, annuler la queue seule laisse seulement **16/37** médianes
sous 100 ms, pire 219,23 ms. Réduire G/C ; G inclut la forêt concurrente, donc mesurer
préfixes/contention par ordre et mur FULL avant de changer les priorités.
CPU : [comparaison v11](../receipts/audit_reponses_20261010/v11_v12_cpu_gpu/README.md), O reste **+13,1–17,0 %** sur ng00–02, **+12,5–16,2 %** même sans P. [Piste census](../receipts/audit_reponses_20261008/cpu_census_reduction/README.md). [Comparaison appariée proposée](../receipts/audit_reponses_20261010/protocole_cpu_v11_v12.md) : produit conservé, validation entre passes différente.
Petits : C1/C2/C3 non tenus ; W48 plus lent que W4 sur 20 très petits, signal à confirmer.

**CST-0244/0245 ouverts.** [Patch d’admission proposé](../receipts/audit_reponses_20261010/a6c_admission_proposition/README.md) :
choix A6c figé avant la borne, supplément N conditionnel, bascule tardive refusée.
[Portes ON/OFF proposées](../receipts/audit_reponses_20261010/a6c_penurie_proposition/README.md) :
budgets exact/moins un, injections et reprise FUL1, grille K2 à cohorte garantie.
Non intégrés, ni compilés ni exécutés ; aucune clôture native.
[Critères du repli mémoire annoncé](../receipts/audit_reponses_20261010/repli_memoire_prelecture.md) : état rendu, résolution complète, route et coût publiés.

**B3-K :** [admission](../receipts/audit_reponses_20261010/session_b3b_admission/README.md),
[statistiques](../receipts/audit_reponses_20261010/b3b_stats/README.md) : 300 processus/2 100 chaudes.
Clés seules : gainGM **1,44–2,85 %**, ng00/01/02 **78,63/64,90/78,12 ms** (médianes de médianes).
Balayage seul rejeté, aucun bénéfice ajouté établi. [Produit vérifié](../receipts/audit_reponses_20261010/b3k_produit/README.md) : 138 src identiques au bras clés. Pas de nouveau jugement des 37 ;
réserves codes G/stderr/clôture ELF dans l’admission, juge v1 incomplet.

**Aide mathématique G :** [quatre réponses](../receipts/audit_reponses_20261010/REPONSE_G_TRAVAIL.md).
Composantes locales séparables, partage entre ordres limité, première sonde sans copie :
preuves/modèles, aucun gain natif. D7 couvre toutes tailles, dont 99 099 sites.
Patches : [égalité exacte dans find](../receipts/audit_reponses_20261010/g_population_egalite/README.md)
avec mutants compagnons, [T1 sur F\S](../receipts/audit_reponses_20261010/t1_support_population/README.md).
53 560/24 300 cas de modèle ; non intégrés, mesurer séparément. [Profil B3b](../receipts/audit_reponses_20261010/b3b_profil/README.md) : 75–77 % de premiers hits, lot complet, pas clés seules.

**Grandes scènes B1o closes (ae8/B3-K).** [Admission](../receipts/audit_reponses_20261010/session_mesb1o_admission/README.md),
[comparaison B1t](../receipts/audit_reponses_20261010/mesb1o_temps/README.md) : 19 processus, 23 FULL,
huit secondes passes uniques, 14 succès/cinq refus. Aucun cas sauté ; quatre identités FUL1 sous 1,6M.
Toutes 23 FULL plus rapides, pics hôte tous en hausse, appareil inchangés ; comparaison descriptive.
K5 GPU : Boreas 10 sans sol **4,89 s chaud** (1,513M), Meadow **13,50 s chaud** (6,181M),
Marseille entier **13,54 s première seule** (6,709M, plus grand succès du lot).
CPU : Boreas 10 sans sol **17,63 s première seule**, aucune chaude CPU. K10 : 36,25/36,34 s premières.
Queue réduite, dernier R désormais K4 dans 17/23 ; réduire G/C, pas seulement R(K5).

**CST-0243 persiste :** TU Wien sans sol 5,200M, **24,82 s première**, puis refus mémoire ;
pic/capacité appareil inchangés 85 886 193 388 octets, cause non localisée. Les quatre autres
refus NIBIO/Boreas 50 n’ont aucune FULL ni stade publié : « hôte pendant la tour » reste inféré.
Erratum des effectifs entier/sans sol dans le reçu comparatif. [Diagnostic proposé](../receipts/audit_reponses_20261008/b1t_tuwien_memoire/README.md)
toujours applicable : source/build distincts pour GPU (`--sonde/--essai` force CPU), lecteur stderr dédié.
[Record R1 antérieur](../receipts/audit_reponses_20261008/session_l2t_admission/README.md) : Paris 9,111M,
K5 GPU 46,453 s première, non retesté ; aucune qualification chaude transférée à A6c/B3-K.

**G4 revérifiée à 21:12:05 UTC :** [TERMINATED, zéro VM E-HGP active](../receipts/audit_reponses_20261010/g4_controle_211205.json), même génération et garde que le [contrôle précédent](../receipts/audit_reponses_20261010/g4_controle_mesb1o/README.md).
Aucun nouvel ordre d’arrêt nécessaire ; constat à cet instant.
