# Pas de descente : deux leviers de constante, et chronologie du pipeline des forêts

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261006.claudev3c`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudev3c/receipt.json`). Source exécutée : `279f6df9e` (code des leviers : `751686868`). Toutes les
mesures portent sur les trois trames LiDAR réelles ng00, ng01 et ng02. Aucune mesure ne promeut un statut public.

## Objet

Deux leviers de constante du pas de descente, qui laissent les sorties inchangées :
- `add_descent` somme les 37 registres de descente sans branche par champ, cumule les débordements et ne publie la
  somme qu'à la fin. Le refus (`tower_capacity`, somme intacte) est le même ; la porte `mhgp11_tower_descent_capacity`
  le vérifie champ par champ ;
- `LatticeSphere` copie une fois par parcours l'ancre, N et D. Elle évalue la puissance aux points de la boîte et aux
  sites par l'expression de `native_power` (mêmes budgets), sans vue ni `Point` refabriqué à chaque appel.

Profil d'instructions de la résolution régulière (callgrind, ng00, K = 5, un fil) : 17,34 G → 15,71 G (×0,906),
même vidage.

## Exactitude

Six mutants tués par le code de sortie de leur porte (`mut_tower.*`, `mut_num.*`) :
- `registres_debordement_ignore` et `lattice_puissance_sans_ancre_z`, nouveaux ;
- `descent_cumul_non_transactionnel`, `diametre_cumul_descente_omis`, `memo_cumul_queries_omis` et
  `descent_singleton_sum_missing`, retargetés sur la nouvelle somme. Le deuxième n'aurait plus été tué sans ce raccord.

Trois bancs `conforme` ; toutes les prises rendent les vidages des empreintes des trames, à K5 et à K10.

## Règle écrite dans le plan avant la session, et mesure

Variantes `new` (cette source) et `v3` (archive de `a1b640763`, V3 sans les leviers ; somme dans
`archives_variantes.sha256`). Statistique : `forest_ms` à froid, médiane de 6 processus neufs par trame et par
variante. Règle : leviers gardés si la moyenne géométrique des 6 rapports new/v3 à K5 est ≤ 1,00 et si aucun rapport ne
dépasse 1,03.

| Bras | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| K5 CPU, feuilles 16 (278523) : v3 → new (ms) | 134,0 → 127,5 (0,952) | 108,9 → 104,2 (0,956) | 139,3 → 133,9 (0,961) |
| K5 GPU+C, feuilles 24 (344059) : v3 → new (ms) | 133,2 → 130,9 (0,982) | 108,0 → 103,7 (0,961) | 132,7 → 130,7 (0,986) |
| K10 GPU+C, feuilles 24, descriptif (ms) | 1 395 → 1 332 (0,955) | 1 030 → 970 (0,942) | 1 152 → 1 123 (0,975) |

Moyenne géométrique à K5 : **0,966**, pire rapport **0,986**. **Les leviers sont gardés.**

## Chronologie du pipeline (descriptive)

`bench/gpu_ab.py` garde désormais, par prise, les phases de l'étage, les voies de résolution, et pour chaque ordre le
publieur et le suiveur vertical. Médianes des six prises à froid de `new`, K5 CPU (ms depuis le début du pipeline ;
le temps CPU est cumulé par tâche) :

| Trame | Étage | Fin des résolveurs | Fin des publieurs 3 / 4 / 5 | CPU des publieurs 3 / 4 / 5 | Attente des publieurs 4 / 5 | Fin des verticales 5 |
| --- | ---: | ---: | --- | --- | --- | ---: |
| ng00 | 127,5 | 82,1 | 92,8 / 105,2 / 101,2 | 77,7 / 104,9 / 97,7 | 0,3 / 1,4 | 107,1 |
| ng01 | 104,2 | 63,6 | 66,6 / 86,0 / 79,6 | 61,1 / 85,7 / 75,9 | 0,3 / 1,8 | 86,4 |
| ng02 | 133,9 | 75,6 | 87,8 / 102,5 / 99,9 | 80,5 / 101,1 / 99,5 | 2,3 / 0,5 | 109,3 |

Les résolveurs ne sont plus le chemin critique.
- Les publieurs des ordres 4 et 5 calculent pendant tout le pipeline, sans presque jamais attendre une résolution, et
  finissent 20 à 27 ms après les résolveurs. Leur temps CPU sous cohabitation (76 à 105 ms) est environ le double de
  celui de la publication isolée, mesurée avant V3 à 35–51 ms.
- Le reste de l'étage tient avant et après le pipeline : préparations, classification (2 à 3 ms), naissances (7 à
  11 ms), libérations ; soit 18 à 25 ms.

Prochain levier de l'étage : le débit du publieur (O2 de la carte : flux compact, état DSU resserré), puis les phases
autour du pipeline (O5).

## Complément synthétique (sans valeur de décision)

Sur les nuages uniformes des tailles d'intérêt (8 000, 16 000 et 32 000 points, K = 5), V3 réduit les tests de points
du census à ×0,155, ×0,139 et ×0,136, avec des sorties et des autres registres identiques. Ce complément vérifie
seulement que l'arbre radix ne dégrade pas un nuage sans lignes de balayage. Les décisions se prennent sur les trames
LiDAR réelles.

## Pièces

`claudev3c/` : `plan.json`, `launch.json`, `receipt.json`, rapports `gpu_ab_report_*.json` (trois bras, chronologie
par prise comprise), rapports des mutants, `archives_variantes.sha256`. Aucune donnée ni coordonnée LiDAR. `SHA256SUMS`
couvre tous les fichiers sauf lui-même.
