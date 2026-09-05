# Raccord de l'index au front et aux covers

5 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Le raccord combinatoire se ferme par composition de la preuve d'index
et des boucles des consommateurs.** Il n'est pas nécessaire de demander
au constructeur une nouvelle preuve générale de couverture de ces piles.
La vérité géométrique et arithmétique des décisions de mort garde son
autorité propre ; ce texte établit la conservation des populations.

Sources relues : [index](../src/tree/cloud_index.hpp),
[front fusionné](../src/pipeline/generate.hpp),
[WSPD](../src/wspd/wavefront.hpp) et [covers](../src/lanes/edge_cover.hpp).
Les hashes sont ceux de la [validation courante](validation_current.json).
La [preuve d'index](AUDIT_INDEX_20260905.md) fournit les enfants disjoints,
leurs plages exactes, l'atteignabilité unique et les boîtes serrées.

## 1. Partition des tâches du front par lane

Toute paire non ordonnée de positions distinctes possède un unique plus
bas ancêtre commun. Elle appartient donc à une unique graine
`(left(v), right(v))`. Les deux facteurs de cette graine sont disjoints.
Remplacer un facteur interne par ses deux enfants conserve cette
disjonction et partitionne le produit cartésien initial en deux produits.

Pour une lane donnée, maintenir trois ensembles de paires : tâches en
attente, rectangles émis, rectangles éliminés. Ils partitionnent les
graines portant cette lane. `alive_rectangles_fused` retire son bit avant
d'émettre une mort ; les enfants ne reçoivent que le masque restant.
Au terminal, le deuxième comptage répartit encore chaque lane entre
émission et mort, puis le code continue sans produire d'enfant.
Une lane ne peut donc être créditée à la fois au parent et au descendant.

Le découpage d'une vague utilise des intervalles d'indices disjoints
couvrant la vague. Chaque shard possède ses sorties, et la fusion
parcourt chacun une fois. L'ordre des workers ne change pas cette
partition. Une scission diminue la somme des hauteurs des deux facteurs ;
deux feuilles distinctes sont séparées et ne sont jamais scindées.
La terminaison combinatoire suit de l'index fini.

Les caps sont testés avant fusion globale. En cas de refus, le pipeline
consomme `cap_refus` avant de pouvoir déclarer un succès ; en cas
d'exception, le travail partiel ne devient pas un succès terminal.
Le théorème de couverture demeure expressément conditionné à ce succès.
L'égalité du ledger seule ne serait pas cette preuve de partition.

## 2. Handles d'antichaîne et absence de double visite

`rect_cover_handles` initialise sa pile avec la racine. À tout instant,
les sous-arbres en pile et ceux déjà émis sont deux à deux disjoints.
Un nœud est soit éliminé, soit émis avec arrêt de sa descente, soit
remplacé par ses deux enfants. Le seuil de 32 positions décide seulement
où arrêter la descente : il ne tronque pas la plage émise. Les handles
finaux constituent donc une antichaîne de plages disjointes.

Pour un rectangle A×B, sa boîte des sommes contient toute somme a+b,
et `dmax2` majore toute distance carrée entre ses facteurs. La distance
minimale d'une boîte doublée à cette boîte des sommes est un minorant de
la distance de chacun de ses points doublés à toute somme a+b. Le rejet
strict `gap2 > coef*dmax2` ne peut donc perdre un point admissible pour
l'une des ancres. Les égalités restent accessibles.

`anchor_cover_from_handles` parcourt chaque plage entière et teste
chaque position une seule fois. Son inégalité fermée est celle du cover
de l'ancre. La bijection PointId/position au sein des buckets et le profil
sans positions répétées excluent tout double crédit d'une identité.

## 3. Largeurs et permutation du cover dans ce raccord

Posons M=65535. Les différences de coordonnées ont un module au plus M ;
les sommes et coordonnées doublées sont dans [0,2M]. Ainsi `dmax2` et D²
sont au plus 3M², tandis que `gap2` et la distance carrée doublée d'un
site sont au plus 12M². Les coefficients produits sont 3 ou 4, donc toutes
ces valeurs, `bound` et `bound+1` tiennent strictement sous 2^36 en i64.

Pour un site retenu, `0 <= d2 <= bound`. Le quotient entier
`32*d2/(bound+1)` est donc dans 0..31 ; son produit est promu en i128
avant multiplication. Les 33 compteurs u32 cumulent au plus n, avec
n au plus 2^30−1. Les sommes préfixes donnent à chaque bin une plage
disjointe de la taille exacte de sa population. L'écriture incrémentale
dans ces plages est une permutation stable de tous les sites retenus.
La borne d'index protège aussi le dernier incrément de la boucle i32.

Ce raccord ferme donc les parcours et la permutation locale du cover,
sans se limiter à un contrôle de masse. Il se compose directement avec
les inclusions géométriques des covers q3/q4 de [S1](S1_COURANT.md).

## 4. Autorité et suite

Le [front et ses témoins](FRONT_ET_TEMOINS_COURANT.md) portaient déjà
l'argument géométrique conditionnel ; le présent raccord décharge son
hypothèse topologique et explicite les plages réellement consommées.
Les portes existantes `mhgp7_wspd_ownership`, `mhgp7_fused_descent`, leurs
mutants et les portes de cover restent des falsifications compilées
distinctes, suivies dans la [qualification Release](receipts_20260905/release/summary.json).
Aucune nouvelle campagne n'est attribuée à cette seule lecture statique.

La fermeture ne porte pas sur tous les calculs des témoins de fuseau,
secteur et corde, ni sur une borne linéaire industrielle de rectangles.
Le prochain inventaire utile est celui des domaines arithmétiques encore
non raccordés, et non une nouvelle preuve de partition de l'index ou du
front. Aucun catalogue Gamma ni mosaïque supplémentaire n'est construit.
GCP non utilisé.
