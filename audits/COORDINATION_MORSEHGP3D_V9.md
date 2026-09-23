# Coordination Morse HGP 3D v9

Canal commun du développeur et des auditeurs indépendants de la v9, ouvert le
22 septembre 2026. Chaque entrée porte un titre daté, son auteur (rôle), le
commit de base, et se termine par les questions posées. Toute recommandation
d'audit reçoit ici une réponse écrite du développeur : acceptée, refusée avec
raison, ou différée avec échéance. Les preuves citées doivent être dans le
dépôt. Coordination d'index : un worktree par acteur ; sinon, vérifier
`git diff --cached --quiet` avant tout `git add` et n'indexer que ses propres
chemins.

## 23 septembre 2026, 05 h 35 UTC — Décision utilisateur sur la session G4 (auditeur B)

Base examinée : worktree produit `6200bb5a`, WIP MEB v3. L'utilisateur a
répondu **« Arrêt immédiat (recommandé) »** à la question portant sur une
session G4 SPOT dont le probe v3 serait rejeté par le validateur v2.
J'ai relu GCP en lecture seule, projet `devpod-gpu-exploration` : la cible
épinglée `ehgp-v7-4fa0e0789a7d5bb06b787d35` en `us-central1-b` est déjà
`TERMINATED` (`lastStopTimestamp=2026-09-22T21:50:54.848-07:00`) ; les
autres instances SPOT listées sont aussi terminées. Je n'ai donc lancé
**aucun stop supplémentaire** ni interrompu la sonde locale.
Ne pas redémarrer G4 pour ce paquet v3 avant d'aligner
`tower_probe.cpp:218` avec `tower_worker_v9.py:109,355–357`, les nouveaux
compteurs MEB, le selftest et le plan/reçu épinglé. Les sorties v3 actuelles
ne sont pas une qualification G4. Question au développeur : peux-tu
confirmer qu'aucune autre session G4 hors cible épinglée n'est en cours et
que le lancement v3 est annulé ?

Autre retour de lecture, sans bloquer la réponse G4 :
[contre-audit WIP C6/tri v6](../morsehgp3D_v9/audits/CONTRE_AUDIT_B_WIP_V6_C6_TRI_20260923.md)
publié sur `main` (`73d1d0a6`). C6 réutilise `c.tours` sans remise à zéro
(deux appels peuvent refuser à tort) et balaie L tickets à chacun des
Θ(L) tours ; le tri v6 peut terminer le processus sur échec de lancement
de fil. Ces chemins ne sont pas raccordés à v9/u18 : aucun crédit G4.
Question au développeur : comptes-tu corriger/qualifier ces prototypes
séparément avant un éventuel port, sans détourner la priorité q3/q4/FULL ?

## 23 septembre 2026, 05 h 48 UTC — Identité manquante du ledger MEB v10 (auditeur B)

Base : `78e94b04`, code et lecteur publiés. Les quatre compteurs du
MEB proposé sont désormais exportés, et les portes locales ont été
rejouées indépendamment (3/3 CTests Release ; selftest protocole normal
21/21, `-O` en cours à l'écriture). Pourtant le lecteur accepte une
sortie `complete_relative` mutée avec `meb_calls=4`,
`meb_proposals=4`, `meb_verified_proposals=2`,
`meb_proposal_fallbacks=1` : une proposition manque dans le ledger.
Dans un appel **terminé** du code actuel, chaque proposition vérifiée
retourne via la coquille exacte ; sinon elle tombe sur le repli. La
canonisation d'un support vérifié ne peut manquer ce même support dans
son bord exact. Il devrait donc tenir
`meb_proposals = meb_verified_proposals + meb_proposal_fallbacks` pour
`complete_relative`, en plus des bornes individuelles actuelles.
Garder les sorties de refus partielles distinctes ; leur compte peut
être interrompu. Question au développeur : peux-tu confirmer cet
invariant, l'ajouter au lecteur et tuer une mutation causale qui
sous-compte seulement un des deux termes ?

## 22 septembre 2026 — Ouverture (développeur sortant de la v8)

Base : `origin/main` 12294241. Cadre : `exploration_v9_hors_registre`,
`backend=none`, `quantized_u18_input_only`, `ouverture_audit_v8_et_v7`,
`not_claimed`. GCP non utilisé.

Sur demande de l'utilisateur, audit général de la v8 et de ce qui était bon en
v7, puis ouverture du dossier minimal `morsehgp3D_v9/`. L'audit a été mené par
douze lentilles contre-vérifiées ; la suite CTest de la v8 a été rejouée au
commit audité (129 tests verts sur 132, trois désactivés par construction).
Documents : `morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md`, `PLAN_V9.md`,
`HERITAGE_V7_V8.md`, `FAUSSES_PISTES.md`, rapports `docs/audit_v8/`, reçu
`receipts/audit_v8_20260922/`.

Constats principaux : la v8 livre un générateur exact de candidats, pas la
tour ; l'aval FULL de la v7 doit être porté (et non le fold v4, faux en
général) ; le flux seul coûte 6 à 100 fois le budget d'une seconde, dominé par
l'atlas q4 ; aucun code GPU ; tests contournables (Boost optionnel, mutations
liées à l'arbre canonique, aucune CI) ; données KITTI et profil OS Login du
compte GCP versionnés dans un dépôt public ; une tranche v8 non commise (portes
jumelles 18 bits du développeur, reprise u18 d'un « constructeur ») reste dans
l'index du worktree partagé et n'est pas incluse dans la v9. Cet audit est mené
par la lignée de session du développeur v8 : il n'est pas indépendant.

Demande détaillée au binôme d'auditeurs, en deux lots (mathématiques et
moteur ; mesures, plan et hygiène) :
`morsehgp3D_v9/audits/QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md`.
En résumé :

1. Contre-prouver les bornes 18 bits (`a74e90f2`) et le certificat d'atlas
   pour q3 (`0948d2d0`) avant leur port.
2. Relire le choix d'objet : tour FULL v7 avec extension non régulière, et
   domaine exact annoncé (plateaux cosphériques, multiplicités).
3. Juger le plan de phases et ses portes : manque-t-il une porte d'entrée
   ou un critère d'arrêt ?

## 22 septembre 2026, soir — Compléments d'ouverture (développeur sortant de la v8)

Base : `3595725a`. GCP non utilisé.

Une critique de complétude et quatre lectures complémentaires (rapports 13 à
16 de `morsehgp3D_v9/docs/audit_v8/` : canal v8, verrous B1–B5, entrées v8,
carte des 78 sources) ont corrigé l'ouverture. Changements :

- **Plan** : q2 et q3/q4 derrière un seul appel public mais sur **deux
  fronts**, car la fenêtre 2K et l'héritage de témoins ne sont qualifiés que
  pour q2 (`morsehgp3D_v8/src/wspd/front.cpp:406-412`) ; différentiels tour
  v9/v7 et flux v9/v8 en sortie de V9-1 ; V9-2 jugée sur le CPU total ;
  V9-3 parallélise aussi l'aval (verrou B4).
- **Décisions** : l'utilisateur a laissé les détails au développeur (« le but
  est d'avoir un livrable qui fonctionne ») ; oracles de correction bornés
  utilisés comme portes, étiquettes exclues ; multi-millions après les
  contrats LiDAR ; régime, chronomètre, sortie et multiplicités fixés comme
  hypothèses révocables (synthèse § 9).
- **Héritage et fausses pistes** : arbre de plages du nuage non porté ;
  sources du chemin mesuré ajoutées ; quatre fermetures reclassées
  « différées » ou « réserve de méthode » ; obligations de l'aval et fixtures
  à graver listées.
- **Preuve rapatriée** : artefact Actions 10704262200 (expiration le
  6 octobre), sans trames KITTI, dans
  `morsehgp3D_v9/receipts/actions_artifact_lidar_rectangles_20260922/`.

La demande de contre-audit reste la même, complétée par ce que l'audit n'a pas
lu (`morsehgp3D_v9/audits/QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md`).

### Réponse aux contre-audits de l'auditeur A (`0674dc02`)

Merci. Constat par constat, avec la modification faite dans le même commit :

| constat A | réponse | où |
| --- | --- | --- |
| Catalogue d'entrée FULL exact (`q_min` recalculé, une boule par clé, incidences de support) | accepté | plan V9-1, héritage § 3 |
| Gardes de clé u16 à remplacer avant `BallKey::power` (A < 2^76, \|B\| < 2^96, \|C\| < 2^116 pour q3) ; comparateur U192/U320 suffisant | accepté ; borne q4 à établir par le développeur | plan V9-1, héritage § 3 |
| Coquille ≤ 12 = refus de domaine ; statuts `exact_full_regular`, `exact_full_quotient_certified`, refus | accepté ; ma ligne « le triangle rectangle doit être accepté » est corrigée | héritage § 4, plan V9-1 |
| Portée du théorème FULL : conditionnel horizontal, verticales non prouvées | accepté | synthèse § 7, héritage § 1 |
| Certificat d'atlas q3 correct ; preuve de `root_lane_skips` ; juge ciblé manquant | accepté, la preuve est citée et le juge ajouté aux fixtures de V9-1 | héritage § 1, plan V9-1 |
| Note 18 bits : huit énoncés faux ; 42 bornes du code vraies ; deux commentaires périmés | accepté ; la note n'est plus une source | héritage § 2 |
| Passkey de `Q4LocalFragment::Key`, `cpu_clock_valid` | accepté | héritage § 2, plan V9-0 |
| Facteurs ×6–×101 = scénarios conditionnels ; 0,49 compare W48 et W4 ; « 129 exécutés et verts » ; masses de l'atlas non additionnables | accepté | synthèse § 4 |
| Porte de V9-1 trop lourde : un reçu FULL (ou un échec borné) sur une trame entière ouvre V9-2 ; six lignes avant tout claim | accepté | plan V9-1 |
| Sortie de V9-2 : travail total hors sortie et grand-livre complet | accepté | plan V9-2 |
| Route k-Gabriel locale par miniballes comme levier prioritaire, invariant de génération à prouver, repli v8 exhaustif | accepté comme candidat prioritaire de V9-2 | plan V9-2 |
| Contexte d'arête possédé, tâches bloc de graines × témoins / cellules | accepté | plan V9-3 |
| Deux lignes de qualification, sans sol et brute | accepté ; l'ordre reste sans sol d'abord (directive du 21 septembre) | plan V9-5, synthèse § 9 |
| « Le float32 original demeure le défaut d'entrée » | précisé : depuis la réponse « Oui, continuons en entier 18 bits » du 22 septembre, le profil de la v9 est `quantized_u18_input_only` ; le float32 sans perte reste un profil qualifié, hors contrat temps et dormant | synthèse § 2 |
| Reçus : sources de l'appelant, répétitions brutes, schéma de champs, contrôleur qui inspecte les archives | accepté | plan V9-0 |
| Cascade non rejouable depuis Git ; collectif LiDAR sur échantillon | accepté ; l'artefact Actions 10704262200 est rapatrié, les zips restent introuvables | reçu `actions_artifact_lidar_rectangles_20260922` |

Aucun refus. GCP non utilisé.

## 22 septembre 2026, nuit — Premier moteur et première base de temps (développeur)

Base : `d2700314` (premier moteur v9). GCP non utilisé pour ces mesures.

- Chaîne générateur exact (port v8) → catalogue recoupé → tour FULL (port v7
  à 18 bits), 22 CTests verts dont le juge T2 sur la chaîne réelle et la porte
  arithmétique 18 bits. Provenance : `morsehgp3D_v9/docs/PROVENANCE.md`.
- Base de temps à reçu, trois trames sans sol 1 mm, K5 et K10, huit fils :
  `morsehgp3D_v9/receipts/first_tower_20260922/`. Aucune coquille au-delà de 12
  (maximum 5), aucune divergence entre les deux implémentations.
- Remarques aux auditeurs : la contre-lecture A § 1 (clés u18, statuts de
  domaine) est appliquée ; la porte arithmétique confirme le débordement du
  plateau v7 à 18 bits que les formules laissaient prévoir. Les notes B sur
  l'ordre pré-atlas et les grandes coquilles sont retenues pour V9-2 ; les
  grandes coquilles ne se présentent pas sur ces trois trames.
- Protocole G4 v9 (`gcp-migration/tower_*_v9.py`) : dérivé explicite du
  protocole v8, mêmes primitives épinglées ; seule la durée d'arrêt invité
  passe de 30 à 40 min, sous la même garde GCE de 3 600 s. Relecture bienvenue
  avant ou après la première session.

## 22 septembre 2026, nuit — Première session G4 (développeur)

GCP utilisé : une session SPOT gardée, cible fixe g4-standard-48,
`TERMINATED` certifié et relu. Paquet construit depuis `e28296bb`, après les
correctifs demandés par la relecture B du protocole (plan plafonné à 48 fils).
Reçu : `morsehgp3D_v9/receipts/g4_tower_r1_20260922/` (sorties VM, reçu hôte,
journaux expurgés de l'adresse du compte ; sorties OS Login non versionnées).
Tour complète à 48 fils : 15–29 s à K5, 82–125 s à K10 ; condensés identiques au
local et entre nombres de fils. Au lecteur de contrat : exiger
`status=completed`, huit cas `complete_relative` et `run_tower=true` (ici
satisfaits), une seule répétition par cas.

## 23 septembre 2026, nuit — Session G4 R2, protocole v4, census q3 sur feuille (développeur)

GCP utilisé : une session SPOT gardée (paquet `0b29b6c3`), `TERMINATED`
certifié et relu. Reçu : `morsehgp3D_v9/receipts/g4_tower_r2_20260923/`,
statut **`probe_failed`** : le validateur du worker a refusé les treize sorties
(`meb_accounting`, `meb_supports_by_size`), comme le contre-audit B l'avait
prédit pendant la session. Sorties brutes toutes `complete_relative`,
condensés identiques à R1 ; chronos exploratoires seulement.

Réponses aux contre-audits B du 23 septembre :

- G4 R2 préflight et port v3 : schéma de sonde v4 ; champs MEB typés et
  épinglés ; `atlas_saturate_deep` et `q3_leaf_census` épinglés par cas
  (`saturate_deep`, `q3_leaf`), passés à la sonde et relus ; plan par défaut
  sur la tour statique à W fils ; somme des sous-chronos ≤ total ≤ mur externe
  (worker et lecteur hôte) ; arrêt de campagne au premier défaut de
  protocole (`skipped_protocol_defect`) ; porte CTest
  `mhgp9_probe_worker_contract_{normal,optimized}` : vraie sonde, queue
  d'arguments exacte d'un cas G4, onze mutants de schéma, identité d'objet
  voies allumées/éteintes. Commentaire du défaut `atlas_saturate_deep`
  corrigé : contrat v9 = activé par défaut, publié et épinglé. Restent
  ouverts : recertification des blobs du commit par le lecteur hôte, clôture
  des groupes tués (`group_closed`).
- Census q3 sur feuille (WIP) : l'option est désormais refusée sans la
  consultation de l'atlas sur Local28 ; la porte `wspd_q34` exerce la voie
  feuille sur toutes ses fixtures contre l'oracle rationnel, avec votre
  fixture K3 (racine profonde exacte au compte 1, servie par un fragment
  conservé : 4 census de feuille sur cette fixture), des identités de registre
  généralisées et deux mutants causaux. Le coût de frontière (610 M tests)
  reste à payer dans les diagnostics 8k/16k/32k ; aucune vitesse relative
  n'est revendiquée.
- Seuil K−2 pour les arêtes q4 seules (auditeur A) : retenu pour la suite,
  avec le registre par masque demandé avant de le prioriser.

## 23 septembre 2026, nuit (suite) — Certificat de voie morte, protocole v5 (développeur)

GCP non utilisé pour ces mesures. Reçu local :
`morsehgp3D_v9/receipts/q34_dead_edges_20260923/`.

- Mesure : 91 % du temps q3/q4 (08/000000, K5) va aux arêtes qui n'émettent
  rien ; leurs covers font 1 453 sites contre 43 pour les arêtes vivantes.
- Certificat (`src/gen/lanes/q34_dead_lanes.hpp`, preuve en tête de fichier) :
  disques $|c-m|^2 \le |ab|^2/12$ (q3) et $|ab|^2/8$ (q4), cellules dyadiques
  fermées, T = K−1 / K−2 intérieurs uniformes stricts, frontière héritée ;
  échec = voie exacte inchangée. Porte `wspd_q34` avec et sans certificat
  contre l'oracle rationnel, cinq mutants causaux dont un disque rétréci tué
  par une fixture q3 quasi équilatérale. Condensés identiques sur trois trames
  à K5 et K10.
- Réponse au contre-audit B des voies mortes : le coût de chargement des
  formes est réel (Σ sites de cover) mais inférieur à ce qu'il épargne :
  592 → 205 CPU·s à K5 et 1 600 → 557 CPU·s à K10 sur 08/000000 (prototype à
  balayage linéaire, même hôte, sans charge concurrente notable pour K5).
  La sonde publie désormais `dead_loads`, `dead_form_sites` et les cellules ;
  le pic de tampon d'arête inclut les capacités du certificat ; `load()`
  invalide l'état avant de recharger. L'ablation appariée on/off sur G4 est
  la prochaine session.
- Réponse au contre-audit B du protocole v4 : les sept mutations acceptées
  sont refusées (clés exactes de `generator`, `ledger`, `catalogue`,
  `tower_work`, histogrammes de longueur fixe) et figurent dans la porte
  réelle (19 mutants) et les selftests ; preflight natif obligatoire dans le
  worker (vraie sonde, 1 500 sites, voies du premier cas) et rejugé par le
  lecteur hôte ; résumés par cas recalculés depuis stdout, GNU time et
  l'enregistrement de commande ; campagne sans tour complète refusée ;
  paquet reconstruit octet pour octet depuis les objets Git du commit
  déclaré avant tout démarrage ; tolérance mur/chrono ramenée à 50 ms ;
  délai interne de la porte réelle 240 s avec fermeture du groupe ; CI
  déclenchée par les quatre scripts de protocole.

## 23 septembre 2026, nuit (fin) — Session G4 R3 : ablation appariée (développeur)

GCP utilisé : une session SPOT gardée (paquet `b4e480fc`, protocole v5
committé), `TERMINATED` certifié et relu. Reçu :
`morsehgp3D_v9/receipts/g4_tower_r3_20260923/`, statut **`completed`**
(quatorze cas `complete_relative`, preflight natif accepté, comparaisons
d'objet toutes égales). Seul levier variant entre cas appariés : le
certificat de voie morte. q3/q4 ÷2,0 à ÷2,6 à K5 et ÷2,3 à ÷3,0 à K10 ;
CPU total ÷2,3 à ÷3,4 ; condensés égaux à R1/R2. La tour domine désormais
K10 et plafonne de 24 à 48 fils : chantier suivant, sa partie séquentielle.
Une répétition par cas apparié ; aucune qualification de contrat.

## 23 septembre 2026, 02 h — Partie séquentielle de la tour (développeur)

GCP non utilisé. Correction : les « 91 % » d'arêtes sans sortie des entrées
précédentes sont faux ; lire 86 % des cycles instrumentés (97 % hors filtre
de paires), cf. `receipts/q34_dead_edges_20260923/ERRATUM.md` (merci B).

Tour FULL, voie statique : facettes représentatives sans allocation (tampon
fixe), cibles statiques consommées sans recopier ni retrier les facettes,
validation du catalogue en deux passes (contrôles locaux en parallèle, puis
plateaux étendus, fenêtres et comptes en série ; échec rapporté au premier
indice comme en série), collecte des requêtes par blocs parallèles
concaténés dans l'ordre du programme, tris par clé et par niveau parallèles
(ordres stricts et totaux : permutation unique, `parallel_sort`), forêts des
K ordres construites en parallèle. Condensé inchangé sur 000100 K10 ; tour
locale W8 42,6 → 28,0 s sur hôte partagé (chronos indicatifs). Réponse au
contre-audit B de la préparation parallèle : le pic du tri compte le tampon
de fusion (`static_peak_request_bytes` double si plusieurs fils), les
tampons des forêts sont libérés dans chaque tâche, porte
`mhgp9_tower_parallel_sort` (permutation égale à `std::sort`, 400 cas, 1 à 48
fils, tranches impaires) et son mutant compilé tué, porte
`mhgp9_chain_static_paths` (1 500 sites, 100 407 requêtes statiques, voies
temporelle et statique 1/4/8 fils : même tour, condensé
`73490cf88c02af30` égal au preflight G4 R3). Reste séquentiel : les lots
(≈ 12 s en W8 local), la banque (2 s).

## 23 septembre 2026, 03 h — Cache des témoins, preuve combinée, leviers (développeur)

GCP non utilisé. Répartition mesurée du q3/q4 après certificat (08/000000
K5, W8, cycles `rdtsc` indicatifs) : filtre de paires 31 %, certificat 33 %
(version à frontière héritée, plus coûteuse en cycles que le prototype
linéaire malgré moins de tests : confirmé, B avait raison), atlas 8 %, voies
q3/q4 15 %, filtre de rectangles 11 %.

- Certificat : les voies q3 et q4 sont prouvées en une seule récursion.
- Cache des nœuds témoins du filtre de paires (preuve dans
  `lanes/q34_witness_search.hpp`) : même admission exacte que la recherche,
  crédit par voie enregistrée seulement (une première version créditait
  toutes les voies et changeait le flux sur la trame réelle : mutant
  `witness_cache_all_lanes`, tué par la nouvelle porte). 70 % / 61 % / 67 %
  des paires rejetées sans recherche à K5 ; condensés inchangés.
- Protocole v6 : `levers` épinglés par cas, sonde `--lever=NOM=0|1`, clés
  exactes ; porte réelle à 20 mutants.

## 23 septembre 2026, 03 h 30 — Réception G4 durcie (développeur)

Réponse au contre-audit B de la réception v5 et du cache (GCP non utilisé) :
groupe de processus d'un cas tué **certifié fermé** sinon la campagne échoue
(worker) et la réception refuse (hôte) ; reçu lié à la cible, à la
génération démarrée et à la provenance du paquet, `guard_evidence.json`
rejugé ; préflight non vacant (travail géométrique et chaque levier actif
exercé) et son GNU time rejugé côté hôte ; identités exactes du registre
pour toute tour complète (paires = covers + rejets, présentations
générateur = catalogue, histogrammes, formes du certificat, voies ouvertes
= arêtes, compteurs nuls quand un levier est éteint) avec sept mutations
refusées dans les selftests ; nœuds témoins en double sur une voie refusés
par l'API publique du cache (fixture dans `q34_witness_cache`). Plan R4 :
ablation du **seul** cache à trame, K, s, W et tour identiques.

## 23 septembre 2026, 03 h — Sessions G4 R4 (préemptée) et R4b (développeur)

GCP utilisé : deux sessions SPOT gardées sur le paquet `a1d7a9bc`
(réception v6 durcie), chacune `TERMINATED` certifiée et relue. R4 : préemptée
par GCE (`compute.instances.preempted`) 18 s après le lancement du worker,
aucune sonde, reçu `receipts/g4_tower_r4_preempted_20260923/`. R4b :
**`completed`**, treize cas, ablation du seul cache à trame/K/s/W/tour
identiques, reçu `receipts/g4_tower_r4b_20260923/`. Le dénominateur des 61–70 %
est précisé (paires résiduelles développées après le filtre de rectangles).
Le gain du cache est modeste (CPU −5 à −19 %, mur q3/q4 −2 à −8 %) ; la tour
K10 passe de 17,5–22,8 s (R3) à 11,4–14,9 s (préparation parallèle), comparaison
entre sessions.

## 23 septembre 2026, 03 h 10 — Tour : ordres K en parallèle (développeur)

GCP non utilisé. Sur la voie statique à plusieurs fils, les K ordres sont
construits concurremment : cibles statiques de chaque ordre (phase 0,
parallèle dans l'ordre), lots de chaque ordre (phase A, un ordre par tâche),
identifiants de populations dans l'ordre exact de la construction séquentielle
(phase B, puis lignes en parallèle), images verticales de l'ordre K depuis
l'histoire et les ancres achevées de K−1 dans l'ordre des nœuds (phase C,
coupes monotones comme en série). Même brouillon, même banque, mêmes forêts :
la voie statique à un fil garde la boucle séquentielle et la porte
`mhgp9_chain_static_paths` compare temporelle / statique 1 / 4 / 8 fils
(même condensé). Banque : lignes déplacées (plus de copie de deux vecteurs
par ligne), validation parallèle, premier défaut au plus petit indice.
Condensés inchangés à K10 sur les trois trames ; tour W8 locale 26 → 18 s
(000100), RSS 3,48 → 3,38 Go.

## 23 septembre 2026, 03 h 30 — Réponse au contre-audit B des ordres parallèles (développeur)

1. Échecs : chaque ordre a son emplacement d'échec (`Failure`) dans les
   phases A et C ; le plus petit K est relancé, comme la boucle séquentielle
   (les autres exceptions se propagent comme avant).
2. Résidence : les cibles, ancres (u32) et histoires de tous les K
   co-résident pendant les phases A–C ; aucun compteur ne les enveloppe
   encore, le pic RSS de la sonde est la mesure publiée (000100 K10 W8 :
   3,38 Go, contre 3,48 Go avant la banque déplacée). La R5 publiera RSS et
   temps par phase.
3. Nœuds en u32 : refus explicite à 2^32−1 nœuds par ordre (≈ 1,2 G nœuds
   par ordre à 30 M sites et K10) ; documenté à la déclaration.
4. Banque déplacée : lancement de fils, allocation et longueur deviennent
   des statuts `kResourceExhausted` (plus d'exception par l'API publique) ;
   porte `mhgp9_tower_population_bank` (compilée `MHGP9_TESTING`) : égalité
   avec la surcharge copiante à 0–8 fils, quatre lignes invalides refusées
   pareil, échec de lancement injecté → statut.

## 23 septembre 2026, 03 h 40 — Réponse au contre-audit B du chargement des formes (développeur)

Les coordonnées en ordre de rang spatial appartiennent désormais à l'index
(`Q2CensusIndex::spatial_points()`, construites une fois avec lui, comptées
dans `retained_bytes`) : plus de copie par worker (O(n) et non O(nW)), plus
d'adresse d'index nue dans le certificat. `load()` invalide l'état dès son
entrée. Formes physiquement calculées = `dead_form_sites + 2·dead_loads`
(les deux extrémités, formes nulles) : identité dérivable, pas de nouveau
compteur.

## 23 septembre 2026, 03 h 45 — Filtre par ligne a × B : constat négatif (développeur)

GCP non utilisé. Prototype hors dépôt de la proposition B (appel
`filter_q34_witnesses(singleton(a), box(B), …)` avant l'expansion de chaque
ligne, bornes Affine), sur 08/000000 K5, W8, flux identique (condensé du
harnais `f31e41f3e76e03a9`) : la ligne rejette 15–17 M des 23,7 M paires
résiduelles, le filtre de paires tombe de 114 à 57 G cycles, mais le filtre de
ligne coûte 49–87 G cycles ; CPU total 156 → 164–171 s (hôte partagé,
indicatif). Avec le cache des nœuds témoins déjà en place, la ligne ne paie
pas : piste écartée en l'état (pourrait revenir si ses bornes de boîte B
devenaient beaucoup moins chères).

## 23 septembre 2026, 03 h 50 — Session G4 R5 (développeur)

**GCP utilisé**, session gardée `completed`, arrêt ciblé certifié,
`TERMINATED` relu. Paquet `aae9da0e` (inclut `684d8fc7`, `133c8653`,
`47f8a5da`). Reçu : `morsehgp3D_v9/receipts/g4_tower_r5_20260923/`. Treize
cas `complete_relative`, sept comparaisons d'objet égales, condensés égaux à
R1–R4b. 48 fils : K5 **4,33 / 5,98 / 7,38 s**, K10 **12,09 / 17,37 / 19,57 s**
(000100 / 000000 / 000200) ; tour K10 5,4 s contre 14,9 s en R4b. 000000 K10
à 24 fils : 21,6 s (les 48 fils sont 24 cœurs × 2). Réponse au contre-audit B
(`133c8653`) : prochaine livraison, la porte causale « deux `Failure` à deux
K », avec une priorité par K sur les deux phases (aujourd'hui une panne de
lots à K5 masque une panne d'images à K2 ; la boucle séquentielle rendrait
K2).

## 23 septembre 2026, 04 h 30 — Priorité des échecs FULL, bilan après échec, propriétaire du certificat (développeur)

GCP non utilisé. Réponses à A (`a4c7007b`, `e25c78e5`) et B (`2414800f`) :

- **Priorité globale par K** (A, B) : quand les lots de l'ordre f échouent,
  les images des ordres K < f (lots réussis) sont encore résolues ; l'échec
  rendu est celui du plus petit K sur les deux phases, comme la boucle
  séquentielle. Les populations ne sont numérotées que si tous les lots
  réussissent.
- **Bilan après échec** (A) : les compteurs privés des ordres sont fusionnés
  exactement une fois, au succès comme à toute sortie d'exception (drapeau
  posé avant la fusion : un échec pendant ou après elle ne refusionne pas).
  Nouveau compteur `parallel_orders` (non publié par la sonde).
- Porte `mhgp9_chain_order_failure_priority` : points de panne
  `MHGP9_TESTING` en fin de lots et d'images de chaque K (chaîne recompilée
  dans la cible de test), six scénarios à deux pannes dont « lots K5 + images
  K2 » → images K2 et « lots K4 + images K3 » → images K3, identiques en
  séquentiel (statique 1) et en ordres concurrents (4 et 8 fils) ; naissances
  et contributions non nulles après échec ; plancher `parallel_orders = 5`.
  Mutants compilés tués (code 1) : `PHASE_PRIORITY` (l'ancienne priorité par
  phase) et `DROP_FAILED_STATS` (l'ancien bilan perdu).
- **Propriétaire du certificat de voie morte** (A) : porte produit
  `mhgp9_gen_q34_dead_lanes_owner`, qui reprend le reproducteur ABA de l'audit
  (adresse d'index effectivement réutilisée localement, masque périmé 2 contre
  neuf 0), 372 arêtes alternées entre deux nuages sur un seul prouveur contre
  un prouveur neuf, `load()` interrompu par un `bad_alloc` injecté (opérateur
  `new` de la porte) puis `prove()` refusé et rechargement conforme, `prove()`
  sans `load()` refusé. Mutant `dead_load_keeps_loaded` tué.
- Ticket de nœuds admis par le rectangle (A) : d'accord avec la lecture
  « 0,83 à 1,06 crédit par rectangle » ; avec la perte nette mesurée du filtre
  par ligne (03 h 45), la piste reste fermée sans partage conjoint du préfixe.
- Copie `spatial_points()` payée en q2 seul (B) : 12·n octets, comptés dans
  `retained_bytes()` ; pas de chrono dédié, la copie est une passe O(n) de
  l'index (0,01 s à 40 k sites dans `gen_index` de R5).

Suite de portes locale `-L gate` : **119/119** (une désactivée préexistante).

## 23 septembre 2026, 05 h 20 — Noyau diamétral du certificat de voie morte (développeur)

GCP non utilisé. Levier `q34_dead_core` (défaut) : le certificat est tenté
d'abord sur la boule diamétrale fermée $|2z-a-b|^2 \le |b-a|^2$, puis sur le
cover seulement pour les voies restées ouvertes. Correct parce que le
certificat ne crédite que des sites strictement intérieurs à toutes les
boules d'une cellule : tout sous-ensemble du cover donne une preuve valide
(moins de crédits). Mesure hors dépôt (harnais q3/q4 seul, W8, hôte partagé,
08/000000) : masse de formes 2,96 G → 0,61 G à K5 ; Gcycles de la preuve
144 → 70 ; CPU 158 → 130 s à K5, 468 → 391 s à K10 ; flux identiques
(`f31e41f3e76e03a9`, `c1437caaf8278f22`). Rayons essayés : 0,3 L (1,5 M
sites, presque rien de prouvé), 0,4 L (44 M), 0,5 L (360 M, meilleur),
0,6 L (1,12 G), 0,7 L. La masse est concentrée loin du milieu : le filtre de
paires a déjà échoué près de lui.

Ventilation K10 avec noyau (Gcycles, 8 fils) : atlas + q3/q4 552 (53 %),
filtre de paires 175, rectangles 120, noyau 112, preuve sur cover 79, cover
18. Le prochain poste q3/q4 est l'énumération elle-même.

Contre-audit B R5 (`9d76d157`) : l'écart R4b → R5 de `dead_uniform_tests`
n'est pas un effet d'ordonnancement. `47f8a5da` charge a et b avec des formes
nulles : environ deux tests de plus par cellule au premier niveau testé
(+3,7 à +4,1 par chargement sur les six cas), cellules et chargements
identiques.

Protocole : sonde `mhgp9_tower_probe_v7`, plan `mhgp9_tower_plan_v5`, cinq
leviers (`q34_dead_core` exige `q34_dead_lanes`, refus au plan), douze
compteurs du noyau avec identités exactes : noyaux = covers + arêtes closes,
expansées = covers + closes + rejetées, formes du noyau = sites − 2 × noyaux,
voies ouvertes après noyau = prouvées + ouvertes sur le cover ; tout à zéro
levier coupé. Portes : `wspd_q34` (variante noyau, planchers dont « voie
prouvée sur le cover après échec du noyau »), `q34_cover` (appartenance
exacte, inclusion, extrémités), deux mutants tués. `-L gate` : **121/121**.

## 23 septembre 2026, 06 h 00 — Réponse aux contre-audits B du noyau et de la réception (développeur)

GCP non utilisé. Réponses à `aed84902`, `190e0a7c`, `c10f43b5`, `916549f5`,
`5a3c8e10` et à `RECEPTION_V6_IDENTITES_MANQUANTES_20260923.md` :

- **Travail caché du noyau publié** (sonde `mhgp9_tower_probe_v8`) :
  `core_cover_node_visits`, `core_cover_bound_tests`, `core_cover_point_tests`,
  `dead_core_outside_cells`, `dead_core_deep_cells`, `dead_core_failed_cells`,
  avec `visites = tests de boîtes + tests ponctuels`, `noyaux ≤ visites` et
  classes de cellules ≤ cellules. La v7 n'a servi à aucune session.
- **Formes** : 0,61 G = 0,36 G de noyaux + 0,25 G de covers restants
  (`dead_core.form_sites + dead.form_sites`), contre 2,96 G ; précisé dans la
  provenance, toujours une indication de harnais, pas un reçu.
- **Garde de type** : `Q34EdgeCover::complete()` est faux pour le noyau et
  `require_complete_q34_cover` le refuse aux huit consommateurs (census
  d'arête et de graine, atlas, géométrie locale, fenêtre q4, peu profond,
  domaine positif, réserve de témoins) ; la porte `q34_cover` l'exige, mutant
  `diametral_core_accepted_by_consumers` tué.
- **Raccord réel** : non-vacuité du noyau (arêtes closes, voies q3 et q4
  prouvées par lui, rien levier coupé) et six mutants de plus (levier, clôture
  non comptée, visites cachées, rejets de cache sans requête, arêtes doubles
  au-delà de q3, coquille au-delà de 12) : **26/26** tués.
- **Lecteur fail-closed** (identités manquantes v6) : rejets du cache ≤
  requêtes ≤ paires développées et rejet ⇒ nœud testé ; arêtes doubles ≤
  min(q3, q4) et q3 + q4 − doubles ≤ covers ; voies prouvées + ouvertes ≤
  covers par voie, voies ouvertes + prouvées couvrent chaque cover ; classes
  de cellules ≤ cellules ; `shell_over_12 = 0`, `max_shell ≤ 12`,
  `by_shell[13..16] = 0` sous statut complet. Douze nouvelles mutations
  refusées dans les autotests.
- **Preuve de garde archivée** : `validate_received` exige génération et
  provenance (plus d'arguments optionnels), rejuge cible, schéma et
  chronologie de la marque, `MODE=poweroff` et l'échéance `USEC` dans
  ]génération, génération + 3 600 − 300]. Huit altérations de
  `guard_evidence.json` refusées dans les autotests.
- **ABA** : la réutilisation effective de l'adresse d'index est maintenant un
  plancher (code 3), sauf sous AddressSanitizer dont la quarantaine
  l'interdit par construction.

Portes locales `-L gate` : **122/122** (une désactivée préexistante).
Prochaine étape : session G4 R6, ablation appariée du noyau (même snapshot,
trois trames, K5/K10, W48) avant de conclure sur le défaut ON.

## 23 septembre 2026, 06 h 40 — Liaison exacte de la garde archivée, garde du noyau à K<3 (développeur)

GCP non utilisé. Réponses à `3f161d74`, `d5431ba4`, `0b452cda` et à
`CONTRE_AUDIT_B_RECEPTION_V8_GARDES_WIP_20260923.md` :

- L'échec `NameError` de `rewrite_guard` (fonction tombée dans la chaîne
  `FAKE_PROBE`) était réel sur `028067a3` ; corrigé par `d49c99f7`.
- **Liaison exacte** : l'hôte garde la marque et le calendrier invité qu'il a
  vérifiés avant le téléversement (`verified_guard`, publié dans le reçu hôte)
  et `validate_received(..., generation, provenance, verified_guard)` exige
  l'égalité de `guard_evidence.json` avec eux ; un calendrier changé entre
  les deux lectures est refusé. En plus : `date_utc` de la marque avant
  l'échéance `USEC`. Autotests : marque datée de 2099, calendrier plausible à
  génération + 600 s, garde vérifiée absente ou différente, tous refusés.
- **Garde du noyau à K<3** : `require_complete_q34_cover` précède maintenant
  le retour anticipé des deux surcharges directes de
  `run_q4_local_edge_candidates` ; la porte `q34_cover` exige dix refus de
  plus à K1 et K2 (census d'arête, fenêtre, peu profond, q4 locale et
  cellules de graines), mutant `diametral_core_accepted_by_inactive_local_q4`
  tué.
- Mesure locale citée par B (quart brut 1 mm de 08/000000, 30 263 sites,
  K5, W8) : CPU 72,0 → 68,6 s, mur non stable. Le gain du noyau dépend donc
  fortement de l'entrée ; l'ablation appariée G4 R6 sur les trames sans sol
  tranchera le défaut ON (repli OFF sinon).

Portes locales `-L gate` : **123/123**.

## 23 septembre 2026, 05 h 05 — Session G4 R6 et queue de chaîne (développeur)

**GCP utilisé** : session gardée `completed`, arrêt ciblé certifié,
`TERMINATED` relu. Un premier lancement a été refusé localement avant tout
appel GCP (clé 644, contrôleur exige 600), trace dans le reçu. Paquet
`78ce9fd4` (sonde v8, plan v5 avec premier cas tout ON, réception liée à la
garde vérifiée). Reçu `morsehgp3D_v9/receipts/g4_tower_r6_20260923/` :
24 cas `complete_relative`, dix-huit comparaisons d'objet égales, noyau OFF →
ON par paires entrelacées : CPU −10 à −20 %, mur −2 à −8 %, formes −77 à
−82 %, covers −55 à −59 %. Le défaut ON reste.

Queue après `tower_ms` (A, B) : mesurée localement sur 000100/K10/W8, 1,20 s
dont **1,08 s pour le condensé FNV octet à octet** de la tour publiée ; la
destruction du résultat, hors chrono, coûte 0,35 s. Le condensé est une
vérification, pas une construction : il est désormais calculé après
`total_ms` et le CPU, et publié à part (`times_ms.digest`, sonde
`mhgp9_tower_probe_v9`) ; le lecteur borne `chain_total + digest` par le mur
externe. Définition du condensé inchangée : valeurs R1–R6 comparables. Tri de
fusion des présentations par tri d'échantillonnage parallèle (emplacements
triés par ouvrier, séparateurs de clés, plages de clés rassemblées, triées et
balayées séparément ; une clé ne chevauche jamais deux plages) : 1,04 → 0,47 s
sur 000100/K10/W8 local, porte `chain_static_paths` à 1, 4 et 7 ouvriers de
chaîne avec plusieurs plages ; la tour certifie un
catalogue strictement trié par un balayage O(B) et saute son tri `by_key`
(porte `chain_static_paths` : catalogue de la chaîne certifié, même catalogue
renversé trié, même condensé).

## 23 septembre 2026, 05 h 40 — Réponse aux contre-audits du tri de fusion et de R6 (développeur)

GCP non utilisé. Réponses à `93b268e0`, `33e03b6c`, `8a0103ff`,
`bc76a597`, `4fd5de52` :

- **Tri de fusion** : le `parallel_sort` relu par B (tampon de n présentations,
  dernière fusion sérielle) est déjà remplacé dans `50690c12` par un tri
  d'échantillonnage : emplacements triés par ouvrier, plages de clés
  rassemblées, triées et balayées en parallèle ; pas de tampon global en plus
  des plages (crête ≈ emplacements + plages, comme l'ancien `all` + tampon).
- **Classement des pannes** : `run_tower_chain` rend `resource_exhausted` sur
  `std::length_error` (`chain_size_overflow`) et `std::system_error`
  (`chain_thread_launch_failed`), comme FULL. Porte causale dans
  `mhgp9_chain_order_failure_priority` : lancement de fil refusé dans le tri
  des présentations → refus de ressource, raison dédiée, temps de fusion payé
  publié, aucun résumé d'ordre.
- **Temps payé sur échec** : fusion et recensement sont chronométrés par une
  horloge de phase qui écrit aussi en sortie d'exception.
- **Échec du condensé** et tout échec : `orders` est vidé avec la tour et le
  catalogue conservé.
- **Mur externe** : le lecteur borne `read + chain_total + digest` (séquentiels
  dans la sonde, GNU time enveloppe l'exécutable) ; mutations `read` et
  `digest` au-delà du mur tuées dans le raccord réel (**28/28**). La lecture
  n'entre pas dans le contrat en mémoire. Le condensé reste synchrone dans
  l'appel : la latence de `run_tower_chain` vaut `chain_total + digest`.
- **Comparaisons R6/R7** : additionner `times_ms.digest` à `chain_total` pour
  le périmètre mural ancien ; aucun `digest_cpu_s` publié, donc pas de
  comparaison CPU·s brute (le CPU du condensé est sériel, ≈ son mur).
- **Erratum R6** : `morsehgp3D_v9/receipts/g4_tower_r6_20260923/ERRATUM.md`
  (« générateur identique » → émissions et masses identiques ;
  `q34_cover_builds` change par construction).
- Reprise du cover complet depuis le noyau (`4fd5de52`) : bornée par les
  visites observées, notée ; pas prioritaire devant un certificat avant
  expansion.

Portes locales `-L gate` : **123/123**.

## 23 septembre 2026, 06 h 30 — Tour : MEB proposé et vérifié (développeur)

GCP non utilisé. Ventilation de la tour sur le catalogue 08/000000/K10
(W8 local, 22,0 s) : cibles statiques **15,3 s** (K10 5,8 s, K9 3,5 s),
validation 1,8, lots 2,8, populations 0,7, images 0,6, finition 0,8. Dans les
cibles statiques, la résolution des facettes domine (13,5 s), avec MEB exact
154 Gcyc contre 57 pour la recherche d'intrus.

`anchor_meb_proposed` : un Welzl en double propose un support, la tentative
exacte existante le vérifie ; par unicité du MEB un support vérifié est le
MEB, tout support valide est sur son bord exact, et le premier support de
l'énumération de référence s'obtient par la même énumération restreinte au
bord. Proposition refusée → énumération complète. Les doubles ne décident
rien (aucune borne d'erreur requise). Résultat identique champ par champ
(clé, niveau, emplacements, coquille) : porte différentielle sur 28 956
ensembles dont points entiers cosphériques, variante à propositions faussées
(5 013 replis), mutant sans canonisation tué. Effet local : supports essayés
÷10 en porte, cycles MEB 154 → 70 G, tour 24,7 → 18,9 s, condensé
`ac108f7f71096c3f` inchangé. Sonde v10 (libellé MEB v3). La référence reste
jugée par le juge rationnel (`mhgp9_tower_anchor_meb`). Portes **126/126**.

Palette d'ancre (piste A, `1140c176`/`c17db454`) prototypée hors dépôt sur
08/000000/K5 : 16 à 128 voisins approchés par ancre, lignes |B| ≥ 2 à 8 ;
elle retire 8,8 à 15,4 M des 23,7 M paires avant expansion pour 1 à 7 Gcyc,
flux identique, mais le CPU q3/q4 ne baisse que d'environ 6 % (les paires
retirées étaient surtout des rejets de cache déjà bon marché). Gain réel mais
modeste : en réserve, pas porté.

## 23 septembre 2026, 06 h 55 — Clôture locale du MEB proposé avant toute campagne (développeur)

GCP non utilisé. Pris acte de l'arrêt demandé à 05 h 35 (`5a57287c`) : aucune
session n'avait été lancée ; aucune ne l'est avant clôture locale. Réponses à
`4776b8f1` et `c4f2cb8d` :

- **Levier** `tower_meb_proposal` (défaut ON) : chaîne → tour
  (`build_full_ball_tower(..., meb_proposal)`) ; OFF = énumération de
  référence partout. Sonde v10 : levier publié, libellé MEB selon le levier
  (v3 proposé, v2 référence), quatre compteurs `meb_proposals`,
  `meb_verified_proposals`, `meb_boundary_canonicalizations`,
  `meb_proposal_fallbacks` dans `tower_work`.
- **Lecteur** : clés exactes, libellé = levier, invariants
  `propositions ≤ appels MEB`, `vérifiées ≤ propositions`,
  `replis ≤ propositions`, `canonisations ≤ vérifiées` ; levier coupé →
  compteurs nuls et libellé v2 ; préflight ON → propositions et vérifiées > 0.
  Cinq mutations d'autotest, deux de plus au raccord réel (**30/30**), et
  non-vacuité réelle (vérifiées > 0 ON, propositions = 0 OFF, objets égaux).
- **Porte MEB** : fixture canonique ABCD de B (slots `[0,1,2]`, coquille 4),
  refus typés et résultat vide pour `proposals`, `verified_proposals`,
  `boundary_canonicalizations` saturés et, dans la variante à propositions
  faussées, `proposal_fallbacks` ; comparaisons sous `FE_UPWARD`,
  `FE_DOWNWARD`, `FE_TOWARDZERO` ; 34 957 ensembles au total.
- R6 et son lecteur restent figés ; aucune requalification sous v10.

Portes locales `-L gate` : **126/126**. Prochaine étape, après autotests
normal et `-O` sur le commit figé : session R7, ablation appariée ON/OFF de
`tower_meb_proposal` (mêmes entrées, sorties égales, CPU/mur et compteurs).

## 23 septembre 2026, 07 h 10 — Tentative G4 R7 : rupture de stock (développeur)

**GCP utilisé** (une demande de démarrage). Paquet `78e94b04` (sonde v10,
ablation appariée de `tower_meb_proposal`, 24 cas), autotests normal et `-O`
verts sur ce commit. GCE a refusé le démarrage : `STOCKOUT` g4-standard-48 +
RTX PRO 6000 en us-central1-b (cible épinglée, aucune autre zone essayée).
Le script gardé n'a certifié aucune génération et a refusé tout arrêt non
versionné (`shutdown_uncertified`, `start_may_have_been_requested`). Relecture
seule : `TERMINATED`, `lastStartTimestamp` inchangé depuis R6 → aucun
démarrage, rien à arrêter. Reçu sans mesure :
`morsehgp3D_v9/receipts/g4_tower_r7_stockout_20260923/`. Reprise à
l'identique quand la capacité reviendra.

## 23 septembre 2026, 07 h 20 — Étiquettes de schéma (développeur)

GCP non utilisé. Réponse à `b6b81ba4` : le levier `tower_meb_proposal` et
les quatre compteurs MEB rompent la compatibilité dans les deux sens ; les
étiquettes deviennent **`mhgp9_tower_probe_v11`** et **`mhgp9_tower_plan_v6`**,
et les anciennes (`probe_v10`, `plan_v5`) sont refusées par des mutations
d'autotest. Le paquet `78e94b04` de la tentative R7 (rupture de stock) portait
encore v10/v5 : il ne sera pas réutilisé ; la reprise construira un paquet
neuf. L'ordre d'insertion de Welzl (inverse de `power_order`) n'agit que sur
le coût de la proposition, jamais sur le résultat : mesure des deux ordres à
faire avant de le figer.

## 23 septembre 2026, 07 h 35 — Tour : index exact des clés (développeur)

GCP non utilisé. Mesure dans la tour instrumentée (08/000000/K10, W8 local,
MEB proposé) : recherches de clé 27,2 Gcyc (11,3 M `lower_bound` sur
5,5 M `BallData` de 224 octets, ≈ 2 400 cycles chacune), MEB 68,9, intrus
54,7, graines 3,3. Remplacement par un **index exact à adressage ouvert**
(`BallId + 1` par case, hachage des cinq coefficients, sondage linéaire,
construction parallèle par CAS) : la recherche compare des clés entières, une
collision coûte une sonde, jamais une mauvaise boule ; la disposition des
cases peut dépendre de l'ordonnancement, la réponse non (une case n'est
jamais vidée). Tour locale 18,5 / 17,7 s → 16,9 / 16,4 s en alternance
(≈ −8 %), condensés inchangés, `-L gate` **127/127** ; mutant « une clé
jamais indexée » tué par le juge T2 de la tour (`--static-4`). Couvre la
piste « filtre d'absence » de `34c3164f` par une variante exacte (présence et
absence) ; mémoire ≈ 4 octets × prochaine puissance de 2 ≥ 2B (64 Mo à
5,5 M boules).

## 23 septembre 2026, 06 h 08 UTC — Contrelecture de l'index et suite du ledger (auditeur B)

Audit publié sur `main` à `95e073a5` : quatre CTests FULL/index rejoués
dans un build Release isolé, dont le mutant « première clé omise » tué
causalement. La preuve d'adressage ouvert et la comparaison de clé entière
ne montrent pas de défaut de correction. Les collisions/sondages, temps de
construction et RSS par trame restent à mesurer ; le gain local ~8 % n'a
pas de reçu G4. La lacune du lecteur MEB signalée à 05 h 48 est encore
reproduite sur `02d55856` : une sortie `complete_relative` avec
`proposals=4`, `verified=2`, `fallbacks=1` est acceptée. Merci de
confirmer l'invariant exhaustif `proposals=verified+fallbacks` et de
tuer une mutation causale avant le prochain paquet G4. Les refus partiels
restent distincts. Aucun nouveau reçu de croissance q3/q4 appariée
8k/16k/32k n'est paru avec l'index ; le verrou pré-expansion demeure.

## 23 septembre 2026, 07 h 55 — Tour : parties sérielles de la phase statique (développeur)

GCP non utilisé. Phase des cibles statiques sur 08/000000/K10 (W8 local,
après MEB proposé et index de clés) : résolution 7,9 s sur K = 2..10, mais
aussi tris 1,35 s, concaténation **sérielle** 0,67 s, collecte 0,53 s,
graines 0,27 s, balayage **sériel** des groupes 0,17 s — parts qui ne
baissent pas avec 48 fils. Changements :

- `tower::parallel_sort` devient un **tri d'échantillonnage** (séparateurs à
  positions fixes, répartition par tranches puis tri de chaque seau en
  parallèle, sans fusion sérielle). Même contrat : pour un ordre strict et
  total, l'unique permutation triée, quel que soit le nombre de fils. Porte
  `parallel_sort` inchangée (égalité à `std::sort`, tailles et fils variés) ;
  mutant compilé renommé « un seau non trié », tué.
- Concaténation des requêtes et graines en parallèle aux décalages calculés
  (l'ordinal reste la position de la collecte séquentielle) ; départs de
  groupes trouvés par tranches en parallèle.

Condensés inchangés, `-L gate` **127/127**. Effet local dans le bruit à 8
cœurs (15,7 s contre 15,7–17,0 s), 14,2 contre 15,6 s à 48 fils
sursouscrits : l'effet attendu concerne G4.

Réponse à `95e073a5` (07 h 58) : identité **exacte** `vérifiées + replis =
propositions` pour une tour complète (issues exhaustives : proposition
refusée ou dégénérée → repli ; vérifiée → retour direct ou canonisation au
bord, qui aboutit toujours puisque le support vérifié est sur le bord ;
saturation → refus, jamais complet). Mutations « proposition non comptée »
dans l'autotest et le raccord réel (**31/31**).

## 23 septembre 2026, 08 h 15 — MEB proposé : Welzl à déplacement en tête (développeur)

GCP : session R7b en cours (paquet `8e8b83a3`, démarrage gardé réussi cette
fois). Ventilation du MEB proposé dans la tour instrumentée (08/000000/K10,
W8 local) : paire diamétrale 5,5 Gcyc, attempt q2 4,0, **Welzl récursif
43,1**, vérification exacte 14,4 dont matérialisation 7,5. La proposition
coûtait donc trois fois la vérification. Welzl à **déplacement en tête**
(Gärtner, profondeur ≤ 4, les deux extrêmes en tête de liste) : 15,8 Gcyc.
Le résultat reste celui de la vérification exacte (porte différentielle :
34 957 ensembles égaux, toutes les propositions vérifiées ; variante à
propositions faussées : 6 410 replis égaux) ; condensé `dbf799c8ed83f53f`
inchangé, 502 662 propositions toutes vérifiées sur 000100/K5. Non couvert
par R7b (paquet antérieur). `-L gate` **127/127**.

