# Audit Codex — état courant v12

8 octobre 2026. Mesures K `c9ac60f20`, L1r `ea62cd691`, L2 `a2c2fccfd` ; pilote MES-C `83ed7620d`.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**SemanticKITTI : contrat de 100 ms non tenu.** [Session K contre-validée](../receipts/audit_reponses_20261008/session_k_full/README.md),
u21/W48, ng00–02 : 39 885 / 35 551 / 45 845 sites.

| Médiane chaude FULL, ms | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| GPU K5, 5 processus × 9 chaudes | **159,30** | **127,63** | **163,27** |
| CPU K5, 3 × 4 | **441,07** | **368,33** | **447,08** |
| GPU K10, 3 × 4, informatif | 793,44 | 603,34 | 715,31 |

**37 trames, six séquences : médiane 241,31 ms, maximum contractuel 467,92 ms** (185 passes chaudes).
610 passes admises, empreintes concordantes. Mur Cloud/index→T/M/V/R ; lecture, masque, validation,
FUL1 et libération exclus. [342 sources identiques au pin](../receipts/audit_reponses_20261008/session_k_sources/README.md),
[arrêt vérifié](../receipts/audit_reponses_20261008/session_k_provenance/README.md) ; hash binaire/journal de compilation,
CPU·s et CPU K10 absents. Face à la v11 : GPU plus rapide, CPU plus lent (historique non apparié).
R seul retiré laisse 116–148 ms médians si le reste reste inchangé.

**LiDAR massifs : derniers murs FULL K5, u21/W48.** L1r : une seule passe chaude par ligne ci-dessous ;
L2 : une seule passe initiale, donc froide. Aucun gain causal entre L1 et L1r.

| Scène | Sites | GPU, s | CPU, s | Prise |
| --- | ---: | ---: | ---: | --- |
| Boreas n1 sans sol | 146 316 | 0,519 | — | L1r chaude |
| Boreas n10 sans sol | 1 513 483 | **10,025** | **22,344** | L1r chaude |
| Marseille sans sol | 2 465 285 | 10,955 | — | L1r chaude |
| Scion sans sol | 3 439 371 | 41,951 | — | L1r chaude |
| Meadow scan1 | 6 181 091 | 35,325 | — | L1r chaude |
| Paris sans sol | 9 111 422 | — | **112,866** | L2 froide |
| Paris brut | 14 551 520 | — | **165,789** | L2 froide |

[L1r admise](../receipts/audit_reponses_20261008/session_l1r_admission/README.md) : 10 succès/8 refus mémoire,
18/30 passes dont huit chaudes ; B1/B2/B4 non tenus, B3 non évalué. Boreas n10 CPU/GPU : FUL1 identique.
Succès à 6,71 M et refus à 5,20 M : aucun plafond universel en sites.
[L2 admise](../receipts/audit_reponses_20261008/session_l2_admission/README.md) : deux succès CPU, trois refus ;
ETH3D 16,83 M `wide_leaf`, Lyon 24,02/32,41 M `memory_budget`. Aucun chaud, GPU, K10 ni digest FULL.
B1–B4 non évalués. [Provenance/arrêt L2](../receipts/audit_reponses_20261008/session_l2_provenance/README.md).
[Errata L1r](../receipts/audit_reponses_20261008/l1r_documentation/README.md) et
[L2](../receipts/audit_reponses_20261008/session_l2_documentation/README.md) proposés : conversions RSS, enveloppes
chronométriques, poste des refus et projections ; les chronos réels restent inchangés.

**Aide au développeur — priorités ouvertes.**

- **CPU/C** : feuilles = 45–48 % du catalogue CPU. [Symétrie des paires et garde redondante](../receipts/audit_reponses_20261008/cpu_live/README.md),
  [popcount](../receipts/audit_reponses_20261008/cpu_popcount_asm/README.md), [réemploi du tri pour S*](../receipts/audit_reponses_20261008/catalogue_radix_reuse/README.md) :
  propositions sans gain natif qualifié. Finition déjà parallèle.
- **T/K1** : [réutiliser les rangs C](../receipts/audit_reponses_20261008/t_naissances_reutilisees/README.md) évite un tri XYZ.
  Preuve et modèle ; export possédé de 4N octets à compter, `births_ns` à mesurer avant raccord.
- **A** : [schéma 902](../receipts/audit_reponses_20261008/t2d_a_schema902/README.md), cohorte/queue/mémoire à renforcer ;
  [patch des instants des feuilles](../receipts/audit_reponses_20261008/t2d_a_fenetre_patch/README.md) proposé, sans modifier le mur FULL.
- **B** : [proposition entière prouvée](../receipts/audit_reponses_20261008/t2d_b_proposition/README.md), bornes B≤32 ;
  [698 portes + une sautée](../receipts/audit_reponses_20261008/t2d_b_traces/README.md), [39 mutants clos](../receipts/audit_reponses_20261008/t2d_b_mutants/README.md).
  [Pilote](../receipts/audit_reponses_20261008/t2d_b_admission/README.md) : mur G, FULL device, A/A et ordre des bras à requalifier après correction.
- **C** : [suivi local](../receipts/audit_reponses_20261008/t2d_c_suivi/README.md), 705 portes + une sautée, modules CUDA u21/u24
  56/56 sans GPU, neuf paires FUL1 CPU. Huit gardes ajoutées ; refus/cohorte encore permissifs.
  Libération avant croissance : capacités après refus modifiées ; reprise sur le même contexte GPU à qualifier.
- **Lecteurs FULL/MES-C** : [lecteur commun a2](../receipts/audit_reponses_20261008/lecteur_full_commun/README.md), types/mémoire/raisons
  encore permissifs ; [MES-C livré](../receipts/audit_reponses_20261008/mes_c_livraison/README.md), cohorte incomplète tenue,
  patch avec porte adaptée proposé. Session MES-C en cours de suivi, aucun résultat admis ici.
- **R** : [raccourci classe unique](../receipts/audit_reponses_20261008/registre_classe_unique_patch/README.md) proposé,
  différé après A ; comparer aussi les CSR absents de FUL1.

[SHA matériel](../receipts/audit_reponses_20261008/sha256_voies/README.md) : trace de 136 contrôles positifs,
accélération de l'empreinte hors mur ; aucun gain FULL déduit. Autres statuts et preuves au registre.
Audit : Python, sources et assembleur ; aucun moteur, GCP ni donnée sous licence.
