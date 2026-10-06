# Porte API supports : compteur corrigé en source pendant la revue

La comparaison `balls <= cells` lue au début de la revue était fausse : les naissances publiées ne figurent pas dans le journal des cellules. Le développeur l'a retirée pendant la capture. **L'état figé ici est corrigé en source**, sans patch supplémentaire et sans qualification native.

`tests/api/supports_route_oracle.py` et `src/supports/hierarchy.cpp` WIP ont été lus deux fois avec octets identiques au-dessus de `9eee2ed4bcef1e960cdf2456012b84416854dc20` ; hashes dans `sources.json`. L'ancien fichier n'est pas double-capturé dans ce complément : aucun ancien hash n'est attribué. Sa comparaison isolée est conservée uniquement avec cette limite explicite.

Le rejeu appelle l'oracle exact S1 épinglé sur **une seule fixture de trois sites**, `triangle_equilateral` déjà présente dans `reference/test_supports.py:78` et sélectionnée à K2. Ses points sont `(0,0,0), (2,2,0), (2,0,2)` :

- trois boules de naissance régulières q2, rayon carré2, p0, m2 ;
- une boule de fusion q3, rayon carré8/3, p0, m3, prior `[0,1,2]` ;
- quatre boules et quatre S* publiés ; le journal réserve une cellule et trois graines.

Le journal épinglé `src/tower/order_tree.cpp:18–21` omet une boule régulière lorsque `K=p+qmin` : les trois naissances sont omises, la fusion est comptée. `tests/api/supports_route.cpp:306` publie précisément `diagnostics.log.cells`, et `src/tower/attachment.cpp:61–78` traite les naissances séparément. Le nombre de boules publiées ne se compare donc pas directement à ce champ. Une comparaison avec les naissances distinctement comptées demanderait un champ absent de la ligne actuelle ; sa suppression est ici la correction minimale.

L'expression réelle du juge capturé, extraite par AST, accepte ces comptes et conserve `S=B`, `multiple=0` et les contrôles positifs. Des mutations `supports=3` ou `multiple=1` restent refusées. `bytes=1` sert uniquement de sentinelle positive pour cette expression Python ; ce n'est pas une taille native prétendument mesurée. La comparaison précédente isolée renvoie faux (`4 <= 1`).

```sh
python3 -B replay.py --repo /workspaces/E-HGP
python3 -O -B replay.py --repo /workspaces/E-HGP
sha256sum -c SHA256SUMS
```

Les deux lectures sont identiques. Les dépendances sont vérifiées par un seul `git archive` local, sans recopie du catalogue ni calcul de la suite complète. Aucun build, test natif, GPU, GCP ou octet LiDAR. Ce reçu clôt uniquement l'attente erronée du compteur à la lecture de source ; il ne clôt ni la sélection Kruskal, ni les versions du dossier, ni une future campagne de publication API.
