# Audit final v11 : sorties, hiérarchies dérivées, `mhgp11` et API, comparaison à HDBSCAN

Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`, `profile=quantized_u21_input_only`, `public_status=not_claimed`. Instantané `ac081a06f`, lu sans rien modifier ; GCP non utilisé.

Légende :
- **[F]** fait vérifié, chemin relatif à `morsehgp3D_v11/` ;
- **[F-hd]** fait vérifié hors dépôt (`build/v11-persist/`, non versionné) ;
- **[M]** note de mémoire du développeur ;
- **[I]** inférence.

Trois lectures parallèles (supports, points, plat et HDBSCAN) ont été recoupées par sondage, commits compris.

## 1. Périmètre et état

[F] Un seul exécutable, `mhgp11` (`cli/mhgp11.cpp`, qui n'inclut que `src/api/api.hpp`), avec `--sortie` obligatoire et `1 ≤ K ≤ 12`.
- **Façade `api`** : `Session` (budget et Pool uniques), `compute`, `publish`, `finish`, `withdraw`.
- **Module `io`** : dossier transactionnel (`D.pending`, manifeste écrit en dernier, `renameat2(RENAME_NOREPLACE)`). Les doubles échecs donnent l'état `published_complete`. Codes 0, 2 et 3 ; une ligne JSON.
- **Signature `tree_k_sha256` v2**, commune aux quatre sorties. Octets identiques quel que soit le nombre de fils (`docs/SORTIES.md`, § 1 à 10).

| Sortie | Objet | Format | Dernière qualification G4 |
| --- | --- | --- | --- |
| `full` | tour FULL, ordres 1..K, verticales | `MHGP11FUL1`, dump de la sonde, inchangé | `receipts/developpement_20261005/qualification_finale` (`98a009550`) |
| `supports` | arbre couvrant d'ordre K : naissances et fusions retenues par Kruskal au plateau, S\* seul | `MHGP11SP` v2 | `receipts/developpement_20261006/supports_kruskal` (`07428324e`) : 15/15, Release u21 seulement |
| `points` | H^r_{K+1} = P₁∘Π_{K+1}, avec m(1)=1, m(K)=K+1 et κ=1 | `MHGP11PT` v1 | `qualification_finale` : différentiels S9 (natif égal à Python et à l'oracle) |
| `plat` | critère A, puis EOM N-aire à φ=r^−z (z = 1, 2 ou 3) ou feuilles ; par défaut EOM, z=1, mcs 20 | `MHGP11ET` v1 | `qualification_finale` : différentiels S10, mutants `head` 10/10 |

[F] Ce que couvre `qualification_finale` :
- Release u18/u21/u24/empoisonnement : 873/783/783/784 portes conformes ;
- échelle et LiDAR : 52/52 et 66/66 par profil ;
- mutants : 485/485 en u18, dont `api` 23/23, `cli` 28/28 et `head` 10/10 ;
- ASan+UBSan u24 et TSan u21 conformes ; `release_long` 35/35.

Deux objets du domaine ne sont pas des sorties natives :
- les polyèdres d'ordre K : prototypes Python en aval (`receipts/polyedre_ordre_k_20261007`) ;
- les comparaisons à HDBSCAN : bancs Python `bench/points_*.py` sur l'export natif, plus de 9 000 lignes, `sklearn` 1.7.2 sur G4.

### 1.1 Décisions de l'utilisateur qui encadrent les sorties

- **2 octobre.** v11 à neuf ; 100 ms ; comparer à HDBSCAN sur données synthétiques et réelles ; trouver des exemples où HGP réussit (`README.md`).
- **Héritées de la v10 [M].**
  - `sklearn` tel quel, jamais réimplémenté.
  - « HDBSCAN ne peut pas battre la tour » : sinon, revoir la tête.
  - Ordre de travail : la tour, puis la hiérarchie, puis z.
  - Les deux triangles de la thèse comme cible.
  - Un cluster n'existe qu'à partir de mcs.
  - Trancher sur LiDAR.
- **3 octobre.** Pour le verrou FULL → points : les mathématiques d'abord, en restant critique envers la thèse comme envers l'auditeur. « Q2 ou Q3 ne sont que de peu d'importance par rapport au modèle mathématique » (`docs/HIERARCHIE_POINTS.md`).
- **4 octobre, matin** (`docs/SORTIE_PLATE.md`, § 3.2) :
  - une sélection « élégante », sans fusions parasites ;
  - « pour les surfaces, z = 2 est LA bonne solution mathématique… mais en pratique ? » ;
  - sur LiDAR, mieux vaut découper que fusionner ; sur synthétique, il faut retrouver exactement les classes ;
  - toute hiérarchie HGP montrée vient du moteur v11 [M].
- **4 octobre, soir** (`receipts/audit_supports_implementation_20261004/evidence/sources/DECISIONS_UTILISATEUR.md`) :
  - tout natif, `--sortie=full|supports|points|plat` ;
  - Q_b seul, sans populations ; compte `kparties_reliees` ;
  - livraison dans l'ordre supports, points, plat ;
  - « mes réponses n'étaient pas intangibles ».
- **5 octobre [M].** Dans les vidéos, effacer le facteur 2 d'échelle de HDBSCAN.
- **6 octobre, vers 10 h.** « Il ne faut surtout pas représenter tous les supports… mais seulement ceux associés au minimum spanning tree de niveau K ». Choix retenu : Kruskal, S\* seul (`audits/REPONSE_CLAUDE_SUPPORTS_20261004.md`, § R).
- **6 octobre, après-midi et soir** (`audits/QUESTION_CLAUDE_POLYEDRE_ORDRE_K_20261006.md`, `audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_20261006.md`) :
  - « des polyèdres et non des points » ;
  - puis : généraliser le complexe alpha à l'ordre K « dans le même esprit » que les parties I et II du manuscrit, qui n'en donnent qu'une analogie, pour « représenter robustement les niveaux d'un polyèdre » ;
  - tester d'abord sur LiDAR réel [M].

## 2. Ce qui a marché

1. **Un socle de sortie transactionnel et qualifié.** [F]
   - La chaîne io, api et cli passe la qualification ci-dessus, et ses trois doubles échecs sont joués (`SORTIES.md`, § 9).
   - Défauts d'audit corrigés, chacun avec sa porte : jeton de Session P1 (`d6082be62`) ; provenance incohérente (`a5e4019b4`, porte `mhgp11_api_publish_reader`) ; SIGXFSZ.
2. **Supports réduits à l'arbre couvrant de Kruskal.** [F]
   - Lemmes A à H prouvés sans position générale. Les naissances et les fusions suffisent à reconstruire T_K (`docs/MATHEMATIQUES.md`, § 10.10).
   - Sélection par un DSU des enfants de chaque multifusion, parcouru dans l'ordre des `BallIdx`, connexion finale vérifiée (`src/supports/hierarchy.cpp`, `Assembly::select`).
   - Sur ng00 à K5 : 576 388 boules pour 576 371 nœuds, contre 789 886 boules et 789 889 supports en v1.
   - Sur l'uniforme de 32 000 points : boules = nœuds = 1 163 756.
   - Le plafond de coquille de 24 sites disparaît ; seul reste m ≤ 255.
   - Contrôles : différentiel exact contre l'oracle S1, lecteur couvrant (`mhgp11_cli_supports_spanning_reader`).
3. **Une règle écrite d'avance a fixé la voie de calcul.** [F]
   - Rapport supports/FULL de l'étage `tree` à W48 : 1,21, 1,18 et 1,09, d'où la livraison de L2b. Après L2b : 1,045, 0,999 et 1,055.
   - SPv1 sur ng00, K5, W48 : 736 ms au total et un fichier de 30,8 Mo. `full` : 2 560 ms, dont 2 163 ms d'écriture, et 300,9 Mo.
4. **La hiérarchie H^r_{K+1}.** [F]
   - Prouvé (H1–H7) : fidèle, laminaire, indépendante de mcs, stable en 3ε pour les dates et les hauteurs, liaison simple à k=1. Les deux triangles sont rendus.
   - Les dates √t+√m−√q sont décidées exactement, par classes de carrés.
   - Porte stricte G4 `claudepts6` (`f02f91c7e`) : 2 854 nuages, 215 974 sites comparés exactement, 12 fixtures.
   - Stabilité mesurée : au plus 2,28ε sur 502 488 paires.
   - Port natif S9 identique à Python, sur synthétique et sur ng00–ng02 entières.
5. **Un gain au niveau B (meilleur bloc) contre HDBSCAN.** [F] (`docs/HIERARCHIE_POINTS.md`, § 7)
   - Synthétique, n = 8 000 : +0,008, +0,026, +0,050 et +0,078 à k = 2, 3, 5 et 10 ; intervalles de confiance tous au-dessus de 0.
   - Trames voisines (859 instances), sauvetages/pertes : +19/−8, +22/−7, +27/−4, +35/−1.
   - Démos : trois vélos sauvés. Exemple, démo 04, vélo C : 0,557 à k=3, contre au plus 0,361 pour HDBSCAN.
6. **Une tête plate exacte.** [F]
   - Aucun flottant dans les décisions : encadrements entiers sur 384 bits, puis repli exact. Le parent ne l'emporte que sur égalité certifiée.
   - Au-delà du budget, `radical_sign_budget` refuse l'appel entier plutôt que de dégrader la sélection.
   - Raffinement en z prouvé à condensation fixée (`receipts/flat_model_followup_20261004`).
   - Natif = Python sur 1 920 appels et 16 474 clusters. Aucun repli exact sur 272 358 décisions réelles.
7. **Le choix LiDAR suit un critère écrit d'avance.** [F] (`SORTIE_PLATE.md`, § 3.1)
   - Sur 1 462 couples (scène, k), l'EOM z=1 retrouve tous les objets dans 68,5 % des couples, contre 56,6 % pour `sklearn`.
   - Taux de fusions : 0,021 contre 0,025.
   - La même tête appliquée à l'arbre de `sklearn` égale `sklearn` (0,564).
8. **Démos et vidéos.** [F] (`Zoltan/demos/`, à la racine du dépôt)
   - Sur 360 bouts de scène : HGP gagne 13 fois et perd 3 fois ; 11 doubles échecs ; 333 doubles réussites, dont les 263 voitures.
   - Sur 97 groupes de vélos et de piétons, gains/pertes de HGP :
     - instances seules : 10/1 à k5, 7/0 à k10 ;
     - scène sans sol : 8/9 à k5, 25/0 à k10.
9. **Polyèdres : l'objet est fixé.** [F]
   - Accord avec l'auditeur (`receipts/audit_hartigan_delaunay_20261006`) : l'objet est A_k(r), la mosaïque d'ordre k filtrée par d_k, et non par la puissance.
   - L'oracle exact ne montre aucun désaccord avec FULL sur 91 ordres et 37 993 coupes. Trois réfutations sont gravées au registre (`1fbeea5b8`).
   - La filtration est robuste (64 cas sur 64) ; les nœuds qui vivent plus de 2δ gardent leur identité.

## 3. Ce qui n'a pas marché, fragile ou abandonné

1. **Supports v1** (tous les Q_b de toute W_K), abandonnés le 6 octobre.
   - [F] Instables : sur le cercle à quatre points, le saut de Hausdorff atteint au moins 1/4 pour une perturbation qui tend vers 0 (`MATHEMATIQUES.md`, § 10.9). Plafond de 24 sites.
   - [F] `kparties_reliees` n'est pas stable non plus : sortir un site de la coquille le fait passer de 3 à 1.
   - [F-hd] Ils ne dessinent pas l'objet : 6 ouvertures gardées sur 44 ; diamètre médian de conv(S\*) de 34 à 36 cm.
   - [F] À une jonction, conv(S\*) est la cellule critique agrandie K fois et retournée : 364 mm contre environ 73 mm à K5.
2. **Un filtre de rôle n'est pas Kruskal.** [F]
   - Le triangle équidistant à K1 ferme un cycle de plateau (`be8085ec1`).
   - Le défaut était présent dans `0cc9cbec4` et a été corrigé en `07428324e`. Kruskal retire 94, 90 et 523 boules sur ng00, ng01 et ng02.
3. **L'arbre d'ordre K brut, non condensé, est inutilisable tel quel comme jetons.**
   - [F-hd] 14,45 nœuds par site à K5, 41,08 à K10.
   - [F-hd] 67,8 % des nœuds à K5 et 85,5 % à K10 meurent à moins de 1 % au-dessus de leur rayon de naissance.
   - [F] 61 à 67 % des nœuds K5 vivent moins de 2δ.
4. **Points : ni les cibles, ni la stabilité par insertion.** [F]
   - Cibles : Q2–Q4 et Q-Π2 perdues ; 70 cellules tenues sur 125.
   - Respect du cœur perdu à k=2.
   - Obstruction de Palm (conditionnelle).
   - Aucune stabilité par insertion : pas de constante uniforme de W_p vers SUP.
   - Les blocs de deux ordres différents se croisent.
   - Le gain au niveau B vient de FULL, pas de la marge : `first` fait mieux à chaque k, `cover` à k ≥ 5 ; la marge coûte 0,002 à 0,006 ; MR₂-bord, sans tour, rattrape aussi le vélo C.
   - E1 à E5 n'ont jamais été joués selon leur définition.
5. **La sélection plate est le goulot.**
   - [F] Une antichaîne oracle retrouverait 252 objets sur 258 ; l'EOM z=1 en retrouve 198.
   - [F] Cause : les séparations fugaces. Par exemple, le vélo 3 n'est séparé qu'entre 107,4 et 107,8 mm, alors que l'union persiste jusqu'à 456,9 mm.
   - [F] z=2 déchiquette les voitures en lignes de balayage.
   - [F-hd] Test synthétique S2b : T_eom2 − R0 = +0,041, +0,045, +0,045 et −0,002 aux quatre k ; T − A vaut environ 0, puis −0,037 à k=10.
   - [I] Le gain vient donc de la tête certifiée, transposable à l'arbre de `sklearn`, et non de la hiérarchie.
   - [F-hd] Le PQ (garde) est significativement pire à k=5 (−0,018) et à k=10 (−0,066).
   - [F] Le préenregistrement n'est pas mené à terme :
     - S3b et P08 (S6/S7) n'ont jamais été lancées ;
     - S2b et S3a n'ont pas de reçu ;
     - H_L1 et H_L2 n'ont jamais été testées ;
     - le choix z=1 sur LiDAR repose sur les exemples de développement.
6. **Le seuil relatif au parent échoue.**
   - [F-hd] REL20 ne garde que 9 objets sur 19, et 0 vélo sur 3 contre un mur.
   - La thèse prescrit un seuil absolu (p. 97).
   - [F] `CLAUDE.md` et `Zoltan/FoundationModel/SPECIFICATION.md` (§ 2.2) demandent pourtant un seuil relatif. La décision de l'utilisateur est en attente.
7. **Polyèdres : taille et robustesse.** [F]
   - Environ 1 000 faces par site à K5, soit 62 à 82 fois la tour.
   - A_5 d'une trame coûterait 6 à 8 s (extrapolation). La réduction certifiée, ×2 à ×7, n'allège pas le dessin.
   - À une coupe critique, le dessin saute pour 44 nœuds sur 141. Un seul site proche crée une composante (33 cas sur 33).
   - Le vélo réel 08/002852 n'est lisible à aucun K.
8. **Les comparaisons sont fragiles.** [F]
   - `sklearn` dépend de la machine : `np.argsort` n'est pas stable dans `_process_mst`.
   - La tête N-aire appliquée à l'arbre de `sklearn` diffère de ses étiquettes sur 520 configurations sur 2 400 (plateaux binarisés, ex æquo).
   - Les versions 1.7.2 (G4) et 1.9.1 (codespace) n'ont jamais été qualifiées sur un même banc (`receipts/flat_selection_evidence_20261004`).
   - L'IoU était arrondi avant le seuil strict de 1/2. Corrigé en `38faaf272` ; impact non établi.

## 4. Pièges et leçons

- **Trois confusions à éviter.**
  - Le rôle n'est pas la sélection, et la suffisance n'est pas la minimalité.
  - Une hyper-arête n'est pas une arête : le Kruskal publié est celui du constructeur, compressé, pas un MST explicite sur Γ_K.
  - La coupe fermée sert à l'attache, la coupe ouverte aux branches ; un plateau se traite en bloc.
- **Témoin D2 (41 < 64 < 1681/25).** Une inégalité de preuve fausse ne doit jamais devenir une garde native.
- **Flottant aux plateaux : seul l'exact tient.**
  - Le filtre `cmp_level` donnait un signe faux à 192 bits.
  - L'oracle décimal était insuffisant à 28 chiffres, puis à 120.
  - Un radicande carré parfait était mal classé.
  - L'EOM en binary64 se trompe aux égalités (F6, F14a).
- **Ce que les contrôles ne prouvent pas.**
  - Le lecteur ne prouve pas la complétude : seul le différentiel contre l'oracle a vu le retrait des tétraèdres du cube.
  - Le niveau B n'est pas une partition.
  - « Identiques » ne couvre que les observables publiés.
  - HDBSCAN se compare sur la même machine, avec une convention de niveau déclarée.
- **Mesurer la règle publiée.** Les sessions A et B mesuraient la marge carrée, et les bancs prenaient m=2 à k=1 jusqu'au 5 octobre.
- **Empreintes à graver par profil.** Des valeurs u21 jouées en u18 et u24 ont donné six échecs, et l'erreur est revenue en v2.
- **Traçabilité.**
  - Des reçus immuables portent des métadonnées fausses (erratum `ef91a7f46`).
  - Des noms entrent en collision : les sessions S6/S7 d'E1 contre les tranches S6/S7 ; « L4 » désigne à la fois la livraison `plat` et le recouvrement GPU.
  - `DECISIONS_UTILISATEUR.md`, qui se déclare prioritaire, n'a jamais reçu la décision du 6 octobre.
- **Lisibilité d'un objet.** Elle vient du rayon choisi le long de sa chaîne d'ancêtres, pas du type de rendu.

## 5. Dettes et problèmes ouverts

**Qualification**
- [F] SPv2 n'a que 15/15 portes en Release u21. Manquent :
  - les mutants sur G4 ;
  - les profils u18 et u24 ;
  - les jumelles `_opt` ;
  - le triangle à W48, la permutation et une cellule à plus de deux branches.
- [F] Couverture indirecte seulement : les matrices courtes ASan et TSan (807/807) de `claudeg1` contiennent `mhgp11_cli_supports_oracle`. Elles ont tourné sur `b6fd3796d`, dont le levier a été retiré.
- [F] Aucune porte K10 pour `supports` ni pour `points`.
- [F] Les différentiels S9 et S10 n'ont pas été rejoués après la garde de chronologie (`b0f2a0a9e`).
- [F] Le coût de SPv2, `points` et `plat` n'est pas mesuré sur G4. Seule mesure, locale : étage `output` de 0,2 à 0,27 s sur ng00.
- [I] Ces qualifications ne se transportent pas au HEAD.

**Documents périmés** [F]
- `README.md` et `SORTIES.md` (§ 2) annoncent encore SP v1 et une « qualification en attente ».
- `MATHEMATIQUES.md` contient deux § 10.10.
- `docs/DEVELOPPEMENT.md` date du 3 octobre.
- `sorties_g4.json` porte une décision et un commit faux.

**Recherche**
- Verrou cibles contre stabilité (P1–P6), critère d'existence des clusters, synthèse multi-K.
- Règle plate encore marquée « Ouvert » ; seuil absolu ou relatif.
- Pas de z optimal universel : 1 sur LiDAR, 2 sur synthétique, 3 pour la densité [M].
- Masse du § 9.1 impossible à calculer depuis SPv2.
- Multiplicités refusées : les retours fusionnés au millimètre ne sont pas traités.
- Un polyèdre à la fois petit, certifié et rapide ; une identité pour les nœuds de vie courte.
- Huit questions à l'auditeur restent sans réponse.

## 6. Recommandations pour la v12

1. **Un produit pivot : un registre d'événements par ordre**, construit une seule fois dans le moteur. Il contient :
   - les nœuds : rang exact, parent, naissance (S\* et K-partie), mort ;
   - les hyper-arêtes de fusion : ant(b) et boule de Kruskal ;
   - les vies, les verticales et les sites couverts à d_v^− ;
   - les masses, entières et au sens du § 9.1.

   Toutes les sorties et le tokenizer en sont des vues. `tree_k_sha256` existe dès la première tranche. [I] Cela évite le cycle SPv1 → SPv2 et l'export de masse manquant.
2. **Garder, par ports explicites :**
   - la transaction de dossier et le manifeste canonique ;
   - les codes 0/2/3 et le déterminisme ;
   - le refus plutôt que la dégradation ;
   - `num` (classes de carrés, encadrements) ;
   - l'oracle borné et le lecteur en bibliothèque standard.
3. **Sorties v12.**
   - `full`, dans un format compact : l'écriture prend aujourd'hui 85 % du temps (2,16 s sur 2,56 s à K5 ; 1,46 Go à K10).
   - `squelette` = SPv2 (Kruskal, S\*), déclaré témoin H0 et non forme.
   - `points` = H^r_{K+1}.
   - `condense`, nouvelle : arbre N-aire à seuil absolu de masse, jamais binarisé.
   - `plat`, à sélection enfichable (EOM z, feuilles, masse), sans revendiquer de règle par défaut.
   - Polyèdres A_k(r) à la demande, hors du chemin des 100 ms : K=2 d'abord, stockés par événements (propriétaire, date).
4. **Ne pas reprendre :**
   - l'énumération de Q_b et le plafond de 24 sites dans le chemin produit (les garder dans l'oracle) ;
   - la publication de `kparties_reliees` ;
   - un seuil relatif qui ne passe pas la fixture du vélo contre le mur ;
   - toute décision flottante.
5. **Protocole de comparaison à HDBSCAN.**
   - `sklearn` épinglé, toujours sur la même machine.
   - Niveaux séparés : tour (A), hiérarchie (B), sélection (C).
   - Bras A et MR_k-bord ; convention de niveau déclarée.
   - LiDAR réel d'abord : lancer P08.
   - Préenregistrement tenu jusqu'au bout ; un reçu par mesure, aucune mesure gardée seulement hors dépôt.
6. **Qualification.** Chaque version de format passe la matrice complète : trois profils, sanitizers, mutants, `_opt`, W48, K10. Un seul fichier d'état, mis à jour à chaque décision de l'utilisateur.

**Questions à trancher d'abord :**
- une sélection sans vérité terrain qui garde les séparations fugaces ;
- seuil absolu ou relatif (décision de l'utilisateur) ;
- le critère d'existence des clusters et la stabilité par insertion ;
- l'identité des nœuds qui vivent moins de 2δ ;
- les multiplicités ;
- le budget de temps de `points` et `plat`.

## 7. Références clés

- **Documents** : `docs/SORTIES.md`, `docs/SORTIE_PLATE.md`, `docs/HIERARCHIE_POINTS.md`, `docs/MATHEMATIQUES.md` (§ 7 et § 10), `docs/PROVENANCE.md` (S5–S10).
- **Code** : `src/api/api.hpp`, `src/supports/hierarchy.cpp`, `src/points/points.hpp`, `src/head/`, `src/io/io.hpp`, `cli/mhgp11.cpp`, `bench/mhgp11_formats.py`, `bench/points_radius.py`, `bench/points_flat.py`.
- **Audits** : `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md`, `audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md`, `audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007.md`.
- **Reçus** (`receipts/`) :
  - qualification : `developpement_20261005/qualification_finale`, `developpement_20261006/supports_kruskal`, `audit_integration_20261006` ;
  - mesures et explorations : `developpement_20261003/points_g4`, `developpement_20261004/e1_sortie_plate`, `polyedre_ordre_k_20261007` ;
  - audits : `audit_supports_20261004`, `audit_supports_mst_20261006`, `audit_hartigan_delaunay_20261006`, `audit_hartigan_robustesse_20261006`, `flat_selection_evidence_20261004`.
- **Plans** : `plans/e1_prereg_*_20261004.json`.
- **Hors dépôt** : `build/v11-persist/polyedres_reconnaissables/SYNTHESE.md`, `build/v11-persist/e1_s2/`, `/workspaces/.ehgp-sessions/`.
