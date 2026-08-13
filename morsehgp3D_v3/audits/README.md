# Index des audits MorseHGP3D v3

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cet index est volontairement réduit aux autorités encore utiles. Les snapshots
historiques conservés sont explicitement étiquetés; aucun statut ancien n'est
recopié dans les documents live. Une référence de prototype vers un fichier
supprimé est un défaut documentaire à inventorier dans l'audit courant, pas une
autorité ressuscitée. Un titre, un message de commit ou un CTest vert ne vaut
jamais réception.

## Verdict live

- [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) est l'unique verdict mutable :
  il distingue le `HEAD`, le delta éventuel du worktree, les empreintes utiles,
  les contre-exemples, les tests qualifiables et les portes ouvertes.
- [`AUDIT_CONTRE_COMPTEUR_FENETRE_32589AD_20260813.md`](AUDIT_CONTRE_COMPTEUR_FENETRE_32589AD_20260813.md)
  réfute le faux `ProjectiveWindowCounter` du pin : `sum_N` vaut exactement
  deux fois la masse q2 résiduelle et sa pente n'est pas gatée. Il définit la
  vraie fenêtre owner-dirigée `N_q(a)`, répond à la question 48/432 et remet le
  reporter q4, son ledger et ses mesures physiques à Claude. Le P0 emploie 48
  chambres, puis raffine seulement les chambres ouvertes dans leurs neuf
  sous-cellules ; aucun `s` n'est encore choisi.
- [`AUDIT_DIRECTIVE_BNODE_PROJECTIF_ET_ARRET_CLIMB_75F16DB_20260813.md`](AUDIT_DIRECTIVE_BNODE_PROJECTIF_ET_ARRET_CLIMB_75F16DB_20260813.md)
  clôt l'ablation `climb`, qui omet sa feuille localisée, puis donne le
  classifieur uniforme exact d'un triple projectif sur un `BNode` : trois
  formes coniques et la quadratique séparable de Farkas. Il borne les largeurs,
  corrige le claim « six formes » et fixe ABI, fixtures et ordre du compteur.
- [`AUDIT_REPONSE_FOURCHE_SOURCE_CENTRAL_VWAVE_DBA8961_20260813.md`](AUDIT_REPONSE_FOURCHE_SOURCE_CENTRAL_VWAVE_DBA8961_20260813.md)
  répond à la question décisive de Claude : la source est factorisée et
  sortie-sensible, jamais par PairId. Il corrige la fausse colonne de masse,
  réfute la loi d'inflation discrète et remplace fenêtre/heap par
  `Central-VWave`, complète pour le masque central. Il fournit aussi le
  classifieur carrier par marges exactes, le join global `QueryTree×PointTree`,
  le contre-audit de la fenêtre projective, le raccord shallow q4 et les
  conditions manquantes à la preuve du WSPD Patricia.
- [`AUDIT_REPONSE_FOURCHE_SOURCE_AF08B0E_20260813.md`](AUDIT_REPONSE_FOURCHE_SOURCE_AF08B0E_20260813.md)
  propose la vraie source après le certificat central : groupes projectifs,
  fenêtre complète de co-sommets par ancre, shallow local éphémère, RLE par
  `BallKey` puis census global. Son lemme ponctuel est admis ; la fermeture
  d'un span entier, la construction factorisée des crédits et les pentes de
  `ProjectiveWindowCounter-v0` restent à recevoir conformément au contre-audit
  précédent.
- [`AUDIT_DEBLOCAGE_WSPD_PREFIX_CARRIERS_20260813.md`](AUDIT_DEBLOCAGE_WSPD_PREFIX_CARRIERS_20260813.md)
  remet à Claude le prochain jalon falsifiable : WSPD entière/canonique,
  classification des seuls terminaux par masque de lanes, cœur central et
  corridor d'ordre, puis vrai raccord q3/q4 par carriers. Il tranche le budget
  de profondeur comme quantum de scheduling, jamais comme cap sémantique.
- [`AUDIT_REPONSE_CLAUDE_DOUBLE_COEUR_RF_GPU_P0_A7F061B_20260813.md`](AUDIT_REPONSE_CLAUDE_DOUBLE_COEUR_RF_GPU_P0_A7F061B_20260813.md)
  répond aux deux questions de Claude, corrige le faux juge AABB du cœur q3/q4
  et réduit la prochaine implémentation à `RF-GPU-P0` : banque Morton bornée,
  recertification commune et compactage du résiduel avant corridors/carriers.
  Il contre-audite aussi le pin `a7f061b` et son worktree WSPD sans toucher au
  logiciel.
- [`AUDIT_REPONSE_BANQUE_MORTON_360EA7C_20260813.md`](AUDIT_REPONSE_BANQUE_MORTON_360EA7C_20260813.md)
  répond aux dernières questions de Claude : repli q2 exact
  `Hmin_singleton>0` en douze produits `i64`, refus de `s=8` global,
  raffinement local guidé par `Vbest`, héritage des preuves et réexamen des
  endpoints relatifs. Il réfute aussi l'encodeur Morton 2D du pin.
- [`AUDIT_DIRECTIVE_JOIN_PERSISTANT_WSPD_90AA941_20260813.md`](AUDIT_DIRECTIVE_JOIN_PERSISTANT_WSPD_90AA941_20260813.md)
  spécifie le join persistant après le tape WSPD : masque central partagé,
  proposition Morton puis range-report batché, split local `A/B`, arène SoA et
  handoff exact. C'est une directive d'implémentation hors claim, pas une
  réception logicielle.
- [`AUDIT_DIRECTIVE_DVT_CWAVE_4F4B463_20260813.md`](AUDIT_DIRECTIVE_DVT_CWAVE_4F4B463_20260813.md)
  ferme l'ordonnance remise à Claude : microkernel P0 sans file, puis une seule
  wavefront `C` fondée sur `D,V,T` qui produit témoins centraux, lentille et
  carriers aigus. Il corrige l'héritage des échecs de certificateur et réduit
  q4 à la relation factorisée `Acute×Lens`, sans boucle sur toutes les paires.
- [`AUDIT_REPONSE_OWNER_SHARD_P0_81D24D0_20260813.md`](AUDIT_REPONSE_OWNER_SHARD_P0_81D24D0_20260813.md)
  répond aux quatre questions live de Claude : impossibilité d'un owner porté
  par une paire nue, owner-shard intensionnel et join tardif, repli q2
  `Hmin_singleton` sans produits larges, requalification de la rampe scanline à
  emprise canonique et maintien explicite d'une WSPD L-infini. Il contre-audite
  aussi les digests FNV présentés comme identités et borne le nouvel oracle
  exhaustif à sa route réellement parcourue.

Le résumé est [`../README.md`](../README.md) et l'architecture durable est
[`../PROPOSITION.md`](../PROPOSITION.md). Ils peuvent conserver un résultat
historique explicitement pincé ou une conclusion d'architecture durable, mais
le statut logiciel du successeur appartient au verdict live. Les spécifications
et le registre des preuves sous `docs/` restent supérieurs.

## Snapshots et preuves conservés

| objet | portée exacte |
| --- | --- |
| [`AUDIT_Q2_SELFJOIN_8A39C53.md`](AUDIT_Q2_SELFJOIN_8A39C53.md) | preuve locale q2, réfutation du différentiel compensable de `8a39c53` et profil de coût du snapshot; tout successeur est jugé dans l'audit courant |
| [`AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md`](AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md) | contre-exemples du sidecar `cbac109` et contrat de frontière; le statut du successeur est uniquement live |
| [`AUDIT_JUNG_ANCHOR_389A742.md`](AUDIT_JUNG_ANCHOR_389A742.md) | contre-fixture permanente à une ancre de Jung insuffisamment certifiée |
| [`NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md`](NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md) | preuve des certificats cœur/profondeur q3/q4, hypothèses, égalités fail-open et limites industrielles |
| [`NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md`](NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md) | preuve du certificat de couverture du disque par groupes disjoints de trois témoins au plus, décision exacte et limites de complexité |
| [`AUDIT_DEGRE_GABRIEL_KISSING_SMAX11_20260811.md`](AUDIT_DEGRE_GABRIEL_KISSING_SMAX11_20260811.md) | réfutation exacte de toute borne de degré Gabriel par le kissing number ou `smax`; deux constructions u16 aux rangs 2 et 11, baseline de Poisson et conséquences industrielles; statut logiciel exclusivement dans l'audit live |
| [`AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md`](AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md) | diagnostic de la proposition 6 et du théorème 5, réparation exacte `G_k^+` par étoiles silencieuses, MSF corrigé, limites exhaustives et route conditionnelle `directes + gateways + carriers` pour le H0 horizontal normalisé |
| [`AUDIT_REPONSES_ROUTE_SPARSE_GABRIEL_20260812.md`](AUDIT_REPONSES_ROUTE_SPARSE_GABRIEL_20260812.md) | réponses Q0--Q6 : supports proposés distincts des cofaces directes, extra-shell distincte d'un support multiple, niveaux fixes 384/256 bits selon la représentation, resolver, MSF et contrat horizontal normalisé |
| [`AUDIT_REPONSES_SOURCE_FRONT_INVERSE_20260812.md`](AUDIT_REPONSES_SOURCE_FRONT_INVERSE_20260812.md) | réponses Q1--Q6 sur le front inverse : Source S générative mais non bijective, réfutation du graphe original limité aux sorties auto-centrées et à deux transitions, absence de coût sortie-sensible, quotient saturé exact et réduction Yao-1 de `k=1`; la connectivité shallow conditionnelle du vrai 1-squelette relevé avec transits n'est pas réfutée |
| [`AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md`](AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md) | correction de l'audit de volume, famille u16 séparant exactement arrangement relevé quadratique et Source S linéaire, premier croisement exact à recevoir, plafonds propriétaires et fixtures montrant que la projection ne conserve pas le niveau |
| [`NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md`](NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md) | théorème global et invariant pool-relative, budgets `h`, scores affines i64, premier RLE `SupportKey`, variantes explicites BallKey-first et SupportKey-first, axe q4 par face quelconque et certificat local d'expansion; parcimonie non prouvée, gates avant G4 |
| [`AUDIT_DEBLOCAGE_GPU_SUPPORTKEY_TOP12_20260812.md`](AUDIT_DEBLOCAGE_GPU_SUPPORTKEY_TOP12_20260812.md) | réponse industrielle aux vingt-quatre millions de supports : minimum auto-centré distinct d'une face shallow arbitraire, premier RLE avant géométrie, layout `6,33 Go` sous bijection dense reçue, top-`(12-q)` hors support minimal, fast path `E=U` et side queue pour toute extra-shell |
| [`AUDIT_CONTRE_AUDIT_407D4D1_SENTINELLE_HORS_SUPPORT_20260812.md`](AUDIT_CONTRE_AUDIT_407D4D1_SENTINELLE_HORS_SUPPORT_20260812.md) | contre-audit pincé du parallélisme `407d4d1` et du delta `UniqueKeyReceipt-v1` : multiplicité encore fausse, cap/HWM non reçus, rétraction de la minimalité top-12, census d'enveloppe et owner génératif exact-once; fermeture G4 ciblée certifiée |
| [`AUDIT_VERROU_MATHEMATIQUE_FRONT_JUNG_H0_GPU_20260812.md`](AUDIT_VERROU_MATHEMATIQUE_FRONT_JUNG_H0_GPU_20260812.md) | prise de recul sur le verrou 50 k : borne directionnelle contrainte, MSF seulement post-découverte, front de Jung coalescé attendu à `141,18n` sous Poisson, réduction q3/q4 aux neuf niveaux hors ancre, census direct et sweep q4 1D; le dual-tree actuel est rouge et `eight_clusters` reste la première falsification |
| [`NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_ENVELOPPE_20260812.md`](NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_ENVELOPPE_20260812.md) | proposition du propriétaire par arête maximale, enveloppe affine du disque de Jung et source exacte-une-fois; ses angles décimaux et son lemme de chambre sont corrigés par le contre-audit live |
| [`NOTE_CLAUDE_REPRISE_LOCALITE_CHAMBRE_FRONT_JUNG_20260812.md`](NOTE_CLAUDE_REPRISE_LOCALITE_CHAMBRE_FRONT_JUNG_20260812.md) | reprise de Claude sur les chambres et coupures angulaires; note de travail non reçue, avec frontières strictes et constantes algébriques à conserver plutôt que leurs arrondis décimaux |
| [`NOTE_CLAUDE_PRODUCTEUR_ANCRE_EXACT_UNE_FOIS_20260812.md`](NOTE_CLAUDE_PRODUCTEUR_ANCRE_EXACT_UNE_FOIS_20260812.md) | description et mesures locales du premier producteur par arête maximale; les portes CMake ont depuis été ajoutées; facteur du front, baseline q3+q4 `440,340886n`, total Source S `480,340886n`, indépendance du différentiel et domaine `smax` sont séparés ou rectifiés ci-après |
| [`AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md`](AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md) | contre-audit pincé de `760469d` : réponses aux cinq questions de Claude, réfutation du contrat `smax` historique, portée des 28 CTests, scratch/device et boucle q4; classifieur AABB exact `NONE/ALL/UNKNOWN`, preuve `O(m(k+1))` des centres shallow mono-ancre et ordonnance exacte `P-P/N-N/P-N` ou cutting certifiée |
| [`REPONSE_CLAUDE_CONTRE_AUDIT_LENTILLE_AIGUE_20260812.md`](REPONSE_CLAUDE_CONTRE_AUDIT_LENTILLE_AIGUE_20260812.md) | réponse de Claude : seuils `smax` paramétrés, facteur Poisson et exclusion des endpoints corrigés, preuve shell--theta, tris remplacés et questions de cisaille/niveaux/plateaux; les verdicts de fermeture restent soumis aux pins et portes du contre-audit |
| [`AUDIT_REPONSES_CLAUDE_CHAMBRES_NIVEAUX_CUTTING_20260812.md`](AUDIT_REPONSES_CLAUDE_CHAMBRES_NIVEAUX_CUTTING_20260812.md) | réponses consolidées : banque entière 432, redondance du filtre theta sur tout domaine vivant, largeurs de cisaille, niveaux q4 pondérés, cutting signée, identities always-inside, dominance exacte des concurrences et limites `J/H`; aucun claim global/G4/SLO |
| [`NOTE_CLAUDE_MUR_CUBIQUE_AMAS_ET_COUT_CENSUS_20260812.md`](NOTE_CLAUDE_MUR_CUBIQUE_AMAS_ET_COUT_CENSUS_20260812.md) | exécution reçue de l'ordre theta et rampe `eight_clusters`; ses colonnes de front `C(n,2)/0`, son impossibilité géométrique, son attribution au seul census et son claim cubique sont réfutés par le contre-audit suivant |
| [`AUDIT_REPONSES_MUR_AMAS_CENSUS_SPINDLE_20260812.md`](AUDIT_REPONSES_MUR_AMAS_CENSUS_SPINDLE_20260812.md) | réponses Q1--Q6 : divergence des reçus, juge indépendant d'abord, spindle AABB avant liste, cône cible exact par huit coins pour une banque k-NN et lift entier suffisant `A×B×C`; garde de densité hors produit, aucune saturation déterministe de `kept`, portes midball historique et spindle produit séparées |
| [`AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md`](AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md) | contre-audit du successeur live : noyau ponctuel admis et `30/30`, mais cast `smax` à faux prune total, cardinalité silencieusement réduite, juge sans vérité par lane, résiduel non reçu, faux verts anchor et ABI CUDA cassée; pentes 48/96 rouges, NO-GO du port littéral avant G4 et reprise collective `A×B×C` avec `ALL` exact aux 512 triples |
| [`NOTE_CLAUDE_REPARATIONS_P0_CONE_20260813.md`](NOTE_CLAUDE_REPARATIONS_P0_CONE_20260813.md) | réponse de Claude aux deux contre-audits du 13 août : NO-GO accepté; domaine `smax` fermé avant tout cast et juge déplacé dans une unité de traduction indépendante à arithmétique deux limbes jugée par `BigInt`, trois lanes jugées séparément avec plancher par lane, cardinalité et digest, réfutations `NONE` comptées en transitions avec plancher corrigé à la baisse, mutant d'héritage enfin porté; neuf points restent explicitement ouverts, dont le résiduel non consommable et dix-huit portes à regex masquant leur code |
| [`NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md`](NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md) | recul de Claude sur les six ordonnances mesurées : toutes éliminent des paires, donc toutes restent quadratiques; rampe cône prolongée à `n=16 000` sans décroissance de pente, mesure des directions fermées par le théorème des calottes (environ neuf dixièmes sur les quatre familles, `terrain` compris) et route générative par fenêtre k-NN à certificat `4R^2<d_M(a)^2`, résiduel directionnel nommé, séquencement vers `10^7`; aucune complexité prouvée, étape de génération locale non écrite |
| [`AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md`](AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md) | réponses Q1--Q5 : certificat de fenêtre admis seulement pour une sous-source complètement énumérée et avec `U_B`; réfutation du shell borné, owner tardif et résiduel couvrant les supports jamais proposés; facteur à deux droites imputé sémantiquement à la positivité sans économie automatique de travail; aucun Yao d'ordre supérieur reçu, prune sûr par cône tangent, supports longs non jetables par Boruvka a priori; tuilage exact par merge global des niveaux/lots et ledger mémoire 10 M |
| [`AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md`](AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md) | voies exactes pour le résiduel après la fenêtre : mur combinatoire local chiffré, cutoffs q4/q3 `3/5` et `5/8` sur la banque rationnelle 432, top-h et range-report par dominance sans boucle sur les paires, groupes coniques de trois témoins pour les amas, cœur commun WSPD puis relation-tree `A×B×C`; ledger par candidature d'arête maximale, fusion OR/AND des orientations et gate counter-only avant CUDA, sans claim sparse universel |
| [`AUDIT_REPONSES_CLAUDE_GEOMETRIE_3D_20260813.md`](AUDIT_REPONSES_CLAUDE_GEOMETRIE_3D_20260813.md) | réponses Q1--Q7 : fixture u16 de treize q2 sans plateau dans une chambre, cutoff direct par réduction finie aux rayons, filtre FP certifié avec repli exact, obstruction au quotient des 48 ordres, lift 4D admis seulement comme index, finitude u16 non industrielle et cœur WSPD conditionné à 8/9 IDs uniques |
| [`NOTE_CLAUDE_DOMINANCE_432_MESURES_20260813.md`](NOTE_CLAUDE_DOMINANCE_432_MESURES_20260813.md) | mesures Claude du premier probe dominance 432 et discussion des owners/mutants; observation non reçue, séries directes/radiales mêlées et ledger encore quadratique |
| [`AUDIT_CONTRE_DOMINANCE_432_5DDF4A3_20260813.md`](AUDIT_CONTRE_DOMINANCE_432_5DDF4A3_20260813.md) | contre-audit du pin `5ddf4a3` : cutoff direct admis sous gate finie, `15/15` diagnostics, mais faux prune `smax=34`, mutant cible--témoin mal modélisé, compte sous-plein mal nommé, rampe incompatible et NO-GO du probe pairwise/bitset avant 50 k ou G4 |
| [`AUDIT_REPONSE_DOMINANCE_GROUPES_5DDF4A3_20260813.md`](AUDIT_REPONSE_DOMINANCE_GROUPES_5DDF4A3_20260813.md) | proposition factorisée après dominance : preuve finie du cutoff direct, génération planaire des groupes, packing fail-open puis régions polyédriques de cibles et nœuds LBVH ; proposition non reçue et repincée vers le successeur groupes |
| [`AUDIT_CONTRE_GROUPES_CONIQUES_2270077_20260813.md`](AUDIT_CONTRE_GROUPES_CONIQUES_2270077_20260813.md) | contre-audit du pin `2270077` : théorème scalaire admis et `11/11` diagnostics, mais `smax` ignoré, H2 partagé avec le juge, mutants equality/reuse survivants, comparaison au spindle ponctuel et non dominance 432, greedy incomplet et coût `O(n^3 log n)` ; NO-GO avant packing exact/factorisé et caps |
| [`AUDIT_WORKTREE_COEUR_COMMUN_20260813.md`](AUDIT_WORKTREE_COEUR_COMMUN_20260813.md) | contre-audit du pin `ec2fbab` et du successeur `d3329fe` : partition/cœur admis; retrait de deux, compteurs et multiplicité réparés; `smax` toujours faux, bord inclus sound, ceil unsafe sans fixture, IDs non reçus, quatre matrices `n^2` et rescans; fast path positif seulement, NO-GO 50 k/G4 |
| [`NOTE_CLAUDE_GATE_TROIS_VOIES_20260813.md`](NOTE_CLAUDE_GATE_TROIS_VOIES_20260813.md) | note Claude comparant dominance, groupes et cœur à tailles/ledgers différents et demandant s'il faut abandonner le cœur; observations non comparables, absence de pentes justement reconnue |
| [`AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md`](AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md) | réponse au pin `ec2fbab` et suivi `d3329fe` : conserver le cœur comme fast path positif opportuniste, jamais préfiltre négatif; ses occupations partielles peuvent compléter les rectangles dominance et les crédits cellulaires par enveloppe projective 2D sous ledger d'IDs disjoints |
| [`AUDIT_WORKTREE_CREDITS_CELLULAIRES_20260813.md`](AUDIT_WORKTREE_CREDITS_CELLULAIRES_20260813.md) | contre-audit de `c46d658` puis `01a3a3f` et Andrew live : réfutation du hull Jarvis, correctif borné `37 752/37 752` + checker carriers, `smax` dynamique; recette positive par piles projectives, reçus, `StarKey/RectKey`, huit coins et `L_z`, sans ledger de paires |
| [`NOTE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md`](NOTE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md) | note Claude du pin `88eb36d` : gain du hull et carriers bas rang, falsificateur axial, un mutant armé sur quatre, question sur fixtures contre différentielle; aucun claim pente/résiduel/G4 |
| [`AUDIT_REPONSE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md`](AUDIT_REPONSE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md) | réponse : une marge différentielle signifie mutant atteint, pas tué; contradictions exactes, Andrew live positif, seuil `smax` dynamique, amortissement du sweep puis front `StarKey/RectKey/CreditKey` recertifié par les huit coins et `L_z` |
| [`QUESTIONS_CLAUDE_TUER_LA_VOIE_20260813.md`](QUESTIONS_CLAUDE_TUER_LA_VOIE_20260813.md) | demande explicite de réfutation : parcimonie du résiduel dominance, plancher indépendant de l'ordonnance et ordre producteur/aval; reconnaît qu'aucune pente physique ni sortie factorisée n'était encore mesurée |
| [`AUDIT_REPONSE_CLAUDE_TUER_LA_VOIE_20260813.md`](AUDIT_REPONSE_CLAUDE_TUER_LA_VOIE_20260813.md) | réponse Q1--Q3 : deux plans u16 laissent exactement `n^2/4` paires croisées au résiduel dominance mais forment un rectangle compressible; deux droites u16 séparent `U_q` universel quadratique et Source S linéaire; vrai plancher `L_q`, borne Gabriel réelle et walking skeleton `RectKey -> SupportKey -> BallKey -> census -> fold` avant G4 |
| [`NOTE_CLAUDE_RESIDUEL_MESURE_ET_SATURATION_20260813.md`](NOTE_CLAUDE_RESIDUEL_MESURE_ET_SATURATION_20260813.md) | transmission Claude des douze masses q4 et adoption du front factorisé; l'anticorrélation couverture--pente, le facteur de sortie et la confusion entre les deux familles sont corrigés par les audits suivants |
| [`AUDIT_RECU_RESIDUEL_DOMINANCE_G4_8F2AD6D_20260813.md`](AUDIT_RECU_RESIDUEL_DOMINANCE_G4_8F2AD6D_20260813.md) | audit du diagnostic G4/CPU : comptes et pentes reçus arithmétiquement, mais aucune loi de saturation, aucun front/temps/p95; équation de coalescence physique, provenance incomplète, `pipefail` et arrêt fail-closed à réparer |
| [`NOTE_CLAUDE_LZ_FERME_LA_CONTRE_FAMILLE_20260813.md`](NOTE_CLAUDE_LZ_FERME_LA_CONTRE_FAMILLE_20260813.md) | réponse exploratoire de Claude : exactitude continue de `L_z`, mesure q2 par 32 tranches, addendum `Lambda` et questions identité/split puis rôle de `residual_pair_mass`; reçu CPU diagnostique, sans pente ni claim produit |
| [`AUDIT_REPONSE_CLAUDE_LZ_RECTANGLE_20260813.md`](AUDIT_REPONSE_CLAUDE_LZ_RECTANGLE_20260813.md) | réponse positive : certificat entier `Lambda(A,B,C)>0`, antichaîne de témoins q2, identité canonique et ordonnance `L_z` adaptative; quatre crédits ferment 624,99 M paires puis front exact de 55 supports q2, sans propagation q3/q4; la masse reste sémantique tandis que la source « par point » doit recevoir son coût physique et son aval |
| [`NOTE_CLAUDE_WSPD_FRONT_LINEAIRE_20260813.md`](NOTE_CLAUDE_WSPD_FRONT_LINEAIRE_20260813.md) | proposition Claude d'une WSPD comme partition linéaire des relations `A×B`, mesures de cardinal et premier certificat spindle séparé ; le théorème borne le front sous hypothèses, pas la descente `C`, la source ou l'aval, et la séparation flottante n'est pas canonique |
| [`NOTE_CLAUDE_DESCENTE_JOINTE_20260813.md`](NOTE_CLAUDE_DESCENTE_JOINTE_20260813.md) | correction par Claude d'un faux `NONE`, best-first borné, arrêt WSPD puis majorant des témoins possibles ; journal de recherche utile mais ses addenda ne constituent ni une descente persistante ni une qualification, et le claim de supports q3/q4 est réfuté |
| [`AUDIT_REPONSE_WSPD_DESCENTE_JOINTE_96BE8E0_20260813.md`](AUDIT_REPONSE_WSPD_DESCENTE_JOINTE_96BE8E0_20260813.md) | contre-audit du pin `96be8e0` : intervalle entier et `ALL` reçus, `POSITIVE_SUPPORT` limité à q2, WSPD reçue pour le seul front, CTest `1/4`, budget `24->25`, script G4 fail-open, ABI de continuation et route GPU `common-core + top-L` recertifiée avant source/fold |
| [`AUDIT_WORKTREE_WINDOW_SOURCE_20260813.md`](AUDIT_WORKTREE_WINDOW_SOURCE_20260813.md) | contre-audit du premier worktree de fenêtre : Cramer, puissance, certificat strict et Jung sous positivité admis; aucun raccord CMake, juge partageant top-M/coupure du sujet, comparaison de comptes au lieu des ensembles `I_B/U_B`, `BallKey` non jugée, fast path Jung public sans précondition et mutant i64 à UB; trois fixtures exactes et ordre de réception remis à Claude |
| [`AUDIT_SUCCESSEUR_WINDOW_SOURCE_FFE5B69_20260813.md`](AUDIT_SUCCESSEUR_WINDOW_SOURCE_FFE5B69_20260813.md) | réception du successeur commité : `21/21` portes et égalité des `SupportKey` certifiables sur quatre petits nuages, avec mesure de `909/303/1 277/129` supports jamais proposés; limites persistantes sur top-M partagé, membres `I_B/U_B`, `BallKey`, provenance par ancre, mutant i64 à UB et fixture d'égalité sans perte réelle de shell |
| [`AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md`](AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md) | juge Gauss/barycentriques indépendant des lifts et quatre accords bornés; porte mutant vacueuse sous `WILL_FAIL`; contre-audit du successeur k1 : poids MST exacts mais facteur quatre implicite, aucun lot/endpoint/payload et coût q2/q3/q4 complet avant lecture |
| [`AUDIT_REPONSES_CELLULES_CENTRES_20260812.md`](AUDIT_REPONSES_CELLULES_CENTRES_20260812.md) | autorité fusionnée L1/L2 et réponses successives : contre-fixtures, Poisson bulk, sortie cosphérique, `SupportKey` avant lift, Johnson/gateway et snapshots; les ledgers successifs restent audités dans leurs fichiers pincés |
| [`NOTE_CLAUDE_ETAT_CELLULES_CENTRES_20260812.md`](NOTE_CLAUDE_ETAT_CELLULES_CENTRES_20260812.md) | questions et observations historiques de Claude, sans hashes/transcripts suffisants pour un reçu; réponses Q1--Q3 dans la section 13 de l'audit fusionné |
| [`AUDIT_REPONSES_ETAT_CELLULES_CENTRES_20260812.md`](AUDIT_REPONSES_ETAT_CELLULES_CENTRES_20260812.md) | renvoi historique vers l'audit fusionné; aucune autorité parallèle |
| [`NOTE_CLAUDE_LEDGER_CAUSES_LIFTS_20260812.md`](NOTE_CLAUDE_LEDGER_CAUSES_LIFTS_20260812.md) | ledger `238cf12` établissant la domination de l'owner tardif, mais dont la partition par rang et les multiplicités annoncées sont corrigées par le contre-audit suivant |
| [`NOTE_CLAUDE_MULTIPLICITE_SUPPORTKEY_20260812.md`](NOTE_CLAUDE_MULTIPLICITE_SUPPORTKEY_20260812.md) | première mesure de clés uniques, corrigée dans le contre-audit : classes par stade maximal, `263 825` clés non dégénérées pour le coût RLE et `52 693` seulement comme floor pending idéal, calibration cycles non reçue |
| [`NOTE_CLAUDE_RETRACTATIONS_ET_COMPTES_EXACTS_20260812.md`](NOTE_CLAUDE_RETRACTATIONS_ET_COMPTES_EXACTS_20260812.md) | rétracte le facteur 42 et la parcimonie locale; contre-audit intégré : ancien transcript supprimé, rampe ouverte/non contractuelle, attribution du coût lift retirée; l'erreur historique d'incidence est corrigée par `3ffff85`, mais la porte n'a ni vérité indépendante, ni fixture saturée `K_24`, ni `T4` publié pour recalculer la garde |
| [`NOTE_CLAUDE_ABLATION_COUT_20260812.md`](NOTE_CLAUDE_ABLATION_COUT_20260812.md) | première ablation préfixée non reçue : différences marginales annoncées sans transcript; le tiers apparent agrège lift, centre, owner et positivité; le successeur refuse différé/planchers dans ses portes mais réutilise encore le schéma exact et n'isole pas causalement les phases |
| [`NOTE_CLAUDE_PENTES_UNIFORM_VERTES_20260812.md`](NOTE_CLAUDE_PENTES_UNIFORM_VERTES_20260812.md) | transcript gelé complet à trois familles et pentes uniformes vertes; diagnostic count-only sans `eight_clusters` ni latence qualifiable; corrections de la normale par adjugée, des côtés non stricts, du stall/overflow et de la lecture k1 qui paie encore toute la source |
| [`AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md`](AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md) | contre-audit comptable canonique : ledger et multiplicité corrigés, rampe mono-ELF réfutée, RLE spatiale exacte mais gain pouvant tendre vers un, `BallOwner` local distingué des shards, cap différé non borné en octets et borne Kruskal--Katona proposée pour les K4 |
| [`NOTE_SOLUTION_SOURCE_CELLULES_CENTRES_20260812.md`](NOTE_SOLUTION_SOURCE_CELLULES_CENTRES_20260812.md) | contrat et portes du prototype cellules-centres, corrigés par budget `h`, partition terminale commune, limites du contrôle runtime et des mesures; 22/22 ciblés sur le snapshot pincé, sans claim sparse/CUDA/SLO |
| [`NOTE_IMPLEMENTATION_SPARSE_COMPLETE_GABRIEL_GATEWAYS_20260812.md`](NOTE_IMPLEMENTATION_SPARSE_COMPLETE_GABRIEL_GATEWAYS_20260812.md) | pipeline conditionnel source directe--facettes--gateways--resolver--MSF, records, repli des plateaux, coûts évités et portes avant CUDA; aucune implémentation reçue |
| [`NOTE_CLAUDE_MESURE_PORTE_REGULIERE_20260812.md`](NOTE_CLAUDE_MESURE_PORTE_REGULIERE_20260812.md) | diagnostic borné corrigé : fractions de records émis portant une extra-shell, jamais fractions de boules, cofaces ou supports minimaux multiples; aucune extrapolation 50 k |
| [`NOTE_CLAUDE_JUGE_RATIONNEL_INDEPENDANT_20260812.md`](NOTE_CLAUDE_JUGE_RATIONNEL_INDEPENDANT_20260812.md) | arithmétique de sphère/positivité indépendante, mais générateur partagé et contrôle limité à des comptes par arité; pire cas `Theta(n^5)` et portée exacte tenue dans le verdict live |
| [`NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md`](NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md) | provenance de la session G4 mass-only et arrêt de la cible; déclaration de session, pas verdict produit |
| [`NOTE_SOLUTION_SOURCE_Q2_YAO48_LBVH_U16_20260811.md`](NOTE_SOLUTION_SOURCE_Q2_YAO48_LBVH_U16_20260811.md) | spécification de la route q2 candidate : Morton/LBVH, cascade Yao--banque affine--dual résiduel, maximum entier, classifieur terminal, census fermé, ledger et gate d'exposant; statut logiciel exclusivement dans l'audit live |
| [`AUDIT_ROUTE_50K_1S_DF9DC77_20260812.md`](AUDIT_ROUTE_50K_1S_DF9DC77_20260812.md) | contrat réellement chronométré, goulets q2/P1a, cascade exacte Yao--banque affine--dual résiduel, transcript Yao-1, fixtures et compteurs avant toute qualification sous une seconde |
| [`AUDIT_DEBLOCAGE_Q2_ET_LOCALITE_20260812.md`](AUDIT_DEBLOCAGE_Q2_ET_LOCALITE_20260812.md) | invariants conditionnels de l'état q2 adaptatif/triple-tree, maximum entier, lemme partiel de localité par inversion, seuil par cellule et réfutation du probe concurrent; aucune borne de complexité ni source reçue |
| [`AUDIT_REPONSES_LOCALITE_INVERSION_20260812.md`](AUDIT_REPONSES_LOCALITE_INVERSION_20260812.md) | réponses closes aux six questions de Claude : lemme et discrétisation admis, égalité fail-open, cellules ouvertes, réouverture conditionnelle du niveau inversé et réfutation du facteur 384 |
| [`AUDIT_PROBE_LOCALITE_778372F_20260812.md`](AUDIT_PROBE_LOCALITE_778372F_20260812.md) | réception du premier raccord CMake : 20/20 déclarés verts, mais fermeture top-M inversée, signe q4 faux, oracle q2 scalaire et faux rejet pré-calcul; prototype non reçu |
| [`AUDIT_PROBE_LOCALITE_8C00AB0_20260812.md`](AUDIT_PROBE_LOCALITE_8C00AB0_20260812.md) | contre-audit pincé du successeur : fenêtre q3/q4 non certifiée, compteurs distincts des identités, mode sparse incomplet, coût cône rouge et parseur acceptant les suffixes |
| [`NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY_20260811.md`](NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY_20260811.md) | spécification auditée du falsificateur P1a q4 mass-only : domaine de Jung, 64 patchs rationnels, témoins collectifs, ledger bijectif et protocole direct `n=32` vers 50 k; statut logiciel exclusivement dans l'audit live |
| [`AUDIT_P1A_CENTER_COVER_B312638_20260811.md`](AUDIT_P1A_CENTER_COVER_B312638_20260811.md) | audit du premier probe v3 P1a : théorème q4 sûr et différentiels bornés verts, mais reçus incomplets et rescan racine pratiquement quadratique; NO-GO du port littéral avant G4 |
| [`AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md`](AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md) | inventaire du prior art CUDA Yao48/LBVH et P1a dans `morsehgp3d/`, limites de qualification et propositions exactes de réemploi; différentiel, jamais autorité v3 |
| [`AUDIT_REEMPLOI_EMST_YAO48_LIGNE_ENREGISTREE_20260811.md`](AUDIT_REEMPLOI_EMST_YAO48_LIGNE_ENREGISTREE_20260811.md) | théorème Yao-1 contenant l'EMST canonique, prior art LBVH/Kruskal enregistré, rejet CPU et contrat de mutualisation exacte avec q2; blueprint, jamais preuve de débit |
| [`AUDIT_RECU_YAO48_ECHELLE_2E49DCF_20260811.md`](AUDIT_RECU_YAO48_ECHELLE_2E49DCF_20260811.md) | audit de la rampe CPU mono-binaire q2 à 12,5/25/50 k; trois familles structurées rouges, temps non qualifiables et ordonnance état--nœud du snapshot NO-GO avant G4; falsificateur unilatéral, jamais preuve de GO |
| [`AUDIT_RECU_YAO48_DUAL_C70974E_20260811.md`](AUDIT_RECU_YAO48_DUAL_C70974E_20260811.md) | audit de la rampe duale : résiduel et classifieur sous `1,35`, mais visites témoins rouges deux fois sur trois familles et uniforme incomplète; gate globale NO-GO avant G4 |

## Lemmes conditionnels, contre-fixtures et portes citées

Ces fichiers ne décrivent pas le `HEAD`. Ils conservent une contre-fixture, un
lemme dont les hypothèses restent explicites ou le contrat d'une porte encore
citée par le code. Leur présence dans cet index ne les promeut pas en preuve
formelle enregistrée :

| objet | portée |
| --- | --- |
| [`AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md`](AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md) | connectivité shallow conditionnelle de l'arrangement |
| [`AUDIT_ORDER_K_FLATS_9C587E6.md`](AUDIT_ORDER_K_FLATS_9C587E6.md) | contre-fixtures permanentes de `order_k_flats` |
| [`AUDIT_SOURCE_DIRECTE_24AD3D37.md`](AUDIT_SOURCE_DIRECTE_24AD3D37.md) | invariants et contre-exemples de la source directe |
| [`AUDIT_VOIE_MULTIPLICITES_ORDER_K.md`](AUDIT_VOIE_MULTIPLICITES_ORDER_K.md) | propriétaire shallow avec multiplicités |
| [`NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md`](NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md) | dichotomie des premières incidences du cœur |
| [`NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md`](NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md) | attache canonique conditionnelle par facette cœur |
| [`NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md`](NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md) | parent local conditionnel de reverse search |
| [`NOTE_POSITIVE_INDEX_KD_EXACT_ET_CERTIFICAT_PINCEAU.md`](NOTE_POSITIVE_INDEX_KD_EXACT_ET_CERTIFICAT_PINCEAU.md) | prédicats d'index spatial exact et contre-fixture flottante |
| [`check_gate_d_fold_f0.py`](check_gate_d_fold_f0.py) | gate Python F0 enregistrée par CMake; son succès reste local à ses fixtures |

## Reçus

Deux reçus à la racine de `receipts/` sont conservés comme diagnostics datés,
jamais comme portes v3 actuelles :

| fichier | SHA-256 | portée |
| --- | --- | --- |
| [`census_tukey_shallow_20260808.json`](../receipts/census_tukey_shallow_20260808.json) | `aba8abc5e479a8900a2c83aa0cc5618a3e0a05bc9a59963572c140738a5ea128` | minorant heuristique par 4 096 directions aléatoires; `git_commit=unavailable`, aucune complétude exacte |
| [`oracle_campaign_20260808.json`](../receipts/oracle_campaign_20260808.json) | `2579cd5a8eee14bc6e3d7e6ef83bdf052faacbcf90d2636e37e5c29c0c755bca` | différentiel exhaustif borné du sujet v2 à `n=8/11`; ne reçoit aucun worktree v3 |

Le dossier
[`../receipts/centre_cell_scale_20260812/`](../receipts/centre_cell_scale_20260812/)
contient la nouvelle campagne gelée encore ouverte. Le transcript mixte
antérieur de 55 lignes, SHA-256 instantané `a5f81584...`, a été supprimé sans
être archivé; seul son prédécesseur de 34 lignes, SHA-256 `0faceefb...`, reste
récupérable dans le commit `64cf6fe` et ne contient que le bloc 12 500 :

| fichier | portée |
| --- | --- |
| `scale_counters_raw.txt` | fichier supprimé; les sorties 25 000 et la commande 50 000 observées par l'audit ne sont plus reproductibles depuis Git; aurait dû être conservé sous `invalid_mixed` |
| `scale_counters_frozen.txt` | transcript clos, 254 lignes/SHA-256 `f02b7c4c...`, neuf commandes, neuf `rc=0`, ELF gelé identique et footer `RAMPE TERMINEE`; diagnostic count-only à trois familles, sans `eight_clusters`, digest d'identités, mémoire ni temps qualifiable; l'en-tête associe en outre `git_commit=64cf6fe` à une source `dbaa2e0` postérieure |

Les reçus G4 mass-only sont dans
[`../receipts/g4_massonly_20260811/`](../receipts/g4_massonly_20260811/).

| fichier | SHA-256 | portée |
| --- | --- | --- |
| `cell_50k_raw.txt` | `6b355d0d9c7bf01dbdeb1d14dc442cab75570e6be044dcd50f314d79b9010afe` | masses de cellules, aucun tuple ni pipeline |
| `mask_scale_raw.txt` | `d82e43c7f4b32a5731cfdb2bbb9edf22cd7cecef0fdc73e84d1457277d61c740` | scaling count-only, aucun fold |

Le dossier
[`../receipts/selfjoin_q2_20260811/`](../receipts/selfjoin_q2_20260811/)
contient trois journaux CPU diagnostiques :

| fichier | SHA-256 | portée |
| --- | --- | --- |
| `scale_counters_raw.txt` | `2685ceb387f46cb0be2f0a04f7b1ad8afbcaa41c521dad20328c7a4cb5332bc5` | snapshot de l'ancien binaire, 15 runs nuls et le contre-exemple 12 500 rouge |
| `scale_counters_correctif_12500_raw.txt` | `3ade1bc74dd2f129a9c26079fe8c52195946e8ccd479c587e462e2d40144149d` | autre binaire et autre contrat local; diagnostic correctif séparé, pas réécriture du reçu rouge |
| `anchor_core_counters_raw.txt` | `6f7938c53da21a55e8e8072d66dc2cea400a2bea2628845f578b1dcf5dfc70a7` | campagne terminée 400/1 200/2 400; en-tête source incomplet et portes core non reçues |

Leurs compteurs peuvent falsifier une route; leurs temps sous charge ne sont
ni un benchmark reçu ni `warm_e2e`.

Le dossier
[`../receipts/yao48_scale_20260811/`](../receipts/yao48_scale_20260811/)
contient la rampe q2 CPU auditée séparément :

| fichier | SHA-256 | portée |
| --- | --- | --- |
| `scale_counters_raw.txt` | `acf8e89248131cc7fdce3246f559d380acbee4ce67548ac9fb5e26efdd67d889` | douze ledgers count-only fermés sur un binaire dont la provenance a été reconstruite; aucun payload ni temps qualifiable |
| `exponents_derived.txt` | `f2d9783211d884fef821a45961d428ee645bad656685d1520337957f54d2776f` | exposants arithmétiquement justes; masses de couverture et secondes exclues de la gate de travail, arrondi `1,35` ambigu pour une valeur brute strictement rouge |

Le dossier
[`../receipts/yao48_dual_20260811/`](../receipts/yao48_dual_20260811/)
contient la rampe de la frontière duale auditée séparément :

| fichier | SHA-256 | portée |
| --- | --- | --- |
| `dual_scale_counters_raw.txt` | `a19ac56290e3262f9f1fc9b05e37952688f3a26db1f80fb989325a53292ce1b1` | trois triplets structurés count-only fermés et un seul cas uniforme; mode CLI faussement imprimé `exact`, provenance reconstruite, temps non qualifiables |
| `dual_exponents_derived.txt` | `d173160efafade5994e2c3faa2ef1fee33c93f128405cfdfc139deb0b5592b01` | fichier combiné c709+v2; sorties/classifieur publiés verts, mais `dual_witness_visits` rouge deux fois dans les trois familles complètes de chaque matrice; gate globale NO-GO |
| `dual_scale_v2_counters_raw.txt` | `e79a7a1cee5b83a114c39332d5e56e4451b41d04eeca0908b23c0de75e7e592a` | trois triplets structurés du snapshot ponctuel v2 et un seul cas uniforme; `dual_point_tests` manque à la sortie, donc ce reçu ne peut jamais prouver un GO |

Le dossier
[`../receipts/p1a_scale_20260811/`](../receipts/p1a_scale_20260811/)
contient la rampe P1a auditée séparément :

| fichier | SHA-256 | portée |
| --- | --- | --- |
| `p1a_scale_counters_raw.txt` | `140320266c11ba74dc0b7f5405c89ccc6ebb166875fa9b469373da4af53ea9b0` | terrain 2/4/8 k à code nul; uniforme 2/4 k à code nul puis 8 k interrompu; même binaire selon la session, provenance externe et temps non qualifiables; les compteurs terrain refusent l'ordonnance |

## Autorités externes

- [`SPECIFICATION_MORSEHGP3D.md`](../../docs/SPECIFICATION_MORSEHGP3D.md) :
  contrat et SLO.
- [`STATUT_PREUVES_ET_HEURISTIQUES.md`](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md) :
  registre des preuves et réfutations.
- [`INCIDENCES_SILENCIEUSES_GAMMA.md`](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md) :
  inertie H0.
- [`CATALOGUE_PAIRES_DIAMETRALES_EXACT.md`](../../docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md) :
  architecture q2 Yao/LBVH.
- [`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md) :
  Jung et limites des graphes low-rank.

GCP non utilisé.
