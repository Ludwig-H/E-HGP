# Contrelecture mathématique S6b — 5 octobre 2026

Source Git figée : `9e7428995e3b359301d58d882610d9d4ee720fad`, assemblage S6b.
44 fichiers sont épinglés par SHA256 dans `source_manifest.json`. Les rapports
`impl_s6b.md` et `verif_s6b.md` ont été lus avant la contre-épreuve ; leurs copies
et empreintes sont conservées. Aucun nouveau défaut mathématique important établi.

En lecture, le postordre suit les enfants croissants ; le remplissage inverse
des fins de seaux conserve les BallIdx croissants dans chaque nœud. Count et fill
désignent la même clé ; les comptes sont contrôlés contre les traces du journal.
Fill garde tous les supports Q_b, y compris ceux sans coface, et recopie les rôles,
propriétaires fermés et branches de S3. Le différentiel complet compare effectivement
tous les supports et leurs comptes à l'étage A indépendant, après un contrôle séparé
de l'ordre natif.

## Ordre fichier : argument autonome

Le catalogue compare les niveaux exacts puis les tableaux S* rembourrés de kNone
(`src/catalogue/assemble.cpp:14`, `sort_indices.cpp:27`).
Si T est une partie propre d'un support positif minimal Q de centre c, alors
c est hors de conv(T). Une séparation stricte fournit v avec
v·(x−c)>0 pour chaque x dans T. Déplacer c vers c+εv, pour ε>0 assez petit,
diminue strictement toutes les distances carrées : la MEB de T a donc un niveau
strictement inférieur à celui de Q. Deux S* distincts de même niveau ne peuvent
être préfixes l'un de l'autre. Le rembourrage par kNone ne change donc pas leur ordre
lexicographique. L'ordre (postordre, rang, BallIdx) de S6b coïncide avec l'ordre
(postordre, rang, S*) prescrit ; aucun verrou nouveau sur ce point.

## Nouvelle contre-épreuve portable bornée

`check_assembly.py` traduit en Python le nouveau postordre sans pile et les seaux
stables, puis les compare à un tri direct et aux documents S1 exacts. Les familles
Q_b et les arbres proviennent de S1 ; le modèle recalcule les coquilles exactes,
fermetures et comptes, préserve leurs dates, et teste les préfixes des instantanés
de chaque nœud vivant. Il construit ensuite des JSON synthétiques aux conventions
du nouveau juge `hierarchy_fraction.py`.

Couverture : six nuages, huit ordres (carré K1/K3, passagère K1, équilatéral faible K2,
cube K1/K2, croissance ABCZ K3, cercle perturbé K2), 60 boules, 86 supports,
19 coquilles étendues, 18 boules à plusieurs supports, quatre tétraèdres.
Les huit contrôles passent (318 contrôles du comparateur). Six altérations ciblées
sont rejetées par code 1 : ordre propre inversé, tétraèdres à zéro coface omis au
cube K1, incidences substituées aux cofaces distinctes, boule interne tardive omise,
passagère classée interne, taille de sous-arbre fausse. Les premiers messages
exactement obtenus figurent dans les JSON.

Normal et -O donnent des résultats identiques, ainsi que les deux rejeux depuis
le pin Git :
```sh
python3 replay.py
python3 replay.py --optimized
```
SHA256 des quatre JSON :
`5e6482f3d035deb496b07200bb8a9066ec76f61cd3f86f76131473f872b5d4e2`.
Commandes et codes sont dans `executions.json` ; aucune tentative en échec dans
cette capture. Le rejeu ne dépend pas du worktree du développeur ni du snapshot local.

Ce modèle ne lance pas le produit C++ et ne prouve pas l'absence d'un écart entre
sa traduction et le binaire. Les tests natifs cités par le développeur restent
ses exécutions, distinctes de cette preuve. Aucun build, test natif ni GCP exécuté,
aucune répétition de la suite complète de 963 ordres. Phase
exploration_v11_hors_registre, backend cpu_reference,
profile quantized_u21_input_only, public_status not_claimed.
