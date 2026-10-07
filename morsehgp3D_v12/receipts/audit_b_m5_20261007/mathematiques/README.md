# MES-M5 — géométrie et numérique du parcours

7 octobre 2026, pin `e30000dec1027c5f0ade3093a94563ee412409d3`.
`phase=exploration_v12_hors_registre`, `backend=cpu_reference`,
`objet=full_pi0`, `quantification=quantized_u21_input_only`,
`public_status=not_claimed`. Fixtures u32 synthétiques explicites.
**GCP/GPU non utilisés.** Deux petites unités C++ optimisées, sans sanitizer
ni campagne de mutants, aucun jeu réel, aucune modification produit.

**Bilan positif sous les préconditions validées du microbanc** : le réservoir,
G1, le compactage, les boîtes ajustées et la subdivision reproduisent les
règles du parcours v11 à ordre parent identique. Neuf parcours du vrai binaire
hôte concordent avec un DFS indépendant en entiers Python, dont profondeur
96 et refus transactionnel. Les garanties arithmétiques s29/i64 puis s33/i128
sont cohérentes et les petites portes les exercent.

Les bornes de compte `EmitKernel` et le placement de la garde de capacité sont
traités par le volet racine ; ils empêchent de généraliser ce bilan positif
à toute la capacité théorique u32. Aucun nouveau constat séparé demandé ici.

Les chemins abrégés `bfs`, `driver`, `warp` désignent les en-têtes de
`microbancs/mes_m5_parcours/include/mhgp12/traversal/` dans v12.

## Résultats exécutés

| Porte ciblée | Résultat |
| --- | --- |
| 110 requêtes de repère, clé de réservoir et G1 | mêmes entiers/signes que les extrema exacts aux huit coins ; contact conservé |
| 50 réservoirs, K=1/5/10/11/12, 31 à 513 sites | mêmes 3K premiers (distance, rang) qu'un tri exact indépendant |
| Six sites, Morton absolu puis normalisé, K1/feuille4 | deux parcours conformes ; 233 puis 230 tests G1 ; feuilles géométriquement différentes |
| Cube de huit sites, K1/K2/K5 | identité des nœuds, feuilles et grand livre |
| Coquille48 u21, K5/feuille24 | profondeur 63 ; 951 nœuds, 328 feuilles |
| Coquille48 u32, K5/feuille24 | profondeur 96 ; 1 479 nœuds, 504 feuilles |
| Même coquille u21, feuille8/max8 | statut `wide_leaf`, zéro feuille/site publié et grand livre nul |
| Deux extrémités u32 | fermeture à 2^32, repère s33 ; parcours conforme |
| Oracle géométrique borné, cinq cas de six ou huit sites | 77 boules critiques : propriétaire unique et population I∪U entière dans sa feuille |

Les réservoirs franchissent 32 voies, 36 témoins (K12) et 256 candidats par
tâche ; les clés contiennent des ex æquo et les ordres parents sont mélangés.
Les coordonnées restent distinctes. La sonde appelle les vrais
`SelectKernel`/`MergeKernel`, pas une copie de leurs algorithmes.

Le DFS indépendant emploie les extrema d'une expression affine en coordonnées
globales ; il ne réutilise pas les termes carrés/décalés du code natif. Le
module Python du développeur sert uniquement d'encodeur/FNV des petits
vidages. Le binaire `traversal_identity` compare tous les nœuds, pas seulement
les comptes et une empreinte. L'oracle géométrique `Reference` n'est utilisé
que pour n≤8 ; aucune énumération géométrique sur la coquille48.

Le producteur `traversal_dump` lié au moteur v11 gelé n'a **pas** été réexécuté
dans ce volet. Son lien avec `prepare_node`/`split_ready` et les règles de
`morsehgp3D_v11/src/catalogue/boxes.cpp` ont été relus. Les résultats ci-dessus
qualifient le parcours hôte contre les références indiquées, sans nouveau
différentiel natif v11 ni qualification du GPU.

## Arguments vérifiés dans les sources

**Réservoir** (`bfs:280–518`). La clé est totale par (distance exacte, rang
parent), avec sentinelle strictement après toute clé valide. Un élément
absent des 3K premiers de son morceau a déjà au moins 3K prédécesseurs dans
ce morceau : il ne peut appartenir au réservoir global. Le sommet des sommets
locaux suffit donc. Dans `Merge`, un morceau dont la plus petite clé n'est
pas sous le seuil courant ne peut rien améliorer ; ce seuil ne fait que
décroître. Les morceaux sont disjoints, leurs rangs sont distincts, ce qui
justifie les positions de fusion par rangs et l'absence d'écritures rivales.

**G1 strict et ex æquo** (`bfs:249–273,520–600`). Pour deux sites x,y,
`|x-c|²−|y-c|²` est affine en c. Son minimum sur la fermeture [lo,hi] est
exactement celui du test `dominates`. Un retrait exige K témoins **distincts**
strictement plus proches pour tout c de la boîte. L'égalité et le site
lui-même ne comptent pas. Cette règle conserve les K plus proches avec tous
leurs ex æquo ; les réservoirs sont des sous-ensembles sans répétition.

**Repère et largeur** (`bfs:213–273,420–447,674`; `driver:81–102`). À la racine,
`max(site)+1` est calculé en i64. Ensuite, `Close` connaît l'enveloppe entière
de la liste gardée et la boîte ajustée ; `adj_lo≥env_min`, d'où le calcul de
`parent_frame_bits` même lorsque des sites sont hors de la boîte. La boîte
fermée d'un enfant reste dans celle du parent. `filter_bits` transmet donc
le domaine nécessaire aux sites, témoins et bornes effectivement lus.

Avec M=2^s, chaque différence x−lo ou x−hi est de magnitude <M. La clé du
réservoir est <12M², donc de budget 2s+4 ; le filtre a le budget 2s+3. Ainsi
i64 est sûr à s≤29 ; à s≤33 les calculs i128 ont une marge très large. Le
passage s30 est nécessaire : le témoin `(2^30−1)^3` face à [0,1]^3 donne
la clé **13 835 058 016 627 458 075**, supérieure à `INT64_MAX`. La sonde
retrouve cet entier par la voie large. Une boîte fermée u32 peut demander
s33, même si tous ses sites sont des u32.

**Compactage et propriétaire** (`bfs:603–678,811–836`). Les préfixes des
comptes de tâches, puis des masques de voies, gardent exactement l'ordre
parent. L'enveloppe intersectée et les cas vides suivent la v11. Axe au
premier maximum et milieu plancher partagent la boîte demi-ouverte sans
recouvrement. Pour une boule critique admissible, le centre appartient à
l'enveloppe de son support ; le resserrement ne le perd pas. Le contrôle des
77 boules vérifie aussi toute sa population, pas seulement son support.

**Profondeur, chemins et refus** (`bfs:179–209`; `driver:110–115,173–211`).
Chaque coupe diminue Σceil(log2 largeur) d'au moins un ; l'ajustement ne
l'augmente pas. Les largeurs initiales sont ≤2^B, donc profondeur ≤3B, dont
96 pour u32. La fermeture s33 ne remplace pas B32 dans cette preuve. Deux
mots de chemin suffisent à ces 96 décisions. La garde du pilote teste la
profondeur avant le niveau suivant. Le refus `wide_leaf` observé efface les
comptes de sortie et le grand livre ; les tampons temporaires ne sont pas une
sortie publiée. Le nombre de nœuds et la mémoire du front restent des
questions séparées.

**Compteurs** (`bfs:550–594,603–809`). Une tâche fait au plus 256×36=9 216
tests, donc son compteur u32 ne déborde pas. Sur un niveau admis avec moins
de 2^32 tâches, tests<2^46 et candidats<2^40. Sur au plus 97 niveaux, les
tests restent <2^53. Les préfixes par enfant restent bornés par sa liste,
elle-même plus petite que 2^32 sites. Ces bornes justifient les sommes du
parcours valide ; elles ne remplacent pas les gardes de conversion et de
capacité examinées dans le volet racine, ni la validation d'un fichier tiers.

## Morton et suivi des constats

Le README de MES-M5 §9 précise maintenant la bonne distinction : comparaison
du front à **ordre SiteIdx identique**, comparaison du produit normalisé au
niveau de Cat_K. Le témoin de six sites de notre précédent reçu est ici
rejoué dans le vrai parcours hôte, au seuil valide 4 : les deux fronts
diffèrent géométriquement, chacun est exact et conserve toutes les boules
critiques de K1. Cela répond au point « réservoir » de **CST-0113**. Le lecteur
de transition FULL/supports/cover reste un autre travail.

Les portes de **CST-0204** (s33), **0205** (profondeur 63/96) et **0208**
(réservoir s30) réussissent dans ce microbanc hôte. Portée proposée au
registre : preuves d'implantation de microbanc ajoutées, **sans clôture du
produit** et sans présumer les résultats CUDA à venir. Aucun constat nouveau
au-delà des défauts de capacité examinés séparément.

## Rejeu

`builds.json` garde les deux commandes C++ et les hashes des binaires, placés
hors dépôt. `sources.json` épingle les sources relues. Le script vérifie
sources/pin/manifeste/sonde/binaires avant/après, avec erreurs explicites sous
Python normal et −O. Il régénère puis détruit les petits vidages synthétiques.

```sh
python3 check.py --identity /tmp/ehgp-v12-audit-b-m5-math-bin-20261007/traversal_identity \
  --probe /tmp/ehgp-v12-audit-b-m5-math-bin-20261007/predicates
python3 -O check.py --identity /tmp/ehgp-v12-audit-b-m5-math-bin-20261007/traversal_identity \
  --probe /tmp/ehgp-v12-audit-b-m5-math-bin-20261007/predicates
```

`normal.json` et `optimized.json` sont identiques, SHA-256
`e5853226a88600aacbb7379bd497958df5555e628bfa7ef43f272fc7d74605c9`.
Les chemins temporaires sont réduits au nom du fichier et les temps du banc
retirés ; ce reçu ne revendique aucune mesure de performance. `SHA256SUMS`
clôt les artefacts. Aucun commit/push.
