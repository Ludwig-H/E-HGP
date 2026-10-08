# Audit Codex — état courant v12

8 octobre 2026, base publiée **`99afc3d65`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Derniers temps vérifiés : session K ; contrat de 100 ms non tenu.**
[Contre-lecture FULL](../receipts/audit_reponses_20261008/session_k_full/README.md), u21/W48,
trames ng00–02 de 39 885 / 35 551 / 45 845 sites :

| Médiane chaude FULL, ms | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| GPU K5, 5 processus × 9 chaudes | **159,30** | **127,63** | **163,27** |
| CPU K5, 3 × 4 | **441,07** | **368,33** | **447,08** |
| GPU K10, 3 × 4, informatif | 793,44 | 603,34 | 715,31 |

**37 trames, six séquences : médiane 241,31 ms, maximum contractuel 467,92 ms** (185 passes chaudes).
610 passes admises au total, empreintes concordantes. Mur de Cloud/index à T/M/V/R ; lecture,
segmentation, validation, FUL1 et libération hors mur. [Provenance et arrêt](../receipts/audit_reponses_20261008/session_k_provenance/README.md),
[342 sources identiques à `c9ac60f20`](../receipts/audit_reponses_20261008/session_k_sources/README.md) ;
ni hash binaire/journal de compilation rapatrié, ni CPU·s, ni CPU K10.
Face à la v11 : GPU plus rapide, CPU plus lent, comparaison historique non appariée.
Catalogue CPU 274–329 ms ; sur GPU G 44–57 ms, T 26–38 ms, R 12–15 ms.
Retirer R seul laisse 116–148 ms médians, sous hypothèse d'aval inchangé.

**L1 : aucun nouveau temps admis.** [Diagnostic](../receipts/audit_reponses_20261008/session_l1_diagnostic/README.md) :
worker code0, récupération refusée par la réserve disque locale, arrêt TERMINATED certifié ; aucun brut
au relevé 05:10:53. [Contre-lecteur prêt](../receipts/audit_reponses_20261008/session_l_contrelecture/README.md),
cohortes synthétiques seulement. [Plans L1/L2](../receipts/audit_reponses_20261008/session_l_preparation/README.md) :
K10 à froid, pas de FUL1 sur L2 ; source L2 à confirmer.

**Aide au développeur et vérifications restantes.**

- **CPU/catalogue** : finition déjà parallèle ; feuilles = 45–48 % de C.
  [Popcount en assembleur](../receipts/audit_reponses_20261008/cpu_popcount_asm/README.md),
  [deux patches CPU](../receipts/audit_reponses_20261008/cpu_live/README.md) : symétrie 2E→E,
  garde Q2 redondant retiré, modèle 17 628 cas/dix mutants. [Réemploi du tri pour S*](../receipts/audit_reponses_20261008/catalogue_radix_reuse/README.md) :
  demande retirée 40n+1056t+1056 octets, pas un pic mesuré. Qualification native et chronos attendus.
- **A, recouvrement G/TMVR** : `noexcept`, frontière de fin G et compteur d'allocations corrigés dans le prototype.
  [Schéma 902 relu](../receipts/audit_reponses_20261008/t2d_a_schema902/README.md) : fenêtres séparées du mur,
  mur impossible refusé ; cohorte d'identité tronquée encore adoptée, fin−G≠queue/mémoire absente admises.
  Journal local : 705 portes vertes, sans preuve G4 ni chaîne complète sources→compilation.
- **B, census/proposition** : [garde](../receipts/audit_reponses_20261008/garde_census/README.md),
  [témoins de frontière](../receipts/audit_reponses_20261008/census_temoins/README.md),
  [proposition entière](../receipts/audit_reponses_20261008/t2d_b_proposition/README.md) : preuve et oracle rationnel
  sur 191 parties, bornes B≤32 ; support local distinct du global. [Traces locales](../receipts/audit_reponses_20261008/t2d_b_traces/README.md) :
  698 portes passées/une sautée, neuf résumés FUL1 égaux, 22 mutants index ; num/tour encore non clos.
  [Pilote B](../receipts/audit_reponses_20261008/t2d_b_admission/README.md) : G seul décisif, mur incohérent admis ;
  littéral FULL `device` à corriger. Retirer l’inversion alternée équilibrerait la parité des positions des huit bras
  sur dix tours ; A/A informatif sans veto, protocole à fixer avant campagne. Aucun gain G4 qualifié.
- **C, catalogue GPU** : [admission réécrite](../receipts/audit_reponses_20261008/t2d_c_admission_reprise/README.md),
  39 cas passent ; mutant code3 avec cause/configuration incompatible encore admis.
  [Budgets/identité locale](../receipts/audit_reponses_20261008/t2d_c_budgets/README.md) : neuf paires concordent ;
  pics du modèle ≠ allocations CUDA, reprise GPU après refus à couvrir. Aucun gain CUDA acquis.
- **MES-B `9feadf927`** : [24 mutants et portes Python vérifiés](../receipts/audit_reponses_20261008/mes_b_memoire/README.md),
  mur/sites nuls corrigés ; trois incohérences mémoire encore admises, patch proposé. B1 tolère les refus K5
  ≥10 M sites. Le nouveau schéma mémoire n'est pas celui de L1.
- **R** : [raccourci proposé](../receipts/audit_reponses_20261008/registre_classe_unique_patch/README.md),
  non compilé, différé par le développeur après A ; comparer aussi les CSR absents de FUL1.

**Portées conservées au registre.** TMVR u21 : 716 portes, une sautée, 27 mutants, 685 contrôles du pic,
sans transfert à T2-d. [D6](../receipts/audit_reponses_20261008/d6_session_k/README.md) : erratum accepté,
seuil produit <3 % ouvert. [Lecteur FULL strict](../receipts/audit_reponses_20261008/mes_full_contrelecture/README.md) :
53 corruptions refusées, intégration au pilote attendue. `0239`, `0240`, `0104`, T7, capacités 256/64,
profils élargis et massif ouverts ; `0105/0107` clos u21, `0009` clos, `0008` ouvert.

Quatre fichiers actifs, 75 constats ; reçus séparés, worktrees persistants. Audit récent : Python, sources et
assembleur seulement ; aucun moteur, GCP ni donnée sous licence. Sources sauvées ≠ qualifications restaurées.
