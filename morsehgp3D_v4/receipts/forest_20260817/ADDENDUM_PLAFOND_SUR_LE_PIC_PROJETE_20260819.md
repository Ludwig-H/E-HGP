# Addendum — le plafond de sortie portait sur la mauvaise grandeur

Date : 19 août 2026 UTC. Exécute le reste ouvert de l'item 2 de la
feuille de route (`PASSATION.md` § 5, audit `57523a` § 3) : *« `octets_resident`
ne compte que `evenements * sizeof(ForestEvent)` et doit devenir
`bytes_forest_events` + bornes par tampon »*.

En allant les câbler, j'ai trouvé pire que ce que l'audit annonçait : les
bornes par tampon **existaient déjà** et étaient **publiées** par le
préflight — mais le **plafond transactionnel ne les regardait pas**.

## 1. La faute, en un chiffre

À n=8000 (`uniform`, s=8, smax=11, seed=3), le préflight publie depuis
plusieurs sessions :

```text
preflight total evenements=3126158 incidences=26650535
                bytes_forest_events=450166752
preflight buffers bytes_facet_incidence_records=1385827820
                  bytes_event_to_fid=137550952
                  bytes_unique_facets_upper=1172623540
                  bytes_union_find=106602140
                  bytes_deltas_upper=200074112
                  bytes_partition_upper=1279225680
```

Et le plafond `--max-output-bytes` décidait sur la **première** ligne
seule :

```cpp
const u64 projected = tot_ev * (u64)sizeof(ForestEvent);   // 450 Mo
if (projected > a.max_output_bytes) { ... return 2; }
```

Un plafond réglé à 600 Mo passait donc, et le fold réservait ensuite
**2,60 Go**. La garantie annoncée — « borner la mémoire résidente » —
était fausse d'un facteur **5,77**.

Ce n'est pas une garde absente : c'est une garde qui **se lit comme une
garde** et refuse au mauvais endroit. C'est strictement pire, parce
qu'un appelant qui lit le refus en déduit que le plafond tient.

## 2. Le pic projeté, et sa preuve

Le plafond porte désormais sur

$$\text{pic} = B_{\text{ev}} + \min\left(\max(\text{budget}, \max_K m_K), \; \sum_K m_K\right), \quad B_{\text{ev}} = \sum_K E_K \cdot \texttt{sizeof(ForestEvent)}$$

où $m_K$ est le majorant d'octets temporaires du fold de niveau $K$.

**Deux termes, deux natures.**

1. `ev_k` porte les événements de **tous** les $K$ à la fois — les folds
   ne démarrent qu'après l'expansion complète. Ce terme est une
   **somme** sur $K$.
2. Les folds, eux, sont ordonnancés **sous budget**. Ce terme n'est donc
   ni une somme ni un simple maximum.

**Preuve de la borne du second terme**, sur les deux règles d'admission
de `run_folds_budgeted` (`fits || alone`) :

- une tâche admise par `fits` laisse `reserve <= budget` ;
- une tâche admise par `alone` l'est avec `running == 0`, donc
  `reserve == 0` avant, donc `reserve == m_K <= max_K m_K` après — et
  tant qu'elle tourne, `running != 0` interdit toute autre admission.

D'où `reserve <= max(budget, max_K m_K)`. Et trivialement la réserve ne
dépasse jamais la somme de toutes les tâches, d'où le `min`.

**Le `min` n'est pas cosmétique.** Sans lui, un nuage dont les dix folds
tiennent dans 20 Mo se verrait imputer les 2 Gio du budget, et le plafond
refuserait un run qui tient largement. Le test existant
`mhgp4_forest_probe_max_output_passe` (n=400, plafond 1 Go) serait passé
au rouge : c'est lui qui m'a fait resserrer la borne. **Un majorant qui
refuse à tort est une faute au même titre qu'un majorant qui accepte à
tort**, et je n'aurais pas trouvé celle-là sans une porte préexistante
qui exerçait le cas d'acceptation.

**Une seule borne pour deux usages.** $m_K$ n'est pas une nouvelle
formule : c'est exactement `fold_bytes_upper`, celle que l'ordonnanceur à
budget utilise déjà, factorisée en une forme **comptée**
(`fold_bytes_upper_from_counts(E, W)`) qui ne demande que le nombre
d'événements et le nombre d'incidences — donc calculable au préflight,
avant qu'un seul événement soit matérialisé. Deux bornes indépendantes
auraient pu diverger ; celle-ci ne le peut pas.

**Portée, dite explicitement.** Le pic couvre ce que la
**matérialisation ajouterait**. Les `cands` et `balls` amont sont déjà
résidents quand le préflight s'exécute — le chemin s'appelle
honnêtement `event_expansion_preflight_after_census` — ils sont
mesurables et non projetés, et ne sont pas dans ce compte. Le préflight
par tuile de clés reste ouvert.

## 3. Mesures

À n=8000 :

```text
preflight pic_projete bytes_evenements=450166752
                      bytes_fold_somme=5375578090
                      bytes_fold_max=1540923248 fold_max_k=10
                      bytes_fold_pic=2147483648
                      bytes_pic=2597650400 budget_fold=2147483648
```

Le plus gros fold ($K=10$) tient sous le budget ; c'est donc le budget
qui borne, et le pic vaut **2,60 Go** contre **450 Mo** pour le seul flux
— facteur **5,77**.

À n=400 : flux 15,1 Mo, somme des folds 176,1 Mo, pic **191,2 Mo**. Ici
la somme est sous le budget, donc c'est elle qui borne — les deux
branches du `min` sont exercées par les deux tailles.

## 4. Portes

**Porte de décision, sans aucune allocation.** C'est le seul type de
porte qui puisse voir cette faute : un plafond menteur ne plante pas, ne
change aucune sortie, et n'apparaît sur aucun chrono ni sur aucun pic RSS
(le pic réel *est* atteint — c'est la **garantie** qui est fausse, pas le
calcul). Elle compare la décision accepter/refuser à ce que la borne
comptée impose.

Les comptes sont **gravés**, et relevés du régime d'intérêt (préflight
réel à n=8000) plutôt qu'inventés : si la forme par $K$ changeait au
point de rendre le terme du fold négligeable, la porte le dirait.

```text
output_budget_gate violations=0 bytes_evenements=450166752
    bytes_fold_somme=5375578090 bytes_fold_max=1540923248(K=10)
    bytes_pic=2597650400 facteur=5.77
    refus_par_le_fold=1 acceptations=1 pics_par_k=1
```

Quatre cas : plafond strictement **entre** le flux et le pic (refus
exigé — le cas discriminant), **au-dessus** du pic (acceptation exigée,
sans quoi « refuser toujours » serait vert), **sous** le flux (l'ancienne
garantie n'est pas perdue), et un $K$ dont la borne dépasse le budget (le
terme est bien un `max`, pas le budget). Planchers sur les trois rôles,
plus un plancher sur le **facteur** lui-même : si le pic cessait de valoir
au moins deux fois le flux, la porte perdrait son objet et doit le dire
plutôt que rester verte.

**Mutant `budget-events-only`** — le terme du fold retiré, c'est-à-dire
exactement le comportement d'avant : tué en code 4.

**Et le câblage, sur le chemin réel.** À n=400, un plafond de 100 Mo
(entre 15,1 et 191,2) doit refuser :

```text
REFUS resource_exhausted : pic projete 191167998 octets
  (evenements 15091488 + fold 176076510 ; somme des folds 176076510,
   plus gros fold 47922318 a K=10 ; 104802 evenements)
  > plafond max_output_bytes=100000000 — aucune materialisation
```

La même ligne avec `--inject=budget-events-only` rend **0** : la
comptabilité d'avant acceptait ce run.

## 5. État

`ctest --test-dir build/v4` : **147 tests**, tous verts (144 + la porte,
son mutant et le câblage). `check_docs`, `check_passation`,
`check_implementation_status`, `check_scope`, `check_references` : verts.

## 6. Ce qui reste ouvert sur cet item

- **Préflight par tuile de clés** : `cands` et `balls` restent
  matérialisés avant le branchement. Le pic ne les compte pas et le dit ;
  il ne les remplace pas.
- Le **tuilage** qui permettrait de dépasser les limites u32/i32 au lieu
  de refuser (§ 2.8, partie OPEN) est intact : cette livraison rend le
  refus honnête, elle ne recule pas la limite.

## 7. Reproduction

```bash
cmake -S morsehgp3D_v4 -B build/v4 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v4 -j
ctest --test-dir build/v4 --output-on-failure                    # 147 tests
./build/v4/mhgp4_forest_probe --output-budget-gate                          # 0
./build/v4/mhgp4_forest_probe --output-budget-gate --inject=budget-events-only  # 4
./build/v4/mhgp4_forest_probe --family=uniform --n=400 --s=8 --seed=3 \
    --min-balls=100 --min-fusions=100 --max-output-bytes=100000000          # 2
./build/v4/mhgp4_forest_probe --family=uniform --n=8000 --s=8 --smax=11 \
    --seed=3 --threads=4 --output-preflight-only
```
