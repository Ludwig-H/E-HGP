# Addendum — banc apparié contrebalancé, `first_batch` hors sémantique, ordonnancement à budget

Date : 19 août 2026 UTC. Exécute deux audits reçus le même jour :

- **contre-audit `21e617d`** — le banc d'internement n'était pas
  contrebalancé et publiait la mauvaise statistique ;
- **réponse `95061c1`** — `first_batch` doit sortir de la sémantique, et
  l'ordonnancement des dix folds demande un budget mémoire, pas un choix
  binaire.

## 1. Ce que le contre-audit corrige, et il a raison

**Biais d'ordre.** Le banc exécutait toujours `tri` puis `streaming` :
le second mode héritait systématiquement de l'état laissé par le
premier (pages engagées, fréquence, caches). L'audit note que le gain
plus fort à n=4000 qu'à n=8000 était même *compatible* avec un coût de
démarrage fixe — donc ne démontrait aucune propriété d'échelle.

**Statistique.** Les mesures sont appariées dans le même processus :
l'estimateur est la médiane des rapports par paire, jamais le rapport
de deux médianes marginales. Sur les cinq paires publiées le 18 août,
les deux divergent — `0,843` (« ×1,19 ») contre médiane appariée
`0,926` (« ×1,08 »). **La conclusion « ×1,19 » et la phrase « le gain
décroît avec la taille » sont retirées.**

## 2. Le banc corrigé

- **échauffement** : un passage non chronométré de chaque mode ;
- **ordre contrebalancé ABBA** : paire paire = tri puis streaming,
  paire impaire = streaming puis tri — chaque mode occupe autant de
  fois chaque position ;
- **plan refusé avant calcul** (code 2) si `--bench-repeat` est impair
  ou `< 4` : le contrebalancement est alors impossible ;
- **signature du résultat complet** comparée à chaque exécution, hors
  chronométrage — le banc prouve qu'il mesure bien le même objet ;
- **publication des mesures brutes** : chaque exécution dans son ordre
  réel, les dix rapports appariés, la médiane appariée, la médiane des
  différences logarithmiques, le rapport de médianes (en second, pour
  relire les anciens reçus), les victoires et l'équilibre des positions.

### Résultat, n=8000, K=10 (718 440 événements, 7 902 840 incidences)

Série de référence, **binaire définitif**, dix paires contrebalancées.
Rapports appariés, dans l'ordre d'exécution :

```text
0,8797  0,8459  0,8963  0,8615  0,9139
0,8661  0,8548  0,8741  0,8829  0,8900
```

```text
mediane_appariee    = 0,8769     -> x1,14   (ESTIMATEUR)
mediane_log         = -0,1313    -> x1,14   (coherent)
rapport_de_medianes = 0,8853     -> x1,13   (publie en second)
victoires_streaming = 10/10
ordre_tri_premier   = 5/5        (plan equilibre)
objet_identique     = oui
```

Test des signes unilatéral : `P(X >= 10 | Bin(10 ; 0,5)) = 1/1024 =
0,00098`. Le gain retenu est donc **×1,14 sur l'internement du K
dominant**, avec dix victoires sur dix et une dispersion serrée
(0,846–0,914).

Seconde série, même protocole, sur le binaire d'avant le passage par
référence — publiée parce qu'elle est cohérente et qu'elle montre la
dispersion réelle :

```text
0,8002  0,9314  0,8489  1,0854  0,7450
0,6384  0,7502  0,8698  0,9566  0,8418
mediane_appariee = 0,8453 -> x1,18 ; victoires 9/10 ; P = 11/1024 = 0,0107
rapport_de_medianes = 0,7561 -> x1,32
```

Les deux séries s'accordent sur l'ordre de grandeur (**×1,14 à ×1,18**)
et divergent fortement sur le rapport de médianes (×1,13 contre ×1,32,
et ×1,08 sur les cinq paires biaisées du 18 août) : cette statistique
SURESTIME ou SOUS-estime selon l'échantillon, ce qui est précisément
l'argument de l'audit.

Aucune conclusion d'échelle n'est tirée : deux tailles ne font pas une
pente, et le contre-audit a montré qu'un coût de démarrage suffisait à
fabriquer l'apparence d'une.

## 3. `first_batch` sort de la sémantique (réponse `95061c1` § 1-2)

L'auditeur renforce le théorème posé la veille : sur un flux sans
`attach_violations`, `T_b(f) => ¬S_b(f)`, donc `S_b(f) => A_b(f)`, et

```text
prebatch_root = active
born          = attach && !active
new_attachment= attach && !active
```

`first_batch` n'était donc pas une entrée du calcul. Il est **supprimé**
et remplacé par un bit `seen` mis à jour **après** le macro-lot, qui
n'alimente que les deux compteurs. Gains concrets, au-delà de la
clarté de preuve :

- un `u32` par facette unique disparaît de `FacetIntern` —
  **74,3 Mio touchés** à n=8000 ;
- une **écriture aléatoire** de moins par sondage réussi dans le chemin
  chaud ;
- la sémantique ne dépend plus d'une donnée globale utile au seul
  diagnostic.

**Portes.** Le mutant `intern-first-batch-last` disparaît (il testait
une redondance, pas une entrée). Deux mutants le remplacent, comme
demandé :

- `attach-detector-disabled` — tué par la fixture de flux incohérent,
  seul endroit où le compteur peut être non nul ;
- `seen-before-check` — `seen` marqué AVANT le contrôle : tous les
  premiers attachements sont signalés à tort, et les familles
  géométriques régulières le tuent (la porte compare désormais aussi
  `attach_violations` et `birth_violations` entre backends).

**Une conséquence assumée.** La fixture de flux incohérent n'exige plus
l'égalité des sorties entre le backend figé et le fold compact : ce
flux viole par construction l'hypothèse du théorème sous laquelle ils
sont prouvés égaux. Elle exige ce qu'elle doit exiger — que le
détecteur parle (`attach_violations = 1`) et que les lots soient justes.
Exiger l'égalité y aurait gravé un comportement hors contrat.

## 4. Ordonnancement des dix folds (réponse `95061c1` § 3)

**Étape préalable exécutée.** `build_forest` recevait ses événements
**par valeur** pour les trier : chaque fold concurrent payait une copie
complète (~130 octets par événement, ~400 Mo cumulés à n=8000). Il
reçoit désormais une référence constante et trie une **permutation
compacte d'indices u32**. Le contrat public est intact :
`batch_of_event` et `ev_fid` restent indexés par la position dans
l'ordre trié.

**Coût par ordre, mesuré avant lancement** (n=8000) :

| K | incidences | part | majorant d'octets |
|---|---|---|---|
| 1 | 61 162 | 0,2 % | 13,0 Mo |
| 2 | 192 291 | 0,7 % | 40,5 Mo |
| 3 | 437 820 | 1,6 % | 89,4 Mo |
| 4 | 827 740 | 3,1 % | 168,1 Mo |
| 5 | 1 395 138 | 5,2 % | 286,5 Mo |
| 6 | 2 171 778 | 8,1 % | 458,6 Mo |
| 7 | 3 182 288 | 11,9 % | 638,3 Mo |
| 8 | 4 457 358 | 16,7 % | 931,6 Mo |
| 9 | 6 022 120 | 22,6 % | 1 208,6 Mo |
| 10 | 7 902 840 | 29,7 % | 1 540,9 Mo |

Total : 26 650 535 incidences, 5 375 578 090 octets de majorant cumulé.

**Les trois modes, même processus, mêmes événements** (`n=8000`,
4 fils, budget 2 Gio) :

| mode | latence | pic RSS | réserve max | signature |
|---|---|---|---|---|
| `contiguous_reference` | 29 951 ms | 4 932 596 Kio | — | `ab003a59158fc7c8` |
| `LPT_unbounded` | **10 705 ms** | 5 230 304 Kio | 4 319 406 292 | identique |
| `memory_budgeted_LPT` | 21 323 ms | 4 952 616 Kio | 2 140 153 484 | identique |

Lecture :

- le découpage contigu donne bien à un ouvrier `{8,9,10}`, soit **69 %
  du travail** — d'où sa latence ;
- `LPT_unbounded` divise la latence par **2,80**, mais réserve
  **4,32 Go**, soit deux fois le budget : c'est une **borne de
  latence**, jamais un défaut, exactement comme l'audit le prescrit ;
- `memory_budgeted_LPT` divise la latence par **1,40** pour **+0,4 %**
  de pic RSS (+20 Mio) et **respecte le plafond** (2 140 153 484 ≤
  2 147 483 648).

Les trois signatures sont **identiques** : l'objet ne dépend pas de
l'ordonnancement, seules la latence et le pic bougent.

**Défaut changé, après la comparaison exigée** : la production utilise
`memory_budgeted_LPT`, budget déclaré par `--fold-memory-budget`
(2 Gio par défaut) et publié avec la réserve atteinte sur sa **propre
ligne** — la ligne `execution` est ancrée `^…$` par le validateur de
campagne et n'est pas touchée :

```text
fold_ordonnancement budget_octets=2147483648 reserves_max_octets=…
```

Garde anti-blocage explicite : une tâche seule plus grosse que le
budget entier s'exécute quand rien d'autre ne tourne. Refuser serait un
interblocage, tronquer serait un mensonge.

## 5. Portes

`ctest --test-dir build/v4` : **134 tests, tous verts**.

Nouvelles :

- `mhgp4_forest_probe_detector_mutant_disabled` (code 4) ;
- `mhgp4_forest_probe_detector_mutant_seen_before` (code 4) ;
- `mhgp4_forest_probe_bench_report_gate` — porte **synthétique** du
  rapporteur, temps fictifs, aucun calcul réel : médiane appariée
  correcte, jeu qui SÉPARE les deux statistiques (0,75 contre 1,1667),
  victoires et équilibre publiés, `--bench-repeat` impair ou `< 4`
  refusé ;
- `mhgp4_forest_probe_bench_refus_plan` (code 2) — le banc lui-même
  refuse un plan non contrebalançable.

Piège gravé : la première écriture de la fixture du rapporteur
attendait `0,875` au lieu de `0,75` (moyenne mentale au lieu de la
médiane d'un échantillon pair). La porte l'a refusée **avant tout
banc** — exactement son travail, et la deuxième fois de la journée
qu'une fixture arithmétique attrape son auteur.

## 6. Reproduction

```bash
cmake -S morsehgp3D_v4 -B build/v4 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v4 -j
ctest --test-dir build/v4 --output-on-failure          # 134 tests
./build/v4/mhgp4_forest_probe --bench-report-gate
./build/v4/mhgp4_forest_probe --fold-intern-bench --family=uniform \
    --n=8000 --s=8 --smax=11 --seed=3 --threads=4 --bench-repeat=10
./build/v4/mhgp4_forest_probe --fold-schedule-bench --family=uniform \
    --n=8000 --s=8 --smax=11 --seed=3 --threads=4 \
    --fold-memory-budget=2147483648
```
