# Plan de la v9

22 septembre 2026. Proposition d'ouverture, tirée de l'[audit général de la
v8](AUDIT_V8_SYNTHESE.md). Le développeur de la v9 peut la réordonner, à
condition de l'écrire dans la passation et le canal de coordination. Aucune
phase n'ouvre sans sa porte d'entrée ; aucune ne se ferme sans reçu.

## Principes

1. **Un objet : la tour HGP FULL** au sens de la v7 (minima Gabriel,
   multifusions, parents, verticales, extension non régulière). Le fold v4 seul
   n'est pas ce produit. Tant que la tour n'existe pas, aucun temps ne se
   compare au contrat.
2. **Un moteur : l'entier exact à 18 bits** sur la grille 1 mm (décision
   utilisateur du 22 septembre). La voie float32 reste dormante.
3. **Un régime prioritaire : SemanticKITTI sans sol, 30 000 à 60 000 sites**,
   K5 puis K10, s = 8 (jamais moins) dans la convention de séparation
   `box_gap_diameter_v1` de la v8 (s = 8 en v8 correspond à s = 14–16 en v4),
   1 s puis 100 ms sur G4 ; une seule ablation s = 10 / 12 sur la tour. Les tailles
   8 000 / 16 000 / 32 000 restent les tailles d'intérêt pour les pentes ;
   les petites tailles restent des oracles de correction.
4. **Mesurer de bout en bout, puis réduire le nombre d'opérations**, puis
   paralléliser, puis porter sur GPU. Chaque tranche publie les compteurs des
   postes dominants avant et après.
5. **Rien d'implicite** : ports épinglés et requalifiés, défauts de la
   bibliothèque égaux à la configuration mesurée, anciens chemins dans une
   cible différentielle séparée.
6. **Un livrable qui fonctionne**, pas un raffinement sans fin (utilisateur,
   22 septembre) : les garde-fous se limitent à ce qui prouve que la tour est
   juste et que le chronomètre est honnête. Pas de comparaison aux étiquettes
   SemanticKITTI ; les oracles de correction bornés (`reference/`, T2) sont
   des portes. L'échelle multi-millions de la v6 vient **après** les
   contrats LiDAR ([synthèse](AUDIT_V8_SYNTHESE.md) § 9, points 6 et 7).

## Phase V9-0 — fondations (avant tout code de moteur)

Porte d'entrée : ce dossier d'ouverture publié ; hypothèses de travail des
points 1 à 3 de la [synthèse](AUDIT_V8_SYNTHESE.md) § 9 (régime, chronomètre,
format de sortie, multiplicités), révocables par l'utilisateur.

- `CMakeLists.txt` v9 (C++20, `-Wall -Wextra -Wpedantic -Werror`, namespace
  `mhgp9`, préfixes `mhgp9_` / `MHGP9_`), Boost **obligatoire**
  (`FATAL_ERROR`), options `MHGP9_SANITIZE` (ASan/UBSan) et `MHGP9_TSAN`.
  Boost n'est pas dans le conteneur par défaut. La CI l'installe
  (`apt-get install libboost-dev`, comme `.github/workflows/morsehgp3d-v7.yml`).
  En local, v7 et v8 ont utilisé des en-têtes extraits hors Git, sans
  privilège : `apt-get download libboost1.83-dev=1.83.0-2.1ubuntu3.2`
  (sha256 `519ecf2c64308527e15b6582955681d192a832250baa4bc424967aaf7d02d68f`),
  puis `dpkg-deb -x` vers un dossier sous `build/`, et
  `-DBOOST_ROOT=<dossier>/usr`. Le README v9 doit donner cette recette pour
  un clone neuf.
- Portes à code de sortie exact portées de la v7 (`run_expect.cmake`) ; mutants
  `--inject=` dans le produit, exécutables dans tout arbre, sanitizers compris.
- Workflow GitHub qui construit la v9 et lance `ctest -L gate --no-tests=error`
  depuis une archive des sources (portes hermétiques : aucun `git rev-parse`,
  aucun chemin absolu).
- Type de point certifié (seule la préparation du nuage le produit) et table
  de bornes vérifiée par `static_assert` générée depuis `coordinate_bits`.
- Format de reçu unique : manifeste minimal versionné, `SHA256SUMS`, contrôleur
  générique en CI qui inspecte aussi les membres des archives (aucun octet de
  scan ni sortie OS Login), plafond de taille, aucune donnée tierce ; champs
  obligatoires `input_sha256`, source et compilateur, commande, statut
  d'échec, frontières du chronomètre, `GPU_executed`, `cpu_clock_valid`,
  taille d'entrée et de sortie, charge de la machine ; sources de l'appelant
  et répétitions brutes conservées ; chaque reçu se rejoue depuis son commit et sa recette de build, sans
  dépendre d'un binaire conservé (la v8 a laissé 150 builds locaux).
- Environnement connu : la VM G4 a GCC 11 (`-Werror` a arrêté une session v8)
  et un Boost ancien ; TSan de GCC inutilisable, prendre celui de Clang ; LSan
  indisponible dans le bac à sable local.
- Données : `data/` ignoré par Git, alimenté par un fetcher paramétré
  (séquences et trames choisies **avant** les chronos) et par les
  préparateurs ; seuls les manifestes (sha256, tailles, effectifs, paramètres)
  sont versionnés.

Porte de sortie : suite verte en CI et en local ; mutant « plage de
coordonnées non vérifiée » tué ; un reçu d'exemple lu par le contrôleur.

## Phase V9-1 — tranche verticale mono, de bout en bout

Porte d'entrée : V9-0 close.

- Ports épinglés du générateur v8 (liste de l'[héritage](HERITAGE_V7_V8.md) § 2)
  en configuration mesurée unique, q2 et q3/q4 derrière **un seul appel
  public**. En interne, **deux fronts** : la fenêtre 2K et l'héritage de
  témoins ne sont qualifiés que pour q2 (`morsehgp3D_v8/src/wspd/front.cpp:406-412`).
  Un front unique multivoie n'est permis qu'après le juge des bornes Ξ.
- Catalogue canonique (dédoublonner, puis collecter ; jamais un census par
  présentation d'une même boule) : union q2 ∪ q3 ∪ q4 dédoublonnée par clé primitive,
  q_min, IDs intérieurs, coquille complète, niveau exact ordonné ; populations
  numérotées indépendamment de l'ordre de découverte.
- Tour FULL : port explicite de la sémantique de
  `morsehgp3D_v7/src/forest/full_ball_tower.hpp` (domaine requalifié à
  18 bits, coquille non plafonnée ou refus explicite), extension non
  régulière, format de sortie décidé. Nommer la voie du résolveur portée : la
  voie statique par facettes est désactivée par défaut en v7
  (`full_ball_tower.hpp:293,751`) ; publier les compteurs MEB et supports et
  juger le terminal avant normalisation. Conditions du port relevées par
  l'auditeur A (`audits/CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md` § 1) :
  - catalogue d'entrée exact : une boule par clé (dédoublonnage par clé, pas
    par couple clé-support), coquille complète, intérieur, niveau exact,
    `q_min` recalculé sur la coquille, et les incidences de support dont le
    quotient local a besoin (ou une reconstruction certifiée, coût compté) ;
  - gardes de clé u16 de la v7 (A < 2^68, |B| < 2^87, |C| < 2^105) remplacées
    **avant** tout appel de `BallKey::power` par A < 2^76, |B| < 2^96,
    |C| < 2^116 pour q3, borne q4 à établir ; le comparateur de niveaux
    U192/U320 de la v7 tient à 18 bits (bornes 2^190 et 2^278) ;
  - la coquille ≤ 12 de la v7 est un refus de domaine, pas un théorème
    (sphère de rayon 5 centrée en (5,5,5) : plus de 12 sites entiers) ;
  - trois statuts distincts : `exact_full_regular`,
    `exact_full_quotient_certified`, refus de domaine explicite ; un taux de
    doublons q4 nul ne certifie pas la régularité (triangle rectangle) ;
  - fixtures gravées ensemble : clé émise par plusieurs présentations ;
    coquille entière de plus de 12 points ; triangle rectangle et carré
    cosphérique de rang pertinent ; clé et niveaux q3 près des majorants u18,
    comparés à un oracle entier plus large ; juge ciblé de `q3_center` et de
    `certified_inside_count` (centre sur frontière fermée, site de puissance
    nulle, cellule `Outside`).
- Avant le port du constructeur : extraire des sondes existantes la
  distribution des tailles de coquille et d'intérieur sur les trois trames
  (le constructeur v7 refuse au-delà de 12 sites de coquille et de
  9 intérieurs), et distinguer, dans les doublons q4, un support répété d'un
  second support.
- Format de sortie : il est fixé par ses deux consommateurs du dépôt, à lire
  avant de le décider. La bibliothèque produit `morsehgp3d/` attend un
  `CertifiedTowerInput` (`morsehgp3d/include/morsehgp3d/api/point_hierarchy.hpp`) :
  nœuds (ordre, rayon carré exact), arêtes horizontales et verticales datées,
  (K−1)-simplexes projetables avec leurs identifiants de points et les rayons
  carrés de leurs cofaces, et reçus liés par `tower_payload_id`. La piste
  SemanticKITTI (`Zoltan/FoundationModel/OBJET.md`)
  lit la tour comme une partition des (K−1)-simplexes, avec les poids
  $w_{x\tau}=S_{\tau}/T_{x}$ qui relient points et facettes. Les feuilles FULL
  ne suffisent pas à ces poids (facettes contributrices plus nombreuses) : le
  profil pondéré est un produit séparé, après le premier livrable.
- Juges : T2 census→FULL (n ≤ 14) porté de la v7 ; fixtures E5, A–E, quatre
  points, coquille à sept points, triangle rectangle, MEB K7, K = 1 et K = n.
- Lanceur chronométré de bout en bout (frontière du chronomètre écrite).

Porte de sortie, en deux temps (proposition de l'auditeur A, acceptée) :

1. invariants de la chaîne fermés avec T2 et des tailles croissantes, puis
   **un reçu FULL sur une trame sans sol entière à 1 mm**, ou un échec
   explicite borné (délai ou mémoire, avec compteurs et frontière du
   chronomètre) : cela suffit à ouvrir V9-2 en parallèle ;
2. avant toute revendication de performance : les trois trames, K5 et K10,
   avec répétitions, nœuds, contributions, octets de sortie, RSS et
   compteurs par phase ; sorties bit-identiques en relecture ; **différentiels**
tour v9 contre tour v7 sur uniforme u16 8k / 16k / 32k (tableaux d'histoire
comparés, pas seulement un digest aveugle à la renumérotation) et flux v9
contre flux v8 sur les trois trames. C'est la vraie base de temps de la v9.

## Phase V9-2 — réduire le nombre d'opérations

Porte d'entrée : V9-1 close ; profil par phase épinglé.

Leviers candidats, chacun jugé par **ablation sur l'appel complet** des trois
trames (sorties identiques, compteurs et temps séparés). Candidat prioritaire,
proposé par l'auditeur A (`audits/AUDIT_A_ARCHITECTURE_K_GABRIEL_20260922.md`,
`Q3_STRUCTURE_ET_BORNES.md`, `Q4_STRUCTURE_ET_BORNES.md`,
`CONTRAT_COUTS_ET_PARALLELISATION.md`) : **certificats k-Gabriel locaux de
miniballes**, construits paresseusement là où le front laisse des supports,
sans mosaïque globale d'ordre K. Deux certificats exacts sont établis (compte
et frontière par signe sur cellule fermée ; borne de rayon par K−2 gardes
distinctes pour q4) ; l'invariant de génération qui couvre chaque arête
propriétaire, bloc de complétions et centre avant toute omission reste à
prouver, et le générateur v8 exhaustif reste le repli jusque-là. Autres
leviers :

- certificat collectif d'arête avant l'atlas (il peut supprimer des arêtes
  que les témoins individuels gardent) ; la cascade de rectangles (paire
  représentante, plans h + h_a + h_b, réemploi des singletons) accélère un
  filtre qui pèse environ 7 % du profil de base (environ 14 % après la
  phase 1, estimation) sans changer les paires survivantes ;
- recherche de témoins q3/q4 relancée de zéro pour chaque paire (1,09 G
  visites à K5 et 1,92 G à K10 sur la scène 0) : héritage par voie sous juge Ξ ;
- travail q3 partagé par blocs de graines (relais et centres conditionnels de
  l'auditeur A) : dernière priorité déclarée du moteur entier en v8, jamais
  portée hors float32 ;
- atlas : borne certifiée ≥ K−1 sur toute la cellule (jamais un compte courant
  saturé avant un balayage q4), census q3 dans les fragments exacts déjà construits,
  une seule traversée d'index par arête, frontières sans copie d'IDs ;
- noyau MEB accéléré de l'aval (paire diamétrale, canonisation sur la coquille) ;
- filtres flottants certifiés à repli exact pour les bornes de census q3 et les
  bornes de blocs d'atlas (borne d'erreur écrite, fixture de contact, mutant,
  compteur de replis) : prérequis du SIMD et du GPU.

Porte de sortie : gain net sur le **travail total hors sortie** de l'appel
complet à K10, sans déplacement de coût vers un autre poste. Le reçu publie le
grand-livre : temps total, CPU·s, RSS, taille de sortie, travail q2 / q3 / q4 /
aval, somme des frontières actives, coût de construction des certificats,
répétitions, et la pente 8k / 16k / 32k puis entre trames entières. Les
compteurs de l'atlas (bornes de blocs, tests de points, IDs copiés) ont des
coûts unitaires différents : ne jamais les sommer (un ×10 sur un poste de 50 à 60 % ne
donne qu'environ ×2 au total), ou un constat négatif épinglé qui réoriente le
plan. Les familles adverses (rangées, adversaire K10 des couches duales) et
le protocole spatial trame / moitiés / quarts restent publiés comme tests de
résistance, sans veto (décision utilisateur du 21 septembre).

## Phase V9-3 — occuper la machine

- Unité de travail : un **contexte d'arête possédé** (index immuable, paire
  propriétaire, masque, cover, atlas immuable), découpé en tâches bloc de
  graines q3 × sous-arbre de témoins et bloc de graines q4 × racine de cellule
  vivante ; la file de plages v8 laisse chaque arête entière dans un worker.
- Plan de travail plat commun CPU/GPU : étages synchrones et compaction
  (arêtes résiduelles, covers et graines en CSR, cellules d'atlas,
  enregistrements), tri canonique par clé en fin d'étage.
- Pool persistant, files par worker et vol de travail, grains pondérés par le
  travail estimé ; arêtes lourdes scindées sur un parent immuable partagé.
- Histogramme du coût par arête, attente et vol par worker, chemin critique.
- **L'aval aussi** : l'aval FULL pesait 93 % de la tour v7 et son calendrier
  est séquentiel (exposant local 1,243 contre 1,094 pour la géométrie ; plafond
  d'Amdahl 1,98× après le noyau MEB). Paralléliser calendrier, histoires et
  export (objets parallèles de la v7, contractions des histoires).
- Porte TSan en CMake ; mutants « tâche perdue / dupliquée / fusion décalée » ;
  sorties bit-identiques W1/W2/W4/W8/W24/W48.
- Protocole G4 v9 réécrit (autorité au commit v9, entrées `.u32le`, budget
  découpé, arrêt certifié). N'acheter du temps G4 que pour trancher une
  question : W24/W48 sur les trois trames ; W1 à K10 ne tient pas dans le
  budget utile d'une session.
- Protocole de qualification du contrat : échauffements, répétitions, p95,
  trois trames puis d'autres séquences, avant tout énoncé « contrat tenu ».

## Phase V9-4 — GPU

- Résidence d'abord : format de fil versionné, stub hôte, validateur
  transactionnel, anneau de lots épinglés ; index et catalogue résidents.
  Sources : `morsehgp3D_v6/docs/GPU.md` et le contrat
  `morsehgp3D_v6/src/gpu/lot_ring.hpp`, jamais mesuré ; le worktree partagé
  porte des modifications v6 non commises de ce fichier et de trois autres,
  qui ne font pas partie de l'héritage.
- Premier étage : tests de l'atlas par lots, puis bornes de census q3, puis
  étages de l'aval ; transferts et reconstruction comptés.
- Critère d'arrêt : si le code hôte dépasse la moitié de l'étage GPU, corriger
  la résidence avant d'ajouter des kernels.

## Phase V9-5 — élargir

Trame brute entière avec sol, autres séquences SemanticKITTI, 100 ms, et
réouverture éventuelle de la voie float32 si l'utilisateur la demande. Deux
lignes de qualification restent distinctes : sans sol (priorité de la
directive du 21 septembre) et trame brute (contrat du même jour, non retiré).
Le contrat sur trame brute n'est pas acquis sans sa propre ligne, et trois
trames de la séquence 08 ne qualifient aucune généralisation.

## Règles de travail tirées de la v8

- **Un worktree Git par acteur**, ou un seul écrivain par index. Avant tout
  `git add` : `git diff --cached --quiet || exit 1` ; indexer chemin par chemin ;
  jamais `git add -A`.
- **Rôles stables** : un développeur et au moins un auditeur nommés ; pas de
  rotation sans passation écrite. Canal
  [`audits/COORDINATION_MORSEHGP3D_V9.md`](../../audits/COORDINATION_MORSEHGP3D_V9.md) ;
  toute recommandation d'audit reçoit une réponse écrite (acceptée, refusée
  avec raison, différée).
- **Preuves d'audit dans le dépôt** : un chiffre dont les sources sont hors du
  dépôt est « non vérifiable » jusqu'à dépôt.
- **Mesures** : hôte calme, charge avant et après consignée, refus
  automatique au-delà d'un seuil, au moins trois répétitions, jamais deux
  campagnes simultanées, CPU·s de GNU time sur toute grande mesure, jamais de
  sentinelle réutilisée, jamais `ctest` dans un build épinglé.
- **Registre des preuves** : toute preuve invoquée est inscrite dans
  `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` avant usage ; toute
  contradiction devient une fixture permanente.
- **Documents d'entrée courts** : le README décrit l'état courant, pas un
  journal ; l'historique va dans un journal daté.
