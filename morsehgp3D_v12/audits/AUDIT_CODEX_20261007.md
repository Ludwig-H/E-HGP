# Audit Codex — état courant v12

8 octobre 2026, 05:09 UTC, base publiée **`7bcf9665e`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Derniers temps G4 : session K, 100 ms non tenu.** [Contre-lecture FULL](../receipts/audit_reponses_20261008/session_k_full/README.md),
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
03:03:03 UTC ; [342 sources/configurations identiques à `c9ac60f20`](../receipts/audit_reponses_20261008/session_k_sources/README.md).
Snapshot sans hash binaire ni journal de compilation rapatrié ; aucun CPU·s par trame, aucun CPU K10.

Face aux mesures historiques v11 : GPU plus rapide, CPU plus lent ; comparaison descriptive, pas A/B apparié.
Catalogue CPU : 274–329 ms. Sur GPU, G : 44–57 ms ; T : 26–38 ms ; R : 12–15 ms.
Retirer R par passe laisse 116–148 ms médians : ce seul poste ne suffit pas à 100 ms.
[Campagnes L1/L2 préparées](../receipts/audit_reponses_20261008/session_l_preparation/README.md) : L1 lancée à
04:39:17 UTC ; à 04:44:40, aucun résultat ni arrêt rapatrié. K10 prévu à froid ; aucune empreinte FUL1 sur L2.

**Contre-lectures courantes et aide au développeur.**

- **CPU, catalogue** : finition déjà parallèle ; feuilles = 45–48 % de C.
  [Popcount, assembleur vérifié](../receipts/audit_reponses_20261008/cpu_popcount_asm/README.md) : cinq appels
  logiciels deviennent zéro ; avec `-mpopcnt`, assembleurs identiques. Deux nouveaux
  [patches séparés](../receipts/audit_reponses_20261008/cpu_live/README.md) : calcul symétrique des masques
  (2E→E unions/popcounts), retrait du garde Q2 déjà garanti par le générateur. Modèle : 17 628 cas,
  dix mutants, aucun tableau ajouté. Corps warp produit conservé ; qualification native et chronos attendus.
- **Mémoire du catalogue** : [réemploi du tri des positions pour S*](../receipts/audit_reponses_20261008/catalogue_radix_reuse/README.md)
  proposé après analyse des durées de vie : demande supprimée de 40n+1056t+1056 octets, t=max(1,ceil(n/1024)).
  Modèle : 227 appels et 92 interruptions ; ni allocation physique, ni pic, ni temps économisé qualifiés.
- **MES-B `9feadf927`** : [contre-lecture achevée](../receipts/audit_reponses_20261008/mes_b_memoire/README.md).
  Mur/sites nuls refusés, portes Python normal/−O et 24 mutants passent. Trois incohérences mémoire restent
  admises (capacité>pic, épinglé>pic hôte, pic suivant<usage précédent) ; témoins et patch fournis.
  B1 tolère explicitement les refus K5 ≥10 M sites. Les nouveaux champs mémoire ne figurent pas dans L1.
- **T2-d-A, recouvrement G/TMVR** : [concurrence](../receipts/audit_reponses_20261008/prelecture_t2d_t/README.md),
  [reprise](../receipts/audit_reponses_20261008/t2d_a_reprise/README.md) : `noexcept` et fin G corrigés ;
  pollution du compteur d'allocations par le digest signalée, test ensuite réécrit dans le prototype.
  [Pilote épinglé avant refonte](../receipts/audit_reponses_20261008/t2d_a_admission/README.md) : mur impossible
  ou cohorte d'identité tronquée permettent encore « adopte ». Corriger avant campagne ; T/M/V/R recouverts
  sont des fenêtres de tâches, pas une partition du mur. Schéma suivant à contre-lire explicitement.
- **T2-d-B, census G** : [garde resserrée et preuve](../receipts/audit_reponses_20261008/garde_census/README.md),
  [témoins de frontière](../receipts/audit_reponses_20261008/census_temoins/README.md) : 1 288 requêtes du modèle
  gardent résultats/parcours ; supports certifiés transportés par valeur dans le prototype.
  Proposition entière puis Welzl amorcé en préparation ; nouveau corps et gain G4 à qualifier.
- **T2-d-C, catalogue GPU** : [copie fusionnée relue](../receipts/audit_reponses_20261008/t2d_c_reprise/README.md) :
  staging adapté au plus grand segment, plancher forcé de 16 Mio corrigé ; ablation de l'anticipation complétée.
  Budget de la sonde catalogue encore commun ; deux bras conservent une réparation différente de celle annoncée.
  [Juge à renforcer](../receipts/audit_reponses_20261008/t2d_c_admission/README.md) : métadonnées incompatibles et
  mutant sans empreinte toujours admis aux fonctions épinglées. Aucun nouveau CUDA ni gain acquis.
- **R** : [raccourci des classes à cellule unique](../receipts/audit_reponses_20261008/registre_classe_unique_patch/README.md)
  fourni, non compilé, sans gain acquis ; comparer aussi les lignes/CSR, absentes de FUL1.
  Le développeur le garde pour après T2-d-A dans sa [réponse publiée](../receipts/developpement_20261008/reponse_audit_k_mes_b.md).

**Qualifications et portes restantes.** Gc/TMVR livrés : détails et limites au registre ; dernière qualification
TMVR u21, 716 portes et une sentinelle LiDAR sautée, 27 mutants, 685 contrôles du pic ; pas de transfert à T2-d.
[D6 rejoué](../receipts/audit_reponses_20261008/d6_session_k/README.md) : 126 sorties/630 passes admises ;
C u24/u32 ×1 : +0,04–1,04 %, G u32 : +4,3–11,2 %. ×2048 reste <2²⁹, sans précision physique nouvelle ;
[erratum accepté](../receipts/g4_fullk_20261008/ERRATUM_20261008.md). Seuil D6 produit <3 % ouvert.
[Contre-lecteur FULL strict](../receipts/audit_reponses_20261008/mes_full_contrelecture/README.md) :
53 corruptions refusées, K admis ; intégration au pilote promise avant sa prochaine session, pas encore livrée.
`0239` (_GLIBCXX_DEBUG), `0240` (historique non contrôlé, aucune sortie produit fausse démontrée), `0104`
(mutant de date Gc), T7, capacités 256/64, profils élargis et massif restent ouverts. `0105/0107` clos u21 ;
`0009` clos, `0008` ouvert. Toutes les preuves de clôture et réserves sont conservées au registre.

Quatre fichiers actifs, 75 constats ; détails dans `receipts/`. Worktrees d'audit désormais persistants sous
`/workspaces`. Aucun GCP ni donnée sous licence par cet audit ; contrôles récents Python, sources et assembleur
seul, sans exécution du moteur. Les qualifications perdues dans `/tmp` ne sont pas recréées par la sauvegarde des sources.
