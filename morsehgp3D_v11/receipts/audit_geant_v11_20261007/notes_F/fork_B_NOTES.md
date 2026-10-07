# Fork B — lignée v5 et v6 (27 août → 4 septembre 2026)

Lecture seule ; GCP non utilisé. [I] = inférence ; [E] = estimation arithmétique ; le reste est vérifié dans la
source citée.

## 0. En bref

- **v5** (27–31 août, 314 commits) et **v6** (31 août–2 sept., 200 commits) calculent **le même objet que la v4** :
  les dix forêts **horizontales** K = 1..10, en u16. Leur sémantique est `verified_events_only` /
  `gabriel_positive_connectivity`. Elles ne livrent ni verticales, ni incidences silencieuses Gamma, ni FULL.
- Ce sont les **seules reprises de la lignée dont le différentiel a été fermé**, par digests d'objet égaux :
  v4≡v5 12/12, puis v5≡v6 15/15 et 8/8 sur G4.
- Aucune ne visait un temps de 1 s ou de 100 ms. Le « contrat 50 000 points » était un contrat **de mesure**.
- Le mur réel était la **résidence mémoire** : environ 4,8·10^5 points à K = 10 sur 180 Gio. Le GPU n'a jamais
  dépassé −10,4 % du mur.
- La v6 n'a pas été « abandonnée ». La **v7 l'a portée telle quelle**, travail non commis compris, le 4 septembre,
  pour changer d'objet (FULL) sous un nouveau contrat de temps (1 s, puis 100 ms).

## 1. v5 — « reprise à propre de la v4 »

### Dates

- Ouverture : `8600c53b9`, 27 août 2026, 07:08 UTC.
- Dernier commit : `f90f2b273`, 31 août 03:10 UTC.
- Pin v4 de référence : `main@d4f3ce59` (`morsehgp3D_v5/docs/PROVENANCE.md:16`).

### Objet

- Les dix forêts horizontales HGP K = 1..10, niveaux ρ² exacts (`morsehgp3D_v5/README.md:7-15`).
- `forest_semantics=verified_events_only`, `proof_basis=gabriel_positive_connectivity`. Aucune sortie
  `require_exact=true`. Verticales : « non livrées ».
- Le P0 de l'auditeur (`morsehgp3D_v5/audits/ETAT_COURANT.md:66-99`) qualifie la forêt de **surqualifiée** : le
  README dit « le même objet que la v4 — les dix forêts horizontales HGP », alors que c'est le sous-flot de Gabriel.
- La fixture `gabriel-point-set-counterexample-5-points-v1` le démontre : les cofaces non-Gabriel `ACD` et `ACE`
  attachent `AC` au niveau carré 33/2. Gamma et le flot de Gabriel n'ont alors ni la même généalogie ni le même
  temps de fusion.

### Ce que la v5 corrige de la v4

D'après l'audit du 22 août (`README.md:22-49`) :
- **Résidence.** La v4 gardait les dix forêts en mémoire : 7,7 Go à 8 000 points, 21 Go à 32 000. Le RSS v4 de
  21,0 Go à 32 000 est confirmé par l'en-tête de `receipts/conformite_v4/digests_v4.txt`. La v5 streame par ordre K
  (`fold_inflight` + 1 ordres résidents) et nomme quatre rôles mémoire.
- **Monolithes.** `forest_probe.cpp` (4 478 lignes) et `ball_stream.hpp` (1 805 lignes) sont découpés en modules,
  avec une porte par thème.
- **Mutants.** Un registre unique `MHGP5_MUTANT`.
- **Code mort.** Les opt-in négatifs et `build_forest_legacy` sont retirés.

### Architecture

Index radix de Karras sur Morton 48 bits → WSPD ternaire par vagues → par ancre : cover, fuseaux W_q, tests d'ancre
(secteurs, théorème 10.3), corde q4 (10.4), grille de cellules sans apex (10.5, `src/lanes/cell_grid.hpp`, pin
`82f613d3`) → RLE par `BallKey` → préfiltre → census inline (intérieur ≤ 9, coquille ≤ 12) → fold par K.

Le fold a deux étages : A, parallèle (tri, internement partitionné) ; B, séquentiel (`reduce_fold`). Ils sont
pipelinés (`PROVENANCE.md:20-60`).

### Précision et séparation

u16. Séparation produit **s ≥ 8**, refusée en dessous avant calcul (`README.md:54-56`). Ce seuil est antérieur à la
consigne de l'utilisateur du 14 septembre.

### Contrats

- Portes à 8k, 16k et 32k en local, puis « contrats à 50 000 points sur G4 », puis des dizaines de millions
  (`README.md:57-60`).
- Le contrat 50k est un « contrat 50 000 points **mesuré** » (`docs/PLAN_DE_TESTS.md:103` ;
  `gcp-migration/session_campagne_v5_scale_g4.sh:3`) : **aucun seuil de temps**.
- Cible fixée le 28 août : 10 à 30 M points sur une G4, K = 10 en principal, K = 5 en secondaire (`smax = 6`,
  préfixe exact) (`docs/ECHELLE.md:18-31`).

### Mesures G4 (48 fils, K = 1..10, s = 8)

- **Session 1** (`f37669ae`, 27 août, `receipts/campagne_g4_v5_20260827/RECU.txt`), à 50k : uniform 219 s,
  eight_clusters 375 s, scanline 54 s, terrain 42 s.
- **Sessions 9 → 10 → 11** (`5c777be3` → `90baa0bb` → `82f613d3`) :
  - uniform 50k : 77,7 → 57,1 → 56,6 s ;
  - uniform 200k : 346 → 258 → 253 s ;
  - scanline 200k : 499 → 502 → 268 s, grâce à la grille 10.5 (`out/contrat_*`, vérifié).
- **Uniform 50k, session 11, par étage** : index 8 ms, génération 15,3 s, RLE 1,8 s, préfiltre 7,5 s, census 4,1 s,
  fold 36,5 s et digest 16,3 s (cumuls d'étage) ; 21 622 480 boules ; RSS 19,8 Go.
- **Uniform 200k** : 66 Go au palier, 72,3 Gio de `ru_maxrss` (`docs/ECHELLE.md:62-75`, `:113-116`).
- **Pentes jusqu'à 200k** (`docs/MESURES_ECHELLE.md:24-43`) :
  - uniform n^1,09 ;
  - eight_clusters n^1,32 ;
  - scanline_single_pass **n^2,72** (les lanes en n^3,14) ;
  - 24,0 G tests de cœur à 100k sur scanline.
- **Multi-CPU** : eight_clusters 16k passe de 215 s à 1 fil à 15 s à 48 fils, soit ×14,1 (`docs/GPU.md:568-578`).

### GPU

- Le chemin device est exact : digests égaux au CPU, mutant tué sur le device.
- Il est **plus lent partout** au pin `8f95df2e` : uniform 89 contre 78 s, eight_clusters 718 contre 246 s
  (`docs/GPU.md:13`).
- La cause est la matérialisation hôte : 18,2 G seeds q3 d'environ 100 octets chacun.
- Session 13 (`c95cfa95`) : noyaux sous 1 s par lane, mais H2D de 40 à 95 Go, enfilement de 33 à 50 s et
  98 exécuteurs éphémères (`GPU.md:549-565`).
- Session 14 (`839cf1ec`, indices u32) : octets H2D divisés par 1,8 à 2,9, **mur inchangé** (mémoire
  `etat-v5-tests-ancre`).

### Sessions G4

- 14 dossiers de reçus entre le 27 août 13:20 et le 28 août 11:21 UTC.
- Trois sont `partial_or_failed` (rc 65). Une quatrième (`_adaptatif`) est partielle : le conteneur a redémarré, le
  journal a été perdu, et la VM est restée `RUNNING` environ 60 min hors session (`RECU.txt` de ce dossier).
- La session 7 a perdu son rapatriement parce qu'un script a été édité pendant son exécution (mémoire
  `script-en-cours-edition`).

### Clôture

Le verdict de l'auditeur du 31 août (`ETAT_COURANT.md:37-60`, `:684-696`) établit deux choses :
- la famille `linked_arcs_u16` (construction d'Edelsbrunner–Pach) donne Θ(n²) boules critiques ;
- « une garantie sous-quadratique pour toute entrée et sortie explicite n'est plus un objectif mathématique
  recevable ».

La v6 s'ouvre le même jour. Elle répond à la commande : « repartir de zéro, viser un coût sous-quadratique par
compteurs sur les régimes annoncés, fortement parallélisable et GPU-isable, en repensant d'abord les lanes q3/q4 »
(`morsehgp3D_v6/audits/NOTE_CLAUDE_CONCEPTION_V6_20260831.md:20-22`). C'est une paraphrase de Claude : le mot exact
de l'utilisateur n'a pas été retrouvé.

## 2. v6 — même objet, génération q3/q4 « sortie-sensible »

### Dates

- Ouverture : `d9cb45db6`, 31 août, 08:06 UTC.
- Dernier commit : `de69851e3` (palier P5), 2 septembre, 16:35 UTC.
- Ni `AGENTS.md` ni `CLAUDE.md` n'ont jamais eu de section v6 (`git log -S morsehgp3D_v6`).

### Objet et profil

- Objet identique à la v5 (`morsehgp3D_v6/README.md:13-21`), en u16.
- s ≥ 8, `smax ≤ 11`.
- **Positions dupliquées refusées** (`unsupported_degeneracy`, `README.md:74-75`), ce qui pose un verrou pour le
  LiDAR (`docs/ECHELLE.md` § 7).

### Architecture

Socle v5 porté et requalifié, auquel s'ajoutent :
- descente WSPD fusionnée à masques ;
- requêtes de facteurs saturées ;
- crédits composés typés ;
- **sweep de corde** : chaque complétion D est une racine μ_d = P(d)/B(d) (message de `d9cb45db6`) ;
- grand-livre des termes de coût ;
- familles **stationnaires** en première classe.

Fait établi par la v6 : les familles `terrain` et `scanline` du dépôt sont **dilatées** (hauteurs ∝ √n). Leur
super-quadraticité vient de la famille : en gelant les deux échelles, q4 redevient linéaire, avec des exposants de
1,003 à 1,014 (`NOTE_CLAUDE_CONCEPTION_V6:44-48`).

Correctif P0 du 31 août (`381ba60b4`) : le cover q4 passe au coefficient 4. Le coefficient 3 hérité de la v5 perdait
des témoins intérieurs (contre-fixture : tétraèdre régulier plus un point z). Le correctif est fail-open : l'objet
est intact, mais `digest_balls` diverge désormais de la v5.

### Mesures G4

Cinq sessions, dont deux échecs : `2a981bc4` (rc 76, 31 août) et `5d886db1` (rc 1, 1er sept.), toutes deux avec arrêt
ciblé certifié.

**Session `d98f4729`** (1er sept., 81/81 ; `audits/NOTE_CLAUDE_MESURES_G4_20260901.md`) :
- À 50k : uniform v5 48,9 s contre v6 49,2 s ; eight_clusters v5 55,7 s contre **v6 62,4 s (+12 %)**. La génération
  neuve n'a **pas** accéléré.
- Fils 1 → 48 : ×13,0 sur uniform, ×15,7 sur eight_clusters. La fraction d'Amdahl vaut 4,4 à 5,7 %.
- Frontière K = 10 :
  - 200k : 225 s, 78,8 Go ;
  - 400k : 479 s, 153,9 Go ;
  - 800k : `bad_alloc` à 550 s.
- Route GPU v5, une prise par route : parité sur uniform et eight_clusters, −19,5 % sur terrain, −9,6 % sur
  scanline.

**Série C, `b97f20ea`** (58/58 ; `gpuv6_resume.txt`) : préfiltre et census sur le device.
- uniform 50k : 59,0 → 52,8 s, soit −10,4 %.
- L'étage device dure 7 717 ms, dont **154 ms de noyaux**, 2 641 ms de sérialisation et 4 110 ms de reconstruction
  hôte monofil (`docs/GPU.md:217-222`).

**Tests K = 10 / K = 5, `c8f69673`** (2 sept. ; `matrice_resume.txt`), à 50k, 48 fils :

| Famille | K = 10 | K = 5 |
|---|---|---|
| uniform | 47,4 s | 9,07 s |
| eight_clusters | 60,7 s | 12,9 s |
| terrain | 16,9 s | 4,9 s |
| scanline | 13,3 s | 4,2 s |

La propriété de préfixe est vérifiée : les `digest_forest_K1..K5` à `smax = 6` sont égaux à ceux de `smax = 11`.

### Murs

D'après `docs/ECHELLE.md` § 2–5 bis :
- **Résidence** : environ 4,8·10^5 points à K = 10 [E sur M] ; de 2,4 à 3,9·10^6 à K = 5 [E sur une observation
  locale].
- **Temps** : exposant 1,097 (K = 10) et 1,088 (K = 5) sur uniform. Sur les familles minces, il monte à 1,60–1,76.
- **Format** : le digest à identifiant u32 cesse d'exprimer l'objet vers 4,3·10^6 points à K = 10.
- **Fold** : le `reduce` ralentit quand on ajoute des fils, de 6,58 s à 1 fil à 7,62 s à 48 (mémoire
  `etat-v6-ouverture` ; non relu dans un reçu).
- **GPU** : plafond de ×1,31, même avec un étage device nul.

### Les deux derniers jours

- Paliers P1 à P5 livrés (`0422faddb`, `de69851e3`). P3 (libérations par tranche) est **négatif** : gain
  structurellement nul à 1 fil, et seuil `mmap` de glibc (§ 6.3).
- Pin KeyCSR (`8afd1057`) posé, mais sa mesure est différée.
- C6 : conception, puis jalons 1 à 3 sous stub. **Jamais mesuré sur G4.**
- La journée du 2 septembre a été absorbée par les harnais : reprise, revalidation, sonde d'ablation,
  préinscription. D'où la directive de l'utilisateur : « Ne t'égare pas avec trop de garde-fous de sécurité […]
  concentre-toi sur l'implémentation multi-CPU et GPU […] Feu vert pour la GCP G4 » (mémoire
  `directive-echelle-multicpu-gpu` ; `audits/NOTE_CLAUDE_PIN_KEYCSR_20260902.md:87-90`).

### Clôture

- La v7 s'ouvre le 4 septembre (premier commit `e0e6396b6`, 20:43 UTC).
- Elle change d'objet : HGP **FULL**.
- Elle reçoit la consigne de l'utilisateur du 4 septembre : « moins d'une seconde pour toute la tour K = 1 à 10 ;
  repli sur toute la tour K = 1 à 5 », puis 100 ms comme jalon suivant (`morsehgp3D_v7/docs/LECTURE_ET_CONTRATS.md:146-152`).
- Elle **porte** la v6 : `morsehgp3D_v7/docs/V6_SOURCE_SNAPSHOT.json` (head `de69851e3`, 4 sept., 119 entrées,
  `source_state=worktree_with_four_tracked_changes_and_three_untracked_files`).

### Les fichiers v6 non commis du `git status`

Il s'agit du **palier P4** et du **jalon C6a** :
- P4 (tri par permutation et piles hissées) : `src/parallel/sort.hpp`, `src/pipeline/census.hpp`,
  `src/pipeline/expand.hpp` et `tests/perm_sort_gate.cpp` ;
- C6a (route C6 sous stub) : `src/gpu/route_c6.hpp`, `src/gpu/lot_ring.hpp` et `tests/route_c6_gate.cpp`.

Les fichiers non suivis datent du 2 septembre, entre 16:43 et 16:54 UTC.

Leurs **SHA-256 sont égaux** aux entrées du snapshot v7 (vérifié pour les sept fichiers). Ce n'est donc pas du travail
perdu : la v7 l'a consommé et le versionne sous `morsehgp3D_v7/`. Le contenu est resté inchangé depuis le
4 septembre. Les dates de modification au 3 octobre 13:03 tiennent à une opération en masse [I].
`AGENTS.md:307` (13 sept.) parle déjà de « modifications v6/v7 préexistantes ».

## 3. Différentiels

**v4≡v5 : fermé.**
- Égalité `digest_balls` et `digest_all` (sérialisation `mhgp4-digest-v1` reproduite).
- Domaine : quatre familles (uniform, terrain, eight_clusters, scanline_single_pass) × {8k, 16k, 32k}, s = 8,
  `smax = 11`, graine 3.
- Trois campagnes à **12/12 `egal/egal`**, le 27 août : `210571fb` (11:59), `d08913ac` (14:13) et `50fee05c` (15:18,
  `worktree_v5_modifie=oui`) (`receipts/conformite_v4/campagne_v5_*.txt`).
- Les références ont été calculées par la v4 à `d4f3ce59` (`digests_v4.txt`, 19 entrées). Le contrôle a été rejoué
  sur la VM en session 1.

**v5≡v6 : fermé.**
- 15/15 (cinq familles × trois tailles) au pin `3bad233d`, sur `digest_all` et les dix `digest_forest_K`.
- 8/8 sur G4 (quatre familles × {32k, 50k}).

**Réserves.**
- `digest_balls` mesure un **filtre**, pas l'objet. Le correctif du coefficient 4 l'a montré : la conformité v4/v5
  sur `digest_balls` avait gravé la force d'un filtre défaillant ouvert (motif n° 6 de
  `morsehgp3D_v5/docs/PISTES_FERMEES.md:98-99`).
- [I] La colonne `maxrss_kb` des campagnes est un **maximum cumulé** sur les processus fils : 2 819 272 kB pour les
  quatre familles à 8k, 10 240 412 kB pour les quatre à 32k. Elle ne donne donc pas le RSS par famille.

## 4. Pistes fermées et résultats négatifs (v5–v6)

### Héritées de v3 et v4, reconduites

- Source kNN à préfixe ; cap de population dans le critère terminal ; scission du facteur le plus peuplé ; deux
  arbres spatiaux ; arrondi du rayon de cœur vers le haut.
- Décomposition ternaire symétrique : Ω(n²) blocs (théorème 4, famille cercle–axe).
- Sélection axiale (+7 % de `t_gen`) ; couches convexes pour q3 ; plafond « pic projeté », faux deux fois.
- Étage i64 du préfiltre q4 (médiane 1,0021) ; refuser les coquilles ; deltas seuls ; `first_batch`.
- **Comparer des constantes entre deux processus** : `t_fold` varie de ±40 %.

Sources : `morsehgp3D_v5/docs/PISTES_FERMEES.md:7-82` ; `morsehgp3D_v6/docs/PISTES_FERMEES.md:9-44`.

### Propres à la v5

- La route q2 reste interdite (fixture à six points).
- `separated` n'est pas héréditaire (fixture 1D `{0, 99, 100, 512, 612}`).
- Le raffinement post-séparation intégré coupe 47 % des ancres, mais le mur augmente de 34 %
  (`MESURES_ECHELLE.md` § 4 ter).
- Le tout-device matérialisé est plus lent partout.
- Un sous-quadratique universel n'est pas recevable (`linked_arcs_u16`).

### Propres à la v6

- n° 14 à 18 : max de deux concaves aux sommets ; requête saturée avec crédits soustraits ; vue mémorisée dans
  `ForestResult` ; retri des deltas ; double réserve.
- **F0** : le reduce du fold ne se porte pas sur GPU. Les racines DSU dépendent de l'historique, et le digest est
  contractuel (`docs/GPU.md:24-37`).
- **Tuilage avec halo** : rejeté sur mesure. Le rayon maximal atteint 149 unités, soit 23 % du domaine.
- Empreinte probabiliste ou compteur tronqué : interdit.
- Le seau Morton n'est pas une autorité d'unicité.
- P3 est négatif.

**C6 : les deux conceptions écartées par doctrine** (`docs/GPU.md:224-233` ; mémoire `palier-c6-gpu`) :
1. Faire lire au device les `BallCandidate` à leur ABI : « jamais un memcpy de struct ABI », et le padding est
   indéterminé.
2. Servir une table SoA aux deux routes : cela change la route CPU, que la couture promet inchangée.

## 5. Motifs observés

- **Cadence extrême.**
  - 314 commits en 4 jours, puis 200 en 2,5 jours.
  - 14 sessions G4 en 22 h.
  - Le pipeline v5 était « conforme à la v4 au digest près » 18 minutes après l'ouverture (`f9b4d7b6e`, 07:26).
    Le code était donc prêt hors dépôt [I].
- **Harnais et qualification avant la vitesse** : le 2 septembre en est l'exemple, d'où la directive de
  l'utilisateur.
- **Objet surqualifié dans les README** (P0 v5).
- **Le GPU est toujours borné par l'hôte** : copies, sérialisation, exécuteurs, monofil. Les noyaux pèsent de 1 à
  2 % du mur.
- **« Promettre avant de mesurer »** : le scan q3 était désigné comme dominant, alors que la descente WSPD faisait
  72 % du mur (motif n° 1 de `PISTES_FERMEES.md:86-88`).
- **Tension entre versions** [I]. La note v6 affirme : « le nombre de rectangles WSPD n'a jamais été le problème »
  (31 août). La mesure du 14 septembre (mémoire `regime-wspd-v4-s8`) désigne au contraire « le nombre de petits
  rectangles » comme poste dominant. Les régimes et les moteurs diffèrent.

## 6. Pour la v12

### À reprendre

- **Méthode de conformité** : familles portées bit à bit, digests gravés, manifeste de campagne (pin, SHA du
  binaire, état du worktree), plusieurs pins.
- **Porte de préfixe Kmax** : un run à `smax` réduit égale, ordre par ordre, le run complet.
- **Banc intra-processus** (`tests/fold_bench.cpp`) : paires ABBA, médiane des rapports appariés, test de signe,
  ouvriers mesurés.
- **Mémoire** : comptabilité par rôle, `VmHWM` à chaque frontière d'étage, et un `bad_alloc` toujours rendu en
  `resource_exhausted` typé par étage (`9243d69f`).
- **Bornes exactes par axe** de la puissance d'une boule sur une boîte (`census_detail::AxisBounds`,
  `morsehgp3D_v6/src/pipeline/census.hpp`). La fouille v6 l'estime à ×0,63–0,72 de nœuds visités [E].
- **Contre-fixtures** : `linked_arcs_u16` (contrat sortie-sensible) et Gabriel à 5 points (Gamma ≠ Gabriel).
- **Familles stationnaires** pour toute pente.
- **Contrat des baux** de `lot_ring` et règle d'erreur au minimum lexicographique, pour tout pipeline hôte/device.
- **Modèle de coût G4 mesuré** : noyau négligeable devant les copies et la reconstruction hôte.

### À ne pas refaire

- Un GPU matérialisé par ancre, ou des exécuteurs éphémères.
- Un DSU ou un reduce sur device sans théorème d'équivalence.
- Un halo.
- Un « sous-quadratique pour toute entrée ».
- Un plafond mémoire projeté.
- Des comparaisons entre processus non appariées.
- Un digest de candidats comme preuve de conformité.
- Des pentes tirées de familles dilatées.
- Des journées de harnais sans moteur.
- Le refus des positions dupliquées sur des données LiDAR.

## 7. Vérification des notes de fouille (4 octobre)

**`fouille/v5.md`**
- **Exact** : uniform 50k en 56,6 s, avec ses étages, 21 622 480 boules et 19,8 Go (`out/contrat_uniform_n50000.txt`
  de la session 11) ; scanline en n^2,72.
- **Exact en tant qu'estimation [E]** : « environ 64 µs de temps-fil par boule ». Le calcul
  (15,35 + 1,84 + 7,51 + 4,06 s) × 48 / 21,62 M donne 63,8 µs. Il suppose 48 fils occupés en permanence.
- **Inexact** : « GPU : […] jamais un gain ». La session `d98f4729` montre −19,5 % sur terrain et −9,6 % sur scanline
  (une prise par route), et la série C −10,4 %. Le verdict de fond reste juste : gain marginal, parité au mieux sur
  les régimes volumiques.

**`fouille/v6.md`**
- **Exact** : K5 uniform 50k en 9,08 s (9 070 ms au premier passage) ; ×13,0–15,7 ; 154 ms de noyaux pour −10,4 % ;
  `AxisBounds` présent dans le `census.hpp` versionné.
- **Non relu dans un reçu** : le reduce de 6,58 → 7,62 s.
- **Incomplet** : sa liste du travail non commis (P4, C6a) est exacte, mais ne dit pas que la v7 l'a porté à
  l'octet près.
