# Audit Codex — état courant v12

8 octobre 2026, base publiée **`4cbcd3852`**.
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
Priorité : chaîne entière ; 100 ms, GPU u24/u32, petits nuages et millions restent à qualifier.

**Livraisons et qualifications.**

- **Gc `4df326cc8` + `10050a96e`** : [16 sources tour identiques au prototype](../receipts/audit_reponses_20261008/gc_livraison/README.md),
  cœur/catalogue inchangés. Empreinte objet et contrôle du travail séparés ; index parallèle, file de sondes G-L7.
  J : 344 sources conformes, 675 portes et six LiDAR passées ; les 26 différentielles v10 locales sont absentes
  sur G4. Campagne des 18 mutants passée. G-L7 et index adoptés, G-L5 rejeté. Pilote : défaut CLI huit maintenu,
  plan G4 explicite dix ; erratum de l'ancien conducteur local conservé dans le registre.
- **TMVR + Gc** : [repo6 u21](../receipts/audit_tmv_repo6_u21_20261008/README.md), arbre 382 fichiers,
  690 tests + une sentinelle sautée, MES-M0 neuf cas × deux modes, EMST et neuf chaînes natives CPU conformes.
  685 contrôles du pic passent ; budget fini/cache restent ouverts. Les 27 mutants, profils larges, livraison
  produit et FULL GPU restent à qualifier. [Repo5 historique](../receipts/audit_tmv_repo5_profils_20261008/README.md) :
  u24 clos, u32 interrompu ; aucune qualification transférée à l'assemblage.
- **D6 `0018`** : [schéma Gc refusé, patch strict proposé](../receipts/audit_reponses_20261008/gc_livraison/README.md),
  deux formats réels admis et 27 corruptions refusées. Composition avec le correctif du plan vérifiée ; quatre plans
  invalides refusés avant toute prise. Ces deux corrections restent à intégrer.

**Aide au développeur et prochains contrôles.**

- **Census G** : [garde resserrée proposée](../receipts/audit_reponses_20261008/garde_census/README.md),
  preuve MEB, 1 617 points/2 470 boîtes exacts, nouveau témoin de débordement intermédiaire. Deux lignes exécutables,
  voies conservées ; promotions plus fines prouvées séparément. Anciens mutants à reclasser, gain G4 à mesurer.
- **`0239`** : [tri pur puis contrôle adjacent](../receipts/audit_reponses_20261007/comparateur_naissances/README.md)
  repris dans repo6 ; faux doublon par auto-comparaison sous `_GLIBCXX_DEBUG`, configuration native non jouée.
- **`0240`** : [historique altéré accepté par le validateur](../receipts/audit_foret_validation_20261008/README.md),
  aucune mauvaise sortie produit démontrée. [Certificat de toutes les coupes](../receipts/audit_reponses_20261008/histoire_coupes_0240/README.md)
  en O(E+B log B), preuve et 27 238 historiques bornés ; gardes et certificat non intégrés. Le header repo6
  déclare sa limite, le contrat global reste à satisfaire ; aucune provenance union-find certifiée par le lemme.
- **`0104`** : [D2/MEMO natifs acquis](../receipts/audit_reponses_20261008/d2_memo/README.md) ;
  [mutant de date adapté à Gc](../receipts/audit_reponses_20261008/d2_memo_mutant_gc/README.md), exécution causale attendue.
- [Translation TMVR](../receipts/audit_reponses_20261008/translation_tmvr/README.md) : permutation Morton réelle,
  16 traces/13 classes, 30 requêtes et sept verticales exactes. Porte native proposée ; TARG général à comparer
  via la composante ouverte, pas comme indice brut. `0105/0107` attendent la livraison de TMVR.

`0009` clos (repli de feuilles distribué et compté) ; `0008` ouvert (admission à un seul pilote).
T7 natif, capacités 256/64 et compactage des réparations GPU restent suivis dans le registre et les reçus liés.
Quatre fichiers actifs, 75 constats ; détails historiques dans `receipts/`. Aucun GCP lancé ni donnée sous licence
par cet audit. Contrôles récents : sources, archives et Python ; natifs antérieurs limités aux formats synthétiques.
