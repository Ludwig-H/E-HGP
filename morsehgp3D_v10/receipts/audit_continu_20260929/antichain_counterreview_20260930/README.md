# Témoins redondants : effet réel et calcul sans tri

30 septembre 2026. Contre-audit du [prototype indépendant](../../audit_independant_20260930/fixed_k_antichain/README.md).
Moteur inchangé, Python exact seulement, aucun nouvel appel natif ou GCP.

La réduction des ancêtres redondants conserve les preuves à K fixé.
Nos six exports archivés avancent dix entrées dans les contextes testés,
mais **aucune des 434 hauteurs de paires** contrôlées. Il ne faut donc pas
déduire un meilleur clustering de ces seules entrées. Le triangle original
est causal : retirer son seul témoin ancestral redonne l'entrée réduite ;
le conserver dans l'antichaîne rétablit le retard. Cette suppression est
un diagnostic en mémoire, pas un univers alternatif complet.

## Un changement de partition au paramètre par défaut

Quatre points `(0,0,0), (10,0,0), (8,4,0), (4,8,0)`, K2, η=1/8.
Γ2 est reconstruit exhaustivement : six paires, quatre unions de trois
points, MEB Fraction exactes. Le catalogue critique complet fournit cinq
boules fortes, toutes leurs incidences I/U, sans supprimer les fusions
FULL de `p+q_min=K+1`.

Le point 0 entre d'abord à β=20 ; sa bande finit à 405/16. Le témoin
ancestral à β=25 appartient à la bande, mais n'ajoute aucune lignée.
Le bras initial attend 25 ; le bras réduit entre à **200/9**. La réunion
des points 0 et 3 est réellement avancée : à cette coupe fermée, les
partitions sont respectivement `{0}|{1,2}|{3}` et `{0,3}|{1,2}`.
Le point 2 reste sur une autre branche : ne pas remplacer la deuxième
partition par `{0,2,3}|{1}`.

La limite des six exports n'était donc pas une impossibilité structurelle.
Ce cas plan plongé en 3D prouve un changement d'objet, **pas** une amélioration
EOM/ARI ni un effet sur LiDAR. La tête doit d'abord corriger les départs
différés et `min_cluster_size` ; aucun test statistique n'est emprunté ici.

## Calcul exact sans trier les témoins de chaque point

Préparer les intervalles Euler `[tin[v],tout[v])` de la forêt. Pour les
nœuds sélectionnés S d'un point, même avec répétitions, prendre

```text
l = argmin_S (tout[v], -tin[v])
r = argmax_S tin[v]
J = LCA(l,r)
```

Ces deux nœuds sont les extrêmes de l'antichaîne minimale A. Un ancêtre
sélectionné ne peut gagner le minimum : son descendant a un tout plus
petit, ou le même tout et un tin plus grand. Sur les minima incomparables,
l'ordre des fins est celui des débuts : l est le premier, r le dernier.
Le LCA des extrêmes contient tous les minima intermédiaires. Ainsi
`J=LCA(A)` sans construire A. Le départage par tin décroissant est
indispensable sur une chaîne suivant les derniers enfants.

Après préparation Euler O(V), un balayage O(D) des incidences déjà
disponibles suffit, avec **au plus une LCA par point** ; plus de tri
`Σ d_x log d_x` ni de stockage des antichaînes. Mémoire ajoutée O(V+n),
hors index LCA et univers préexistant. Le coût et la mémoire propres à
cet index restent à publier. Cela ne borne ni D ni la génération FULL.

Les deux extrêmes sont des réductions associatives : des tâches parallèles
peuvent les combiner par point. Comparer tin en ordre décroissant, pas
le négatif d'un u32 converti en i32 ; une clé 64 bits `(tout,UINT32_MAX-tin)`
demande ses propres gardes de cardinalité. Garder les seuils exacts, toutes
les incidences, les plateaux fermés et le propriétaire vivant. Racines
distinctes : refus ou règle explicite +∞, jamais une fusion inventée.
La bande complète doit toujours être lue avant décision.

Contrôles : 482 contextes/2 892 bandes contre l'antichaîne pairwise et
les attaches initiales ; 6 227 sélections supplémentaires, arbres unary,
non binaires, doublons, inversions ; 4 086 contrôles à racines distinctes.
Normal/−O concordent. Un contre-test supprime le départage et retrouve
causalement le mauvais ancêtre. C'est un contre-test logique, pas un
mutant compilé du moteur.

## Reçus et portée

[receipt.json](receipt.json) capture les deux contre-relectures des six
exports ; [additional_receipt.json](additional_receipt.json) capture
géométrie et extrêmes, quatre nouveaux appels Python, avec scripts et
dépendances épinglés avant/après. Les chemins ont été rendus relatifs
avant ces nouvelles captures ; les archives sources restent inchangées.

`python3 -B verify.py` et `python3 -B -O verify.py` vérifient d'abord les
empreintes, puis rejugent les trois sondes. Aucun compilateur, générateur,
HDBSCAN, profilage ou chrono LiDAR/G4 ; pas de gain de débit qualifié.
