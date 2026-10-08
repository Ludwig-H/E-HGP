# Audit Codex — état courant v12

8 octobre 2026, base publiée **`cffe3e0da`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Derniers temps G4 : FULL mesuré, 100 ms non tenu.** [Session K contre-jugée](../receipts/audit_reponses_20261008/session_k_full/README.md),
u21, W48, trames ng00–02 de 39 885 / 35 551 / 45 845 sites :

| Médiane chaude FULL, ms | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| GPU K5, 5 processus × 9 chaudes | **159,30** | **127,63** | **163,27** |
| CPU K5, 3 × 4 | **441,07** | **368,33** | **447,08** |
| GPU K10, 3 × 4, informatif | 793,44 | 603,34 | 715,31 |

GPU K5 : maxima des médianes de processus 160,76 / 127,80 / 164,64 ms ; maxima bruts 164,46 / 132,95 / 173,55 ms.
**37 trames de six séquences : médiane 241,31 ms, maximum contractuel 467,92 ms**, 185 passes chaudes,
33 179–99 099 sites. Aucune trame sous le maximum de 100 ms. Les 610 passes et leurs empreintes concordent.
Mur de Cloud/index à T/M/V/R ; segmentation, lecture, validation, FUL1 et libération hors mur.
[Sources/commandes et arrêt ciblé vérifiés](../receipts/audit_reponses_20261008/session_k_provenance/README.md),
03:03:03 UTC ; **snapshot, sans hash binaire ni journal de compilation rapatrié**. Aucun CPU·s par trame.

Face aux mesures historiques v11 : GPU désormais plus rapide, CPU toujours plus lent ; comparaison descriptive,
pas A/B apparié. **Catalogue CPU : 274–329 ms**. Côté GPU, G coûte 44–57 ms, T 26–38 ms, R 12–15 ms.
Retirer comptablement R par passe laisse 116–148 ms médians : R seul ne suffit pas à 100 ms.
GPU u24/u32, petits nuages et millions restent à qualifier ; aucun CPU K10 joué dans K.

**Livraisons et qualifications.**

- **Gc `4df326cc8` + `10050a96e`** : [sources livrées vérifiées](../receipts/audit_reponses_20261008/gc_livraison/README.md).
  J valide index parallèle et file G-L7 ; G-L5 rejeté. Défaut CLI huit et collecte C restent ouverts.
- **TMVR `7398aed7d`** : [livraison identique au prototype](../receipts/audit_reponses_20261008/tmvr_livraison/README.md).
  MES-M0 neuf cas × deux modes, neuf chaînes CPU conformes sémantiquement à la v11, W1/W8 identiques, EMST.
  Intégration : 716 portes passées + une LiDAR sautée ; [27 mutants](../receipts/audit_tmv_repo6_mutants_20261008/README.md),
  685 contrôles du pic. Budget fini/cache, FULL GPU et u24/u32 de cet assemblage restent ouverts.
  Repo6 arrêté pendant la construction u24 ; aucun résultat de repo5 transféré implicitement.
- **D6 `0018/0207`** : [livraison `e37fd8935` vérifiée](../receipts/audit_reponses_20261008/d6_livraison_stricte/README.md),
  deux schémas et plan stricts. [K rejoué hors ligne](../receipts/audit_reponses_20261008/d6_session_k/README.md) :
  ancien lecteur → 66 alertes ; corrigé → 126 sorties/630 passes admises, ratios recalculés.
  C u24/u32 ×1 : +0,04–1,04 % ; G u32 : +4,3–11,2 %. ×2048 : C ×2,9, mais coordonnées **<2²⁹**, pas «32 bits pleins».
  Homothétie d'entiers déjà quantifiés, sans détail physique nouveau ; seuil D6 produit <3 % encore ouvert.

**Aide au développeur et prochains contrôles.**

- **MES-FULL `0018`** : [lacunes d’admission](../receipts/audit_reponses_20261008/mes_full_admission/README.md),
  [pilote livré `c9ac60f20` aux mêmes octets](../receipts/audit_reponses_20261008/mes_full_livraison/README.md), présent
  dans K : GPU inconnu, contreflux de temps/métadonnées et cohortes incomplètes admis. Bruts à contre-juger ;
  code de commande zéro insuffisant. [Contre-lecteur strict](../receipts/audit_reponses_20261008/mes_full_contrelecture/README.md) :
  53 corruptions refusées, 38 processus/610 passes K admis ; verdict « non tenu ». Pilote produit encore à renforcer.
- **R** : [classes à cellule unique](../receipts/audit_reponses_20261008/registre_classe_unique/README.md) :
  `q=d+1` autorise la copie des enfants sans recherche historique ni tri, mémoire temporaire réduite.
  Preuve et modèle ; sur les comptes ng00, au moins 78,81 % des requêtes K5 et 71,95 % K10 évitables.
  [Patch minimal fourni](../receipts/audit_reponses_20261008/registre_classe_unique_patch/README.md), non compilé,
  admission et pointeur nul contre-vérifiés. Aucun gain temps acquis. Comparer aussi les lignes/CSR de R :
  [FUL1](../src/tower/export_full.cpp) ne les encode pas ; la porte native `branches` les compare à la coupe ouverte.
- **Census G** : [garde resserrée proposée](../receipts/audit_reponses_20261008/garde_census/README.md),
  preuve MEB, modèles exacts et témoin de débordement intermédiaire. Voies du patch conservées ;
  promotions plus fines prouvées séparément. Anciens mutants à reclasser, adoption et gain G4 à mesurer.
- **`0239`** : [tri pur puis contrôle adjacent](../receipts/audit_reponses_20261007/comparateur_naissances/README.md)
  livré en `7398aed7d` ; faux doublon par auto-comparaison sous `_GLIBCXX_DEBUG`, configuration native non jouée.
- **`0240`** : [historique altéré accepté par le validateur](../receipts/audit_foret_validation_20261008/README.md),
  aucune mauvaise sortie produit démontrée. [Certificat de toutes les coupes](../receipts/audit_reponses_20261008/histoire_coupes_0240/README.md)
  en O(E+B log B), preuve et modèle ; gardes/certificat non intégrés. Header désormais explicite,
  contrat global encore ouvert ; le lemme ne certifie pas la provenance union-find.
- **`0104`** : [D2/MEMO natifs acquis](../receipts/audit_reponses_20261008/d2_memo/README.md) ;
  [mutant de date adapté à Gc](../receipts/audit_reponses_20261008/d2_memo_mutant_gc/README.md), exécution causale attendue.
- [Translation TMVR](../receipts/audit_reponses_20261008/translation_tmvr/README.md) : modèle vérifié, porte native proposée ;
  comparer TARG via la composante ouverte. `0105/0107` clos sur u21 (domaine, numérotation, mutant et empreintes),
  indépendamment de `0240`.

`0009` clos (repli de feuilles distribué et compté) ; `0008` ouvert (admission à un seul pilote).
T7 natif, capacités 256/64 et compactage des réparations GPU restent suivis dans le registre et les reçus liés.
Quatre fichiers actifs, 75 constats ; détails historiques dans `receipts/`. Aucun GCP lancé ni donnée sous licence
par cet audit. Contrôles récents : sources, archives et Python ; natifs antérieurs limités aux formats synthétiques.
