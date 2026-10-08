# Audit Codex — état courant v12

8 octobre 2026. Base publiée **`485fb68ea`** ; prototypes distingués des livraisons.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Derniers temps G4 vérifiés.** [Session I](../receipts/audit_reponses_20261008/session_i_catalogue/README.md),
catalogue C hybride u21, 48 fils, transferts compris :

| Médiane chaude, ms | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| GPU K5 | **35,07** | **30,82** | **37,58** |
| GPU K10 | 137,38 | 112,90 | 146,22 |
| CPU K5, feuille 24 | 327,79 | 280,88 | 331,99 |
| CPU K10, feuille 24 | 1 066,82 | 889,36 | 1 054,34 |

K5 GPU : cinq processus × neuf passes chaudes ; maxima 36,92 / 32,30 / 38,51 ms.
K10 GPU : trois × quatre ; CPU : un × neuf. **Budget C de 45 ms tenu**, juge strict adopté,
identité dédiée sur neuf cas, trois mutants tués, exécution GPU et arrêt certifié archivés.
CPU/F2 : comparaison descriptive, sans attribution à A ou B seuls. Aucun temps FULL v12 acquis.

[G CPU, session H](../receipts/audit_reponses_20261007/session_h_mesures/README.md), W48 :
80,13 / 63,24 / 75,70 ms K5 ; 633,63 / 449,31 / 517,49 ms K10. Un processus, neuf/deux passes chaudes.
**Priorité : réduire G puis mesurer la chaîne entière** ; aucune somme de médianes H+I ne vaut un chrono FULL.
Trois trames d'une seule séquence ; les 100 ms, u24/u32 GPU et les autres régimes restent ouverts.

**Livraison et qualification suivies.**

- **Gc livré `4df326cc8` + `10050a96e`** : empreinte de l'objet séparée du travail, index des naissances,
  file de sondes, préparation parallèle. [Prototype rebasé](../receipts/audit_reponses_20261007/gc_rebase_portes/README.md) :
  675 portes rapides + 6 LiDAR, 680 distinctes sans saut ; [18 mutants finaux](../receipts/audit_reponses_20261007/gc_mutants_finaux/README.md)
  tués par code. [Livraison contre-vérifiée](../receipts/audit_reponses_20261008/gc_livraison/README.md) :
  16 sources tour identiques au prototype, cœur/catalogue inchangés. [Journal local](../receipts/audit_reponses_20261008/gc_livraison_complement/README.md) :
  700 Passed + une sentinelle sautée sur 701, sans clôture des sources ; aucun nouveau temps G4.
- **Pilote T2-c** : [essai relu](../receipts/audit_reponses_20261008/gc_pilote_essai/README.md), 68 prises/308 passes,
  jugement identique, refus attendu de deux tours sur dix. **Erratum** : le conducteur perd le statut externe après
  `date` ; les bilans primaires CTest/mutants restent acquis. Le défaut CLI huit subsiste ; plan G4 explicite dix.
- **T/M/V/R** : [repo5 u21](../receipts/audit_tmv_repo5_u21_20261008/README.md), 680 tests, une sentinelle sautée,
  neuf chaînes CPU C/G natives conformes et 16 mutants tués. Admission R corrigée : offsets CSR compris,
  685 contrôles du pic ; budget fini et cache encore à qualifier. Traces u24 closes (audit groupé en préparation),
  profil u32 en cours ; TMVR non livré.
  `0105/0107` attendent l'intégration ; `0212` clos au microbanc seulement.
- **Juges `0018`** : CUDA strict utilisé en I ; MES-P et D6 livrés et contre-jugés dans les reçus du registre.
  Plan D6 encore ouvert : u21, doublons, combinaison vide. [Schéma Gc](../receipts/audit_reponses_20261008/gc_livraison/README.md)
  refusé par D6 ; patch proposé, deux formats réels admis et 27 corruptions refusées.
  Composition avec le correctif du plan vérifiée, quatre plans invalides refusés avant toute prise.

[Raccord Gc + TMVR proposé](../receipts/composition_gc_tmvr_20261008/README.md) : deux conflits résolus,
19 sources, 27 mutants conservés ; aucune qualification native de la combinaison.
[Comparateur `0239`](../receipts/audit_reponses_20261007/comparateur_naissances/README.md) : rendre le tri sans effet
de bord ; faux doublon sous auto-comparaison `_GLIBCXX_DEBUG`, configuration non jouée.
**`0240`, mineur** : [validateur d'historique](../receipts/audit_foret_validation_20261008/README.md), objet altéré
accepté malgré une requête erronée ; preuve statique et modèle, aucune mauvaise sortie produit démontrée.
Gardes structurelles proposées, non compilées ; la sémantique complète des attaches reste à vérifier.

**Aide mathématique et preuves.**

- [D2/MEMO `0104`](../receipts/audit_reponses_20261008/d2_memo/README.md) : portes natives livrées et passées en I,
  niveaux rationnels confirmés. [Mutant proposé](../receipts/audit_reponses_20261008/d2_memo_mutant/README.md),
  [adapté à Gc](../receipts/audit_reponses_20261008/d2_memo_mutant_gc/README.md) ; exécution causale attendue.
- [Translation TMVR](../receipts/audit_reponses_20261008/translation_tmvr/README.md) : quatre sites, permutation
  Morton réelle, 16 traces stockées/13 classes, 30 requêtes et sept verticales exactes. Protocole natif proposé ;
  TARG général à comparer via la composante ouverte, pas comme indice brut.
- [Repli `0009` clos](../receipts/audit_reponses_20261008/repli_unresolved_0009/README.md) : lots parallèles,
  tampons comptés, portes CPU/GPU acquises ; aucun gain isolé. `0008` ouvert : admission à un seul pilote.
- [Témoin T7](../receipts/audit_reponses_20261007/t7_cercle25/README.md) prêt ; porte native absente.
  [Compactage des réparations GPU](../receipts/audit_reponses_20261008/chaines_compactes/README.md) proposé :
  10 449 cas, huit mutants, travail O(C). En I, ng02 répare 9/16 éléments mais rapatrie 22,5/87,7 Mo à K5/K10 ;
  aucun gain temps acquis. À mesurer après G.

Quatre fichiers actifs, 75 constats ; capacités 256/64 de `0237` ouvertes. Aucun GCP lancé ni donnée sous licence
par cet audit. Natifs antérieurs limités aux formats CPU synthétiques (deux points catalogue, deux sondes G à huit
points). Derniers contrôles : archives, sources et Python ; aucun benchmark supplémentaire.
