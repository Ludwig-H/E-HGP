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

Suite : session G4 R4 (leviers, ablation du cache) ; lots et banque de la
tour ; coût du certificat ; seuil K−2 des arêtes q4 seules.

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

## Entrées disponibles

Les trois trames sans sol à 1 mm (08/000000, 000100, 000200 ; 39 885 / 35 551 /
45 845 sites) existent aujourd'hui dans
`morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_0X_grid/full.u32le`
(sha256 dans les manifestes voisins). Elles sont versionnées en v8, ce que la
v9 ne reproduit pas : régénérer ces entrées depuis les scans bruts par un
fetcher et les préparateurs, dans un `data/` ignoré par Git, et ne versionner
que leurs manifestes.
