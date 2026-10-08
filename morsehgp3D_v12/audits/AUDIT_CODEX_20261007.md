# Audit Codex — état courant v12

8 octobre 2026. B2 mesuré : `4171b2653` ; T1-d : `5f8e777cf`/`c31beaf22` ; R1 : `47feedc96`.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**B2 adopté ; le contrat FULL multi-séquences à 100 ms reste non tenu.**
[Relecture B2](../receipts/audit_reponses_20261008/session_b2_admission/README.md) :373 processus/2816 FULL,
dont2100 chaudes décisives ; u21/W48/cache8 Gio, catalogue GPU puis G/T/M/V/R CPU. ng00/01/02 :
39 885/35 551/45 845 sites. Millisecondes, médianes des médianes par processus :

| FULL | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| **GPU K5 B2** | **82,63** | **67,77** | **84,87** |
| GPU K5 référence appariée | 87,29 | 71,63 | 88,07 |
| GPU K10 B2, informatif | 519,57 | 384,93 | 444,93 |
| CPU K5 M, ancien sans cache | 381,80 | 323,10 | 385,94 |

B2 K5 : dix processus × sept chaudes/trame. Maxima des médianes processus82,96/68,49/85,43 ms ;
maxima bruts **102,81/70,41/86,68 ms**. Une passe ng00 sur70 dépasse100 ms.
Deux trames décisives de64 740/67 114 sites : **147,50/172,66 ms** ; toutes leurs passes>100 ms.
K10 : un seul processus, quatre chaudes, sans identité K10 dans ce lot. **Aucun FULL CPU pur nouveau** :
`cpu_ns` est le travail CPU du processus hybride et `--sequentiel` garde le catalogue GPU.
FULL couvre Cloud/index→T/M/V/R ; lecture, masque, validation, FUL1 et libération hors mur.
Les fenêtres G/forêt se recouvrent : ne pas sommer leurs médianes.

B2 et tables seuls adoptés ; relecture et census seuls rejetés selon la règle écrite. Pour census, seule ng02
échoue : borne haute1,000220 ; la trame64 740 passe à0,999638 malgré l'arrondi1,000 du reçu développeur.
Sources/archives/arrêt certifié relus,733 portes socle passées ; fermeture ELF après le décisif K5,
avant les informations K10/grande/séquentiel. Après reconstruit :132/134 sources identiques, deux différences
de crochets CST0241 vides hors macro ; équivalence statique déclarée, pas identité littérale de tous les fichiers.

**Couverture des grandes trames inchangée.** [M](../receipts/audit_reponses_20261008/session_m_admission/README.md),
37 trames/six séquences sans cache : médiane160,64 ms, pire médiane319,78 ms, maximum contractuel358,86 ms.
[Cache2b](../receipts/audit_reponses_20261008/cache2_admission/README.md), lot37 informatif avec cache :
médiane147,77 ms, pire médiane297,53 ms ; deux secondes visites/trame, distinct du protocole M.
B2 ne remplace pas ces campagnes par cinq seules trames.

**A6 retiré ; CST-0242 clos par retrait.** [Pont, primaires récupérés et verdicts](../receipts/audit_reponses_20261008/a6_retrait_qualification/README.md) :
leaf release/acquire intégré6497, juge fermé livré exactement ; les deux campagnes85 processus/1306 passes
sont rejetées sans refus d'admission. Sur6497, grandes−8,6% mais ng00/01+3,3/+4,3%, au-delà du garde-fou2%.
`ab5614c2a` restaure exactement les trois scopes tower de bdf. Le gain grandes ne justifie pas une adoption.
Les anciennes archives manquantes sont récupérées ; leurs échecs de rapatriement restent historiques.
Ni panne native de concurrence ni coût causal isolé du pont établis ; A6b doit être requalifié.
[Borne de scratch proposée](../receipts/audit_reponses_20261008/a6_admission_n/README.md) : aucun gain mesuré.
**CST-0241 clos séparément** : [retrait des travailleurs corrigé](../receipts/audit_reponses_20261008/a_terminaison_native/README.md).

**[T1-d2 admis](../receipts/audit_reponses_20261008/session_t1d_admission/README.md), coût C stable à1% près.**
168 processus/campagne, C GPU K5 **26,23/23,20/26,46 ms** ; IC95 hauts1,0034/1,0063/1,0082.
Archives/sources/arrêts clos ; T1-d1 reste refusé pour son format FULL, T1-d2 emploie FUL1 séquentiel.
Le [port strict c31](../receipts/audit_reponses_20261008/t1d_admission/README.md) conserve ces deux verdicts.
Sous budgets :12 succès/24 refus initiaux ; jusqu'à3 tranches K5,7 tranches et7 lots K10.
Pics propres cumulatifs contrôlés. Aucun FULL massif ni nouveau chrono CPU seul dans ces sessions.

**Identité sous budget à compléter** : MHGP12DP omet niveaux et table ; FUL1 est joué sans ces budgets.
[Complément prêt](../receipts/audit_reponses_20261008/t1d_identite_flux_proposition/README.md), après port_c31 :
empreintes hors mur, référence CPU→libre→budgets, préfixes avant refus compris ;46 injections/6 mutations Python.
Cela couvre niveaux exacts et requêtes positives S*, pas les octets internes ni toutes les requêtes négatives.
Les anciennes campagnes gardent leur portée ; seuil1% sur **C seul**, aucun contrat FULL100ms déduit.

[Lecture produit](../receipts/audit_reponses_20261008/t1d_produit/README.md) : frontières F4/rebasage/table cohérents
sous leurs invariants. [Correction mémoire proposée](../receipts/audit_reponses_20261008/t1d_metadonnees_proposition/README.md) :
plan compté, coexistence ancien/nouveau réservée ; raccord des descripteurs propriétaires encore en pseudocode.
Publication des niveaux et rassemblement manquent au détail appareil, mais restent dans le mur C.
Aucun code natif exécuté par l'auditeur ; refus/réemploi/identités de ces corrections restent à qualifier.

**[R1 intégré47feed](../receipts/audit_reponses_20261008/r1_raccord_math/README.md), avis mathématique favorable.**
Critère q=d+1, lignes et ordre des enfants conservés. Admission Session encore conservative ; R ne débloque
ni G ni V. Source des portes causales lue ; campagne G4 rapatriée, admission des temps en cours.
**CPU** : [diagnostic antérieur](../receipts/audit_reponses_20261008/cpu_feuilles_finition/README.md), C représente83–84% FULL ;
finition déjà parallèle, feuille16 sans gain global face à24. Comparaison v11 non appariée : GPU plus rapide,
CPU encore plus lent. [G-APP2](../receipts/audit_reponses_20261008/gapp2_admission/README.md) rejeté pour D1/D2,
lots préparés et certification hors chrono : aucun gain FULL GPU de cette piste. Raccord census B2 mesuré dans le lot. [Fixtures u32 proposées](../receipts/audit_reponses_20261008/b2_norme_u32/README.md) :
deux segments d’étendues30/32 atteignent certified et exigent une norme i128, Fraction conforme.
Aucun défaut actuel trouvé ; porte native u32 restant à exécuter.

**Petits LiDAR** : [C3](../receipts/audit_reponses_20261008/session_c3_admission/README.md),132 réels W48/cache8 Gio,
K5 CPU/GPU25,003/7,856 ms ; K10 46,420/14,571 ms. Huit refus `wide_leaf` dans la cohorte difficile ;
W1 absent, W48 pénalise≤150 sites. [Deux campagnes cache appariées](../receipts/audit_reponses_20261008/session_c3_apparies/README.md)
refusées par A/A : aucune absence d'effet conclue.

**Massifs** : [L1p](../receipts/audit_reponses_20261008/session_l1p_admission/README.md), Boreas sans sol1,513M :
GPU7,298s / CPU20,056s ; GPU Marseille sans sol2,465M8,349s, Scion sans sol3,439M30,982s,
Meadow6,181M30,565s, une chaude par cas. 18 processus/15 scènes :10 succès/8 refus mémoire, B1/B2/B4 non tenus.
Plus grands succès FULL K5 : GPU Marseille brut6,709M en26,602s à froid ;
[CPU L2](../receipts/audit_reponses_20261008/session_l2_admission/README.md), Paris brut14,552M en165,789s à froid.
FUL1 seulement jusqu'à1,6M en L1p ; pas de plafond universel en sites ni de contrat100ms massif acquis.

Canal courant compact ; détails et anciens états dans les reçus. Nettoyage machine antérieur : environ8,4Go libérés,
sources/builds/preuves préservés.
