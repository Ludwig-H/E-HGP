# Invariant d'Euler par ordre K — sondes et résultats de l'auditeur C

23 septembre 2026. Note principale :
[NOTE_C_INVARIANT_EULER_20260923.md](../NOTE_C_INVARIANT_EULER_20260923.md).
Hors produit, hors registre, `public_status=not_claimed`, GCP non utilisé.

## Fichiers

- `euler_check.cpp` : exécute la chaîne v9 publiée jusqu'au catalogue
  (`run_tower=false`, `keep_catalogue=true`, leviers par défaut, s = 8),
  somme les contributions génériques par ordre et écrit chaque boule
  dégénérée (coquille recomptée par balayage exact du nuage) dans un JSONL
  local. Le JSONL contient des coordonnées issues de SemanticKITTI : il
  n'est **pas** versionné.
- `euler_degenerate.py` : contribution exacte $1-\chi(\Lambda_m)$ d'une
  coquille quelconque par l'arrangement de grands cercles en entiers, puis
  bilan $E_K$ ; autotests intégrés (q2, q3, q4 génériques et coquille q2 plus
  un point).
- `euler_oracle.py` : oracle exhaustif indépendant du code v9 (rationnels
  exacts, n ≤ 10), vérifie $E_K=1$ pour tous les ordres.
- `run_euler_mutants.py` : compile chaque mutant de
  `morsehgp3D_v9/tests/gen/mutants.json` contre `euler_check.cpp` et classe
  le verdict (`killed_euler`, `killed_chain`, `survived`, `inert`, `error`).
- `run_euler_mutants_p2.py` : protocole « Kmax+2 » contre les mêmes mutants
  (chaîne mutée à K5 et K7, Euler jusqu'à K5 sur le catalogue K7, égalité du
  catalogue K5 avec la restriction du catalogue K7).
- `run_key_compare.py`, `compare_dumps.py` : comparaison clé par clé des
  catalogues mutés et sain (vidage local `EULER_DUMP`, jamais versionné).
- `euler_scale8000_gate.patch` : porte CTest `mhgp9_chain_euler_scale8000`
  (label `scale8000`, hors CI rapide), écrite à la demande du développeur et
  **appliquée** par lui en `a08378da`, qui y a ajouté l'égalité avec les sommes
  `euler_by_k` publiées par la chaîne v13 (`c768e06a`). Trois familles v8
  épinglées à n = 8 000 (`uniform`, `terrain`, `clusters`), chaîne sans tour
  jusqu'au catalogue K5, **validation des listes fournies par la chaîne**
  (signe exact de la puissance de chaque site listé ; aucun site hors liste
  n'est recherché : le recensement complet reste celui de la chaîne), $q_{\min}$
  recalculé par `ShellTable` pour chaque boule, contributions par
  sous-coquilles, Euler pour K ≤ 3 ; puis K7 sur `uniform` (Euler K ≤ 5 et
  restriction égale **clé par clé** au catalogue K5). Depuis la v13, le mutant
  `dead_q3_disk_too_small` est refusé par la chaîne elle-même
  (`chain_catalogue_euler_violated`). Les 52 s mesurés concernent la version
  du correctif, pas le port v13.
- `euler_scale_gate_hygiene.patch` : correctif d'hygiène de cette porte
  (commentaire « validation des listes fournies », `--n` refusé en code 2
  s'il n'est pas un entier décimal, nouvelles portes `--n=8000junk` et
  `--n=abc`, refus au configure d'un mutant nommé absent de
  `tests/gen/mutants.json`) ; testé dans une copie privée.
- `euler_chain_probe.patch` : correctif prêt à porter (chaîne et sonde), non
  appliqué ; `git apply --check` propre sur `4079cceb`.
- `results/` : sorties JSON agrégées (aucune coordonnée), dont
  `mutants_k5.json`, `mutants_protocol_k5_k7.json`,
  `mutants_key_compare_k5.json` et `patched_probe_s02_8000.json`.

## Entrées

Sous-nuages emboîtés sans sol à 1 mm produits par le runner v2 du
développeur (reçu `receipts/lidar_scaling_local_20260923`, champ
`points_sha256` concordant pour 08/000000 8k) :

```text
cf84866390ede2e53a20b4166e8dc1a14cb5791eb78b6efba147941b7cd61c54  s00 8000
adcc30417de9ed88ab3b44bf19d6a658bf1fc45451ab82375b322674f0b2ca0a  s00 16000
bf61b64c1281245ec37767f22714bd7ab9fee1ee8c7290ba7be003cc62068dd9  s00 32000
d141e843dcdd3c56d3b396431c9699d94b45b586bc318de70f7aeb77049a6d2a  s01 8000
ab488fd0ae1fef9f8aad57f5586533924bd7b4df602f4b8c28f9cec80a64278f  s01 16000
52bea2a0e853e861d308cee26b4952755f5f70408a88e157907d1030f348bf61  s01 32000
255f6433034a6d9e1531dce11cb880725c3bf3e46b6b66078596341dbd7f8d5e  s02 8000
6461cf43c258bc31cf3e9e1170a0b5af9a0e4a4d2f6216cdc58a78ea1665138a  s02 16000
ecfd72935b7609a7e7b8ba827bebe960535c03e9fc7312df1fd2695061370bb1  s02 32000
```

## Reproduction

```bash
# sources de la chaîne : archive du commit (src et bench identiques à 4530644b)
git archive 93066733 morsehgp3D_v9 | tar -x -C "$SRC"
cmake -S "$SRC/morsehgp3D_v9" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=<prefixe boost>/usr
cmake --build "$BUILD" --target mhgp9_chain -j2
g++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  -I"$SRC/morsehgp3D_v9" -I"$SRC/morsehgp3D_v9/src/gen" euler_check.cpp \
  "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" -lpthread -o euler_check
./euler_check s00_8000.u32le 10 2 deg.jsonl > euler.json
python3 euler_degenerate.py deg.jsonl 10 "$(python3 -c 'import json;print(json.dumps(json.load(open("euler.json"))["generic_sum"]))')" 8000
python3 euler_oracle.py 1 400
```
