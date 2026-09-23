# Passation v9

22 septembre 2026. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `quantized_u18_input_only`, `not_claimed`. Aucun
statut formel modifié ; `docs/implementation_status.toml` n'est pas touché par
cette exploration hors registre.

## État courant (22 septembre, soir) : premier moteur v9

La tranche verticale de V9-1 existe et passe ses portes :

- `src/tower/` : la tour FULL de la v7 portée au domaine 18 bits ;
  `src/gen/` : le générateur exact de la v8 ; `src/chain/` : la chaîne
  générateur → catalogue recoupé → tour. Détail et empreintes :
  [provenance](docs/PROVENANCE.md).
- 20 CTests verts (`ctest --test-dir build/v9 -L gate`), dont le juge T2 de la
  v7 appliqué à la chaîne réelle (catalogue égal à l'inventaire rationnel
  exhaustif, tour égale au modèle Γ, K = 1..10, s = 8/10/12, un et quatre fils).
- Premier essai à l'échelle, **exploratoire et sans reçu** (hôte chargé, charge
  32 au départ) : trame 08/000000 sans sol à 1 mm, 39 885 sites, K = 5, huit
  fils : tour complète en 131 s de mur et 834 CPU·s, RSS 1,06 Go. Catalogue
  de 1 306 696 boules, coquilles d'au plus 5 sites (227 coquilles étendues,
  aucune au-delà de 12), aucune divergence entre les deux implémentations.
  Le générateur q3/q4 prend 117 s (89 %), la tour 11 s en un fil, q2 1,7 s.
  Le poste dominant est donc l'amont q3/q4, pas l'aval comme en v7 uniforme.

Base de temps à reçu ([`first_tower_20260922`](receipts/first_tower_20260922/README.md),
huit fils, hôte local) : tour complète en 143 / 132 / 264 s à K5 et
523 / 381 / 802 s à K10 sur les trames 000000 / 000100 / 000200 ; q3/q4 fait
73 à 93 % du mur, la tour en un fil 13 à 25 % à K10. Porte arithmétique
18 bits de la tour (`mhgp9_tower_arith_u18`) verte : elle montre que l'ancien
test de plateau i128 de la v7 rendait de vraies réponses fausses à 18 bits.
Protocole G4 v9 écrit (`gcp-migration/tower_*_v9.py`, selftests hors ligne
verts), pour une première session à 48 fils.

Première session G4 ([`g4_tower_r1_20260922`](receipts/g4_tower_r1_20260922/README.md),
48 fils, `TERMINATED` certifié) : tour complète en 18,8 / 15,1 / 29,3 s à K5
et 111,7 / 82,3 / 125,4 s à K10 ; condensés identiques au local. Le générateur
q3/q4 passe à l'échelle (×11,6 de 8 fils locaux à 48 fils G4) ; à K10 la tour
en un fil domine (56–76 s), 34 s en voie statique à 48 fils.

Noyau MEB « première paire maximale » (port du prototype qualifié de la v7) :
condensés identiques, tests de puissance −56 % à K5 et −64 % à K10, tour K10
locale 130 → 109 s. La sonde publie maintenant le registre du générateur
(`ledger`).

Deuxième session G4 ([`g4_tower_r2_20260923`](receipts/g4_tower_r2_20260923/README.md),
paquet `0b29b6c3`, `TERMINATED` certifié) : **refusée** par le validateur du
worker (`probe_failed`, champs MEB non entiers), donc sans qualification. Les
sorties brutes, toutes `complete_relative`, donnent 10,5–20,2 s à K5 et
37,1–64,4 s à K10 (48 fils, tour statique), condensés identiques à R1 ; la
tour K10 plafonne à 23 s de 24 à 48 fils. Correctif : schéma de sonde v4,
voies géométriques épinglées par cas, arrêt au premier défaut de protocole,
porte `mhgp9_probe_worker_contract` qui fait juger la vraie sonde.

Census q3 sur feuille exacte de l'atlas (levier de l'auditeur A, défaut de la
chaîne) et portes du générateur portées de la v8 (27 portes, mutants compilés) :
voir la [provenance](docs/PROVENANCE.md). Condensé identique sur 000100 K5 en
local ; bornes de census q3 −54 % mais 610 M tests ponctuels de frontière
(contre-audit B) : gain net non qualifié, parcours de frontière par boîtes à
faire.

Arêtes sans sortie ([reçu](receipts/q34_dead_edges_20260923/README.md)) :
86 % des cycles q3/q4 instrumentés (97 % hors filtre de paires) vont aux arêtes qui n'émettent rien. Le **certificat de voie
morte** (`lanes/q34_dead_lanes`, défaut de la chaîne, épinglé par la sonde v5
et le plan G4 v3) couvre le disque des centres possibles de chaque voie par
des cellules portant T intérieurs uniformes ; il divise le CPU q3/q4 par 2,9 à
K5 et K10 sur 08/000000 (prototype), condensés identiques sur les six cas.
Protocole v5 durci : schémas exacts de la sonde, preflight natif sur la vraie
sonde avant tout cas, résumés recalculés à la réception, campagne sans tour
complète refusée, paquet recertifié contre les objets Git de son commit.

Session G4 R3 ([`g4_tower_r3_20260923`](receipts/g4_tower_r3_20260923/README.md),
paquet `b4e480fc`, **`completed`**, `TERMINATED` certifié) : ablation appariée
du certificat, quatorze cas complets, preflight natif accepté. q3/q4 ÷2,0 à
÷3,0 ; chaîne complète à 48 fils **6,2 / 8,9 / 10,9 s à K5** et
**26,0 / 35,6 / 37,8 s à K10** (000100 / 000000 / 000200), condensés
inchangés. À K10 la tour domine (17,5 à 22,8 s) et plafonne dès 24 fils.

Depuis R3 : préparation parallèle de la voie statique de la tour (validation,
collecte, tris, forêts ; lots et banque restent séquentiels), preuve des deux
voies mortes en une récursion, et **cache des nœuds témoins** du filtre de
paires (70 % des paires rejetées sans recherche à K5 sur 08/000000).
Protocole v6 : tous les leviers de la chaîne sont épinglés par cas
(`levers`, `--lever=NOM=0|1`).

Session G4 R4 préemptée par GCE (aucune mesure, [reçu](receipts/g4_tower_r4_preempted_20260923/README.md)) ;
reprise R4b ([`g4_tower_r4b_20260923`](receipts/g4_tower_r4b_20260923/README.md), paquet `a1d7a9bc`,
**`completed`**) : cache −5 à −19 % de CPU, tour K10 −35 % contre R3 ; chaîne
complète **5,4 / 7,4 / 9,2 s à K5** et **19,4 / 26,7 / 29,5 s à K10** à 48 fils,
condensés inchangés.

Tour à ordres K construits en parallèle (voie statique à plusieurs fils) :
lots de chaque ordre en parallèle, identifiants de populations attribués dans
l'ordre séquentiel puis lignes construites en parallèle, images verticales de
l'ordre K depuis l'histoire achevée de K−1, banque par déplacement et
validation parallèle. Lots 12 → 2,1 s en W8 local sur 000100 K10 ; tour
26 → 18 s ; condensés inchangés sur 000000, 000100, 000200 à K10.

Session G4 R5 ([`g4_tower_r5_20260923`](receipts/g4_tower_r5_20260923/README.md),
paquet `aae9da0e`, **`completed`**, `TERMINATED` certifié) : tour K10
14,9 → 5,4 s contre R4b ; chaîne complète à 48 fils **4,3 / 6,0 / 7,4 s à K5**
et **12,1 / 17,4 / 19,6 s à K10**, condensés inchangés, deux répétitions. q3/q4
redevient le premier poste (65 à 70 % à K5). Filtre par ligne a × B
(proposition B) prototypé : perte nette, écarté (coordination, 23 septembre
03 h 45).

Échecs de la tour : priorité au plus petit K sur les phases A et C (comme la
boucle séquentielle) et bilan de travail fusionné une fois même après échec
(porte `mhgp9_chain_order_failure_priority`, deux mutants compilés). Porte
produit du propriétaire du certificat (`mhgp9_gen_q34_dead_lanes_owner` : ABA,
`load()` interrompu).

**Noyau diamétral** du certificat de voie morte (levier `q34_dead_core`,
défaut) : le certificat est d'abord tenté sur la boule diamétrale fermée de
l'arête, sous-ensemble du cover, et le cover n'est construit que pour les
voies restées ouvertes. Harnais local W8 sur 08/000000 : CPU q3/q4 −18 % à K5,
−16,5 % à K10, flux identique ; sonde W8 K5 : condensé inchangé. Protocole G4
v7 (sonde) / v5 (plan) : cinq leviers, douze compteurs du noyau et leurs
identités exactes.

Session G4 R6 ([`g4_tower_r6_20260923`](receipts/g4_tower_r6_20260923/README.md),
paquet `78ce9fd4`, **`completed`**, `TERMINATED` certifié) : ablation
appariée du noyau, 24 cas, objets égaux ON/OFF ; CPU −10 à −20 %, mur −2 à
−8 %, formes chargées −77 à −82 % ; meilleurs totaux **4,15 / 5,76 / 6,87 s à
K5** et **11,73 / 16,67 / 18,06 s à K10**. Défaut ON gardé.

Queue de chaîne : le condensé FNV de vérification (≈1,1 s local à K10) est
sorti du chrono de chaîne et publié à part (`times_ms.digest`, sonde v9) ; la
fusion des présentations est un tri d'échantillonnage parallèle par plages de
clés (1,04 → 0,47 s en W8 local à K10) ; la tour certifie par un balayage
un catalogue déjà strictement trié (`presorted_catalogues`) et ne le retrie
pas.

Tour à K10 (catalogue 08/000000, W8 local) : la phase de cibles statiques
prend 70 % de la tour, dont la résolution des facettes (MEB exact puis
recherche d'intrus). **MEB proposé** (Welzl en double vérifié exactement,
canonisé au bord, repli exact) : même résultat que l'énumération, cycles MEB
÷2,2, tour −24 % local, condensé inchangé (sonde v10, levier
`tower_meb_proposal`). Index exact par hachage des clés du catalogue à la
place des recherches dichotomiques : tour −8 % local.

Session G4 R7b ([`g4_tower_r7b_20260923`](receipts/g4_tower_r7b_20260923/README.md),
paquet `8e8b83a3`, **`completed`**, `TERMINATED` certifié ; R7 avait été
refusée par rupture de stock GCE, [reçu](receipts/g4_tower_r7_stockout_20260923/README.md)) :
ablation appariée du MEB proposé, objets égaux, tour K10 −5 à −8 %. Chaîne
hors condensé (publié à part) : **3,67 / 5,31 / 6,40 s à K5** et
**9,58 / 13,91 / 15,28 s à K10** ; tour K10 3,2–4,0 s ; fusion 0,04–0,12 s.
Depuis : Welzl à déplacement en tête (proposition ÷2,7), séparateurs
pseudo-aléatoires du tri parallèle, index de clés libéré avant la banque.

Croissance LiDAR locale ([`lidar_scaling_local_20260923`](receipts/lidar_scaling_local_20260923/README.md),
sonde v12 `4530644b`, runner v2, trois trames sans sol, K5 et K10, emboîtés
8k/16k/32k + entier + morceaux, 60 cas conformes, revalidés par le lecteur durci : [addendum](receipts/lidar_scaling_local_20260923_revalidation/README.md)) : temps de chaîne ×1,7 à
×2,8 par doublement et boules sous-linéaires, mais `core_sites` (sites
énumérés dans les cœurs diamétraux) atteint p = 2,5 à 3,05 sur un doublement
de s00 et de s02 aux deux K. Le cœur est la première cible d'échelle.
Le crédit par nœuds du certificat, décisions identiques, est mesuré puis
fermé : CPU +27 à +32 % ([reçu](receipts/dead_node_credit_negative_20260923/README.md)).

Sonde **v13** (`c768e06a`) :
- **Invariant d'Euler** du catalogue (auditeur C, preuve par le nerf de B),
  imposé par la chaîne : `chain_catalogue_euler_violated`.
- Portes : `chain_euler`, porte T2, porte `scale8000` de C avec juge
  d'échantillon.
- Occupation par ouvrier de q34, chronos par phase de la tour ; lecteur G4
  revu par une revue multi-agents (un faux refus corrigé, `515b3666`).

Session G4 R8 ([`g4_tower_r8_20260923`](receipts/g4_tower_r8_20260923/README.md),
paquet `515b3666`, **`completed`**, `TERMINATED` certifié) :
- Euler « holds » sur les trois trames réelles.
- Chaîne K5 3,67 / 5,46 / 6,39 s, K10 9,40 / 13,69 / 15,19 s.
- **q34 affamé à W48** : 35 à 49 % du temps des fils à K5 passe à attendre
  la file de tâches (jobs du front trop gros).
- Tour K10 : phase 0 1,1–1,5 s, lots séquentiels 0,8–1,2 s.
- s = 8 reste le meilleur choix, devant s = 10 et 12.

Session G4 R9 ([`g4_tower_r9_20260923`](receipts/g4_tower_r9_20260923/README.md),
paquet `fe1142b5`, **`completed`**, `TERMINATED` certifié) : jobs du front
préparés par masse et 4× plus fins (sonde v14). **Chaîne K5 2,80 / 3,80 /
3,99 s**, **K10 8,60 / 11,75 / 11,89 s**, condensés égaux ON/OFF. Sonde
**v15** : phase A de la tour recouvrant la phase 0 (`tower_overlap_static`).

Session G4 R10 ([`g4_tower_r10_20260923`](receipts/g4_tower_r10_20260923/README.md),
paquet `33d51efd`, **`completed`**, `TERMINATED` certifié) : recouvrement de
la tour, avec la tour K10 −0,46 à −0,76 s. **Chaîne K5 2,74 / 3,64 / 3,94 s**,
**K10 8,07 / 11,13 / 11,36 s**. Validation du catalogue : passe 2 et
programmes parallélisés.

Session G4 R11 ([`g4_tower_r11_20260923`](receipts/g4_tower_r11_20260923/README.md),
paquet `f685461a`, **`completed`**, `TERMINATED` certifié) : jobs du front q2
par masse (sonde v16), q2 divisé par 3 à 4,5. **Chaîne K5 2,54 / 3,21 /
3,52 s**, **K10 7,68 / 10,36 / 10,53 s**. Juge d'échantillon des clés jamais
émises (`chain_absent_keys`) : 77 000 boules admissibles, toutes présentes, à 2k et
8k. Mesures sans suite immédiate :
- l'index des selles seul ([négatif](receipts/saddle_index_negative_20260923/README.md)) ;
- le pouvoir de preuve des voisins proches pour le certificat, 96 % des
  fermetures du cœur à K5 ([mesure](receipts/knn_core_probe_20260923/README.md)).
- le certificat sur les voisins proches **globaux** (liste kNN de 16 par
  site) : exact mais neutre, CPU q34 −2 % à K5 et +3,7 % à K10
  ([négatif](receipts/near_sites_negative_20260923/README.md)).

Profil q3/q4 par échantillonnage (`SIGPROF`) : le filtrage (front,
rectangles, paires) fait environ 40 % du CPU, le cœur et le certificat 18 %,
la génération q3/q4 25 %, sans poste au-delà de 16 %
([reçu](receipts/q34_micro_levers_20260923/README.md)). Aucun micro-levier
retenu : pas +1 des compteurs non contrôlé (−4 %, mais contrat public de
dépassement changé), compteurs locaux du DFS (−0,9 %), raffinement des
rectangles et cache par `b` (pertes).

Voie GPU S1 (23 septembre après-midi) : filtre témoin exact q3/q4 porté
hôte/device (`src/gpu/`), protocole G4 dédié `gcp-migration/gpu_filter_*_v9.py`.
Session G4 S1 ([reçu](receipts/g4_gpu_s1_20260923/README.md), paquet
`6e0e43a0`, **`completed`**, `TERMINATED` certifié) : sur le sous-nuage
08/000000 **sans sol** à K5, les filtres rectangles et paires sans cache
**hors construction du front WSPD** (3,13 M rectangles, 23,7 M paires)
tiennent en
**63,8 ms** sur la RTX PRO 6000, contre 1,18 s au CPU à 48 fils avec cache
(×18 à ×24 sur les six cas). Masques et totaux de visites identiques. Seuil
S1 (0,1 s) franchi. Suite GPU : raccorder le passage à la chaîne (front CPU,
filtre GPU, survivants au CPU), puis porter le cœur et le certificat de voie
morte. La tentative 1 a échoué à la configuration (CMake 3.22.1 sur la VM,
[reçu](receipts/g4_gpu_s1_attempt1_20260923/README.md)).

Voie GPU S2 (23 septembre, fin d'après-midi) : filtre témoin q3/q4 par lots
dans la chaîne (`run_wspd_q34_batched`, leviers `q34_batch_filter` et
`q34_gpu_filter`, sonde v17, protocole de la tour v17 avec build CUDA).
Frontière de confiance à trois couches :
- contrôle structurel des survivants ;
- différentiel moteur/lots (porte `chain_batch_filter`, jumeau moteur du
  préflight et des cas GPU) ;
- juges indépendants de C.

Session G4 R12 ([reçu](receipts/g4_tower_r12_20260923/README.md), paquet
`2059189d`, **`completed`**, `TERMINATED` certifié), toutes les tours
égales à leur jumeau moteur :
- **K5 2,01 / 2,47 / 2,69 s** et **K10 6,63 / 8,32 / 8,70 s**, soit −15 à
  −25 % contre le chemin moteur ;
- appel du filtre 0,2–0,3 s, dont 47–119 ms de passe GPU, le reste côté
  hôte (contexte, copies) ;
- survivants 0,8–1,2 s à K5, tour 0,6–0,8 s.

Suites : contexte GPU et index préparés pendant q2 ; cœur et certificat sur
GPU ; tour D5.

Voie GPU S3 (23 septembre, soir) : certificats de voie morte par lots. Le
[reçu de ventilation](receipts/q34_survivor_phases_20260923/README.md)
situe la phase des survivants à 08/000000, en ticks TSC écoulés sur hôte
chargé (parts indicatives, pas des cycles CPU ;
[addendum](receipts/q34_survivor_phases_20260923/ADDENDUM_20260923.md)) :
- cœur et couverture, avec leurs certificats : environ 35 % à K5, 28 % à K10 ;
- atlas, q3 et q4 des arêtes restées vivantes : environ 65 % et 72 %.

Calculer les formes du cœur à la première consultation rapporterait de
l'ordre de 1 % (projection, non bornée) ; seule cette variante est
écartée, et sur CPU. Même en supprimant idéalement tous les survivants,
R12 garderait 1,18 à 1,51 s à K5 : S3 n'est pas le dernier levier.

Livré (sans GCP) :
- préchauffage du contexte CUDA et de l'index plat pendant q2 ;
- libellé GPU honnête (`GPU_executed` exige une tour LiDAR achevée sur
  l'appareil) ;
- phase de certificats par lots (leviers `q34_batch_certificates`,
  `q34_gpu_certificates`), avec son port portable exact, un warp par arête,
  et la mise en attente sur le CPU d'une arête trop grosse ;
- juge de l'appel des certificats, arête par arête contre la référence CPU,
  exécuté par les deux préflights G4 ;
- condensé canonique (FNV-64) du catalogue, hors chronomètre ; ce n'est
  pas une égalité littérale du catalogue ;
- sonde et protocole G4 v18, plan R13 à 18 cas (voir `docs/PROVENANCE.md`).

Local : même tour, même catalogue et même travail des certificats que le
moteur sur la trame entière 08/000000/K5.

Session G4 R13 ([reçu](receipts/g4_tower_r13_20260923/README.md), paquet
`46c50432`, **`completed`**, `TERMINATED` certifié) :
- préflights GPU jugés arête par arête contre la référence CPU : ardoise par
  défaut, puis ardoise de 64 sites (3 404 mises en attente, le compte local) ;
- les 18 cas reproduisent les six condensés épinglés par C ;
- **K5 1,74 / 2,20 / 2,35 s**, **K10 5,91 / 7,83 / 7,97 s** (−11 à −13 %
  contre R12 à K5) ;
- préchauffage : appel du filtre de 205–315 ms à 81–154 ms ;
- S3 : 211 ms d'appel (189 d'appareil) pour 351 ms retirés aux survivants à
  000000/K5. Le noyau (un warp par arête, 250 registres, 8 warps par SM) a
  encore de la marge.

Reste à 000000/K5 (2,20 s) : survivants 0,73 s (atlas, q3 et q4 des arêtes
vivantes, et 0,7 M covers reconstruits), tour 0,75 s, certificats 0,21 s,
q2 et recensement 0,20 s. Suite :
- réduire le travail des survivants : atlas, q3 et q4 sur GPU (le shadow
  à huit cellules avant cœur ferme moins de 0,14 % des formes, écarté) ;
- tour : le chemin critique à K5 est la phase A mono-fil de l'ordre K5.
  Le saut D5 seul ne gagnerait que 5 à 10 ms à K5 (100 à 125 ms à K10,
  conception multi-agents du 23 septembre) ;
- noyau S3 : compteurs par arête en u32, 128 registres, 16 warps par SM
  (`0b41e4c86`, à mesurer sur G4).

Phase A allégée (`aa29245f`,
[reçu](receipts/tower_phaseA_lean_local_20260923/README.md)) :
- rangs de plateau exacts au lieu de produits U320 par facette ;
- lots singletons sans allocation pour les blocs inertes.

Localement, la phase A de l'ordre le plus élevé baisse d'environ 30 % à K5
et de 35 à 40 % à K10 (tour K10 −10 à −16 %). Prochaine étape de la tour :
un brouillon plat (sans allocation par action publiée), puis la validation,
les images et l'encodage à K10.

Suite : tour maigre (D5 de l'auditeur C :
index des selles, saut au centre, images de naissance directes), puis
réfutation des ancres longues avant expansion (66–70 % du CPU q3/q4 selon C).

Suite : coût q3/q4 (ventilation K10 W8 locale en Gcycles : atlas 229, q4 235,
paires 187, rectangles 123, noyau 120, q3 120, preuve sur cover 86) ; seuil
K−2 des arêtes q4 seules ; équilibrage de la tour K10 ; voie GPU (V9-4).

## État du dépôt au moment de l'ouverture (historique)

- `origin/main` avant l'ouverture : **12294241** (dernier commit d'audit v8 du
  22 septembre). La v8 est gelée à ce commit pour ses sources publiées ; la v7
  n'a pas changé depuis `dc57ffd5`.
- Le commit d'ouverture v9 a été préparé dans un worktree séparé, sans toucher
  l'index du worktree partagé `/workspaces/E-HGP`.
- **Worktree partagé** : `HEAD` local à `a74e90f2`, en retard des 13 commits
  d'audit et de ce commit d'ouverture. Son index contient une tranche **non
  commise** de 89 fichiers, qui mêle le travail du développeur v8 (portes
  jumelles 18 bits et campagne d'identité u16, 06:29–06:53 UTC) et celui d'un
  « constructeur » (reprise u18 et atlas saturant, 09:58–10:56 UTC). L'arbre
  porte aussi des modifications non indexées (`AGENTS.md`, passation, journal
  et coordination v8, fichiers v6/v7) et non suivies (captures
  `u18_resume_20260922`, brouillon float32 global, notes de l'auditeur
  complémentaire du 13 septembre, `claude-install.sh`). Rien de cela n'est
  dans la v9. La tranche doit être commise en v8 (après correction de son
  lecteur de captures, qui refuse des CTests pourtant verts) ou abandonnée par
  écrit ; c'est une décision pour l'utilisateur. Toute personne qui travaille
  dans ce worktree doit d'abord vérifier `git diff --cached --quiet`.

## Ordre de lecture

1. [README](README.md), puis la [synthèse de l'audit](docs/AUDIT_V8_SYNTHESE.md).
2. [Plan](docs/PLAN_V9.md), [héritage](docs/HERITAGE_V7_V8.md),
   [fausses pistes](docs/FAUSSES_PISTES.md).
3. Selon la tâche, les [rapports détaillés](docs/audit_v8/README.md).
4. Pour l'objet : `morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md`,
   `morsehgp3D_v7/docs/TOUR_FULL_PAR_BOULES.md`, le registre
   `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, et les parties I–II du
   manuscrit (`docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`).
5. Pour le générateur : `morsehgp3D_v8/docs/ALGORITHME_EXPLIQUE.md`,
   `morsehgp3D_v8/docs/Q34_GLOBAL_ET_LIDAR_20260921.md`,
   `morsehgp3D_v8/docs/Q3_CERTIFICAT_ATLAS_20260921.md`,
   `morsehgp3D_v8/docs/ELARGISSEMENT_18_BITS_20260922.md` (le code fait foi
   sur les bornes : la note publiée contient des bornes fausses, relevées par
   l'audit, sans effet sur le code).

## Première tranche proposée

Phase V9-0 du [plan](docs/PLAN_V9.md) : manifeste CMake, CI, portes à code
exact, mutants `--inject`, type de point certifié, table de bornes, format de
reçu, données hors Git. Puis V9-1 : la tranche verticale mono de bout en bout
(générateur porté, catalogue canonique, tour FULL, lanceur chronométré) sur les
trois trames sans sol 1 mm. Le premier chiffre utile de la v9 est le temps de
la **tour complète** sur ces trames, pas celui d'un composant.

## Décisions et questions ouvertes

L'utilisateur veut un livrable qui fonctionne et laisse les choix de détail
au développeur (22 septembre). La [synthèse](docs/AUDIT_V8_SYNTHESE.md) § 9
fixe donc des hypothèses de travail révocables : contrat jugé d'abord sur les
trames sans sol, chronomètre du nuage préparé en mémoire à la tour complète en
mémoire (lecture, grille et masque mesurés à part) ; nœuds explicites
convertibles en `CertifiedTowerInput` ; sites distincts. Les oracles de
correction bornés servent de portes ; aucune étiquette SemanticKITTI.
L'échelle multi-millions vient après les contrats LiDAR. Restent à
l'utilisateur : les données KITTI et le profil OS Login versionnés dans un
dépôt public, et le sort du travail non commis d'autres acteurs. En
attendant, la v9 ne versionne aucun octet KITTI.

Rectificatif au 23 septembre : « jugé d'abord sur les trames sans sol »
désigne le **premier jalon de développement**, non une substitution au
contrat principal sur trame brute entière de plusieurs séquences. La
grille 1 mm est devenue prioritaire sur choix explicite de l'utilisateur ;
float32 reste un objectif secondaire distinct. Le reçu GPU S1 ci-dessus
mesure un filtre isolé sur sans-sol, jamais cette tour contractuelle.

## Entrées disponibles

Les trois trames sans sol à 1 mm (08/000000, 000100, 000200 ; 39 885 / 35 551 /
45 845 sites) existent aujourd'hui dans
`morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_0X_grid/full.u32le`
(sha256 dans les manifestes voisins). Elles sont versionnées en v8, ce que la
v9 ne reproduit pas : régénérer ces entrées depuis les scans bruts par un
fetcher et les préparateurs, dans un `data/` ignoré par Git, et ne versionner
que leurs manifestes.
