# Addendum — le plafond mémoire, corrigé une seconde fois, et le budget de fold qui aurait vidé la campagne G4

Date : 27 août 2026 UTC. Exécute la **Priorité 0** de l'audit `bab37b9`
(« État courant — audit complet de `morsehgp3D_v4` », 22 août), points 1
à 3, et corrige un second défaut trouvé en préparant la campagne G4.

---

## 1. L'audit a raison, et sur mon propre correctif

Le reçu `ADDENDUM_PLAFOND_SUR_LE_PIC_PROJETE_20260819.md` annonçait un
« pic de résidence projeté »

$$B_{\text{ev}} + \min\left(\max(\text{budget}, \max_K m_K), \; \sum_K m_K\right).$$

La preuve que j'en donnais est correcte — **pour la variable
d'ordonnancement `reserved`**. Le pas invalide est de l'avoir identifiée
à la résidence : `run_folds_budgeted` fait `reserved -= bytes[idx]` dès
que la tâche **retourne**, alors que la tâche a déplacé son résultat dans
`per_k_result[K]`. `nodes`, `deltas`, `batch_levels`, `batch_of_event`,
`facet_keys` et `final_canon_fid` restent **vivants jusqu'au dernier
fold**. Les sorties terminées s'accumulent pendant que leur $m_K$ sort du
compte.

La formule mélangeait donc trois grandeurs — événements résidents,
temporaires des folds actifs, **sorties persistantes des folds
terminés** — et n'en comptait que deux.

**La contre-preuve de l'auditeur, retrouvée à l'identique.** Sur les
comptes finaux du reçu n=8000 et les `sizeof` réels, le seul état final
obligatoire vaut **2 599 581 876** octets quand j'annonçais un « pic » de
**2 597 650 400**. Manque : **1 931 476** octets. La garde acceptait un
calcul dont l'état final *connu* dépassait déjà le plafond annoncé.

**Et la mesure sur objets vivants est bien pire que le minorant.** Les
dix `ForestResult` maintenus vivants à n=8000 pèsent **2 548 288 512**
octets mesurés — 18,6 % de plus que le minorant, qui excluait les
`parents` des deltas et `batch_levels`. L'ancienne formule manque alors
**400 804 864** octets, pas 1,9 Mo.

## 2. Ce qui est corrigé, et ce qui ne l'est pas

**Le stopgap exigé au § 4.3** est en place : la décision porte sur

$$B_{\text{ev}} + \sum_K m_K .$$

Il peut refuser trop tôt. Il ne ment pas.

**Et il ne s'appelle plus un pic.** Le champ est `bytes_bound`, la ligne
publiée est `preflight borne_travail_cumule`, et le refus dit
explicitement « Cette borne est un MAJORANT CONSERVATEUR, pas un pic de
résidence ». C'est la moitié la plus importante de la correction : la
faute d'origine n'était pas l'arithmétique, c'était le **nom**.

**Ce qui reste ouvert**, et que ce reçu ne prétend pas fermer : le modèle
final demandé par l'audit — *événements + sorties terminées + sortie en
construction + temporaires des folds actifs + amont explicitement inclus
ou publié séparément*. La comptabilité actuelle ne sépare pas ces quatre
postes ; elle les majore par une somme.

## 3. Deux autorités indépendantes, parce que la porte précédente était auto-référentielle

Le reproche du § 4.3 est juste : ma porte comparait la décision de la
formule à **la même formule gravée**. Elle ne pouvait donc pas voir une
erreur de modèle, seulement une erreur de recopie.

**Autorité 1 — le minorant de l'état final** (`--output-budget-gate`).
Reconstruit à partir des comptes finaux du reçu n=8000 et des `sizeof`
**réels** de `ForestEvent`, `FacetKey`, `ComponentDelta`, `ForestNode` :
aucune ligne de `project_output_budget` n'y entre.

```text
output_budget_gate violations=0 bytes_evenements=450166752
    bytes_fold_somme=5375578090 bytes_fold_max=1540923248(K=10)
    bytes_borne=5825744842 minorant_etat_final=2599581876
    marge=3226162966 acceptations=1 refus=1
```

**Autorité 2 — les objets vivants** (`--fold-residency-gate`). C'est la
« fixture où plusieurs `ForestResult` terminés restent vivants » exigée
au § 4.3 : les dix folds sont déroulés, **les dix résultats sont
gardés**, et on mesure ce qu'ils pèsent.

```text
n=6000 : persistants_somme=1866883164 persistants_max=553551716
         requis=2196209004 bytes_borne=4303491724 marge=+2107282720
n=8000 : persistants_somme=2548288512 persistants_max=757536456
         requis=2998455264 (ancienne formule 2597650400, marge -400804864)
```

L'accumulation est le fait central, et il est mesuré : la somme des dix
vaut **3,4 fois** le plus gros pris seul. Oublier les terminés n'est pas
une approximation, c'est un changement d'ordre de grandeur.

**Trois mutants tués en code 4** : `budget-events-only` (le flux seul —
la faute du 19 août au matin), `budget-drop-finished` (la formule du
19 août au soir — la faute que l'audit relève), sur les deux portes.

À n=6000 l'ancienne formule **couvre encore** (marge +280 Mo) : la
divergence n'apparaît qu'au-delà d'environ n=7000, quand la somme des
folds dépasse le budget. La porte rapide tourne donc à n=6000 pour le
contrat et le mutant est tué à n=8000, là où la faute existe. Le dire est
plus utile que de choisir la taille qui arrange.

---

## 4. Second défaut, trouvé en préparant la G4 : le budget de fold était calibré sur ce conteneur

En vérifiant que la campagne était lançable, la ligne
`fold_ordonnancement budget_octets=2147483648` a arrêté ma lecture. Ce
chiffre n'est pas neutre : **2 Gio, c'est exactement le huitième des
16 Gio de RAM de ce conteneur.** Écrit en dur, il voyage avec le binaire.

Sur la `g4-standard-48` de la campagne — **48 vCPU, 180 Go de RAM**
(`gcp-migration/deploy.sh:210`) — il vaut **1,1 %** de la mémoire. Or la
borne d'un **seul** fold le dépasse bien avant n=64000. Mesuré au
préflight, pas extrapolé pour les deux premières lignes :

| taille | plus gros $m_K$ | folds forcés à tourner **seuls** |
|---|---:|---|
| n=8000 (mesuré) | 1,44 Gio | aucun |
| n=16000 (mesuré) | **3,02 Gio** | K=9, K=10 |
| n=32000 (extrapolé ×2,10) | 6,8 Gio | K=7…10 |
| **n=64000 — la phase de la campagne** | **14,2 Gio** | **K=5…10** |

Et K=5…10 portent **94 %** des incidences. La garde anti-blocage de
`run_folds_budgeted` — « si ça ne tient pas dans le budget, on l'admet
**seul** » — aurait donc sérialisé l'essentiel du fold, aujourd'hui le
**poste dominant** (32,7 s contre 25,5 s pour la génération à n=8000),
sur une machine louée précisément pour mesurer la montée en fils. **La
campagne aurait mesuré le budget, pas la machine.**

Le facteur de croissance ×2,101 des incidences par doublement de $n$ est
mesuré entre n=8000 et n=16000, pas supposé.

**Deuxième défaut trouvé en chemin** : `--fold-memory-budget`
n'atteignait que le banc d'ordonnancement — le chemin de production
prenait le défaut de la signature. Le drapeau était **mort là où il
comptait**. C'est l'incohérence entre `budget_source=memoire_hote` et
`budget_octets=2147483648` sur la même ligne qui l'a révélé.

### 4.1 La règle

$$\text{budget} = \max\left(2\ \text{Gio}, \; \frac{\text{MemAvailable}}{4}\right)$$

lue **une fois**, publiée avec sa provenance sur la ligne
`fold_ordonnancement` (`budget_source` ∈ {`plancher`, `memoire_hote`,
`explicite`}), donc aucun reçu ne peut être relu sans savoir sous quel
budget il a été obtenu.

Le quart est choisi pour que la **somme** des dix folds tienne quand la
machine le permet : 46,8 Gio à n=64000 contre 45 Gio pour 180 Go / 4 —
la concurrence redevient bornée par les cœurs, pas par une constante.

Le **plancher** garantit qu'aucune petite machine ne régresse, donc
qu'aucun reçu antérieur n'est invalidé.

Les **portes gardent le plancher figé** : une porte dont le verdict
dépendrait de la RAM de la machine d'intégration ne serait pas une porte.

### 4.2 Mesure, intra-processus, quatre modes

Même discipline que les bancs précédents — même processus, mêmes
événements, signature du résultat vérifiée. n=8000, quatre fils :

```text
contiguous_reference   21 477 ms   reserve=0            signature=ab003a59158fc7c8
LPT_unbounded           9 177 ms   reserve=4 319 406 292 signature=ab003a59158fc7c8
plancher_2Gio          15 593 ms   reserve=2 140 153 484 signature=ab003a59158fc7c8
memory_budgeted_LPT     6 959 ms   reserve=3 846 466 156 signature=ab003a59158fc7c8
```

**Signature identique aux quatre modes : l'objet ne bouge pas d'un bit.**
Le pic RSS ne monte pas non plus (4,42 Go au plancher contre 4,14 Go au
budget dérivé — il *baisse*).

Une réserve d'honnêteté : `LPT_unbounded` (9 177 ms) et
`memory_budgeted_LPT` (6 959 ms) sont dans le même voisinage et ne sont
pas départagés par une seule paire ; seul l'écart avec le plancher
(15 593 ms) est net. Ce que la mesure établit, c'est que **le plancher
coûte**, pas le classement des deux autres.

**Mutants tués** : `budget-ignore-host` (la constante seule) et
`budget-no-floor` (le plancher retiré).

---

## 5. Ce que cela change pour la campagne G4

La campagne `n64000` **n'aurait pas produit la mesure qu'elle promet**.
Elle aurait tourné, produit des digests corrects — l'objet ne dépend pas
de l'ordonnancement — et un profil de montée en fils faux sur son poste
dominant.

C'est la raison d'être des reçus : ce défaut ne se voit ni sur un digest,
ni sur une porte, ni sur un test. Il se voit sur **une ligne publiée**
qu'on relit avant de dépenser.

## 6. État

`ctest --test-dir build/v4` : **153 tests** (147 + six portes :
`output_budget` sain + deux mutants, `fold_residency` sain + un mutant,
`fold_budget` sain + deux mutants).

`check_docs`, `check_passation`, `check_implementation_status`,
`check_scope`, `check_gcp_workflows`, `check_references` : verts.

## 7. Reproduction

```bash
cmake -S morsehgp3D_v4 -B build/v4 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v4 -j
ctest --test-dir build/v4 --output-on-failure                     # 153 tests

./build/v4/mhgp4_forest_probe --output-budget-gate                            # 0
./build/v4/mhgp4_forest_probe --output-budget-gate --inject=budget-drop-finished  # 4
./build/v4/mhgp4_forest_probe --fold-residency-gate --family=uniform --n=6000 \
    --s=8 --smax=11 --seed=3 --threads=4                                      # 0
./build/v4/mhgp4_forest_probe --fold-residency-gate --family=uniform --n=8000 \
    --s=8 --smax=11 --seed=3 --threads=4 --inject=budget-drop-finished        # 4
./build/v4/mhgp4_forest_probe --fold-budget-gate                              # 0
./build/v4/mhgp4_forest_probe --fold-budget-gate --inject=budget-ignore-host  # 4
./build/v4/mhgp4_forest_probe --fold-schedule-bench --family=uniform --n=8000 \
    --s=8 --smax=11 --seed=3 --threads=4
./build/v4/mhgp4_forest_probe --family=uniform --n=16000 --s=8 --smax=11 \
    --seed=3 --threads=4 --output-preflight-only
```
