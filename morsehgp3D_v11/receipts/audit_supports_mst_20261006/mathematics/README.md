# Supports associés au MST : le rôle du plateau ne suffit pas

Contre-exemple exact au WIP supports v2 basé sur `9eee2ed4bcef1e960cdf2456012b84416854dc20`, capturé deux fois sans changement. La décision utilisateur « seulement les supports associés au minimum spanning tree de niveau K » et S* seul est acceptée. Le problème concerne la sélection `role!=internal`, pas la réduction à S*.

K=1, trois sites entiers en Morton croissant : `(0,0,0), (1,1,0), (1,0,1)`. Leurs trois boules de paire sont Gabriel, de rayon carré `1/2`, p=0, coquille2. FULL ferme ce plateau en une multifusion à trois enfants. Les trois boules ont donc le rôle `merge`, y compris la troisième arête qui ferme un cycle. Le filtre WIP garde trois supports ; Kruskal en retient deux.

Correction proposée : pour chaque nœud de multifusion, DSU locale sur ses enfants à la coupe ouverte ; traiter les boules dans l'ordre `(rang,S*)`, relier leurs `prior` par étoile déterministe et garder S*(b) seulement si au moins une union réussit. Garder les naissances. Un support q3/q4 peut être associé à plusieurs arêtes Kruskal : la liste de supports n'est pas elle-même une liste d'arêtes binaires.

```sh
python3 -S -B replay.py --repo /workspaces/E-HGP
python3 -O -S -B replay.py --repo /workspaces/E-HGP
sha256sum -c SHA256SUMS
```

Normal/−O code0, sorties identiques, 30 contrôles ; aucun essai en échec. La sortie constate l'échec du contrat de sélection WIP tout en validant la contre-preuve portable. Aucun produit natif, build, GPU ou GCP exécuté. Voir `REPORT.md`.
