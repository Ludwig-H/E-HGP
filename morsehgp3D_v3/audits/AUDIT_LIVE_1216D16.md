# Audit live stabilisé — commit `1216d16`

Date de l'audit : 9 août 2026 UTC.

Phase annoncée : M2.2, constructeur `edge_shallow` et juge différentiel. Backend : CPU exact. Profil : Release puis ASan/UBSan et MSan ciblé. Mode : audit strictement en lecture seule ; constructions et probes uniquement sous `/tmp`.

## 0. Ancrage reproductible et verdict

Le périmètre principal de ce rapport est le commit complet :

`1216d16c9b7b3778e284f827424db7e395fae604`

| Fichier commité | SHA-256 du blob reconstitué par `git show 1216d16:<fichier>` |
|---|---|
| `morsehgp3D_v3/CMakeLists.txt` | `fc92fc5dc32ce562572999c98c3d1a904a368e1fa797136e3f13b62791048a3a` |
| `morsehgp3D_v3/oracle/oracle_main.cpp` | `3672b207882ad4216cc2c5ab642f4a7ad9344ed99c049b625bbfe3b98adfbc74` |
| `morsehgp3D_v3/prototype/edge_shallow.hpp` | `7d5b501595c323527d88564b58898e79a667299666667a81524a0a4eb2d72c4e` |
| `morsehgp3D_v3/README.md` | `22a82d15bd7b2ceaa6f9eb195d7dae5cb9a1f3f05c2d26cbfc5d1008d473f2f1` |
| `.github/workflows/ci.yml` | `331147141ff402cb44f3cdf70d19edba30146664ff01267fc643ec7c8ebf7b31` |

**Verdict : non qualifiable.** Les 23 tests CTest du commit passent en Release, mais ils coexistent avec trois échecs bloquants reproduits : lecture de mémoire non initialisée confirmée par MSan dans le tri du catalogue, désaccord de domaine `RelevantGP` sur la petite grille, et reçus pouvant annoncer `qualified:true` ou `exit_code:0` avant un échec réel du processus. Les propositions Q0 et Q1 du README ne constituent pas encore les certificats annoncés.

| Priorité | Finding | État au commit `1216d16` |
|---|---|---|
| P0 | Queue de `support` non initialisée, lue par le tri | Confirmé dynamiquement par MSan, code 86 |
| P0 | Désaccord de domaine `edge_shallow` / référence | Confirmé, campagne petite grille : 10 échecs sur 100 |
| P0 | Reçus sérialisés avant les derniers verdicts | Confirmé : processus 1, reçu `qualified:true`, `exit_code:0` |
| P0 | Q0 a un cas d'égalité non couvert si la boule est fermée ou testée par `<=` | Preuve incomplète ; marge stricte requise |
| P1 | Le contre-exemple Q1 du README utilise deux paires parallèles | Confirmé algébriquement ; aucun sommet fini |
| P1 | Croisements concurrents traités séquentiellement | Garde et fixture de permutation absentes |
| P1 | Compteurs de clipping et portes de couverture ne mesurent pas le coût revendiqué | Confirmé statiquement et par `measure-only` |
| P1 | La CI racine ne construit ni ne teste la v3 | Confirmé dans le workflow commité |

Les sections 1 à 10 portent exclusivement sur les blobs commités ci-dessus. La section 11 isole les modifications de travail postérieures et ne leur accorde aucun crédit de qualification.

## 1. Q1 : le « contre-exemple minimal » commité ne possède pas les sommets invoqués

Le README affirme aux lignes 302–315 que les quatre points duaux `(1,0,1)`, `(0,1,1)`, `(-1,0,1)`, `(0,-1,1)` donnent deux « diagonales » `{0,2}` et `{1,3}` de profondeur 1, « vérifié exactement ».

Avec la convention écrite juste au-dessus, un point dual `(a,b,c)` représente la droite $a s_1+b s_2=c$. Les deux déterminants sont nuls :

$$D_{02}=1\cdot 0-(-1)\cdot 0=0,\qquad D_{13}=0\cdot(-1)-0\cdot 1=0.$$

Concrètement, la première paire est `s1=1` et `s1=-1`, la seconde `s2=1` et `s2=-1`. Ce sont deux paires de droites parallèles distinctes. Elles ne définissent **aucun sommet fini** de l'arrangement ; une profondeur de sommet ne peut donc pas leur être attribuée. Le texte « vérifié exactement » est factuellement faux au commit audité.

Conséquences :

- Q1 n'est pas répondue par les lignes 302–315 ;
- aucune fixture CTest commise ne protège cette affirmation ;
- la conclusion générale « le peeling convexe n'est pas exact » peut être vraie, mais elle doit être soutenue par un exemple réalisable dans la géométrie produit, bien centré, dans `RelevantGP`, et vérifié par les deux catalogues ;
- un contre-exemple d'arrangement abstrait ne suffit pas encore à réfuter le peeling des **événements de Morse admissibles**.

Une tentative antérieure à six points, visible dans les audits de travail, ne réparait pas cette preuve. Pour le support ordonné `(p,q,x1,x3)` de cette tentative, le centre `(9.5,9.5,15)` a les coordonnées barycentriques exactes $(\frac{1}{10},-\frac{1}{10},\frac{1}{2},\frac{1}{2})$ : la coordonnée de `q` est négative, donc le tétraèdre n'est pas bien centré et le support est absent du catalogue de référence. Cet exemple réfute au mieux « couche convexe = complexe shallow brut », pas le chemin spécialisé des sphères critiques bien centrées.

Porte minimale demandée : fixture littérale permanente, contrôle explicite des déterminants non nuls, `well_centred=true`, aucun `extra_on_shell`, présence du support dans l'oracle exhaustif, puis absence démontrée dans le peeling testé.

> [!NOTE]
> **Différentiel postérieur à l'ancre.** Le commit `389a7428c88d9dede7a9c767634774b9ea842ca0` contient désormais, dans `audits/REPONSE_README_PREFIXE_SHALLOW.md` §2.4, une vraie fixture à coefficient `c` commun : `p=(10,10,1)`, `q=(10,10,9)`, `z=(13,13,5)`, `w=(13,7,5)`, `u=(14,9,6)`, `v=(11,6,6)`. Les formes `(48,-48,8)`, `(-48,-48,8)`, `(-16,-64,8)`, `(-64,-16,8)` donnent pour la diagonale `(z,w)` le sommet exact `s=(0,-1/6)`, les résidus `(0,0,8/3,-16/3)`, le centre traduit de `(1/3,0,0)`, le rayon carré `145/9` et les barycentriques strictement positives `(4/9,4/9,1/18,1/18)`. Ces identités ont été revérifiées algébriquement : **Q1 est donc bien fermée négativement sur le plan mathématique dans ce document d'audit postérieur**. En revanche, ni le carré parallèle encore présent dans le README de `389a742`, ni la fixture CTest nommée `convex_layer_refutation` de ce commit n'encodent ce témoin décisif ; voir §11.

## 2. Q0 : l'égalité tangentielle n'est pas exclue par le certificat tel qu'il est rédigé

La proposition des lignes 275–294 utilise $V^{(M)}\subseteq B(p,\frac{d_{M+1}}{2})$. Sa preuve montre seulement qu'un plan restant ne coupe pas la **boule ouverte**. Aux lignes 285–289, la contradiction annoncée oppose :

$$\left\Vert c-p\right\Vert\geq\frac{d_{M+1}}{2}\qquad\text{et}\qquad c\in V^{(M)}\subseteq B\left(p,\frac{d_{M+1}}{2}\right).$$

Cette conclusion n'est valide que si `B` désigne explicitement la boule ouverte et si le test réalisable certifie une marge strictement positive. Si `B` est fermé, ou si l'implémentation teste une inclusion par `<=`, les deux inégalités sont compatibles à l'égalité. Un bissecteur exclu peut être tangent à la frontière et y ajouter une strate fermée ou un support.

Le cas limite est immédiat : choisir un point frontière `c` et poser $u=2c-p$. Alors le bissecteur de `p` et `u` passe exactement par `c`, à distance $\frac{1}{2}\left\Vert u-p\right\Vert=\left\Vert c-p\right\Vert$. « Ne coupe pas l'intérieur » n'implique pas « ne change pas l'ensemble fermé ».

La version sûre doit demander explicitement :

$$\sup_{c\in V^{(M)}}\left\Vert c-p\right\Vert<\frac{d_{M+1}}{2},$$

ou bien traiter la bande d'égalité par insertion et rejeu exact de tous les bissecteurs tangents. Tant que le statut ouvert/fermé et la marge calculable ne sont pas fixés, Q0 ne justifie aucune règle d'arrêt locale exacte.

Porte minimale demandée : deux fixtures permanentes, une avec marge stricte qui doit certifier et une tangentielle $u=2c-p$ qui doit refuser de certifier ou rejouer l'égalité.

## 3. P0 mémoire et déterminisme : queue de support non initialisée

Le type v2 contient `i32 support[4]` sans initialiseur. Dans le chemin singleton, le commit remplit correctement les trois cases inutilisées avec `-1` (`edge_shallow.hpp`, lignes 471–474). Mais le chemin générique construit `mhgp::CriticalSphere critical;`, ne copie que `item.support.size()` cases (`lignes 500–504`), puis le comparateur lit systématiquement les quatre cases (`lignes 515–519`). Les queues des supports d'arité 2 et 3 sont donc indéterminées.

Ce n'est pas une alerte théorique. Le probe a été recompilé depuis un `git archive 1216d16` dans `/tmp`, avec Clang 18 et MemorySanitizer :

```sh
clang++ -std=c++20 -O1 -g -fsanitize=memory -fno-omit-frame-pointer \
  -I<snapshot> -I<snapshot>/morsehgp3D_v2/include -I<snapshot>/morsehgp3D_v3 \
  uninit_tail_probe.cpp catalogue.cpp forest.cpp run.cpp -o uninit_tail_probe_msan
MSAN_OPTIONS=halt_on_error=1:exit_code=86:symbolize=0 ./uninit_tail_probe_msan
```

Résultat : code 86, `MemorySanitizer: use-of-uninitialized-value`. La symbolisation locale place la première lecture dans `std::__final_insertion_sort`, via le comparateur de `edge_shallow_catalogue`, à `edge_shallow.hpp:518`.

Un probe non instrumenté sur la petite grille avait aussi rendu des queues impossibles, par exemple :

```text
n_support=2 support=[0,3,4,-1]
n_support=2 support=[1,3,6,-1]
n_support=2 support=[2,3,5,-1]
n_support=2 support=[3,6,5,-1]
```

Une sphère `{0,3}` peut ainsi être triée comme si `4` participait à sa clé. Le catalogue change d'ordre, donc les indices de sphères et le champ public `ForestNode::source` peuvent changer. Cela explique au moins plausiblement l'échec structurel « source contributrice non minimale » observé au trial 98 ; cette causalité doit être figée par une fixture avant d'être déclarée certaine.

ASan/UBSan et Valgrind peuvent rester verts lorsque les octets physiques de pile ont déjà reçu une valeur ; ils ne blanchissent pas une lecture d'objet non initialisé. Ici MSan est la preuve décisive.

Porte minimale demandée : initialisation de **toutes** les cases à `-1`, comparateur limité par une clé canonique définie, test de sentinelle pour chaque arité 1–4, test de permutation, MSan en CI ou job périodique.

## 4. P0 domaine : l'oracle de référence censure à tort un cas hors périmètre utile

Commande exécutée avec le binaire Release figé du commit :

```sh
mhgp3v_oracle --subject edge_shallow --clouds 5 --seed 314159 \
  --min-points 4 --max-points 7 --max-order 3 --coord-max 10 \
  --min-decided 1 --min-nodes 1
```

Résultat : code 1 et `DOMAINE desaccord : sujet=dans reference=hors trial=4`. La fermeture termine avec `attempted=5 decided=1 rejected_domain=3`.

Le nuage littéral du trial 4 est :

```text
(4,4,2), (1,5,0), (8,8,4), (5,5,2), (1,10,7), (2,3,6), (7,4,4)
```

Ici `n=7`, ordre 2, donc `s_max=3`. Un probe indépendant trouve dans la référence `kept=30`, `degenerate_shells=1`, avec support `{4,6}` et point supplémentaire de coquille `{5}` ; le sujet rend `kept=30`, `degenerate_shells=0`.

La dissymétrie est visible dans le code :

- la référence compte `extra_on_shell` pour toute sphère bien centrée avant le filtre `members.size() > s_max` (`oracle_main.cpp`, lignes 211–227) ;
- le sujet ne construit et ne teste la coquille d'arité 2 que si `rank <= s_max` (`edge_shallow.hpp`, lignes 242–276), et saute pareillement les candidats d'arités 3 et 4 dont le budget est déjà dépassé (`lignes 285–306` et `352–405`).

La définition normative tranche déjà ce désaccord. `docs/SPECIFICATION_MORSEHGP3D.md`, lignes 971–979, définit `RelevantGP` seulement sur les ensembles utiles $A$ tels que $2\leq\lvert A\rvert\leq s_{\max}$ et donne la profondeur de census suffisante $m_{\star}=s_{\max}-2$. Pour le support `{4,6}` du trial 4, les points `2` et `3` sont strictement intérieurs et le point `5` est sur la coquille. Avec `s_max=3`, l'ensemble Gabriel à considérer aurait déjà $\lvert A\rvert=2+2=4>s_{\max}$, ou de façon équivalente deux intérieurs alors que $m_{\star}=1$. L'antécédent de `RelevantGP` n'est donc pas satisfait.

**Résolution normative : le sujet `edge_shallow` est cohérent sur ce cas ; c'est l'oracle de référence qui produit une fausse censure.** Le census inconditionnel des lignes 211–225 est trop large. Il doit filtrer selon le périmètre utile sans pour autant manquer le cas distinct exigé par la spécification : un candidat Gabriel propre vérifiant $\lvert U\rvert+m\leq s_{\max}$, donc nécessairement $m\leq m_{\star}$, doit rougir si un point extérieur supplémentaire est sur son shell, même lorsque l'ajout de ce point porte le rang fermé au-dessus de `s_max`.

La campagne étendue confirme que ce n'est pas un cas isolé :

```sh
mhgp3v_oracle --subject edge_shallow --clouds 100 --seed 314159 \
  --min-points 4 --max-points 7 --max-order 3 --coord-max 10 \
  --min-decided 1 --min-nodes 1
```

Résultat : code 1, 10 échecs. Désaccords de domaine aux trials `4,40,42,52,81,84,92,99`, désaccord structurel au trial 98, puis fermeture non satisfaite. Le trial 4 est une fausse censure de référence démontrée ; les sept autres désaccords doivent être classés avec le même calcul d'intérieurs avant de leur attribuer la même cause. Résumé exact :

```text
attempted=100 decided=44 rejected_domain=48 failures=10
edges=1407 retained=18 active=2725 constant_inside=262
vertices=417 shallow=151 arity2/3/4=1036/312/22 depth_tests=9296
spheres=698 forests=94 nodes=815 positive_depth=381 positive_constant=16
```

Cette campagne appartient au contrat CLI accepté mais n'est pas présente dans CTest. Le trial 4 doit devenir une fixture littérale dont l'attendu est `reference=in-domain, subject=in-domain`; le trial 98 doit devenir une fixture distincte d'ordre/source. Il faut aussi une fixture positive à exactement $m_{\star}$ intérieurs plus un shell extérieur, qui doit rendre `unsupported_degeneracy`, afin que la correction du faux positif ne crée pas un faux négatif.

## 5. P0 reçus : plusieurs faux verts et un schéma JSON ambigu

### 5.1 Le reçu précède encore des verdicts bloquants

Le commentaire des lignes 1525–1526 affirme que le verdict est calculé avant sérialisation. En réalité, seul un verdict partiel est calculé aux lignes 1527–1547 et le reçu est écrit aux lignes 1549–1605. Les contrôles suivants arrivent après :

- propriété `--require-incomplete-anchors`, lignes 1610–1627 ;
- plancher `--min-positive-depth`, lignes 1644–1657 ;
- validation finale de l'injection, lignes 1666–1680.

Reproduction exacte du cas `edge_shallow` :

```sh
mhgp3v_oracle --subject edge_shallow --clouds 1 --seed 4242 \
  --min-points 8 --max-points 8 --min-order 3 --max-order 3 \
  --min-decided 1 --min-nodes 1 --min-positive-depth 19 \
  --receipt receipt.json
```

Le programme observe 18 émissions de profondeur positive et rend **1**. Pourtant le reçu déjà écrit contient :

```json
{"subject":"mhgp3v anchored_catalogue","status":"qualified","exit_code":0,"baseline_passed":true,"probe_passed":true,"qualified":true,"failures":0}
```

Reproduction indépendante du property-test : `--require-incomplete-anchors 13` observe 12 ancres incomplètes et le processus rend 1, mais le reçu annonce `status:"diagnostic_only"`, `exit_code:0`, `baseline_passed:true` et `probe_passed:true`.

Un reçu de qualification doit être construit **après** toutes les portes ou être réécrit atomiquement à la sortie. Le code de processus et le champ `exit_code` doivent être identiques par construction.

### 5.2 Une injection hostile générique est sérialisée comme qualification

À la ligne 1545, `diagnostic_only` vaut seulement `require_incomplete_anchors >= 0 || !fixture.empty()`. Il omet `!injection.empty()` et ne déclasse pas non plus le sujet expérimental `edge_shallow`.

La campagne suivante rend 0, comme attendu pour un test de garde, mais son reçu prétend qualifier le sujet :

```sh
mhgp3v_oracle --inject member_unsorted --clouds 4 --seed 4242 \
  --min-points 8 --max-points 10 --max-order 2 \
  --min-decided 1 --min-nodes 1 --receipt receipt.json
```

Valeurs lues : `status:"qualified"`, `qualified:true`, `injection:"member_unsorted"`, `injections_applied:1`, `injection_escapes:0`. Un probe hostile peut attester qu'un garde rougit ; il ne doit jamais produire un reçu de qualification positive.

### 5.3 Sujet faux et clés dupliquées

Le ternaire de la ligne 1581 étiquette tout sujet autre que `v2` comme `mhgp3v anchored_catalogue`. Un run réellement lancé avec `--subject edge_shallow` est donc sérialisé sous le mauvais sujet.

Les clés `injection`, `injections_applied` et `injection_escapes` sont chacune émises deux fois, aux lignes 1562–1563 puis 1574–1575. Le JSON est syntaxiquement lisible par certains parseurs, mais les clés dupliquées n'ont pas de sémantique interopérable sûre : premier gagnant, dernier gagnant ou rejet selon le consommateur.

Porte minimale demandée : un seul objet de verdict final, un writer JSON structuré, unicité des clés testée, `subject` exact, `diagnostic_only=true` pour fixtures, injections et sujets prototypes, écriture temporaire puis renommage atomique, et property-tests qui comparent systématiquement code de processus et reçu.

## 6. Balayage : les égalités de position ne sont pas traitées par lots

Le commit trie les croisements avec `compare_positions` (`edge_shallow.hpp`, lignes 124–130 et 389–393), puis applique chaque franchissement séparément aux lignes 396–405. Si trois droites ou plus sont concurrentes, plusieurs croisements ont la même position exacte. À ce sommet, la profondeur stricte doit exclure **toutes** les formes nulles simultanément, puis le balayage doit mettre à jour le groupe atomiquement.

Le traitement séquentiel crée des profondeurs intermédiaires qui dépendent de l'ordre équivalent choisi par `std::sort`. Un probe concurrent historique donne, pour trois droites actives rencontrées au même point, seulement `shallow=2` sur `vertices=3` avec `s_max=4`, alors que la profondeur stricte commune est zéro. Ce probe est hors domaine à cause d'une coquille supplémentaire ; il démontre le défaut mécanique du balayage, pas encore une contradiction dans `RelevantGP`.

Le compteur `degenerate_shells` ne constitue pas une preuve suffisante : un sommet concurrent peut être sauté par le budget avant la construction de la sphère et avant le census de coquille, et aucune fixture ne démontre que toute concurrence influente est nécessairement censurée par la définition retenue de `RelevantGP`.

Porte minimale demandée : grouper les positions exactes égales, calculer la profondeur stricte une fois par groupe, appliquer toutes les variations ensemble, puis tester toutes les permutations d'identifiants d'une fixture concurrente admissible ou prouver formellement que ce cas est hors domaine avant le balayage.

## 7. Clipping, compteurs et portes de couverture

### 7.1 Les nouveaux compteurs ne sont pas agrégés au commit

`EdgeShallowStatistics` déclare `lines_total`, `lines_outside_lens`, `lines_constant_outside` et `lines_constant_inside_clip` aux lignes 73–77. L'agrégation du juge aux lignes 1378–1392 ne somme aucun de ces quatre champs. Ni le résumé de console aux lignes 1510–1517 ni le reçu commité ne les publient. Ils restent donc invisibles dans une campagne de qualification.

### 7.2 Les noms ne correspondent pas tous au travail mesuré

- `classify` est appelé pour les arités 3 et 4 (`lignes 214–231`, puis 284 et 349). `lines_constant_outside` et `lines_constant_inside_clip` additionnent donc deux classifications par point et par ancre ; ils ne comptent pas des lignes uniques comme leur nom le suggère.
- `vertices_examined` n'est incrémenté qu'après le filtre `j < i || !active_lens[j]` (`lignes 396–404`). Le tri de chaque porteur inclut néanmoins toutes les formes actives témoins, y compris celles hors lentille. Le compteur ne mesure donc pas le volume réellement trié et balayé.
- `depth_tests += active.size()` à la ligne 394 n'instrumente ni les comparaisons 256 bits de `std::sort`, ni la construction des croisements, ni les validations `sphere_side`. L'expression « tests de profondeur » ne permet pas d'établir le coût exact annoncé.
- `edges_retained` est documenté « arête diamétrale d'au moins un support » à la ligne 52, mais `retained` n'est mis à vrai que pour une émission d'arité 4 (`ligne 442`). Les émissions d'arités 2 ou 3 ne retiennent pas l'arête dans ce compteur.
- `rank_histogram[16]` ignore silencieusement tous les rangs supérieurs ou égaux à 16 (`lignes 272,342,441`), alors que le CLI autorise des ordres plus élevés.
- `anchored_campaign` est explicitement ignoré à la ligne 457, malgré le commentaire « catalogue hybride » des lignes 448–450.

### 7.3 Les portes peuvent être vertes sans exercer les branches décisives

CTest impose seulement `--min-positive-depth 500` sur la somme de toutes les arités (`CMakeLists.txt`, lignes 94–97). Une régression ramenant à zéro la profondeur positive d'arité 4 peut rester verte grâce aux arités 2 et 3. `emitted_positive_constant` est affiché mais n'a aucun plancher ; zéro est accepté. Aucune porte ne demande des minima par arité, des constantes intérieures, des lignes hors lentille, des lignes clippées, des égalités de tri ou des rangs proches de la limite.

### 7.4 `measure-only` accepte une sortie numérique invalide

Le calcul normalise par `edges_examined * lines_per_edge^2` aux lignes 1237–1249 sans traiter `lines_per_edge=0`. Reproduction :

```sh
mhgp3v_oracle --subject edge_shallow --measure-only --points 2 \
  --clouds 1 --max-order 1 --seed 4242
```

Le processus rend 0 et imprime `-nan par m_e^2`. Par ailleurs, `kMaximumDiagnosticPoints=1'000'000` et le contrôle des lignes 1148–1150/1210–1214 acceptent des tailles extrêmes, alors que le chemin `edge_shallow` commence par toutes les paires et n'a ni budget de travail ni échéance.

Porte minimale demandée : schéma de compteurs défini, agrégation centralisée, invariants algébriques entre compteurs, planchers par arité et par branche, compte des comparaisons exactes de tri et des appels aval, refus fail-fast des divisions nulles et budget explicite avant toute énumération quadratique.

## 8. Résultats Release et sanitizers au commit

Tous les builds ont été placés sous `/tmp`.

### 8.1 Release

Configuration : GCC 13.3, C++20, `CMAKE_BUILD_TYPE=Release`. CMake détecte GMP pour le second témoin arithmétique. Les seuls avertissements de compilation observés sont les avertissements pédantiques attendus sur `__int128`.

`ctest -N` recense exactement **23 tests** : 19 tests v3 et 4 tests hérités de la sous-arborescence v2 ajoutée sans condition. La campagne complète `ctest --output-on-failure -j2` a rendu **23/23 PASS** en 138,05 s ; le test `mhgp3v_edge_shallow_depth_dictionary` a pris 77,26 s.

Résumé du test `edge_shallow` de haut ordre :

```text
edges=664 retained=129 active=2796 constant_inside=264
vertices=3285 shallow=1779 arity2/3/4=621/767/242 depth_tests=24743
dictionary_refuted=0 spheres=1726 forests=69 nodes=2458
rank_histogram=250/298/333/330/283/136
positive_depth=1105 positive_constant=322
```

Une campagne complémentaire de 100 nuages sur la grille complète, `max-order=3`, a également passé le différentiel : `edges=4447`, `retained=64`, `active=16687`, `constant_inside=1770`, `vertices=3590`, `shallow=614`, `arity2/3/4=2694/1203/75`, `depth_tests=62411`, `attempted=100`, `decided=100`.

Ces verts ne couvrent ni la petite grille qui rougit à la section 4, ni la lecture non initialisée détectée par MSan.

### 8.2 ASan/UBSan

Build ciblé avec `-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all`. Les probes sélectionnés suivants n'ont produit aucun diagnostic : selftest arithmétique 2000 itérations avec témoin GMP, campagne v2 de base, sujet `anchored`, sujet `edge_shallow` à 8 points, property-test à échec attendu, injection hostile à succès attendu, et cube extrême sur la grille u16.

Le probe cube rend notamment `spheres=28 edges=28 active=96 vertices=108 degenerate_shells=28 dictionary_refuted=0` sans diagnostic ASan/UBSan. La CTest sanitizer complète de haut ordre n'a pas été achevée : ce rapport ne la présente pas comme verte.

### 8.3 MSan

Le probe ciblé de la section 3, recompilé sur le hash exact `7d5b501…`, **rougit** avec `use-of-uninitialized-value`. Le bilan sanitizer global est donc rouge malgré les probes ASan/UBSan propres.

## 9. CMake, CTest et CI

- `morsehgp3D_v3/CMakeLists.txt:18` ajoute inconditionnellement la v2, puis `enable_testing()` à la ligne 45. Il n'existe pas de commutateur `BUILD_TESTING` propre à la v3.
- Le README indique encore `# 14 tests` à la ligne 448, alors que le graphe réel en contient 23.
- Aucun `TIMEOUT` n'est fixé sur les tests ; CTest conserve son plafond effectif très large. Un blocage algorithmique peut donc immobiliser la CI.
- Le commentaire d'en-tête CMake « uniquement M1 » à la ligne 4 est périmé alors que M2.2 est compilé et testé.
- `.github/workflows/ci.yml`, lignes 28–83, construit `morsehgp3d`, ses contrats, son oracle Python et ses scripts. Aucune commande ne configure, ne construit ou ne teste `morsehgp3D_v3` ou `mhgp3v`.

Ainsi, les 23 tests locaux ne sont pas une porte de dépôt. Un commit peut casser complètement la v3 tout en gardant le workflow racine vert.

## 10. Contrat 50 000 points : conditions quantitatives nécessaires

Le prototype commité commence encore par toutes les arêtes (`edge_shallow.hpp`, lignes 486–488). À `n=50 000`, cela représente exactement :

$$E=\binom{50000}{2}=1\,249\,975\,000\ \text{ancres}.$$

Sans clipping préalable des ancres, le seul débit nécessaire pour les visiter en moins d'une seconde dépasse 1,25 milliard d'ancres par seconde. Si chaque ancre garde tous les autres points comme formes actives, le nombre de sommets candidats est :

$$\binom{50000}{2}\binom{49998}{2}=1\,562\,312\,506\,874\,925\,000.$$

Même à un milliard de candidats par seconde, ce seul étage prendrait environ 49,5 ans, avant tri, test exact, construction de sphère et forêt.

Mesures Release `measure-only`, une graine et un nuage par taille, donc strictement diagnostiques :

| `n` | arêtes | `m_e` moyen | sommets comptés | tests de profondeur | temps |
|---:|---:|---:|---:|---:|---:|
| 40 | 780 | 17,4 | 5 069 | 57 431 | 0,01 s |
| 80 | 3 160 | 35,9 | 21 002 | 360 712 | 0,09 s |
| 160 | 12 720 | 72,3 | 87 953 | 2 464 109 | 0,51 s |

Le clipping réduit fortement les **porteurs comptés**, mais `m_e` reste ici proche de `0,45 n` et les compteurs omettent les comparaisons de tri. À `n=50 000`, conserver toutes les arêtes avec cette densité donnerait environ $2,81\times 10^{13}$ incidences arête–forme avant même les croisements. Le contrat `<1 s` exige donc structurellement un générateur d'ancres sous-quadratique et une sortie sensible à `k`, pas seulement un clipping dans chaque ancre déjà énumérée.

Une porte honnête doit enregistrer séparément `A` ancres candidates, `L` incidences actives, `P` prédicats exacts, `S` comparaisons de tri, `V` événements shallow et `D` opérations aval, puis vérifier sur la cible matérielle :

$$\frac{A}{R_A}+\frac{L}{R_L}+\frac{P}{R_P}+\frac{S}{R_S}+\frac{D}{R_D}<1\ \mathrm{s}.$$

Conditions nécessaires, indépendantes d'une micro-optimisation :

- `A` ne peut pas être `1 249 975 000` ; les ancres doivent être générées implicitement ou certifiées localement en nombre bien plus petit ;
- le tri ne peut pas recevoir toutes les paires de croisements ; il faut borner directement les événements du préfixe, avec une preuve de complétude ;
- `P`, `S` et `D` doivent être effectivement comptés, pas estimés par `depth_tests` ;
- le nombre de sphères critiques et de mises à jour de forêt doit lui-même rester sous le budget aval ;
- les mesures à 40–160 points ne peuvent pas être extrapolées en qualification à 50 000 sans courbe multi-graines, quantiles, mémoire maximale et timeout.

Au commit audité, aucune de ces conditions n'est démontrée. Le prototype reste un falsificateur borné utile, pas une voie produit `<1 s`.

## 11. Différentiel postérieur : commit `389a742`

Pendant la rédaction, Claude a commis les changements live. Cette section est réancrée au nouveau HEAD complet :

`389a7428c88d9dede7a9c767634774b9ea842ca0`

| Fichier commité à `389a742` | SHA-256 du blob |
|---|---|
| `morsehgp3D_v3/CMakeLists.txt` | `384b940d52b883a98f06657389bc7da8ec5474dcdef4519d7c80e3aa733e0874` |
| `morsehgp3D_v3/oracle/oracle_main.cpp` | `ed0fe1c1b86a5d0b4dd1a96a6ab00ccd094f0dbd1f3e5abcff83b27029989dbc` |
| `morsehgp3D_v3/prototype/edge_shallow.hpp` | `43992a786bbed0c6ff1877f39b828ae8442cf77cf7bb9a1df5306c0f861f91b1` |
| `morsehgp3D_v3/README.md` | `6611f773e98f9985f84a8ca2c85c27ac2999656d76d18ab3a49d019acd3634f4` |
| `morsehgp3D_v3/audits/REPONSE_README_PREFIXE_SHALLOW.md` | `d41dcba0f14093b0833e74d37284810f88f58b23a9cb3fd1c301dcb122f66def` |

Le commit corrige Q0 dans le README par une marge explicitement stricte, documente la vraie fixture Q1 bien centrée dans `REPONSE_README_PREFIXE_SHALLOW.md` §2.4, remplace le comparateur rationnel par un chirotope, agrège et sérialise les nouveaux compteurs, supprime les clés JSON dupliquées et donne un libellé propre à `edge_shallow`. Ces corrections sont postérieures aux résultats Release/sanitizer de la section 8 et ne doivent pas leur être attribuées.

### 11.1 La régression CTest Q1 ne teste pas la fixture Q1 correcte

Le CTest ajouté aux lignes 147–153 de `CMakeLists.txt` appelle `--fixture convex_layer_refutation`. Or cette fixture, dans `oracle_main.cpp` lignes 1341–1369, réutilise l'ancien support `(p,q,x1,x3)` :

```text
p=(10,10,10), q=(10,10,20), x1=(6,13,16), x2=(6,7,16),
x3=(13,6,16), x4=(13,14,17)
```

Déplacer seulement `x4.z` de 16 à 17 peut supprimer des cosphéricités impliquant `x4`, mais ne change ni le centre ni les barycentriques du support `(p,q,x1,x3)`. Elles restent $(\frac{1}{10},-\frac{1}{10},\frac{1}{2},\frac{1}{2})$ : le support n'est toujours pas bien centré.

Les commentaires de la fixture sont en outre numériquement faux. Les formes de `x1` et `x3` se coupent en `s=(-1/5,1/5)`, pas `(-61/20,61/20)`. Avec `x4.z=17`, les quatre formes sont `(60,80,4)`, `(-60,80,4)`, `(-80,-60,4)`, `(80,-60,16)` : elles n'ont plus un coefficient `c` commun. Au point faux annoncé, leurs résidus sont `(57,423,57,-443)`, pas `(0,366,0,-488)`.

Le test peut donc passer simplement parce que l'oracle et le sujet omettent tous deux le support non bien centré ; il n'assert pas la réfutation du peeling. La fixture permanente doit reprendre exactement les six points de `REPONSE_README_PREFIXE_SHALLOW.md` §2.4 et vérifier support `{0,1,2,3}`, rang 5, membres `{0,1,2,3,4}`, bon centrage, profondeur 1, `degenerate_shells=0` et diagonale absente de l'onion.

Le README principal de `389a742`, lignes 324–328, conserve par ailleurs le carré dual parallèle réfuté au §1. Le résultat mathématique est réparé dans l'audit §2.4, pas encore dans la réponse Q1 publiée par le README.

### 11.2 Findings techniques encore présents au nouveau HEAD

- la queue de `support` reste non initialisée et le tri lit toujours quatre cases (`edge_shallow.hpp`, lignes 562–581) ;
- les croisements de chirotope égaux restent traités un par un (`lignes 452–466`) ;
- `diagnostic_only` omet toujours `!injection.empty()` et les verdicts tardifs restent après le reçu (`oracle_main.cpp`, lignes 1557–1579 puis 1676 et suivantes) ;
- `EdgeShallowStatistics::absorb` additionne déjà `rank_histogram` (`edge_shallow.hpp`, ligne 104), puis l'oracle le somme une seconde fois (`oracle_main.cpp`, lignes 1422–1424) : l'histogramme de rang est exactement doublé ;
- la fausse censure de référence de la section 4 n'est pas réglée par ce commit.

### 11.3 Probes Release et MSan ciblés sur `389a742`

Le commit a été extrait par `git archive 389a742` puis construit en Release dans `/tmp/mhgp3v-389a742.Kk3tmh`. `ctest -N` compte désormais **25 tests**. Les deux tests ajoutés passent :

```text
mhgp3v_oracle_convex_layer_refutation  PASS  0.01 s
mhgp3v_oracle_constant_inside_witness  PASS  0.00 s
```

Ce vert confirme le faux sentiment de couverture Q1 : `convex_layer_refutation` rend `attempted=1 decided=1 rejected_domain=0`, mais ne vérifie jamais que le support décisif est présent ni qu'il est bien centré. Son résumé donne `arity2/3/4=15/19/6`, soit 40 émissions, tandis que l'histogramme imprimé somme `26+30+14+10=80`. Le double comptage de `rank_histogram` est donc confirmé dynamiquement, pas seulement par lecture. Le test `constant_inside_witness` présente la même signature : 15 émissions et une somme d'histogramme égale à 30.

La petite grille de la section 4 rougit encore au trial 4 avec `DOMAINE desaccord : sujet=dans reference=hors`. Le reçu tardif rougit encore de la même manière : un run avec 18 émissions et `--min-positive-depth 19` rend 1 mais sérialise `status:"qualified"`, `exit_code:0`, `qualified:true`. Le libellé du sujet et les clés dupliquées sont en revanche corrigés. Une injection `member_unsorted` reste sérialisée `qualified:true`, ce qui confirme l'omission de `!injection.empty()` dans `diagnostic_only`.

Enfin, le probe de queue de support a été recompilé avec Clang 18 et `-fsanitize=memory` sur le blob exact `43992a…`. Il rend 86 avec `MemorySanitizer: use-of-uninitialized-value`. Le P0 mémoire de la section 3 est donc toujours actif au nouveau HEAD.

Contre-vérification d'une alerte extrême : sur le hash exact `43992a…`, les lignes 527–532 contiennent une boucle externe sur `p` et **une seule** boucle sur `z`. Il n'y a pas deux boucles `z` imbriquées et le rang singleton n'est pas multiplié par `n` dans ce blob. Ce défaut ne doit pas être publié sans un autre hash qui le contiendrait réellement.

## 12. Ordre de fermeture recommandé

1. Corriger et tester la queue de support ; exiger MSan vert et stabilité du catalogue sous permutation.
2. Aligner l'oracle de référence sur la définition normative utile de `RelevantGP`, puis figer les trials 4 et 98 en fixtures permanentes avec leurs attendus distincts.
3. Refondre le verdict/reçu en un objet final unique, écrit après toutes les portes ; interdire toute qualification d'une fixture, injection ou sujet prototype.
4. Remplacer le contre-exemple Q1 parallèle par une fixture produit réellement bien centrée et admissible ; préciser Q0 avec marge stricte et fixture tangentielle.
5. Traiter les croisements égaux par lots ou prouver leur exclusion avant balayage.
6. Définir les compteurs de coût et les portes par arité ; ajouter petite grille, égalités, clipping constant et sentinelles à CTest.
7. Ajouter une CI v3 Release, ASan/UBSan et MSan ciblé, avec timeouts.
8. Ne promouvoir aucune promesse 50 000 points tant qu'un générateur d'ancres sous-quadratique et un budget complet prédicats/tri/aval ne sont pas démontrés.

## 13. Traçabilité opérationnelle

- Dépôt source : lecture seule, hors création de ce rapport d'audit.
- Builds, snapshots et probes : `/tmp` uniquement.
- Git : aucune mutation, aucune branche, aucun commit.
- GCP : non utilisé.
