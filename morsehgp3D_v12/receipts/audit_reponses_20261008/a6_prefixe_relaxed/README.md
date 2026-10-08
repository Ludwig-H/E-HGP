# A6 / CST-0242 — un fragment de préfixe sous mémoire relâchée

8 octobre 2026, Codex. Complément au [reçu I/N](../a6_indices_concurrence/README.md), sans changement de produit ni d'état du constat. Prototype local `69dd8e9021fb9b4203e72374629aaf6c4c6610dd` ; quatre corps exacts épinglés dans [capture.json](capture.json). Ce reçu apporte un graphe d'événements qui satisfait les relations C++20 **examinées**, et précise pourquoi le pont proposé l'exclut. Aucun échec natif n'a été observé.

## Résultat et frontière

[check.py](check.py) examine un graphe explicite de 47 événements atomiques : noyau, aide, compteurs d'activité, fermeture SC, réservation du noyau et retraits `in_flight`. Sous accès relâchés aux parents et feuilles, aucune des contraintes contrôlées n'est violée. Avec publication release de la feuille et consommation acquire, le graphe est refusé. La variante symétrique release/acquire des parents le refuse aussi ; elle n'est pas nécessaire au patch proposé.

Ce vérificateur maison n'est **ni un outil formel C++ indépendant, ni une preuve d'atteignabilité géométrique de FULL, ni un test de compilation ou d'exécution**. Il traite un fragment conditionnel du noyau. Il ne suffit donc pas à déclarer une panne du produit ou d'une campagne. Il réfute en revanche, dans ce modèle explicite, l'argument selon lequel l'atomicité, l'arbre final monotone et la garde de fermeture imposeraient à eux seuls des indices appartenant au préfixe consommé.

## Le parent futur n'est pas autojustifié

Après deux unions antérieures `(2,3)` puis `(2,4)`, les composantes sont `{0}`, `{1}`, `{2,3,4}`. Les racines ont les tailles 1, 1, 3. Deux cellules de rangs **strictement différents** suivent : `t=(0,1)`, puis `u=(0,2)`. Les cibles sont ici des naissances antérieures, sans lecture concurrente d'`element`.

| Indice lu pour la première cible de t | Union de t | Coupe après t | Union ultérieure de u |
|---|---|---|---|
| 0, feuille d'origine | 1 → 0 | `{0,1}` / `{2,3,4}` | 0 → 2, car 2 < 3 |
| 2, parent futur de 0 | 1 → 2 | `{0}` / `{1,2,3,4}` | 0 → 2, car 1 < 4 |

Dans les deux branches, la seconde union écrit **`up[0]=2`**. Le 2 vient d'une racine déjà existante ; cette écriture ne requiert pas que l'aide ait préalablement fourni le mauvais indice. Le modèle calcule les deux branches et leurs événements, pas seulement une valeur supposée. La partition finale est identique, mais la coupe intermédiaire et les attaches diffèrent. Des rangs distincts empêchent de masquer cette différence par contraction d'un même plateau.

## Cycle de lecture, sans cycle HB dans la variante relâchée

Le cœur du graphe est le suivant (`SB` : ordre du fil, `RF` : écriture lue) :

```text
noyau : L = load(leaf[t,0]) == 2  --SB--> U = store(up[0], 2)
aide  : H = load(up[0]) == 2     --SB--> S = store(leaf[t,0], 2)
RF    : U → H ; S → L
```

L'ordre des modifications de chaque objet reste simple : initialisation puis U pour `up[0]`, initialisation puis S pour la feuille. Ce cycle mélange SB et RF ; il n'est pas un cycle HB lorsque les deux transferts sont relâchés. La lecture ultérieure de `up[0]` par le noyau, avant U, prend bien l'ancienne valeur 0. L'aide peut encore lire l'ancien parent de 1, objet distinct, et conserver la seconde feuille à 1.

La norme impose notamment l'absence de cycle HB, une source de lecture qui ne soit pas HB-future et quatre contraintes de cohérence par objet. Elle n'impose pas un ordre global fusionnant tous les ordres de modification. Le lecteur vérifie ces relations, les valeurs lues et les sources des RMW. [N4861, intro.races §§4, 10, 14–18](https://timsong-cpp.github.io/cppwp/n4861/intro.races).

Le graphe donne explicitement un ordre SC pour la garde : `active++ aide`, `closed=false aide`, `closed=true noyau`, `active-- aide`, `active=0 noyau`. Les RMW `in_flight` lisent chacun leur prédécesseur immédiat ; leur chaîne ne ramène pas la fin de l'aide avant L. Le contrôle SC est volontairement plus fort que nécessaire : il respecte tout HB entre événements SC, ainsi que leur ordre de cohérence. Aucun fence ni consume ne figure dans ce fragment. [N4861, atomics.order §§2–4, 10](https://timsong-cpp.github.io/cppwp/n4861/atomics.order).

## Raccord aux corps épinglés

`forest_kernel.cpp:35–60, 86–108, 230–246` contient les lectures/écritures relâchées, l'union par taille et l'aide. `pipeline_run.cpp:157–173, 303–344` autorise un même propriétaire du noyau à poursuivre les tranches prêtes, puis attend les aides **à la clôture**. `pipeline_steps.cpp:122–139` annonce l'aide avant sa lecture SC de fermeture. Cette protection assure la durée de vie ; elle ne fournit pas une réception acquire de S avant chaque consommation L.

Pour raccorder le fragment, l'aide réclame la tranche 1 alors que `kernel_slice=0`. Les unions préliminaires sont dans le préfixe de la tranche 0 ; t est à la fin de la tranche 1, u dans la suivante. Ainsi l'aide n'a pas à réécrire les feuilles de u. Les publications initiales de l'UF et des feuilles sont acquises ; les tranches sont prêtes ; le même fil garde le noyau. La publication de progression reçue par l'aide peut rester à 0, choix autorisé dans le graphe. Les compteurs et la fermeture n'introduisent alors aucun pont vers L.

**Les cinq feuilles seules ne déclenchent pas le mécanisme de tranches de 256 cellules.** Le remplissage des tranches par un véritable catalogue géométrique n'est pas construit ici. Les accès aux autres feuilles, préchargements et suffixes sans retour de synchronisation vers L sont omis de cette réduction ; ils ne sont pas certifiés par une extraction automatique du C++. Cette frontière demeure nécessaire même si les opérations montrées correspondent aux corps.

## Pourquoi le pont feuille ferme le problème de préfixe

Avec S release et L acquire lisant S, chaque lecture H d'un parent pendant la construction de l'indice vérifie `H SB S SW L SB U_futur`. Donc H est HB-antérieure à tout store futur du même noyau sur ce parent. La cohérence lecture/écriture oblige H à lire une modification strictement antérieure à ce store dans l'ordre de modification : elle ne peut lire U. C'est précisément le refus observé par le modèle ; la synchronisation release/acquire et cette contrainte sont celles des paragraphes cités ci-dessus.

Plus généralement, les écritures du noyau doivent former **une chaîne ordonnée par HB**, y compris lors de ses reprises sur un autre fil : la publication release de son état disponible et sa réservation acquire assurent ce relais, relu dans le reçu I/N. Une simple absence de chevauchement temporel entre écrivains ne suffirait pas. Sous cette précondition, initialisation publiée, indices et tampons vivants, chaque arête ainsi lue appartient au préfixe du noyau lors de L. Unions et compressions ne relient que des nœuds de sa même composante : le chemin de l'aide reste dans cette composante. Une feuille initiale lue sans transfert depuis une aide conserve son identité d'origine. Les cibles cellules restent soumises au contrôle `target < processed` et à la publication acquire de leur `element`. Le pont ne remplace aucune de ces préconditions, ni la garde de fermeture.

Le patch de deux ordres mémoire est déjà proposé dans le reçu I/N ; il n'est pas dupliqué. Aucune conclusion sur son coût machine ou sur les temps FULL n'est tirée ici.

## Rejeu borné

```sh
python check.py /chemin/a6_concurrence_20261008 --check
python -O check.py /chemin/a6_concurrence_20261008 --check
```

Les quatre hashes source sont contrôlés à chaque lecture. [results.json](results.json) conserve les deux coupes et les quatre variantes : relâchée admise par les clauses vérifiées ; pont feuille, pont parent et deux ponts refusés. Aucune campagne, donnée de scène, compilation ou invocation native n'est nécessaire. Le contrôle ne prétend pas formaliser toutes les règles C++, les transformations d'un compilateur, ni l'absence générale de valeurs sans justification ; ici la justification de 2 est calculée séparément dans les deux branches.
