# Audit — `two_lines` rend les propositions W4 quadratiques, pas la sortie q4

Date : 15 août 2026.

## Verdict

Le tableau du commit `5ce2634` ne mesure pas le nombre de tétraèdres q4 produits. Il mesure, lane par lane, le nombre `V_q` de **paires-ancrages W-vivantes**. Sur `two_lines`, `V_4 = Theta(n^2)`, mais la source géométrique q3/q4 est vide : aucun triangle porteur strictement aigu n'existe, donc aucun q4 bien centré n'existe.

La formulation « q4 croît quadratiquement » est donc trompeuse. La formulation exacte est :

> Le broad phase pair-level `W4Depth` peut conserver `Theta(n^2)` propositions d'ancres alors que le nombre de seeds q4 et de supports q4 vaut zéro.

Renommer les compteurs :

```text
V2_pair_walive
V3_pair_walive
V4_pair_walive
```

et réserver les noms q4 aux étages suivants :

```text
C4_carrier       # triples porteurs aigus possédés
M4_apex          # joins carrier--apex avant positivité
W4_positive      # q4 bien centrés
H4_rank          # q4 ayant passé rang/census
```

## Ce que `two_lines` prouve réellement

Pour `A_i=(i,0,0)` et `B_j=(0,j,H)`, les `m^2` paires croisées admettent une sphère vide compatible avec l'ancre. Aucun certificat universel sound fondé seulement sur la profondeur du fuseau de la paire ne peut donc les fermer.

Mais, pour `i<k`,

```text
(A_k-A_i) · (B_j-A_i) = -(k-i)i < 0,
```

si bien que `A_i A_k B_j` est obtus en `A_i`. Il n'existe aucun porteur aigu q3/q4. Par conséquent :

```text
V4_pair_walive = Theta(n^2)
C4_carrier     = 0
M4_apex        = 0
W4_positive    = 0
H4_rank        = 0
```

Cette famille est donc une réfutation d'une **proposition par paires matérialisée**, pas une borne inférieure quadratique sur l'objet q4.

## Le vrai verrou mathématique

Le verrou est de tuer ou de conserver **symboliquement** la masse des paires W4-vivantes avant toute expansion en `PairId`.

La bonne source q4 change d'arité :

1. garder la partition de paires comme tape neutre factorisé ;
2. chercher directement un seed possédé `(a,b,x)` où `ab` est l'arête maximale et `abx` est strictement aigu ;
3. utiliser le lemme : tout q4 bien centré d'arête maximale `ab` possède au moins une des deux faces adjacentes `abx`, `aby` strictement aiguë ;
4. choisir le porteur primaire par `PointId`, puis énumérer les seuls centres de profondeur au plus sept sur la ligne du plan médiateur associée au seed ;
5. appliquer ensuite owner-six-arêtes, positivité barycentrique, census et rang.

Sur `two_lines`, l'étape 2 rend immédiatement zéro sans créer les `m^2` tâches singleton, à condition que le test `NONE_ACUTE` soit lui-même disponible au niveau bloc ou que le tape pair-level soit consommé par une jointure factorisée.

## Proposition constructive à privilégier

La route `Q4SeedAxisTopR4` est précisément adaptée au problème. Pour chaque seed `(a,b,x)`, les autres sites définissent des roots sur une ligne du plan médiateur. Un q4 de rang pertinent ne peut apparaître qu'aux premiers/derniers groupes nécessaires avant d'atteindre huit intérieurs. Il faut donc sélectionner les extrêmes shallow, et non former tous les couples `carrier x apex`.

Pour `r4=8`, la masse de groupes proposée est au plus linéaire dans le nombre de seeds :

```text
root_groups <= 2 * r4 * M4_seed.
```

Cela supprime le terme artificiel

```text
sum_e binom(m_e, 2)
```

qui était le produit quadratique des propositions pour une arête.

Ce mécanisme ne prouve pas que la sortie q4 est toujours sous-quadratique : il existe peut-être des familles ayant réellement beaucoup de q4 bien centrés et shallow. Aucune telle famille n'est fournie par `two_lines`, et aucune conclusion de ce type ne doit être tirée du compteur `V4_pair_walive`.

## Gates demandées

Ajouter une porte `two_lines_q4_stage_separation` exigeant simultanément :

```text
V4_pair_walive >= c * n^2
C4_carrier == 0
M4_apex == 0
W4_positive == 0
H4_rank == 0
```

et une porte mémoire exigeant qu'aucun tableau de taille `V4_pair_walive` ne soit alloué. Le tape doit rester factorisé jusqu'au verdict `NONE_ACUTE`.

Ajouter également un nuage positif distinct, construit pour avoir des q4 bien centrés shallow, puis publier les cinq compteurs. Sans cette séparation, le mot `q4` continuera de désigner tantôt une paire, tantôt une face, tantôt un tétraèdre. C'est une manière remarquablement efficace de rendre toute pente vraie et fausse à la fois.

## Conclusion à transmettre

Le résultat quadratique actuel n'est **pas** une fatalité géométrique de q4. Il localise le défaut : la proposition par ancre de paire est trop large. Le prochain gain ne viendra pas d'un cinquième certificat de profondeur W4 légèrement meilleur, puisque `two_lines` rend ce plancher lui-même quadratique. Il faut déplacer la frontière de matérialisation vers les seeds aigus possédés et traiter les niveaux shallow sans produit de complétions.
