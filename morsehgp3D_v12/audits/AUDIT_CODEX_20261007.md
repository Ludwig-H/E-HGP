# Audit Codex — état courant v12

8 octobre 2026, base publiée **`9c9c25893`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Derniers temps G4 vérifiés.** [Session I](../receipts/audit_reponses_20261008/session_i_catalogue/README.md),
catalogue C hybride u21, W48, transferts compris :

| Médiane chaude, ms | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| GPU K5 | **35,07** | **30,82** | **37,58** |
| GPU K10 | 137,38 | 112,90 | 146,22 |
| CPU K5, feuille 24 | 327,79 | 280,88 | 331,99 |
| CPU K10, feuille 24 | 1 066,82 | 889,36 | 1 054,34 |

K5 GPU : cinq processus × neuf passes chaudes ; maxima 36,92 / 32,30 / 38,51 ms.
K10 GPU : trois × quatre ; CPU : un × neuf. **Budget C de 45 ms tenu**, identité dédiée sur neuf cas,
juge strict, mutants et arrêt certifié vérifiés. Gain du lot A/B, sans attribution isolée.
[G CPU, session J](../receipts/audit_reponses_20261008/session_j_gc/README.md), W48 : **53,06 / 42,11 / 48,20 ms K5**,
maxima 54,76 / 42,95 / 50,18 ms ; médiane des dix médianes de processus, neuf passes chaudes chacun.
K10 informatif : 440,90 / 321,28 / 354,65 ms, trois processus × deux passes chaudes.
188 journaux stricts, sources et arrêt archivé contre-vérifiés ; gains appariés K5 de 33–37 %.
**Aucun temps FULL v12 acquis** ; une somme de médianes I+J ne le remplace pas. Trois trames d'une seule séquence.
La sonde FULL `c40318ebd` reprend la [frontière proposée](../receipts/audit_reponses_20261008/frontiere_full_proposee/README.md),
de Cloud/index à T/M/V/R en mémoire. [Paquet K vérifié](../receipts/audit_reponses_20261008/session_k_snapshot/README.md) :
FULL puis D6 ; aucun résultat rapatrié au constat du 8 octobre 02:36:54 UTC.
100 ms, GPU u24/u32, petits nuages et millions restent à qualifier.

**Livraisons et qualifications.**

- **Gc `4df326cc8` + `10050a96e`** : [sources livrées vérifiées](../receipts/audit_reponses_20261008/gc_livraison/README.md).
  J valide index parallèle et file G-L7 ; G-L5 rejeté. Défaut CLI huit et collecte C restent ouverts.
- **TMVR `7398aed7d`** : [livraison identique au prototype](../receipts/audit_reponses_20261008/tmvr_livraison/README.md).
  MES-M0 neuf cas × deux modes, neuf chaînes CPU conformes sémantiquement à la v11, W1/W8 identiques, EMST.
  Intégration : 716 portes passées + une LiDAR sautée ; [27 mutants](../receipts/audit_tmv_repo6_mutants_20261008/README.md),
  685 contrôles du pic. Budget fini/cache, FULL GPU et u24/u32 de cet assemblage restent ouverts.
  Repo6 arrêté pendant la construction u24 ; aucun résultat de repo5 transféré implicitement.
- **D6 `0018/0207`** : [livraison `e37fd8935` vérifiée](../receipts/audit_reponses_20261008/d6_livraison_stricte/README.md),
  deux schémas exacts et admission du plan ; 27 corruptions et quatre plans fautifs refusés. Essai local ng00 K3 :
  huit journaux/seize passes relus, sans qualification de temps. **K conserve l’ancien lecteur**, sans transfert.

**Aide au développeur et prochains contrôles.**

- **MES-FULL `0018`** : [lacunes d’admission](../receipts/audit_reponses_20261008/mes_full_admission/README.md),
  [pilote livré `c9ac60f20` aux mêmes octets](../receipts/audit_reponses_20261008/mes_full_livraison/README.md), présent
  dans K : GPU inconnu, contreflux de temps/métadonnées et cohortes incomplètes admis. Bruts à contre-juger ;
  code de commande zéro insuffisant. Aucun temps brut déclaré faux ; contre-lecteur strict en préparation.
- **R** : [classes à cellule unique](../receipts/audit_reponses_20261008/registre_classe_unique/README.md) :
  `q=d+1` autorise la copie des enfants sans recherche historique ni tri, mémoire temporaire réduite.
  Preuve et modèle ; sur les comptes ng00, au moins 78,81 % des requêtes K5 et 71,95 % K10 évitables.
  Aucun pourcentage de temps ni implantation native acquis. Comparer aussi les lignes/CSR de R :
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
