# Audit Codex — état courant v12

8 octobre 2026. Base publiée **`0a5ebf29f`** ; prototypes distingués des livraisons.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Nouvelles mesures G4 contre-vérifiées.** [Session I](../receipts/audit_reponses_20261008/session_i_catalogue/README.md),
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

**Intégration suivie.**

- **Juges, `0018`** : [G typé](../receipts/audit_reponses_20261007/livraison_juges/README.md),
  [CUDA strict](../receipts/audit_reponses_20261008/cuda_juge_livraison/README.md),
  [MES-P](../receipts/audit_mes_p_livraison_20261008/README.md) et
  [D6](../receipts/audit_reponses_20261008/d6_compatibilite/README.md) livrés et contre-jugés.
  CUDA strict effectivement utilisé en I ; **plan D6 encore ouvert** : u21, doublons, combinaison vide.
- **T2-c** : [lecteur strict du prototype](../receipts/audit_reponses_20261007/t2c_pilote_integration/README.md),
  21 témoins, trois campagnes, cinq auto-tests, 30 journaux réels. Défaut CLI huit à corriger pour la règle dix.
  Collecte C séparée retirée ; aucune nouvelle campagne G4 de Gc acquise.
- **T/M/V/R** : [preuves antérieures](../receipts/audit_tmv_profils_20261007/README.md) conservées.
  [Patch `6f0643ac`](../receipts/audit_tmvr_admission_20261008/README.md) : admission CSR corrigée,
  `4 A + 8 Σ(R_k+1)`, garde avant allocation, porte du pic et mutant ciblé sans cache.
  Qualification repo5 en cours ; `0105/0107` attendent la livraison ; `0212` clos au microbanc seulement.
- **Gc `45976be8`** : [table 16 octets retirée](../receipts/audit_reponses_20261007/gc_support_domain_delta/README.md),
  catalogue A/B conservé ; garde de tous les SiteIdx proposée, aucun défaut FULL valide déduit.
  [Nouvelles portes](../receipts/audit_reponses_20261007/gc_rebase_portes/README.md) : 675 rapides + 6 LiDAR,
  680 distinctes, sans saut, CPU/u21 ; nouveaux mutants en cours. Anciennes mesures non transférables.

**Aide mathématique et coût.** Gc : recherche exacte logarithmique sous collisions ; gain du hash par cellule
conditionnel, identité G liée à la politique. Les preuves détaillées restent liées au registre.
[Témoin T7](../receipts/audit_reponses_20261007/t7_cercle25/README.md) prêt ; porte native absente.
En I, ng02 répare 9/16 éléments mais rapatrie 22,5/87,7 Mo de tableaux complets à K5/K10 :
compactage des chaînes entières à étudier après G, sans gain temps présumé.

Quatre fichiers actifs, 73 constats ; clôtures cache/documents/cohorte conservées, capacités 256/64 de `0237` ouvertes.
Aucun GCP lancé ni donnée sous licence dans cet audit. Natifs antérieurs limités aux formats CPU synthétiques
(deux points catalogue, deux sondes G à huit points). Derniers contrôles : archives et Python, aucun benchmark.
