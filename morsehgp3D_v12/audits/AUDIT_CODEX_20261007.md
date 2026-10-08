# Audit Codex — état courant v12

8 octobre 2026. Base publiée **`d2f39fe82`** ; intégration en cours, prototypes distingués dans les reçus.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité des états : [registre](CONSTATS.md).

**À intégrer ou corriger.**

- **Juges, `0018`** : [G typé corrigé](../receipts/audit_reponses_20261007/livraison_juges/README.md), puis
  [CUDA strict livré `781fbe8d1`](../receipts/audit_reponses_20261008/cuda_juge_livraison/README.md), 49 contrôles Python.
  `device_open` renforcé reste à jouer sur appareil réel. Lecteurs [MES-P](../receipts/audit_mes_p_livraison_20261008/README.md)
  et [D6](../receipts/audit_reponses_20261008/d6_compatibilite/README.md) livrés et contre-jugés ;
  **plan D6 encore ouvert** (u21, doublons, combinaison vide), correctif proposé toujours applicable.
- **T2-c** : [lecteur strict intégré au prototype](../receipts/audit_reponses_20261007/t2c_pilote_integration/README.md),
  21 témoins, trois campagnes, cinq auto-tests, 30 journaux réels. Plan dix tours et garde `rapport` confirmés.
  Défaut CLI encore huit à corriger. Le rebasage Gc a retiré la collecte séparée de C ; aucune somme C+G intégrée
  ni campagne G4 acquise à ce pin.
- **Catalogue A/B livré** : [sources contre-vérifiées](../receipts/audit_reponses_20261007/cuda_livraison_a/README.md),
  `8ba7d7287` puis `c903774b1`. Voie hybride ≤32 sites/16 bits locaux, sinon reprise CPU exacte ;
  paires CPU calculées une fois et census arrêté au rejet. A local : 686 réussites + une sentinelle sautée, CUDA désactivé.
  [B local](../receipts/audit_reponses_20261007/cuda_b3_portes_locales/README.md) : 15 mutants, ASan/UBSan 44/44,
  TSan 12/12 ; cache actif et GPU non exercés. Aucun gain temps G4 encore acquis.
- **T/M/V/R** : [preuves antérieures](../receipts/audit_tmv_profils_20261007/README.md) u21/24/32 conservées.
  [Nouveau patch `6f0643ac`](../receipts/audit_tmvr_admission_20261008/README.md) : admission CSR corrigée,
  `4 A + 8 Σ(R_k+1)`, garde avant allocation ; porte du pic et mutant ciblé, budget illimité sans cache.
  **Qualification repo5 en cours**, sans transfert de repo3/repo4. `0105/0107` attendent la livraison ; `0212` reste
  clos au microbanc, portée produit à étendre. T7 toujours absent des portes natives.
- **Gc rebasé `45976be8`** : [table 16 octets retirée](../receipts/audit_reponses_20261007/gc_support_domain_delta/README.md),
  catalogue A/B conservé. Anciennes performances non transférables ; profil du lot complet sans attribution causale
  à la seule table. Garde indépendante de tous les SiteIdx proposée ; aucun défaut FULL valide déduit.

**Temps disponibles.** [G4 H](../receipts/audit_reponses_20261007/session_h_mesures/README.md), **G CPU, 48 fils**,
ng00/01/02 : **80,13 / 63,24 / 75,70 ms K5**, **633,63 / 449,31 / 517,49 ms K10**.
Un processus par cas, neuf passes chaudes K5, deux K10. Catalogue CPU K5 : 451,6 / 372,7 / 469,5 ms ;
v11 historique 200 / 163 / 195 ms, comparaison descriptive. **Aucun temps intégré catalogue GPU ou FULL v12 publié.**
Trois trames d'une seule séquence ; les 100 ms restent ouverts.

**Mathématiques et travail.** [Index Gc](../receipts/audit_reponses_20261007/gc_index_borne/README.md) : recherche exacte
logarithmique sous collisions, gros seau encore séquentiel ; G-L5 reste une dichotomie par requête.
[Hash par cellule](../receipts/audit_reponses_20261007/gc_hash_cellule/README.md) : 5 000 égalités, gain conditionnel ;
W1/W8 seul ne détecte pas un mutant commun déterministe. [Portes au pin `1b562def`](../receipts/audit_reponses_20261007/gc_portes_locales/README.md) :
18 mutants tués par code, 71/71 en u24 et u32 ; TSan tracé mais commandes/code externe non archivés.
Changements postérieurs exclus. Chronos disjoints, reconstruction par appel.
[Identité G](../receipts/audit_reponses_20261007/gc_empreinte/README.md) encore liée à la politique de résolution.
[Témoin T7 prêt](../receipts/audit_reponses_20261007/t7_cercle25/README.md) : quatre sites exacts, cinq fenêtres dont
seulement trois maximales, mêmes partitions 1/2/2/0 ; future porte native à graver, aucun défaut FULL actuel déduit.

Pistes : [finition et feuilles CPU](../receipts/audit_performance_20261007/README.md),
[census/supports](../receipts/audit_g_pistes_20261007/README.md),
[préparation D6 hors chrono](../receipts/audit_d6_preparation_20261007/README.md).

Quatre fichiers actifs, 73 constats ; clôtures cache/documents/cohorte conservées, capacités 256/64 de `0237` ouvertes.
Aucun GCP ni donnée sous licence dans cet audit. Natifs limités aux formats CPU synthétiques : deux points catalogue,
puis deux sondes G à huit points ; aucun benchmark. Les derniers contrôles sont des relectures et modèles Python.
