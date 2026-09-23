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
- `results/` : sorties JSON agrégées (aucune coordonnée).

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
