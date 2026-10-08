# Census : raffinement certain par un témoin de frontière

8 octobre 2026. Contrelecture mathématique du prototype **hors produit**
T2d_B/`var_temoin`, base `8dc5d6b16`. Aucune compilation, sonde, donnée réelle ou
commande distante exécutée/lue par cet audit. Le delta index capturé ici n'est
pas une livraison et n'est pas raccordé à G à cet instant. Il ajoute les témoins
au census emprunté `CensusWorkspace`, pas au census possédé en deux passes.

## Lemme et portée exacte

Soit un nœud portant une plage de SiteIdx `[begin,end)` et une boîte qui contient
ses sites. Soit `w` dans cette plage, dans **le même Cloud**, avec puissance
exacte `power(w)=0` pour la sphère recensée. Alors le minimum de puissance sur
les points entiers de la boîte est ≤0 et son maximum est ≥0. Les deux décisions
de raccourci du parcours, `lower>0` (extérieur) et `upper<0` (intérieur strict),
sont donc impossibles. On peut directement descendre, ou tester tous les points
de la feuille, exactement comme le parcours ordinaire.

Par induction sur `(cursor,p,m)`, mêmes nœuds visités, mêmes tests de sites,
mêmes blocs intérieurs/extérieurs, même premier préfixe de K intérieurs en cas
de saturation, mêmes I/U complets sinon. La coquille du support n'est jamais
comptée comme intérieur ; même une coquille non régulière ou un singleton garde
le même traitement. Une saturation antérieure à la visite d'un témoin ne change
pas l'argument. Le lemme utilise la frontière exacte ; il ne dépend pas de la
propriété MEB ni de la nouvelle taille de pavé.

Les signes produits par `GuardedBounds` suffisent : son minorant est celui du
minimum entier ; un majorant artificiellement positif d'une boîte partielle ne
permet pas non plus le raccourci intérieur. L'index est réellement un arbre de
plages de **SiteIdx Morton**, pas de PointId originaux (`index/index.hpp:18`).

Nuance : le prototype ne considère pas le témoin comme une preuve pour accepter
ou rejeter des points ; il force seulement un raffinement. **Même un marqueur
erroné reste géométriquement conservateur dans ce corps précis**, puisque les
points sont finalement évalués. Il peut augmenter le travail. La validité et
l'appartenance au même Cloud sont donc nécessaires à la promesse d'identité du
parcours, sans inventer une faute géométrique quand cette précondition manque.
Un futur raccourci supprimant aussi les tests ponctuels aurait une autre preuve.

## Raccord et mesure à conserver

`CertifiedBall::certify` garantit que les 1–4 sites utilisés sont sur la sphère.
`tower/resolve.cpp` dispose déjà de `Certified.support/arity`, liés aux points du
`Domain`. Le `Located` actuel ne conserve ensuite que la boule : le raccord doit
transporter ces SiteIdx avec leur arité par valeur jusqu'à la requête, sans
référence pendante au certificat local. Ne pas prendre les IDs de la seule
proposition flottante avant certification. Cette adaptation n'est pas incluse
dans le delta capturé. Les sources de ces préconditions sont épinglées.

Pour q≤4 témoins, le test de plage coûte O(q) par nœud, sans allocation par
requête ; aucun gain asymptotique du census n'est revendiqué. Il peut éviter des
évaluations exactes mais ajoute des comparaisons sur tous les nœuds visités.
Le span emprunté et ses supports doivent vivre pendant `query` ; les états de
parcours restent propres au workspace comme auparavant. Pas de stockage de
résultats supplémentaire requis par le mécanisme présenté.

`ledger.bounds` est incrémenté avant le test du témoin : il reste ici un compte
des nœuds interrogés, **pas des appels effectifs à `bound_signs`**. Le nouveau
`guard_witness` permet de distinguer les appels évités ; `LaneCount` et les
autres compteurs de garde peuvent varier. Les nœuds/sites et les sorties restent
identiques avec les vrais supports. Garder les bras garde seule/témoin/combiné
distincts. Les −49 % d'évaluations annoncés par le microbanc local ne sont ni un
gain de temps G/FULL, ni une qualification G4. Son minimum des répétitions et
son FNV64 du flux genre/I/U sont des diagnostics, pas un oracle d'identité complet.

Le prototype conserve exactement le [resserrement déjà prouvé](../garde_census/README.md)
dans `guard.cpp` SHA `87ca932d…` : seules les deux bornes changent, pas `s+2`,
`6s+11` ou le choix des voies. Aucun transfert implicite aux promotions de voie,
ni au census des feuilles CUDA. Aucun nouveau constat ou état fermé.

## Modèle borné et pins

`model.py` reconstruit par Gram rationnel sept fixtures synthétiques q1–q4,
coquilles étendues, singleton, saturation et coordonnées proches du domaine u32.
Il construit un arbre de plages Morton et oppose le parcours à un **oracle
indépendant évaluant chaque site**. Quatre tailles de feuilles, plusieurs seuils,
tous les sous-ensembles de support, ordre inversé et répétitions de marqueurs.
Ce domaine fini ne se prétend pas exhaustif sur les nuages.

Résultat : **132 requêtes de référence, 1 288 avec témoins valides**, identité
exacte résultat/parcours ; quatre cas saturent avant un test du support.
1 084 essais de marqueurs arbitraires gardent les résultats, dont 168 modifient
le travail. Deux mauvaises classifications (témoin ⇒ intérieur ou extérieur)
sont rejetées par l'oracle. Les 3 631 évaluations de boîte évitées sont un compte
de ce modèle choisi pour exercer les frontières, pas une estimation LiDAR.

Le delta de deux fichiers index est conservé sous `prototype_index.diff` :
`census_workspace.cpp` SHA `7d3de9fd…`, `index.hpp` SHA `c8e34dd2…`.
Le lecteur reconstruit ces octets depuis Git en dossier temporaire, vérifie les
pins et les sources des préconditions ; aucun produit ou prototype n'est modifié.

```sh
python -B model.py --repo DEPOT
python -B -O model.py --repo DEPOT
```

Normal/−O identiques. Ce modèle n'exécute pas les corps C++ et ne remplace pas
leurs portes natives, leurs tests de durée de vie ni le mur G/FULL à mesurer.
