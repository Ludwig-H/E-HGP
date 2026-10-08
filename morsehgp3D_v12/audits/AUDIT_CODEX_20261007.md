# Audit Codex — état courant v12

8 octobre 2026, base publiée **`bec107f7d`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Derniers temps G4 : FULL mesuré, 100 ms non tenu.** [Session K contre-jugée](../receipts/audit_reponses_20261008/session_k_full/README.md),
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
**Snapshot, sans hash binaire ni journal de compilation rapatrié**. Aucun CPU·s par trame.

Face aux mesures historiques v11 : GPU désormais plus rapide, CPU toujours plus lent ; comparaison descriptive,
pas A/B apparié. **Catalogue CPU : 274–329 ms**. Côté GPU, G coûte 44–57 ms, T 26–38 ms, R 12–15 ms.
Retirer comptablement R par passe laisse 116–148 ms médians : R seul ne suffit pas à 100 ms.
GPU u24/u32, petits nuages et millions restent à qualifier ; aucun CPU K10 joué dans K.

**Livraisons et qualifications.**

- **Gc `4df326cc8` + `10050a96e`** : [sources livrées vérifiées](../receipts/audit_reponses_20261008/gc_livraison/README.md).
  J valide index parallèle et file G-L7 ; G-L5 rejeté. CLI huit et collecte C ouverts.
- **TMVR `7398aed7d`** : [livraison identique au prototype](../receipts/audit_reponses_20261008/tmvr_livraison/README.md).
  MES-M0, neuf chaînes CPU/v11, W1/W8 et EMST ; 716 portes, une LiDAR sautée,
  [27 mutants](../receipts/audit_tmv_repo6_mutants_20261008/README.md), 685 contrôles du pic.
  Budget fini/cache et u24/u32 restent ouverts ; construction repo6 u24 interrompue.
- **D6 `0018/0207`** : [livraison `e37fd8935` vérifiée](../receipts/audit_reponses_20261008/d6_livraison_stricte/README.md),
  [K rejoué hors ligne](../receipts/audit_reponses_20261008/d6_session_k/README.md) :
  ancien lecteur → 66 alertes ; corrigé → 126 sorties/630 passes admises, ratios recalculés.
  C u24/u32 ×1 : +0,04–1,04 % ; G u32 : +4,3–11,2 %. ×2048 : C ×2,9, mais coordonnées **<2²⁹**, pas «32 bits pleins».
  Homothétie d'entiers déjà quantifiés, sans détail physique nouveau ; seuil D6 produit <3 % encore ouvert.

**Aide au développeur et prochains contrôles.**

- **T2-d actif** : [prélecture du recouvrement](../receipts/audit_reponses_20261008/prelecture_t2d_t/README.md) :
  Pool non réentrant, admission commune des coexistences, dépendances V/R et nouvelle enveloppe de temps à déclarer.
  [Protocole G](../receipts/audit_reponses_20261008/t2d_g_prelecture/README.md) : les deux bras sont désormais Gc ;
  séparer garde, index exact et changement déclaré de sauts.
  [Premiers corps A](../receipts/audit_reponses_20261008/prelecture_t2d_corps/README.md) : `open_session noexcept`
  peut terminer sur allocation ; numérotation doublée déjà corrigée, frontière `g_end` à préciser.
  [Flux C](../receipts/audit_reponses_20261008/t2d_c_prelecture/README.md) : lecture favorable, 13 tests Pool relus ;
  staging demandé à 16 Mio, allocation anticipée et quatrième levier à distinguer. CUDA exécuté : non.
- **CPU `0233/0234`** : [finition déjà parallèle et piste popcount](../receipts/audit_reponses_20261008/cpu_popcount/README.md).
  Feuilles = 45–48 % de C ; appels logiciels dans l'objet local, pas de preuve binaire K.
  Patch SWAR explicite depuis v11 proposé ; instructions puis gain CPU à qualifier.
- **MES-FULL `0018`** : [lacunes d’admission](../receipts/audit_reponses_20261008/mes_full_admission/README.md),
  pilote `c9ac60f20` présent dans K : GPU inconnu, incohérences et cohortes incomplètes admis.
  [Contre-lecteur strict](../receipts/audit_reponses_20261008/mes_full_contrelecture/README.md) :
  53 corruptions refusées, 38 processus/610 passes K admis ; verdict « non tenu ». Pilote produit encore à renforcer.
- **MES-B en construction `0018`** : [blocages reproduits et patch](../receipts/audit_reponses_20261008/mes_b_prelecture/README.md) :
  étiquette longue répétée → boucle ; refus K10 sans passe oublié → bilan tenu ; quatre corruptions JSON admises.
  Correctifs Python vérifiés ; aucun lancement massif par cet audit. CPU/RSS et budgets séparés relus.
- **R** : [classes à cellule unique](../receipts/audit_reponses_20261008/registre_classe_unique/README.md) :
  `q=d+1` autorise la copie des enfants sans recherche historique ni tri, mémoire temporaire réduite.
  Preuve et modèle ; sur les comptes ng00, au moins 78,81 % des requêtes K5 et 71,95 % K10 évitables.
  [Patch minimal fourni](../receipts/audit_reponses_20261008/registre_classe_unique_patch/README.md), non compilé,
  sans gain temps acquis. Comparer aussi les lignes/CSR de R :
  [FUL1](../src/tower/export_full.cpp) ne les encode pas ; la porte native `branches` les compare à la coupe ouverte.
- **Census G** : [garde resserrée proposée](../receipts/audit_reponses_20261008/garde_census/README.md),
  preuve MEB et témoin de débordement ; voies du patch conservées, promotions séparées.
  [Témoins de frontière](../receipts/audit_reponses_20261008/census_temoins/README.md) : 1 288 requêtes du modèle
  gardent résultats/parcours ; ≤4 supports certifiés du même Cloud à transporter. Intégration/G4 à qualifier.
Autres portes ouvertes : `0239` (_GLIBCXX_DEBUG non joué), `0240` (historique non certifié par le validateur,
aucune sortie produit fausse démontrée), `0104` (mutant de date adapté à Gc non joué). Preuves, propositions et
périmètres dans le [registre](CONSTATS.md). `0105/0107` clos sur u21 ; aucune extension automatique à `0240`.
`0009` clos ; `0008` ouvert. T7 natif, capacités 256/64, profils élargis et massif restent à qualifier.

Quatre fichiers actifs, 75 constats ; détails historiques dans `receipts/`. Aucun GCP lancé ni donnée sous licence
par cet audit. Contrôles récents : sources, archives et Python ; natifs antérieurs limités aux formats synthétiques.
