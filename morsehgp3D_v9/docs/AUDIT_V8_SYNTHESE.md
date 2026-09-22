# Audit général de la v8 (et de ce qui était bon en v7)

22 septembre 2026. Base auditée : `origin/main` **12294241**, lue dans un
worktree détaché. Cadre de l'audit : `phase=exploration_v9_hors_registre`,
`backend=none`, `mode=audit_v8_et_v7_avant_v9`, `public_status=not_claimed`.
GCP non utilisé. Audit demandé par l'utilisateur le 22 septembre :

> « Fais un audit général de tout ce qui a été fait dans cette version de
> Morse HGP 3D v8. On va passer à la v9. Une fois que tu auras tout audité de
> la v8 (et aussi de ce qui était bien de la v7), crée le dossier minimal de
> morsehgp3D_v9 pour passer la main aux développeurs et auditeurs de la v9. »

Méthode : douze lentilles indépendantes en lecture seule (trajectoire et
contrats ; chaîne q2 ; voies q3/q4 ; float32 ; élargissement 18 bits et
tranche non commise ; portes et qualité ; mesures et reçus ; entrées LiDAR et
G4 ; audits indépendants ; héritage v7 ; objet et preuves ; parallélisme et
GPU), chacune suivie d'une contre-vérification adversariale de ses chiffres et
de ses preuves, puis d'une critique de complétude. Les rapports détaillés sont
dans [audit_v8/](audit_v8/README.md). Deux constructions neuves du commit
audité ont rejoué la suite CTest (voir le [reçu d'audit](../receipts/audit_v8_20260922/README.md)).
Ce document ne promeut aucun statut.

**Indépendance.** Cet audit de clôture est mené par la même lignée de session
que l'auditeur B, devenu constructeur puis développeur de la v8 (commits
signés de la même session depuis le 14 septembre). Il est rigoureux et
contre-vérifié, mais ce n'est pas un audit indépendant : les auditeurs de la
v9 doivent le rejuger.

## 1. Verdict

La v8 a produit, en neuf jours et 169 commits, un **générateur exact de
candidats q2/q3/q4** en arithmétique entière, parallèle sur CPU, élargi à
18 bits par coordonnée, avec une discipline de preuve et de reçus très
supérieure aux versions précédentes. Elle **n'a pas produit la tour HGP** :
ni catalogue canonique des boules, ni intérieurs q3/q4, ni fold, ni forêts
K=1..10, ni parents, ni verticales. Le contrat (tour entière en 1 s puis
100 ms) n'est donc **pas mesurable** sur la v8 ; seul un flux de candidats a
été chronométré, et il est lui-même à un ou deux ordres de grandeur du
budget. Aucune ligne de code GPU n'existe en v8.

La v7 avait, elle, construit et mesuré la tour FULL (50 000 points uniformes :
418,9 s pour K=1..10, 33,9 s pour K=1..5, dont 93 % dans le constructeur
FULL mono-thread). La v8 a décidé de reconstruire l'amont et a laissé tout
l'aval de côté. La v9 doit réunir les deux : le générateur exact de la v8
et l'objet FULL de la v7, mesurés **de bout en bout** sur le régime
prioritaire.

## 2. Contrats et décisions, dans l'ordre

| date | source | énoncé | état au 22 septembre |
| --- | --- | --- | --- |
| hérité v7 | `AGENTS.md` | tour FULL K=1..10 à 50 000 points en moins de 1 s sur G4, repli K=1..5, puis 100 ms | jamais mesuré en v8 (aucune tour) ; référence v7 419 s / 34 s |
| 13 sept. | ouverture v8 | P0 : supprimer les histogrammes locaux O(\|A\|²+\|B\|²) | non clos au sens strict ; constantes divisées (tranches 12, 20, 21), exposants inchangés |
| 14 sept. | consigne utilisateur | séparation WSPD : « ne prends jamais s en-dessous de 8 » ; s ∈ {8, 10, 12} donne le même objet, s = 8 par défaut | respectée ; consignée seulement dans `morsehgp3D_v8/audits/SEPARATION_20260914.md`, reportée dans `AGENTS.md` à l'ouverture v9 |
| 21 sept. matin | consigne utilisateur à l'auditeur B | « Pas de confrontation à la vérité terrain pour l'instant. On ne s'occupe que du clustering, pas encore de la pertinence du clustering. » | respectée : aucune étiquette SemanticKITTI utilisée, et la confrontation prévue des entrées q3/q4 à `reference/morsehgp3d_oracle` a été abandonnée ; absente du dépôt jusqu'à l'ouverture v9, citée d'après le journal de session ; portée exacte en § 9 |
| 21 sept. | `morsehgp3D_v8/docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md` | une trame SemanticKITTI **entière**, plusieurs scènes, tour K=1..10 en moins de 1 s sur G4, repli 1..5, puis 100 ms | non mesurable (pas de tour) |
| 21 sept. | `morsehgp3D_v8/docs/PRECISION_FLOAT32_ET_GRILLE_20260921.md` | float32 original par défaut, grille isotrope optionnelle à 1 mm | briques float32 qualifiées, aucun générateur global |
| 21 sept. | décision utilisateur (`AGENTS.md`, section q3/q4 global et LiDAR) | la croissance sur les régimes visés, surtout SemanticKITTI, prime sur l'attente d'une borne sous-quadratique universelle ; les contre-régimes restent publiés, sans veto ; l'exactitude reste exigée sur tout le profil | respectée ; reprise dans le plan v9 |
| 21 sept. 20:10 | directive utilisateur (citée ci-dessous) | régime prioritaire : SemanticKITTI **sans sol**, 30 000 à 60 000 points ; contrats temps 1 s / 100 ms et K5 / K10 à y passer ; multi-CPU puis GPU ; feu vert G4 | `AGENTS.md` cite le sans-sol comme régime prioritaire « supplémentaire », sans la taille ni le transfert du contrat ; mesurée sur le flux seul |
| 22 sept. ~01:20 | réponse utilisateur à la question Q2 de l'audit de reprise | « Oui, continuons en entier 18 bits » : contrat temps sur le moteur entier élargi à 18 bits (grille 1 mm), float32 sans perte qualifié mais hors contrat temps | port `a74e90f2` publié ; `AGENTS.md` dit encore « le moteur existant demeure u16 » |
| 22 sept. | consigne utilisateur | « Ne développe pas plus pour le float32 pour l'instant » | respectée |
| 22 sept. soir | réponse utilisateur aux points 6 et 7 du § 9 | « Je te laisse décider pour l'oracle ; le but est d'avoir un livrable qui fonctionne, pas de pinailler sur les détails à l'infini. L'objectif multi-millions est un objectif secondaire, une fois les contrats satisfaits sur LiDAR » | tranchée : oracles de correction bornés utilisés comme portes, étiquettes exclues ; échelle multi-millions après les contrats LiDAR |

Directive du 21 septembre, texte de l'utilisateur : « La priorité de régimes
est pour les nuages LiDAR SemanticKITTI sans sol ; c'est principalement sur
ces nuages avec entre 30 000 et 60 000 points que les contrats temps
(1 seconde ou 100 ms) et K (5 ou 10) doivent passer. Continue le
développement, la parallélisation. multi-CPU et GPU. Feu vert pour tester sur
GCP G4 dès que tu en auras besoin. »

La question Q2 à laquelle l'utilisateur a répondu « Oui » était : « Accepter
que le contrat temps soit poursuivi sur le moteur entier élargi à 18 bits
(grille 1 mm), le profil float32 sans perte restant qualifié mais hors
contrat temps » (`morsehgp3D_v8/docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md`,
§ Questions). Cette réponse n'avait pas été consignée dans le dépôt ; une
tranche non commise d'un autre acteur la requalifie en « priorité de
développement » (voir § 8). La v9 part de la décision telle qu'elle a été
donnée ; les points encore ambigus sont listés au § 9.

Questions posées à l'utilisateur par l'audit de reprise du 21 septembre
(`morsehgp3D_v8/docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md`, § Questions),
et leur sort :

| question | réponse | provenance | état pour la v9 |
| --- | --- | --- | --- |
| Q1 — session concurrente dans l'arbre partagé, sort des brouillons float32 | aucune réponse écrite | — | ouverte ; un « constructeur » écrivait encore le 22 septembre de 09:58 à 10:56 UTC (§ 8) ; reprise en § 9, point 5 |
| Q2 — contrat temps sur le moteur entier 18 bits, float32 qualifié hors contrat | « Oui, continuons en entier 18 bits » | réponse en conversation le 22 septembre, non consignée en v8 ; consignée dans `AGENTS.md` à l'ouverture v9 | tranchée |
| Q3 — dérivés KITTI versionnés (111 Mo), licence Patchwork++ | aucune réponse | — | ouverte ; élargie au profil OS Login en § 9, point 4 |

## 3. Ce que la v8 calcule

| objet | existe à 12294241 ? | preuve |
| --- | --- | --- |
| nuage préparé, index spatial à coupes au milieu, front WSPD multivoie | oui | `src/pipeline/prepared_cloud.*`, `src/pipeline/q2_census.*`, `src/wspd/front.*` |
| census q2 exact (paires diamétrales, intérieurs, coquille complète) | oui, entrées séparées `run_wspd_q2_*` | tranches 4 à 21 ; jamais mesuré sans sol |
| flux global de **candidats** q3/q4, exact, parallèle | oui, `run_wspd_q34_parallel` | `src/pipeline/wspd_q34.hpp:100-114` : « not a deduplicated ball catalogue, interior-ID payload, HGP forest or FULL tower » |
| catalogue canonique dédoublonné (clé, q_min, intérieurs, niveau exact) | **non** | doublons mesurés : 1,7 % des records q4 à K5 sur une trame (audit B) |
| forêts K=1..10, parents, verticales, tour FULL | **non** | `src/forest/` ne contient que `.gitkeep` |
| entrée grille 1 mm (18 bits, `.u32le`) | oui depuis `a74e90f2` | profil `quantized_u18_input_only` |
| briques float32 sans perte | primitives qualifiées, pas de générateur | voir [float32](audit_v8/04_float32.md) |
| GPU | **non** | aucun `.cu`, `GPU_executed=false` dans tous les reçus G4 |

## 4. Chiffres qui comptent

Toutes les mesures locales proviennent d'un hôte partagé de 8 vCPU (4 cœurs
physiques). « Flux » désigne le seul générateur q3/q4 en mode digest.

| grandeur | valeur | source |
| --- | --- | --- |
| tour FULL v7, 50k uniforme, G4 : K=1..10 / K=1..5 | 418,9 s (FULL 389,7 s) / 33,9 s | `morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910/` |
| tour v7 K10 : nœuds, boules, RSS | 27,27 M ; 21,47 M ; environ 15,5 Gio | idem |
| flux v8, trames brutes entières (119–121k sites), G4 W48, K5 | 165,2 / 34,3 / 505,5 s ; 4,2 / 11,1 / 1,9 CPU occupés sur 48 | `morsehgp3D_v8/receipts/q34_spatial_20260921/` |
| flux v8 sans sol 2 cm, scène 0 (39 815 sites), avant / après phases 1-2 | K5 W1 889,5 → 453,3 s ; K5 W8 108,0 s pour 741,5 CPU·s ; K10 W8 323,0 s pour 2 212,1 CPU·s | `receipts/ground_baseline_20260921`, `receipts/ground_phase1_20260921` |
| flux v8 sans sol 2 cm, scène 2 (45 114 sites), K10 W8 | 824,1 s ; 3 811,0 CPU·s (charge croisée déclarée) | `receipts/ground_phase1_20260921` |
| flux v8 sans sol 1 mm, scène 0 (39 885 sites), K5 W8 | 104,6 s ; 812,8 CPU·s ; une répétition | **non commis, non suivi** (`receipts/u18_resume_20260922/ground_1mm_first`) |
| entrées u16 après le port 18 bits (six lignes W8) | sorties et 449 compteurs logiques identiques à la phase 1 ; CPU +5 à +8 % sous une charge de 8 à 10 (non comparable) | **indexé, non commis** (`receipts/ground_18bits_20260922/u16_identity`) |
| atlas q4, scène 0, K10 | 10,7 G bornes de blocs + 24,2 G tests ponctuels + 18,3 G IDs copiés, inchangés par les phases 1-2 | `ground_phase1_20260921/only_probe_02…json` |
| graines q3 rejetées par l'atlas sans census | 82,7 à 95,0 % | idem |
| budget du contrat 1 s sur G4 | 48 CPU·s (24 cœurs physiques, 48 fils) | — |
| facteur de travail à gagner sur le **flux seul**, 1 s, trames sans sol 2 cm : **scénarios conditionnels de débit**, ni mesures ni minorants | K5 ×6 à ×36 ; K10 ×17 à ×101 ; 100 ms : ×10 de plus (selon la scène, CPU W1 mesuré ou inféré, charge croisée, 24 ou 48 fils équivalents, vitesse G4 supposée) | [parallélisme](audit_v8/12_parallelisme_gpu_perf.md) § 4 bis et sa contre-vérification |
| suite CTest au commit audité | 132 enregistrés ; 129 exécutés et verts (dont 8 mutations) ; 3 désactivés par construction, donc non jugés | [reçu d'audit](../receipts/audit_v8_20260922/README.md) |
| volume versionné de la v8 | 19 281 fichiers ; reçus 979,3 Mo en 16 016 fichiers | inventaire du reçu d'audit |

Lecture : la conclusion robuste est que le flux actuel **dépasse fortement**
le budget, pas que le gain requis est connu au facteur près. L'aval absent
(q2 dans l'appel, catalogue, tour) s'y ajoutera ; il représentait 93 % du
temps de la tour v7 sur nuage uniforme K10, ce qui est une alerte et non un
Amdahl applicable au LiDAR. Sur la trame brute 0 (K5, mêmes sorties), la G4
a consommé 0,49 fois les CPU·s de l'hôte local, mais ce rapport compare G4
W48 (4,19 CPU logiques occupés en moyenne) et local W4 sous charges
différentes : il ne donne pas de vitesse G4 par fil. À K10, l'atlas q4 paie
sur la scène 0 10,7 G bornes de blocs, 24,2 G tests de points et 18,3 G copies
d'IDs, masses de coûts unitaires différents à publier séparément : le
**nombre** d'opérations doit baisser d'au moins un ordre de grandeur, aucune
constante ni aucun parallélisme ne suffit seul.

Chiffres **sans reçu** qui circulent dans les documents v8 et dans certains
rapports d'audit : ils ne doivent ni fermer une piste ni ordonner une
priorité en v9 tant qu'une mesure épinglée ne les a pas remplacés.

| chiffre | où il apparaît | pourquoi il n'est pas une mesure |
| --- | --- | --- |
| coût float32 « ×20/×23 à ×70 par prédicat », « clé 78 µs », « 68–185 ms par arête » | audit de reprise, rapports 01, 03, 12 | la source déclare « aucune mesure n'est épinglée » ; aucun reçu |
| profil après phase 1 : atlas ≈ 52 %, filtres 13 % | journal du 21 septembre | aucun profil épinglé après phase 1 ; le seul gprof épinglé est celui de la base (famille atlas ≈ 57,6 %, dont `node_bounds_unchecked` 40,32 %) |
| chronos du quart de 7 067 sites (48,5 → 39,9 → 27,3 s ; W8 7,2 s) et variantes d'atlas rejetées | journal du 21 septembre | diagnostic sur une sonde de scratchpad perdue |
| écriture de la sortie v7 à 50k : 62,7 ms | note v7 | seule la taille (1,75 Go = 27 273 218 × 64 octets) est vérifiée |
| taux de rejet des graines q3 « 73,8–90,7 % » | note de tranche | quart de trame et préfixe 8k, déclaré diagnostic ; les reçus épinglés donnent 82,7–95,0 % |
| « ×2,6–3,9 par site sans sol », « 250 nuages », « 3/366 », « 415 dépendances 16 bits », « 79 portes, 129 exécutées » | notes d'audit et journal | comptes ou rapports non recalculables depuis un reçu |
| MEB pivot4 0,262–0,271 ; exhaustifs du quart et de la moitié x+ | notes d'auditeur | sources hors dépôt ou captures non épinglées |
| gains de la cascade (×2,55–4,46), présélection (×5,13–11,10), collectif LiDAR (221/763) | audits du 22 septembre | sources dans des archives jointes à une conversation ; seul l'artefact Actions a été rapatrié ([reçu](../receipts/actions_artifact_lidar_rectangles_20260922/README.md)), et il ne mesure que le filtre sur échantillon |

## 5. Ce qui est bon et doit être gardé

- **Exactitude en entier** : prédicats i64/i128 sans jitter ni flottant,
  bornes écrites par degré, élargissement 18 bits explicite (clés 3×18 bits,
  refus de plage, bornes q2 en u64, localisation du centre q3 par division
  longue) ; entrées u16 bit-identiques après le port sur six lignes appariées
  (reçu encore non commis).
- **Certificats de rejet prouvés** : lemme du citron (α3 = 3, α4 = 2) et son
  contre-exemple sur q4, certificat frère, théorème H des témoins hérités,
  Pool terminal par facteurs (×3,7 à ×17,2 sur amas selon n et K ; aucun plan retenu sur uniforme et terrain), certificat familial,
  corde resserrée, fenêtre q4 [L, U], couches duales, certificat d'atlas pour
  les graines q3, certificat collectif d'arête (prototype d'audit).
- **Indépendance des voies** q2, q3, q4, gravée par contre-fixtures et
  mutants ; propriété unique par arête maximale et paire d'IDs.
- **Juges** : oracles rationnels Boost indépendants, planchers de
  non-vacuité, mutants compilés, injection déterministe d'échecs d'allocation
  (dix portes remplacent `operator new`), harnais de contre-vérification
  indépendants de l'auditeur B (flux q3/q4 identiques jusqu'à 8k pour la
  tranche 32, 4k pour les tranches 31, 33 et 34 ; validité bilatérale de tous
  les records q4 d'une trame brute entière).
- **Discipline de reçu** : captures immuables (46 dossiers sur 48 jamais
  modifiés), sha256 de sources et binaires, échecs conservés et non promus,
  lecteurs sous `python3 -O`, refus d'écraser.
- **Régime LiDAR** : masque sans sol Patchwork++ sur IDs, trois trames
  préparées en 2 cm, 1 mm et float32, protocole spatial trame / moitiés /
  quarts, protocole G4 à arrêt certifié (quatre sessions, toutes TERMINATED).
- **Parallélisme** : file bornée de plages de rectangles (occupation 686 % sur
  8 vCPU au lieu de 430 %), identités `published = consumed`, chronos par
  worker, bit-identité W1/W8 des sorties et des compteurs.

## 6. Ce qui ne va pas

Gravité haute :

1. **Pas de tour.** Tous les contrats portent sur un objet que le code ne
   produit pas ; l'aval FULL de la v7 n'a pas été porté.
2. **Mauvaise recommandation d'objet dans l'audit de reprise v8.** La phase 3
   de `morsehgp3D_v8/docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md` proposait
   de porter « le fold en forêts K = 1..10 de la v4 ». Ce fold (union des
   facettes des boules émises) ne calcule pas FULL : le registre des preuves
   le classe `false_in_general` (fixture E5). La v9 porte la sémantique FULL
   de la v7 (`morsehgp3D_v7/src/forest/full_ball_tower.hpp`), pas le fold v4.
3. **Écart de travail d'un à deux ordres de grandeur** sur le flux seul, dominé
   par la partition de l'atlas q4, que les phases 1-2 n'ont pas réduite.
4. **Couverture de test contournable.** Sans Boost, 25 juges disparaissent
   sans erreur ; cinq scripts de mutation ne tournent que dans l'arbre de
   build canonique ; trois mutants du citron sont désactivés depuis
   `2629a536` ; la porte spatiale n'accepte qu'un build épinglé où `ctest` est
   interdit ; aucune CI ne construit la v8 ; la file de tâches n'est jamais
   passée sous TSan ; aucun sanitizer sur les commits moteur du 21–22
   septembre.
5. **Contrats et provenance.** Quatre énoncés coexistent (trame brute, sans
   sol, float32 par défaut, entier 18 bits) ; `AGENTS.md` n'a enregistré ni la
   directive sans sol ni la décision 18 bits.
6. **Données tierces et données personnelles dans un dépôt public.** Trois
   scans KITTI bruts (`RAW.bin` dans
   `receipts/q34_spatial_20260921/gcp_package_r1/snapshot.tar.gz` et ses deux
   copies, avec les nuages 2 cm des sept morceaux) et environ 114 Mo de dérivés
   (float32, grilles, masques) sont versionnés dans un dépôt GitHub **public**
   sous licence MIT, alors que KITTI est publié sous licence non commerciale à
   partage à l'identique ; la question posée le 21 septembre est restée sans
   réponse. Les archives hôte des sessions G4 versionnées
   (`receipts/q34_spatial_20260921/gcp_r1*`, `receipts/lidar_global_20260921/gcp_r*`)
   contiennent en outre le profil OS Login du compte (adresse e-mail,
   identifiant numérique, uid, projet, cinq clés SSH publiques) : pas un secret,
   mais une donnée personnelle publiée.
7. **Tranche non commise dans l'index partagé** (89 fichiers du développeur
   et d'un « constructeur », voir § 8) : un commit imprudent l'emporterait,
   comme `4c3cdb0c` a emporté 300 fichiers de la tranche 34. L'ouverture v9 a
   été commise depuis un worktree séparé pour cette raison.

Gravité moyenne : défauts de bibliothèque lents (la configuration mesurée
`rectangle-pair boxes affine live atlas` reste opt-in) ; cinq entrées q2 et
environ 1 900 lignes de prototypes compilées dans la bibliothèque produit ;
reçus non autonomes (lecteurs LIVE liés à 181 builds locaux non versionnés,
sonde de la campagne de phase 1 perdue dans un scratchpad, preuves d'audit du
22 septembre dans des archives jointes à une conversation) ; canal de coordination v8
muet après `204b0620` (21 septembre) : les six commits moteur et les audits
du 22 n'y sont jamais annoncés, et l'auditeur complémentaire ne répondait
plus depuis le 13 ; documents de verrous et de refonte périmés (§ 10) ;
arbre de plages du nuage (80 % de sa mémoire) construit pour un prototype
abandonné ; campagnes sous
charge croisée (sentinelle périmée) ; voie q2 jamais mesurée sans sol ;
registre des preuves `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` jamais mis
à jour en v8 ; domaine non régulier (plateaux cosphériques) sans sémantique
aval ; la voie q3 n'a aucun juge indépendant à l'échelle d'une trame (le
protocole bilatéral ne couvre que q4) ; aucune mesure G4 depuis la file de
tâches ; une seule séquence SemanticKITTI (08, trois trames), sans labels.

Gravité basse : documentation d'entrée périmée (`README.md` v8 de 879 lignes
en journal, `CLAUDE.md` désignant la v5), bornes fausses dans la note 18 bits
publiée (le code est juste), commentaires 16 bits résiduels, `root_lane_skips`
structurellement nul, double recherche de témoins sur les rectangles
singletons, cover construit pour les arêtes q3 seules.

## 7. Ce que la v7 avait de bon et que la v8 a laissé

- L'**objet FULL** défini et contrelu : feuilles = minima Gabriel de cardinal
  K, nœuds internes = vraies multifusions de cardinal K+1 avec parents
  pré-lot, portails silencieux internes, ancres et verticales, extension non
  régulière (contributions datées, ancres inertes). Portée exacte de la
  preuve (registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, l. 120-122 et
  131-134) : un **théorème conditionnel horizontal** sous prémisses
  régulières ; la fenêtre d'inertie ne prouve ni toutes les verticales ni
  l'objet public. Sur une grille 1 mm, plateaux et coquilles supplémentaires
  de rang pertinent exigent un quotient de plateau certifié ou un refus de
  domaine explicite.
- Le **constructeur de référence** `full_ball_tower.hpp`, la MEB à coquille
  libre, le journal daté v2, le quotient local de coquille, le juge T2
  census→FULL (n ≤ 14) et ses fixtures (E5, quatre points, A–E, coquille à
  sept points, triangle rectangle, MEB K7).
- La **borne de sortie** quadratique (m² minima de cardinal 2 sur 2m points
  réguliers) : les contrats doivent parler de surcoût au-delà de la sortie.
- Les **mesures de tour**, seules du dépôt (50k sur G4, et 8k / 16k / 32k en
  local ; toutes sur nuages uniformes u16, aucune sur LiDAR ni sur famille non
  uniforme, et aucune à 50k après les optimisations du 11 septembre) : aval
  FULL 93 % du temps, 27 M nœuds et 15,5 Gio à 50k K10, écriture de la sortie
  au format v7 estimée à 62,7 ms (hôte local, sans reçu), kernels GPU à 4 %
  de leur étage.
- L'**outillage** : portes à code de sortie exact (`run_expect.cmake`), mutants
  `--inject` exécutables dans tout arbre, CI avec Boost obligatoire,
  manifestes `SHA256SUMS` vérifiés par un contrôleur unique.
- Les **prototypes privés** scellés (graphes datés sur naissances, contraction
  des pivots, workers persistants, gardes par rangs) et le noyau MEB accéléré
  des auditeurs (×3,25 supports, ×1,36 sur la tour à 8k, digest identique).

Détail, pins et fausses pistes fermées : [héritage v7 et v8](HERITAGE_V7_V8.md)
et [fausses pistes](FAUSSES_PISTES.md).

## 8. État en suspens au moment de l'ouverture

Dans le worktree partagé `/workspaces/E-HGP`, au-dessus de `a74e90f2` (et donc
en retard de 13 commits d'audit sur `origin/main`), 89 fichiers sont indexés
sans être commis (+7 962 / −979). Ils mêlent deux auteurs : le développeur de
la v8 (portes jumelles 18 bits de 42 fichiers de test et campagne d'identité
u16 à six lignes, 06:29–06:53 UTC) et un « constructeur » (reprise u18 et
atlas saturant, 09:58–10:56 UTC : gardes de domaine des fabriques publiques,
option `Q4LocalOptions::saturate_deep` désactivée par défaut, juge de centre
corrigé, modifications non indexées d'`AGENTS.md`, de la passation, du journal
et de la coordination). Environ 955 fichiers de captures de ce constructeur
sont en outre non suivis. Ses quatre captures de qualification sont closes en
échec, dont deux pour une raison de lecteur (« disabled CTests differ »,
alors que CTest passe 136/136 en Release et 131/131 sous ASan/UBSan). Cette
tranche n'est **pas** dans l'ouverture v9 : il faut la commettre en v8 après
correction du lecteur, ou l'abandonner par écrit. D'autres états non commis subsistent : brouillon float32 global (21
septembre), delta v7 « marques au premier parcours », notes de l'auditeur
complémentaire du 13 septembre.

## 9. Questions à l'utilisateur et décisions prises

Le 22 septembre, l'utilisateur a demandé un livrable qui fonctionne plutôt
qu'un raffinement sans fin, et laissé les choix de détail au développeur.
Les points 1 à 3 sont donc tranchés ici comme **hypothèses de travail
révocables** ; les points 4 et 5 restent à l'utilisateur.

1. **Régime et chronomètre** — hypothèse. Le contrat temps se juge d'abord sur
   les trois trames sans sol de 30 000 à 60 000 sites (directive du
   21 septembre, explicite sur ce régime) ; la trame brute entière vient en
   phase V9-5, comme ligne de qualification distincte : le contrat sur trame
   brute du 21 septembre n'est pas retiré et n'est pas acquis sans elle. Le chronomètre part du nuage préparé en mémoire (sites 18 bits
   distincts, masque sans sol déjà appliqué) et s'arrête à la tour complète en
   mémoire. Lecture du fichier, grille et masque sont mesurés à part et
   publiés dans le même reçu, jamais cachés (en v8 : environ 30 ms mono-thread
   de la lecture du brut au masque fermé, `morsehgp3D_v8/receipts/lidar_ground_20260921/README.md`).
2. **Format de sortie** — hypothèse. Nœuds explicites en mémoire (ordre, rayon
   carré exact, parents, verticales), convertibles sans perte en
   `CertifiedTowerInput` de `morsehgp3d/` ; la sérialisation est mesurée à
   part. Une sortie implicite ne sera envisagée que si le 100 ms l'exige, avec
   reçu.
3. **Multiplicités** — hypothèse. Sites distincts, comme la v8 : les retours de
   même position sont fusionnés avec leur compte conservé en métadonnée (aucune
   fusion à 1 mm sur les trois trames). Les multiplicités pondérées viennent
   après le premier livrable.
4. **Données KITTI et profil OS Login dans le dépôt public.** Notice de
   licence et exception documentée, ou retrait avec réécriture d'historique
   (hors du pouvoir d'un agent) ? Même question pour les archives G4 qui
   contiennent l'adresse e-mail et les clés publiques du compte. La v9 ne
   versionne aucun octet KITTI ni aucune archive hôte en attendant.
5. **Travail non commis** : sort de la tranche u18 du constructeur, du
   brouillon float32 et du delta v7.
6. **Portée de « pas de confrontation à la vérité terrain »** — tranchée le
   22 septembre, décision laissée au développeur : aucune comparaison aux
   étiquettes SemanticKITTI ; `reference/morsehgp3d_oracle`, le juge T2 de la
   v7 et les oracles rationnels servent de portes de correction sur petits
   nuages, parce qu'un livrable qui fonctionne doit pouvoir être vérifié.
7. **Échelle v6** — tranchée le 22 septembre : l'objectif du 2 septembre
   (« des dizaines de milliers de points jusqu'à des dizaines de millions »,
   mur de résidence v6 estimé vers 480 000 points à K = 10 sur 180 Gio,
   `morsehgp3D_v6/docs/ECHELLE.md` § 2) est **secondaire** et vient après les
   contrats sur les trames LiDAR sans sol.

## 10. Verrous d'architecture v8 et limites de cet audit

Les documents `morsehgp3D_v8/docs/VERROUS_ARCHITECTURE.md` (dernière
modification `785d0589`) et `PLAN_DE_REFONTE.md` (`204b0620`) sont périmés : ils
portent encore le contrat 50k et ignorent les phases 1-2 et le port 18 bits.
État réel au commit audité ([rapport 14](audit_v8/14_verrous_et_plan_de_refonte.md)) :

| verrou | état | preuve |
| --- | --- | --- |
| P0 (histogrammes locaux) | objet levé (Pool terminal O(KF)), critère de clôture jamais prononcé ; défauts de l'API restés lents | `P0_POOL_TERMINAL_Q2.md` |
| B1 recherche spatiale | partiel en q2 (fenêtre 2K, témoins hérités, opt-in, masque 1) ; ouvert en q3/q4 (recherche relancée par paire : 1,09 G visites à K5, 1,92 G à K10, scène 0) | `front.cpp:406-412`, `wspd_q34.cpp:485-492` |
| B2 carré de voisinages | partiel : graines q3 construites 179,7 M → 31,0 M ; atlas q4 inchangé | reçus `ground_phase1_20260921` |
| B3 recherche lexicographique de l'aval | ouvert, jamais commencé en v8 | `src/forest/` vide |
| B4 calendrier séquentiel de l'aval | ouvert ; en v7, exposant 1,243 contre 1,094 pour la géométrie | note v7 de découpe de la tour |
| B5 résidence | ouvert ; aucun code GPU | `src/gpu/` vide |
| packing de la WSPD à bissection au milieu | lacune de preuve ouverte, question du 13 septembre jamais répondue | canal v8 |

Ce que cet audit n'a pas couvert, soumis aux auditeurs v9
([question](../audits/QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md)) :
confrontation de la tour FULL aux Déf. 20–31 du manuscrit et à la
spécification en entier ; corps des notes q3/q4 des tranches 22 à 34 ; 15 notes
de l'auditeur complémentaire et six dossiers de l'auditeur A ; deux archives
zip du 22 septembre restées hors dépôt. Les rapports 13 à 16 sont des
lectures simples, non contre-vérifiées.
