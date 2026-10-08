# Audit Codex — état courant v12

8 octobre 2026, reprise après coupure, base publiée **`403736300`**.
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

**Contre-lectures courantes et aide au développeur.**

- **CPU, catalogue** : finition déjà parallèle depuis `8ba7d7287` ; feuilles = 45–48 % de C.
  [Patch popcount explicite depuis v11](../receipts/audit_reponses_20261008/cpu_popcount/README.md),
  puis [assembleur vérifié](../receipts/audit_reponses_20261008/cpu_popcount_asm/README.md) : cinq appels logiciels
  deviennent zéro dans les deux wrappers génériques ; avec `-mpopcnt`, assembleurs identiques.
  Aucun gain temps ni objet produit corrigé qualifié. Cette piste concerne le catalogue, pas l'étage G.
- **MES-B `8da450ab7`** : [livraison contre-jugée](../receipts/audit_reponses_20261008/mes_b_livraison/README.md).
  Étiquettes, refus K10 et JSON corrigés ; tests Python normal/−O et 20 mutants passent.
  Résidu synthétique : mur nul admis puis `log(0)` dans B3 ; refuser ce mur avant les statistiques.
  B1 tolère explicitement les refus K5 ≥10 M sites : « tenu » ne signifie pas achèvement de toutes ces scènes.
  CPU/RSS et budgets séparés ajoutés ; aucune mesure massive nouvelle contre-certifiée.
- **T2-d-A, recouvrement G/TMVR** : [conditions de concurrence](../receipts/audit_reponses_20261008/prelecture_t2d_t/README.md),
  [copie reprise sur 8da](../receipts/audit_reponses_20261008/t2d_a_reprise/README.md) : `noexcept` retiré et fin G
  mesurée avant les feuilles. Test mémoire récupéré : le digest pollue son compteur ; patch ciblé fourni.
  Nouvelle qualification native attendue. T/M/V/R recouverts sont des sommes de fenêtres, pas du temps CPU.
- **T2-d-B, census G** : [garde resserrée et preuve](../receipts/audit_reponses_20261008/garde_census/README.md),
  [témoins de frontière](../receipts/audit_reponses_20261008/census_temoins/README.md) : 1 288 requêtes du modèle
  gardent résultats/parcours ; supports certifiés transportés par valeur dans le prototype.
  Proposition entière puis Welzl amorcé en préparation ; nouveau corps et gain G4 à qualifier.
- **T2-d-C, catalogue GPU** : [prélecture du flux](../receipts/audit_reponses_20261008/t2d_c_prelecture/README.md) :
  staging 16 Mio, mémoire anticipée et réparation compacte à compter. Treize tests Pool relus ; aucun CUDA exécuté.
  [Juge à renforcer](../receipts/audit_reponses_20261008/t2d_c_admission/README.md) : métadonnées incompatibles admises,
  mutant code 0 sans empreinte déclaré tué ; témoins rejoués après coupure et patch partiel fournis.
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
