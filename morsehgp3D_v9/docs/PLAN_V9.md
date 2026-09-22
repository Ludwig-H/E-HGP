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
   K5 puis K10, s = 8 (jamais moins), 1 s puis 100 ms sur G4. Les tailles
   8 000 / 16 000 / 32 000 restent les tailles d'intérêt pour les pentes ;
   les petites tailles restent des oracles de correction.
4. **Mesurer de bout en bout, puis réduire le nombre d'opérations**, puis
   paralléliser, puis porter sur GPU. Chaque tranche publie les compteurs des
   postes dominants avant et après.
5. **Rien d'implicite** : ports épinglés et requalifiés, défauts de la
   bibliothèque égaux à la configuration mesurée, anciens chemins dans une
   cible différentielle séparée.

## Phase V9-0 — fondations (avant tout code de moteur)

Porte d'entrée : ce dossier d'ouverture publié ; réponses de l'utilisateur aux
questions 1 à 3 de la [synthèse](AUDIT_V8_SYNTHESE.md) § 9, ou hypothèses
écrites en leur absence.

- `CMakeLists.txt` v9 (C++20, `-Wall -Wextra -Wpedantic -Werror`, namespace
  `mhgp9`, préfixes `mhgp9_` / `MHGP9_`), Boost **obligatoire**
  (`FATAL_ERROR`), options `MHGP9_SANITIZE` (ASan/UBSan) et `MHGP9_TSAN`.
- Portes à code de sortie exact portées de la v7 (`run_expect.cmake`) ; mutants
  `--inject=` dans le produit, exécutables dans tout arbre, sanitizers compris.
- Workflow GitHub qui construit la v9 et lance `ctest -L gate --no-tests=error`
  depuis une archive des sources (portes hermétiques : aucun `git rev-parse`,
  aucun chemin absolu).
- Type de point certifié (seule la préparation du nuage le produit) et table
  de bornes vérifiée par `static_assert` générée depuis `coordinate_bits`.
- Format de reçu unique : manifeste minimal versionné, `SHA256SUMS`, contrôleur
  générique en CI, plafond de taille, aucune copie de sources, aucune donnée
  tierce, sondes conservées sous `build/v9_*` (jamais dans un scratchpad).
- Données : `data/` ignoré par Git, alimenté par un fetcher paramétré
  (séquences et trames choisies **avant** les chronos) et par les
  préparateurs ; seuls les manifestes (sha256, tailles, effectifs, paramètres)
  sont versionnés.

Porte de sortie : suite verte en CI et en local ; mutant « plage de
coordonnées non vérifiée » tué ; un reçu d'exemple lu par le contrôleur.

## Phase V9-1 — tranche verticale mono, de bout en bout

Porte d'entrée : V9-0 close.

- Ports épinglés du générateur v8 (liste de l'[héritage](HERITAGE_V7_V8.md) § 2)
  en configuration mesurée unique, voie q2 **dans le même appel** que q3/q4.
- Catalogue canonique : union q2 ∪ q3 ∪ q4 dédoublonnée par clé primitive,
  q_min, IDs intérieurs, coquille complète, niveau exact ordonné ; populations
  numérotées indépendamment de l'ordre de découverte.
- Tour FULL : port explicite de la sémantique de
  `morsehgp3D_v7/src/forest/full_ball_tower.hpp` (domaine requalifié à
  18 bits, coquille non plafonnée ou refus explicite), extension non
  régulière, format de sortie décidé.
- Juges : T2 census→FULL (n ≤ 14) porté de la v7 ; fixtures E5, A–E, quatre
  points, coquille à sept points, triangle rectangle, MEB K7, K = 1 et K = n.
- Lanceur chronométré de bout en bout (frontière du chronomètre écrite).

Porte de sortie : **premier reçu de tour complète** sur les trois trames sans
sol 1 mm, K5 puis K10, W1, avec nœuds, contributions, octets de sortie, RSS,
et compteurs par phase ; sorties bit-identiques en relecture. C'est la vraie
base de temps de la v9.

## Phase V9-2 — réduire le nombre d'opérations

Porte d'entrée : V9-1 close ; profil par phase épinglé.

Leviers candidats, chacun jugé par **ablation sur l'appel complet** des trois
trames (sorties identiques, compteurs et temps séparés) :

- certificat collectif d'arête avant l'atlas (il peut supprimer des arêtes
  que les témoins individuels gardent) ; la cascade de rectangles (paire
  représentante, plans h + h_a + h_b, réemploi des singletons) accélère un
  filtre qui pèse environ 7 % du profil sans changer les paires survivantes :
  à ne porter que pour ce gain borné ;
- atlas : saturation à K−1, census q3 dans les fragments exacts déjà construits,
  une seule traversée d'index par arête, frontières sans copie d'IDs ;
- noyau MEB accéléré de l'aval (paire diamétrale, canonisation sur la coquille) ;
- filtres flottants certifiés à repli exact pour les bornes de census q3 et les
  bornes de blocs d'atlas (borne d'erreur écrite, fixture de contact, mutant,
  compteur de replis) : prérequis du SIMD et du GPU.

Porte de sortie : au moins ×10 d'opérations en moins sur le poste dominant à
K10, ou un constat négatif épinglé qui réoriente le plan.

## Phase V9-3 — occuper la machine

- Plan de travail plat commun CPU/GPU : étages synchrones et compaction
  (arêtes résiduelles, covers et graines en CSR, cellules d'atlas,
  enregistrements), tri canonique par clé en fin d'étage.
- Pool persistant, files par worker et vol de travail, grains pondérés par le
  travail estimé ; arêtes lourdes scindées sur un parent immuable partagé.
- Histogramme du coût par arête, attente et vol par worker, chemin critique.
- Porte TSan en CMake ; mutants « tâche perdue / dupliquée / fusion décalée » ;
  sorties bit-identiques W1/W2/W4/W8/W24/W48.
- Protocole G4 v9 réécrit (autorité au commit v9, entrées `.u32le`, budget
  découpé, arrêt certifié) ; première session CPU sur les trois trames, K5 et
  K10, W1/W24/W48.

## Phase V9-4 — GPU

- Résidence d'abord : format de fil versionné, stub hôte, validateur
  transactionnel, anneau de lots épinglés ; index et catalogue résidents.
- Premier étage : tests de l'atlas par lots, puis bornes de census q3, puis
  étages de l'aval ; transferts et reconstruction comptés.
- Critère d'arrêt : si le code hôte dépasse la moitié de l'étage GPU, corriger
  la résidence avant d'ajouter des kernels.

## Phase V9-5 — élargir

Trame brute entière avec sol, autres séquences SemanticKITTI, 100 ms, et
réouverture éventuelle de la voie float32 si l'utilisateur la demande.

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
