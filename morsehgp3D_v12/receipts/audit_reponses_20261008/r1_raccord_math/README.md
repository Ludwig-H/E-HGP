# R1 livré : raccord du critère de classe unique

Relecture statique au commit `47feedc96ee4b4cb85b5a7c590f38c874bb04855`.
**Avis mathématique favorable dans le domaine des sorties valides de T/M.**
Aucun natif, compilateur, GPU, donnée de scène ou nouveau chrono utilisé.
`capture.json` épingle 14 fichiers Git. Le seul fichier `src/` modifié par
ce commit est `tower/registry_branches.cpp`. Les campagnes annoncées par le
développeur ne sont pas qualifiées par ce reçu.

La [preuve publiée](../registre_classe_unique/README.md) et le
[patch antérieur](../registre_classe_unique_patch/README.md) restent les
références. Le fichier livré n'est pas l'ancien patch octet pour octet : R
a depuis été découpé en étapes partagées avec la Session recouverte. Ce reçu
contrôle leur raccord actuel, sans rejouer le modèle historique.

## Identité des branches

`plan_row` parcourt le bloc contigu de `d>0` événements de la cellule retenue
`t`, lit `z=cell_node[t]` et l'arité `q` de `children[z]`. Dans T, ces `d`
unions successives forment une chaîne du même rang. Dans M, une classe de
`m` unions binaires possède exactement `m+1` enfants externes distincts.
Ainsi `q=d+1` équivaut à `m=d` : aucune autre cellule n'a contribué à cette
classe. Ses enfants sont exactement les composantes ouvertes touchées par
les représentants de `t`. `contract_children` les trie déjà par identifiant
canonique ; leur copie a donc le même ordre que le tri/unique général.

Le corps livré conserve les points nécessaires de cette preuve :

- Les blocs et toutes les lignes retenues sont conservés ; aucune déduplication
  entre lignes. Un plateau partagé par plusieurs cellules garde la voie générale.
- La longueur de travail nulle désigne seulement une ligne directe. Le contrôle
  `n>=2` interdit de la confondre avec une ligne générale vide ; `q>=2` garantit
  une source non vide pour la copie.
- `collect_row` teste cette longueur avant de former un pointeur dans
  `branch_nodes`. Le tampon n'est alloué que pour `Q_g>0`.
- `branches` inclut les deux voies et reste le nombre de valeurs publiées.
  `branch_reads` compte seulement les représentants physiquement relus, soit
  `Q_g`. Sa baisse est voulue ; elle ne constitue pas une identité du travail.
- Les cibles cellule inertes, les doublons et les naissances datées restent
  couverts par la preuve initiale. Avec une seule naissance, aucun événement
  ni ligne retenue. La sortie `A` reste non bornée par `B−1`.

Les vérifications locales de `plan_row` ne valident pas une forêt arbitraire :
CSR, dates, contraction et blocs proviennent des étapes T/M validées.
Ce raccord ne clôt ni n'étend le validateur public CST-0240.

## Durées de vie et mémoire

Dans la Session, `kRows` attend **M terminé et l'historique T terminé**.
`release_events` libère seulement `OrderWork::events` ; `OrderForest::event_cell`,
`cell_node` et `children` restent possédés. Ils suffisent à `prepare_rows`.
`collect → place → fill` conserve ses barrières ; les lignes de sortie sont
disjointes. `close_rows` ne rend les tampons qu'après la dernière tâche de
remplissage, ou le nettoyage global après jonction en cas de refus.

R n'attend pas V. Ce recouvrement est compatible avec les accès lus : V écrit
`lower` et ses compteurs propres, tandis que R lit les structures immuables
M/historique et écrit les tableaux du registre. Aucun alias avec `children`
n'est introduit. Les compteurs temporaires restent privés au couple ordre/fil.

Pour `R` lignes, le chemin **séquentiel `run_registry`** admet d'abord
`12R+8(R+1)+4Q_g+4R` octets, plus tâches/compteurs, puis exactement
`4A+8(R+1)` après comptage. Le plan de ligne partagé est employé pour cette
admission et pour les offsets effectifs.

**La Session recouverte garde une admission conservatrice inchangée** :
`region_bytes → registry_bytes_bound` utilise au plus `B−1` lignes et tous
les représentants d'entrée, pour le temporaire et la borne de sortie.
L'allocation effective bénéficie bien de `Q_g`, mais le seuil préalable
d'admission ne se resserre pas automatiquement. Aucune baisse mesurée du
pic, de la mémoire résidente ou du cache inactif n'est déduite ici.

## Portes présentes et effet possible sur le mur

`registry_unit.cpp` décide les lignes générales en comptant les cellules
contributrices par classe dans une forêt de référence, **sans utiliser
`q=d+1`**. Les branches sont comparées à la coupe ouverte indépendante.
Le code contient neuf fixtures, 147 cas bornés, 200 ordres aléatoires à
W1/W3/W8, des morceaux multiples et une porte des allocations/admissions.
Six mutants R1 sont ajoutés au manifeste. Présence et contenu relus,
**pas de trace d'exécution contre-qualifiée dans ce reçu**.

Le graphe ne contient ni arête R→V, ni V→R. R commence après M et son
historique ; V attend aussi M de l'ordre inférieur et, pour ses fusions,
l'historique inférieur. Une fin R anticipée ne supprime donc aucune attente
logique de V ou G. Elle peut réduire leur contention sur les fils/mémoire,
mais cela demande une mesure.

Diagnostic purement conditionnel : si tous les autres travaux, leurs dates
et le nettoyage restent fixes, poser `O` leur dernière fin et `r,r'` les
dernières fins R avant/après. La baisse de fin de région serait
`max(O,r)−max(O,r')`, donc au plus `r−r'` lorsque `r'≤r`, et nulle si R
finissait déjà avant `O`. Ce n'est ni une prédiction pour le planificateur
à W fils, ni une identification du chemin critique par le dernier marqueur.
La publication garde au moins `Ω(A)` copies. Les temps R physiques ne
s'additionnent pas au mur d'une région recouverte.

Vérification légère des épingles, du delta produit et du manifeste du reçu :

```sh
python check.py /workspaces/E-HGP
python -O check.py /workspaces/E-HGP
```

Ces commandes ne lancent aucun test produit. L'argument Git suffit ; aucun
état de travail mutable ou archive de données n'est nécessaire.
