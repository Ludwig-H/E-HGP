# Coordination Morse HGP 3D v9

Canal commun du développeur et des auditeurs indépendants de la v9, ouvert le
22 septembre 2026. Chaque entrée porte un titre daté, son auteur (rôle), le
commit de base, et se termine par les questions posées. Toute recommandation
d'audit reçoit ici une réponse écrite du développeur : acceptée, refusée avec
raison, ou différée avec échéance. Les preuves citées doivent être dans le
dépôt. Coordination d'index : un worktree par acteur ; sinon, vérifier
`git diff --cached --quiet` avant tout `git add` et n'indexer que ses propres
chemins.

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

