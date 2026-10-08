# Audit Codex — état courant v12

8 octobre 2026, lecteur produit **`a2c2fccfd`**, mesures L1r **`ea62cd691`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**SemanticKITTI : session K ; contrat de 100 ms non tenu.**
[Contre-lecture FULL](../receipts/audit_reponses_20261008/session_k_full/README.md), u21/W48,
ng00–02 : 39 885 / 35 551 / 45 845 sites :

| Médiane chaude FULL, ms | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| GPU K5, 5 processus × 9 chaudes | **159,30** | **127,63** | **163,27** |
| CPU K5, 3 × 4 | **441,07** | **368,33** | **447,08** |
| GPU K10, 3 × 4, informatif | 793,44 | 603,34 | 715,31 |

**37 trames, six séquences : médiane 241,31 ms, maximum contractuel 467,92 ms** (185 passes chaudes).
610 passes admises au total, empreintes concordantes. Mur Cloud/index→T/M/V/R ; lecture, masque, validation, FUL1 et libération exclus. [Provenance et arrêt](../receipts/audit_reponses_20261008/session_k_provenance/README.md),
[342 sources identiques à `c9ac60f20`](../receipts/audit_reponses_20261008/session_k_sources/README.md) ;
ni hash binaire/journal de compilation rapatrié, ni CPU·s, ni CPU K10.
Face à la v11 : GPU plus rapide, CPU plus lent (historique non apparié).
R seul retiré laisse 116–148 ms médians si le reste reste inchangé.

**Nouveaux LiDAR massifs : L1r contre-validée**, u21/W48, FULL K5, une seule passe chaude par cas.
[L1 originale](../receipts/audit_reponses_20261008/session_l1_admission/README.md),
[seconde exécution distincte](../receipts/audit_reponses_20261008/session_l1r_admission/README.md) et
[récupération/arrêt vérifiés](../receipts/audit_reponses_20261008/session_l1_recuperation/README.md).

| Scène | Sites | GPU FULL K5, s |
| --- | ---: | ---: |
| Boreas n1 sans sol | 146 316 | 0,519 |
| Boreas n10 sans sol | 1 513 483 | **10,025** |
| Marseille sans sol | 2 465 285 | 10,955 |
| Scion sans sol | 3 439 371 | 41,951 |
| Meadow scan1 | 6 181 091 | 35,325 |

Boreas n10 sans sol **CPU 22,344 s**, FUL1 identique au GPU. Chaque série : 10 succès/8 refus mémoire,
18 passes complètes sur 30, dont huit chaudes. Deux K10 refusés ; B1/B2/B4 non tenus, B3 non évalué.
Six refus K5 ; succès à 6,71 M et refus à 5,20 M : aucun plafond universel en sites. Aucun gain causal déduit entre L1 et L1r. Sur les succès GPU, pic hôte à TMVR ;
Boreas n10 : C/G/TMVR=1,742/2,947/5,261 s. Catalogue seul insuffisant. [Errata proposés](../receipts/audit_reponses_20261008/l1r_documentation/README.md) : huit refus,
RSS en Gio, enveloppes temporelles ; poste mémoire des refus et loi de croissance non établis.

**Aide au développeur et vérifications restantes.**

- **CPU/catalogue** : finition déjà parallèle ; feuilles = 45–48 % de C.
  [Popcount en assembleur](../receipts/audit_reponses_20261008/cpu_popcount_asm/README.md),
  [deux patches CPU](../receipts/audit_reponses_20261008/cpu_live/README.md) : symétrie 2E→E,
  garde Q2 redondant retiré, modèle 17 628 cas/dix mutants. [Réemploi du tri pour S*](../receipts/audit_reponses_20261008/catalogue_radix_reuse/README.md) :
  demande retirée 40n+1056t+1056 octets, pas un pic mesuré. Qualification native et chronos attendus.
- **A, recouvrement G/TMVR** : `noexcept`, frontière de fin G et compteur d'allocations corrigés dans le prototype.
  [Schéma 902 relu](../receipts/audit_reponses_20261008/t2d_a_schema902/README.md) : fenêtres séparées du mur,
  mur impossible refusé ; cohorte d'identité tronquée encore adoptée, fin−G≠queue/mémoire absente admises.
  [Fenêtre des feuilles déplacée](../receipts/audit_reponses_20261008/t2d_a_fins/README.md) : sous-compte
  `foret_apres_g` ; [patch proposé](../receipts/audit_reponses_20261008/t2d_a_fenetre_patch/README.md).
  Mur FULL inchangé. 705 portes locales vertes ; G4 à qualifier.
- **B, census/proposition** : [garde](../receipts/audit_reponses_20261008/garde_census/README.md),
  [témoins de frontière](../receipts/audit_reponses_20261008/census_temoins/README.md),
  [proposition entière](../receipts/audit_reponses_20261008/t2d_b_proposition/README.md) : preuve et oracle rationnel
  sur 191 parties, bornes B≤32 ; support local distinct du global. [Traces locales](../receipts/audit_reponses_20261008/t2d_b_traces/README.md) :
  698 portes passées/une sautée, neuf résumés FUL1 égaux ; [39 mutants clos](../receipts/audit_reponses_20261008/t2d_b_mutants/README.md).
  [Pilote B](../receipts/audit_reponses_20261008/t2d_b_admission/README.md) : G seul décisif, mur incohérent admis ;
  littéral FULL `device` à corriger. Retirer l’inversion alternée équilibrerait la parité des positions des huit bras
  sur dix tours ; A/A informatif sans veto, protocole à fixer avant campagne. Aucun gain G4 qualifié.
- **C, catalogue GPU** : [suivi local](../receipts/audit_reponses_20261008/t2d_c_suivi/README.md) : 705 portes + une sautée,
  modules CUDA u21/u24 56/56 sans GPU, neuf paires FUL1 CPU. Lecteur : huit incohérences nouvellement refusées ;
  code3/configuration et cohorte encore permissifs. Variante de libération avant croissance : propriétaire cohérent
  sous libération réussie, capacités après refus différentes ; reprise sur le même contexte GPU à qualifier.
- **Lecteur FULL commun livré** : [a2 contre-jugé](../receipts/audit_reponses_20261008/lecteur_full_commun/README.md),
  trois portes normal/−O et 28 mutants passent. Cohérence mémoire, type du code et couples statut/raison
  restent à renforcer ; patch partiel fourni. L1/L1r ont leurs contre-lecteurs épinglés indépendants.
- **MES-C petits** : [cohorte incomplète déclarée tenue](../receipts/audit_reponses_20261008/mes_c_prelecture/README.md),
  correctif proposé avant campagne ; douze scénarios synthétiques.
- **T/K1** : [réutiliser les rangs déjà calculés par C](../receipts/audit_reponses_20261008/t_naissances_reutilisees/README.md)
  évite un tri XYZ ; preuve et modèle, export possédé de 4N octets à compter. Mesurer `births_ns` avant raccord.
- **R** : [raccourci proposé](../receipts/audit_reponses_20261008/registre_classe_unique_patch/README.md),
  non compilé, différé par le développeur après A ; comparer aussi les CSR absents de FUL1.

Autres statuts et preuves au registre, sans transfert des qualifications TMVR à T2-d.
Audit : Python, sources et assembleur ; aucun moteur, GCP ni donnée sous licence.
