# Canal d'audit de la v12

7 octobre 2026. Règles proposées, tirées des leçons du canal de la v11 (audit géant, § 7.9 et § 9.5 ; passation, § 6.4).
Elles s'appliquent dès l'ouverture formelle.

## 1. Règles

- **Rôles stables** : un développeur, des auditeurs nommés ; l'utilisateur tranche les contrats. Un rôle ne change pas
  en cours de chantier sans le dire ici.
- **Une note vivante d'une page par acteur.** L'historique va dans des reçus immuables (`receipts/`), pas dans des notes
  réécrites en place : la v11 a eu des notes de 9 500 à 17 500 mots et 74 à 89 versions, où l'état courant se perdait.
- **Nommage** : `AUDIT_*`, `CONTRE_AUDIT_*`, `ADDENDUM_*` pour les auditeurs ; `REPONSE_CLAUDE_*`, `QUESTION_CLAUDE_*`,
  `NOTE_CLAUDE_*` pour le développeur ; date `_AAAAMMJJ` ; tout document qui juge du code est ancré à un commit.
- **Registre des constats** (`audits/CONSTATS.md`, à créer à l'ouverture), une ligne par constat : identifiant `CST-nnnn`,
  date, rôle de l'auteur, classe (exactitude, concurrence, mémoire, numérique, outillage, mesure, document), gravité,
  pin, témoin (fixture ou porte), état (ouvert, en cours, clos, refusé), preuve de clôture (commit et porte). Un outil
  `tools/check_*` le contrôle.
- **Relecture avant adoption** de tout changement de budget mémoire, de concurrence ou de format de sortie. La v11 a
  adopté le cache de blocs et le publieur O2 sans relecture.
- **Les audits motivent des corrections, ils ne certifient rien** ; aucun banc ne promeut un statut public.

## 2. Constats reportés de la v11

À reprendre au registre dès l'ouverture ; la colonne de droite dit ce que la v12 en fait.

| Constat (v11) | Lieu dans la v11 | Pour la v12 |
| --- | --- | --- |
| différentiel canonique v10/v11 jamais fermé sur trames ; portes `diff_v10` absentes des matrices | `reference/tests.cmake` | différentiel contre la v11 dès la première tranche |
| code exact du gel jamais passé en matrice ; mutants au profil u18 ; 17 mutants jamais joués ; `--check` ne vérifie pas l'existence de la porte | `tools/g4_matrix.json`, `tests/mutants/run_mutants.py` | matrice au profil produit, mutants compris, à chaque jalon ; contrôle d'existence des portes |
| portes LiDAR sautées en silence sans `MHGP11_DATA_DIR` | `tests/*/tests.cmake` | une donnée absente rend la porte rouge ou la déclare absente au résumé |
| runner de matrice sans `--output-on-failure` | `tools/g4_matrix.py` | à corriger dans l'outil de la v12 |
| isolation des processus de banc non certifiée ; `gpu_ab.py` sans groupe de processus | `tools/g4_matrix.py`, `bench/gpu_ab.py` | banc avec groupe de processus et contrôle de quiescence |
| juges de banc ad hoc, non hachés au lancement ; modes en masques entiers | `bench/` | juge unique en bibliothèque, haché ; modes nommés |
| cache de blocs : blocs inactifs et arrondi de classe hors du budget compté, jamais testés sous limite finie | `src/core/buffer.cpp` (`ccdd4db75`) | cache compté comme réserve, porte « limite finie + cache », passe `poison` avec cache |
| exécuteur partagé : deux fils pilotent le même budget, contre le contrat « un seul pilote » | `src/catalogue/leaf_batch_split.cpp` | budget à plusieurs pilotes déclaré |
| repli `unresolved` en série | `src/catalogue/single_pass_batch.cpp` | repli parallèle et budgété |
| garde du préchargement O2 correcte mais non mutable | `src/tower/forest_concurrent.cpp` | prédicat de garde en fonction pure, testé à la borne |
| portes numériques natives demandées (q3 extrême, q4 à $2^{20}$ et $2^{20}+1$, préfixe obtus, coquille à $q=2$), couvertes côté hôte seulement | `mhgp11_catalogue_leaf_narrow` | portes natives, y compris sur l'appareil |
| squelette (SPv2) non équivariant par translation ; $S^{*}$ qui saute sur le cercle | `src/supports/`, `docs/SORTIES.md` § 10 | départage par coordonnées ; témoin `WIT-TRANSL` |
| ligne E du registre fusionnée à D2 ; suffisance de Kruskal absente ; juge EMST jamais implanté | registre racine l. 1370 ; `docs/MATHEMATIQUES.md` | section V12 du registre ; `JUG-EMST` |
| une porte lit `receipts/` | `bench/full_baseline_source.py` | interdit |
| identité du compte GCP dans 528 fichiers de reçus ; 2 041 copies de sources | `receipts/`, `gcp-migration/v11_session.py` | reçus sans identité ni arbres de sources |
| scan KITTI brut versionné | `morsehgp3D_v8/receipts/q34_spatial_20260921/gcp_package_r1/snapshot.tar.gz` | décision de l'utilisateur (D14) |
| `CLAUDE.md` en retard (v11 en u18, pas de v10, seuil relatif attribué à tort à la thèse) | `CLAUDE.md` | ouverture ([`../docs/OUVERTURE_PROPOSEE.md`](../docs/OUVERTURE_PROPOSEE.md)) |

## 3. Questions restées sans réponse dans la v11

- **T1–T2** : un prédicteur exact et bon marché du travail d'une feuille ; le découpage des seules feuilles lourdes est-il
  couvert par les preuves de complétude ?
- **V1–V2** : un invariant structurel par nœud de l'arbre radix ; un contrat de budget supposait-il la forme médiane ?
- **X (= Y3)** : rendre mutable la garde du préchargement d'O2.
- **Y1** : pourquoi les pages de 2 Mio perdent sur 48 fils.
- **Y2** : un compte honnête de la réutilisation des blocs.
- **Polyèdre d'ordre k** : les huit questions de
  `../../morsehgp3D_v11/audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007.md` (§ 4).
- **Contre-lecture** des lemmes de la conception d'origine (`LEM-T1`, `LEM-T3` à `LEM-T7`), préalable à la tranche T2.
