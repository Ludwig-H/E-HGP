# Audit Codex — état courant v12

7 octobre 2026. Base publiée **`af0c2ecd7`** ; intégration en cours, prototypes distingués dans les reçus.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité des états : [registre](CONSTATS.md).

**À intégrer ou corriger.**

- **Juges, `0018`** : propositions [CUDA](../receipts/audit_reponses_20261007/cuda_juge_proposition/README.md),
  [MES-P](../receipts/audit_reponses_20261007/mes_p_admission/README.md) et
  [D6](../receipts/audit_reponses_20261007/d6_admission_proposition/README.md) encore non intégrées.
  D6 requiert référence u21 et sorties complètes ; `device_open` proposé reste à qualifier sur GPU.
  [Livraison contre-jugée](../receipts/audit_reponses_20261007/livraison_juges/README.md) : G typé corrigé ;
  **juge CUDA encore permissif dans `8ba7d7287`**, correction requise avant adoption des temps.
- **T2-c** : [lecteur strict intégré au prototype](../receipts/audit_reponses_20261007/t2c_pilote_integration/README.md),
  21 témoins, trois campagnes, cinq auto-tests, 30 journaux réels. Plan dix tours et garde `rapport` confirmés.
  Défaut CLI encore huit et collecte C permissive : correctif proposé. C vient après G, dans d'autres processus ;
  leur somme indicative n'est ni C+G intégré ni FULL. Pas de campagne G4 à ce pin.
- **Catalogue hybride A″ livré `8ba7d7287`** : ≤32 sites/16 bits locaux sur appareil, sinon reprise CPU exacte ;
  [compteurs corrigés](../receipts/audit_reponses_20261007/cuda_compteurs_reponse/README.md).
  Variante CPU [B″ locale](../receipts/audit_reponses_20261007/cuda_b3_portes_locales/README.md) : 15 mutants (un par signal),
  ASan/UBSan 44/44, TSan 12/12, **CUDA et cache actif désactivés**. Ni GPU ni poison des blocs inactifs requalifiés.
- **T/M/V/R** : [preuves u21](../receipts/audit_tmv_traces_20261007/README.md),
  [profils 24/32 et patch `39622284`](../receipts/audit_tmv_profils_20261007/README.md) vérifiés : chacun 651 tests
  réussis + un sauté, neuf cas graines et un cas cibles v12. Mêmes coordonnées/vidages v11 ; ces MES-M0 ne reconstruisent
  pas C/G. Le patch inclut R et préserve les cinq portes G récentes. **Admission des offsets CSR encore à corriger.**
  `0105/0107` restent candidats à clôture après livraison ; `0212` clos au microbanc, portée produit à étendre.
- **Table S* / finition** : [conversion 4→16 octets à raccorder](../receipts/audit_reponses_20261007/gc_support_fusion/README.md).
  Proposition hôte sans transfert supplémentaire : +12 octets par boule au final, +16 au pic local avant arrondis du cache.
  Construction payée dans C, donc gain à juger sur C+G puis FULL.

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

Pistes détaillées : [jointure G-L5](../receipts/audit_reponses_20261007/g_l5_proposition/README.md),
[census/supports](../receipts/audit_g_pistes_20261007/README.md),
[finition/feuilles CPU](../receipts/audit_performance_20261007/README.md),
[petits nuages](../receipts/audit_reponses_20261007/session_h_mes_p/README.md),
[préparation D6 hors chrono](../receipts/audit_d6_preparation_20261007/README.md).

Quatre fichiers actifs, 73 constats ; clôtures cache/documents/cohorte conservées, capacités 256/64 de `0237` ouvertes.
Aucun GCP ni donnée sous licence dans cet audit. Natifs limités aux formats CPU synthétiques : deux points catalogue,
puis deux sondes G à huit points ; aucun benchmark. Les derniers contrôles sont des relectures et modèles Python.
