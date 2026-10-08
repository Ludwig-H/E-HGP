# Proposition avant banc : réemploi du tri des positions pour la table S*

8 octobre 2026. `phase=exploration_v12_hors_registre`, `public_status=not_claimed`.
[Patch unique](proposition.patch) contre **9feadf927**, `src/catalogue/finish_driver.hpp` (`4d15554c…`),
[pins complets](pins.json). Proposition non appliquée au produit, non compilée, sans mesure ni qualification GPU.

`FinishArrays::table` et sa réservation sont supprimés ; les références à ce seul membre utilisent `positions`.
`table_offset`, `pkey`, le tri F3 `keys`, les sorties et l'indice dynamique `ct` sont conservés. Aucun nouveau tampon.

La durée de vie autorise ce réemploi dans le chemin lu :

1. `finish_order:107–115` trie les positions ; `GatherKeyKernel` lit `positions.vals[cp]` pour remplir `keys`, puis
   le tri F3 se termine et `b.sync()` clôt cette utilisation. Le résultat utile est dans **keys**, séparé.
2. Vérification et repli exact (`finish_check`/`finish_repair`) utilisent keys, levels et **pkey** ; aucun ne lit
   `positions`. `finish_emit` utilise aussi keys et termine par une synchronisation.
3. `TableKeyKernel` réécrit les n clés et n valeurs du tampon 0 ; `radix_sort` réinitialise bornes/histogrammes,
   repart du tampon 0 et rend **ct**, qui dépend des octets réellement triés. `TableOffsetKernel`, puis `take_cast`,
   doivent suivre cet indice. La table n'est pas systématiquement dans le tampon 0.
4. Les sorties CPU `take_cast` et CUDA sont des **copies**, donc un appel suivant ne modifie pas la table déjà
   rendue. `finish_reserve` garantit à nouveau les deux clés/valeurs ; chaque nouvelle entrée réécrit son préfixe
   [0,n), même après une prise plus grande ou un refus entre phases. Les capacités peuvent rester résidentes.

Le contexte est utilisé **séquentiellement**, conformément au chemin existant. La proposition n'autorise pas deux
appels simultanés sur un même `CatalogueDevice`. Un défaut du pilote CUDA marque le contexte perdu ; l'alias ne
transforme pas ce défaut en reprise autorisée. Un refus de ressources réutilisable suit la réinitialisation habituelle.

Revue de port seulement, **C/repo902 non modifié** : `finish_take` transmettrait `positions.vals[ct]` comme segment ;
`stream_out` attend les copies avant retour et draine celles en vol en cas de refus. Les sources C sont épinglées
séparément ; le patch ci-joint ne leur est ni appliqué ni automatiquement qualifié.

Pour n boules et t=max(1,ceil(n/1024)), les demandes de `RadixArrays<Key2>` retirées sont :

- deux tableaux de clés 16 octets et deux de valeurs 4 octets : **40n** ;
- histogrammes par tuile (256×4) et deux bornes 16 octets : **1056t** ;
- totaux des 256 cases et résultat de deux clés : **1056**.

Soit **40n + 1056t + 1056 octets demandés logiquement**. Ce n'est pas l'allocation physique : CUDA arrondit les
capacités (minimum 1024 éléments au premier agrandissement) et les conserve ; le pic global inclut les autres
étapes, les copies et les sorties. Aucune économie de pic, délai ou quota chiffrée n'est promise.

[check.py](check.py), normal/−O : application et empreinte du patch sur copie temporaire, contrôle inverse ; modèle
Python indépendant de tri stable par octets, comparé à `sorted` et à deux espaces distincts. **227 appels aboutis,
92 interruptions entre phases**, toutes parités cp/ct, tailles variables de 0 à 1025 ; sorties antérieures et clés
séparées conservées. Le témoin `ct=1` rend la table `[1,0]`. Les cas 0/1 portent sur les tris abstraits, pas sur
l'admission d'un catalogue vide. Ce modèle ne qualifie ni noyaux, allocateur, refus CUDA réels, ni concurrence.
[Résultats](results.json) identiques dans les deux modes.

```sh
python check.py --repo /chemin/vers/E-HGP
python -O check.py --repo /chemin/vers/E-HGP
```

Avant adoption : comparer les sorties complètes du catalogue et la table S* avant/après, avec parités différentes,
clés aux octets communs, repli exact, séquences résidentes petit/grand/petit, sorties précédentes encore vivantes,
puis refus de budget et reprise. Rejouer CPU et CUDA sur les sources exactes ; publier mémoire réellement allouée
et pic séparément. Aucun gain ne découle du seul retrait de la demande.
