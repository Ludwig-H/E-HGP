# Audit Codex — état courant v12

8 octobre 2026, 05:13 UTC, base publiée **`0347e3675`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Derniers temps G4 : session K, 100 ms non tenu.** [Contre-lecture FULL](../receipts/audit_reponses_20261008/session_k_full/README.md),
u21, W48, trames ng00–02 de 39 885 / 35 551 / 45 845 sites :

| Médiane chaude FULL, ms | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| GPU K5, 5 processus × 9 chaudes | **159,30** | **127,63** | **163,27** |
| CPU K5, 3 × 4 | **441,07** | **368,33** | **447,08** |
| GPU K10, 3 × 4, informatif | 793,44 | 603,34 | 715,31 |

**37 trames de six séquences : médiane 241,31 ms, maximum contractuel 467,92 ms**, 185 passes chaudes,
33 179–99 099 sites. Aucune trame sous le maximum de 100 ms. Les 610 passes et leurs empreintes concordent.
Mur de Cloud/index à T/M/V/R ; segmentation, lecture, validation, FUL1 et libération hors mur.
[Sources/commandes et arrêt ciblé vérifiés](../receipts/audit_reponses_20261008/session_k_provenance/README.md),
03:03:03 UTC ; [342 sources/configurations identiques à `c9ac60f20`](../receipts/audit_reponses_20261008/session_k_sources/README.md).
K : ni hash binaire/journal de compilation rapatrié, ni CPU·s, ni CPU K10.

Face aux mesures historiques v11 : GPU plus rapide, CPU plus lent ; comparaison descriptive, pas A/B apparié.
Catalogue CPU : 274–329 ms. Sur GPU, G : 44–57 ms ; T : 26–38 ms ; R : 12–15 ms.
Retirer R par passe laisse 116–148 ms médians : ce seul poste ne suffit pas à 100 ms.
[L1](../receipts/audit_reponses_20261008/session_l1_diagnostic/README.md) : worker code 0, mais récupération
refusée par la réserve disque locale ; arrêt TERMINATED certifié. Aucun brut disponible au relevé 05:10:53.
[Plans L1/L2](../receipts/audit_reponses_20261008/session_l_preparation/README.md) : K10 à froid, pas de FUL1 sur L2.

**Contre-lectures courantes et aide au développeur.**

- **CPU, catalogue** : finition déjà parallèle ; feuilles = 45–48 % de C.
  [Popcount vérifié en assembleur](../receipts/audit_reponses_20261008/cpu_popcount_asm/README.md) ; deux nouveaux
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
  [Nouveau schéma 902 relu](../receipts/audit_reponses_20261008/t2d_a_schema902/README.md) : mur impossible
  désormais refusé, fenêtres séparées ; cohorte d'identité tronquée encore « adopte », fin−G≠queue et mémoire
  absente admises. Journal local clos : 705 portes vertes, sans preuve G4 ni chaîne complète de compilation.
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
TMVR u21 : 716 portes, une sautée, 27 mutants, 685 contrôles du pic ; pas de transfert à T2-d.
[D6 rejoué](../receipts/audit_reponses_20261008/d6_session_k/README.md) : 126 sorties/630 passes admises ;
C u24/u32 ×1 : +0,04–1,04 %, G u32 : +4,3–11,2 %. ×2048 reste <2²⁹, sans précision physique nouvelle ;
[erratum accepté](../receipts/g4_fullk_20261008/ERRATUM_20261008.md). Seuil D6 produit <3 % ouvert.
[Contre-lecteur FULL strict](../receipts/audit_reponses_20261008/mes_full_contrelecture/README.md) :
53 corruptions refusées, K admis ; intégration au pilote promise avant sa prochaine session, pas encore livrée.
Restent ouverts : `0239`, `0240`, `0104`, T7, capacités 256/64, profils élargis et massif ; `0105/0107` clos u21,
`0009` clos, `0008` ouvert. Portées et preuves de clôture au registre.

Quatre fichiers actifs, 75 constats ; reçus séparés, worktrees persistants. Audit récent : Python, sources et
assembleur seulement, aucun moteur, GCP ni donnée sous licence. Sources sauvées ≠ qualifications restaurées.
