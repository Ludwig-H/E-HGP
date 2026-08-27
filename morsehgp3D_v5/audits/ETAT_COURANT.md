# État courant — morsehgp3D_v5

Date : 27 août 2026. Auteur : Claude (implémenteur) — **ce fichier n'est pas un
audit** ; il devient le verdict de l'auditeur dès son premier passage.
Pin : le commit qui contient ce fichier (voir `git log -- morsehgp3D_v5/audits/ETAT_COURANT.md`).
Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Ce qui est livré

- Fondations, familles bit à bit (12 digests v4 gravés), index spatial, WSPD,
  fuseaux/témoins, formes exactes q2/q3/q4, filtre flottant certifié,
  génération à trois lanes, RLE, préfiltre, census, plateaux, fold par K,
  digest au format v4, pipeline en bibliothèque (`src/pipeline/run.hpp`) et
  pilote `cli/mhgp5`.
- **Conformité v4 ≡ v5 mesurée** (`digest_balls` et `digest_all` identiques,
  reçu `receipts/conformite_v4/digests_v4.txt` calculé par la v4) : toutes les
  familles à n=400, `uniform`/`eight_clusters` à 1200, `two_lines` à 2000, et
  **les quatre familles de mesure à n=8000** (`uniform` 129 s, `terrain`
  19 s, `eight_clusters` 219 s, `scanline_single_pass` 24 s, 8 fils, machine
  partagée avec la campagne de référence v4 — pas une mesure de coût). Les
  tailles 16000 et 32000 sont enregistrées (labels `scale16000`,
  `scale32000`) et restent à exécuter.
- Portes CTest (label `gate`) : arbre, familles + mutant, WSPD (ledger,
  équivariance, `wspd-drop-rect`, portes appariées cap/scission), fuseaux
  (juge fail-open, `core-ball-ceil-distance`, `witness-no-lane-mask`).
  Labels `scale8000/16000/32000` : conformité v4 par famille.

## Ce qui n'est PAS livré (et ne doit pas être lu comme acquis)

- l'oracle indépendant v5 (`oracle/`) : les portes q3/q4/forêt à petit n, le
  juge du juge, les fixtures u16 extrêmes — **à écrire** ; jusque-là la seule
  autorité de correction est la conformité au digest v4, qui hérite des
  limites de la v4 ;
- les mutants déclarés dans `kMutants` sans porte à code 4 (liste : `grep`
  des `MHGP5_MUTANT` contre `CMakeLists.txt`) ;
- le rendu § 9.1, la porte de relabeling, les gardes de capacité de sortie,
  la comptabilité mémoire par rôle (§ 4 de `../docs/ARCHITECTURE.md`) ;
- tout claim de temps, de capacité (50 k, 10 M+) ou GPU.

## Verrous soumis à l'auditeur

Voir `QUESTION_CLAUDE_VERROUS_OUVERTURE_20260827.md`.
